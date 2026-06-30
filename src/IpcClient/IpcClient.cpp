#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcFraming.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Shared/HexUtils.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/ChangeTeam/ChangeTeam.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/SpawnBot/SpawnBot.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/Deploy/Deploy.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/PossessPawn/PossessPawn.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/ReturnHomeArea/ReturnHomeArea.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/TopDown/TopDown.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/Coords/Coords.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Globals.hpp"

namespace {

// PvP gate for cheat-style chat actions. -spawnfriend/-spawnenemy, -deployfriend/
// -deployenemy, -possess/-unpossess, -topdown are dev/test-facing and would let
// one player grief the other in a PvP match. -changeteam is intentionally NOT
// gated — it's the team-balance escape hatch.
//
// Returns false on any null in the chain: a missing GRI means "match not yet
// initialized" (lobby / map transition), which is not a PvP context. r_bIsPVP
// defaults to 0 in TgGame__InitGameRepInfo so this matches the engine default.
bool CurrentMatchIsPVP() {
    ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
    if (!Game) return false;
    ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
    if (!GRI) return false;
    return GRI->r_bIsPVP != 0;
}

} // namespace

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

CRITICAL_SECTION        IpcClient::outbound_cs_;
std::deque<std::string> IpcClient::outbound_queue_;

CRITICAL_SECTION        IpcClient::inbound_cs_;
std::deque<std::string> IpcClient::inbound_queue_;

std::string             IpcClient::host_;
uint16_t                IpcClient::port_         = 0;
int64_t                 IpcClient::instance_id_  = 0;

asio::io_context*       IpcClient::io_ctx_       = nullptr;
bool                    IpcClient::write_in_progress_ = false;
asio::ip::tcp::socket*  IpcClient::socket_       = nullptr;

std::array<uint8_t, 4> IpcClient::header_buf_;

