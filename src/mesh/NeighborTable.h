#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace mesh {

struct NeighborInfo {
    std::string node_id;
    std::chrono::steady_clock::time_point last_seen;
};

class NeighborTable {
public:
    void updateNeighbor(const std::string& node_id);
    std::unordered_map<std::string, NeighborInfo> snapshot() const;

private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, NeighborInfo> neighbors_;
};

} // namespace mesh

