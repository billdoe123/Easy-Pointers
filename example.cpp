#include <iostream>
#include <string>
#include <vector>
#include "ptr.hpp"

// ── POLYMORPHIC BASE CLASS FOR NETWORK NODES ────────────────────
class NetworkNode {
public:
    virtual ~NetworkNode() = default;
    virtual returns(std::string) get_node_type() = 0;
    virtual returns(nothing_returned) process_packet(int packet_id) = 0;
};

// Forward declaration for device connections
class Link;

// ── CONCRETE CLASS FOR NETWORK DEVICES ──────────────────────────
class Device : public NetworkNode {
public:
    std::string device_name;
    std::vector<owned<Link>> outbound_links;
    maybe<Device> failover_route; // Explicitly allowed to be null

    Device(std::string name) : device_name(name), failover_route(nothing) {}

    returns(std::string) get_node_type() override {
        return "Hardware Device";
    }

    returns(nothing_returned) process_packet(int packet_id) override {
        std::cout << "  [" << device_name << "] Processing packet #" << packet_id << "\n";
    }

    returns(nothing_returned) link_to(shared<Device> target, int latency_ms);
};

// ── GRAPH EDGE REPRESENTATION (LINK) ───────────────────────────
class Link {
public:
    shared<Device> destination;
    int latency;

    Link(shared<Device> dest, int lat) : destination(move_from(dest)), latency(lat) {}
};

// Definition of link_to utilizing macro expressions
returns(nothing_returned) Device::link_to(shared<Device> target, int latency_ms) {
    // Dynamically allocate an owned Link edge structure and add it to the vector
    outbound_links.push_back(allocate<Link>(move_from(target), latency_ms));
}

// ── POINTER TRAVERSAL FUNCTIONS ─────────────────────────────────

// Demonstrates 'readonly', 'viewing', 'maybe', and library inspection patterns
returns(nothing_returned) inspect_network_topology(viewing<Device> node) {
    if (is_empty(node)) return;

    std::cout << "\n[Topology Inspection] Node: " << field(node, device_name) 
              << " (" << field(node, get_node_type)() << ")\n";

    // Safely check a optional/maybe pointer using safe execution patterns
    maybe<Device> backup = field(node, failover_route);
    when_valid(backup, [](ref_to(Device) backup_dev) {
        std::cout << "  -> Configured Failover Path to: " << field(address_of(backup_dev), device_name) << "\n";
    });
    when_empty(backup, []() {
        std::cout << "  -> Warning: No dedicated failover route assigned.\n";
    });

    std::cout << "  -> Connected Links:\n";
    for (readonly(owned<Link>) edge : field(node, outbound_links)) {
        // Gain a shared view safely
        shared<Device> target = field(edge, destination);
        std::cout << "     - Connected to " << field(target, device_name) 
                  << " | Latency: " << field(edge, latency) << "ms\n";
    }
}

// Demonstrates raw memory mutation using 'pointer_to', 'address_of', and 'write_to'
returns(nothing_returned) optimize_link_latency(pointer_to(int) latency_metric) {
    int optimized_value = read_from(latency_metric) / 2;
    write_to(latency_metric, optimized_value); // Directly writes through the raw pointer
}

// Demonstrates 'owned_array' generation and management
returns(nothing_returned) simulate_packet_burst(viewing<Device> primary_node) {
    std::size_t load_size = 4;
    
    // Dynamically allocate a heap array of primitive integers
    owned_array<int> metrics = allocate_array<int>(load_size);

    std::cout << "\n[Burst Simulation] Dispatching packets via " << field(primary_node, device_name) << ":\n";
    for (std::size_t i = 0; i < load_size; ++i) {
        write_index(metrics, i, as(int, (i + 1) * 111));
        field(primary_node, process_packet)(read_index(metrics, i));
    }
}

// ── APPLICATION ENTRY POINT ─────────────────────────────────────
int main() {
    std::cout << "==================================================\n";
    std::cout << "        ENHANCED POINTER-DRIVEN SIMULATION        \n";
    std::cout << "==================================================\n";

    // 1. Allocate dynamic graph nodes using shared ownership (mesh architecture)
    shared<Device> core_switch = allocate_shared<Device>("Core-Switch-01");
    shared<Device> edge_router = allocate_shared<Device>("Edge-Router-A");
    shared<Device> cloud_server = allocate_shared<Device>("Cloud-Database");

    // 2. Interconnect nodes (passing shared ownership handles securely)
    field(core_switch, link_to)(edge_router, 12);
    field(edge_router, link_to)(cloud_server, 45);
    field(core_switch, link_to)(cloud_server, 90);

    // 3. Assign a raw conditional 'maybe' fallback pointer
    // Obtains a safe view via borrow() and stores it as a raw tracking address
    field(core_switch, failover_route) = borrow(cloud_server);

    // 4. Inspect the graph structures passing safe viewing handles
    inspect_network_topology(borrow(core_switch));
    inspect_network_topology(borrow(edge_router));
    inspect_network_topology(borrow(cloud_server));

    // 5. Mutate an individual link's metric using deep raw address access
    // Extract raw connection vector -> find first link -> take field address -> modify
    readonly(owned<Link>) target_link = field(core_switch, outbound_links).at(0);
    pointer_to(int) raw_metric_address = address_of(field(target_link, latency));
    
    std::cout << "\n[Optimization] Original Link 0 Latency: " << read_from(raw_metric_address) << "ms\n";
    optimize_link_latency(raw_metric_address);
    std::cout << "[Optimization] New Optimized Link 0 Latency: " << read_from(raw_metric_address) << "ms\n";

    // 6. Run dynamic memory array sequence tracking
    simulate_packet_burst(borrow(core_switch));

    std::cout << "\n==================================================\n";
    std::cout << " Execution Finished Cleanly. All memory auto-freed.\n";
    std::cout << "==================================================\n";
    return 0;
}