// ---------------------------------------------------------------------------
// RAII guard for CRITICAL_SECTION
// ---------------------------------------------------------------------------
struct IpcCsGuard {
    CRITICAL_SECTION& cs;
    IpcCsGuard(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
    ~IpcCsGuard() { LeaveCriticalSection(&cs); }
};

namespace {

struct PendingProfileAssemblyPulse {
    FCustomCharacterAssembly assembly;
    int item_profile_id;
    uint64_t refresh_token;
};

std::unordered_map<std::string, PendingProfileAssemblyPulse> g_pending_profile_assembly_pulses;

void MarkProfileAssemblyDirty(ATgPawn_Character* pawn) {
    if (pawn == nullptr) return;
    pawn->bNetDirty = 1;
    pawn->bForceNetUpdate = 1;

    auto* pri = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
    if (pri != nullptr) {
        pri->bNetDirty = 1;
        pri->bForceNetUpdate = 1;
    }
}

void CopyAssemblyToPawnAndPri(ATgPawn_Character* pawn, const FCustomCharacterAssembly& assembly) {
    if (pawn == nullptr) return;
    pawn->r_CustomCharacterAssembly = assembly;

    auto* pri = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
    if (pri != nullptr) {
        pri->r_CustomCharacterAssembly = assembly;
    }

    MarkProfileAssemblyDirty(pawn);
}

void LogProfileAssemblyPulse(const char* tag, const std::string& guid, ATgPawn_Character* pawn,
                             const FCustomCharacterAssembly& assembly, uint64_t refresh_token) {
    Logger::Log("loadout",
        "[IPC] profile_assembly_pulse %s guid=%s pawn=%p pawnId=%d itemProf=%d token=%llu "
        "head=%d helmet=%d suit=%d suitFlair=%d trail=%d dyes=%d,%d,%d,%d,%d\n",
        tag,
        guid.c_str(),
        pawn,
        pawn ? (int)pawn->r_nPawnId : 0,
        pawn ? (int)pawn->r_nItemProfileId : 0,
        (unsigned long long)refresh_token,
        assembly.HeadFlairId,
        assembly.HelmetMeshId,
        assembly.SuitMeshId,
        assembly.SuitFlairId,
        assembly.JetpackTrailId,
        assembly.DyeList[0],
        assembly.DyeList[1],
        assembly.DyeList[2],
        assembly.DyeList[3],
        assembly.DyeList[4]);
}

void BeginProfileAssemblyPulse(ATgPawn_Character* pawn, const std::string& guid, int item_profile_id,
                               uint64_t refresh_token) {
    if (pawn == nullptr) return;

    const FCustomCharacterAssembly real = pawn->r_CustomCharacterAssembly;
    g_pending_profile_assembly_pulses[guid] = PendingProfileAssemblyPulse{real, item_profile_id, refresh_token};

    // Cheap workaround: we have not found a safe way to recreate the original
    // live-pawn appearance refresh on profile switch. Force a replicated
    // assembly diff, then restore it so same-instance jetpack dyes repaint.
    FCustomCharacterAssembly pulse = real;
    pulse.HeadFlairId = -1;
    pulse.HelmetMeshId = -1;
    pulse.SuitFlairId = -1;
    pulse.JetpackTrailId = 0;
    for (int i = 0; i < 5; ++i) {
        pulse.DyeList[i] = 0;
    }

    CopyAssemblyToPawnAndPri(pawn, pulse);
    LogProfileAssemblyPulse("begin", guid, pawn, pulse, refresh_token);
}

void FinishProfileAssemblyPulse(ATgPawn_Character* pawn, const std::string& guid, int item_profile_id,
                                uint64_t refresh_token) {
    if (pawn == nullptr) return;

    auto it = g_pending_profile_assembly_pulses.find(guid);
    if (it == g_pending_profile_assembly_pulses.end()) {
        Logger::Log("loadout",
            "[IPC] profile_assembly_pulse finish guid=%s skipped: no pending pulse\n",
            guid.c_str());
        return;
    }

    const PendingProfileAssemblyPulse pending = it->second;
    if (pending.refresh_token != refresh_token || pending.item_profile_id != item_profile_id) {
        Logger::Log("loadout",
            "[IPC] profile_assembly_pulse finish guid=%s skipped: stale pulse pendingProf=%d msgProf=%d pendingToken=%llu msgToken=%llu pawnProf=%d\n",
            guid.c_str(), pending.item_profile_id, item_profile_id,
            (unsigned long long)pending.refresh_token,
            (unsigned long long)refresh_token,
            (int)pawn->r_nItemProfileId);
        return;
    }

    g_pending_profile_assembly_pulses.erase(it);

    if (item_profile_id > 0 && pawn->r_nItemProfileId != item_profile_id) {
        Logger::Log("loadout",
            "[IPC] profile_assembly_pulse finish guid=%s skipped: pawn profile changed msgProf=%d pawnProf=%d token=%llu\n",
            guid.c_str(), item_profile_id, (int)pawn->r_nItemProfileId,
            (unsigned long long)refresh_token);
        return;
    }

    CopyAssemblyToPawnAndPri(pawn, pending.assembly);
    LogProfileAssemblyPulse("finish", guid, pawn, pending.assembly, refresh_token);
}

}  // namespace

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void IpcClient::Init(const std::string& host, uint16_t port, int64_t instance_id) {
    host_ = host;
    port_ = port;
    instance_id_ = instance_id;

    InitializeCriticalSection(&outbound_cs_);
    InitializeCriticalSection(&inbound_cs_);

    Logger::Log("ipc", "[IPC] Init: connecting to %s:%d\n", host_.c_str(), (int)port_);

    CreateThread(NULL, 0, IpcThread, NULL, 0, NULL);
}

// ---------------------------------------------------------------------------
// IpcThread -- reconnect loop with exponential backoff
// ---------------------------------------------------------------------------

DWORD WINAPI IpcClient::IpcThread(LPVOID) {
    int backoff_secs = 2;

    while (true) {
        asio::io_context io;
        asio::ip::tcp::socket sock(io);

        {
            IpcCsGuard lock(outbound_cs_);
            io_ctx_ = &io;
        }
        socket_ = &sock;
        write_in_progress_ = false;

        // Resolve and connect synchronously.
        asio::error_code ec;
        asio::ip::tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host_, std::to_string(port_), ec);

        if (ec) {
            Logger::Log("ipc", "[IPC] Resolve failed: %s\n", ec.message().c_str());
        } else {
            asio::connect(sock, endpoints, ec);
            if (ec) {
                Logger::Log("ipc", "[IPC] Connect failed: %s\n", ec.message().c_str());
            } else {
                // Connected successfully -- reset backoff.
                backoff_secs = 2;
                Logger::Log("ipc", "[IPC] Connected to %s:%d\n", host_.c_str(), (int)port_);

                // Send initial PING.
                nlohmann::json ping_msg;
                ping_msg["type"] = IpcProtocol::MSG_PING;
                Send(ping_msg.dump());

                // Send INSTANCE_HELLO to identify this game instance.
                nlohmann::json hello;
                hello["type"]        = IpcProtocol::MSG_INSTANCE_HELLO;
                hello["instance_id"] = instance_id_;
                Send(hello.dump());
                Logger::Log("ipc", "[IPC] Sent INSTANCE_HELLO: instance_id=%lld\n", instance_id_);

                // Arm the first async read.
                do_read_header();

                // Run the ASIO event loop -- blocks until socket error or close.
                io.run();
            }
        }

        // Disconnected or failed to connect.
        {
            IpcCsGuard lock(outbound_cs_);
            io_ctx_ = nullptr;
        }
        socket_ = nullptr;
        write_in_progress_ = false;

        Logger::Log("ipc", "[IPC] Disconnected. Reconnecting in %ds...\n", backoff_secs);
        Sleep(backoff_secs * 1000);
        backoff_secs = backoff_secs < 30 ? backoff_secs * 2 : 30;

        io.restart();
    }

