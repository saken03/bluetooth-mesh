#pragma once
#include "../mesh/MessageRouter.h"
#include "../mesh/TopologyManager.h"
#include <thread>
#include <atomic>

namespace ui {

class ConsoleUI {
public:
    ConsoleUI(mesh::MessageRouter& router,
              mesh::TopologyManager& topology,
              const std::string& self_id);

    void start();
    void stop();

private:
    mesh::MessageRouter& router_;
    mesh::TopologyManager& topology_;
    std::string self_id_;
    std::thread input_thread_;
    std::atomic<bool> running_{false};

    void inputLoop();
};

} // namespace ui

