#include "src/ControlServer/NodeRegistry/NodeRegistry.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeData/ContentSnapshot.hpp"
#include "src/ControlServer/NodeData/AgentArtifact.hpp"
#include "src/Shared/NetAddress.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif
#include "src/ControlServer/NodeData/PlayerBundle.hpp"
#include "src/ControlServer/NodeLink/NodeMessages.hpp"
#include "src/Shared/Base64.hpp"
#include "lib/nlohmann/json.hpp"

#include <chrono>
#include <utility>

namespace {

std::map<int, NodeRuntime>                            g_nodes;
std::map<int, NodeCrypto::Key>                        g_keys;   // never logged
std::map<std::pair<int, int64_t>,
         std::shared_ptr<NodeLink::NodeStreamFramePipe>> g_streams;
NodeRegistry::Hooks                                   g_hooks;

// Forwarded to every node in NODE_WELCOME so a node can answer its own instances
// INSTANCE_HELLO_ACK while the link is down.
bool g_device_stats_enabled = true;
bool g_effectiveness_enabled = true;

// Control-message budget per node: a burst of kMsgBurst, refilled at kMsgPerSec.
// Generous enough for a status heartbeat every 5s plus a spawn/stop storm, tight
// enough that a runaway node cannot create instances faster than the database can
// absorb them.
constexpr double kMsgPerSec = 100.0;
constexpr double kMsgBurst  = 300.0;
// Sustained abuse (this many drops without recovering) drops the link.
constexpr std::int64_t kMsgDropLimit = 5000;

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::string Describe(int node_id) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return "node?" + std::to_string(node_id);
    return "node '" + it->second.name + "' (id=" + std::to_string(node_id) + ")";
}

