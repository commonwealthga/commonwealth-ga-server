#include "src/ControlServer/NodeAgent/NodeAgent.hpp"

#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeAgent/InstanceRelay.hpp"
#include "src/ControlServer/NodeData/NodeDataStore.hpp"
#include "src/ControlServer/NodeData/PlayerBundle.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/NodeControl/NodeControl.hpp"
#include "src/ControlServer/NodeLink/NodeLinkClient.hpp"
#include "src/ControlServer/NodeLink/NodeMessages.hpp"
#include "src/Shared/Base64.hpp"
#include "lib/nlohmann/json.hpp"
#include "monocypher.h"

#include <asio.hpp>

#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <thread>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

namespace {

asio::io_context* g_io = nullptr;

void OnSignal(int /*sig*/) {
    if (g_io) g_io->stop();
}

#ifndef _WIN32
// A pid read back from the database cannot be trusted on its own: the process may
// have exited and the id been recycled. Only signal it if /proc says it is still
// an instance launcher (docker client, or the wine/xvfb tree in bare-metal mode).
bool PidStillLooksLikeInstance(int pid) {
    if (pid <= 0) return false;
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    char buf[4096];
    const std::size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
    std::fclose(f);
    if (n == 0) return false;
    std::string cmd(buf, n);          // argv is NUL-separated
    for (char& c : cmd) {
        if (c == '\0') c = ' ';
    }
    return cmd.find("docker") != std::string::npos
        || cmd.find("wine") != std::string::npos
        || cmd.find("xvfb") != std::string::npos
        || cmd.find("GlobalAgenda") != std::string::npos;
}
#endif

// Both proxy flavours are builds of the same server DLL; which one is installed
// only decides which Windows loader path the game takes. So probe both rather
// than assuming -- the dinput8 build is the preferred one, but existing installs
// (including this project's own long-running server) use version.dll.
constexpr const char* kProxyNames[] = {"dinput8.dll", "version.dll"};

// True when `dll_overrides` ("version=n,b;dinput8=n,b") names this DLL, i.e. when
// wine will actually load it instead of the system copy.
bool OverridesCover(const std::string& overrides, const std::string& dll_name) {
    const std::string base = dll_name.substr(0, dll_name.find('.'));
    std::size_t pos = 0;
    while (pos <= overrides.size()) {
        const std::size_t end = overrides.find(';', pos);
        const std::string entry = overrides.substr(
            pos, end == std::string::npos ? std::string::npos : end - pos);
        const std::size_t eq = entry.find('=');
        std::string name = (eq == std::string::npos) ? entry : entry.substr(0, eq);
        while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) name.erase(name.begin());
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) name.pop_back();
        for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (name == base) return true;
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    return false;
}

std::string HashFile(const std::string& path);   // defined below

std::string HashBytes(const std::string& bytes) {
    std::uint8_t digest[32];
    crypto_blake2b(digest, sizeof(digest),
                   reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (std::uint8_t b : digest) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0F]);
    }
    crypto_wipe(digest, sizeof(digest));
    return out;
}

std::string JoinPath(const std::string& dir, const std::string& name) {
    if (dir.empty() || dir == ".") return name;
    const char last = dir.back();
    if (last == '/' || last == '\\') return dir + name;
    return dir + "/" + name;
}

void EnsureDirExists(const std::string& dir) {
    if (dir.empty() || dir == ".") return;
#ifdef _WIN32
    const int rc = _mkdir(dir.c_str());
#else
    const int rc = mkdir(dir.c_str(), 0755);
#endif
    (void)rc;   // EEXIST is the normal case
}

// Finds the installed proxy next to the game binary and hashes it. Returns the
// file name ("" when neither is present) and fills `hash`.
std::string FindProxyDll(const std::string& game_binary, std::string& hash) {
    hash.clear();
    const auto slash = game_binary.find_last_of("/\\");
    if (slash == std::string::npos) return std::string();

    for (const char* name : kProxyNames) {
        const std::string path = game_binary.substr(0, slash + 1) + name;
        std::FILE* probe = std::fopen(path.c_str(), "rb");
        if (!probe) continue;
        std::fclose(probe);
        hash = HashFile(path);
        return name;
    }
    return std::string();
}

std::string HashFile(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return std::string();

    crypto_blake2b_ctx ctx;
    crypto_blake2b_init(&ctx, 32);
    char buf[64 * 1024];
    std::size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
        crypto_blake2b_update(&ctx, reinterpret_cast<const std::uint8_t*>(buf), n);
    }
    std::fclose(f);

    std::uint8_t digest[32];
    crypto_blake2b_final(&ctx, digest);
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (std::uint8_t b : digest) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0F]);
    }
    crypto_wipe(digest, sizeof(digest));
    return out;
}

// "host:port" -> host, port. Port 0 means unparsable.
bool SplitHostPort(const std::string& addr, std::string& host, std::uint16_t& port) {
    const auto colon = addr.rfind(':');
    if (colon == std::string::npos || colon == 0) return false;
    host = addr.substr(0, colon);
    try {
        const int p = std::stoi(addr.substr(colon + 1));
        if (p <= 0 || p > 65535) return false;
        port = static_cast<std::uint16_t>(p);
    } catch (...) {
        return false;
    }
    return !host.empty();
}

class Agent : public std::enable_shared_from_this<Agent> {
public:
    explicit Agent(const ControlServerConfig& cfg) : cfg_(cfg) {}