    return 0;
}

// ---------------------------------------------------------------------------
// do_read_header -- async read of 4-byte LE length prefix
// ---------------------------------------------------------------------------

void IpcClient::do_read_header() {
    asio::async_read(
        *socket_,
        asio::buffer(header_buf_),
        asio::transfer_exactly(4),
        [](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                Logger::Log("ipc", "[IPC] Read header error: %s\n", ec.message().c_str());
                return; // io.run() will exit, triggering reconnect.
            }

            // Decode LE uint32 length.
            uint32_t len =
                  (static_cast<uint32_t>(header_buf_[0])      )
                | (static_cast<uint32_t>(header_buf_[1]) <<  8)
                | (static_cast<uint32_t>(header_buf_[2]) << 16)
                | (static_cast<uint32_t>(header_buf_[3]) << 24);

            do_read_body(len);
        }
    );
}

// ---------------------------------------------------------------------------
// do_read_body -- async read of payload
// ---------------------------------------------------------------------------

void IpcClient::do_read_body(uint32_t len) {
    auto body_buf = std::make_shared<std::vector<uint8_t>>(len);

    asio::async_read(
        *socket_,
        asio::buffer(*body_buf),
        asio::transfer_exactly(len),
        [body_buf](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                Logger::Log("ipc", "[IPC] Read body error: %s\n", ec.message().c_str());
                return; // io.run() will exit, triggering reconnect.
            }

            // Push into inbound queue for game-thread drain.
            std::string msg(reinterpret_cast<const char*>(body_buf->data()), body_buf->size());
            {
                IpcCsGuard lock(inbound_cs_);
                inbound_queue_.push_back(std::move(msg));
            }

            // Chain next read.
            do_read_header();
        }
    );
}

// ---------------------------------------------------------------------------
// Send -- game-thread enqueue + write kick
// ---------------------------------------------------------------------------

void IpcClient::Send(const std::string& json_msg) {
    IpcCsGuard lock(outbound_cs_);
    outbound_queue_.push_back(json_msg);

    // Post kick_write onto the ASIO thread if the io_context is live.
    // io_ctx_ is protected by outbound_cs_ to prevent use-after-free:
    // without the lock, the game thread could read a valid io_ctx_ pointer,
    // then the IPC thread disconnects and destroys the stack-local io_context,
    // and asio::post would write to freed memory — corrupting the heap.
    if (io_ctx_ != nullptr) {
        asio::post(*io_ctx_, kick_write);
    }
}

