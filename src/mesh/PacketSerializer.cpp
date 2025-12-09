#include "PacketSerializer.h"
#include <sstream>

namespace mesh {

std::string PacketSerializer::serialize(const Packet& p) {
    std::ostringstream oss;
    oss << static_cast<int>(p.type) << "|"
        << p.message_id << "|"
        << p.timestamp_ms << "|"
        << p.sender_id << "|"
        << p.destination_id << "|"
        << p.payload << "\n";
    return oss.str();
}

static bool splitFields(const std::string& s, char delim, std::vector<std::string>& out) {
    std::string current;
    for (char c : s) {
        if (c == delim) {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(current);
    return true;
}

bool PacketSerializer::deserialize(const std::string& line, Packet& out) {
    std::string trimmed = line;
    if (!trimmed.empty() && trimmed.back() == '\n') {
        trimmed.pop_back();
    }

    std::vector<std::string> parts;
    splitFields(trimmed, '|', parts);

    if (parts.size() < 6) return false;

    try {
        out.type = static_cast<PacketType>(std::stoi(parts[0]));
        out.message_id = std::stoull(parts[1]);
        out.timestamp_ms = std::stoull(parts[2]);
        out.sender_id = parts[3];
        out.destination_id = parts[4];
        out.payload = parts[5];
    } catch (...) {
        return false;
    }
    return true;
}

} // namespace mesh

