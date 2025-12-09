#pragma once

#include "../../mesh/ILink.h"
#include <functional>
#include <memory>
#include <string>

namespace ble {

class MacBLE {
public:
    using NewLinkCallback = std::function<void(std::shared_ptr<mesh::ILink>)>;

    explicit MacBLE(const std::string& selfId);
    ~MacBLE();

    // Start BLE peripheral + central.
    // cb is called whenever a new BLE-based link is ready.
    void start(NewLinkCallback cb);

    void stop();

private:
    struct Impl;
    Impl* impl_;
};

} // namespace ble

