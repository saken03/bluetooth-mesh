#pragma once
#include "TcpConnection.h"
#include <functional>
#include <thread>
#include <atomic>

namespace net {

class TcpServer {
public:
    using NewConnectionHandler = std::function<void(std::shared_ptr<TcpConnection>)>;

    TcpServer(const std::string& self_id, int port);
    ~TcpServer();

    void start(NewConnectionHandler handler);
    void stop();

private:
    std::string self_id_;
    int port_;
    int listen_fd_{-1};
    std::thread accept_thread_;
    std::atomic<bool> running_{false};
};

} // namespace net