// Only accept an instance id this node was actually given. A node reporting
// numbers it was not handed is either confused or hostile; either way it must not
// be able to touch another instance's rows.
bool OwnsInstance(int node_id, int64_t instance_id) {
    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    if (!inst) {
        Logger::Log("node", "[NodeRegistry] %s referenced unknown instance=%lld -- ignoring\n",
            Describe(node_id).c_str(), (long long)instance_id);
        return false;
    }
    if (inst->node_id != node_id) {
        Logger::Log("node",
            "[NodeRegistry] %s referenced instance=%lld which belongs to node_id=%d -- ignoring\n",
            Describe(node_id).c_str(), (long long)instance_id, inst->node_id);
        return false;
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

namespace {

// The config-derived part of a node's runtime record. Shared by Init and Reload
// so the two can never disagree about what a config change means.
NodeRuntime MakeRuntime(const NodeEntryConfig& entry) {
    NodeRuntime rt;
    rt.id       = entry.id;
    rt.name     = entry.name.empty() ? std::to_string(entry.id) : entry.name;
    rt.site     = entry.site;
    rt.address  = entry.address;
    rt.capacity = entry.capacity;
    rt.home_map = entry.home_map;
    rt.enabled  = entry.enabled;
    rt.health   = NodeHealth::Configured;
    return rt;
}

}  // namespace

void NodeRegistry::Reload(const ControlServerConfig& cfg) {
    // Order matters: detach first, then carry state forward. Building the new
    // map before detaching would copy a link pointer that Detach had already
    // closed, resurrecting a dead link for a node that was just disabled.
    std::map<int, bool> wanted;          // id -> enabled
    for (const auto& entry : cfg.nodes) {
        if (entry.id > 0) wanted[entry.id] = entry.enabled;
    }

    for (const auto& kv : g_nodes) {
        const int id = kv.first;
        auto it = wanted.find(id);
        const bool gone     = (it == wanted.end());
        const bool disabled = !gone && !it->second;
        if (!gone && !disabled) continue;

        const bool was_live = kv.second.link != nullptr;
        Logger::Log("node",
            "[NodeRegistry] reload: node '%s' (id %d) %s%s\n",
            kv.second.name.c_str(), id, gone ? "removed" : "disabled",
            was_live ? " -- closing its link" : "");
        if (was_live) {
            // Closes the tunnel streams, which makes the instances' sessions run
            // their normal disconnect cleanup. That is the intent of disabling or
            // removing a node: stop placing work there and stop trusting it.
            Detach(id, gone ? "removed by config change" : "disabled by config change");
        }
    }

    std::map<int, NodeRuntime> next;
    std::map<int, NodeCrypto::Key> next_keys;

    for (const auto& entry : cfg.nodes) {
        if (entry.id <= 0) {
            Logger::Log("node",
                "[NodeRegistry] reload: entry with id %d ignored (0 is the local master)\n",
                entry.id);
            continue;
        }
        if (next.count(entry.id)) {
            Logger::Log("node", "[NodeRegistry] reload: duplicate id %d ignored\n", entry.id);
            continue;
        }
        if (entry.key.empty()) {
            Logger::Log("node",
                "[NodeRegistry] reload: id %d has no key -- ignoring\n", entry.id);
            continue;
        }
        NodeCrypto::Key key{};
        if (!NodeCrypto::KeyFromHex(entry.key, key)) {
            Logger::Log("node",
                "[NodeRegistry] reload: id %d has a malformed key (need 64 hex chars) "
                "-- ignoring\n", entry.id);
            continue;
        }

        NodeRuntime rt = MakeRuntime(entry);
        auto old = g_nodes.find(entry.id);
        if (old != g_nodes.end()) {
            // Carry runtime state across: a config edit must not look like a
            // disconnect to a node that is happily connected.
            rt.health          = old->second.health;
            rt.link            = old->second.link;
            rt.instances       = old->second.instances;
            rt.content_hash    = old->second.content_hash;
            rt.agent_version   = old->second.agent_version;
            rt.dll_name        = old->second.dll_name;
            rt.dll_hash        = old->second.dll_hash;
            rt.last_status_ms  = old->second.last_status_ms;
            rt.connected_at_ms = old->second.connected_at_ms;
            rt.last_error      = old->second.last_error;
            rt.link_gen        = old->second.link_gen;
            rt.address_observed       = old->second.address_observed;
            rt.address_resolved       = old->second.address_resolved;
            rt.address_resolved_at_ms = old->second.address_resolved_at_ms;

            const bool changed =
                old->second.site != rt.site || old->second.capacity != rt.capacity ||
                old->second.address != rt.address || old->second.home_map != rt.home_map ||
                old->second.enabled != rt.enabled;
            if (changed) {
                Logger::Log("node",
                    "[NodeRegistry] reload: node '%s' (id %d) updated: site %d->%d capacity "
                    "%d->%d home_map %d->%d enabled %d->%d address '%s'->'%s'\n",
                    rt.name.c_str(), rt.id, old->second.site, rt.site,
                    old->second.capacity, rt.capacity, (int)old->second.home_map,
                    (int)rt.home_map, (int)old->second.enabled, (int)rt.enabled,
                    old->second.address.c_str(), rt.address.c_str());
            }
        } else {
            Logger::Log("node", "[NodeRegistry] reload: node '%s' (id %d) added, site %d\n",
                rt.name.c_str(), rt.id, rt.site);
        }

        next[entry.id] = std::move(rt);
        next_keys[entry.id] = key;
    }

    g_nodes = std::move(next);
    g_keys  = std::move(next_keys);
    g_device_stats_enabled  = cfg.device_stats_enabled;
    g_effectiveness_enabled = cfg.effectiveness_enabled;
    Logger::Log("node", "[NodeRegistry] reload complete: %zu node(s)\n", g_nodes.size());
}

void NodeRegistry::Init(const ControlServerConfig& cfg) {
    g_nodes.clear();
    g_keys.clear();
    g_streams.clear();
    g_device_stats_enabled  = cfg.device_stats_enabled;
    g_effectiveness_enabled = cfg.effectiveness_enabled;

    for (const auto& entry : cfg.nodes) {
        if (entry.id <= 0) {
            Logger::Log("node",
                "[NodeRegistry] nodes[]: entry with id %d ignored (ids must be > 0; 0 is the local master)\n",
                entry.id);
            continue;
        }
        if (g_nodes.count(entry.id)) {
            Logger::Log("node", "[NodeRegistry] nodes[]: duplicate id %d ignored\n", entry.id);
            continue;
        }
        if (entry.key.empty()) {
            Logger::Log("node", "[NodeRegistry] nodes[]: id %d has no key -- ignoring\n", entry.id);
            continue;
        }
        NodeCrypto::Key key{};
        if (!NodeCrypto::KeyFromHex(entry.key, key)) {
            Logger::Log("node",
                "[NodeRegistry] nodes[]: id %d has a malformed key (need 64 hex chars) -- ignoring\n",
                entry.id);
            continue;
        }
        if (!entry.enabled) {
            Logger::Log("node", "[NodeRegistry] nodes[]: id %d is disabled\n", entry.id);
            // Still registered so its key never becomes someone else's id, but it
            // is never Up, so it is never placed into.
        }

        NodeRuntime rt = MakeRuntime(entry);

        for (const auto& kv : g_nodes) {
            if (kv.second.name == rt.name) {
                Logger::Log("node",
                    "[NodeRegistry] nodes[]: duplicate name '%s' (ids %d and %d) -- the second is unusable\n",
                    rt.name.c_str(), kv.first, rt.id);
            }
        }

        Logger::Log("node",
            "[NodeRegistry] configured node '%s' id=%d site=%d capacity=%d home_map=%d address='%s' enabled=%d\n",
            rt.name.c_str(), rt.id, rt.site, rt.capacity, (int)rt.home_map,
            rt.address.c_str(), (int)rt.enabled);

        g_nodes[rt.id] = std::move(rt);
        g_keys[entry.id] = key;
    }

    Logger::Log("node", "[NodeRegistry] %zu node(s) configured\n", g_nodes.size());
}

void NodeRegistry::SetHooks(Hooks hooks) {
    g_hooks = std::move(hooks);
}

// ---------------------------------------------------------------------------
// Handshake plumbing
// ---------------------------------------------------------------------------

bool NodeRegistry::LookupKey(const std::string& node_name, NodeCrypto::Key& out) {
    for (const auto& kv : g_nodes) {
        if (kv.second.name != node_name) continue;
        if (!kv.second.enabled) {
            Logger::Log("node",
                "[NodeRegistry] node '%s' is configured but disabled -- refusing its handshake\n",
                node_name.c_str());
            return false;
        }
        auto key = g_keys.find(kv.first);
        if (key == g_keys.end()) return false;
        out = key->second;
        return true;
    }
    return false;
}

namespace {

// Defined next to the tick; declared here because Attach records the peer address
// of the link it just installed. All unnamed namespaces in a TU are one namespace,
// so these refer to the definitions below.
std::string PeerIPv4(const std::string& peer_label);
std::string ResolveIPv4(const std::string& host);

}  // namespace

int NodeRegistry::Attach(std::shared_ptr<NodeLink::Link> link) {
    if (!link) return -1;

    int node_id = -1;
    for (const auto& kv : g_nodes) {
        if (kv.second.name == link->NodeId()) {
            node_id = kv.first;
            break;
        }
    }
    if (node_id < 0) {
        // Cannot normally happen: LookupKey already refused this name.
        Logger::Log("node", "[NodeRegistry] refusing link from unconfigured node '%s'\n",
            link->NodeId().c_str());
        link->Close("node not configured");
        return -1;
    }

    NodeRuntime& rt = g_nodes[node_id];
    auto previous = rt.link;

    // Install the new link and bump the generation BEFORE closing the old one, so
    // the old link's close callback cannot tear down its replacement.
    rt.link = link;
    ++rt.link_gen;
    const std::uint64_t gen = rt.link_gen;
    rt.health = NodeHealth::Up;
    rt.connected_at_ms = NowMs();
    rt.last_status_ms = rt.connected_at_ms;
    rt.last_error.clear();
    rt.instances.clear();

    // Where this link came from. Used only when no address is configured, so an
    // ordinary node with a rotating residential IP needs no config and no DynDNS:
    // each reconnect records the new address and placements follow it.
    {
        const std::string observed = PeerIPv4(link->PeerLabel());
        const bool changed = !observed.empty() && observed != rt.address_observed;
        if (!observed.empty()) rt.address_observed = observed;
        if (changed) {
            Logger::Log("node", "[NodeRegistry] %s connected from %s%s\n",
                Describe(node_id).c_str(), observed.c_str(),
                rt.address.empty() ? " (no address configured -- using it)"
                                   : " (a configured address takes precedence)");
        }
    }

    if (previous && previous->IsOpen()) {
        Logger::Log("node", "[NodeRegistry] %s reconnected; dropping the previous link\n",
            Describe(node_id).c_str());
        previous->Close("superseded by a new connection");
    }

    link->Start(
        [node_id](std::uint8_t kind, std::uint64_t stream, std::string payload) {
            OnFrame(node_id, kind, stream, std::move(payload));
        },
        [node_id, gen](const std::string& reason) {
            DetachIfCurrent(node_id, gen, reason);
        });

    Logger::Log("node", "[NodeRegistry] %s up from %s\n",
        Describe(node_id).c_str(), link->PeerLabel().c_str());

    // Welcome: everything the node needs to serve its own instances while the
    // link is down, so it never has to ask mid-match.
    nlohmann::json welcome;
    welcome["type"]                     = NodeMsg::kWelcome;
    welcome["master_ts"]                = NowMs();
    welcome["device_stats_enabled"]     = g_device_stats_enabled;
    welcome["effectiveness_enabled"]    = g_effectiveness_enabled;
    // The node's integer identity: it needs it to find its own ga_instances rows
    // (a node only ever knows its name from its config).
    welcome["node_id"]                  = node_id;
    // What content the node should be serving from. It compares this against what
    // it has and pulls the difference before its first spawn.
    const auto& content = ContentSnapshot::Current();
    welcome["content_hash"]             = content.ok ? content.hash : std::string();
    welcome["content_size"]             = content.ok ? content.size : 0;
    // What this master can hand out besides content: its own build, and the proxy
    // DLL if the operator configured one. Empty hash = nothing offered, which is
    // also what a node sees when node_auto_update is off.
    const auto& agent = AgentArtifact::Agent();
    const auto& dll   = AgentArtifact::Dll();
    welcome["agent_offer_hash"]         = agent.ok ? agent.hash : std::string();
    welcome["agent_offer_size"]         = agent.ok ? agent.size : 0;
    welcome["dll_offer_hash"]           = dll.ok ? dll.hash : std::string();
    welcome["dll_offer_size"]           = dll.ok ? dll.size : 0;
    SendControl(node_id, welcome.dump());

    if (g_hooks.on_up) g_hooks.on_up(node_id);
    return node_id;
}

void NodeRegistry::Detach(int node_id, const std::string& reason) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return;

    NodeRuntime& rt = it->second;
    auto link = rt.link;
    rt.link.reset();
    ++rt.link_gen;
    rt.health = NodeHealth::Down;
    rt.instances.clear();
    rt.last_error = reason;

    CloseStreamsFor(node_id);

    if (link && link->IsOpen()) link->Close(reason);

    Logger::Log("node", "[NodeRegistry] %s down: %s\n",
        Describe(node_id).c_str(), reason.c_str());

    if (g_hooks.on_down) g_hooks.on_down(node_id, reason);
}

void NodeRegistry::DetachIfCurrent(int node_id, std::uint64_t gen, const std::string& reason) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return;
    if (it->second.link_gen != gen) {
        // A superseded link finishing its teardown; nothing to do.
        return;
    }
    Detach(node_id, reason);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool NodeRegistry::IsConfigured(int node_id) {
    return g_nodes.count(node_id) != 0;
}

bool NodeRegistry::IsUp(int node_id) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return false;
    return it->second.health == NodeHealth::Up && it->second.enabled
        && it->second.link && it->second.link->IsOpen();
}

