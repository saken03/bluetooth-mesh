#include "TcpServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

namespace net {

TcpServer::TcpServer(const std::string& self_id, int port)
    : self_id_(self_id), port_(port) {}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::start(NewConnectionHandler handler) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "[TcpServer] socket() failed\n";
        return;
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[TcpServer] bind() failed\n";
        close(listen_fd_);
        listen_fd_ = -1;
        return;
    }

    if (listen(listen_fd_, 5) < 0) {
        std::cerr << "[TcpServer] listen() failed\n";
        close(listen_fd_);
        listen_fd_ = -1;
        return;
    }

    running_ = true;
    accept_thread_ = std::thread([this, handler]() {
        while (running_) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &len);
            if (client_fd < 0) {
                if (running_) std::cerr << "[TcpServer] accept() failed\n";
                continue;
            }

            auto conn = std::make_shared<TcpConnection>(client_fd, self_id_, "peer");
            handler(conn);
            conn->start();
        }
    });
}

void TcpServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (accept_thread_.joinable()) accept_thread_.join();
}

} // namespace net

