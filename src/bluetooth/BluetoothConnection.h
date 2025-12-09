#pragma once
#include "../mesh/ILink.h"
#include "../mesh/PacketSerializer.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

namespace bt {

class BluetoothConnection : public mesh::ILink,
                            public std::enable_shared_from_this<BluetoothConnection> {
public:
    using PacketHandler = std::function<void(const mesh::Packet&)>;

    BluetoothConnection(const std::string& self_id,
                        const std::string& peer_id);

    ~BluetoothConnection() override;

    std::string peerId() const override { return peer_id_; }

    void sendPacket(const mesh::Packet& packet) override;

    void setPacketHandler(PacketHandler handler);

    // TODO: add real socket setup here for Bluetooth
    void startFakeLoop(); // temporary stdin/stdout simulator

private:
    std::string self_id_;
    std::string peer_id_;

    PacketHandler handler_;
    std::thread recv_thread_;
    std::atomic<bool> running_{false};
};

} // namespace bt