std::string NodeRegistry::AddressOf(int node_id) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return std::string();
    const NodeRuntime& rt = it->second;

    // Precedence: an explicitly configured address wins (resolved if it is a
    // hostname), because that is the operator saying "this is where players reach
    // it" -- which is what a VPN'd or multi-homed node needs. Otherwise fall back
    // to the address its own link came from, which is right for the ordinary case
    // of a residential IP that rotates: no config edit, no DynDNS.
    if (!rt.address_resolved.empty()) return rt.address_resolved;
    if (NetAddress::IsIPv4Literal(rt.address)) return rt.address;
    if (!rt.address_observed.empty()) return rt.address_observed;
    return rt.address;   // unresolved hostname; callers must check it is a literal
}

std::string NodeRegistry::NameOf(int node_id) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return std::string();
    return it->second.name;
}

int NodeRegistry::CapacityOf(int node_id) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return 0;
    return it->second.capacity;
}

bool NodeRegistry::CanAcceptWork(int node_id, std::string& why_not) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) {
        why_not = "node " + std::to_string(node_id) + " is not configured";
        return false;
    }
    const NodeRuntime& rt = it->second;
    if (!rt.enabled) {
        why_not = "node '" + rt.name + "' is disabled";
        return false;
    }
    if (!IsUp(node_id)) {
        why_not = "node '" + rt.name + "' is not connected";
        return false;
    }
    if (rt.capacity > 0) {
        const int active = InstanceRegistry::CountActiveInstancesForNode(node_id);
        if (active >= rt.capacity) {
            why_not = "node '" + rt.name + "' is at capacity (" + std::to_string(active) +
                      "/" + std::to_string(rt.capacity) + ")";
            return false;
        }
    }
    return true;
}

