#include "ConsoleUI.h"
#include <iostream>

namespace ui {

ConsoleUI::ConsoleUI(mesh::MessageRouter& router,
                     mesh::TopologyManager& topology,
                     const std::string& self_id)
    : router_(router), topology_(topology), self_id_(self_id) {}

void ConsoleUI::start() {
    running_ = true;
    input_thread_ = std::thread(&ConsoleUI::inputLoop, this);
}

void ConsoleUI::stop() {
    running_ = false;
    if (input_thread_.joinable()) input_thread_.join();
}

void ConsoleUI::inputLoop() {
    std::cout << "Node ID: " << self_id_ << "\n";
    std::cout << "Commands:\n";
    std::cout << "  msg <dest_id> <text>   - send message\n";
    std::cout << "  topo                   - show topology\n";
    std::cout << "  me                     - print my id\n";
    std::cout << "  quit                   - exit\n";

    std::string line;
    while (running_) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit") {
            running_ = false;
            break;
        } else if (line == "me") {
            std::cout << "My ID: " << self_id_ << "\n";
        } else if (line == "topo") {
            std::cout << topology_.prettyPrint();
        } else if (line.rfind("msg ", 0) == 0) {
            auto first_space = line.find(' ');
            auto second_space = line.find(' ', first_space + 1);
            if (second_space == std::string::npos) {
                std::cout << "Usage: msg <dest_id> <text>\n";
                continue;
            }
            std::string dest = line.substr(first_space + 1,
                                           second_space - first_space - 1);
            std::string text = line.substr(second_space + 1);
            router_.sendTextMessage(dest, text);
        } else {
            std::cout << "Unknown command\n";
        }
    }
}

} // namespace ui