    int Run() {
        if (!Init()) return 1;

        g_io = &io_;
        std::signal(SIGINT, OnSignal);
        std::signal(SIGTERM, OnSignal);
        // SIGCHLD is deliberately NOT ignored here: Tick() reaps children with
        // waitpid(WNOHANG) because the master wants exit codes.
        if (!StartListener()) {
            // A node that cannot accept instance IPC cannot host anything. Failing
            // here beats connecting to the master and silently accepting spawns
            // whose instances then never reach it.
            return 1;
        }
        StartLink();
        StartTimers();

        Logger::Log("node", "Node agent '%s' up: master=%s ipc=%s:%d db=%s\n",
            cfg_.node.id.c_str(), cfg_.node.master_addr.c_str(),
            cfg_.ipc_bind.c_str(), (int)cfg_.ipc_port, cfg_.db_path.c_str());

        io_.run();
        Shutdown();
        return 0;
    }

private:
    bool Init() {
        if (cfg_.node.id.empty() || cfg_.node.key.empty() || cfg_.node.master_addr.empty()) {
            Logger::Log("node",
                "role=\"node\" needs a complete \"node\" block: id, key and master_addr\n");
            return false;
        }
        if (!NodeCrypto::KeyFromHex(cfg_.node.key, psk_)) {
            Logger::Log("node", "node.key must be 64 hex characters\n");
            return false;
        }
        // The node's own database. Phase 1 runs the master's schema so the DLL's
        // queries work; Phase 2 replaces it with a content snapshot the master
        // ships, at which point nothing here needs to migrate anything.
        Database::SetDbPath(cfg_.db_path);
        Database::SetBaseDb(cfg_.base_db);
        Database::Init();
        // Player tables come from dbbase/base.db now, so there is no per-node
        // schema upkeep: PlayerSessionStore::Init() is master-side data hygiene
        // and is not called here.
        InstanceRegistry::Init();

        // NOTE: no ClearStaleInstances() here. If the node shares the master's
        // database (the single-machine test setup) that would mark the master's
        // live rows STOPPED.

        if (!NodeControl::EnsureSlotPrefixes(cfg_)) {
            Logger::Log("node", "EnsureSlotPrefixes failed -- aborting\n");
            return false;
        }

        dll_hash_.clear();
        dll_name_ = FindProxyDll(cfg_.game_binary, dll_hash_);
        if (dll_name_.empty()) {
            Logger::Log("node",
                "WARNING: neither dinput8.dll nor version.dll is next to %s -- instances "
                "will start WITHOUT the server DLL and never connect back\n",
                cfg_.game_binary.c_str());
        } else {
            Logger::Log("node", "agent %s, proxy %s (%s)\n", NodeMsg::kAgentVersion,
                dll_name_.c_str(), dll_hash_.empty() ? "unreadable" : dll_hash_.c_str());
            if (!OverridesCover(cfg_.dll_overrides, dll_name_)) {
                // The classic silent failure: the DLL is installed, wine never
                // loads it, and the instance looks alive but has no IPC at all.
                Logger::Log("node",
                    "WARNING: %s is installed but dll_overrides is '%s', which does not "
                    "load it -- add \"%s=n,b\" (or install the other proxy)\n",
                    dll_name_.c_str(), cfg_.dll_overrides.c_str(),
                    dll_name_.substr(0, dll_name_.find('.')).c_str());
            }
        }
        load_local_content_hash();
        if (cfg_.apply_content) {
            Logger::Log("node", "apply_content=on: this node owns its content tables\n");
        }
        return true;
    }

    // ── inbound: the game instance's own IPC ─────────────────────────────────

    bool StartListener() {
        asio::error_code ec;
        acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_);

