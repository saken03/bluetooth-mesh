#pragma once
#include "Packet.h"
#include <string>

namespace mesh {

// Abstract connection to a neighbor (Bluetooth, TCP, whatever)
class ILink {
public:
    virtual ~ILink() = default;
    virtual std::string peerId() const = 0;
    virtual void sendPacket(const Packet& packet) = 0;
};

} // namespace mesh

