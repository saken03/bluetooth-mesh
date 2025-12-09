#pragma once
#include <string>
#include <cstdint>

namespace mesh {

enum class PacketType : uint8_t {
    HELLO = 0,
    TOPOLOGY = 1,
    MESSAGE = 2
};

constexpr uint16_t MESH_PROTO_VERSION = 1;

// Simple constants
constexpr uint32_t TOPOLOGY_BROADCAST_INTERVAL_MS = 5000;
constexpr uint32_t HELLO_INTERVAL_MS = 8000;

// Max payload sizes (just sanity)
constexpr size_t MAX_NODE_ID_LEN = 64;
constexpr size_t MAX_PAYLOAD_LEN = 1024;

} // namespace mesh

