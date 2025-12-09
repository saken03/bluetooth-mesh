#include "MessageRouter.h"
#include "MeshProtocol.h"
#include <chrono>

namespace mesh {

static uint64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

MessageRouter::MessageRouter(const std::string& self_id,
                             NeighborTable& neighbors,
                             TopologyManager& topology)
    : self_id_(self_id),
      neighbors_(neighbors),
      topology_(topology) {}

void MessageRouter::setReceiveCallback(ReceiveCallback cb) {
    on_receive_ = std::move(cb);
}

void MessageRouter::addLink(const std::shared_ptr<ILink>& link) {
    std::lock_guard<std::mutex> lock(mtx_);
    links_[link->peerId()] = link;
}

void MessageRouter::removeLink(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    links_.erase(peer_id);
}

void MessageRouter::handleIncoming(const Packet& p) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (seen_messages_.count(p.message_id)) {
        return; // already processed
    }
    seen_messages_.insert(p.message_id);

    neighbors_.updateNeighbor(p.sender_id);

    if (p.type == PacketType::MESSAGE && p.destination_id == self_id_) {
        if (on_receive_) on_receive_(p);
    } else if (p.type == PacketType::TOPOLOGY) {
        // payload expected as "node1:dist,node2:dist,..."
        std::unordered_map<std::string, int> neighbor_view;
        std::string token;
        for (size_t i = 0; i <= p.payload.size(); ++i) {
            if (i == p.payload.size() || p.payload[i] == ',') {
                if (!token.empty()) {
                    auto pos = token.find(':');
                    if (pos != std::string::npos) {
                        std::string node = token.substr(0, pos);
                        int dist = std::stoi(token.substr(pos + 1));
                        neighbor_view[node] = dist;
                    }
                }
                token.clear();
            } else {
                token.push_back(p.payload[i]);
            }
        }
        topology_.updateFromNeighbor(p.sender_id, neighbor_view);
    }

    // Forward everything except what we originated and what is for us
    if (!(p.type == PacketType::MESSAGE && p.destination_id == self_id_)) {
        forward(p);
    }
}

void MessageRouter::forward(const Packet& p) {
    for (auto& [peer_id, link] : links_) {
        if (peer_id == p.sender_id) continue; // basic loop control
        link->sendPacket(p);
    }
}

void MessageRouter::sendTextMessage(const std::string& dest_id,
                                    const std::string& text) {
    Packet p;
    p.type = PacketType::MESSAGE;
    p.sender_id = self_id_;
    p.destination_id = dest_id;
    p.payload = text;
    p.message_id = next_message_id_++;
    p.timestamp_ms = now_ms();

    // deliver locally if destination is self
    if (dest_id == self_id_ && on_receive_) {
        on_receive_(p);
    }

    forward(p);
}

} // namespace mesh

