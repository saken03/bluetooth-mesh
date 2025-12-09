#include "NeighborTable.h"

namespace mesh {

void NeighborTable::updateNeighbor(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    neighbors_[node_id] = NeighborInfo{
        node_id,
        std::chrono::steady_clock::now()
    };
}

std::unordered_map<std::string, NeighborInfo> NeighborTable::snapshot() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return neighbors_;
}

} // namespace mesh

