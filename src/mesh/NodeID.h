#pragma once
#include <string>

namespace mesh {

class NodeID {
public:
    // Returns stable ID for this node. For now: hostname + random suffix on first run.
    static std::string get();

private:
    static std::string cached_id;
};

} // namespace mesh