std::vector<NodeRuntime> NodeRegistry::Snapshot() {
    std::vector<NodeRuntime> out;
    out.reserve(g_nodes.size());
    for (const auto& kv : g_nodes) out.push_back(kv.second);
    return out;
}

std::shared_ptr<NodeLink::Link> NodeRegistry::CurrentLink(int node_id) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return nullptr;
    return it->second.link;
}

// ---------------------------------------------------------------------------
// Control plane
// ---------------------------------------------------------------------------

bool NodeRegistry::SendControl(int node_id, const std::string& json) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return false;
    auto link = it->second.link;
    if (!link || !link->IsOpen()) return false;
    return link->SendFrame(NodeLink::kFrameControl, 0, json);
}

namespace {

// How often a configured hostname is re-resolved. DynDNS default TTLs are 60s, so
// polling faster only adds DNS traffic.
constexpr std::int64_t kAddressRefreshMs = 60 * 1000;

// "<1.2.3.4>:<port>" -> "1.2.3.4". "" when the peer is not IPv4 (the packet field
// is four bytes, so an IPv6 peer simply cannot be advertised to clients this way).
std::string PeerIPv4(const std::string& peer_label) {
    const auto colon = peer_label.rfind(':');
    if (colon == std::string::npos) return std::string();
    const std::string host = peer_label.substr(0, colon);
    return NetAddress::IsIPv4Literal(host) ? host : std::string();
}

// Resolve a hostname to its first IPv4 address. "" on failure.
//
// Blocking, deliberately: it runs from the registry tick with a 60s per-node TTL,
// so a placement never carries the cost. The alternative -- resolving on the
// GO_PLAY path -- would block the io thread on DNS once per player.
std::string ResolveIPv4(const std::string& host) {
    struct addrinfo hints {};
    hints.ai_family   = AF_INET;      // the packet field is 4 bytes
    hints.ai_socktype = SOCK_DGRAM;   // instances are UDP

    struct addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || res == nullptr) {
        return std::string();
    }
    std::string out;
    for (struct addrinfo* it = res; it != nullptr; it = it->ai_next) {
        if (it->ai_family != AF_INET || it->ai_addr == nullptr) continue;
        const auto* sin = reinterpret_cast<const struct sockaddr_in*>(it->ai_addr);
        const std::uint32_t be = sin->sin_addr.s_addr;
        out = std::to_string(be & 0xFF) + "." +
              std::to_string((be >> 8) & 0xFF) + "." +
              std::to_string((be >> 16) & 0xFF) + "." +
              std::to_string((be >> 24) & 0xFF);
        break;
    }
    freeaddrinfo(res);
    return out;
}

