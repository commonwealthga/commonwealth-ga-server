#pragma once

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/NodeLink/NodeCrypto.hpp"
#include "src/ControlServer/NodeLink/NodeLink.hpp"
#include "src/ControlServer/NodeLink/NodeStreamFramePipe.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// NodeRegistry.hpp -- the master's view of its nodes.
//
// Owns the authenticated links, tracks per-node health and instance lists, routes
// control messages, and turns instance tunnel streams into FramePipes that the
// existing IpcServer serves unchanged.
//
// Threading: io thread only (the control server runs one io_context, see
// main.cpp). No locking, deliberately -- invoking callbacks while holding a lock
// is how deadlocks get invented, and nothing here is touched off-thread.
//
// When a node link dies, every IPC stream it was carrying is closed, which makes
// the instance's IpcSession run its normal disconnect cleanup (mark STOPPED,
// discard a pending match, clear overlay state). No new code path needed.

enum class NodeHealth {
    Configured,  // in nodes.json, never connected this run
    Up,          // link authenticated and heartbeating
    Stale,       // link alive but no status within kStaleAfterMs: no new work
    Down,        // link gone
};

struct NodeRuntime {
    int         id = 0;
    std::string name;
    int         site = 0;
    std::string address;    // public address its instances are reachable at (IP or hostname)
    // Resolved from `address` when that is a hostname. A rotating residential IP
    // with a DynDNS name would otherwise have to be edited in config every time it
    // moves; the master resolves the name and refreshes it from the tick.
    std::string address_resolved;
    std::int64_t address_resolved_at_ms = 0;
    // Source address of the node's authenticated link. Recorded so a node whose
    // public IP rotates does not need config edits or DynDNS: the address players
    // are sent to follows the link. Not an identity -- the handshake is, which
    // binds node_id to the pre-shared key -- so this only decides where to *point*
    // clients, never whether to trust the peer.
    std::string address_observed;
    int         capacity = 0;  // 0 = unlimited
    bool        home_map = false;
    bool        enabled = true;

    NodeHealth  health = NodeHealth::Configured;
    std::shared_ptr<NodeLink::Link> link;
    std::vector<int64_t> instances;
    std::string content_hash;
    std::string agent_version;
    // Which proxy flavour the node has installed ("dinput8.dll" or
    // "version.dll"; both are builds of the same server) and its hash.
    std::string dll_name;
    std::string dll_hash;
    // Byte identity of the node agent it is running (BLAKE2b-256), reported in
    // NODE_STATUS. Version strings collide across builds; this does not.
    std::string agent_hash;
    int64_t     last_status_ms = 0;
    int64_t     connected_at_ms = 0;
    std::string last_error;
    // Bumped on every attach/detach so a late close callback from a superseded
    // link cannot tear down its replacement.
    std::uint64_t link_gen = 0;

    // Control-message budget. An authenticated node is trusted for identity, not
    // for discipline: a buggy or malicious one could otherwise ask for thousands
    // of instances a second.
    double        msg_tokens = 0.0;
    std::int64_t  msg_refilled_ms = 0;
    std::int64_t  msg_dropped = 0;
};

class NodeRegistry {
public:
    // Invoked from the io thread. main.cpp supplies the policy (mark STOPPED,
    // discard a pending match, log) so the registry stays free of matchmaking.
    struct Hooks {
        std::function<void(int node_id)> on_up;
        std::function<void(int node_id, const std::string& reason)> on_down;
        std::function<void(int node_id, int64_t instance_id, std::uint16_t udp_port, int pid)>
            on_spawning;
        std::function<void(int node_id, int64_t instance_id, const std::string& reason)>
            on_failed;
        std::function<void(int node_id, int64_t instance_id, int exit_code)> on_exited;
    };

    static void Init(const ControlServerConfig& cfg);

    // Re-applies a changed config without restarting the master.
    //
    // Runtime state (link, health, instance list, content hash, last error) is
    // carried across for nodes that still exist; a node that was removed or
    // disabled has its live link closed, which is what makes the change take
    // effect. Keys and names take effect on the *next* handshake -- an existing
    // link already agreed its session keys, so rotating a PSK never drops a
    // running match.
    static void Reload(const ControlServerConfig& cfg);
    static void SetHooks(Hooks hooks);

    // Handshake plumbing -----------------------------------------------------

    // Resolve a node name to its pre-shared key. False refuses the node before
    // any crypto runs.
    static bool LookupKey(const std::string& node_name, NodeCrypto::Key& out);

    // Take ownership of an authenticated link. Returns the node id, or -1 if the
    // name is not configured (defence in depth -- LookupKey already refused it).
    static int Attach(std::shared_ptr<NodeLink::Link> link);

    static void Detach(int node_id, const std::string& reason);

    // Queries ----------------------------------------------------------------

    static bool IsConfigured(int node_id);
    static bool IsUp(int node_id);
    // The address to put in a client packet: the resolved IPv4 when the node's
    // configured address is a hostname, otherwise the configured value. "" if the
    // node is unknown.
    static std::string AddressOf(int node_id);
    static std::string NameOf(int node_id);
    // nodes[].capacity, 0 = unlimited.
    static int CapacityOf(int node_id);
    // True when the node is Up and below its configured capacity.
    static bool CanAcceptWork(int node_id, std::string& why_not);
    static std::vector<NodeRuntime> Snapshot();

    // Control plane ----------------------------------------------------------

    static bool SendControl(int node_id, const std::string& json);

    // Ages health and forces out nodes that stopped heartbeating. Call on a
    // timer faster than kStaleAfterMs.
    static void Tick();

    // Closes the tunnel stream for one instance (after INSTANCE_EXITED), which
    // makes its IpcSession run its normal disconnect cleanup.
    static void CloseInstanceStream(int node_id, int64_t instance_id);

private:
    static void OnFrame(int node_id, std::uint8_t kind, std::uint64_t stream,
                        std::string payload);
    static void HandleControl(int node_id, const std::string& payload);
    static void DetachIfCurrent(int node_id, std::uint64_t gen, const std::string& reason);
    static std::shared_ptr<NodeLink::Link> CurrentLink(int node_id);
    static void CloseStreamsFor(int node_id);

    // Returns the pipe for (node, instance), creating and adopting it on first
    // use. Null when the node has no live link.
    static std::shared_ptr<NodeLink::NodeStreamFramePipe>
        StreamFor(int node_id, int64_t instance_id);
};
