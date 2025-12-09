#pragma once
#include "../mesh/ILink.h"
#include "../mesh/PacketSerializer.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

namespace net {

class TcpConnection : public mesh::ILink,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    using PacketHandler = std::function<void(const mesh::Packet&)>;

    TcpConnection(int socket_fd,
                  const std::string& self_id,
                  const std::string& peer_id);

    ~TcpConnection() override;

    std::string peerId() const override { return peer_id_; }
    void sendPacket(const mesh::Packet& packet) override;

    void setPacketHandler(PacketHandler handler);
    void start();

private:
    int sock_;
    std::string self_id_;
    std::string peer_id_;
    PacketHandler handler_;

    std::thread recv_thread_;
    std::atomic<bool> running_{false};
};

} // namespace net