void IpcClient::SendRequestSuccessor() {
    nlohmann::json msg;
    msg["type"]        = IpcProtocol::MSG_REQUEST_SUCCESSOR;
    msg["instance_id"] = instance_id_;
    Send(msg.dump());
    Logger::Log("ipc", "[IPC] Sent REQUEST_SUCCESSOR: instance_id=%lld\n",
        (long long)instance_id_);
}

void IpcClient::SendRequestRebalance() {
    nlohmann::json msg;
    msg["type"]        = IpcProtocol::MSG_REQUEST_REBALANCE;
    msg["instance_id"] = instance_id_;
    Send(msg.dump());
    Logger::Log("team-balance", "[IPC] Sent REQUEST_REBALANCE: instance_id=%lld\n",
        (long long)instance_id_);
}

void IpcClient::SendChatCommandAudit(const std::string& session_guid,
                                     const std::string& command,
                                     const std::string& outcome,
                                     const std::string& details) {
    nlohmann::json ev;
    ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
    ev["subtype"]      = "chat_command_audit";
    ev["instance_id"]  = instance_id_;
    ev["session_guid"] = session_guid;
    ev["command"]      = command;
    ev["outcome"]      = outcome;
    ev["details"]      = details;

    auto info = PlayerRegistry::GetByGuid(session_guid);
    ev["player_name"] = info ? info->player_name : "";

    Send(ev.dump());
}

// ---------------------------------------------------------------------------
// kick_write -- posted to ASIO thread by Send()
// ---------------------------------------------------------------------------

void IpcClient::kick_write() {
    if (!write_in_progress_) {
        write_in_progress_ = true;
        do_write();
    }
}

// ---------------------------------------------------------------------------
// do_write -- ASIO thread only, write_in_progress chain pattern
// ---------------------------------------------------------------------------

void IpcClient::do_write() {
    std::string msg;
    {
        IpcCsGuard lock(outbound_cs_);
        if (outbound_queue_.empty()) {
            write_in_progress_ = false;
            return;
        }
        msg = outbound_queue_.front();
        outbound_queue_.pop_front();
    }

    std::string frame = IpcFraming::Encode(msg);
    auto buf = std::make_shared<std::string>(std::move(frame));

    asio::async_write(
        *socket_,
        asio::buffer(*buf),
        [buf](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                Logger::Log("ipc", "[IPC] Write error: %s\n", ec.message().c_str());
                write_in_progress_ = false;
                return; // io.run() will exit, triggering reconnect.
            }

            // Chain next write.
            do_write();
        }
    );
}

// ---------------------------------------------------------------------------
// DrainInbound -- game-thread only, swap-drain pattern
// ---------------------------------------------------------------------------

