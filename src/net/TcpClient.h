#pragma once
#include "TcpConnection.h"

namespace net {

class TcpClient {
public:
    static std::shared_ptr<TcpConnection> connectTo(
        const std::string& self_id,
        const std::string& host,
        int port);
};

} // namespace net

