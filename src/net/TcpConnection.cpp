#include "TcpConnection.h"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

namespace net {

TcpConnection::TcpConnection(int socket_fd,
                             const std::string& self_id,
                             const std::string& peer_id)
    : sock_(socket_fd), self_id_(self_id), peer_id_(peer_id) {}

TcpConnection::~TcpConnection() {
    running_ = false;
    if (recv_thread_.joinable()) recv_thread_.join();
    if (sock_ >= 0) close(sock_);
}

void TcpConnection::setPacketHandler(PacketHandler handler) {
    handler_ = std::move(handler);
}

void TcpConnection::start() {
    running_ = true;
    recv_thread_ = std::thread([this]() {
        std::string buffer;
        char chunk[256];
        while (running_) {
            ssize_t n = recv(sock_, chunk, sizeof(chunk), 0);
            if (n <= 0) break;

            buffer.append(chunk, n);
            // process line-based packets
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos + 1);
                buffer.erase(0, pos + 1);

                mesh::Packet p;
                if (mesh::PacketSerializer::deserialize(line, p)) {
                    if (handler_) handler_(p);
                }
            }
        }
        running_ = false;
    });
}

void TcpConnection::sendPacket(const mesh::Packet& packet) {
    std::string wire = mesh::PacketSerializer::serialize(packet);
    ssize_t sent = send(sock_, wire.data(), wire.size(), 0);
    if (sent < 0) {
        std::cerr << "[TcpConnection] send failed\n";
    }
}

} // namespace net