void IpcClient::DrainInbound() {
    std::deque<std::string> local;
    {
        IpcCsGuard lock(inbound_cs_);
        local.swap(inbound_queue_);
    }

    for (const std::string& raw : local) {
        auto j = nlohmann::json::parse(raw, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/true);
        if (j.is_discarded()) {
            Logger::Log("ipc", "[IPC] Malformed JSON: %s\n", raw.c_str());
            continue;
        }

        std::string type = j.value("type", "");

        if (type == IpcProtocol::MSG_PONG) {
            Logger::Log("ipc", "[IPC] PONG received\n");
        } else if (type == IpcProtocol::MSG_INSTANCE_HELLO_ACK) {
            bool accepted = j.value("accepted", false);
            bool stats_enabled = j.value("stats_enabled", false);
            MatchStats::SetEnabled(stats_enabled);
            Logger::Log("ipc", "[IPC] INSTANCE_HELLO_ACK: accepted=%d stats=%d\n",
                (int)accepted, (int)stats_enabled);
        } else if (type == IpcProtocol::MSG_PLAYER_REGISTER) {
            std::string guid        = j.value("session_guid", "");
            std::string pname       = j.value("player_name", "");
            int64_t uid             = j.value("user_id", (int64_t)0);
            int64_t char_id         = j.value("character_id", (int64_t)0);
            uint32_t profile_id     = j.value("profile_id", (uint32_t)0);
            int task_force          = j.value("task_force", 1);
            uint64_t register_token = j.value("register_token", uint64_t{0});
            // head_asm_id, gender_type_value_id, morph_data available in payload but not stored
            // in PlayerInfo at registration time -- SpawnPlayerCharacter reads morph_data from
            // the DLL's own sqlite DB (inserted at character creation via old flow).

            PlayerInfo info;
            info.session_guid             = guid;
            info.player_name              = pname;
            info.ip_address               = "";  // Not known yet; set by JOIN when UDP connects
            info.user_id                  = uid;
            info.selected_character_id    = char_id;
            info.selected_profile_id      = profile_id;
            info.task_force               = task_force;
            info.current_item_profile_id  = j.value("current_item_profile_id", 1);
            if (info.current_item_profile_id < 1 || info.current_item_profile_id > 5)
                info.current_item_profile_id = 1;
            info.control_register_token = register_token;

            // Convert player_name to wide string for UE3 APIs.
            info.player_name_w = std::wstring(pname.begin(), pname.end());

            // Skill-tree allocation payload (optional). Consumed at pawn spawn
            // by ReapplyCharacterSkillTree to materialize s_SkillBasedEffectGroups.
            if (j.contains("skills") && j["skills"].is_array()) {
                for (const auto& s : j["skills"]) {
                    SkillAllocation a;
                    a.skill_group_id = s.value("skill_group_id", 0);
                    a.skill_id       = s.value("skill_id", 0);
                    a.points         = s.value("points", 0);
                    if (a.skill_group_id && a.skill_id && a.points > 0)
                        info.skills.push_back(a);
                }
            }
            info.last_respec_at = j.value("last_respec_at", (int64_t)0);

            PlayerRegistry::Register(info);

            Logger::Log("ipc",
                "[IPC] PLAYER_REGISTER: guid=%s profile=%u itemProf=%d char=%lld user=%lld tf=%d skills=%d\n",
                guid.c_str(), profile_id, info.current_item_profile_id,
                char_id, uid, task_force, (int)info.skills.size());

            // Send ACK back to control server.
            // pawn_id is 0 at registration time (pawn not yet spawned -- spawns at JOIN).
            nlohmann::json ack;
            ack["type"]         = IpcProtocol::MSG_PLAYER_REGISTER_ACK;
            ack["session_guid"] = guid;
            ack["success"]      = true;
            ack["pawn_id"]      = 0;
            IpcClient::Send(ack.dump());
        } else if (type == IpcProtocol::MSG_PLAYER_ACTION) {
            std::string guid   = j.value("session_guid", "");
            std::string action = j.value("action", "");

            Logger::Log("chat-command",
                "[ChatCmd][DLL] inbound PLAYER_ACTION guid=%s action=%s\n",
                guid.c_str(), action.c_str());

            if (action == "change_team") {
                std::string target_str;
                bool is_autobalance = false;
                if (j.contains("args") && j["args"].is_object()) {
                    target_str     = j["args"].value("target", "");
                    is_autobalance = j["args"].value("autobalance", false);
                }

                using TgPlayerActions::ChangeTeamCmd::Target;
                Target target;
                if (target_str == "toggle") {
                    target = Target::Toggle;
                } else if (target_str == "attackers") {
                    target = Target::Attackers;
                } else if (target_str == "defenders") {
                    target = Target::Defenders;
                } else {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] change_team guid=%s: invalid target '%s'; dropping\n",
                        guid.c_str(), target_str.c_str());
                    SendChatCommandAudit(guid, "change_team", "ignored",
                        "invalid target '" + target_str + "'");
                    continue;
                }

                TgPlayerActions::ChangeTeamCmd::Execute(guid, target, is_autobalance);
            } else if (action == "spawn_target") {
                if (CurrentMatchIsPVP()) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] spawn_target guid=%s: blocked in PvP match; dropping\n",
                        guid.c_str());
                    continue;
                }
                int bot_id = 0;
                std::string team_str;
                float difficulty_scalar = 0.0f;
                bool henchman = false;
                if (j.contains("args") && j["args"].is_object()) {
                    bot_id            = j["args"].value("bot_id", 0);
                    team_str          = j["args"].value("team", "");
                    difficulty_scalar = j["args"].value("difficulty_scalar", 0.0f);
                    henchman          = j["args"].value("henchman", false);
                }

                if (bot_id <= 0) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] spawn_target guid=%s: invalid bot_id=%d; dropping\n",
                        guid.c_str(), bot_id);
                    SendChatCommandAudit(guid, "spawn_target", "ignored",
                        "invalid bot_id=" + std::to_string(bot_id));
                    continue;
                }

                using TgPlayerActions::SpawnBotCmd::Team;
                Team team;
                if (team_str == "friend") {
                    team = Team::Friend;
                } else if (team_str == "enemy") {
                    team = Team::Enemy;
                } else {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] spawn_target guid=%s: invalid team '%s'; dropping\n",
                        guid.c_str(), team_str.c_str());
                    SendChatCommandAudit(guid, "spawn_target", "ignored",
                        "invalid team '" + team_str + "'");
                    continue;
                }

                TgPlayerActions::SpawnBotCmd::Execute(guid, bot_id, team, difficulty_scalar,
                                                      henchman);
            } else if (action == "deploy_target") {
                if (CurrentMatchIsPVP()) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] deploy_target guid=%s: blocked in PvP match; dropping\n",
                        guid.c_str());
                    continue;
                }
                int deployable_id = 0;
                std::string team_str;
                if (j.contains("args") && j["args"].is_object()) {
                    deployable_id = j["args"].value("deployable_id", 0);
                    team_str      = j["args"].value("team", "");
                }

                if (deployable_id <= 0) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] deploy_target guid=%s: invalid deployable_id=%d; dropping\n",
                        guid.c_str(), deployable_id);
                    SendChatCommandAudit(guid, "deploy_target", "ignored",
                        "invalid deployable_id=" + std::to_string(deployable_id));
                    continue;
                }

                using TgPlayerActions::DeployCmd::Team;
                Team team;
                if (team_str == "friend") {
                    team = Team::Friend;
                } else if (team_str == "enemy") {
                    team = Team::Enemy;
                } else {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] deploy_target guid=%s: invalid team '%s'; dropping\n",
                        guid.c_str(), team_str.c_str());
                    SendChatCommandAudit(guid, "deploy_target", "ignored",
                        "invalid team '" + team_str + "'");
                    continue;
                }

                TgPlayerActions::DeployCmd::Execute(guid, deployable_id, team);
            } else if (action == "possess") {
                if (CurrentMatchIsPVP()) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] possess guid=%s: blocked in PvP match; dropping\n",
                        guid.c_str());
                    continue;
                }
                TgPlayerActions::PossessCmd::ExecutePossess(guid);
            } else if (action == "unpossess") {
                if (CurrentMatchIsPVP()) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] unpossess guid=%s: blocked in PvP match; dropping\n",
                        guid.c_str());
                    continue;
                }
                TgPlayerActions::PossessCmd::ExecuteUnpossess(guid);
            } else if (action == "coords") {
                TgPlayerActions::CoordsCmd::Execute(guid);
            } else if (action == "topdown") {
                if (CurrentMatchIsPVP()) {
                    Logger::Log("chat-command",
                        "[ChatCmd][DLL] topdown guid=%s: blocked in PvP match; dropping\n",
                        guid.c_str());
                    continue;
                }
                float lift_z = 0.0f;
                if (j.contains("args") && j["args"].is_object()) {
                    lift_z = j["args"].value("lift_z", 0.0f);
                }
                TgPlayerActions::TopDownCmd::Execute(guid, lift_z);
            } else if (action == "return_home_area") {
                TgPlayerActions::ReturnHomeAreaCmd::Execute(guid);
            } else if (action == "refresh_profile_ui") {
                const std::string phase = j.value("phase", "");
                const int item_profile_id = j.value("item_profile_id", 0);
                const uint64_t refresh_token = j.value("refresh_token", (uint64_t)0);
                int refreshed = 0;
                for (auto& kv : GClientConnectionsData) {
                    ClientConnectionData& data = kv.second;
                    if (data.SessionGuid != guid || data.Pawn == nullptr) continue;

                    ATgPlayerController* controller =
                        (ATgPlayerController*)data.Pawn->Controller;
                    if (controller == nullptr || controller->Player == nullptr) continue;

                    controller->eventClientResetEquipScreen();
                    ++refreshed;
                }
                Logger::Log("loadout",
                    "[IPC] refresh_profile_ui guid=%s phase=%s itemProf=%d token=%llu refreshed=%d\n",
                    guid.c_str(), phase.c_str(), item_profile_id,
                    (unsigned long long)refresh_token, refreshed);
            } else {
                Logger::Log("chat-command",
                    "[ChatCmd][DLL] PLAYER_ACTION guid=%s: unknown action '%s'; dropping\n",
                    guid.c_str(), action.c_str());
                SendChatCommandAudit(guid, action.empty() ? "unknown" : action, "ignored",
                    "unknown player action");
            }
        } else if (type == IpcProtocol::MSG_PLAYER_LEAVE) {
            // Forwarded by the control server when the client sends
            // PLAYER_LEAVE_GAME (0x0200) — the user clicked Disconnect in the
            // in-game menu. The UDP UNetConnection isn't reaped automatically
            // (close-notify only sets a flag; UNetConnection::Tick timeout is
            // 30s), so we drive UNetConnection::CleanUp by hand on the
            // connection that matches the session_guid. Cleanup removes the
            // entry from GClientConnectionsData, removes from
            // NetDriver->ClientConnections, closes all channels and destroys
            // the attached PlayerController via the engine's CleanUpActor.
            std::string guid = j.value("session_guid", "");
            uint64_t expected_token = j.value("register_token", uint64_t{0});
            if (guid.empty()) {
                Logger::Log("ipc", "[IPC] PLAYER_LEAVE: missing session_guid; dropping\n");
                continue;
            }

            int matches = 0;
            // Snapshot the matching connection pointers — NetConnection__Cleanup
            // erases entries from GClientConnectionsData, so iterating the map
            // and deleting concurrently would invalidate iterators.
            std::vector<UNetConnection*> to_cleanup;
            for (auto& kv : GClientConnectionsData) {
                if (kv.second.SessionGuid == guid) {
                    if (expected_token != 0 &&
                        kv.second.PlayerInfo.control_register_token != expected_token) {
                        Logger::Log("ipc",
                            "[IPC] PLAYER_LEAVE: stale token guid=%s expected=%llu current=%llu; skipping connection 0x%p\n",
                            guid.c_str(), (unsigned long long)expected_token,
                            (unsigned long long)kv.second.PlayerInfo.control_register_token,
                            (UNetConnection*)kv.first);
                        continue;
                    }
                    to_cleanup.push_back((UNetConnection*)kv.first);
                }
            }

            for (UNetConnection* Connection : to_cleanup) {
                Logger::Log("ipc", "[IPC] PLAYER_LEAVE: cleaning up connection 0x%p for guid=%s\n",
                    Connection, guid.c_str());
                NetConnection__Cleanup::Call(Connection);
                matches++;
            }

            Logger::Log("ipc", "[IPC] PLAYER_LEAVE guid=%s token=%llu: cleaned up %d connection(s)\n",
                guid.c_str(), (unsigned long long)expected_token, matches);
        } else {
            Logger::Log("ipc", "[IPC] Unknown message type: %s\n", type.c_str());
        }
    }
}
