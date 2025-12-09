#pragma once
#include "Packet.h"
#include <string>
#include <vector>

namespace mesh {

// Very simple length-prefixed text format:
// type|msg_id|timestamp|sender|dest|payload\n
// Not efficient, but easy to debug.
class PacketSerializer {
public:
    static std::string serialize(const Packet& p);
    static bool deserialize(const std::string& line, Packet& out);
};

} // namespace mesh