// Re-announce the artifacts on offer to every node that could act on it. Sent on
// change only: a node ignores a message for bytes it already has, but there is no
// reason to generate traffic every tick.
void BroadcastArtifactOffers() {
    const auto& agent = AgentArtifact::Agent();
    const auto& dll   = AgentArtifact::Dll();

    nlohmann::json j;
    j["type"]             = NodeMsg::kArtifactOffer;
    j["agent_offer_hash"] = agent.ok ? agent.hash : std::string();
    j["agent_offer_size"] = agent.ok ? agent.size : 0;
    j["dll_offer_hash"]   = dll.ok ? dll.hash : std::string();
    j["dll_offer_size"]   = dll.ok ? dll.size : 0;

    const std::string payload = j.dump();
    for (const auto& rt : NodeRegistry::Snapshot()) {
        if (!rt.enabled) continue;
        if (!NodeRegistry::IsUp(rt.id)) continue;
        NodeRegistry::SendControl(rt.id, payload);
    }
}

}  // namespace

void NodeRegistry::Tick() {
    // A DLL dropped in while the master runs has to reach nodes without a
    // restart -- a restart is exactly what a DLL-only deploy is trying to avoid.
    if (AgentArtifact::Refresh()) BroadcastArtifactOffers();

    // Re-resolve configured hostnames (a DynDNS name for a node that cannot be
    // inferred from its link -- e.g. it reaches the master over a VPN). A literal
    // address needs none of this; an unset one falls back to address_observed.
    const std::int64_t now_ms = NowMs();
    for (auto& kv : g_nodes) {
        NodeRuntime& rt = kv.second;
        if (rt.address.empty() || NetAddress::IsIPv4Literal(rt.address)) continue;
        if (now_ms - rt.address_resolved_at_ms < kAddressRefreshMs) continue;
        rt.address_resolved_at_ms = now_ms;

        const std::string resolved = ResolveIPv4(rt.address);
        if (resolved.empty()) {
            // Keep whatever we resolved last time: a DNS hiccup must not stop
            // placements. Log once per refresh so a permanently bad name shows up.
            Logger::Log("node",
                "[NodeRegistry] %s: cannot resolve '%s'%s\n", Describe(kv.first).c_str(),
                rt.address.c_str(),
                rt.address_resolved.empty() ? " -- no address to advertise"
                                            : " -- keeping the previous resolution");
            continue;
        }
        if (resolved != rt.address_resolved) {
            Logger::Log("node", "[NodeRegistry] %s: '%s' resolves to %s%s\n",
                Describe(kv.first).c_str(), rt.address.c_str(), resolved.c_str(),
                rt.address_resolved.empty() ? "" : " (was " + rt.address_resolved + ")");
            rt.address_resolved = resolved;
        }
    }


    const int64_t now = NowMs();
    for (auto& kv : g_nodes) {
        NodeRuntime& rt = kv.second;
        if (rt.health != NodeHealth::Up && rt.health != NodeHealth::Stale) continue;
        if (!rt.link || !rt.link->IsOpen()) continue;

        const int64_t silence = now - rt.last_status_ms;
        if (silence > 2 * NodeMsg::kStaleAfterMs) {
            Logger::Log("node", "[NodeRegistry] %s silent for %lldms -- dropping the link\n",
                Describe(rt.id).c_str(), (long long)silence);
            Detach(rt.id, "heartbeat timeout");
            continue;
        }
        if (silence > NodeMsg::kStaleAfterMs && rt.health == NodeHealth::Up) {
            rt.health = NodeHealth::Stale;
            Logger::Log("node",
                "[NodeRegistry] %s silent for %lldms -- marking stale (no new work)\n",
                Describe(rt.id).c_str(), (long long)silence);
        }
    }
}

