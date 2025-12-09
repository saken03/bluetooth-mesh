#include "TopologyManager.h"
#include <sstream>

namespace mesh {

void TopologyManager::setSelfId(const std::string& id) {
    std::lock_guard<std::mutex> lock(mtx_);
    self_id_ = id;
    nodes_[id] = 0;
}

void TopologyManager::updateFromNeighbor(
    const std::string& neighbor_id,
    const std::unordered_map<std::string, int>& neighbor_view) {

    std::lock_guard<std::mutex> lock(mtx_);

    // neighbor is always distance 1 from self
    auto it = nodes_.find(neighbor_id);
    if (it == nodes_.end() || it->second > 1) {
        nodes_[neighbor_id] = 1;
    }

    for (const auto& [node, dist_from_neighbor] : neighbor_view) {
        if (node == self_id_) continue;
        int candidate = dist_from_neighbor + 1;
        auto it2 = nodes_.find(node);
        if (it2 == nodes_.end() || candidate < it2->second) {
            nodes_[node] = candidate;
        }
    }
}

std::unordered_map<std::string, int> TopologyManager::snapshot() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return nodes_;
}

std::string TopologyManager::prettyPrint() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ostringstream oss;
    oss << "Topology view (from " << self_id_ << "):\n";
    for (const auto& [node, dist] : nodes_) {
        oss << " - " << node << "  (hops=" << dist << ")\n";
    }
    return oss.str();
}

} // namespace mesh

