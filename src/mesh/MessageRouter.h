#pragma once
#include "Packet.h"
#include "ILink.h"
#include "NeighborTable.h"
#include "TopologyManager.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <functional>

namespace mesh {

class MessageRouter {
public:
    using ReceiveCallback = std::function<void(const Packet&)>;

    MessageRouter(const std::string& self_id,
                  NeighborTable& neighbors,
                  TopologyManager& topology);

    void setReceiveCallback(ReceiveCallback cb);

    void addLink(const std::shared_ptr<ILink>& link);
    void removeLink(const std::string& peer_id);

    void handleIncoming(const Packet& p);
    void sendTextMessage(const std::string& dest_id,
                         const std::string& text);

private:
    std::string self_id_;
    NeighborTable& neighbors_;
    TopologyManager& topology_;
    ReceiveCallback on_receive_;

    std::mutex mtx_;
    std::unordered_map<std::string, std::shared_ptr<ILink>> links_;
    std::unordered_set<uint64_t> seen_messages_;
    uint64_t next_message_id_{1};

    void forward(const Packet& p);
};

} // namespace mesh

