#include "BluetoothConnection.h"
#include <iostream>

namespace bt {

BluetoothConnection::BluetoothConnection(const std::string& self_id,
                                         const std::string& peer_id)
    : self_id_(self_id), peer_id_(peer_id) {}

BluetoothConnection::~BluetoothConnection() {
    running_ = false;
    if (recv_thread_.joinable()) recv_thread_.join();
}

void BluetoothConnection::setPacketHandler(PacketHandler handler) {
    handler_ = std::move(handler);
}

void BluetoothConnection::sendPacket(const mesh::Packet& packet) {
    // TEMP: just print to stdout. Replace with real Bluetooth send.
    std::string wire = mesh::PacketSerializer::serialize(packet);
    std::cout << "[SEND -> " << peer_id_ << "] " << wire;
}

void BluetoothConnection::startFakeLoop() {
    running_ = true;
    recv_thread_ = std::thread([this]() {
        std::string line;
        while (running_) {
            if (!std::getline(std::cin, line)) break;
            mesh::Packet p;
            if (mesh::PacketSerializer::deserialize(line, p)) {
                if (handler_) handler_(p);
            }
        }
    });
}

} // namespace bt