        asio::ip::address bind_address = asio::ip::make_address(cfg_.ipc_bind, ec);
        if (ec) {
            Logger::Log("node", "invalid ipc_bind '%s': %s\n",
                cfg_.ipc_bind.c_str(), ec.message().c_str());
            return false;
        }
        const asio::ip::tcp::endpoint endpoint(bind_address, cfg_.ipc_port);
        acceptor_->open(endpoint.protocol(), ec);
        if (!ec) acceptor_->set_option(asio::socket_base::reuse_address(true), ec);
        if (!ec) acceptor_->bind(endpoint, ec);
        if (!ec) acceptor_->listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            Logger::Log("node", "IPC listener failed on %s:%d: %s\n",
                cfg_.ipc_bind.c_str(), (int)cfg_.ipc_port, ec.message().c_str());
            if (ec == asio::error::address_in_use) {
                // Usually not "another node": a previous node's spawned instance can
                // hold the inherited listening fd (see the fd sweep in
                // InstanceSpawner::Spawn). Look for a listener, not for a node.
                Logger::Log("node",
                    "  something is already listening on that port. Find it with\n"
                    "    ss -tlnp 'sport = :%d'\n"
                    "  which may be a leftover game instance holding an inherited "
                    "descriptor rather than another node.\n",
                    (int)cfg_.ipc_port);
            }
            acceptor_.reset();
            return false;
        }
        Logger::Log("node", "Waiting for game instances on %s:%d\n",
            cfg_.ipc_bind.c_str(), (int)cfg_.ipc_port);
        DoAccept();
        return true;
    }

    void DoAccept() {
        if (!acceptor_) return;
        auto self = shared_from_this();
        acceptor_->async_accept([this, self](const asio::error_code& ec,
                                             asio::ip::tcp::socket sock) {
            if (!ec) {
                auto relay = std::make_shared<InstanceRelay>(
                    std::move(sock),
                    [this](std::int64_t id, std::string frame) {
                        // Buffered by the client while the link is down; false means
                        // the offline buffer is full, which the relay treats as fatal.
                        if (!link_) return false;
                        return link_->SendInstance(id, std::move(frame));
                    },
                    [this](std::int64_t id, std::shared_ptr<InstanceRelay> relay) {
                        Adopt(id, std::move(relay));
                    },
                    [this](std::int64_t id) { Release(id); });
                relay->Start();
            }
            if (acceptor_) DoAccept();
        });
    }

    // ── inbound: the master ──────────────────────────────────────────────────

    void StartLink() {
        std::string host;
        std::uint16_t port = 0;
        if (!SplitHostPort(cfg_.node.master_addr, host, port)) {
            Logger::Log("node", "cannot parse master_addr '%s' (want host:port)\n",
                cfg_.node.master_addr.c_str());
            return;
        }
        link_ = std::make_shared<NodeLink::Client>(io_);
        link_->Configure(host, port, cfg_.node.id, psk_);
        auto self = shared_from_this();
        link_->Start(
            [this, self](std::uint8_t kind, std::uint64_t stream, std::string payload) {
                OnFrame(kind, stream, std::move(payload));
            },
            [this, self](bool connected, const std::string& detail) {
                if (connected) {
                    Logger::Log("node", "link to master is up\n");
                    SendStatus();
                } else {
                    Logger::Log("node", "link to master is down: %s\n", detail.c_str());
                }
            });
    }

    void OnFrame(std::uint8_t kind, std::uint64_t stream, std::string payload) {
        if (kind == NodeLink::kFrameControl) {
            HandleControl(payload);
            return;
        }
        if (kind == NodeLink::kFrameInstance) {
            Deliver(static_cast<std::int64_t>(stream), payload);
            return;
        }
        Logger::Log("node", "master sent unknown frame kind %u\n", (unsigned)kind);
    }

    void HandleControl(const std::string& payload) {
        auto j = nlohmann::json::parse(payload, nullptr, /*allow_exceptions=*/false,
                                       /*ignore_comments=*/true);
        if (j.is_discarded() || !j.is_object()) {
            Logger::Log("node", "master sent malformed control JSON\n");
            return;
        }
        const std::string type = j.value("type", "");

        if (type == NodeMsg::kWelcome) {
            device_stats_  = j.value("device_stats_enabled", device_stats_);
            effectiveness_ = j.value("effectiveness_enabled", effectiveness_);
            own_node_id_   = j.value("node_id", own_node_id_);
            Logger::Log("node", "handshake accepted by master (node_id=%d stats=%d/%d)\n",
                own_node_id_, (int)device_stats_, (int)effectiveness_);
            maybe_fetch_content(j.value("content_hash", std::string()),
                                j.value("content_size", (std::int64_t)0));
            ReportOrphanedInstances();
            return;
        }
        if (type == NodeMsg::kInstanceSpawn) {
            HandleSpawn(j);
            return;
        }
        if (type == NodeMsg::kInstanceStop) {
            const auto id = j.value("instance_id", (std::int64_t)0);
            const int grace = j.value("grace_seconds", 3);
            StopLocal(id, j.value("reason", "master asked").c_str(), grace);
            return;
        }
        if (type == NodeMsg::kPlayerBundle) {
            handle_player_bundle(j, payload);
            return;
        }

        if (type == NodeMsg::kContentChunk) {
            handle_content_chunk(j);
            return;
        }

        if (type == NodeMsg::kPing) {
            nlohmann::json pong;
            pong["type"] = NodeMsg::kPong;
            pong["ts"]   = static_cast<long long>(std::time(nullptr));
            SendControl(pong.dump());
            return;
        }
        if (type == NodeMsg::kError) {
            Logger::Log("node", "master reported an error: %s\n",
                j.value("text", "unspecified").c_str());
            return;
        }
        Logger::Log("node", "master sent unhandled message type '%s'\n", type.c_str());
    }

    void HandleSpawn(const nlohmann::json& j) {
        const auto instance_id = j.value("instance_id", (std::int64_t)0);
        if (instance_id <= 0) {
            Logger::Log("node", "INSTANCE_SPAWN without a valid instance_id\n");
            return;
        }
        if (pids_.count(instance_id)) {
            Logger::Log("node", "INSTANCE_SPAWN for instance=%lld which is already running\n",
                (long long)instance_id);
            return;
        }

        if (cfg_.apply_content && content_hash_.empty()) {
            // Owning node without content: spawning now would run an instance whose
            // every effect/device query hits empty tables.
            Fail(instance_id, "content snapshot not applied yet");
            return;
        }

        SpawnRequest req;
        req.map_name    = j.value("map_name", "");
        req.game_mode   = j.value("game_mode", "");
        req.instance_id = instance_id;
        req.match_seed  = static_cast<std::uint64_t>(
            j.value("seed", (unsigned long long)0));
        req.queue_id    = static_cast<std::uint32_t>(j.value("queue_id", 0));
        req.is_home_map = j.value("is_home_map", false);
        req.node_id     = 0;   // local: this machine owns the process

        if (req.map_name.empty() || req.game_mode.empty()) {
            Fail(instance_id, "INSTANCE_SPAWN missing map_name/game_mode");
            return;
        }

        const std::string dk = j.value("difficulty_kind", "default");
        if (dk == "id") {
            req.difficulty_type     = eDifficulty::Id;
            req.difficulty_value_id = static_cast<std::uint32_t>(
                j.value("difficulty_value_id", 0));
        } else if (dk == "scalar") {
            req.difficulty_type = eDifficulty::Scalar;
            DifficultyScalar s;
            s.HP  = j.value("difficulty_hp", 100) / 100.0f;
            s.Dmg = j.value("difficulty_dmg", 100) / 100.0f;
            req.difficulty_scalar = s;
        }

        // Port: the master sends 0 meaning "you pick", which is the normal case
        // because ports are a property of this machine.
        req.udp_port = static_cast<std::uint16_t>(j.value("udp_port", 0));
        if (req.udp_port == 0) {
            auto port = InstanceRegistry::AllocatePort(cfg_.udp_port_range.lo,
                                                      cfg_.udp_port_range.hi);
            if (!port) {
                Fail(instance_id, "no UDP ports available on this node");
                return;
            }
            req.udp_port = *port;
        }

        const int bonus = j.value("victory_bonus_lives", 0);
        const int master_node_id = j.value("node_id", 0);

        // Mirror the master's row locally, so the DLL's own ga_instances queries
        // (InitGameRepInfo reads victory_bonus_lives) see it.
        InstanceRegistry::InsertStartingWithId(
            instance_id, req.map_name, req.game_mode, req.udp_port, 0, req.is_home_map,
            req.queue_id, /*predecessor_instance_id=*/0, "OPEN", "",
            master_node_id, req.match_seed, bonus);

        const SpawnOutcome out = NodeControl::Spawn(cfg_, req);
        if (!out.dispatched || out.pid <= 0) {
            Fail(instance_id, out.error.empty() ? "spawn failed" : out.error);
            return;
        }

        pids_[instance_id] = static_cast<int>(out.pid);
        InstanceRegistry::UpdatePid(instance_id, static_cast<int>(out.pid));

        nlohmann::json sp;
        sp["type"]        = NodeMsg::kInstanceSpawning;
        sp["instance_id"] = instance_id;
        sp["udp_port"]    = req.udp_port;
        sp["pid"]         = static_cast<int>(out.pid);
        SendControl(sp.dump());

        Logger::Log("node", "spawned instance=%lld map=%s port=%d pid=%d\n",
            (long long)instance_id, req.map_name.c_str(), (int)req.udp_port,
            static_cast<int>(out.pid));
    }

    void Fail(std::int64_t instance_id, const std::string& reason) {
        Logger::Log("node", "cannot start instance=%lld: %s\n",
            (long long)instance_id, reason.c_str());
        nlohmann::json j;
        j["type"]        = NodeMsg::kInstanceFailed;
        j["instance_id"] = instance_id;
        j["reason"]      = reason;
        SendControl(j.dump());
    }

    // ── outbound ─────────────────────────────────────────────────────────────

    // Lifecycle events must survive a brief outage, so they go through the
    // client's offline buffer rather than being dropped here. SendStatus()
    // deliberately does NOT do this: current state is worthless once replayed.
    void SendControl(const std::string& json) {
        if (!link_) return;
        link_->SendControl(json);
    }

    void SendStatus() {
        if (!link_ || !link_->IsConnected()) return;
        nlohmann::json j;
        j["type"]          = NodeMsg::kNodeStatus;
        j["ts"]            = static_cast<long long>(std::time(nullptr));
        j["slots_used"]    = static_cast<int>(pids_.size());
        j["slots_total"]   = 0;   // 0 = unlimited until capacity limits land
        j["agent_version"] = NodeMsg::kAgentVersion;
        // What the node is actually serving from. A node that trusts its existing
        // database does not manage content tables, but it still knows which
        // snapshot it holds, and that is what the master wants to compare.
        j["content_hash"] = content_hash_.empty() ? held_hash_ : content_hash_;
        j["dll_name"]      = dll_name_;
        j["dll_hash"]      = dll_hash_;
        auto list = nlohmann::json::array();
        for (const auto& kv : pids_) list.push_back(kv.first);
        j["instances"] = std::move(list);
        link_->SendControl(j.dump());
    }

    void Deliver(std::int64_t instance_id, const std::string& encoded_frame) {
        auto it = relays_.find(instance_id);
        if (it == relays_.end() || !it->second) {
            // The master can send before the instance's IPC has connected (e.g. a
            // PLAYER_REGISTER racing the DLL's own startup). Nothing sensible to
            // buffer it into: the DLL will resend state it cares about.
            Logger::Log("node", "master sent bytes for instance=%lld which is not connected\n",
                (long long)instance_id);
            return;
        }
        it->second->Deliver(encoded_frame);
    }

    // ── instance lifecycle ───────────────────────────────────────────────────

    void StopLocal(std::int64_t instance_id, const char* reason, int grace) {
        auto it = pids_.find(instance_id);
        if (it == pids_.end()) {
            Logger::Log("node", "stop requested for instance=%lld which is not running\n",
                (long long)instance_id);
            return;
        }
        auto inst = InstanceRegistry::GetInstanceById(instance_id);
        if (inst) {
            // StopLocally, not Stop: the mirrored row carries the master's node id,
            // which makes IsRemote() true here even though this process owns the
            // pid. NodeControl::Stop would refuse and leave the instance running.
            NodeControl::StopLocally(*inst, reason, grace);
        }
        InstanceRegistry::MarkStopped(instance_id);
        pids_.erase(it);
        Release(instance_id);
    }

    // An instance's IPC connection identified itself. A second connection for
    // the same instance (the DLL reconnected) supersedes the first.
    void Adopt(std::int64_t instance_id, std::shared_ptr<InstanceRelay> relay) {
        auto it = relays_.find(instance_id);
        if (it != relays_.end() && it->second && it->second.get() != relay.get()) {
            Logger::Log("node", "instance=%lld reconnected; dropping the previous relay\n",
                (long long)instance_id);
            auto previous = it->second;
            // Erase first so the old relay's Release() callback is a no-op and
            // cannot remove the relay we are about to insert.
            relays_.erase(it);
            previous->Close("superseded by a new connection");
        }
        relays_[instance_id] = std::move(relay);
    }

    // Called when an instance's IPC connection ends (either side).
    void Release(std::int64_t instance_id) {
        auto it = relays_.find(instance_id);
        if (it != relays_.end()) relays_.erase(it);
    }

    void StartTimers() {
        tick_timer_ = std::make_unique<asio::steady_timer>(io_);
        ScheduleTick();
    }

    void ScheduleTick() {
        tick_timer_->expires_after(std::chrono::milliseconds(NodeMsg::kStatusPeriodMs));
        auto self = shared_from_this();
        tick_timer_->async_wait([this, self](const asio::error_code& ec) {
            if (ec) return;   // cancelled: shutting down
            Tick();
            ScheduleTick();
        });
    }

    void Tick() {
        ReapChildren();
        if (content_apply_pending_ && pids_.empty()) apply_content_if_idle();
        MaybePushPlayers();
        SendStatus();
    }

    // How a node decides to push player rows back.
    //
    // PRAGMA data_version increments when *another* connection commits to this
    // database -- i.e. when the instance's DLL wrote something. Our own writes do
    // not bump it for us, which is exactly the signal wanted: "the instance changed
    // something". One pragma per 5s tick replaces pushing the whole player set on a
    // timer, which meant ~1800 INSERTs on the master's io thread per player per
    // push and showed up as pauses for everyone else.
    //
    // The backstop covers a change we somehow missed; the instance-exit push covers
    // a player who comes and goes between ticks.
    void MaybePushPlayers() {
        if (seated_.empty()) return;

        const int64_t version = DbDataVersion();
        const bool changed = (version != last_data_version_);
        if (changed) last_data_version_ = version;

        if (++push_backstop_ticks_ < kPushBackstopTicks) {
            if (!changed) return;
            push_players_up(0);
            return;
        }
        push_backstop_ticks_ = 0;
        push_players_up(0);
    }

    std::int64_t DbDataVersion() const {
        sqlite3* db = Database::GetConnection();
        if (!db) return 0;
        sqlite3_stmt* stmt = nullptr;
        std::int64_t v = 0;
        if (sqlite3_prepare_v2(db, "PRAGMA data_version", -1, &stmt, nullptr) == SQLITE_OK
            && stmt) {
            if (sqlite3_step(stmt) == SQLITE_ROW) v = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return v;
    }

    void ReapChildren() {
#ifndef _WIN32
        for (auto it = pids_.begin(); it != pids_.end();) {
            const std::int64_t instance_id = it->first;
            const int pid = it->second;
            int status = 0;
            const pid_t reaped = waitpid(pid, &status, WNOHANG);
            if (reaped == 0) {
                ++it;
                continue;
            }
            if (reaped == -1 && errno == EINTR) {
                // A signal interrupted the poll; the child may well still be alive,
                // so keeping the entry is the only safe reading.
                ++it;
                continue;
            }
            if (reaped == pid) {
                const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                Logger::Log("node", "instance=%lld (pid=%d) exited code=%d\n",
                    (long long)instance_id, pid, code);
                nlohmann::json j;
                j["type"]        = NodeMsg::kInstanceExited;
                j["instance_id"] = instance_id;
                j["exit_code"]   = code;
                SendControl(j.dump());
            } else {
                // -1 without EINTR: ECHILD means it was already reaped (or was
                // never ours). Log errno so the cause is never a guess again.
                Logger::Log("node",
                    "instance=%lld (pid=%d) is no longer a child (errno=%d %s) -- no longer "
                    "tracking it\n",
                    (long long)instance_id, pid, errno, std::strerror(errno));
            }
            // Final write-back for this instance's players before it is forgotten.
            push_players_up(instance_id);
            InstanceRegistry::MarkStopped(instance_id);
            Release(instance_id);
            it = pids_.erase(it);
        }
#endif
    }

    // ── player rows ──────────────────────────────────────────────────────────
    // The master ships a player's rows down before the instance is told about
    // them; the node reports them back periodically and when an instance ends, so
    // a loadout change or a spent consumable survives the match.

    void handle_player_bundle(const nlohmann::json& j, const std::string& payload) {
        if (j.value("direction", std::string()) != "down") {
            Logger::Log("node", "ignoring a non-'down' player bundle from the master\n");
            return;
        }
        const auto user_id = j.value("user_id", (std::int64_t)0);
        const auto character_id = j.value("character_id", (std::int64_t)0);
        const auto instance_id = j.value("instance_id", (std::int64_t)0);
        if (user_id <= 0) {
            Logger::Log("node", "player bundle without a user id -- ignoring\n");
            return;
        }

        std::string err;
        if (!PlayerBundle::Apply(payload, PlayerBundle::Direction::Down, err)) {
            Logger::Log("node", "could not store player user=%lld character=%lld: %s\n",
                (long long)user_id, (long long)character_id, err.c_str());
            return;
        }

        SeatedPlayer sp;
        sp.user_id      = user_id;
        sp.character_id = character_id;
        sp.instance_id  = instance_id;
        seated_[character_id] = sp;
        // Baseline after our own apply: our writes do not bump our own view.
        last_data_version_ = DbDataVersion();

        Logger::Log("node", "holding player user=%lld character=%lld for instance=%lld\n",
            (long long)user_id, (long long)character_id, (long long)instance_id);
    }

    // Push the current rows back up. `only_instance` limits it to one instance's
    // players and drops them from tracking afterwards (they are done).
    void push_players_up(std::int64_t only_instance) {
        if (seated_.empty()) return;
        if (!link_ || !link_->IsConnected()) {
            // Periodic pushes are worthless once stale; the next tick will retry.
            return;
        }

        for (auto it = seated_.begin(); it != seated_.end();) {
            const SeatedPlayer sp = it->second;
            if (only_instance != 0 && sp.instance_id != only_instance) {
                ++it;
                continue;
            }

            std::string bundle, err;
            if (!PlayerBundle::Build(sp.user_id, sp.character_id,
                                     PlayerBundle::Direction::Up, bundle, err)) {
                Logger::Log("node", "cannot read back player character=%lld: %s\n",
                    (long long)sp.character_id, err.c_str());
                ++it;
                continue;
            }

            auto msg = nlohmann::json::parse(bundle, nullptr, false, true);
            if (msg.is_discarded()) {
                ++it;
                continue;
            }
            msg["type"]        = NodeMsg::kPlayerBundle;
            msg["instance_id"] = sp.instance_id;
            msg["direction"]   = "up";
            SendControl(msg.dump());

            if (only_instance != 0) {
                it = seated_.erase(it);   // instance is over: this was the last push
            } else {
                ++it;
            }
        }
    }

    // ── content snapshot ─────────────────────────────────────────────────────
    // The immutable game data (asm_*/map_*/mod_*) the instances read constantly.
    // Fetched once per master content change; the node compares hashes rather
    // than downloading on every reconnect.

    void load_local_content_hash() {
        content_dir_ = cfg_.content_dir;
        const std::string marker = JoinPath(content_dir_, "current.content");
        std::FILE* f = std::fopen(marker.c_str(), "rb");
        if (!f) return;
        char buf[128] = {0};
        const std::size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
        std::fclose(f);
        if (n == 0) return;
        std::string hash(buf, n);
        while (!hash.empty() && (hash.back() == '\n' || hash.back() == '\r' || hash.back() == ' ')) {
            hash.pop_back();
        }
        if (hash.empty()) return;
        const std::string path = JoinPath(content_dir_, "content-" + hash + ".db");
        std::FILE* data = std::fopen(path.c_str(), "rb");
        if (!data) {
            Logger::Log("node", "content marker says %s but %s is missing\n",
                hash.c_str(), path.c_str());
            return;
        }
        std::fclose(data);
        held_hash_ = hash;
        Logger::Log("node", "content snapshot %s (%s)\n", hash.c_str(), path.c_str());
        apply_content_if_idle();
    }

    void maybe_fetch_content(const std::string& wanted, std::int64_t size) {
        if (wanted.empty()) return;                    // master has none
        if (wanted == held_hash_) {                     // bytes already here
            apply_content_if_idle();
            return;
        }
        if (wanted == fetching_hash_) {                 // resume interrupted transfer
            if (link_ && link_->IsConnected()) request_content_chunk();
            return;
        }
        if (size <= 0 || size > kMaxContentBytes) {
            Logger::Log("node", "refusing a content snapshot of %lld bytes\n", (long long)size);
            return;
        }
        Logger::Log("node",
            "content snapshot differs (have '%s', master has '%s', %lld KB) -- fetching\n",
            held_hash_.empty() ? "none" : held_hash_.c_str(), wanted.c_str(),
            (long long)(size / 1024));
        fetching_hash_ = wanted;
        fetching_size_ = size;
        fetching_buf_.clear();
        request_content_chunk();
    }

    void request_content_chunk() {
        if (fetching_hash_.empty()) return;
        const std::int64_t have = static_cast<std::int64_t>(fetching_buf_.size());
        std::int64_t want = fetching_size_ - have;
        if (want > NodeMsg::kMaxContentChunk) want = NodeMsg::kMaxContentChunk;
        if (want <= 0) {
            // Everything announced has arrived but no eof chunk was marked.
            finish_content_fetch();
            return;
        }
        nlohmann::json j;
        j["type"]   = NodeMsg::kFetchContent;
        j["hash"]   = fetching_hash_;
        j["offset"] = have;
        j["len"]    = want;
        SendControl(j.dump());
    }

    void handle_content_chunk(const nlohmann::json& j) {
        if (fetching_hash_.empty()) return;             // late chunk: ignore

        const std::string hash = j.value("hash", "");
        const auto offset = j.value("offset", (std::int64_t)-1);
        const std::string error = j.value("error", "");

        if (!error.empty() || hash != fetching_hash_ ||
            offset != static_cast<std::int64_t>(fetching_buf_.size())) {
            Logger::Log("node", "content fetch aborted: %s\n",
                !error.empty() ? error.c_str()
                               : "chunk out of sequence or hash changed");
            reset_content_fetch();
            return;
        }

        std::string bytes;
        if (!Base64::Decode(j.value("data", std::string()), bytes)) {
            Logger::Log("node", "content fetch aborted: malformed base64 chunk\n");
            reset_content_fetch();
            return;
        }
        if (fetching_buf_.size() + bytes.size() > static_cast<std::size_t>(fetching_size_)) {
            Logger::Log("node", "content fetch aborted: master sent more than it announced\n");
            reset_content_fetch();
            return;
        }
        fetching_buf_ += bytes;

        if (!j.value("eof", false)) {
            request_content_chunk();
            return;
        }
        finish_content_fetch();
    }

    void finish_content_fetch() {
        const std::string got = HashBytes(fetching_buf_);
        if (got != fetching_hash_) {
            Logger::Log("node",
                "content fetch FAILED verification: got %s, wanted %s -- discarding\n",
                got.c_str(), fetching_hash_.c_str());
            reset_content_fetch();
            return;
        }
        if (static_cast<std::int64_t>(fetching_buf_.size()) != fetching_size_) {
            Logger::Log("node", "content fetch size mismatch (%zu vs %lld) -- discarding\n",
                fetching_buf_.size(), (long long)fetching_size_);
            reset_content_fetch();
            return;
        }

        EnsureDirExists(content_dir_);
        const std::string path = JoinPath(content_dir_, "content-" + fetching_hash_ + ".db");

        // Content-addressed, so an existing file already holds exactly these bytes
        // (we verified them above). Do not rewrite it: in a single-machine setup
        // the master is serving this very transfer out of that file, and
        // truncating it underneath an in-flight ReadChunk would corrupt both.
        std::FILE* existing = std::fopen(path.c_str(), "rb");
        if (existing) {
            std::fclose(existing);
            Logger::Log("node", "content snapshot %s already on disk -- not rewriting\n",
                path.c_str());
        } else {
            // Write aside and rename, so a crash mid-write cannot leave a
            // truncated file under a name that claims to be complete.
            const std::string tmp = JoinPath(content_dir_, ".content-download.tmp");
            std::FILE* f = std::fopen(tmp.c_str(), "wb");
            if (!f) {
                Logger::Log("node", "cannot write %s -- content fetch discarded\n", tmp.c_str());
                reset_content_fetch();
                return;
            }
            const std::size_t written =
                std::fwrite(fetching_buf_.data(), 1, fetching_buf_.size(), f);
            std::fflush(f);
            std::fclose(f);
            if (written != fetching_buf_.size() || std::rename(tmp.c_str(), path.c_str()) != 0) {
                Logger::Log("node", "could not store %s -- content fetch discarded\n",
                    path.c_str());
                std::remove(tmp.c_str());
                reset_content_fetch();
                return;
            }
        }

        const std::string marker = JoinPath(content_dir_, "current.content");
        std::FILE* m = std::fopen(marker.c_str(), "wb");
        if (m) {
            std::fprintf(m, "%s\n", fetching_hash_.c_str());
            std::fclose(m);
        }

        held_hash_ = fetching_hash_;
        Logger::Log("node", "content snapshot %s ready (%zu KB, %s)\n",
            held_hash_.c_str(), fetching_buf_.size() / 1024, path.c_str());
        reset_content_fetch();
        apply_content_if_idle();
    }

    // Copy the snapshot into this node's own database. Deferred while instances are
    // running: the operation drops and refills tables they are reading.
    void apply_content_if_idle() {
        if (!cfg_.apply_content) return;          // node trusts its existing DB
        if (held_hash_.empty()) return;
        if (held_hash_ == content_hash_) return;  // already applied
        if (!pids_.empty()) {
            if (!content_apply_pending_) {
                Logger::Log("node",
                    "content %s is ready but %zu instance(s) are running; applying it once "
                    "they finish\n", held_hash_.c_str(), pids_.size());
                content_apply_pending_ = true;
            }
            return;
        }

        std::string err;
        if (!NodeDataStore::SyncContent(
                JoinPath(content_dir_, "content-" + held_hash_ + ".db"), held_hash_, err)) {
            Logger::Log("node", "could not apply content %s: %s\n",
                held_hash_.c_str(), err.c_str());
            return;
        }
        content_hash_ = held_hash_;
        content_apply_pending_ = false;
    }

    void reset_content_fetch() {
        fetching_hash_.clear();
        fetching_size_ = 0;
        fetching_buf_.clear();
    }

    // Instances the database says this node owns but that are not running in this
    // process: left behind by an earlier node run (or by a pids_ entry we lost).
    void ReportOrphanedInstances() {
        if (own_node_id_ == 0) return;
        for (const auto& inst : InstanceRegistry::GetInstancesForNode(own_node_id_)) {
            if (pids_.count(inst.instance_id)) continue;
            Logger::Log("node",
                "instance=%lld (state=%s pid=%d port=%d) is recorded against this node but "
                "not running here -- it will be stopped when this node shuts down\n",
                (long long)inst.instance_id, inst.state.c_str(), inst.pid, (int)inst.udp_port);
        }
    }

    // Stop whatever still claims this node, memory or not. Without this a node
    // restarted after a crash leaves its instances running with nobody to reap
    // them -- the operator ends up killing containers by hand.
    void StopOrphanedInstances(std::vector<int>& signalled) {
        if (own_node_id_ == 0) {
            Logger::Log("node",
                "never learned my node_id from the master -- cannot reconcile instances "
                "from the database; only in-memory ones were stopped\n");
            return;
        }
        for (const auto& inst : InstanceRegistry::GetInstancesForNode(own_node_id_)) {
            if (pids_.count(inst.instance_id)) continue;   // already handled above
#ifndef _WIN32
            if (inst.pid > 0 && PidStillLooksLikeInstance(inst.pid)) {
                Logger::Log("node",
                    "stopping orphaned instance=%lld (pid=%d, port=%d)\n",
                    (long long)inst.instance_id, inst.pid, (int)inst.udp_port);
                NodeControl::StopLocally(inst, "node shutting down (orphan)", 2);
                signalled.push_back(inst.pid);
            } else {
                Logger::Log("node",
                    "orphaned instance=%lld (pid=%d) is not running here -- marking STOPPED "
                    "only\n",
                    (long long)inst.instance_id, inst.pid);
            }
#endif
            InstanceRegistry::MarkStopped(inst.instance_id);
        }
    }

    void Shutdown() {
        Logger::Log("node", "shutting down -- stopping %zu instance(s)\n", pids_.size());
        // Last chance to hand back what the instances changed, while the link is
        // still up.
        push_players_up(0);

        std::vector<int> signalled;
        std::vector<std::int64_t> ids;
        ids.reserve(pids_.size());
        for (const auto& kv : pids_) ids.push_back(kv.first);
        for (const auto id : ids) {
            auto inst = InstanceRegistry::GetInstanceById(id);
            if (inst && inst->pid > 0) signalled.push_back(inst->pid);
            StopLocal(id, "node shutting down", 2);
        }

        StopOrphanedInstances(signalled);

        // StopInstanceProcess sends SIGTERM and escalates to SIGKILL from a
        // detached thread after the grace period. Returning from main() before
        // that thread runs would drop the escalation, so wait briefly for each
        // child to actually go away.
#ifndef _WIN32
        for (int slice = 0; slice < 30 && !signalled.empty(); ++slice) {
            bool any_alive = false;
            for (auto it = signalled.begin(); it != signalled.end();) {
                int status = 0;
                const pid_t r = waitpid(*it, &status, WNOHANG);
                if (r == 0) {
                    any_alive = true;
                    ++it;
                } else {
                    it = signalled.erase(it);   // exited, or not ours any more
                }
            }
            if (!any_alive) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!signalled.empty()) {
            Logger::Log("node",
                "%zu instance process(es) still alive after the grace period; leaving them\n",
                signalled.size());
        }
#endif

        if (link_) link_->Stop();
        if (tick_timer_) {
            asio::error_code ignored;
            tick_timer_->cancel();
        }
        if (acceptor_) {
            asio::error_code ignored;
            acceptor_->close(ignored);
        }
        relays_.clear();
        g_io = nullptr;
        Logger::Log("node", "shutdown complete\n");
    }

    const ControlServerConfig& cfg_;
    asio::io_context io_;
    NodeCrypto::Key psk_{};
    std::shared_ptr<NodeLink::Client> link_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<asio::steady_timer> tick_timer_;

    std::map<std::int64_t, std::shared_ptr<InstanceRelay>> relays_;
    std::map<std::int64_t, int> pids_;

    bool device_stats_ = true;
    bool effectiveness_ = true;
    // Learned from NODE_WELCOME: the master's integer id for this node, which is
    // what ga_instances.node_id holds.
    int own_node_id_ = 0;

    struct SeatedPlayer {
        std::int64_t user_id = 0;
        std::int64_t character_id = 0;
        std::int64_t instance_id = 0;
    };
    // Players whose rows this node holds, keyed by character id.
    std::map<std::int64_t, SeatedPlayer> seated_;
    std::int64_t last_data_version_ = 0;
    int push_backstop_ticks_ = 0;
    static constexpr int kPushBackstopTicks = 12;   // 12 x 5s = 60s

    // Content snapshot bookkeeping. content_hash_ is what this node would serve
    // instances from; fetching_* is a transfer in progress.
    std::string content_dir_;
    // held_hash_  -- snapshot bytes on disk; content_hash_ -- what node.db has.
    std::string held_hash_;
    std::string content_hash_;
    bool        content_apply_pending_ = false;
    std::string fetching_hash_;
    std::int64_t fetching_size_ = 0;
    std::string fetching_buf_;
    // Backstop against a master (or a corrupted message) announcing an absurd size.
    static constexpr std::int64_t kMaxContentBytes = 256ll * 1024 * 1024;
    // Which proxy flavour this node has installed ("dinput8.dll" / "version.dll",
    // "" when neither) and its BLAKE2b-256.
    std::string dll_name_;
    std::string dll_hash_;
};

}  // namespace

int NodeAgent::Run(const ControlServerConfig& cfg) {
    auto agent = std::make_shared<Agent>(cfg);
    return agent->Run();
}
