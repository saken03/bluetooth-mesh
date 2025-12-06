# Bluetooth Mesh Network (C++ / BlueZ)

This project implements a simple **Bluetooth-based mesh network** that allows
laptops to discover each other, form a mesh topology, and exchange text messages
without Wi-Fi or internet access. Each device runs the application locally and
acts as a node in the mesh, forwarding packets for others.

The goal is to build an offline, self-organizing communication network using
only Bluetooth connections.

---

## 🚀 Features (MVP)

- 🔵 Scan for nearby Bluetooth devices running the same program  
- 🔗 Establish RFCOMM connections (Bluetooth serial sockets)  
- 🗺️ Maintain a table of direct neighbors  
- 🌐 Build a multi-hop topology (discover indirect nodes)  
- 💬 Send text messages across the mesh  
- 🔁 Flood-based routing with message deduplication  
- 🖥️ Console UI to view topology and send messages  

---

## 🧱 Project Structure

bluetooth-mesh/
│
├── src/
│ ├── main.cpp
│ ├── bluetooth/
│ ├── mesh/
│ └── ui/
│
├── tests/
├── config/
├── scripts/
└── CMakeLists.txt

markdown
Copy code

### Folder Overview

| Folder          | Purpose |
|----------------|---------|
| `src/bluetooth` | Low-level Bluetooth code (BlueZ, scanning, RFCOMM) |
| `src/mesh`      | Mesh logic (topology, routing, packet serialization) |
| `src/ui`        | Console interface and optional graph view |
| `config`        | Future configuration options |
| `tests`         | Routing/topology unit tests |
| `scripts`       | Helper scripts (run, discover, debug) |

---

## 🔧 Build Instructions (Linux)

This project uses **CMake** as the build system and depends on:
- BlueZ (`libbluetooth-dev`)
- pthread

### 1. Install dependencies

```bash
sudo apt update
sudo apt install cmake build-essential libbluetooth-dev
2. Build the project
bash
Copy code
mkdir build
cd build
cmake ..
make
3. Run
bash
Copy code
./meshnode
🕸️ How the Mesh Works
1. Node Identification
Each node generates a unique ID based on its Bluetooth MAC + random seed.

2. Discovery
Nodes scan for devices advertising a special name:

Copy code
BTMeshNode-XXXX
When found, they establish an RFCOMM (serial) connection.

3. Topology Sharing
Each node periodically broadcasts a Topology Packet containing:

known nodes

their distances (hop count)

This allows nodes to learn about multi-hop neighbors.

4. Message Routing
Routing uses simple controlled flooding:

Each message has a unique message ID

Each node forwards it to all neighbors except the sender

Duplicate messages are ignored

If the destination matches the local node → display message

This is easy to implement and reliable for small networks.

🧪 Testing
Unit tests are located in tests/.

To run tests:

bash
Copy code
make test