void NodeRegistry::OnFrame(int node_id, std::uint8_t kind, std::uint64_t stream,
                           std::string payload) {
    if (kind == NodeLink::kFrameControl) {
        HandleControl(node_id, payload);
        return;
    }
    if (kind != NodeLink::kFrameInstance) {
        Logger::Log("node", "[NodeRegistry] %s sent unknown frame kind %u -- ignoring\n",
            Describe(node_id).c_str(), (unsigned)kind);
        return;
    }

    const int64_t instance_id = static_cast<int64_t>(stream);
    auto pipe = StreamFor(node_id, instance_id);
    if (!pipe) {
        Logger::Log("node",
            "[NodeRegistry] %s sent instance bytes for instance=%lld with no live link -- dropping\n",
            Describe(node_id).c_str(), (long long)instance_id);
        return;
    }
    pipe->Feed(std::move(payload));
}

void NodeRegistry::HandleControl(int node_id, const std::string& payload) {
    auto it = g_nodes.find(node_id);
    if (it == g_nodes.end()) return;
    NodeRuntime& rt = it->second;

    nlohmann::json j =
        nlohmann::json::parse(payload, nullptr, /*allow_exceptions=*/false,
                              /*ignore_comments=*/true);
    if (j.is_discarded() || !j.is_object()) {
        Logger::Log("node", "[NodeRegistry] %s sent malformed control JSON -- ignoring\n",
            Describe(node_id).c_str());
        return;
    }

    // Token bucket, checked after parsing (parsing a status heartbeat is the
    // common case and must not be starved by the accounting).
    {
        const int64_t now = NowMs();
        if (rt.msg_refilled_ms == 0) {
            rt.msg_tokens = kMsgBurst;
            rt.msg_refilled_ms = now;
        } else {
            const double elapsed_s = static_cast<double>(now - rt.msg_refilled_ms) / 1000.0;
            rt.msg_tokens += elapsed_s * kMsgPerSec;
            if (rt.msg_tokens > kMsgBurst) rt.msg_tokens = kMsgBurst;
            rt.msg_refilled_ms = now;
        }
        if (rt.msg_tokens < 1.0) {
            ++rt.msg_dropped;
            if (rt.msg_dropped == 1) {
                Logger::Log("node",
                    "[NodeRegistry] %s is sending control messages faster than the "
                    "budget (%.0f/s) -- dropping\n",
                    Describe(node_id).c_str(), kMsgPerSec);
            }
            if (rt.msg_dropped > kMsgDropLimit) {
                Logger::Log("node",
                    "[NodeRegistry] %s dropped %lld messages -- disconnecting it\n",
                    Describe(node_id).c_str(), (long long)rt.msg_dropped);
                Detach(node_id, "control message flood");
            }
            return;
        }
        rt.msg_tokens -= 1.0;
    }

    const std::string type = j.value("type", "");

    if (type == NodeMsg::kNodeStatus) {
        rt.last_status_ms = NowMs();
        rt.msg_dropped = 0;   // heartbeats are the recovery signal
        if (rt.health == NodeHealth::Stale) {
            rt.health = NodeHealth::Up;
            Logger::Log("node", "[NodeRegistry] %s heartbeating again\n",
                Describe(node_id).c_str());
        }
        const std::string prev_version = rt.agent_version;
        rt.content_hash  = j.value("content_hash", rt.content_hash);
        rt.agent_version = j.value("agent_version", rt.agent_version);
        rt.dll_name      = j.value("dll_name", rt.dll_name);
        rt.dll_hash      = j.value("dll_hash", rt.dll_hash);
        rt.agent_hash    = j.value("agent_hash", rt.agent_hash);
        if (rt.agent_version != prev_version) {
            // Once per change, not once per heartbeat.
            const auto& mine = AgentArtifact::Agent();
            Logger::Log("node", "[NodeRegistry] %s runs agent %s, proxy %s, dll %s, content %s\n",
                Describe(node_id).c_str(),
                rt.agent_version.empty() ? "?" : rt.agent_version.c_str(),
                rt.dll_name.empty() ? "<none>" : rt.dll_name.c_str(),
                rt.dll_hash.empty() ? "?" : rt.dll_hash.c_str(),
                rt.content_hash.empty() ? "?" : rt.content_hash.c_str());
            // Say whether it has actually converged on our bytes, and in which
            // direction -- a rollback shows up here as "behind" turning into
            // "matches" once the node restarts on the older build.
            if (!rt.agent_hash.empty() && mine.ok) {
                const char* verdict = (rt.agent_hash == mine.hash) ? "matches" : "differs from";
                Logger::Log("node", "[NodeRegistry] %s agent bytes %s this master's\n",
                    Describe(node_id).c_str(), verdict);
            }
        }

        rt.instances.clear();
        if (j.contains("instances") && j["instances"].is_array()) {
            for (const auto& v : j["instances"]) {
                if (v.is_number_integer()) {
                    rt.instances.push_back(v.get<int64_t>());
                }
            }
        }
        return;
    }

    if (type == NodeMsg::kInstanceSpawning) {
        const int64_t instance_id = j.value("instance_id", (int64_t)0);
        if (!OwnsInstance(node_id, instance_id)) return;
        const auto udp_port = static_cast<std::uint16_t>(j.value("udp_port", 0));
        const int pid = j.value("pid", 0);
        Logger::Log("node", "[NodeRegistry] %s spawned instance=%lld port=%u pid=%d\n",
            Describe(node_id).c_str(), (long long)instance_id, (unsigned)udp_port, pid);
        InstanceRegistry::UpdateUdpPort(instance_id, udp_port);
        if (pid > 0) InstanceRegistry::UpdatePid(instance_id, pid);
        if (g_hooks.on_spawning) g_hooks.on_spawning(node_id, instance_id, udp_port, pid);
        return;
    }

    if (type == NodeMsg::kInstanceFailed) {
        const int64_t instance_id = j.value("instance_id", (int64_t)0);
        if (!OwnsInstance(node_id, instance_id)) return;
        const std::string reason = j.value("reason", "unspecified");
        Logger::Log("node", "[NodeRegistry] %s failed to start instance=%lld: %s\n",
            Describe(node_id).c_str(), (long long)instance_id, reason.c_str());
        // Tear the tunnel down so the instance's IpcSession unwinds like any other
        // dead instance.
        CloseInstanceStream(node_id, instance_id);
        if (g_hooks.on_failed) g_hooks.on_failed(node_id, instance_id, reason);
        return;
    }

    if (type == NodeMsg::kInstanceExited) {
        const int64_t instance_id = j.value("instance_id", (int64_t)0);
        if (!OwnsInstance(node_id, instance_id)) return;
        const int exit_code = j.value("exit_code", 0);
        Logger::Log("node", "[NodeRegistry] %s instance=%lld exited (code=%d)\n",
            Describe(node_id).c_str(), (long long)instance_id, exit_code);
        CloseInstanceStream(node_id, instance_id);
        if (g_hooks.on_exited) g_hooks.on_exited(node_id, instance_id, exit_code);
        return;
    }

    if (type == NodeMsg::kPlayerBundle) {
        // A node reporting the rows it finished with. The master is authoritative;
        // Apply(Up) upserts what the instance owned and reconciles only the tables
        // the instance genuinely owns, so a row the master gained mid-match cannot
        // be deleted by a node that never saw it.
        const auto instance_id = j.value("instance_id", (std::int64_t)0);
        const auto character_id = j.value("character_id", (std::int64_t)0);
        if (j.value("direction", std::string()) != "up") {
            Logger::Log("node", "[NodeRegistry] %s sent a non-'up' player bundle -- ignoring\n",
                Describe(node_id).c_str());
            return;
        }
        if (instance_id != 0 && !OwnsInstance(node_id, instance_id)) return;

        std::string err;
        if (!PlayerBundle::Apply(payload, PlayerBundle::Direction::Up, err)) {
            Logger::Log("node", "[NodeRegistry] %s player bundle for character=%lld rejected: %s\n",
                Describe(node_id).c_str(), (long long)character_id, err.c_str());
            return;
        }
        Logger::Log("node", "[NodeRegistry] %s persisted character=%lld from instance=%lld\n",
            Describe(node_id).c_str(), (long long)character_id, (long long)instance_id);
        return;
    }

    if (type == NodeMsg::kFetchContent) {
        const auto& content = ContentSnapshot::Current();
        const std::string want = j.value("hash", "");
        const std::string kind = j.value("kind", std::string("content"));

        nlohmann::json out;
        out["type"] = NodeMsg::kContentChunk;
        out["kind"] = kind;

        // The agent binary and the proxy DLL reuse this transfer: same chunking,
        // same hash verification, same resume-on-reconnect. Only the source file
        // differs.
        if (kind == "agent" || kind == "dll") {
            const auto& info = (kind == "dll") ? AgentArtifact::Dll() : AgentArtifact::Agent();
            if (!info.ok) {
                out["hash"]   = std::string();
                out["offset"] = 0;
                out["eof"]    = true;
                out["error"]  = "master has no " + kind + " artifact";
                SendControl(node_id, out.dump());
                return;
            }
            if (want != info.hash) {
                out["hash"]   = info.hash;
                out["offset"] = 0;
                out["eof"]    = true;
                out["error"]  = "hash mismatch (the master's artifact changed)";
                SendControl(node_id, out.dump());
                return;
            }

            const std::int64_t offset = j.value("offset", (std::int64_t)0);
            std::int64_t len = j.value("len", (std::int64_t)0);
            if (len <= 0 || len > NodeMsg::kMaxContentChunk) len = NodeMsg::kMaxContentChunk;
            if (offset < 0 || offset >= info.size) {
                out["hash"]   = info.hash;
                out["offset"] = offset;
                out["eof"]    = true;
                out["error"]  = "offset past the end of the artifact";
                SendControl(node_id, out.dump());
                return;
            }
            if (offset + len > info.size) len = info.size - offset;

            std::string bytes, err;
            if (!AgentArtifact::ReadChunk(kind == "dll", offset, len, bytes, err)) {
                out["hash"]   = info.hash;
                out["offset"] = offset;
                out["eof"]    = true;
                out["error"]  = err;
                SendControl(node_id, out.dump());
                return;
            }
            out["hash"]   = info.hash;
            out["offset"] = offset;
            out["eof"]    = (offset + static_cast<std::int64_t>(bytes.size()) >= info.size);
            out["data"]   = Base64::Encode(
                reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
            SendControl(node_id, out.dump());
            return;
        }

        if (!content.ok) {
            out["hash"]  = std::string();
            out["offset"] = 0;
            out["eof"]   = true;
            out["error"] = "master has no content snapshot";
            SendControl(node_id, out.dump());
            return;
        }
        if (want != content.hash) {
            out["hash"]  = content.hash;
            out["offset"] = 0;
            out["eof"]   = true;
            out["error"] = "hash mismatch (the master's snapshot changed)";
            SendControl(node_id, out.dump());
            return;
        }

        const std::int64_t offset = j.value("offset", (std::int64_t)0);
        std::int64_t len = j.value("len", (std::int64_t)0);
        if (len <= 0 || len > NodeMsg::kMaxContentChunk) len = NodeMsg::kMaxContentChunk;

        if (offset < 0 || offset >= content.size) {
            out["hash"]   = content.hash;
            out["offset"] = offset;
            out["eof"]    = true;
            out["error"]  = "offset past the end of the snapshot";
            SendControl(node_id, out.dump());
            return;
        }
        // The peer asks for a fixed chunk size and has no way to know the final
        // remainder, so clamp here. Refusing the overlong request would reject the
        // tail of every transfer.
        if (offset + len > content.size) len = content.size - offset;

        std::string bytes, err;
        if (!ContentSnapshot::ReadChunk(offset, len, bytes, err)) {
            out["hash"]   = content.hash;
            out["offset"] = offset;
            out["eof"]    = true;
            out["error"]  = err;
            SendControl(node_id, out.dump());
            return;
        }

        out["hash"]   = content.hash;
        out["offset"] = offset;
        out["eof"]    = (offset + static_cast<std::int64_t>(bytes.size()) >= content.size);
        out["data"]   = Base64::Encode(
            reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
        SendControl(node_id, out.dump());
        return;
    }

    if (type == NodeMsg::kPing) {
        nlohmann::json pong;
        pong["type"] = NodeMsg::kPong;
        pong["ts"]   = NowMs();
        SendControl(node_id, pong.dump());
        return;
    }

    if (type == NodeMsg::kPong) {
        rt.last_status_ms = NowMs();
        return;
    }

    if (type == NodeMsg::kError) {
        rt.last_error = j.value("text", "unspecified");
        Logger::Log("node", "[NodeRegistry] %s reported an error: %s\n",
            Describe(node_id).c_str(), rt.last_error.c_str());
        return;
    }

    // Anything else (an instance message smuggled onto the control stream, or an
    // admin action -- neither of which a node may originate) is refused.
    Logger::Log("node", "[NodeRegistry] %s sent unhandled message type '%s' -- ignoring\n",
        Describe(node_id).c_str(), type.c_str());
}

// ---------------------------------------------------------------------------
// Instance tunnel streams
// ---------------------------------------------------------------------------

std::shared_ptr<NodeLink::NodeStreamFramePipe>
NodeRegistry::StreamFor(int node_id, int64_t instance_id) {
    const auto key = std::make_pair(node_id, instance_id);
    auto it = g_streams.find(key);
    if (it != g_streams.end()) return it->second;

    auto node_it = g_nodes.find(node_id);
    if (node_it == g_nodes.end() || !node_it->second.link) return nullptr;

    auto pipe = std::make_shared<NodeLink::NodeStreamFramePipe>(
        node_it->second.link, instance_id, node_it->second.name);
    g_streams[key] = pipe;

    Logger::Log("node",
        "[NodeRegistry] %s: adopting IPC stream for instance=%lld\n",
        Describe(node_id).c_str(), (long long)instance_id);
    IpcServer::AdoptPipe(pipe);
    return pipe;
}

void NodeRegistry::CloseInstanceStream(int node_id, int64_t instance_id) {
    const auto key = std::make_pair(node_id, instance_id);
    auto it = g_streams.find(key);
    if (it == g_streams.end()) return;
    auto pipe = it->second;
    g_streams.erase(it);
    if (pipe) pipe->Close();
}

void NodeRegistry::CloseStreamsFor(int node_id) {
    std::vector<std::shared_ptr<NodeLink::NodeStreamFramePipe>> doomed;
    for (auto it = g_streams.begin(); it != g_streams.end();) {
        if (it->first.first == node_id) {
            doomed.push_back(it->second);
            it = g_streams.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& pipe : doomed) {
        if (pipe) pipe->Close();
    }
    if (!doomed.empty()) {
        Logger::Log("node", "[NodeRegistry] %s: closed %zu instance stream(s)\n",
            Describe(node_id).c_str(), doomed.size());
    }
}
