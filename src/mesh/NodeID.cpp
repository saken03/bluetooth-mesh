#include "NodeID.h"
#include <random>
#include <sstream>
#include <mutex>
#include <unistd.h>

namespace mesh {

std::string NodeID::cached_id;
static std::mutex id_mutex;

static std::string generateRandomSuffix() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t x = dist(gen);
    std::ostringstream oss;
    oss << std::hex << x;
    return oss.str();
}

std::string NodeID::get() {
    std::lock_guard<std::mutex> lock(id_mutex);
    if (!cached_id.empty()) return cached_id;

    char hostname_buf[256];
    if (gethostname(hostname_buf, sizeof(hostname_buf)) != 0) {
        cached_id = "node-" + generateRandomSuffix();
    } else {
        cached_id = std::string(hostname_buf) + "-" + generateRandomSuffix();
    }
    return cached_id;
}

} // namespace mesh

