#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

namespace mesh {

// Stores node_id -> hop_count (1 = direct neighbor)
class TopologyManager {
public:
    void setSelfId(const std::string& id);

    void updateFromNeighbor(const std::string& neighbor_id,
                            const std::unordered_map<std::string, int>& neighbor_view);

    std::unordered_map<std::string, int> snapshot() const;

    // build a simple text representation
    std::string prettyPrint() const;

private:
    std::string self_id_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, int> nodes_; // node -> hops from self
};

} // namespace mesh

