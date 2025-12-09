#pragma once
#include "MeshProtocol.h"
#include <string>
#include <cstdint>

namespace mesh {

struct Packet {
    PacketType type{};
    std::string sender_id;
    std::string destination_id; // empty for broadcast
    std::string payload;        // text, topology JSON, etc.
    uint64_t message_id{0};     // for deduplication
    uint64_t timestamp_ms{0};   // for info/logging
};

} // namespace mesh

