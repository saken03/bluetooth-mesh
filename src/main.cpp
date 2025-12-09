#include "mesh/NodeID.h"
#include "mesh/NeighborTable.h"
#include "mesh/TopologyManager.h"
#include "mesh/MessageRouter.h"
#include "mesh/Packet.h"
#include "mesh/PacketSerializer.h"
#include "bluetooth/BluetoothConnection.h"
#include "ui/ConsoleUI.h"
#include "net/TcpServer.h"
#include "net/TcpClient.h"
#include "bluetooth/ble/MacBLE.h"

#include <iostream>
#include <memory>
#include <csignal>
#include <atomic>

static std::atomic<bool> g_running(true);

void sigint_handler(int) {
    g_running = false;
}

int main() {
    
    std::signal(SIGINT, sigint_handler);

    std::string self_id = mesh::NodeID::get();
    mesh::NeighborTable neighbor_table;
    mesh::TopologyManager topology;
    topology.setSelfId(self_id);

    mesh::MessageRouter router(self_id, neighbor_table, topology);
    router.setReceiveCallback([](const mesh::Packet& p) {
        #ifdef __APPLE__
        ble::MacBLE bleManager(self_id);

        bleManager.start([&](std::shared_ptr<mesh::ILink> link) {
            link->setPacketHandler([&router](const mesh::Packet& p) {
                router.handleIncoming(p);
            });
        router.addLink(link);
        std::cout << "[INFO] New BLE link: " << link->peerId() << "\n";
        });
        #endif

        std::cout << "\n[MSG from " << p.sender_id << "] " << p.payload << "\n";
        std::cout << "> " << std::flush;
    });

    // TEMP: Single fake connection to "demo-peer".
    // Later you will replace this with real Bluetooth discovery + connections.
    auto bt_conn = std::make_shared<bt::BluetoothConnection>(self_id, "demo-peer");
    bt_conn->setPacketHandler([&router](const mesh::Packet& p) {
        router.handleIncoming(p);
    });
    // bt_conn->startFakeLoop(); // Uncomment if you want to simulate incoming packets from stdin

    router.addLink(bt_conn);

    int port = 9000; // same for all nodes

    net::TcpServer server(self_id, port);
    server.start([&](std::shared_ptr<net::TcpConnection> conn) {
        conn->setPacketHandler([&router](const mesh::Packet& p) {
            router.handleIncoming(p);
        });
        router.addLink(conn);
        std::cout << "[INFO] New incoming TCP connection\n";
    });

    // If you pass "connect" argument, act as client
    // Example: ./meshnode connect 127.0.0.1
    if (char* env = std::getenv("MESH_CONNECT_HOST")) {
        std::string host = env;
        auto conn = net::TcpClient::connectTo(self_id, host, port);
        if (conn) {
            conn->setPacketHandler([&router](const mesh::Packet& p) {
                router.handleIncoming(p);
            });
            conn->start();
            router.addLink(conn);
            std::cout << "[INFO] Connected to " << host << "\n";
        }
    }


    ui::ConsoleUI console(router, topology, self_id);
    console.start();

    while (g_running) {
        // In a real impl, you would periodically send TOPOLOGY packets here.
        // For now, just sleep.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    console.stop();
    return 0;
}

