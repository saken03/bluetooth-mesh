#include "TcpClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

namespace net {

std::shared_ptr<TcpConnection> TcpClient::connectTo(
    const std::string& self_id,
    const std::string& host,
    int port) {

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "[TcpClient] socket() failed\n";
        return nullptr;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "[TcpClient] inet_pton() failed\n";
        close(fd);
        return nullptr;
    }

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[TcpClient] connect() failed\n";
        close(fd);
        return nullptr;
    }

    auto conn = std::make_shared<TcpConnection>(fd, self_id, "peer");
    return conn;
}

} // namespace net

