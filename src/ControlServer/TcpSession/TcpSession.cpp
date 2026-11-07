#include <fstream>
#include "src/Shared/VendorRotation.hpp"
#include <cstring>
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/PvpParity/PvpParity.hpp"
#include "src/ControlServer/MatchModeFilter/MatchModeFilter.hpp"
#include <algorithm>
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "src/ControlServer/ChatSession/PlayerCommands.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/ApRewards/ApRewards.hpp"
#include "src/ControlServer/Auth/LoginAuth.hpp"
#include "src/ControlServer/Constants/DeviceIds.hpp"
#include "src/ControlServer/Loadouts/ArmorLoadouts.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/MatchmakingService/TicketInfoEncoder.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "src/ControlServer/MatchmakingService/RuntimeStats.hpp"
#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"
#include "src/ControlServer/MmrService/MmrService.hpp"
#include "src/ControlServer/CombatRating/CombatRating.hpp"
#include "src/ControlServer/SpectatorOverlay/SkillTreeCatalog.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include <set>
#include <cstdio>
#include <map>
#include <array>
#include <cctype>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <mutex>
#ifndef _WIN32
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <cerrno>
#include "src/ControlServer/AchievementService/AchievementService.hpp"
#include "src/ControlServer/MissionLoot/MissionLoot.hpp"
#include "src/ControlServer/QuestStore/TurninCheck.hpp"
#endif
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "src/ControlServer/InstanceAdmission/InstanceAdmission.hpp"
#include "src/ControlServer/Social/AgencyBank.hpp"
#include "src/ControlServer/Social/Friends.hpp"
#include "src/ControlServer/Social/Teams.hpp"
#include "src/ControlServer/Social/Inspect.hpp"
#include "src/ControlServer/Parity/AssignmentMapType.hpp"
#include "src/ControlServer/EconParity/EconParity.hpp"
#include "src/ControlServer/Durability/Durability.hpp"
#include "src/ControlServer/Appearance/Appearance.hpp"
#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/AvaShields/AvaShields.hpp"
#include "src/ControlServer/AvaZones/AvaZones.hpp"
#include "src/ControlServer/MatchmakingService/MatchKey.hpp"
#include "src/ControlServer/Social/AgencyApply.hpp"
#include <cctype>
#include "src/ControlServer/ServiceAgents/ServiceAgents.hpp"
#include "src/ControlServer/AvaCargo/AvaCargo.hpp"

static void deliver_login_mail(int64_t user_id, int64_t character_id, const std::string& name);

// True when the queue has at least one map the account can play — base-game,
// or locked behind a pack in `installed` (ga_user_dlc, launcher/admin-set).
// A pool-less queue is always playable. Used by the GET_TICKET_INFO queue
// filter and the solo MATCH_JOIN gate; the team gate instead requires a map
// the WHOLE party owns (see the MATCH_JOIN handler).
static bool QueuePlayableWithDlc(const QueueConfig& cfg,
                                 const std::vector<int64_t>& installed) {
	if (cfg.map_pool.empty()) return true;
	for (const auto& m : cfg.map_pool) {
		if (!m.dlc_id) return true;
		for (int64_t id : installed)
			if (id == *m.dlc_id) return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// TcpSession static member definitions
// ---------------------------------------------------------------------------

std::mutex TcpSession::sessions_mutex_;
std::map<std::string, std::weak_ptr<TcpSession>> TcpSession::g_sessions_;
std::mutex TcpSession::early_moves_mutex_;
std::map<int64_t, std::vector<TcpSession::EarlyMove>> TcpSession::early_moves_;
std::mutex TcpSession::pending_alliance_mutex_;
std::map<int64_t, int64_t> TcpSession::pending_alliance_invites_;
std::mutex TcpSession::pending_agency_mutex_;
std::map<int64_t, int64_t> TcpSession::pending_agency_invites_;
std::function<void()> TcpSession::on_need_home_map_;
std::function<int64_t(const std::string&, const std::string&, IntroHost&)> TcpSession::on_need_intro_mission_;
std::string TcpSession::s_host_ = "127.0.0.1";
std::string TcpSession::s_public_host_;
uint16_t    TcpSession::s_chat_port_ = 9001;
bool        TcpSession::s_allow_duplicate_account_logins_ = false;
bool        TcpSession::s_require_password_verification_   = true;
bool        TcpSession::s_require_provisioned_accounts_    = false;
std::string TcpSession::s_ban_spoof_mode_                = "silent";
int         TcpSession::s_ban_spoof_fallback_close_sec_  = 0;
int         TcpSession::s_kick_fallback_close_sec_       = 30;
int         TcpSession::s_max_online_players_            = 0;
std::mutex  TcpSession::login_queue_mutex_;
std::set<std::string>   TcpSession::s_admitted_guids_;
std::deque<std::string> TcpSession::s_login_queue_;

// Tuned tighter than ChatSession's helper because this is the gating socket
// for re-login: dead peers MUST be reaped fast or the next attempt hits the
// "duplicate account" rejection. Detection ~25s: 10s idle + 3 probes * 5s.
// Tolerates a ~10s network blip before probing starts; declared dead at 25s.
void TcpSession::EnableKeepAlive() {
    std::error_code ec;
    socket_.set_option(asio::socket_base::keep_alive(true), ec);
    if (ec) {
        Logger::Log("tcp", "[TCP] set keep_alive failed: %s\n", ec.message().c_str());
        return;
    }
#ifndef _WIN32
    int fd = socket_.native_handle();
    int idle = 10, intvl = 5, count = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &idle,  sizeof(idle))  < 0 ||
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)) < 0 ||
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &count, sizeof(count)) < 0) {
        Logger::Log("tcp", "[TCP] TCP_KEEPIDLE/INTVL/CNT tuning failed (errno=%d)\n", errno);
    }
#endif
}

namespace {

constexpr auto kFailedLoginCooldown = std::chrono::seconds(5);
std::mutex g_failed_login_cooldown_mutex;
std::map<std::string, std::chrono::steady_clock::time_point> g_failed_login_cooldowns;

bool IsLoginIpCoolingDown(const std::string& remote_ip) {
    if (remote_ip.empty()) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(g_failed_login_cooldown_mutex);
    auto it = g_failed_login_cooldowns.find(remote_ip);
    if (it == g_failed_login_cooldowns.end()) {
        return false;
    }
    if (it->second <= now) {
        g_failed_login_cooldowns.erase(it);
        return false;
    }
    return true;
}

void MarkFailedLoginIp(const std::string& remote_ip) {
    if (remote_ip.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_failed_login_cooldown_mutex);
    g_failed_login_cooldowns[remote_ip] =
        std::chrono::steady_clock::now() + kFailedLoginCooldown;
}

// Failed-password lockout. A wrong password keeps the connection open so the
// client can retry, which on its own allows unlimited guesses; count failures
// per source IP and per account name and refuse further attempts for a while.
constexpr int  kAuthFailIpLimit      = 5;
constexpr auto kAuthFailIpWindow     = std::chrono::minutes(10);
constexpr int  kAuthFailAccountLimit = 20;
constexpr auto kAuthFailAccountWindow = std::chrono::minutes(15);
constexpr auto kAuthLockDuration     = std::chrono::minutes(10);

struct AuthFailWindow {
    std::deque<std::chrono::steady_clock::time_point> fails;
    std::chrono::steady_clock::time_point locked_until{};
};
std::mutex g_auth_fail_mutex;
std::map<std::string, AuthFailWindow> g_auth_fail_ips;
std::map<std::string, AuthFailWindow> g_auth_fail_accounts;

std::string AuthAccountKey(const std::string& name) {
    std::string key = name;
    for (auto& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return key;
}

bool IsAuthLocked(const std::map<std::string, AuthFailWindow>& table, const std::string& key) {
    auto it = table.find(key);
    return it != table.end() && it->second.locked_until > std::chrono::steady_clock::now();
}

// Returns true when this failure starts a lockout.
bool NoteAuthFail(std::map<std::string, AuthFailWindow>& table, const std::string& key,
                  int limit, std::chrono::steady_clock::duration window) {
    if (key.empty()) return false;
    const auto now = std::chrono::steady_clock::now();
    if (table.size() > 20000) {
        for (auto it = table.begin(); it != table.end();) {
            bool idle = it->second.locked_until <= now &&
                        (it->second.fails.empty() || now - it->second.fails.back() > window);
            it = idle ? table.erase(it) : std::next(it);
        }
    }
    auto& w = table[key];
    while (!w.fails.empty() && now - w.fails.front() > window) w.fails.pop_front();
    w.fails.push_back(now);
    if (static_cast<int>(w.fails.size()) < limit) return false;
    w.fails.clear();
    w.locked_until = now + kAuthLockDuration;
    return true;
}

bool IsAuthLockedOut(const std::string& remote_ip, const std::string& account) {
    std::lock_guard<std::mutex> lock(g_auth_fail_mutex);
    return IsAuthLocked(g_auth_fail_ips, remote_ip) ||
           IsAuthLocked(g_auth_fail_accounts, AuthAccountKey(account));
}

// The per-ACCOUNT lock must not let strangers keep a player out: while it
// is on, only one-time launcher tickets (login_alt_verifier) are accepted for that
// account; the per-IP lock still refuses everything from the guessing address.
bool IsAuthIpLocked(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(g_auth_fail_mutex);
    return IsAuthLocked(g_auth_fail_ips, remote_ip);
}

bool IsAuthAccountLocked(const std::string& account) {
    std::lock_guard<std::mutex> lock(g_auth_fail_mutex);
    return IsAuthLocked(g_auth_fail_accounts, AuthAccountKey(account));
}

void MarkAuthFailure(const std::string& remote_ip, const std::string& account) {
    std::lock_guard<std::mutex> lock(g_auth_fail_mutex);
    if (NoteAuthFail(g_auth_fail_ips, remote_ip, kAuthFailIpLimit, kAuthFailIpWindow)) {
        Logger::Log("tcp", "[TcpSession] login lockout ip=%s (%d failures)\n", remote_ip.c_str(), kAuthFailIpLimit);
    }
    if (!account.empty() &&
        NoteAuthFail(g_auth_fail_accounts, AuthAccountKey(account), kAuthFailAccountLimit, kAuthFailAccountWindow)) {
        Logger::Log("tcp", "[TcpSession] login lockout account=%s (%d failures)\n", account.c_str(), kAuthFailAccountLimit);
    }
}

uint32_t FallbackClassMsgId(uint32_t profile_id) {
    switch (profile_id) {
        case GA_G::PROFILE_ID_ROBOTIC: return 15447;
        case GA_G::PROFILE_ID_ASSAULT: return 15448;
        case GA_G::PROFILE_ID_RECON:   return 15449;
        case GA_G::PROFILE_ID_MEDIC:   return 15450;
        default:                       return 22976;
    }
}

std::string TrimAsciiWhitespace(const std::string& s) {
    size_t first = 0;
    while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) {
        ++first;
    }
    size_t last = s.size();
    while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) {
        --last;
    }
    return s.substr(first, last - first);
}

bool IsPrintableAsciiUsername(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

bool EqualsIgnoreAsciiCase(const std::string& a, const char* b) {
    size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; ++i) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) return false;
    }
    return i == a.size() && b[i] == '\0';
}

uint32_t ResolveClassMsgId(uint32_t profile_id, uint32_t fallback, const char* context) {
    const uint32_t profile_fallback = FallbackClassMsgId(profile_id);
    const uint32_t resolved_fallback =
        (profile_fallback != 22976 || fallback == 0) ? profile_fallback : fallback;
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("tcp", "[TcpSession] %s: no DB for class_msg profile=%u, fallback=%u\n",
            context ? context : "class_msg", profile_id, resolved_fallback);
        return resolved_fallback;
    }

    sqlite3_stmt* stmt = nullptr;
    const int rc = sqlite3_prepare_v2(db,
        "SELECT COALESCE(name_msg_id, 0) FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("tcp", "[TcpSession] %s: class_msg prepare failed profile=%u fallback=%u err=%s\n",
            context ? context : "class_msg", profile_id, resolved_fallback, sqlite3_errmsg(db));
        if (stmt) sqlite3_finalize(stmt);
        return resolved_fallback;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(profile_id));
    uint32_t class_msg_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        class_msg_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    if (class_msg_id == 0) {
        Logger::Log("tcp", "[TcpSession] %s: no class_msg row for profile=%u, fallback=%u\n",
            context ? context : "class_msg", profile_id, resolved_fallback);
        return resolved_fallback;
    }

    return class_msg_id;
}

// Home map is whatever the config says (Dome City hub), not the VR arena.
static std::string g_home_map_name = "Dome3_VR_Arena_P";
static uint32_t g_home_arrival_min_level = 0;  // 0 = off (config home_arrival_min_level)

uint32_t ResolveHomeMapGameId() {
    if (auto info = MapGameInfo::LookupByName(g_home_map_name)) {
        return info->map_game_id;
    }
    return 100005;
}

int ChooseVrHomeTaskForce(const InstanceInfo& home_instance,
                          uint32_t joining_profile_id,
                          const std::string& joining_session_guid) {
    if (home_instance.map_name != "Dome3_VR_Arena_P") {
        return 1;
    }

    struct TeamComposition {
        int players = 0;
        int same_class = 0;
    };

    TeamComposition team1;
    TeamComposition team2;
    for (const auto& row : InstanceRegistry::GetActivePlayersForInstance(home_instance.instance_id)) {
        if (row.guid == joining_session_guid) {
            continue;
        }

        TeamComposition& team = (row.task_force == 2) ? team2 : team1;
        team.players += 1;
        if (joining_profile_id != 0 && row.profile_id == joining_profile_id) {
            team.same_class += 1;
        }
    }

    int task_force = 1;
    if (team2.players < team1.players) {
        task_force = 2;
    } else if (team1.players == team2.players && team2.same_class < team1.same_class) {
        task_force = 2;
    }

    Logger::Log("tcp",
        "[TcpSession] VR join team balance: instance=%lld profile=%u team1=(players=%d class=%d) team2=(players=%d class=%d) -> tf=%d\n",
        (long long)home_instance.instance_id, joining_profile_id,
        team1.players, team1.same_class, team2.players, team2.same_class, task_force);
    return task_force;
}

bool SendProfileUiRefresh(int64_t instance_id, const std::string& session_guid, int item_profile_id,
                          uint64_t refresh_token, const char* reason) {
    if (instance_id == 0) {
        return false;
    }

    nlohmann::json action;
    action["type"] = IpcProtocol::MSG_PLAYER_ACTION;
    action["session_guid"] = session_guid;
    action["action"] = "refresh_profile_ui";
    action["item_profile_id"] = item_profile_id;
    action["refresh_token"] = refresh_token;
    action["phase"] = reason ? reason : "unknown";
    const bool ok = IpcServer::SendToInstance(instance_id, action.dump());
    Logger::Log("loadout",
        "[TcpSession] profile_switch refresh_profile_ui %s instance=%lld send=%d guid=%s itemProf=%d token=%llu\n",
        reason ? reason : "unknown", (long long)instance_id, (int)ok, session_guid.c_str(),
        item_profile_id, (unsigned long long)refresh_token);
    return ok;
}

}  // namespace

void TcpSession::SetHomeMapName(const std::string& name) { g_home_map_name = name; }
void TcpSession::SetHomeArrivalMinLevel(uint32_t level) { g_home_arrival_min_level = level; }

void TcpSession::SetHomeMapSpawner(std::function<void()> cb) {
    on_need_home_map_ = std::move(cb);
}

// Tutorial chapters
std::vector<std::string> TcpSession::intro_chain_;
std::vector<int64_t>     TcpSession::intro_chain_users_;

void TcpSession::SetIntroChain(const std::string& first_map, const std::vector<std::string>& chain,
                               const std::vector<int64_t>& user_ids) {
    intro_chain_.clear();
    if (!chain.empty()) {
        intro_chain_.push_back(first_map);
        intro_chain_.insert(intro_chain_.end(), chain.begin(), chain.end());
    }
    intro_chain_users_ = user_ids;
    Logger::Log("tcp", "[TcpSession] Tutorial chapters: %zu maps after %s, accounts=%s\n",
        chain.size(), first_map.c_str(), user_ids.empty() ? "all" : "listed only");
}

// The chapter maps end with Kismet TgSeqAct_RequestMission (Inception 1264 =
// Adrenaline, 1271 = Skylark, 1281 = Agency Zero, 1175 = Dome City); the
// client then sends CHANGE_INSTANCE{0,0}. Send the agent to the next chapter
// in its own solo instance; the last chapter (or any failure) goes home.
bool TcpSession::start_next_tutorial_chapter(const std::string& parent_map, const char* reason,
                                             int64_t parent_instance_id) {
    if (intro_chain_.size() < 2 || !on_need_intro_mission_) return false;
    if (!reason || (std::string(reason) != "client change-instance"
                    && std::string(reason) != "request-mission")) return false;
    if (!intro_chain_users_.empty()
        && std::find(intro_chain_users_.begin(), intro_chain_users_.end(), (int64_t)user_id_) == intro_chain_users_.end())
        return false;
    size_t i = 0;
    while (i < intro_chain_.size() && intro_chain_[i] != parent_map) ++i;
    if (i + 1 >= intro_chain_.size()) return false;
    const std::string& next = intro_chain_[i + 1];
    // Chapter levels. Each finished chapter is a level-up
    // (original: Inception -> 2, Adrenaline -> 3, Skylark -> 4, Agency Zero
    // -> 5 = home arrival floor). Without it the agent stayed level 1 and the
    // game's slot unlock table (Ranged lv2, Offhand1 lv3, Specialty+Offhand2
    // lv4) kept the chapter's gear red/locked. Raised before the spawn so the
    // node seed (ga_characters.xp) and SpawnPlayerCharacter read it.
    const uint32_t chapter_level = (uint32_t)(i + 2);
    if (selected_character_id_ > 0
        && PlayerSessionStore::RaiseCharacterToLevel(selected_character_id_, chapter_level))
        Logger::Log("tcp", "[TcpSession] Tutorial chapter level: char=%lld raised to level %u\n",
            (long long)selected_character_id_, chapter_level);
    IntroHost host;  // the player's own node is asked first
    host.user_id = user_id_;
    host.character_id = selected_character_id_;
    host.session_guid = session_guid_;
    host.parent_instance_id = parent_instance_id;
    const int64_t instance_id = on_need_intro_mission_(next, "TgGame.TgGame_Mission", host);
    Logger::Log("tcp", "[TcpSession] Tutorial chapter %zu/%zu: %s -> %s instance=%lld for %s%s\n",
        i + 2, intro_chain_.size(), parent_map.c_str(), next.c_str(), (long long)instance_id,
        session_guid_.c_str(), host.handover ? " (node handover)" : "");
    if (instance_id == 0) return false;
    chapter_pending_instance_ = instance_id;
    tutorial_quit_ = false;
    travel_in_flight_ = true;
    if (host.handover) {
        // The player's node still runs the chapter being left and hosts one
        // map at a time. Send the client ahead to the new chapter's address now
        // (stock loading screen, the client keeps trying for up to 500 s) so the
        // node can close the old map and start the next one.
        if (auto row = InstanceRegistry::GetInstanceById(instance_id)) {
            if (host.parent_instance_id != 0)
                InstanceRegistry::MarkInstancePlayerLeft(host.parent_instance_id, session_guid_);
            travel_sent_instance_id_ = instance_id;
            travel_sent_ip_ = row->ip_address;
            travel_sent_port_ = row->udp_port;
            send_go_play_to_instance(*row, /*task_force=*/1);
            Logger::Log("tcp", "[TcpSession] Tutorial: %s sent ahead to instance=%lld %s:%d (node switches maps)\n",
                session_guid_.c_str(), (long long)instance_id, row->ip_address.c_str(), (int)row->udp_port);
        }
    }
    travel_to_instance_when_ready(instance_id, /*task_force=*/1, host.wait_s);
    return true;
}

void TcpSession::SetIntroMissionSpawner(std::function<int64_t(const std::string&, const std::string&, IntroHost&)> cb) {
    on_need_intro_mission_ = std::move(cb);
}

void TcpSession::EnsureHomeMapWarm(const char* reason) {
    if (!on_need_home_map_) return;
    auto home = InstanceRegistry::GetHomeInstance();
    if (home) return;  // STARTING or READY already exists; spawner would no-op.
    Logger::Log("tcp", "[TcpSession] EnsureHomeMapWarm: spawning on demand (%s)\n",
        reason ? reason : "unspecified");
    on_need_home_map_();
}

namespace {
// public_host may be a DNS name (dynamic home IP). A background thread
// re-resolves it every 5 min so sessions never block on DNS; the last good
// address is kept when a lookup fails.
std::mutex g_public_addr_mutex;
std::string g_public_addr;

std::string ResolveIpv4(const std::string& name) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (getaddrinfo(name.c_str(), nullptr, &hints, &res) != 0 || !res) {
        return "";
    }
    char buf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr, buf, sizeof(buf));
    freeaddrinfo(res);
    return buf;
}

void StartPublicHostResolver(const std::string& name) {
    std::error_code ec;
    asio::ip::make_address_v4(name, ec);
    if (!ec) {
        std::lock_guard<std::mutex> lk(g_public_addr_mutex);
        g_public_addr = name;  // literal IPv4, nothing to resolve
        return;
    }
    std::thread([name] {
        std::string last;
        for (;;) {
            std::string a = ResolveIpv4(name);
            if (!a.empty() && a != last) {
                {
                    std::lock_guard<std::mutex> lk(g_public_addr_mutex);
                    g_public_addr = a;
                }
                Logger::Log("tcp", "public_host %s -> %s\n", name.c_str(), a.c_str());
                last = a;
            } else if (a.empty()) {
                Logger::Log("tcp", "public_host %s: lookup failed, keeping '%s'\n", name.c_str(), last.c_str());
            }
            std::this_thread::sleep_for(std::chrono::seconds(a.empty() ? 30 : 300));
        }
    }).detach();
}
} // namespace

void TcpSession::SetNetworkConfig(const std::string& host, uint16_t chat_port,
                                  const std::string& public_host) {
    s_host_ = host;
    s_chat_port_ = chat_port;
    s_public_host_ = public_host;
    if (!public_host.empty()) {
        StartPublicHostResolver(public_host);
    }
}

namespace {
// 10/8, 172.16/12, 192.168/16, 127/8, 169.254/16, 100.64/10 (Tailscale CGNAT).
bool IsPrivateClientAddress(const asio::ip::address& a) {
    asio::ip::address_v4 v4;
    if (a.is_v4()) {
        v4 = a.to_v4();
    } else if (a.is_v6() && a.to_v6().is_v4_mapped()) {
        v4 = asio::ip::make_address_v4(asio::ip::v4_mapped, a.to_v6());
    } else {
        return a.is_loopback() || (a.is_v6() && a.to_v6().is_link_local());
    }
    const uint32_t x = v4.to_uint();
    return (x >> 24) == 10 || (x >> 20) == ((172u << 4) | 1) ||
           (x >> 16) == ((192u << 8) | 168) || (x >> 24) == 127 ||
           (x >> 16) == ((169u << 8) | 254) || (x >> 22) == ((100u << 2) | 1);
}
} // namespace

std::string TcpSession::AdvertisedIp(const std::string& ip) {
    if (s_public_host_.empty() || ip != s_host_) {
        return ip;
    }
    std::error_code ec;
    auto ep = socket_.remote_endpoint(ec);
    if (ec || IsPrivateClientAddress(ep.address())) {
        return ip;
    }
    std::lock_guard<std::mutex> lk(g_public_addr_mutex);
    return g_public_addr.empty() ? ip : g_public_addr;
}

void TcpSession::SetLoginPolicy(bool allow_duplicate_account_logins,
                               bool require_password_verification,
                               bool require_provisioned_accounts) {
    s_allow_duplicate_account_logins_ = allow_duplicate_account_logins;
    // provisioned-only mode is meaningless without verification
    s_require_password_verification_  = require_password_verification || require_provisioned_accounts;
    s_require_provisioned_accounts_   = require_provisioned_accounts;
}

void TcpSession::SetModerationConfig(const std::string& ban_spoof_mode,
                                     int ban_spoof_fallback_close_sec,
                                     int kick_fallback_close_sec) {
    s_ban_spoof_mode_               = ban_spoof_mode;
    s_ban_spoof_fallback_close_sec_ = ban_spoof_fallback_close_sec;
    s_kick_fallback_close_sec_      = kick_fallback_close_sec;
}

void TcpSession::SetMaxOnlinePlayers(int max_online_players) {
    s_max_online_players_ = max_online_players;
}

// ---------------------------------------------------------------------------
// Login queue
// ---------------------------------------------------------------------------
// Client side (GlobalAgenda.exe, decoded statically): the GSC_CHARACTER_LIST
// handler 0x109159e0 reads MIN_WAIT_SEC / ACTIVE_POSITION / ENTRY_NUMBER
// (reader 0x10920370). MIN_WAIT_SEC != 0 -> login scene event 0x39 ->
// 0x113d8a80 shows the Login_Queue panel ("Server Is Currently Full",
// "Position in queue: <ACTIVE_POSITION>", "Estimated time: <MIN_WAIT_SEC>",
// Disconnect button). A later normal character list -> event 0x3d -> panel
// closes and character select opens.
static constexpr int kLoginQueueTickSeconds = 3;
static constexpr int kLoginQueueResendTicks = 5;   // refresh the panel every 15 s

void TcpSession::handle_character_list_request() {
    bool admit = false;
    {
        std::lock_guard<std::mutex> lock(login_queue_mutex_);
        if (login_admitted_ || s_max_online_players_ <= 0) {
            admit = true;
        } else if (std::find(s_login_queue_.begin(), s_login_queue_.end(), session_guid_)
                   != s_login_queue_.end()) {
            return;  // already waiting; the tick timer answers
        } else if (s_login_queue_.empty()
                   && (int)s_admitted_guids_.size() < s_max_online_players_) {
            admit = true;
        } else {
            s_login_queue_.push_back(session_guid_);
        }
        if (admit && !session_guid_.empty()) s_admitted_guids_.insert(session_guid_);
    }
    if (admit) {
        login_admitted_ = true;
        send_character_list_response();
        return;
    }
    Logger::Log("tcp", "[LoginQueue] server full (cap %d) - %s (%s) queued\n",
        s_max_online_players_, player_name.c_str(), session_guid_.c_str());
    login_queue_tick(kLoginQueueResendTicks, 0);
}

void TcpSession::login_queue_tick(int ticks_since_send, uint32_t last_pos) {
    if (session_guid_.empty()) return;
    uint32_t pos = 0, len = 0;
    bool admit = false;
    {
        std::lock_guard<std::mutex> lock(login_queue_mutex_);
        auto it = std::find(s_login_queue_.begin(), s_login_queue_.end(), session_guid_);
        if (it == s_login_queue_.end()) return;
        pos = (uint32_t)(it - s_login_queue_.begin()) + 1;
        len = (uint32_t)s_login_queue_.size();
        if (pos == 1 && (int)s_admitted_guids_.size() < s_max_online_players_) {
            s_login_queue_.pop_front();
            s_admitted_guids_.insert(session_guid_);
            admit = true;
        }
    }
    if (admit) {
        login_admitted_ = true;
        Logger::Log("tcp", "[LoginQueue] slot free - admitting %s (%s)\n",
            player_name.c_str(), session_guid_.c_str());
        send_character_list_response();
        return;
    }
    if (pos != last_pos || ticks_since_send >= kLoginQueueResendTicks) {
        // Estimated time: a rough 2 minutes per place in line (min 1).
        send_character_list_queue_response(pos, pos * 2, len);
        ticks_since_send = 0;
    }
    auto self = shared_from_this();
    auto t = std::make_shared<asio::steady_timer>(io_ctx_);
    t->expires_after(std::chrono::seconds(kLoginQueueTickSeconds));
    t->async_wait([self, t, ticks_since_send, pos](std::error_code ec) {
        if (!ec) self->login_queue_tick(ticks_since_send + 1, pos);
    });
}

void TcpSession::login_queue_forget() {
    std::lock_guard<std::mutex> lock(login_queue_mutex_);
    s_admitted_guids_.erase(session_guid_);
    s_login_queue_.erase(std::remove(s_login_queue_.begin(), s_login_queue_.end(), session_guid_),
                         s_login_queue_.end());
    login_admitted_ = false;
}

// ---------------------------------------------------------------------------
// TcpSession registry
// ---------------------------------------------------------------------------

void TcpSession::RegisterSession(const std::string& guid, std::weak_ptr<TcpSession> session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    g_sessions_[guid] = session;
}

void TcpSession::UnregisterSession(const std::string& guid) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    g_sessions_.erase(guid);
}

void TcpSession::handle_socket_disconnect() {
    if (session_row_id_) {
        Database::FinalizeSession(session_row_id_);
        session_row_id_ = 0;
    }
    // Cancel any pending ACK wait timer.
    if (pending_ack_timer_) {
        pending_ack_timer_->cancel();
        pending_ack_timer_.reset();
    }
    if (session_guid_.empty()) {
        return;
    }
    login_queue_forget();
    InstanceAdmission::Leave(session_guid_);

    // Reap the instance-side NetConnection now instead of waiting for the
    // engine's ~30s ConnectionTimeout — a zombie client that still sends UDP
    // heartbeats would never hit that timeout at all. PLAYER_CLOSE (state
    // flip + engine reap), NOT PLAYER_LEAVE: driving NetConnection__Cleanup
    // on a still-live connection crashes (see GSC_CHANGE_INSTANCE below).
    if (assigned_instance_id_ != 0) {
        nlohmann::json close_msg;
        close_msg["type"]           = IpcProtocol::MSG_PLAYER_CLOSE;
        close_msg["session_guid"]   = session_guid_;
        close_msg["register_token"] = active_register_token_;
        const bool ok = IpcServer::SendToInstance(assigned_instance_id_, close_msg.dump());
        Logger::Log("tcp", "[TcpSession] disconnect: PLAYER_CLOSE guid=%s instance=%lld send=%d\n",
            session_guid_.c_str(), (long long)assigned_instance_id_, (int)ok);
        if (!ok) {
            // Instance unreachable — keep the registry honest at least.
            InstanceRegistry::MarkInstancePlayerLeft(assigned_instance_id_, session_guid_);
        }
        assigned_instance_id_  = 0;
        active_register_token_ = 0;
    }

    MatchmakingService::RemovePlayer(session_guid_);
    TeamService::HandleDisconnect(session_guid_);
    UnregisterSession(session_guid_);
    IpcServer::ClearPendingAck(session_guid_);
    PlayerSessionStore::Unregister(session_guid_);
    session_guid_.clear();
}

bool TcpSession::HasLiveSession(const std::string& guid) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(guid);
    return it != g_sessions_.end() && !it->second.expired();
}

int TcpSession::EvictStaleSession(const std::string& guid, const char* reason) {
    std::shared_ptr<TcpSession> s;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(guid);
        if (it != g_sessions_.end()) {
            s = it->second.lock();
        }
    }
    Logger::Log("tcp", "[TcpSession] evicting stale session guid=%s live=%d reason=%s\n",
        guid.c_str(), s ? 1 : 0, reason ? reason : "unspecified");
    if (s) {
        // Single io_context thread — safe to tear the other session down inline.
        // Closing the socket makes its pending do_read fire with an error; the
        // error branch then no-ops because session_guid_ is already cleared.
        s->handle_socket_disconnect();
        std::error_code ignored;
        s->socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        s->socket_.close(ignored);
    } else {
        MatchmakingService::RemovePlayer(guid);
        TeamService::HandleDisconnect(guid);
        UnregisterSession(guid);
        IpcServer::ClearPendingAck(guid);
    }
    // Both branches: guarantee the store row is gone so the caller's
    // GetByPlayerName loop always makes progress.
    PlayerSessionStore::Unregister(guid);
    return s ? 1 : 0;
}

// GIVE_REWARD_DETAILS (0x012C). Client side (GlobalAgenda.exe):
// MarshalReceived switch -> 0x10913840 -> HUD vtable[247] -> end screen
// SetRewardDetails 0x11467ea0, which copies PVE_FLAG, DISPLAY_FLAGS,
// XP/CURRENCY/TOKEN_GAINED, the ELITE_AGENT_* and BOOST_* bonus values and an
// optional loot DATA_SET into m_pXpMarshal; the reader 0x11466290 prints them
// into the Basic Rewards panel. Elite/Boost were the paid upsells - sent as 0.
void TcpSession::SystemMessageToCharacter(int64_t character_id, const std::string& text) {
    std::string guid;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& kv : g_sessions_) {
            auto s = kv.second.lock();
            if (s && character_id != 0 && s->selected_character_id_ == character_id) {
                guid = s->session_guid_;
                break;
            }
        }
    }
    if (!guid.empty()) ChatSession::SystemMessageToGuid(guid, text);
}

void TcpSession::SendRewardDetails(const std::vector<int64_t>& character_ids,
                                   int64_t xp, int64_t credits, int64_t tokens) {
    PlayerSessionStore::PveRewardResult rw;
    rw.characters = character_ids;
    rw.xp = xp; rw.credits = credits; rw.tokens = tokens;
    for (int64_t cid : character_ids) {
        auto& p = rw.paid[cid];
        p.xp = xp; p.credits = credits; p.tokens = tokens;
        p.line = " ";  // nothing known about bonuses: blank line
    }
    SendRewardDetails(rw);
}

// Results screen bottom line = which boosts were applied, no
// amounts (the rows above already show the totals paid): "Boost applied: Booster
// x2, Friday PvE Day x1.5" or "No boost applied". The client patch shows the part
// before ':' and the sources on a second line.
static std::string RewardBonusLine(const PlayerSessionStore::PveRewardResult::Paid& p) {
    if (!p.line.empty()) return p.line;
    std::string s;
    for (const auto& n : p.notes) {
        if (n.xp == 0 && n.credits == 0 && n.tokens == 0) continue;
        if (s.find(n.label) != std::string::npos) continue;
        s += (s.empty() ? "Boost applied: " : ", ") + n.label;
    }
    if (s.empty()) return "No boost applied";
    if (s.size() > 200) s = s.substr(0, 197) + "...";
    return s;
}

void TcpSession::SendRewardDetails(const PlayerSessionStore::PveRewardResult& rw) {
    std::vector<std::shared_ptr<TcpSession>> targets;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& kv : g_sessions_) {
            auto s = kv.second.lock();
            if (!s) continue;
            for (int64_t cid : rw.characters) {
                if (cid != 0 && s->selected_character_id_ == cid) { targets.push_back(s); break; }
            }
        }
    }
    for (auto& s : targets) {
        PlayerSessionStore::PveRewardResult::Paid p;
        auto it = rw.paid.find(s->selected_character_id_);
        if (it != rw.paid.end()) p = it->second;
        else { p.xp = rw.xp; p.credits = rw.credits; p.tokens = rw.tokens; p.agenda_points = rw.agenda_points; }
        const std::string line = RewardBonusLine(p);
        std::vector<uint8_t> r;
        // "DOUBLE_OR_INT_SIGNED" fields go as 4-byte ints on the wire (same as
        // INVENTORY_ID in SEND_INVENTORY); 8-byte doubles desync the parse.
        auto writeInt = [&](uint16_t type, int64_t v) { s->Write4B(r, type, (uint32_t)(int32_t)v); };
        auto writeText = [&](uint16_t type, const std::string& t) { s->WriteString(r, type, t); };
        s->append(r, GA_U::GIVE_REWARD_DETAILS & 0xFF, GA_U::GIVE_REWARD_DETAILS >> 8);
        s->append(r, 14, 0x00);
        s->Write1B(r, GA_T::PVE_FLAG, 1);
        // 0x1000000 = no loot roll to wait for (reader 0x1146640a); without it the
        // bonus panels spin "Getting Loot..." forever since we send no loot rows.
        s->Write4B(r, GA_T::DISPLAY_FLAGS, 0x1000000);
        // Earned = everything this agent was really paid (booster/bonuses
        // included); the Elite/Boost upsell panels get 0.
        writeInt(GA_T::XP_GAINED, p.xp);
        writeInt(GA_T::CURRENCY_GAINED, p.credits);
        s->Write4B(r, GA_T::TOKEN_GAINED, (uint32_t)p.tokens);
        writeInt(GA_T::ELITE_AGENT_XP, 0);
        writeInt(GA_T::ELITE_AGENT_CREDIT, 0);
        writeInt(GA_T::ELITE_AGENT_TOKEN, 0);
        writeInt(GA_T::BOOST_XP, 0);
        writeInt(GA_T::BOOST_CREDIT, 0);
        writeInt(GA_T::BOOST_TOKEN, 0);
        // Read only by the patched client (results screen):
        // bottom bonus line + an Agenda Points row. Unpatched clients ignore them.
        writeText(GA_T::MESSAGE, line);
        writeText(GA_T::NAME_MSG_TEXT, "Earned Agenda Points");
        writeText(GA_T::DESC_MSG_TEXT, std::to_string(p.agenda_points));
        // The client's HUD hands 0x012C only to an end screen that already
        // exists (scene lookup at 0x113a1a01, else dropped), and MISSION_ENDED
        // reaches us before it opens (boss kills open it ~8 s later). Send on
        // the session's io thread at 3 s and again at 10 s; the copy is
        // idempotent.
        for (int delay : {3, 10}) {
            auto t = std::make_shared<asio::steady_timer>(s->io_ctx_, std::chrono::seconds(delay));
            t->async_wait([s, t, r](const asio::error_code& ec) mutable {
                if (!ec) s->send_response(r);
            });
        }
        // The original's chat lines at mission end, in its order.
        using MF = ChatSession::MsgField;
        if (p.xp > 0) ChatSession::SystemMsgIdToGuid(s->session_guid_, 22229, {MF::U(GA_T::XP_VALUE, p.xp)});
        if (p.tokens > 0) ChatSession::SystemMsgIdToGuid(s->session_guid_, 53748, {MF::F(GA_T::CURRENT_VALUE, p.tokens)});
        if (p.credits > 0) ChatSession::SystemMsgIdToGuid(s->session_guid_, 55329, {MF::U(GA_T::CURRENCY_VALUE, p.credits)});
        s->level_lines_if_up();
        Logger::Log("rewards", "[Rewards] GIVE_REWARD_DETAILS -> char=%lld xp=%lld credits=%lld tokens=%lld ap=%lld line=\"%s\"\n",
            (long long)s->selected_character_id_, (long long)p.xp, (long long)p.credits, (long long)p.tokens,
            (long long)p.agenda_points, line.c_str());
    }
}

// Push Complete Mission (1432) quest progress to the online session
// playing `character_id` (on its io thread, like SendRewardDetails).
void TcpSession::SendQuestProgress(int64_t character_id, int quest_id,
                                   int quest_requirement_id, int count) {
    std::shared_ptr<TcpSession> target;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& kv : g_sessions_) {
            auto s = kv.second.lock();
            if (s && character_id != 0 && s->selected_character_id_ == character_id) {
                target = s; break;
            }
        }
    }
    if (!target) return;
    asio::post(target->io_ctx_, [target, quest_id, quest_requirement_id, count]() {
        target->send_quest_requirement_progress_response(quest_id, quest_requirement_id, count);
    });
}

void TcpSession::PushAchievementUnlocks(const std::vector<std::pair<int64_t, int>>& unlocks) {
    std::map<int64_t, std::vector<int>> by_char;
    for (const auto& [cid, id] : unlocks) by_char[cid].push_back(id);
    for (auto& [cid, ids] : by_char) {
        std::shared_ptr<TcpSession> target;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& kv : g_sessions_) {
                auto s = kv.second.lock();
                if (s && cid != 0 && s->selected_character_id_ == cid) { target = s; break; }
            }
        }
        if (!target) continue;
        asio::post(target->io_ctx_, [target, ids]() {
            for (int id : ids) target->send_achievement_notice(id);
            target->send_player_achievements_response();
        });
    }
}

// ---------------------------------------------------------------------------
// DeliverGameEvent -- route IPC GAME_EVENT to the correct TcpSession
// ---------------------------------------------------------------------------

bool TcpSession::RefreshFriendsList(const std::string& session_guid) {
    std::shared_ptr<TcpSession> s;
    { std::lock_guard<std::mutex> lock(sessions_mutex_);
      auto it = g_sessions_.find(session_guid); if (it != g_sessions_.end()) s = it->second.lock(); }
    if (!s) return false;
    s->send_friends_list();
    return true;
}

bool TcpSession::SessionHasCharacter(const std::string& session_guid, int64_t character_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return false;
    auto s = it->second.lock();
    return s && character_id != 0 && s->selected_character_id_ == character_id;
}

void TcpSession::DeliverGameEvent(const std::string& session_guid, const nlohmann::json& j) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            Logger::Log("ipc", "[TcpSession] DeliverGameEvent: no session for guid=%s -- dropped\n",
                session_guid.c_str());
            return;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            Logger::Log("ipc", "[TcpSession] DeliverGameEvent: expired session guid=%s -- dropped\n",
                session_guid.c_str());
            return;
        }
    }

    std::string subtype = j.value("subtype", "");

    if (subtype == "request_mission") {
        session->request_mission_instance_ = j.value("instance_id", (int64_t)0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: request_mission instance=%lld map_game_id=%d guid=%s\n",
            (long long)session->request_mission_instance_, j.value("map_game_id", 0), session_guid.c_str());
        // Route at once. The client's own CHANGE_INSTANCE{0,0}
        // comes ~30 s after the closing cinematic; the instance holds the
        // screen black from its RequestMission hook until travel.
        const int64_t from = session->request_mission_instance_;
        asio::post(session->io_ctx_, [session, from]() {
            if (from == 0 || session->assigned_instance_id_ != from || session->travel_in_flight_) return;
            auto parent = InstanceRegistry::GetInstanceById(from);
            if (!parent || parent->is_home_map || parent->state != "READY") return;
            session->early_route_from_ = from;
            Logger::Log("tcp", "[TcpSession] RequestMission: routing %s from instance=%lld now (not waiting for the client)\n",
                session->session_guid_.c_str(), (long long)from);
            session->route_from_mission_instance(from, "request-mission");
        });
        return;
    }

    if (subtype == "spawn") {
        int pawn_id = j.value("pawn_id", 0);
        // Remembered so control-server-side tooling (-components) can push
        // inventory records without waiting for a game event to carry a pawn.
        if (pawn_id != 0) session->last_pawn_id_ = pawn_id;
        session->item_profile_id_ = j.value("item_profile_id", 0);
        const int64_t character_id = session->selected_character_id_;
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: spawn pawn_id=%d item_profile_id=%d char=%lld guid=%s\n",
            pawn_id, (int)session->item_profile_id_, (long long)character_id, session_guid.c_str());
        int spawn_grants = 0;
        session->level_lines_if_up();  // baseline for this agent (or a level paid while away)
        if (pawn_id != 0 && character_id != 0) {
            // Achievement rewards land in the bag here, before
            // the full push below lists them (DB never leads the client).
            spawn_grants += AchievementService::GrantPendingRewards(character_id);
            std::vector<std::pair<int, int>> loot_items;  // name the mission loot in chat (end screen shows none)
            spawn_grants += MissionLoot::GrantPending(character_id, &loot_items);  // mission-end awarded loot
            for (const auto& li : loot_items) session->received_item_line(li.first, li.second, "quest reward");
            session->send_inventory_clear(pawn_id, character_id);
        }
        session->send_inventory_response(pawn_id, character_id);
        // The pawn's item count was set before these grants: stale, the bag lists 0/0.
        if (spawn_grants > 0) session->sync_item_count(character_id);
        // Push the character's skill-tree allocation so the in-game skill UI
        // renders the saved state. Client caches the response; opening the
        // skill screen then doesn't require a server round-trip. A later
        // SendCharacterSkillMarshal RPC (when supported) can trigger another
        // push if the data becomes stale.
        session->send_player_skills_response();

        // Replay persisted quest state once per character. The client's tracker
        // starts empty on every connect, so without this a finished quest is
        // re-offered by its giver (and re-accepting it fails). Spawn is the
        // point we know the client is fully in-world — same reason inventory and
        // skills go out here.
        if (character_id != 0 && session->quests_synced_for_character_ != character_id) {
            session->quests_synced_for_character_ = character_id;
            session->send_quest_state_sync();
        } else if (character_id != 0) {
            // Later maps (Dome, missions incl. player-hosted ones): the new
            // HUD waits for the loaded flag again.
            session->send_quest_list_loaded();
        }
        // Every spawn (new map or respawn): the HUD's Tracked Assignment panel
        // keeps its placeholder text ("Quest Name") until the client refreshes
        // it, and nothing we send otherwise triggers that refresh.
        if (character_id != 0) {
            session->send_quest_timeframes_refresh();
            session->schedule_quest_tracker_refresh();
        }
    }
    else if (subtype == "beacon_pickup") {
        int pawn_id            = j.value("pawn_id", 0);
        int device_id          = j.value("device_id", 0);
        int inventory_id       = j.value("inventory_id", 0);
        int equip_slot_value_id = j.value("equip_slot_value_id", 0);
        int item_profile_id    = j.value("item_profile_id", 1);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: beacon_pickup pawn=%d dev=%d inv=%d slot=%d profile=%d\n",
            pawn_id, device_id, inventory_id, equip_slot_value_id, item_profile_id);
        session->send_beacon_pickup_response(pawn_id, device_id, inventory_id, equip_slot_value_id, item_profile_id);
    }
    else if (subtype == "beacon_remove") {
        int pawn_id      = j.value("pawn_id", 0);
        int inventory_id = j.value("inventory_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: beacon_remove pawn=%d inv=%d\n",
            pawn_id, inventory_id);
        session->send_beacon_remove_response(pawn_id, inventory_id);
    }
    else if (subtype == "possess_show_loadout") {
        int pawn_id = j.value("pawn_id", 0);
        std::vector<TcpSession::LoadoutItem> items;
        if (j.contains("devices") && j["devices"].is_array()) {
            for (const auto& d : j["devices"]) {
                TcpSession::LoadoutItem it{};
                it.device_id     = d.value("device_id", 0);
                it.inventory_id  = d.value("inventory_id", 0);
                it.quality       = d.value("quality", 1162);
                it.slot_value_id = d.value("slot_value_id", 0);
                if (it.device_id != 0 && it.inventory_id != 0 && it.slot_value_id != 0) {
                    items.push_back(it);
                }
            }
        }
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: possess_show_loadout pawn=%d items=%d\n",
            pawn_id, (int)items.size());
        session->send_loadout_inventory_response(pawn_id, items);
    }
    else if (subtype == "possess_restore_inventory") {
        int     pawn_id      = j.value("pawn_id", 0);
        int64_t character_id = j.value("character_id", (int64_t)0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: possess_restore_inventory pawn=%d char=%lld\n",
            pawn_id, (long long)character_id);
        if (pawn_id != 0 && character_id != 0) {
            session->send_inventory_response(pawn_id, character_id);
        }
    }
    else if (subtype == "inspect_request") {   // X inspect: answer the viewer with the target's Information panel
        const int64_t vc = j.value("character_id", (int64_t)0);
        if (const int64_t tc = Inspect::NoteRequest(session_guid, vc, j.value("source_character_id", (int64_t)0))) session->send_agent_info(tc, tc == vc, true);
    }
    else if (subtype == "billboard_request") {
        session->send_billboard_content(j.value("key", 0));
    }
    else if (subtype == "skill_request") {
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: skill_request guid=%s\n",
            session_guid.c_str());
        session->send_player_skills_response();
    }
    else if (subtype == "skill_save") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        if (j.contains("item_profile_id")) {
            int32_t ipid = j.value("item_profile_id", 0);
            if (ipid >= 0) session->item_profile_id_ = ipid;
        }
        std::vector<SkillRow> skills;
        if (j.contains("skills") && j["skills"].is_array()) {
            for (const auto& s : j["skills"]) {
                SkillRow r{};
                r.skill_group_id = s.value("skill_group_id", 0);
                r.skill_id       = s.value("skill_id", 0);
                r.points         = s.value("points", 0);
                if (r.skill_group_id && r.skill_id && r.points > 0)
                    skills.push_back(r);
            }
        }
        Logger::Log("ipc",
            "[TcpSession] DeliverGameEvent: skill_save char=%lld pawnId=%d rows=%d guid=%s\n",
            (long long)character_id, pawn_id, (int)skills.size(), session_guid.c_str());
        std::string why;
        if (character_id != 0 &&
            !PlayerSessionStore::ValidateSkillAllocation(character_id, skills, why)) {
            // Client-side UI enforces the same rules; a failing save is a
            // modified client or a bug. Keep the DB, echo it back below.
            Logger::Log("ipc", "[TcpSession] skill_save REJECTED char=%lld: %s\n",
                (long long)character_id, why.c_str());
        }
        else if (character_id != 0) {
            PlayerSessionStore::SetSkillsForCharacter(
                character_id, (int)session->item_profile_id_, skills);
            SkillTreeCatalog::InvalidateCache(character_id);
        }
        // Echo current DB state so the UI's inbound 0x00A0 handler has
        // authoritative data whether this was a save or a request.
        session->send_player_skills_response();
		// Avoid a full SEND_INVENTORY blast here. Skill saves can happen while
		// the profile UI is open and the bag pool is 1000+ entries; repeatedly
		// rebuilding it tanks the client. Equipment changes own inventory sync.
    }
    else if (subtype == "skill_respec") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: skill_respec char=%lld pawnId=%d\n",
            (long long)character_id, pawn_id);
        if (character_id != 0) {
            PlayerSessionStore::ClearSkillsForCharacter(
                character_id, (int)session->item_profile_id_);
            SkillTreeCatalog::InvalidateCache(character_id);
        }
        // Echo the zeroed allocation to the client so the UI matches.
        session->send_player_skills_response();
		// Do not resend inventory for respec; skill response is authoritative
		// and equipment changes have their own inventory sync.
    }
    else if (subtype == "equip_save") {
        // Client's equip-screen save (RPC ServerAcceptNewProfileFromEquipScreen).
        // Payload shape:
        //   character_id, pawn_id, loadout_profile (1 today — loadout slots
        //   1..5 are a future-scope feature, we ignore the value for now),
        //   slot_to_inventory: { "equip_point": inventory_id, ... }  -- the
        //                        client's FTGEQUIP_SLOTS_STRUCT.SlotIndices[]
        //                        for engine equip-points 1..24 (weapons +
        //                        cosmetics).
        //   misc_items:         { "index": inventory_id, ... }  -- the
        //                        client's FTGEQUIP_SLOTS_STRUCT.MiscItems[],
        //                        which (hypothesis) carries armor equip
        //                        changes keyed by group-126 sort_order.
        //                        Indexing scheme is the open question for
        //                        Phase 4 swap-armor; this log line + the
        //                        per-entry breakdown above are what we use
        //                        to decode it from real packet data.
        // Validation + persistence lives in PlayerSessionStore.
        const int64_t character_id    = j.value("character_id", (int64_t)0);
        const int     pawn_id         = j.value("pawn_id", 0);
        const int     loadout_profile = j.value("loadout_profile", 1);
        std::map<int, int> slot_to_inventory;
        if (j.contains("slot_to_inventory") && j["slot_to_inventory"].is_object()) {
            for (auto it = j["slot_to_inventory"].begin(); it != j["slot_to_inventory"].end(); ++it) {
                const int equip_point  = std::atoi(it.key().c_str());
                const int inventory_id = it.value().get<int>();
                if (equip_point > 0 && inventory_id > 0) {
                    slot_to_inventory[equip_point] = inventory_id;
                }
            }
        }
        std::map<int, int> misc_items;
        if (j.contains("misc_items") && j["misc_items"].is_object()) {
            for (auto it = j["misc_items"].begin(); it != j["misc_items"].end(); ++it) {
                const int idx          = std::atoi(it.key().c_str());
                const int inventory_id = it.value().get<int>();
                if (inventory_id > 0) {
                    misc_items[idx] = inventory_id;
                }
            }
        }
        // Engine slots (6=Suit, 12=Helmet) the client explicitly blanked —
        // SlotIndices[slot]==0, a genuine "make this empty" signal, not
        // "didn't touch this slot". See CosmeticEquip::ClearSlot.
        std::set<int> cleared_cosmetic_slots;
        if (j.contains("cleared_cosmetic_slots") && j["cleared_cosmetic_slots"].is_array()) {
            for (const auto& v : j["cleared_cosmetic_slots"]) {
                if (v.is_number_integer()) cleared_cosmetic_slots.insert(v.get<int>());
            }
        }
        Logger::Log("armor",
            "[equip_save] char=%lld pawnId=%d loadout=%d slots=%zu misc=%zu cleared=%zu guid=%s\n",
            (long long)character_id, pawn_id, loadout_profile,
            slot_to_inventory.size(), misc_items.size(), cleared_cosmetic_slots.size(),
            session_guid.c_str());
        for (const auto& kv : slot_to_inventory) {
            Logger::Log("armor",
                "[equip_save]   SlotIndices[%d] = inv_id %d\n", kv.first, kv.second);
        }
        for (const auto& kv : misc_items) {
            Logger::Log("armor",
                "[equip_save]   MiscItems[%d]   = inv_id %d\n", kv.first, kv.second);
        }
        for (int slot : cleared_cosmetic_slots) {
            Logger::Log("armor",
                "[equip_save]   ClearedCosmeticSlot[%d]\n", slot);
        }

        if (character_id != 0) {
            const bool ok = PlayerSessionStore::SaveEquippedDevices(
                character_id, session->user_id_, session->selected_profile_id_,
                loadout_profile,
                slot_to_inventory, misc_items, cleared_cosmetic_slots);
            if (!ok) {
                Logger::Log("armor",
                    "[equip_save] REJECTED — DB state unchanged; client will resync from existing\n");
            }
			// Push the authoritative equipped set back regardless of
			// accept/reject — accept = client sees its save; reject = client's
			// optimistic UI gets corrected to what's actually persisted.
			if (pawn_id != 0) {
				session->send_inventory_clear(pawn_id, character_id);
				session->send_inventory_response(pawn_id, character_id);
			}
		}
	}
    else if (subtype == "vendor_purchase") {
        // The instance already charged the wallet + added the
        // ga_players_inventory rows; add just those to the client's bag.
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: vendor_purchase char=%lld pawnId=%d item=%d\n",
            (long long)character_id, pawn_id, j.value("item_id", 0));
        std::vector<int64_t> inv_ids;
        if (j.contains("inventory_ids") && j["inventory_ids"].is_array())
            for (const auto& v : j["inventory_ids"]) if (v.is_number_integer()) inv_ids.push_back(v.get<int64_t>());
        if (character_id != 0 && pawn_id != 0) {
            if (j.value("component", false)) {
                // Component stack grew: restate the stack + r_ItemCount.
                session->send_component_refresh(pawn_id);
                session->sync_item_count(character_id);
            } else {
                session->send_inventory_added(pawn_id, character_id, inv_ids);
            }
            // The original's purchase line, by what was paid in.
            using MF = ChatSession::MsgField;
            const int item = j.value("item_id", 0), count = std::max(1, j.value("count", 1)), price = j.value("price", 0);
            const int cur = j.value("currency", 0);
            const std::string name = MissionLoot::ItemName(item);
            if (!name.empty() && (cur == 1603 || cur == 1604 || cur == 1605)) {
                const uint32_t id = cur == 1603 ? 23701 : cur == 1604 ? 54857 : 64868;
                const uint16_t tag = cur == 1603 ? GA_T::PRICE : cur == 1604 ? GA_T::TOKEN_PRICE : GA_T::HZPOINT_PRICE;
                ChatSession::SystemMsgIdToGuid(session->session_guid_, id, {MF::U(GA_T::ITEM_ID, item), MF::S(GA_T::ITEM_NAME, name),
                    MF::U(GA_T::INSTANCE_COUNT, count), MF::U(tag, price)});
            }
        }
    }
    else if (subtype == "craft_item") {
        // Crafting: recipe + components checked and spent in the DB;
        // the crafted item is added to the bag, spent stacks restated.
        const int64_t character_id = j.value("character_id", (int64_t)0);
        const int     pawn_id      = j.value("pawn_id", 0);
        const auto r = Database::CraftItem(session->user_id_, character_id, j.value("blueprint_id", 0));
        Logger::Log("loot", "[Craft] char=%lld ref=%d blueprint=%d -> %s item=%d rows=%zu depleted=%zu bp_spent=%lld skill=%d:%d (%s)\n",
            (long long)character_id, j.value("blueprint_id", 0), r.blueprint_item_id, r.ok ? "OK" : "REFUSED",
            r.created_item_id, r.inventory_ids.size(), r.depleted.size(), (long long)r.spent_blueprint_row,
            r.skill_id, r.skill_points, r.reason.c_str());
        if (r.ok) EconParity::StampCraftedMod(r.blueprint_item_id, r.inventory_ids);
        if (r.ok && character_id != 0 && pawn_id != 0) {
            if (r.spent_blueprint_row > 0) {
                session->send_inventory_removed(pawn_id, character_id, { r.spent_blueprint_row });
                session->sync_item_count(character_id);
            }
            session->send_component_removals(pawn_id, r.depleted);
            session->send_component_refresh(pawn_id);
            session->send_inventory_added(pawn_id, character_id, r.inventory_ids);
            // 31311 "X (xN) has been created and is of Y quality!", 31370 skill-up.
            using MF = ChatSession::MsgField;
            const std::string made = MissionLoot::ItemName(r.created_item_id);
            if (!made.empty())
                ChatSession::SystemMsgIdToGuid(session->session_guid_, 31311, {MF::U(GA_T::ITEM_ID, r.created_item_id), MF::S(GA_T::ITEM_NAME, made),
                    MF::U(GA_T::INSTANCE_COUNT, std::max<size_t>(r.inventory_ids.size(), 1)), MF::S(GA_T::QUALITY_NAME, MissionLoot::CraftedQualityName(r.inventory_ids, r.created_item_id))});
            if (r.skill_gain > 0 && !r.skill_name.empty())
                ChatSession::SystemMsgIdToGuid(session->session_guid_, 31370, {MF::S(GA_T::SKILL_NAME, r.skill_name),
                    MF::U(GA_T::RANK, r.skill_points), MF::U(GA_T::LEVEL_MAX, 300)});
        }
        // The craft screen waits for the backend's answer (msg 60195 "Db request
        // succeed, ready to close UI scene" / 41694 refused); the instance sends it.
        if (pawn_id != 0) {
            nlohmann::json res;
            res["type"]    = IpcProtocol::MSG_CRAFT_RESULT;
            res["pawn_id"] = pawn_id;
            res["ok"]      = r.ok;
            IpcServer::SendToInstance(j.value("instance_id", (int64_t)0), res.dump());
        }
    }
    else if (subtype == "combine_items") {  // crafted mod onto gear (EconParity)
        const int64_t cid = j.value("character_id", (int64_t)0); const int pid = j.value("pawn_id", 0);
        const auto r = EconParity::CombineItems(session->user_id_, cid, j.value("inventory_id", (int64_t)0), j.value("mod_inventory_id", (int64_t)0));
        if (r.ok && cid != 0 && pid != 0) {
            session->send_inventory_removed(pid, cid, { r.mod_row, r.target_row });
            session->send_inventory_added(pid, cid, { r.target_row });
            session->sync_item_count(cid);
        }
        if (!r.ok) TcpSession::SystemMessageToCharacter(cid, "Modification failed: " + r.reason + ".");
    }
    else if (subtype == "repair_item") {  // Agent Profile Repair (one same-quality kit -> full)
        const int64_t cid = j.value("character_id", (int64_t)0); const int pid = j.value("pawn_id", 0);
        const auto r = Durability::Repair(session->user_id_, cid, j.value("inventory_id", (int64_t)0));
        if (r.ok && cid != 0 && pid != 0) { session->send_inventory_removed(pid, cid, { r.kit_row, r.target_row }); session->send_inventory_added(pid, cid, { r.target_row }); session->sync_item_count(cid); }
        if (!r.ok) TcpSession::SystemMessageToCharacter(cid, "Repair failed: " + r.reason + ".");
    }
    else if (subtype == "salvage_item" || subtype == "activate_item") {
        // Bag Salvage / Activate (instance hooks). The DB checks
        // ownership, equipped, quality / item type; ok -> the row leaves the
        // bag, components (and container drops) are restated; refused -> the
        // row is re-sent in case the client took it out of its list.
        const int64_t character_id = j.value("character_id", (int64_t)0);
        const int     pawn_id      = j.value("pawn_id", 0);
        const int64_t inv_id       = j.value("inventory_id", (int64_t)0);
        const bool    salvage      = subtype == "salvage_item";
        const auto r = salvage ? Database::SalvageItem(session->user_id_, inv_id)
                               : Database::ActivateItem(session->user_id_, inv_id);
        // 0959: a container's auto-created gear gets its row quality + blueprint stat before the client hears of it
        if (!salvage && r.ok && !r.gear.empty()) EconParity::ApplyAutoGear(r.inventory_ids, r.gear);
        std::string got;
        for (const auto& kv : r.components) got += std::to_string(kv.first) + "x" + std::to_string(kv.second) + " ";
        Logger::Log("loot", "[Bag] %s char=%lld inv=%lld item=%d table=%d -> %s components=[%s] bag_rows=%zu (%s)\n",
            salvage ? "salvage" : "activate", (long long)character_id, (long long)inv_id, r.item_id, r.loot_table_id,
            r.ok ? "OK" : "REFUSED", got.c_str(), r.inventory_ids.size() - (salvage && r.ok ? 1 : 0), r.reason.c_str());
        if (character_id != 0 && pawn_id != 0) {
            const std::vector<int64_t> gone{ inv_id };
            if (r.ok) {
                session->send_inventory_removed(pawn_id, character_id, gone);
                // result panel first; the full component push after it keeps the bag totals right
                if (salvage) {
                    session->send_salvaged_items(r.components);
                    // 59255 "X has been salvaged." + 37473 "Y (xN) has been added to inventory."
                    using MF = ChatSession::MsgField;
                    const std::string name = MissionLoot::ItemName(r.item_id);
                    if (!name.empty()) ChatSession::SystemMsgIdToGuid(session->session_guid_, 59255, {MF::S(GA_T::ITEM_NAME, name)});
                    for (const auto& kv : r.components) {
                        const std::string got_name = MissionLoot::ItemName(kv.first);
                        if (!got_name.empty()) ChatSession::SystemMsgIdToGuid(session->session_guid_, 37473, {MF::S(GA_T::ITEM_NAME, got_name), MF::U(GA_T::INSTANCE_COUNT, kv.second)});
                    }
                }
                if (!r.components.empty()) session->send_component_refresh(pawn_id);
                if (!salvage && !r.inventory_ids.empty()) session->send_inventory_added(pawn_id, character_id, r.inventory_ids);
                session->sync_item_count(character_id);
            } else {
                session->send_inventory_added(pawn_id, character_id, gone);
            }
        }
    }
    else if (subtype == "news_stand") {
        // Player walked up to the Dome City Global News stand
        // (instance-side NewsStand, throttled there) -> news feed in their chat
        // + the newest headline as the game's centre-screen alert toast.
        std::string headline;
        const bool sent = ChatSession::SendNewsToGuid(session_guid, &headline);
        if (sent && !headline.empty()) {
            if (headline.size() > 150) headline = headline.substr(0, 147) + "...";
            nlohmann::json action;
            action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
            action["session_guid"] = session_guid;
            action["action"]       = "alert_text";
            action["args"] = {{"text", "Global News: " + headline}, {"priority", 2}, {"type", 1}, {"duration", 10.0f}};
            TcpSession::DeliverPlayerAction(session_guid, action);
        }
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: news_stand guid=%s sent=%d\n",
            session_guid.c_str(), sent ? 1 : 0);
    }
    else if (subtype == "hall_of_legends") {
        // Player walked up to the Dome City AvA Hall of Legends POI
        // (instance-side NewsStand watcher, throttled there) -> leaderboards in chat.
        const bool sent = ChatSession::SendHallOfLegendsToGuid(session_guid);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: hall_of_legends guid=%s sent=%d\n",
            session_guid.c_str(), sent ? 1 : 0);
    }
    else if (subtype == "inventory_removed") {
        // Vendor sell / bag destroy done on the instance. ok -> drop the
        // row from the client's bag; rejected -> re-add it (the client may have
        // removed it optimistically).
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        const bool ok        = j.value("ok", false);
        std::vector<int64_t> inv_ids;
        if (j.contains("inventory_ids") && j["inventory_ids"].is_array())
            for (const auto& v : j["inventory_ids"]) if (v.is_number_integer()) inv_ids.push_back(v.get<int64_t>());
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: inventory_removed char=%lld pawnId=%d ok=%d rows=%zu\n",
            (long long)character_id, pawn_id, ok ? 1 : 0, inv_ids.size());
        if (character_id != 0 && pawn_id != 0) {
            if (ok) session->send_inventory_removed(pawn_id, character_id, inv_ids);
            else    session->send_inventory_added(pawn_id, character_id, inv_ids);
        }
    }
    else if (subtype == "profile_switch") {
        // Game-server has finished its atomic switch (revert + apply + IPC).
        // Control-server's job: persist new active profile, sync TcpSession
        // mirror, push refreshed inventory + skills so the client UI sees
        // the new build's stats and allocations.
        const int64_t character_id    = j.value("character_id", (int64_t)0);
        const int64_t instance_id      = j.value("instance_id", (int64_t)0);
        const int     pawn_id         = j.value("pawn_id", 0);
        const int     item_profile_id = j.value("item_profile_id", 1);
        Logger::Log("loadout",
            "[TcpSession] profile_switch char=%lld pawnId=%d itemProf=%d guid=%s\n",
            (long long)character_id, pawn_id, item_profile_id, session_guid.c_str());

        if (character_id != 0 && item_profile_id >= 1 && item_profile_id <= 5) {
            PlayerSessionStore::SetCurrentItemProfile(character_id, item_profile_id);
            session->item_profile_id_ = item_profile_id;
            // The only way a build changes mid-match (no mid-match respec --
            // see SkillTreeCatalog.hpp) -- drop the cached summary now so the
            // overlay's next poll picks up the new profile's points instead
            // of waiting out the cache's stale-backstop window.
            SkillTreeCatalog::InvalidateCache(character_id);

            if (pawn_id != 0) {
                // Clear-then-re-add so the client's m_InventoryMap entries get
                // fully rebuilt instead of merge-updated. The bar widget binds
                // to the per-profile slot data inside those entries; without
                // the wipe, stale "equipped in profile N" rows survive and
                // the bar keeps rendering old icons on full → empty switches.
                // Cheap (~12 bytes/record, batched) and mirrors the proven
                // disconnect+reconnect refresh shape.
                session->send_inventory_clear(pawn_id, character_id);
                session->send_inventory_response(pawn_id, character_id);
            }
            session->send_player_skills_response();

            if (instance_id != 0) {
                const uint64_t refresh_token = ++session->profile_refresh_token_;
                SendProfileUiRefresh(instance_id, session_guid, item_profile_id, refresh_token, "immediate");
            }
        }
    }
    else if (subtype == "quest") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int quest_id         = j.value("quest_id", 0);
        bool abandon         = j.value("abandon", false);

        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: quest char=%lld quest=%d abandon=%d\n",
            (long long)character_id, quest_id, (int)abandon);

        if (abandon) {
            PlayerSessionStore::Quests().Abandon(character_id, quest_id);
            session->send_quest_abandon_response(quest_id);
            session->schedule_quest_tracker_refresh();
        } else {
            std::string status = PlayerSessionStore::Quests().GetStatus(character_id, quest_id);
            std::string missing;
            const int level_min = Database::LookupQuestLevelMin(quest_id);
            if (status.empty() && !Database::QuestPrerequisitesMet(character_id, quest_id, &missing)) {
                // The client only offers eligible quests; this
                // stops a forged accept skipping a chain (e.g. the Dome exit).
                Logger::Log("quest", "[TcpSession] quest %d accept REFUSED for char=%lld - prerequisite(s) %s not complete\n",
                    quest_id, (long long)character_id, missing.c_str());
            } else if (status.empty() && level_min > 0 &&
                       (int)PlayerSessionStore::GetCharacterLevelXp(character_id).level < level_min) {
                Logger::Log("quest", "[TcpSession] quest %d accept REFUSED for char=%lld - level below %d\n",
                    quest_id, (long long)character_id, level_min);
            } else if (status.empty()) {
                PlayerSessionStore::Quests().Accept(character_id, quest_id);
                session->send_quest_accept_response(quest_id);
                session->schedule_quest_tracker_refresh();
                session->run_quest_followups(character_id, j.value("pawn_id", 0));
            } else if (status == "active") {
                // Turn-in gate: every requirement must be satisfied. Collect
                // requirements read component stock — the same source the
                // client checks — so a player who already farmed the parts
                // hands in on the first interaction.
                std::vector<std::string> unmet;
                if (!Database::AreQuestRequirementsMet(session->user_id_, character_id,
                                                       quest_id, &unmet)) {
                    std::string why;
                    for (const auto& u : unmet) { if (!why.empty()) why += "; "; why += u; }
                    Logger::Log("quest",
                        "[TcpSession] quest %d turn-in REFUSED for char=%lld — %s\n",
                        quest_id, (long long)character_id, why.c_str());
                } else if (TurninCheck::Allow(character_id, quest_id)) {  // turn-in location
                    session->finish_quest(character_id, quest_id, j.value("pawn_id", 0),
                                         j.value("reward_item_id", 0));
                    session->run_quest_followups(character_id, j.value("pawn_id", 0));
                }
            } else {
                // Complete -> a repeatable quest is taken again
                // (dailies once per UTC day); anything else stays done.
                std::string why;
                if (Database::ReacceptRepeatableQuest(character_id, quest_id, &why)) {
                    session->send_quest_accept_response(quest_id);
                    session->schedule_quest_tracker_refresh();
                    session->run_quest_followups(character_id, j.value("pawn_id", 0));
                } else {
                    Logger::Log("quest", "[TcpSession] quest %d already complete for char=%lld, not re-accepted (%s)\n",
                        quest_id, (long long)character_id, why.c_str());
                }
            }
        }
    }
    else if (subtype == "quest_kill_credit") {
        // One bot of `bot_id` died to this player's task force. The DLL sends
        // one event per player on that task force (retail credits kills
        // team-wide — TgPawn.uc passes the TaskForce, not the killer).
        const int64_t character_id = j.value("character_id", (int64_t)0);
        const int     bot_id       = j.value("bot_id", 0);

        if (character_id == 0 || bot_id <= 0) {
            Logger::Log("quest",
                "[TcpSession] quest_kill_credit: bad payload char=%lld bot=%d — ignored\n",
                (long long)character_id, bot_id);
        } else {
            session->level_lines_if_up();
            const auto moved = Database::CreditKillQuestRequirements(character_id, bot_id);
            for (const auto& pr : moved) {
                session->send_quest_requirement_progress_response(
                    pr.quest_id, pr.quest_requirement_id, pr.count);
            }
            if (!moved.empty()) {
                Logger::Log("quest",
                    "[TcpSession] quest_kill_credit char=%lld bot=%d -> %zu requirement(s) updated\n",
                    (long long)character_id, bot_id, moved.size());
            }

            // Component drop: roll the dead bot's loot table and grant what
            // falls out (inventory record + r_ItemCount bump + quest toast).
            // no_loot: the bot came from a no_kill_reward factory (quest credit only).
            const int pawn_id = j.value("pawn_id", 0);
            if (!j.value("no_loot", false))
                session->credit_loot_table(Database::GetBotLootTableId(bot_id),
                                           character_id, pawn_id, "bot kill");
        }
    }
    else if (subtype == "chest_loot") {
        // A placed TgChestActor was opened; the DLL names the opener and the
        // chest's m_nLootTableId. Only 'Chest' tables (1413) are honoured.
        const int64_t character_id  = j.value("character_id", (int64_t)0);
        const int     loot_table_id = j.value("loot_table_id", 0);
        if (character_id == 0 || !Database::IsChestLootTable(loot_table_id)) {
            Logger::Log("loot",
                "[TcpSession] chest_loot: bad payload char=%lld table=%d — ignored\n",
                (long long)character_id, loot_table_id);
        } else {
            session->credit_loot_table(loot_table_id, character_id,
                                       j.value("pawn_id", 0), "chest");
        }
    }
    else if (subtype == "quest_script_credit") {
        // Map Kismet (TgSeqAct_QuestIncrementReqCount) advanced a Script
        // Triggered (1617) step; the DLL names quest + requirement.
        const int64_t character_id   = j.value("character_id", (int64_t)0);
        const int     quest_id       = j.value("quest_id", 0);
        const int     requirement_id = j.value("requirement_id", 0);
        const auto moved = Database::CreditScriptQuestRequirement(character_id, quest_id, requirement_id);
        for (const auto& pr : moved) {
            session->send_quest_requirement_progress_response(
                pr.quest_id, pr.quest_requirement_id, pr.count);
        }
        Logger::Log("quest",
            "[TcpSession] quest_script_credit char=%lld quest=%d req=%d -> %zu requirement(s) updated\n",
            (long long)character_id, quest_id, requirement_id, moved.size());
    }
    else if (subtype == "quest_volume_use") {
        // The player pressed Use on an omega volume. The DLL reports every
        // press without knowing whether a quest wants it — quest state lives
        // here, same division of labour as quest_kill_credit.
        const int64_t character_id = j.value("character_id", (int64_t)0);
        const int     ui_volume_id = j.value("ui_volume_id", 0);

        if (character_id == 0 || ui_volume_id <= 0) {
            Logger::Log("quest",
                "[TcpSession] quest_volume_use: bad payload char=%lld volume=%d — ignored\n",
                (long long)character_id, ui_volume_id);
        } else {
            TurninCheck::NoteVolumeUse(character_id, ui_volume_id);
            const auto moved = Database::CreditVolumeQuestRequirements(character_id, ui_volume_id);
            for (const auto& pr : moved) {
                session->send_quest_requirement_progress_response(
                    pr.quest_id, pr.quest_requirement_id, pr.count);
            }
            if (!moved.empty()) {
                Logger::Log("quest",
                    "[TcpSession] quest_volume_use char=%lld volume=%d -> %zu requirement(s) updated\n",
                    (long long)character_id, ui_volume_id, moved.size());
            }
            if (!moved.empty())
                session->run_quest_followups(character_id, session->last_pawn_id_);
        }
    }
    else {
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: unknown subtype=%s\n", subtype.c_str());
    }
}

// ---------------------------------------------------------------------------
// DeliverPlayerAction -- forward a PLAYER_ACTION IPC to the game-DLL instance
// the player is currently assigned to. Mirrors DeliverGameEvent's locking
// pattern. Silent on failure (logged on the "chat-command" channel).
// ---------------------------------------------------------------------------

bool TcpSession::DeliverPlayerAction(const std::string& session_guid, const nlohmann::json& payload) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: no TcpSession for guid=%s\n",
                session_guid.c_str());
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: expired TcpSession for guid=%s\n",
                session_guid.c_str());
            return false;
        }
    }
    if (session->assigned_instance_id_ == 0) {
        Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: guid=%s has no assigned instance\n",
            session_guid.c_str());
        return false;
    }
    bool ok = IpcServer::SendToInstance(session->assigned_instance_id_, payload.dump());
    if (!ok) {
        Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: guid=%s instance=%lld send=0\n",
            session_guid.c_str(), (long long)session->assigned_instance_id_);
    }
    return ok;
}

bool TcpSession::AdminMovePlayerToInstance(const std::string& session_guid,
                                           int64_t target_instance_id,
                                           int target_task_force,
                                           int64_t source_instance_id,
                                           int source_task_force,
                                           std::string& message) {
    if (target_instance_id == 0) {
        message = "missing target_instance_id";
        return false;
    }
    if (target_task_force != 1 && target_task_force != 2) {
        message = "target_task_force must be 1 or 2";
        return false;
    }

    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            message = "player session not found";
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            message = "player session expired";
            return false;
        }
    }

    auto target = InstanceRegistry::GetInstanceById(target_instance_id);
    if (!target) {
        message = "target instance not found";
        return false;
    }
    if (target->state != "READY") {
        message = "target instance is not READY";
        return false;
    }

    int64_t current_instance_id = session->assigned_instance_id_;
    if (current_instance_id == 0 && source_instance_id != 0) {
        current_instance_id = source_instance_id;
    }

    if (current_instance_id == target_instance_id) {
        if (source_task_force == target_task_force) {
            message = "already on requested side";
            return true;
        }

        nlohmann::json action;
        action["type"] = IpcProtocol::MSG_PLAYER_ACTION;
        action["session_guid"] = session_guid;
        action["action"] = "change_team";
        action["args"] = {
            {"target", target_task_force == 1 ? "attackers" : "defenders"}
        };

        bool ok = IpcServer::SendToInstance(current_instance_id, action.dump());
        if (!ok) {
            message = "failed to send change_team action";
            return false;
        }

        InstanceRegistry::UpdateInstancePlayerTaskForce(current_instance_id, session_guid,
            target_task_force);
        message = "team change requested";
        return true;
    }

    if (current_instance_id != 0) {
        InstanceRegistry::MarkInstancePlayerLeft(current_instance_id, session_guid);
    }

    Logger::Log("tcp", "[TcpSession] Admin move player %s from instance=%lld to instance=%lld tf=%d\n",
        session_guid.c_str(), (long long)current_instance_id, (long long)target_instance_id,
        target_task_force);
    session->initiate_player_register_for_target(*target, target_task_force);
    message = "instance move requested";
    return true;
}

bool TcpSession::AdminMovePlayerWhenReady(const std::string& session_guid,
                                          int64_t target_instance_id,
                                          int target_task_force,
                                          int64_t source_instance_id,
                                          int source_task_force,
                                          std::string& message,
                                          uint32_t title_msg_id) {
    auto target = InstanceRegistry::GetInstanceById(target_instance_id);
    if (target && target->state == "READY") {
        return AdminMovePlayerToInstance(session_guid, target_instance_id, target_task_force,
            source_instance_id, source_task_force, message);
    }
    if (!target || target->state == "STOPPED" || target->state == "DRAINING") {
        message = "target instance not found or stopping";
        return false;
    }
    if (target_task_force != 1 && target_task_force != 2) {
        message = "target_task_force must be 1 or 2";
        return false;
    }
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it != g_sessions_.end()) session = it->second.lock();
    }
    if (!session) {
        message = "player session not found";
        return false;
    }
    {
        // A newer early move replaces any earlier one for this player.
        std::lock_guard<std::mutex> lock(early_moves_mutex_);
        for (auto& [id, list] : early_moves_) {
            list.erase(std::remove_if(list.begin(), list.end(),
                [&](const EarlyMove& m) { return m.session_guid == session_guid; }), list.end());
        }
        early_moves_[target_instance_id].push_back({session_guid, target_task_force});
    }

    int64_t current_instance_id = session->assigned_instance_id_;
    if (current_instance_id == 0) current_instance_id = source_instance_id;
    if (current_instance_id != 0 && current_instance_id != target_instance_id) {
        InstanceRegistry::MarkInstancePlayerLeft(current_instance_id, session_guid);
    }

    Logger::Log("tcp", "[TcpSession] Admin early move %s from instance=%lld to instance=%lld (state=%s) tf=%d\n",
        session_guid.c_str(), (long long)current_instance_id, (long long)target_instance_id,
        target->state.c_str(), target_task_force);
    session->travel_sent_instance_id_ = target_instance_id;
    session->send_go_play_to_instance(*target, target_task_force, title_msg_id);

    if (session->early_move_timer_) session->early_move_timer_->cancel();
    auto timer = std::make_shared<asio::steady_timer>(session->io_ctx_);
    session->early_move_timer_ = timer;
    timer->expires_after(std::chrono::seconds(90));
    std::weak_ptr<TcpSession> weak = session;
    timer->async_wait([weak, target_instance_id](const asio::error_code& ec) {
        if (ec) return;
        auto s = weak.lock();
        if (!s || s->travel_sent_instance_id_ != target_instance_id) return;
        {
            std::lock_guard<std::mutex> lock(early_moves_mutex_);
            auto it = early_moves_.find(target_instance_id);
            if (it != early_moves_.end()) {
                auto& list = it->second;
                list.erase(std::remove_if(list.begin(), list.end(),
                    [&](const EarlyMove& m) { return m.session_guid == s->session_guid_; }), list.end());
                if (list.empty()) early_moves_.erase(it);
            }
        }
        Logger::Log("tcp", "[TcpSession] Early move of %s: instance=%lld not READY after 90 s — routing home\n",
            s->session_guid_.c_str(), (long long)target_instance_id);
        s->travel_sent_instance_id_ = 0;
        s->assigned_instance_id_ = 0;
        s->wait_for_home_map_then_register(120);
    });

    message = "early move sent";
    return true;
}

void TcpSession::FlushEarlyMoves(int64_t instance_id) {
    std::vector<EarlyMove> moves;
    {
        std::lock_guard<std::mutex> lock(early_moves_mutex_);
        auto it = early_moves_.find(instance_id);
        if (it == early_moves_.end()) return;
        moves = std::move(it->second);
        early_moves_.erase(it);
    }
    for (const auto& m : moves) {
        std::shared_ptr<TcpSession> session;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            auto it = g_sessions_.find(m.session_guid);
            if (it != g_sessions_.end()) session = it->second.lock();
        }
        if (!session || session->travel_sent_instance_id_ != instance_id) continue;
        if (session->early_move_timer_) session->early_move_timer_->cancel();
        std::string message;
        if (!AdminMovePlayerToInstance(m.session_guid, instance_id, m.task_force, 0, 0, message)) {
            Logger::Log("tcp", "[TcpSession] Early move of %s to instance=%lld failed at READY (%s) — routing home\n",
                m.session_guid.c_str(), (long long)instance_id, message.c_str());
            session->travel_sent_instance_id_ = 0;
            session->assigned_instance_id_ = 0;
            session->wait_for_home_map_then_register(120);
        }
    }
}

bool TcpSession::ForceMissionExit(const std::string& session_guid,
                                  int64_t parent_instance_id,
                                  const char* reason) {
    if (session_guid.empty() || parent_instance_id == 0) {
        return false;
    }

    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid);
            Logger::Log("tcp",
                "[TcpSession] ForceMissionExit: no TcpSession for guid=%s parent=%lld reason=%s\n",
                session_guid.c_str(), (long long)parent_instance_id,
                reason ? reason : "unspecified");
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid);
            Logger::Log("tcp",
                "[TcpSession] ForceMissionExit: expired TcpSession for guid=%s parent=%lld reason=%s\n",
                session_guid.c_str(), (long long)parent_instance_id,
                reason ? reason : "unspecified");
            return false;
        }
    }

    if (session->assigned_instance_id_ != parent_instance_id) {
        InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid);
        Logger::Log("tcp",
            "[TcpSession] ForceMissionExit: guid=%s already assigned instance=%lld parent=%lld reason=%s\n",
            session_guid.c_str(), (long long)session->assigned_instance_id_,
            (long long)parent_instance_id, reason ? reason : "unspecified");
        return true;
    }

    session->route_from_mission_instance(parent_instance_id, reason);
    return true;
}

// Delay between the hold-black fade and GO_PLAY on a no-end-screen mission exit.
static constexpr int kExitFadeHoldMs = 300;

void TcpSession::route_from_mission_instance(int64_t parent_instance_id,
                                             const char* reason) {
    std::optional<InstanceInfo> parent;
    if (parent_instance_id != 0) {
        parent = InstanceRegistry::GetInstanceById(parent_instance_id);
    }
    if (parent && parent->is_home_map) {
        Logger::Log("tcp",
            "[TcpSession] Mission exit ignored for home instance=%lld guid=%s reason=%s\n",
            (long long)parent_instance_id, session_guid_.c_str(),
            reason ? reason : "unspecified");
        return;
    }

    std::optional<InstanceInfo> successor;
    bool tryContinue = false;
    if (parent && parent->end_mission_at != 0 && parent->queue_id != 0) {
        if (MatchmakingService::GetContinueInQueue(parent->queue_id)) {
            successor = InstanceRegistry::GetSuccessor(parent_instance_id);
            if (successor && successor->state == "READY") {
                const bool hasSeat = (successor->max_players == 0)
                    || (successor->player_count < successor->max_players);
                if (hasSeat) {
                    tryContinue = true;
                } else {
                    Logger::Log("tcp",
                        "[TcpSession] Successor %lld for parent=%lld is full (%d/%d) -- routing home instead\n",
                        (long long)successor->instance_id,
                        (long long)parent_instance_id,
                        successor->player_count, successor->max_players);
                }
            } else {
                Logger::Log("tcp",
                    "[TcpSession] No READY successor for parent=%lld (got %s) -- routing home instead\n",
                    (long long)parent_instance_id,
                    successor ? successor->state.c_str() : "<none>");
            }
        }
    }

    if (parent_instance_id != 0 && !session_guid_.empty()) {
        InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid_);
    }

    if (pending_ack_timer_) {
        pending_ack_timer_->cancel();
        pending_ack_timer_.reset();
        IpcServer::ClearPendingAck(session_guid_);
    }
    assigned_instance_id_      = 0;
    pending_match_instance_id_ = 0;
    pending_match_game_mode_.clear();

    if (tryContinue) {
        int tf = 1;
        auto cached = MatchmakingService::ConsumePreAssignedTeam(
            successor->instance_id, session_guid_);
        if (cached) {
            tf = *cached;
            Logger::Log("tcp",
                "[TcpSession] Continue-in-queue: pre-assigned tf=%d for %s on successor=%lld\n",
                tf, session_guid_.c_str(), (long long)successor->instance_id);
        } else {
            auto queue_cfg = MatchmakingService::GetQueueConfig(parent->queue_id);
            if (queue_cfg) {
                switch (queue_cfg->taskforce_policy) {
                    case TaskforcePolicy::Pinned1: tf = 1; break;
                    case TaskforcePolicy::Pinned2: tf = 2; break;
                    case TaskforcePolicy::Balanced: {
                        auto counts = InstanceRegistry::GetTeamCounts(successor->instance_id);
                        tf = (counts.first <= counts.second) ? 1 : 2;
                        break;
                    }
                    case TaskforcePolicy::BalancedPvp: {
                        auto roster =
                            InstanceRegistry::GetActivePlayersForInstance(successor->instance_id);
                        // Seed class counts (per-class term) and MMR sums
                        // (cost tie-break) alongside heal/size.
                        RoleWeightedSplit::TeamState t1, t2;
                        for (const auto& r : roster) {
                            const float v = RoleWeightedSplit::HealValue(r.profile_id, r.task_force);
                            const double mmr =
                                MmrService::GetCurrentRating(r.user_id, r.profile_id);
                            if (r.task_force == 1) {
                                t1.heal_score += v; t1.size += 1;
                                t1.class_counts[r.profile_id] += 1; t1.mmr_sum += mmr;
                                t1.class_mmr_sum[r.profile_id] += mmr;
                            } else {
                                t2.heal_score += v; t2.size += 1;
                                t2.class_counts[r.profile_id] += 1; t2.mmr_sum += mmr;
                                t2.class_mmr_sum[r.profile_id] += mmr;
                            }
                        }
                        tf = RoleWeightedSplit::PlaceSingle(selected_profile_id_, t1, t2);
                        break;
                    }
                }
            }
            Logger::Log("tcp",
                "[TcpSession] Continue-in-queue: cache miss for %s on successor=%lld -- policy fallback tf=%d\n",
                session_guid_.c_str(), (long long)successor->instance_id, tf);
        }

        Logger::Log("tcp",
            "[TcpSession] Mission exit: routing %s to successor %lld of parent=%lld queue=%u tf=%d reason=%s\n",
            session_guid_.c_str(), (long long)successor->instance_id,
            (long long)parent_instance_id, parent->queue_id, tf,
            reason ? reason : "unspecified");
        initiate_player_register_for_target(*successor, tf);
        return;
    }

    // Tutorial chapters
    // Only a completed chapter (its Kismet RequestMission reported by the
    // instance) chains on; Esc > Quit Tutorial goes home (level 5 + Dome arrival).
    // Only the RequestMission route ("request-mission") completes a
    // chapter. A client change-instance is always Quit Tutorial / End Mission ->
    // home, even when the chapter's RequestMission was already reported (a quit
    // at the black hand-off was otherwise sent on to chapter 2).
    const bool rm_seen = parent && request_mission_instance_ == parent_instance_id;
    const bool chapter_done = rm_seen && reason && std::string(reason) == "request-mission";
    request_mission_instance_ = 0;
    if (parent && !chapter_done && reason && std::string(reason) == "client change-instance")
        Logger::Log("tcp", "[TcpSession] Tutorial: %s left %s by client change-instance (quit, request_mission seen=%d) -> home\n",
            session_guid_.c_str(), parent->map_name.c_str(), (int)rm_seen);
    if (chapter_done && start_next_tutorial_chapter(parent->map_name, reason, parent_instance_id)) return;

    Logger::Log("tcp",
        "[TcpSession] Mission exit: routing %s to home from parent=%lld reason=%s\n",
        session_guid_.c_str(), (long long)parent_instance_id,
        reason ? reason : "unspecified");
    wait_for_home_map_then_register(120);
}

void TcpSession::initiate_player_register_and_go_play() {
    // Build PLAYER_REGISTER JSON with full character state.
    auto session = PlayerSessionStore::GetByGuid(session_guid_);
    if (!session) {
        Logger::Log("tcp", "[TcpSession] Cannot send PLAYER_REGISTER: session %s not found\n",
            session_guid_.c_str());
        return;
    }

    auto home_instance = InstanceRegistry::GetReadyHomeInstance();
    if (!home_instance) {
        Logger::Log("tcp", "[TcpSession] No READY home map instance -- cannot send PLAYER_REGISTER for %s\n",
            session_guid_.c_str());
        return;
    }

    const int task_force =
        ChooseVrHomeTaskForce(*home_instance, selected_profile_id_, session_guid_);
    home_task_force_ = task_force;
    const uint64_t register_token = next_register_token_++;
    active_register_token_ = register_token;

    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    reg["register_token"] = register_token;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = task_force;

    // Fetch character for morph_data, head_asm_id, gender_type_value_id
    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    // Embed the character's skill-tree allocation so the game server can apply
    // skill-based effect groups during pawn spawn without a second IPC round-
    // trip. Saves (inbound 0xA0) persist to the same DB, so the next spawn or
    // PLAYER_REGISTER sees fresh rows.
    {
        const int item_profile_id =
            PlayerSessionStore::GetCurrentItemProfile(selected_character_id_);
        this->item_profile_id_ = item_profile_id;
        reg["current_item_profile_id"] = item_profile_id;
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(
                 selected_character_id_, item_profile_id)) {
            nlohmann::json row;
            row["skill_group_id"] = s.skill_group_id;
            row["skill_id"]       = s.skill_id;
            row["points"]         = s.points;
            skillsArr.push_back(row);
        }
        reg["skills"] = skillsArr;
        reg["last_respec_at"] = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
    }

    // Arm 60-second ACK-wait timer.
    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));

    // Register callback for when ACK arrives.
    auto timer = pending_ack_timer_;  // capture by value
    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer, home_instance_id = home_instance->instance_id, task_force](
            bool success, int pawn_id) {
            timer->cancel();
            if (success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK success for %s pawn_id=%d\n",
                    session_guid_.c_str(), pawn_id);
                // Track player in home map instance
                auto home = InstanceRegistry::GetReadyHomeInstance();
                if (home && home->instance_id == home_instance_id) {
                    InstanceRegistry::InsertInstancePlayer(
                        home->instance_id, session_guid_, selected_character_id_, task_force,
                        selected_profile_id_);
                }
                send_go_play_response();
            } else {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK failed for %s\n",
                    session_guid_.c_str());
                // Silent: do not send go_play, do not send error to client
            }
        });

    // Send PLAYER_REGISTER to game instance.
    if (!IpcServer::SendToInstance(home_instance->instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session -- cannot send PLAYER_REGISTER for %s\n",
            session_guid_.c_str());
        pending_ack_timer_->cancel();
        IpcServer::ClearPendingAck(session_guid_);
        pending_ack_timer_.reset();
        return;
    }

    // Timeout handler.
    pending_ack_timer_->async_wait([this, self, timer](std::error_code ec) {
        if (!ec) {
            // Timer expired (60-second timeout). ACK never arrived.
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK timeout for %s\n",
                session_guid_.c_str());
            IpcServer::ClearPendingAck(session_guid_);
            // Silent: do not send go_play, do not send error to client
        }
        // ec == operation_aborted means timer was cancelled (ACK arrived or disconnect) -- do nothing
    });
}


void TcpSession::initiate_player_register_for_target(const InstanceInfo& target, int task_force) {
    auto session = PlayerSessionStore::GetByGuid(session_guid_);
    if (!session) {
        Logger::Log("tcp", "[TcpSession] Cannot send PLAYER_REGISTER (target): session %s not found\n",
            session_guid_.c_str());
        return;
    }

    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    const uint64_t register_token = next_register_token_++;
    active_register_token_ = register_token;
    reg["register_token"] = register_token;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = task_force;

    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    {
        const int item_profile_id =
            PlayerSessionStore::GetCurrentItemProfile(selected_character_id_);
        this->item_profile_id_ = item_profile_id;
        reg["current_item_profile_id"] = item_profile_id;
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(
                 selected_character_id_, item_profile_id)) {
            nlohmann::json row;
            row["skill_group_id"] = s.skill_group_id;
            row["skill_id"]       = s.skill_id;
            row["points"]         = s.points;
            skillsArr.push_back(row);
        }
        reg["skills"] = skillsArr;
        reg["last_respec_at"] = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
    }

    int64_t target_instance_id = target.instance_id;
    std::string target_map = target.map_name;

    // ACK timer — same 60s budget as the home path.
    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));
    auto timer = pending_ack_timer_;

    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer, target_instance_id, target_map, task_force](bool success, int pawn_id) {
            timer->cancel();
            if (success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (target) success for %s instance=%lld pawn_id=%d\n",
                    session_guid_.c_str(), (long long)target_instance_id, pawn_id);
                // Bind player to the successor instance and dispatch GSC_GO_PLAY.
                assigned_instance_id_ = target_instance_id;
                InstanceRegistry::InsertInstancePlayer(
                    target_instance_id, session_guid_, selected_character_id_, task_force,
                    selected_profile_id_);
                auto fresh = InstanceRegistry::GetInstanceById(target_instance_id);
                if (fresh && fresh->state == "READY") {
                    if (travel_sent_instance_id_ == target_instance_id) {
                        // Client is already on its way there (sent ahead).
                        Logger::Log("tcp", "[TcpSession] GO_PLAY for instance=%lld already sent ahead to %s\n",
                            (long long)target_instance_id, session_guid_.c_str());
                    } else {
                        send_go_play_to_instance(*fresh, task_force);
                    }
                    travel_sent_instance_id_ = 0;
                } else {
                    travel_sent_instance_id_ = 0;
                    Logger::Log("tcp", "[TcpSession] Successor instance %lld no longer READY (state=%s) — falling back to home\n",
                        (long long)target_instance_id, fresh ? fresh->state.c_str() : "<missing>");
                    assigned_instance_id_ = 0;
                    wait_for_home_map_then_register(120);
                }
            } else {
                // ACK failure usually means "no seat" (successor full, or game
                // server rejected for some other reason). Fall back to home so
                // the player still ends up somewhere.
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (target) failed for %s instance=%lld — falling back to home\n",
                    session_guid_.c_str(), (long long)target_instance_id);
                travel_sent_instance_id_ = 0;
                assigned_instance_id_ = 0;
                wait_for_home_map_then_register(120);
            }
        });

    if (!IpcServer::SendToInstance(target_instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session for successor %lld — falling back to home for %s\n",
            (long long)target_instance_id, session_guid_.c_str());
        pending_ack_timer_->cancel();
        IpcServer::ClearPendingAck(session_guid_);
        pending_ack_timer_.reset();
        wait_for_home_map_then_register(120);
        return;
    }

    pending_ack_timer_->async_wait([this, self, timer, target_instance_id](std::error_code ec) {
        if (!ec) {
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (target) timeout for %s instance=%lld — falling back to home\n",
                session_guid_.c_str(), (long long)target_instance_id);
            IpcServer::ClearPendingAck(session_guid_);
            wait_for_home_map_then_register(120);
        }
    });
}

bool TcpSession::DeliverSpectateJoin(const std::string& session_guid,
                                     int64_t target_instance_id,
                                     int team_task_force,
                                     std::string& message) {
    auto target = InstanceRegistry::GetInstanceById(target_instance_id);
    if (!target || target->state != "READY" || target->is_home_map) {
        message = "That instance is not available to spectate.";
        return false;
    }

    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            message = "Your session was not found.";
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            message = "Your session was not found.";
            return false;
        }
    }

    session->initiate_player_register_for_spectate(*target, team_task_force);
    message = "Connecting you to instance " + std::to_string(target_instance_id) + " as a spectator"
             + (team_task_force == 1 ? " (attackers)" : team_task_force == 2 ? " (defenders)" : "")
             + "...";
    return true;
}

void TcpSession::initiate_player_register_for_spectate(const InstanceInfo& target,
                                                       int team_task_force) {
    auto session = PlayerSessionStore::GetByGuid(session_guid_);
    if (!session) {
        Logger::Log("tcp", "[TcpSession] Cannot send PLAYER_REGISTER (spectate): session %s not found\n",
            session_guid_.c_str());
        return;
    }

    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    const uint64_t register_token = next_register_token_++;
    active_register_token_ = register_token;
    reg["register_token"] = register_token;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    // 0/1/2 — purely cosmetic for a spectator (see initiate_player_register_for_spectate
    // doc comment in the header). The DLL never lets this trigger a pawn spawn;
    // is_spectator is what gates that, independent of this value.
    reg["task_force"]   = team_task_force;
    reg["is_spectator"] = true;

    // Head/gender/morph only matter for a spawned pawn's appearance; a
    // spectator never spawns one, but the DLL parses these fields
    // unconditionally, so fill them the same as the normal register path.
    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    int64_t target_instance_id = target.instance_id;

    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));
    auto timer = pending_ack_timer_;

    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer, target_instance_id](bool success, int pawn_id) {
            timer->cancel();
            if (success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (spectate) success for %s instance=%lld\n",
                    session_guid_.c_str(), (long long)target_instance_id);
                assigned_instance_id_ = target_instance_id;
                is_spectating_ = true;
                auto fresh = InstanceRegistry::GetInstanceById(target_instance_id);
                if (fresh && fresh->state == "READY") {
                    send_go_play_to_instance(*fresh, 0);
                } else {
                    Logger::Log("tcp", "[TcpSession] Spectate target instance %lld no longer READY (state=%s)\n",
                        (long long)target_instance_id, fresh ? fresh->state.c_str() : "<missing>");
                    assigned_instance_id_ = 0;
                    is_spectating_ = false;
                }
            } else {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (spectate) failed for %s instance=%lld\n",
                    session_guid_.c_str(), (long long)target_instance_id);
            }
            (void)pawn_id;
        });

    if (!IpcServer::SendToInstance(target_instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session for spectate target %lld for %s\n",
            (long long)target_instance_id, session_guid_.c_str());
        pending_ack_timer_->cancel();
        IpcServer::ClearPendingAck(session_guid_);
        pending_ack_timer_.reset();
        return;
    }

    pending_ack_timer_->async_wait([this, self, timer, target_instance_id](std::error_code ec) {
        if (!ec) {
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (spectate) timeout for %s instance=%lld\n",
                session_guid_.c_str(), (long long)target_instance_id);
            IpcServer::ClearPendingAck(session_guid_);
        }
    });
}

bool TcpSession::DeliverSpectateExit(const std::string& session_guid, std::string& message) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            message = "Your session was not found.";
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            message = "Your session was not found.";
            return false;
        }
    }

    if (!session->is_spectating_) {
        message = "You're not currently spectating.";
        return false;
    }

    Logger::Log("tcp", "[TcpSession] -unspectate: guid=%s leaving instance=%lld\n",
        session_guid.c_str(), (long long)session->assigned_instance_id_);
    session->exit_spectate();
    message = "Leaving spectator mode, returning you to the home map...";
    return true;
}

void TcpSession::exit_spectate() {
    is_spectating_ = false;

    // Same PLAYER_CLOSE primitive as handle_socket_disconnect -- state flip +
    // engine reap of the NetConnection. NOT PLAYER_LEAVE: driving
    // NetConnection__Cleanup on a still-live connection crashes.
    if (assigned_instance_id_ != 0) {
        nlohmann::json close_msg;
        close_msg["type"]           = IpcProtocol::MSG_PLAYER_CLOSE;
        close_msg["session_guid"]   = session_guid_;
        close_msg["register_token"] = active_register_token_;
        const bool ok = IpcServer::SendToInstance(assigned_instance_id_, close_msg.dump());
        Logger::Log("tcp", "[TcpSession] -unspectate: PLAYER_CLOSE guid=%s instance=%lld send=%d\n",
            session_guid_.c_str(), (long long)assigned_instance_id_, (int)ok);
        assigned_instance_id_  = 0;
        active_register_token_ = 0;
    }

    // Straight home -- no route_from_mission_instance, no queue-continuation.
    wait_for_home_map_then_register(120);
}

void TcpSession::wait_for_home_map_then_register(int remaining_seconds) {
    if (remaining_seconds <= 0) {
        Logger::Log("tcp", "[TcpSession] Home map instance not ready (timeout): %s\n",
            session_guid_.c_str());
        return;  // Client stays on loading screen; no go_play sent
    }

    // Check for READY home instance first
    auto ready = InstanceRegistry::GetReadyHomeInstance();
    if (ready) {
        assigned_instance_id_ = ready->instance_id;
        Logger::Log("tcp", "[TcpSession] Home map ready: instance_id=%lld for %s\n",
            assigned_instance_id_, session_guid_.c_str());
        // The training mission ends at level 5 (skipping it lands there too):
        // lift the agent to the configured floor before the home spawn reads it.
        if (g_home_arrival_min_level > 1 && selected_character_id_ > 0 &&
            PlayerSessionStore::RaiseCharacterToLevel(selected_character_id_, g_home_arrival_min_level))
            Logger::Log("tcp", "[TcpSession] Home arrival: char=%lld raised to level %u\n",
                (long long)selected_character_id_, g_home_arrival_min_level);
        initiate_player_register_and_go_play();
        return;
    }

    // Check if a home instance is STARTING (someone else triggered it)
    auto home = InstanceRegistry::GetHomeInstance();
    if (!home && on_need_home_map_) {
        // No home instance at all — spawn one
        Logger::Log("tcp", "[TcpSession] No home map instance — spawning on demand for %s\n",
            session_guid_.c_str());
        on_need_home_map_();
    }

    // Poll again in 2 seconds
    auto self(shared_from_this());
    auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
    timer->expires_after(std::chrono::seconds(2));
    timer->async_wait([this, self, timer, remaining_seconds](std::error_code ec) {
        if (!ec) {
            wait_for_home_map_then_register(remaining_seconds - 2);
        }
    });
}


bool TcpSession::start_intro_mission(uint32_t training_map_game_id) {
    if (!on_need_intro_mission_) return false;

    // Prefer the map the client asked for when map_game_info knows it; the
    // spawner falls back to the configured default otherwise.
    std::string map_name, game_mode;
    if (training_map_game_id != 0) {
        auto row = MapGameInfo::LookupByMapGameId(training_map_game_id);
        if (row && !row->map_name.empty() && !row->game_class.empty()) {
            map_name  = row->map_name;
            game_mode = row->game_class;
        }
    }
    Logger::Log("tcp", "[TcpSession] Intro mission for %s: client TRAINING_MAP_GAME_ID=%u -> %s\n",
        session_guid_.c_str(), training_map_game_id,
        map_name.empty() ? "<config default>" : map_name.c_str());

    IntroHost host;  // the player's own node is asked first
    host.user_id = user_id_;
    host.character_id = selected_character_id_;
    host.session_guid = session_guid_;
    const int64_t instance_id = on_need_intro_mission_(map_name, game_mode, host);
    if (instance_id == 0) {
        Logger::Log("tcp", "[TcpSession] Intro mission spawn failed for %s -- home map instead\n",
            session_guid_.c_str());
        return false;
    }
    travel_in_flight_ = true;
    travel_to_instance_when_ready(instance_id, /*task_force=*/1, host.wait_s);
    return true;
}

void TcpSession::travel_to_instance_when_ready(int64_t instance_id, int task_force,
                                               int remaining_seconds) {
    if (instance_id == chapter_pending_instance_ && tutorial_quit_) {
        // Quit Tutorial arrived while the next chapter was spawning.
        Logger::Log("tcp", "[TcpSession] Tutorial: quit beats chapter instance=%lld -- %s -> home\n",
            (long long)instance_id, session_guid_.c_str());
        chapter_pending_instance_ = 0;
        tutorial_quit_ = false;
        travel_in_flight_ = false;
        wait_for_home_map_then_register(120);  // unused chapter instance: idle cleanup
        return;
    }
    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    if (remaining_seconds <= 0 || !inst || inst->state == "STOPPED") {
        Logger::Log("tcp", "[TcpSession] Intro instance=%lld not usable (%s) for %s -- home map instead\n",
            (long long)instance_id,
            !inst ? "gone" : (remaining_seconds <= 0 ? "timeout" : inst->state.c_str()),
            session_guid_.c_str());
        travel_in_flight_ = false;
        wait_for_home_map_then_register(120);
        return;
    }
    if (inst->state == "READY") {
        Logger::Log("tcp", "[TcpSession] Intro instance=%lld ('%s') ready -- sending %s tf=%d\n",
            (long long)instance_id, inst->map_name.c_str(), session_guid_.c_str(), task_force);
        // Sent ahead to the node, but the chapter ended up elsewhere (local
        // fallback) -> forget the early GO_PLAY so the register ACK sends a new one.
        if (travel_sent_instance_id_ == instance_id
            && (travel_sent_ip_ != inst->ip_address || travel_sent_port_ != inst->udp_port)) {
            Logger::Log("tcp", "[TcpSession] Intro instance=%lld moved %s:%d -> %s:%d -- GO_PLAY again for %s\n",
                (long long)instance_id, travel_sent_ip_.c_str(), (int)travel_sent_port_,
                inst->ip_address.c_str(), (int)inst->udp_port, session_guid_.c_str());
            travel_sent_instance_id_ = 0;
        }
        if (instance_id == chapter_pending_instance_) chapter_pending_instance_ = 0;
        travel_in_flight_ = false;
        initiate_player_register_for_target(*inst, task_force);
        return;
    }
    auto self(shared_from_this());
    auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
    timer->expires_after(std::chrono::seconds(2));
    timer->async_wait([this, self, timer, instance_id, task_force, remaining_seconds](std::error_code ec) {
        if (!ec) {
            travel_to_instance_when_ready(instance_id, task_force, remaining_seconds - 2);
        } else {
            travel_in_flight_ = false;
        }
    });
}

void TcpSession::DeliverOpenWorldTravel(const std::string& session_guid,
                                        const std::string& map_name,
                                        int task_force) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            Logger::Log("travel", "[TcpSession] DeliverOpenWorldTravel: no session for guid=%s\n",
                session_guid.c_str());
            return;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            Logger::Log("travel", "[TcpSession] DeliverOpenWorldTravel: expired session guid=%s\n",
                session_guid.c_str());
            return;
        }
    }

    if (session->travel_in_flight_) {
        // Using the exit again while waiting for a full zone = leave the line.
        if (InstanceAdmission::Leave(session_guid)) {
            Logger::Log("travel", "[TcpSession] %s left the line for '%s'\n",
                session_guid.c_str(), map_name.c_str());
            session->send_alert_text("You left the line.", 4.0f);
            return;
        }
        Logger::Log("travel",
            "[TcpSession] DeliverOpenWorldTravel: guid=%s already travelling — ignoring repeat\n",
            session_guid.c_str());
        return;
    }
    session->travel_in_flight_ = true;
    session->admission_since_ = 0;
    session->admission_from_instance_ = session->assigned_instance_id_;
    Logger::Log("travel", "[TcpSession] Travel requested: guid=%s → '%s' tf=%d\n",
        session_guid.c_str(), map_name.c_str(), task_force);
    session->travel_to_map_when_ready(map_name, task_force, 120);
}

void TcpSession::DeliverZoneQueueJoin(const std::string& session_guid, uint32_t queue_id) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it != g_sessions_.end()) session = it->second.lock();
    }
    if (!session) {
        Logger::Log("travel", "[TcpSession] DeliverZoneQueueJoin: no session for guid=%s\n",
            session_guid.c_str());
        return;
    }
    if (session->current_match_queue_id_ == queue_id) return;  // Use-spam
    if (session->current_match_queue_id_ != 0) {
        ChatSession::SystemMessageToGuid(session_guid, "*** Leave your current queue first. ***");
        return;
    }
    session->handle_match_join(queue_id, 0);
}

void TcpSession::travel_to_map_when_ready(const std::string& map_name, int task_force,
                                          int remaining_seconds) {
    if (remaining_seconds <= 0) {
        Logger::Log("travel",
            "[TcpSession] Travel to '%s' timed out for %s — staying put\n",
            map_name.c_str(), session_guid_.c_str());
        travel_in_flight_ = false;
        InstanceAdmission::Leave(session_guid_);
        admission_since_ = 0;
        return;
    }

    auto ready = InstanceRegistry::GetReadyInstance(map_name);
    if (ready) {
        if (assigned_instance_id_ == ready->instance_id) {
            Logger::Log("travel", "[TcpSession] %s is already on '%s' (instance=%lld)\n",
                session_guid_.c_str(), map_name.c_str(), (long long)ready->instance_id);
            travel_in_flight_ = false;
            return;
        }
        if (!admission_wait(map_name, *ready, task_force)) return;
        if (assigned_instance_id_ != 0) {
            InstanceRegistry::MarkInstancePlayerLeft(assigned_instance_id_, session_guid_);
        }
        Logger::Log("travel", "[TcpSession] Travelling %s from instance=%lld to '%s' instance=%lld tf=%d\n",
            session_guid_.c_str(), (long long)assigned_instance_id_, map_name.c_str(),
            (long long)ready->instance_id, task_force);
        travel_in_flight_ = false;
        initiate_player_register_for_target(*ready, task_force);
        return;
    }

    // Still STARTING (or the spawn hasn't inserted its row yet) — poll.
    auto self(shared_from_this());
    auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
    timer->expires_after(std::chrono::seconds(2));
    timer->async_wait([this, self, timer, map_name, task_force, remaining_seconds](std::error_code ec) {
        if (!ec) {
            travel_to_map_when_ready(map_name, task_force, remaining_seconds - 2);
        } else {
            travel_in_flight_ = false;
        }
    });
}

// The target zone is at its player cap -> wait in its FIFO line
// (InstanceAdmission) where the player stands. True = go now; false = still
// waiting (re-polls itself every 3 s) or gave up.
bool TcpSession::admission_wait(const std::string& map_name, const InstanceInfo& target,
                                int task_force) {
    const time_t now = time(nullptr);
    if (admission_since_ != 0) {
        const char* why = nullptr;
        if (!socket_.is_open())                                    why = "disconnected";
        else if (!InstanceAdmission::IsWaiting(session_guid_))     why = "left the line";
        else if (assigned_instance_id_ != admission_from_instance_) why = "moved elsewhere";
        else if (now - admission_since_ > InstanceAdmission::kMaxWaitSeconds) why = "waited too long";
        if (why) {
            InstanceAdmission::Leave(session_guid_);
            Logger::Log("travel", "[TcpSession] %s stops waiting for '%s' (%s)\n",
                session_guid_.c_str(), map_name.c_str(), why);
            if (now - admission_since_ > InstanceAdmission::kMaxWaitSeconds && socket_.is_open())
                send_alert_text("The zone is still full - please try again later.", 6.0f);
            admission_since_ = 0;
            travel_in_flight_ = false;
            return false;
        }
    }
    int pos = 0, len = 0;
    if (InstanceAdmission::Poll(map_name, target, session_guid_, s_zone_cap_, &pos, &len)
            == InstanceAdmission::Result::Admit) {
        if (admission_since_ != 0)
            send_alert_text("Your turn - entering the zone.", 4.0f);
        admission_since_ = 0;
        return true;
    }
    if (admission_since_ == 0) {
        admission_since_ = now;
        admission_shown_at_ = 0;
        admission_shown_pos_ = 0;
        Logger::Log("travel", "[TcpSession] '%s' full (instance=%lld) - %s waits, #%d of %d\n",
            map_name.c_str(), (long long)target.instance_id, session_guid_.c_str(), pos, len);
    }
    if (pos != admission_shown_pos_ || now - admission_shown_at_ >= 20) {
        char line[160];
        snprintf(line, sizeof line,
            "This zone is full - you are #%d in line. Use the exit again to leave the line.", pos);
        send_alert_text(line, 8.0f);
        if (admission_shown_pos_ == 0) ChatSession::SystemMessageToGuid(session_guid_, line);
        admission_shown_pos_ = pos;
        admission_shown_at_ = now;
    }
    auto self(shared_from_this());
    auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
    timer->expires_after(std::chrono::seconds(3));
    timer->async_wait([this, self, timer, map_name, task_force](std::error_code ec) {
        if (!ec) {
            travel_to_map_when_ready(map_name, task_force, 120);
        } else {
            InstanceAdmission::Leave(session_guid_);
            travel_in_flight_ = false;
        }
    });
    return false;
}

void TcpSession::send_alert_text(const char* text, float duration) {
    nlohmann::json action;
    action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    action["session_guid"] = session_guid_;
    action["action"]       = "alert_text";
    action["args"] = {{"text", text}, {"priority", 2}, {"type", 3}, {"duration", duration}};
    TcpSession::DeliverPlayerAction(session_guid_, action);
}

// Agent Profile > Stats > Information panel (SEND_CHARACTER_CRAFTING_SKILLS
// 0x017F, decoded from the client: HUD vtable+0x408 -> 0x1142a780 -> 0x1142a120 -> 0x11428b90).
// XP_VALUE gives level + Next Level bar (client XP table), SKILL_RATING = Combat Rating
// stars (float, clamped 0..5, half steps), CURRENCY_VALUE / TOKEN_VALUE / HZPOINT_VALUE =
// Credits / Tokens / Agenda Pts. Wallet only for your own agent.
void TcpSession::send_agent_info(int64_t cid, bool own, bool log) {
	// Own agent: pay any due Agenda Points (level-ups, daily login) first.
	if (own) {
		int64_t daily = 0, streak = 0;
		PlayerSessionStore::SyncAgendaPoints(cid, &daily, &streak);
		if (const int64_t bt = EconParity::BoosterNightlyTokens(cid))  // booster nightly tokens
			ChatSession::SystemMessageToGuid(session_guid_, "Booster: +" + std::to_string(bt) + " tokens (2,000 for every midnight GMT with an active booster).");
		// (6b) daily login bonus popup: the game's own centre-screen alert toast
		// (IPC alert_text -> SendAlert::SendTextAlert) + a chat line fallback.
		// Paid at most once per account per UTC day, first push after spawn.
		if (daily > 0) {
			char line[160];
			snprintf(line, sizeof line, "Daily login bonus: +%lld Agenda Points (day %lld streak)",
				(long long)daily, (long long)streak);
			nlohmann::json action;
			action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
			action["session_guid"] = session_guid_;
			action["action"]       = "alert_text";
			action["args"] = {{"text", line}, {"priority", 2}, {"type", 1}, {"duration", 8.0f}};
			TcpSession::DeliverPlayerAction(session_guid_, action);
			ChatSession::SystemMessageToGuid(session_guid_, std::string(line) +
				" - log in every day for +5 more (max 100/day).");
		}
	}
	// (7) level reached by mission-end XP: level-up fx + message once, in the next map.
	if (own && assigned_instance_id_ != 0) {
		if (const uint32_t lv = PlayerSessionStore::TakeLevelUp(cid)) {
			nlohmann::json action;
			action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
			action["session_guid"] = session_guid_;
			action["action"]       = "level_up";
			action["args"] = {{"level", lv}};
			TcpSession::DeliverPlayerAction(session_guid_, action);
			Logger::Log("rewards", "[LevelUp] char=%lld level %u -> instance fx\n", (long long)cid, lv);
		}
	}
	const auto info = PlayerSessionStore::GetAgentInfo(cid);
	if (!info.found) {
		if (log) Logger::Log("tcp", "[TCP] SEND_CHARACTER_CRAFTING_SKILLS: char=%lld not found\n", (long long)cid);
		return;
	}
	// Was one star per 10 levels; now PvP MMR based (CombatRating.hpp).
	const float rating = CombatRating::Stars(cid, info.level);  // PvP MMR, lv30 >= 2
	std::vector<uint8_t> r;
	append(r, GA_U::SEND_CHARACTER_CRAFTING_SKILLS & 0xFF, GA_U::SEND_CHARACTER_CRAFTING_SKILLS >> 8);
	append(r, 6, 0x00);
	Write4B(r, GA_T::XP_VALUE, (uint32_t)info.xp);
	WriteFloat(r, GA_T::SKILL_RATING, rating);
	// CURRENCY_VALUE is a "double" type but goes as a 4-byte int (as in 0x012C).
	Write4B(r, GA_T::CURRENCY_VALUE, own ? (uint32_t)(int32_t)info.credits : 0);
	Write4B(r, GA_T::TOKEN_VALUE, own ? (uint32_t)info.tokens : 0);
	Write4B(r, GA_T::HZPOINT_VALUE, own ? (uint32_t)info.agenda_points : 0);
	// AP is paid/spent here (daily, level-up, mission end) but most client AP displays
	// read the pawn's replicated r_nHZPoints -> restate the wallet on the instance
	// whenever the value changed since the last restate (also once per session).
	if (own && info.agenda_points != last_ap_restated_ && assigned_instance_id_ != 0) {
		last_ap_restated_ = info.agenda_points;
		send_mail_notify(0, true);
	}
	// Crafting Skills tab: one {SKILL_ID, POINTS_ALLOCATED} row per crafting skill.
	const auto crafting = Database::GetCraftingSkills(cid);
	append(r, GA_T::DATA_SET_CHAR_CRAFTING_SKILLS & 0xFF, GA_T::DATA_SET_CHAR_CRAFTING_SKILLS >> 8);
	append(r, crafting.size() & 0xFF, (crafting.size() >> 8) & 0xFF);
	for (const auto& cs : crafting) {
		append(r, 0x02, 0x00);
		Write4B(r, GA_T::SKILL_ID, (uint32_t)cs.first);
		Write4B(r, GA_T::POINTS_ALLOCATED, (uint32_t)cs.second);
	}
	send_response(r);
	if (log) Logger::Log("tcp", "[TCP] SEND_CHARACTER_CRAFTING_SKILLS -> char=%lld own=%d lv=%u xp=%lld cr=%lld tok=%lld rating=%.1f\n",
		(long long)cid, own ? 1 : 0, info.level, (long long)info.xp,
		(long long)(own ? info.credits : 0), (long long)(own ? info.tokens : 0), rating);
}

void TcpSession::handle_packet(const uint8_t* data, size_t length) {
	if (length < 6) {
		Logger::Log("tcp", "[%s] Packet too short\n", Logger::GetTime());
		return;
	}

	const uint8_t* p = data + 2;
	uint16_t packet_type = p[0] | (p[1] << 8);
	uint16_t item_count  = p[2] | (p[3] << 8);

	switch (packet_type) {
		case GA_U::GSC_USER_LOGIN: {
			std::error_code endpoint_ec;
			auto remote_endpoint = socket_.remote_endpoint(endpoint_ec);
			std::string remote_ip;
			if (!endpoint_ec) {
				remote_ip = remote_endpoint.address().to_string();
				ip_address_ = remote_ip + ":" + std::to_string(remote_endpoint.port());
			} else {
				ip_address_ = "<unknown>";
			}

			if (IsLoginIpCoolingDown(remote_ip)) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN ignored during failed-login cooldown ip=%s\n",
					Logger::GetTime(), remote_ip.c_str());
				break;
			}

			// IP ban check — runs BEFORE username parsing so banned-by-IP
			// sources don't even get the transient pre-login response.
			if (auto ip_ban = Database::FindActiveBanForIp(remote_ip)) {
				Database::InsertSession(0, /*username yet unknown*/"", remote_ip, "banned");
				Logger::Log("ban", "[ban] ip-match ip=%s reason=\"%s\" mode=%s\n",
					remote_ip.c_str(), ip_ban->reason.c_str(), s_ban_spoof_mode_.c_str());
				ApplyBanSpoof();
				break;
			}

			PacketView pkt(data + 6, length - 6);
			auto read_login_name = [&pkt](uint16_t type) -> std::string {
				auto field = pkt.ReadString(type);
				return field ? TrimAsciiWhitespace(*field) : std::string();
			};
			std::string requested_user_name = read_login_name(GA_T::USER_NAME);
			const char* login_field_name = "USER_NAME";
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::PLAYER_NAME);
				login_field_name = "PLAYER_NAME";
			}
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::DATABASE_LOGIN);
				login_field_name = "DATABASE_LOGIN";
			}
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::HOST_LOGIN);
				login_field_name = "HOST_LOGIN";
			}
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::HOST_SQL_LOGIN);
				login_field_name = "HOST_SQL_LOGIN";
			}
			const bool has_user_name = !requested_user_name.empty();

			if (!has_user_name) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN pre-login frame with no account fields; transient response only ip=%s item_count=%u len=%zu\n",
					Logger::GetTime(), ip_address_.c_str(), item_count, length);
				// Remember the issued nonce: the client encrypts its credential
				// blob against it, and the next (credentialed) frame on this
				// connection verifies against it.
				pending_login_challenge_guid_ = GenerateSessionGuid();
				send_login_response_for("", pending_login_challenge_guid_);
				break;
			}
			if (EqualsIgnoreAsciiCase(requested_user_name, "unknown")) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN reserved username rejected ip=%s\n",
					Logger::GetTime(), ip_address_.c_str());
				Database::InsertSession(0, requested_user_name, remote_ip, "rejected");
				MarkFailedLoginIp(remote_ip);
				close_after_login_rejection();
				break;
			}
			if (!IsPrintableAsciiUsername(requested_user_name)) {
				std::string hex_dump;
				hex_dump.reserve(requested_user_name.size() * 2);
				static const char kHex[] = "0123456789ABCDEF";
				for (unsigned char c : requested_user_name) {
					hex_dump.push_back(kHex[c >> 4]);
					hex_dump.push_back(kHex[c & 0x0F]);
				}
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN invalid characters in account name rejected ip=%s hex=%s\n",
					Logger::GetTime(), ip_address_.c_str(), hex_dump.c_str());
				Database::InsertSession(0, requested_user_name, remote_ip, "rejected");
				MarkFailedLoginIp(remote_ip);
				send_login_rejected_response("Invalid characters in account name.");
				close_after_login_rejection();
				break;
			}

			if (!session_guid_.empty()) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN replacing prior session guid=%s name=%s\n",
					Logger::GetTime(), session_guid_.c_str(), player_name.c_str());
				if (session_row_id_) {
					Database::FinalizeSession(session_row_id_);
					session_row_id_ = 0;
				}
				MatchmakingService::RemovePlayer(session_guid_);
				// Same-socket relogin means the player went back to the login
				// screen voluntarily — leave the team, don't park offline.
				TeamService::HandleVoluntaryExit(session_guid_);
				UnregisterSession(session_guid_);
				IpcServer::ClearPendingAck(session_guid_);
				PlayerSessionStore::Unregister(session_guid_);
				login_queue_forget();
				session_guid_.clear();
				user_id_ = 0;
				selected_character_id_ = 0;
				selected_profile_id_ = 0;
				item_profile_id_ = 0;
				assigned_instance_id_ = 0;
				current_match_queue_id_ = 0;
				pending_match_instance_id_ = 0;
				pending_match_game_mode_.clear();
				pending_match_task_force_ = 1;
			}

			// NB: a login colliding with an existing session is no longer
			// rejected here — it evicts the stale session AFTER password
			// verification below (re-login takeover). Rejecting pre-verify
			// locked crashed players out until keepalive reaped the dead
			// socket — and forever when a hung client kept ACKing probes.

			// ---- Password verification (challenge-response) -------------------
			// The client encrypted a credential blob against the SESSION_GUID
			// nonce we issued in the pre-login frame. Recover the per-account
			// verifier (SHA256 of the RC4 keystream) and compare. A brand-new
			// account — or a legacy account with no verifier yet — registers it
			// (trust-on-first-use). See
			// .planning/2026-06-08-password-verification-design.md.
			const bool account_locked = IsAuthAccountLocked(requested_user_name);
			if (IsAuthIpLocked(remote_ip)) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN refused during failed-password lockout name=%s ip=%s\n",
					Logger::GetTime(), requested_user_name.c_str(), ip_address_.c_str());
				Database::InsertSession(0, requested_user_name, remote_ip, "locked");
				send_login_rejected_response("Incorrect password.", 18760);
				break;
			}
			bool store_new_verifier = false;
			std::vector<uint8_t> new_verifier;
			if (s_require_provisioned_accounts_) {
				// Accounts come from the website. Only the two verified
				// name fields are accepted, the name must already exist with a
				// verifier (matched case-insensitively), and we then use the stored
				// spelling. Unknown names get the same answer as a wrong password.
				std::string canonical;
				if (login_field_name == std::string("USER_NAME") ||
				    login_field_name == std::string("PLAYER_NAME")) {
					canonical = PlayerSessionStore::FindProvisionedUsername(requested_user_name);
				}
				if (canonical.empty()) {
					Logger::Log("tcp",
						"[%s] GSC_USER_LOGIN not a provisioned account name=%s field=%s ip=%s\n",
						Logger::GetTime(), requested_user_name.c_str(), login_field_name, ip_address_.c_str());
					Database::InsertSession(0, requested_user_name, remote_ip, "rejected");
					MarkAuthFailure(remote_ip, "");
					send_login_rejected_response("Incorrect password.", 18760);
					break;
				}
				requested_user_name = canonical;
			}
			if (s_require_password_verification_ &&
			    (login_field_name == std::string("USER_NAME") ||
			     login_field_name == std::string("PLAYER_NAME"))) {
				const std::string challenge = pending_login_challenge_guid_;
				pending_login_challenge_guid_.clear();  // one-shot: no replay

				// DYNAMIC_UINT32_SIZE fields come back with a 4B length prefix;
				// strip it to get the raw RC4 ciphertext (same as the DWORDS
				// morph-blob handler in ADD_PLAYER_CHARACTER).
				auto blobRaw = pkt.ReadNBytes(GA_T::BIN_BLOB);
				std::vector<uint8_t> blob;
				if (blobRaw && blobRaw->size() > 4)
					blob.assign(blobRaw->begin() + 4, blobRaw->end());

				std::vector<uint8_t> computed;
				if (!blob.empty() && !challenge.empty())
					computed = LoginAuth::ComputeVerifier(challenge, requested_user_name, blob);

				PlayerSessionStore::UserAuth auth =
					PlayerSessionStore::GetUserAuth(requested_user_name);

				if (auth.exists && !auth.verifier.empty()) {
					// Registered account — the blob MUST verify.
					bool alt_ok = false;
					if (!computed.empty() && computed != auth.verifier &&
					    ServiceHooks::Enabled("login_alt_verifier")) {
						// Optional external service may accept a one-time
						// verifier for this account (e.g. a short-lived login
						// ticket). No service / timeout / no "accept" = refuse.
						static const char* hx = "0123456789abcdef";
						std::string vhex;
						for (uint8_t b : computed) { vhex += hx[b >> 4]; vhex += hx[b & 15]; }
						auto d = ServiceHooks::Call("login_alt_verifier",
							{{"user_id", auth.user_id}, {"user", requested_user_name},
							 {"verifier", vhex}, {"ip", remote_ip}});
						alt_ok = d && d->is_object() && d->value("accept", false) == true;
						if (alt_ok)
							Logger::Log("tcp", "[%s] GSC_USER_LOGIN accepted by login_alt_verifier name=%s ip=%s\n",
								Logger::GetTime(), requested_user_name.c_str(), ip_address_.c_str());
					}
					if (account_locked && !alt_ok) {   // password logins wait out the account lock
						Logger::Log("tcp",
							"[%s] GSC_USER_LOGIN password login refused during account lockout name=%s ip=%s (launcher tickets still work)\n",
							Logger::GetTime(), requested_user_name.c_str(), ip_address_.c_str());
						Database::InsertSession(auth.user_id, requested_user_name, remote_ip, "locked");
						MarkAuthFailure(remote_ip, "");
						send_login_rejected_response("Incorrect password.", 18760);
						break;
					}
					if (!alt_ok && (computed.empty() || computed != auth.verifier)) {
						Logger::Log("tcp",
							"[%s] GSC_USER_LOGIN incorrect password name=%s ip=%s blob=%s challenge=%s\n",
							Logger::GetTime(), requested_user_name.c_str(), ip_address_.c_str(),
							blob.empty() ? "missing" : "present", challenge.empty() ? "missing" : "present");
						Database::InsertSession(auth.user_id, requested_user_name, remote_ip, "rejected");
						MarkAuthFailure(remote_ip, requested_user_name);
						// Unlike bans (which stay silent so the player just sees a
						// hang), a wrong password gets a real error the client can
						// display. 18760 = the client's "incorrect password" msg id.
						//
						// Do NOT close the socket here and do NOT trip the failed-
						// login IP cooldown: closing immediately after the write
						// races the response into a TCP RST (the client never parses
						// it → spinner hangs), and the cooldown would block the
						// retry. Leaving the connection open lets the client show
						// the error and re-attempt on the same socket, exactly like
						// the normal success response path.
						send_login_rejected_response("Incorrect password.", 18760);
						break;
					}
				} else if (!computed.empty()) {
					// New account, or legacy row with no verifier — capture it.
					store_new_verifier = true;
					new_verifier = std::move(computed);
				}
				// else: new/legacy account with no usable blob (non-standard
				// login path) — allow through; there is nothing to verify yet.
			}

			player_name = requested_user_name;
			const int64_t resolved_user_id = PlayerSessionStore::UpsertUser(player_name);

			if (store_new_verifier) {
				PlayerSessionStore::SetUserVerifier(resolved_user_id, new_verifier,
				                                    (int64_t)std::time(nullptr));
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN registered password verifier name=%s user_id=%lld\n",
					Logger::GetTime(), player_name.c_str(), (long long)resolved_user_id);
			}

			// Service agents are driven server-side and never log in.
			if (ServiceAgents::IsServiceUser(resolved_user_id)) {
				Database::InsertSession(resolved_user_id, player_name, remote_ip, "rejected");
				Logger::Log("tcp", "[%s] GSC_USER_LOGIN refused: service agent name=%s ip=%s\n",
					Logger::GetTime(), player_name.c_str(), ip_address_.c_str());
				send_login_rejected_response("Incorrect password.", 18760);
				break;
			}

			// Account ban check — runs after username validation/dedup so a
			// banned user typing a different name doesn't accidentally match.
			if (auto user_ban = Database::FindActiveBanForUser(resolved_user_id)) {
				Database::InsertSession(resolved_user_id, player_name, remote_ip, "banned");
				Logger::Log("ban", "[ban] user-match user_id=%lld name=%s ip=%s reason=\"%s\" mode=%s\n",
					(long long)resolved_user_id, player_name.c_str(), remote_ip.c_str(),
					user_ban->reason.c_str(), s_ban_spoof_mode_.c_str());
				ApplyBanSpoof();
				break;
			}

			// Re-login takeover: this login passed verification, so it owns
			// the account — evict any session still registered under the same
			// name (crashed or hung client whose socket wasn't reaped yet)
			// instead of rejecting. Loop in case multiple stale rows exist;
			// EvictStaleSession always removes the store row, so it converges.
			if (!s_allow_duplicate_account_logins_) {
				while (auto existing = PlayerSessionStore::GetByPlayerName(player_name)) {
					Logger::Log("tcp",
						"[%s] GSC_USER_LOGIN takeover name=%s stale_guid=%s stale_ip=%s new_ip=%s\n",
						Logger::GetTime(), player_name.c_str(),
						existing->session_guid.c_str(), existing->ip_address.c_str(),
						ip_address_.c_str());
					EvictStaleSession(existing->session_guid, "re-login takeover");
				}
			}

			session_guid_ = GenerateSessionGuid();
			RegisterSession(session_guid_, shared_from_this());
			user_id_ = resolved_user_id;
			session_row_id_   = Database::InsertSession(user_id_, player_name, remote_ip, "ok");
			session_login_at_ = (int64_t)std::time(nullptr);

			// Strip any inventory rows / equip references for items the
			// operator has marked as crash-on-equip in ga_item_blacklist.
			// Runs before send_login_response so the client never sees the
			// bad item in any subsequent SEND_INVENTORY / character data.
			PlayerSessionStore::PurgeBlacklistedItems(user_id_);

			{
				SessionInfo info;
				info.session_guid = session_guid_;
				info.player_name = player_name;
				info.ip_address = ip_address_;
				info.user_id = user_id_;
				PlayerSessionStore::Register(info);
			}

			Logger::Log("tcp", "[%s] Received: GSC_USER_LOGIN [0x%04X], name: %s field=%s guid: %s ip: %s\n",
			   Logger::GetTime(), packet_type, player_name.c_str(), login_field_name, session_guid_.c_str(), ip_address_.c_str());

			// Preemptively ensure a home map instance is starting so it's
			// likely READY by the time the player selects a character.
			EnsureHomeMapWarm("login pre-spawn");

			send_login_response();
			break;
		}
		case GA_U::GSC_CHARACTER_LIST:
			Logger::Log("tcp", "[%s] Received: GSC_CHARACTER_LIST [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			handle_character_list_request();
			break;
		case GA_U::PLAYER_UPDATE_CLIENT:
			Logger::Log("tcp", "[%s] Received: PLAYER_UPDATE_CLIENT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_player_update_client_response();
			break;
		case GA_U::GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED:
		{
			PacketView pkt(data + 6, length - 6);
			int nLootTableId = (int)pkt.Read4B(GA_T::LOOT_TABLE_ID).value_or(0);
			Logger::Log("tcp", "[%s] Received: GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED [0x%04X], item count: %d, loot table %d\n", Logger::GetTime(), packet_type, item_count, nLootTableId);
			LogData(data, length);
			send_get_loot_table_items_by_id_filtered_response(nLootTableId);
			break;
		}
		case GA_U::RELAY_LOG: {
			// Phase 10: IPC GAME_EVENT will populate event queues here.
			// For now, RELAY_LOG is a keepalive -- no action needed.
			// The client never sends its 0x017F request in practice
			// (verified by capture), and it keeps a 0x017F reply only while the
			// Stats panel is open (scene lookup in 0x113a1e90, else dropped) -
			// so push the small Information-panel reply on the keepalive, at most
			// every 2 s, once the agent is in the world.
			if (selected_character_id_ != 0 && last_pawn_id_ != 0) {
				const auto now = std::chrono::steady_clock::now();
				if (now - last_agent_info_push_ >= std::chrono::seconds(2)) {
					last_agent_info_push_ = now;
					if (const int64_t it = Inspect::ActiveTarget(session_guid_)) send_agent_info(it, false, false); else
					send_agent_info(selected_character_id_, true, false);
				}
			}
			break;
		}
		case GA_U::GSC_SELECT_CHARACTER: {
			PacketView pkt(data + 6, length - 6);
			int64_t nCharacterId = pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);

			Logger::Log("tcp", "[%s] Received: GSC_SELECT_CHARACTER [0x%04X] charId=%lld session_guid=%s\n",
				Logger::GetTime(), packet_type, nCharacterId, session_guid_.c_str());

			LogData(data, length);

			// Look up the requested character from DB and select it.
			if (nCharacterId != 0) {
				auto charInfo = PlayerSessionStore::GetCharacterById(nCharacterId);
				if (charInfo && charInfo->user_id == user_id_ && charInfo->deleted_at == 0) {
					selected_character_id_ = charInfo->id;
					selected_profile_id_   = charInfo->profile_id;
					PlayerSessionStore::SetSelectedCharacter(session_guid_, charInfo->id, charInfo->profile_id);
					// Top up this user's account-scoped inventory pool from
					// ClassLoadouts.cpp. Idempotent — existing inventory rows
					// keep their stable `inventory_id`. Gameplay devices stay
					// bag-only until the player equips them; stable IDs make the
					// equip screen safe between sessions.
					// Bags + components are per agent - swap this
					// agent's pool in before anything reads or seeds it.
					PlayerSessionStore::ActivateAgentPool(charInfo->user_id, charInfo->id);
					PlayerSessionStore::SeedInventoryFromLoadouts(charInfo->user_id);
					// HUMAN BASE ATTRIBUTES (slot 14) is server-pinned — the
					// player can't equip it via the UI, so the row must exist
					// in ga_character_devices before SEND_INVENTORY ships or
					// the client hits the resubmit loop on the REST device.
					// INSERT OR IGNORE per loadout profile — defensive against
					// existing characters whose slot 14 was never written.
					PlayerSessionStore::PinClassDeviceSlot14(selected_character_id_);
					PlayerSessionStore::SeedStarterWeapons(selected_character_id_);
					deliver_login_mail(user_id_, selected_character_id_, player_name);
					// Back at character select the client drops its quest
					// manager, even when the same agent is picked again in the
					// same TCP session -> replay quest state on the next spawn.
					quests_synced_for_character_ = 0;
					Logger::Log("tcp", "[%s] GSC_SELECT_CHARACTER: selected charId=%lld profile=%u\n",
						Logger::GetTime(), selected_character_id_, selected_profile_id_);
				} else {
					Logger::Log("tcp", "[%s] GSC_SELECT_CHARACTER: charId=%lld not found or wrong user (user_id_=%lld)\n",
						Logger::GetTime(), nCharacterId, user_id_);
				}
			}

			send_select_character_response();

			// Wait for a READY home map instance, then send PLAYER_REGISTER.
			wait_for_home_map_then_register(120);  // 120 second timeout

			break;
		}
		case GA_U::PLAYER_LOGOFF:
			Logger::Log("tcp", "[%s] Received: PLAYER_LOGOFF [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			// Quitting the game is a voluntary exit — leave the team now
			// instead of lingering as an offline member.
			if (!session_guid_.empty())
				TeamService::HandleVoluntaryExit(session_guid_);
			break;
		case GA_U::PLAYER_LEAVE_GAME: {
			// Client sends this when the user clicks "Disconnect" in the in-game
			// menu — TCP stays alive but the player goes back to character select.
			// We must tell the game-DLL instance to tear down the UDP UNetConnection,
			// PlayerController and Pawn for this session_guid right now. Without
			// this the stale connection lingers in NetDriver->ClientConnections
			// until the engine's ConnectionTimeout (~30s), so picking a character
			// again routes the new HELLO into the stale USOCK_Open connection
			// and the client times out back to the login screen.
			Logger::Log("tcp", "[%s] Received: PLAYER_LEAVE_GAME [0x%04X] guid=%s instance=%lld\n",
				Logger::GetTime(), packet_type, session_guid_.c_str(), (long long)assigned_instance_id_);

			// Disconnect button = voluntary exit → leave the team (a socket
			// drop keeps offline-marked membership instead).
			if (!session_guid_.empty())
				TeamService::HandleVoluntaryExit(session_guid_);

			if (assigned_instance_id_ != 0 && !session_guid_.empty()) {
				nlohmann::json leave;
				leave["type"]         = IpcProtocol::MSG_PLAYER_LEAVE;
				leave["session_guid"] = session_guid_;
				leave["register_token"] = active_register_token_;
				bool ok = IpcServer::SendToInstance(assigned_instance_id_, leave.dump());
				Logger::Log("tcp", "[TcpSession] PLAYER_LEAVE dispatched guid=%s instance=%lld token=%llu send=%d\n",
					session_guid_.c_str(), (long long)assigned_instance_id_,
					(unsigned long long)active_register_token_, (int)ok);
			}

			// Drop the instance assignment so a subsequent SELECT_CHARACTER goes
			// through wait_for_home_map_then_register again from a clean slate.
			assigned_instance_id_      = 0;
			active_register_token_     = 0;
			pending_match_instance_id_ = 0;
			pending_match_game_mode_.clear();
			break;
		}
		case GA_U::GSC_CHANGE_INSTANCE: {
			// Sent when the player clicks "End Mission" on the post-match
			// screen OR opens the in-mission escape menu and chooses Exit.
			// Same packet, two scenarios — distinguished by whether the
			// parent's BeginEndMission has fired (end_mission_at IS NOT NULL).
			//
			// Decision matrix:
			//   parent.end_mission_at IS NULL    → in-mission exit, always home
			//   ... NOT NULL, queue.continue=0   → end-mission button, home (vanilla)
			//   ... NOT NULL, queue.continue=1   → end-mission button, try successor
			//                                       — if no successor or full → home fallback
			PacketView pkt(data + 6, length - 6);
			uint32_t targetMapGameId  = pkt.Read4B(GA_T::MAP_GAME_ID).value_or(0);
			uint32_t targetInstanceId = pkt.Read4B(GA_T::MAP_INSTANCE_ID).value_or(0);

			Logger::Log("tcp", "[%s] Received: GSC_CHANGE_INSTANCE [0x%04X] guid=%s mapGameId=%u instanceId=%u (from=%lld)\n",
				Logger::GetTime(), packet_type, session_guid_.c_str(),
				targetMapGameId, targetInstanceId, (long long)assigned_instance_id_);

			if (targetMapGameId != 0 || targetInstanceId != 0) {
				Logger::Log("tcp", "[TcpSession] GSC_CHANGE_INSTANCE: targeted switch not implemented (mapGameId=%u instanceId=%u) — ignoring\n",
					targetMapGameId, targetInstanceId);
				break;
			}

			// Snapshot the parent before we tear down the seat — we need
			// its end_mission_at + queue_id to decide where to route.
			int64_t parent_instance_id = assigned_instance_id_;
			// Already routed from this instance on its RequestMission: the
			// client's own late change-instance must not route a second time.
			if (parent_instance_id != 0 && parent_instance_id == early_route_from_) {
				// The client sends CHANGE_INSTANCE only from Esc >
				// Quit Tutorial / End Mission (TgUIInGameMainMenu.OnEndMissionButton,
				// the exit-mission key) or its idle auto-kick - never after a
				// RequestMission. An explicit quit beats the pending chapter.
				if (chapter_pending_instance_ != 0) {
					tutorial_quit_ = true;
					Logger::Log("tcp", "[TcpSession] Quit Tutorial during chapter hand-off (instance=%lld -> pending %lld): %s goes home\n",
						(long long)parent_instance_id, (long long)chapter_pending_instance_, session_guid_.c_str());
				} else {
					Logger::Log("tcp", "[TcpSession] GSC_CHANGE_INSTANCE: already routing from instance=%lld after RequestMission -- ignored for %s\n",
						(long long)parent_instance_id, session_guid_.c_str());
				}
				break;
			}
			std::optional<InstanceInfo> parent;
			if (parent_instance_id != 0) {
				parent = InstanceRegistry::GetInstanceById(parent_instance_id);
			}
			if (parent && parent->is_home_map && parent->state == "READY") {
				pending_match_instance_id_ = 0;
				pending_match_game_mode_.clear();

				if (parent->map_name == "Dome3_VR_Arena_P") {
					nlohmann::json action;
					action["type"] = IpcProtocol::MSG_PLAYER_ACTION;
					action["session_guid"] = session_guid_;
					action["action"] = "return_home_area";

					bool ok = IpcServer::SendToInstance(parent_instance_id, action.dump());
					Logger::Log("tcp",
						"[TcpSession] GSC_CHANGE_INSTANCE: same-home VR return action instance=%lld send=%d for %s\n",
						(long long)parent_instance_id, (int)ok, session_guid_.c_str());
					break;
				}

				Logger::Log("tcp",
					"[TcpSession] GSC_CHANGE_INSTANCE: home map %s has no same-home return handler; keeping connection for %s\n",
					parent->map_name.c_str(), session_guid_.c_str());
				break;
			}

			// NB: do NOT send MSG_PLAYER_LEAVE here, even though that would
			// tear the mission-side connection down immediately. PLAYER_LEAVE
			// → NetConnection__Cleanup → engine UNetConnection::Cleanup
			// crashes mid-mission with a null deref at GlobalAgenda.exe+0x4283,
			// because the connection is still fully live (live PlayerController,
			// Pawn, channel array, AI/objective references). The Disconnect
			// path doesn't crash because the client tears down its UDP socket
			// first so the engine has already quiesced the connection by the
			// time we ask it to clean up.
			//
			// We still mark the control-server roster row left before routing,
			// so empty mission instances can be stopped without waiting for
			// the old NetConnection timeout. We just avoid calling the unsafe
			// in-engine cleanup path on a live connection.
			//
			// TODO(crash-debug): find what's null inside CallOriginal at
			// NetConnection__Cleanup.cpp:84 when called on a live connection,
			// then re-enable an immediate teardown here.
			// No end screen (e.g. the intro's closing matinee hands the client
			// straight to the next map): the matinee fade ends and the client
			// falls back to its own pawn camera until the loading screen. Hold
			// the screen black on the old instance, then route a moment later
			// so the fade lands before the client drops that connection.
			if (parent && !parent->is_home_map && parent->end_mission_at == 0
			    && parent->state == "READY") {
				nlohmann::json fade;
				fade["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
				fade["session_guid"] = session_guid_;
				fade["action"]       = "camera_fade";
				fade["args"]         = { {"on", true}, {"alpha", 1.0}, {"time", 0.05} };
				const bool sent = IpcServer::SendToInstance(parent_instance_id, fade.dump());
				Logger::Log("tcp",
					"[TcpSession] GSC_CHANGE_INSTANCE: hold-black on instance=%lld send=%d, routing in %d ms for %s\n",
					(long long)parent_instance_id, (int)sent, sent ? kExitFadeHoldMs : 0, session_guid_.c_str());
				if (sent) {
					auto self = shared_from_this();
					auto t = std::make_shared<asio::steady_timer>(io_ctx_);
					t->expires_after(std::chrono::milliseconds(kExitFadeHoldMs));
					t->async_wait([self, t, parent_instance_id](std::error_code ec) {
						if (ec || self->assigned_instance_id_ != parent_instance_id) return;
						self->route_from_mission_instance(parent_instance_id, "client change-instance");
					});
					break;
				}
			}
			route_from_mission_instance(parent_instance_id, "client change-instance");
			break;
		}
		case GA_U::ADD_PLAYER_CHARACTER: {
			Logger::Log("tcp", "[%s] Received: ADD_PLAYER_CHARACTER [0x%04X]\n", Logger::GetTime(), packet_type);
			// Field set confirmed from a live capture: PROFILE_ID, HEAD_ASM_ID,
			// HAIR_ASM_ID, GENDER_TYPE_VALUE_ID, DWORDS (morph blob), CALLER_ID.
			// The client sends no SKIN/EYE_MATERIAL_PARAMETER_ID — skin tone is
			// carried inside the morph blob, so those two columns stay 0.
			PacketView pkt(data + 6, length - 6);
			// The client names its training map here; used by
			// start_intro_mission() below (intro_mission config).
			uint32_t trainingMapGameId  = pkt.Read4B(GA_T::TRAINING_MAP_GAME_ID).value_or(0);
			uint32_t profileId          = pkt.Read4B(GA_T::PROFILE_ID).value_or(GA_G::PROFILE_ID_ASSAULT);
			uint32_t headAsmId          = pkt.Read4B(GA_T::HEAD_ASM_ID).value_or(1605);
			uint32_t genderTypeId       = pkt.Read4B(GA_T::GENDER_TYPE_VALUE_ID).value_or(853);
			// CharacterInfoStruct also carries hair / skin / eye selections
			// from the head menu (TgDataInterface.uc + TgLoginHUD.uc:128).
			// Previously discarded → server hardcoded HAIR_ASM_ID=0x85D in
			// the character-list response, which is incompatible with most
			// non-default heads and crashes the client during pawn-asm
			// attach. Persist what the player actually picked.
			// 1974 = NewHair15 (asm_mesh_type_value_id=850, the in-game
			// pawn hair category) — same asm SpawnBotPawn sets on bots.
			// Asm 0 doesn't exist; asm 403 PC_CHARBUILD_Bald is in the
			// character-builder UI category (type 596) and isn't
			// loadable from the in-game pawn-attach pipeline — both
			// crash the client at 0x109d1f5b.
			uint32_t hairAsmId          = pkt.Read4B(GA_T::HAIR_ASM_ID).value_or(1974);
			uint32_t skinMatParamId     = pkt.Read4B(GA_T::SKIN_MATERIAL_PARAMETER_ID).value_or(0);
			uint32_t eyeMatParamId      = pkt.Read4B(GA_T::EYE_MATERIAL_PARAMETER_ID).value_or(0);

			// DWORDS payload: 4B length prefix + raw morph bytes — strip prefix before storing.
			std::vector<uint8_t> morphBlob;
			auto dwordsRaw = pkt.ReadNBytes(GA_T::DWORDS);
			if (dwordsRaw && dwordsRaw->size() > 4)
				morphBlob.assign(dwordsRaw->begin() + 4, dwordsRaw->end());

			// Server-side creation rules. The client UI only
			// offers valid choices, but nothing stopped a crafted packet from
			// storing an unknown class / gender (later crashes the pawn
			// build) or creating unlimited agents. Same cap as the client's
			// CONST_TGCS_MAX_CHARACTERS (8). On reject, resend the roster so
			// the character-select scene leaves its wait state.
			{
				const char* reject = nullptr;
				if (profileId != GA_G::PROFILE_ID_ASSAULT && profileId != GA_G::PROFILE_ID_MEDIC &&
				    profileId != GA_G::PROFILE_ID_RECON && profileId != GA_G::PROFILE_ID_ROBOTIC)
					reject = "unknown class profile";
				else if (genderTypeId != GA_G::GENDER_TYPE_ID_MALE && genderTypeId != GA_G::GENDER_TYPE_ID_FEMALE)
					reject = "unknown gender";
				else if (morphBlob.size() > 4096 || morphBlob.size() % 8 != 0)
					reject = "bad morph blob size";
				else if (user_id_ == 0)
					reject = "not logged in";
				else if (PlayerSessionStore::GetCharactersByUserId(user_id_).size() >= 8)
					reject = "account already has 8 agents";
				if (reject) {
					Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER rejected (%s): profile=%u gender=%u morphBytes=%u user=%lld\n",
						Logger::GetTime(), reject, profileId, genderTypeId,
						(unsigned)morphBlob.size(), (long long)user_id_);
					send_character_list_response();
					break;
				}
			}

			EconParity::SanitizeCreateLooks(headAsmId, hairAsmId);  // no bald 403 / unknown asm
			selected_character_id_ = PlayerSessionStore::InsertCharacter(
				user_id_, profileId, headAsmId, genderTypeId, morphBlob,
				hairAsmId, skinMatParamId, eyeMatParamId);
			if (selected_character_id_ == 0) {
				selected_profile_id_ = 0;
				Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER failed: profile=%u user=%lld -- not registering player\n",
					Logger::GetTime(), profileId, (long long)user_id_);
				break;
			}
			selected_profile_id_   = profileId;
			PlayerSessionStore::SetSelectedCharacter(session_guid_, selected_character_id_, selected_profile_id_);

			Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER: profile=%u head=%u gender=%u hair=%u skin=%u eye=%u morphBytes=%u charId=%lld\n",
				Logger::GetTime(), profileId, headAsmId, genderTypeId,
				hairAsmId, skinMatParamId, eyeMatParamId,
				(unsigned)morphBlob.size(), selected_character_id_);

			// Creating an agent selects it without a
			// GSC_SELECT_CHARACTER, so run the login mail hook here too or a
			// brand-new player's first session has an empty inbox.
			deliver_login_mail(user_id_, selected_character_id_, player_name);
			send_add_player_character_response();
			// Pair the ADD with an implicit SELECT so the client opens its
			// chat connection. CGameClient__HandleSelectCharacterResponseInternal
			// is the only response handler in the binary that reads CHAT_NET_ADDR
			// and sends the SESSION_GUID handshake — without this, new-player
			// chat sockets never open. selected_character_id_/profile_id_ are
			// already set from InsertCharacter above, so the response auto-
			// targets the freshly-created character.
			send_select_character_response();
			// Both tutorial and normal paths use wait-for-READY then PLAYER_REGISTER flow.
			// New agents go to the intro mission first when enabled.
			if (!start_intro_mission(trainingMapGameId))
				wait_for_home_map_then_register(120);  // 120 second timeout
			break;
		}
		case GA_U::DELETE_CHARACTER: {
			PacketView pkt(data + 6, length - 6);
			int64_t nCharacterId = pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
			Logger::Log("tcp", "[%s] Received: DELETE_CHARACTER [0x%04X] charId=%lld user=%lld\n",
				Logger::GetTime(), packet_type, (long long)nCharacterId, (long long)user_id_);

			bool deleted = nCharacterId != 0 &&
				PlayerSessionStore::SoftDeleteCharacter(nCharacterId, user_id_);
			if (!deleted)
				Logger::Log("tcp", "[%s] DELETE_CHARACTER: charId=%lld not deleted (not found, wrong user, or already deleted)\n",
					Logger::GetTime(), (long long)nCharacterId);

			// Push a fresh GSC_CHARACTER_LIST either way — the character-select
			// scene's UpdateCharacterListCallback rebuilds the roster from it,
			// which both removes the deleted entry and leaves the
			// CSS_DELETING_CHARACTER wait state.
			send_character_list_response();
			break;
		}
		case GA_U::GET_CHARACTER_AUCTION_SELL_ITEMS:
		case GA_U::GET_CHARACTER_AUCTION_BID_ITEMS:
		case GA_U::GET_FILTERED_AUCTION_ITEMS:
		case GA_U::ADD_AUCTION_ITEM:
		case GA_U::BID_AUCTION_ITEM:
		case GA_U::CANCEL_AUCTION_ITEM:
		case GA_U::BUYOUT_AUCTION_ITEM: {
			Logger::Log("mail", "[%s] Received: auction op [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_auction_packet(packet_type, pkt);
			break;
		}
		case GA_U::UPDATE_NEW_MAIL_COUNT:
			Logger::Log("tcp", "[%s] Received: UPDATE_NEW_MAIL_COUNT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_update_new_mail_count_response();
			break;
		case GA_U::GET_CHARACTER_MAILS:
		case GA_U::READ_MAIL:
		case GA_U::DELETE_MAIL:
		case GA_U::ACCEPT_MAIL:
		case GA_U::SEND_MAIL:
		case GA_U::DECLINE_COD_MAIL: {
			Logger::Log("mail", "[%s] Received: mail op [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_mail_packet(packet_type, pkt);
			break;
		}
		case GA_U::HEX_GET_ALL_ZONES:
			Logger::Log("tcp", "[%s] Received: HEX_GET_ALL_ZONES [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_hex_get_all_zones_response();
			break;
		case GA_U::HEX_GET_ZONE_DATA: {
			PacketView pkt(data + 6, length - 6);
			uint32_t nZoneObjectId = pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0);
			Logger::Log("tcp", "[%s] Received: HEX_GET_ZONE_DATA [0x%04X], zone object %u\n", Logger::GetTime(), packet_type, nZoneObjectId);
			send_hex_get_zone_data_response(nZoneObjectId);
			send_strike_force_inventory();
			send_ava_stock_line();
			ava_zone_asked_ = nZoneObjectId;
			ava_zone_asked_at_ = (int64_t)std::time(nullptr);
			if (AvaShields::DeedsToBag(selected_character_id_)) {
				send_inventory_clear(last_pawn_id_, selected_character_id_);
				send_inventory_response(last_pawn_id_, selected_character_id_);
				sync_item_count(selected_character_id_);
			}
			break;
		}
		case GA_U::HEX_CAPTURE:
		case GA_U::HEX_DEFEND: {
			PacketView pkt(data + 6, length - 6);
			uint32_t nZoneObjectId = pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0);
			uint32_t nPosition = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			Logger::Log("tcp", "[%s] Received: %s [0x%04X], zone %u pos %u\n", Logger::GetTime(),
				packet_type == GA_U::HEX_CAPTURE ? "HEX_CAPTURE" : "HEX_DEFEND", packet_type, nZoneObjectId, nPosition);
			handle_hex_attack(nZoneObjectId, nPosition, packet_type == GA_U::HEX_DEFEND);
			break;
		}
		case GA_U::HEX_GOTO_BATTLE:
			if (const uint32_t g = AvaZones::BattleOf(session_guid_); g != AvaZones::kBad) {
				Logger::Log("tcp", "[%s] Received: HEX_GOTO_BATTLE [0x%04X] -> open fight %u\n", Logger::GetTime(), packet_type, g);
				handle_hex_attack(AvaZones::ZoneOf(g), g, false);
				break;
			}
			[[fallthrough]];
		case GA_U::HEX_WITHDRAW:
		case GA_U::HEX_LEAVE_BATTLE:
			// AvA battles run through the normal match queue + invite popup (handle_hex_attack);
			// these campaign-map buttons have no separate state yet.
			Logger::Log("tcp", "[%s] Received: HEX battle button [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			if (packet_type != GA_U::HEX_GOTO_BATTLE && current_match_queue_id_ != 0)
				ChatSession::SystemMessageToGuid(session_guid_, "*** To call off an AvA attack, leave the match queue (queue panel). ***");
			break;
		case GA_U::HEX_ADD_FACTORY_ITEM:
		case GA_U::HEX_REMOVE_FACTORY_ITEM: {
			PacketView pkt(data + 6, length - 6);
			uint32_t nPosition = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			uint32_t nItemId = pkt.Read4B(GA_T::INVENTORY_ID).value_or(0);
			uint32_t nCount = pkt.Read4B(GA_T::INSTANCE_COUNT).value_or(1);
			Logger::Log("tcp", "[%s] Received: %s [0x%04X], pos %u item %u x%u\n", Logger::GetTime(),
				packet_type == GA_U::HEX_ADD_FACTORY_ITEM ? "HEX_ADD_FACTORY_ITEM" : "HEX_REMOVE_FACTORY_ITEM",
				packet_type, nPosition, nItemId, nCount);
			handle_hex_factory_item(nPosition, nItemId, nCount, packet_type == GA_U::HEX_REMOVE_FACTORY_ITEM);
			break;
		}
		case GA_U::HEX_ASSIGN_RESOURCE_ITEM:
		case GA_U::HEX_ASSIGN_HEADQUARTERS: {
			// Client senders 0x10926960 {ZONE_OBJECT_ID, POSITION, ITEM_ID} /
			// 0x10926a20 {ZONE_OBJECT_ID, POSITION}.
			PacketView pkt(data + 6, length - 6);
			uint32_t nPosition = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			uint32_t nItemId = pkt.Read4B(GA_T::ITEM_ID).value_or(0);
			Logger::Log("tcp", "[%s] Received: %s [0x%04X], pos %u item %u\n", Logger::GetTime(),
				packet_type == GA_U::HEX_ASSIGN_HEADQUARTERS ? "HEX_ASSIGN_HEADQUARTERS" : "HEX_ASSIGN_RESOURCE_ITEM",
				packet_type, nPosition, nItemId);
			if (packet_type == GA_U::HEX_ASSIGN_HEADQUARTERS) handle_hex_assign_headquarters(nPosition);
			else handle_hex_assign_resource(nPosition, nItemId);
			break;
		}
		case GA_U::HEX_ADD_FACILITY: {
			// Client sender 0x109267d0 {ZONE_OBJECT_ID, POSITION, INVENTORY_ID}.
			PacketView pkt(data + 6, length - 6);
			uint32_t nPosition = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			uint32_t nId = pkt.Read4B(GA_T::INVENTORY_ID).value_or(0);
			Logger::Log("tcp", "[%s] Received: HEX_ADD_FACILITY [0x%04X], pos %u id %u\n", Logger::GetTime(),
				packet_type, nPosition, nId);
			handle_hex_add_facility(nPosition, nId);
			break;
		}
		case GA_U::GET_COMBAT_LOG: {
			PacketView pkt(data + 6, length - 6);
			send_ava_combat_log(pkt.Read4B(GA_T::HEX_ZONE_ID).value_or(1), pkt.ReadString(GA_T::AGENCY_NAME).value_or("%"),
				pkt.Read4B(GA_T::EVENT_VALUE_ID).value_or(0), pkt.ReadString(GA_T::DATE_START).value_or(""), pkt.ReadString(GA_T::DATE_END).value_or(""));
			break;
		}
		case GA_U::HEX_GET_FULL_ACA:
			send_hex_full_aca();
			break;
		case GA_U::HEX_GET_AUCTION: {  // the auction panel opens (exe 0x113df960): {ZONE_OBJECT_ID, POSITION}
			PacketView pkt(data + 6, length - 6);
			send_hex_auction(AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF)));
			break;
		}
		case GA_U::HEX_BID:
		case GA_U::HEX_BID_CANCEL: {
			// Client sender 0x10926640: HEX_BID {ZONE_OBJECT_ID, POSITION, BID_AMOUNT,
			// HEX_ATTACK_TYPE, INVENTORY_ID}. No client sender for HEX_BID_CANCEL found; handled anyway.
			PacketView pkt(data + 6, length - 6);
			uint32_t nPosition = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			uint32_t nAmount = pkt.Read4B(GA_T::BID_AMOUNT).value_or(0);
			uint32_t nAttackType = pkt.Read4B(GA_T::HEX_ATTACK_TYPE).value_or(0);
			Logger::Log("tcp", "[%s] Received: %s [0x%04X], pos %u amount %u attack type %u\n", Logger::GetTime(),
				packet_type == GA_U::HEX_BID ? "HEX_BID" : "HEX_BID_CANCEL", packet_type, nPosition, nAmount, nAttackType);
			if (packet_type == GA_U::HEX_BID && nAmount != 0) {  // bid gate (shielded hex needs a missile in range)
				const auto ms = AvaShields::CheckBid(selected_character_id_, nPosition, pkt.Read4B(GA_T::INVENTORY_ID).value_or(0));
				if (ms.msg_id) send_team_system_message(ms.msg_id, "");
				if (!ms.text.empty()) ChatSession::SystemMessageTo(player_name, ms.text);
				if (ms.refuse) break;
			}
			handle_hex_bid(nPosition, nAmount, nAttackType, packet_type == GA_U::HEX_BID_CANCEL || nAmount == 0);
			break;
		}
		case GA_U::ADD_STRIKE_FORCE_TEAM_ITEM:
		case GA_U::REMOVE_STRIKE_FORCE_TEAM_ITEM: {  // cargo menu {INVENTORY_ID, [ITEM_ID,] INSTANCE_COUNT, TASK_TEAM} -> AvaCargo
			PacketView pkt(data + 6, length - 6);
			const uint32_t nInv = pkt.Read4B(GA_T::INVENTORY_ID).value_or(0);
			const uint32_t nItem = pkt.Read4B(GA_T::ITEM_ID).value_or(0);
			const uint32_t nCount = pkt.Read4B(GA_T::INSTANCE_COUNT).value_or(1);
			Logger::Log("tcp", "[%s] Received: %s_STRIKE_FORCE_TEAM_ITEM [0x%04X] inv %u item %u x%u tt %u\n", Logger::GetTime(),
				packet_type == GA_U::ADD_STRIKE_FORCE_TEAM_ITEM ? "ADD" : "REMOVE", packet_type, nInv, nItem, nCount, pkt.Read4B(GA_T::TASK_TEAM).value_or(0));
			const std::string line = packet_type == GA_U::ADD_STRIKE_FORCE_TEAM_ITEM
				? AvaCargo::Load(selected_character_id_, nInv, nItem, nCount) : AvaCargo::Unload(selected_character_id_, nInv, nCount);
			if (!line.empty()) ChatSession::SystemMessageTo(player_name, line);
			send_strike_force_inventory();
			break;
		}
		case GA_U::HEX_ASSIGN_DEFENSIVE_SHIELDS: {  // {ZONE_OBJECT_ID, POSITION, SHIELD_POSITION_FLAGS} -> AvaShields
			PacketView pkt(data + 6, length - 6);
			const uint32_t nZone = pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0);
			const uint32_t nPos = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			const uint32_t nFlags = pkt.Read4B(GA_T::SHIELD_POSITION_FLAGS).value_or(0);
			Logger::Log("tcp", "[%s] Received: HEX_ASSIGN_DEFENSIVE_SHIELDS [0x%04X], pos %u flags 0x%x\n", Logger::GetTime(), packet_type, nPos, nFlags);
			const auto r = AvaShields::Assign(selected_character_id_, nPos, nFlags);
			if (r.msg_id) send_team_system_message(r.msg_id, "");
			if (!r.text.empty()) ChatSession::SystemMessageTo(player_name, r.text);
			if (r.zone_changed && nZone) send_hex_get_zone_data_response(nZone);
			break;
		}
		case GA_U::HEX_GIVE_OWNERSHIP:
		case GA_U::HEX_ABANDON: {  // hex trading: {ZONE_OBJECT_ID, POSITION[, NAME]} -> AvaParity
			PacketView pkt(data + 6, length - 6);
			const uint32_t nPos = AvaZones::ToGlobal(pkt.Read4B(GA_T::ZONE_OBJECT_ID).value_or(0), pkt.Read4B(GA_T::POSITION).value_or(0xFFFFFFFF));
			const auto r = packet_type == GA_U::HEX_ABANDON ? AvaParity::AbandonTerritory(selected_character_id_, nPos)
				: AvaParity::GiveTerritory(selected_character_id_, nPos, pkt.ReadString(GA_T::NAME).value_or(""), ChatSession::OnlinePlayerNames());
			if (r.msg_id) send_team_system_message(r.msg_id, "");
			if (!r.text.empty()) ChatSession::SystemMessageTo(player_name, r.text);
			if (!r.notify_name.empty()) ChatSession::SystemMessageTo(r.notify_name, r.notify_text);
			break;
		}
		case GA_U::HEX_CANCEL_UPDATES:
			// client leaves/switches the zone view; we push no live zone updates, nothing to cancel
			Logger::Log("tcp", "[%s] Received: HEX_CANCEL_UPDATES [0x%04X]\n", Logger::GetTime(), packet_type);
			if (AvaZones::BattleOf(session_guid_) != AvaZones::kBad) send_hex_campaign_state(0, AvaZones::kBad);
			break;
		case GA_U::GET_TICKET_INFO:
			Logger::Log("tcp", "[%s] Received: GET_TICKET_INFO [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_get_ticket_info_response();
			break;
		case GA_U::MATCH_JOIN: {
			Logger::Log("tcp", "[%s] Received: MATCH_JOIN [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			uint32_t matchQueueId = pkt.Read4B(GA_T::MATCH_QUEUE_ID).value_or(0);
			uint32_t matchFilters = pkt.Read4B(GA_T::MATCH_FILTERS).value_or(0);

			if (session_guid_.empty() || selected_character_id_ == 0) {   // queue only after login
				Logger::Log("tcp", "[%s] MATCH_JOIN before login/character select -- dropped\n", Logger::GetTime());
				break;
			}
			MatchModeFilter::OnClientJoinFlags(session_guid_, matchFilters);  // Late-backfill volunteer box
			handle_match_join(matchQueueId, matchFilters);
			break;
		}
		case GA_U::MATCH_LEAVE: {
			Logger::Log("tcp", "[%s] Received: MATCH_LEAVE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);

			// Teamed leave works for every member, queued or
			// popped. Still queued: the team queues as one party, so any
			// member's leave pulls the whole team out (the composition-change
			// dequeue clears every member's HUD + "you left the match queue").
			// Popped (invitation up, instance starting on the server or a
			// player's PC): the party is past the queue, so RemoveParty finds
			// nothing - the leader then cancels for the whole team, a member
			// declines only for themselves (the rest keep their invitation).
			if (TeamService::IsTeamed(session_guid_)) {
				const bool leader = TeamService::IsLeader(session_guid_);
				TeamService::DequeueForCompositionChange(session_guid_);
				std::vector<std::string> targets{session_guid_};
				if (leader) targets = TeamService::GetTeamMemberGuids(session_guid_);
				Logger::Log("team", "[team] MATCH_LEAVE from %s (%s) -> %zu member(s)\n",
					player_name.c_str(), leader ? "leader" : "member", targets.size());
				for (const auto& g : targets) {
					if (g == session_guid_) send_match_leave_response();
					else DeliverMatchLeave(g);
				}
				break;
			}

			send_match_leave_response();
			break;
		}
		case GA_U::MATCH_ACCEPT:
			Logger::Log("tcp", "[%s] Received: MATCH_ACCEPT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_match_accept_response();
			break;
		case GA_U::AGENCY_GET_ROSTER:
			Logger::Log("tcp", "[%s] Received: AGENCY_GET_ROSTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_agency_get_roster_response();
			break;
		case GA_U::AGENCY_GET_DIRECTORY: {
			Logger::Log("tcp", "[%s] Received: AGENCY_GET_DIRECTORY [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			send_agency_get_directory_response(pkt);
			break;
		}
		case GA_U::AGENCY_CREATE: {
			Logger::Log("tcp", "[%s] Received: AGENCY_CREATE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_create(pkt);
			break;
		}
		// Solo leader disband: "remove" on the last member confirms as a disband.
		// The client may send DISBAND, LEAVE, or a self-targeted REMOVE — all mean
		// "delete my agency" for a solo leader, so route them together.
		case GA_U::AGENCY_DISBAND:
		case GA_U::AGENCY_LEAVE:
		case GA_U::AGENCY_REMOVE:
		case GA_U::AGENCY_PLAYER_REMOVE: {
			Logger::Log("tcp", "[%s] Received: AGENCY disband/leave/remove [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			// A PLAYER_ID that isn't mine = kicking that member; anything else
			// (no target, or myself) is the leave/disband path.
			const int64_t target = (int64_t)pkt.Read4B(GA_T::PLAYER_ID).value_or(0);
			if (target != 0 && target != selected_character_id_) {
				handle_agency_kick(target);
				break;
			}
			handle_agency_disband();
			break;
		}
		case GA_U::AGENCY_SET_NOTE: {
			Logger::Log("tcp", "[%s] Received: AGENCY_SET_NOTE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_set_note(pkt);
			break;
		}
		case GA_U::AGENCY_UPDATE_RECRUITING: {
			Logger::Log("tcp", "[%s] Received: AGENCY_UPDATE_RECRUITING [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_update_recruiting(pkt);
			break;
		}
		case GA_U::AGENCY_SET_OWNER: {
			Logger::Log("tcp", "[%s] Received: AGENCY_SET_OWNER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_set_owner(pkt);
			break;
		}
		case GA_U::AGENCY_UPDATE_RANKS: {
			Logger::Log("tcp", "[%s] Received: AGENCY_UPDATE_RANKS [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_update_ranks(pkt);
			break;
		}
		case GA_U::AGENCY_PROMOTE:
		case GA_U::AGENCY_DEMOTE: {
			Logger::Log("tcp", "[%s] Received: AGENCY_PROMOTE/DEMOTE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_rank_change(pkt, packet_type == GA_U::AGENCY_PROMOTE);
			break;
		}
		case GA_U::AGENCY_INVITE: {
			Logger::Log("tcp", "[%s] Received: AGENCY_INVITE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			if (auto t = pkt.ReadString(GA_T::PLAYER_NAME); t && !Social::Friends::InviteAllowed(*t, player_name, Social::Friends::InviteKind::Agency)) {
				ChatSession::SystemMessageTo(player_name, "*** " + *t + " is not accepting agency invites ***"); break; }
			handle_agency_invite(pkt);
			break;
		}
		// UC has ONE native for both answers — ATgPlayerController::AgencyAccept(bool
		// bAccepted) (TgPlayerController.uc) — so the accept/decline split is assumed
		// to be 0xB7 vs 0xB8 (both opcodes exist). LogData confirms on first test; if
		// decline arrives as 0xB7 + a flag field, gate the join on that flag here.
		case GA_U::AGENCY_ACCEPT_INVITE:
			Logger::Log("tcp", "[%s] Received: AGENCY_ACCEPT_INVITE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			handle_agency_accept();
			break;
		// Directory "Apply/Join": request carries AGENCY_ID (assumed; 0xBA join, 0x194
		// application). Recruiting agencies take the player straight in as Member.
		case GA_U::AGENCY_APPLICATION: {  // ACTION_FLAGS 1 status / 2 apply / 3 cancel / 4 accept / 5 decline / 6 list
			Logger::Log("tcp", "[%s] Received: AGENCY_APPLICATION [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_application(pkt);
			break;
		}
		case GA_U::AGENCY_JOIN: {
			Logger::Log("tcp", "[%s] Received: AGENCY_JOIN/APPLICATION [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_join(pkt);
			break;
		}
		case GA_U::AGENCY_REJECT_INVITE: {
			Logger::Log("tcp", "[%s] Received: AGENCY_REJECT_INVITE [0x%04X]\n", Logger::GetTime(), packet_type);
			std::lock_guard<std::mutex> lock(pending_agency_mutex_);
			pending_agency_invites_.erase(user_id_);
			break;
		}
		// Agency bank / tax / bank log (Social/AgencyBank.cpp)
		case GA_U::AGENCY_UPDATE_TAX: case GA_U::AGENCY_ADD_INVENTORY: case GA_U::AGENCY_MAIL_INVENTORY:
		case GA_U::AGENCY_REMOVE_INVENTORY: case GA_U::AGENCY_UPDATE_CURRENCY: case GA_U::GET_AGENCY_INV_LOG: {
			Logger::Log("tcp", "[%s] Received: AGENCY bank [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_agency_bank_packet(packet_type, pkt);
			break;
		}
		case GA_U::ALLIANCE_GET_MEMBER_LIST:
			// Agency menu. Its Facilities board loads on a zone reply that arrives while the tab is visible
			// (exe 0x113f7d40 event 0x42), the reply to the tab's own request comes too early: send it once more.
			if ((int64_t)std::time(nullptr) - ava_zone_asked_at_ <= 2) {
				auto weak = std::weak_ptr<TcpSession>(shared_from_this());
				auto t = std::make_shared<asio::steady_timer>(io_ctx_, std::chrono::milliseconds(1500));
				const uint32_t zone = ava_zone_asked_;
				t->async_wait([weak, t, zone](const asio::error_code& ec) {
					if (ec) return;
					if (auto s = weak.lock()) s->send_hex_get_zone_data_response(zone);
				});
			}
			Logger::Log("tcp", "[%s] Received: ALLIANCE_GET_MEMBER_LIST [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_alliance_member_list_response();
			break;
		case GA_U::ALLIANCE_CREATE: {
			Logger::Log("tcp", "[%s] Received: ALLIANCE_CREATE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_alliance_create(pkt);
			break;
		}
		case GA_U::ALLIANCE_DISBAND:
			Logger::Log("tcp", "[%s] Received: ALLIANCE_DISBAND [0x%04X]\n", Logger::GetTime(), packet_type);
			handle_alliance_disband();
			break;
		case GA_U::ALLIANCE_REMOVE: {
			Logger::Log("tcp", "[%s] Received: ALLIANCE_REMOVE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_alliance_remove(pkt);
			break;
		}
		case GA_U::ALLIANCE_INVITE: {
			Logger::Log("tcp", "[%s] Received: ALLIANCE_INVITE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			handle_alliance_invite(pkt);
			break;
		}
		case GA_U::ALLIANCE_ACCEPT_INVITE: {
			Logger::Log("tcp", "[%s] Received: ALLIANCE_ACCEPT_INVITE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			handle_alliance_accept();
			break;
		}
		case GA_U::ALLIANCE_REJECT_INVITE: {
			Logger::Log("tcp", "[%s] Received: ALLIANCE_REJECT_INVITE [0x%04X]\n", Logger::GetTime(), packet_type);
			int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
			if (my_agency != 0) {
				std::lock_guard<std::mutex> lock(pending_alliance_mutex_);
				pending_alliance_invites_.erase(my_agency);
			}
			break;
		}
		case GA_U::QUERY_PLAYERS: {
			Logger::Log("tcp", "[%s] Received: QUERY_PLAYERS [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			send_query_players_response(pkt);
			break;
		}
		case GA_U::PLAYER_COMMAND: {
			PacketView pkt(data + 6, length - 6);
			handle_player_command(pkt);
			break;
		}
		// Settings invite flags, report button, GM gag -> Social/Friends.cpp
		case GA_U::SET_INVITE_FLAGS: { PacketView pkt(data + 6, length - 6); LogData(data, length); Social::Friends::HandleSetInviteFlags(user_id_, pkt); break; }
		case GA_U::GET_INVITE_FLAGS: { LogData(data, length); auto r = Social::Friends::BuildInviteFlagsReply(user_id_); send_response(r); break; }
		case GA_U::PLAYER_REPORTED: { PacketView pkt(data + 6, length - 6); LogData(data, length); Social::Friends::HandlePlayerReported(session_guid_, player_name, pkt); break; }
		case GA_U::GAG_PLAYER: { PacketView pkt(data + 6, length - 6); LogData(data, length); Social::Friends::HandleGag(user_id_, player_name, pkt); break; }
		case GA_U::FRIEND_GET_LIST:
			Social::Friends::LogFriendPacket("FRIEND_GET_LIST", user_id_, player_name);
			send_friends_list();
			break;
		case GA_U::FRIEND_UPDATE: {
			PacketView pkt(data + 6, length - 6);
			Social::Friends::LogFriendPacket("FRIEND_UPDATE", user_id_, player_name);
			handle_friend_update(pkt);
			// No incoming FRIEND_UPDATE handler on the client — ack every
			// update by re-sending the full list (client re-renders on 0x1AE).
			send_friends_list();
			break;
		}
		case GA_U::TEAM_INVITE: {
			Logger::Log("tcp", "[%s] Received: TEAM_INVITE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			std::string target = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
			if (target.empty() || session_guid_.empty()) {
				Logger::Log("tcp", "[TCP] TEAM_INVITE dropped (target='%s' guid='%s')\n",
					target.c_str(), session_guid_.c_str());
				break;
			}
			// Ignore gate: invitee has the inviter on their F3 ignore list —
			// the invite is never delivered, inviter gets the same chat line
			// as a blocked whisper.
			if (Database::IsIgnoring(target, player_name)) {
				Logger::Log("friends", "[Friends] team invite blocked: '%s' ignores '%s'\n",
					target.c_str(), player_name.c_str());
				ChatSession::SystemMessageTo(player_name,
					"*** " + target + " is ignoring you ***");
				break;
			}
			if (!Social::Friends::InviteAllowed(target, player_name, Social::Friends::InviteKind::Team)) {   // Settings invite flags
				ChatSession::SystemMessageTo(player_name, "*** " + target + " is not accepting team invites ***"); break; }
			TeamService::RequestInvite(session_guid_, player_name, target);
			break;
		}
		case GA_U::TEAM_ACCEPT:
			// Bare frame — the invitee accepted the invitation popup.
			Logger::Log("tcp", "[%s] Received: TEAM_ACCEPT [0x%04X]\n", Logger::GetTime(), packet_type);
			if (!session_guid_.empty())
				TeamService::AcceptInvite(session_guid_);
			break;
		case GA_U::TEAM_LEAVE:
			// Bare frame — popup decline OR leave-team; TeamService routes.
			Logger::Log("tcp", "[%s] Received: TEAM_LEAVE [0x%04X]\n", Logger::GetTime(), packet_type);
			if (!session_guid_.empty())
				TeamService::HandleLeave(session_guid_);
			break;
		case GA_U::TEAM_KICK: {
			// C→S: team leader removes a member from the team.
			// Payload: CHARACTER_ID of the member to kick.
			Logger::Log("tcp", "[%s] Received: TEAM_KICK [0x%04X]\n", Logger::GetTime(), packet_type);
			PacketView pkt(data + 6, length - 6);
			int64_t target_char_id = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
			if (!session_guid_.empty() && target_char_id != 0)
				TeamService::KickMember(session_guid_, target_char_id);
			break;
		}
		case GA_U::GROUP_SET_LEADER: {
			// C→S: current leader promotes a team member to leader.
			// Payload: CHARACTER_ID of the member to promote.
			Logger::Log("tcp", "[%s] Received: GROUP_SET_LEADER [0x%04X]\n", Logger::GetTime(), packet_type);
			PacketView pkt(data + 6, length - 6);
			int64_t target_char_id = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
			if (!session_guid_.empty() && target_char_id != 0)
				TeamService::PromoteLeader(session_guid_, target_char_id);
			break;
		}
		case GA_U::SEND_PLAYER_SKILLS: {
			// CGameClient::SendSkillsToServer @ 0x1141fd00 sends this when the player
			// hits Save on the skill tree. Payload carries a DATA_SET_CHARACTER_SKILLS
			// array; each record is {SKILL_GROUP_ID, SKILL_ID, POINTS_ALLOCATED}.
			// Persist the allocation and echo the saved state back so the UI picks up
			// the authoritative values.
			Logger::Log("tcp", "[%s] Received: SEND_PLAYER_SKILLS [0x%04X], item count: %d\n",
				Logger::GetTime(), packet_type, item_count);

			if (selected_character_id_ == 0) {
				Logger::Log("tcp", "[%s] SEND_PLAYER_SKILLS: no selected character, dropping\n",
					Logger::GetTime());
				break;
			}

			PacketView pkt(data + 6, length - 6);
			std::vector<SkillRow> skills;
			for (const auto& rec : pkt.GetDataSet(GA_T::DATA_SET_CHARACTER_SKILLS)) {
				SkillRow row{};
				row.skill_group_id = (int)rec.Read4B(GA_T::SKILL_GROUP_ID).value_or(0);
				row.skill_id       = (int)rec.Read4B(GA_T::SKILL_ID).value_or(0);
				row.points         = (int)rec.Read4B(GA_T::POINTS_ALLOCATED).value_or(0);
				// The client only emits rows with points > 0, but guard anyway.
				if (row.skill_group_id && row.skill_id && row.points > 0)
					skills.push_back(row);
			}

			Logger::Log("tcp", "[TCP] SEND_PLAYER_SKILLS: charId=%lld parsed %d skill rows\n",
				(long long)selected_character_id_, (int)skills.size());

			// Same rules as the in-game save (IPC skill_save): a modified
			// client cannot give itself extra points. Rejected = DB kept, echoed back.
			std::string why;
			if (!PlayerSessionStore::ValidateSkillAllocation(selected_character_id_, skills, why)) {
				Logger::Log("tcp", "[TCP] SEND_PLAYER_SKILLS REJECTED char=%lld: %s\n",
					(long long)selected_character_id_, why.c_str());
				send_player_skills_response();
				break;
			}
			PlayerSessionStore::SetSkillsForCharacter(
				selected_character_id_, (int)item_profile_id_, skills);
			SkillTreeCatalog::InvalidateCache(selected_character_id_);

			// Echo back so the UI updates; also refreshes LAST_RESPEC_DATETIME client-side.
			send_player_skills_response();
			break;
		}
		case GA_U::SEND_CHARACTER_CRAFTING_SKILLS: {
			// The client asks with 0x017F when Agent Profile > Stats opens
			// (TgUIAgentProfile_Stat 0x11425ef0, SOURCE_CHARACTER_ID only when inspecting
			// someone else) and fills the Information panel only from the reply
			// (HUD vtable+0x408 -> 0x1142a780 -> 0x1142a120 -> 0x11428b90): XP_VALUE
			// (level + Next Level bar from the client's own XP table), SKILL_RATING =
			// Combat Rating stars (float, clamped 0..5, half steps), CURRENCY_VALUE,
			// TOKEN_VALUE, HZPOINT_VALUE (Agenda Pts). No reply = empty panel.
			PacketView pkt(data + 6, length - 6);
			const int64_t src = (int64_t)pkt.Read4B(GA_T::SOURCE_CHARACTER_ID).value_or(0);
			const bool own = (src == 0 || src == selected_character_id_);
			send_agent_info(own ? selected_character_id_ : src, own, true);
			break;
		}
		case GA_U::SEND_PLAYER_ACHIEVEMENTS: {
			// Achievements tab / Agent Profile asks for the earned
			// list; the reply always fills the OWN cache.
			if (selected_character_id_ == 0) break;
			std::vector<std::pair<int64_t, int>> got;
			for (const auto& u : AchievementService::CheckStarRating(selected_character_id_))
				got.emplace_back(u.character_id, u.achievement_id);
			for (const auto& g : got) send_achievement_notice(g.second);
			send_player_achievements_response();
			break;
		}
		case GA_U::PLAYER_EMOTE: {
			// /emote commands (native handler 0x10914300 -> 0x109217c0)
			// only send {EMOTE_VALUE_ID = TG_EMOTE_TYPE 1..58, PLAYER_ID target};
			// the client plays nothing itself. Forward to the agent's instance,
			// which runs TgPawn.SetEmote (replicated r_eEmoteUpdate -> everyone).
			PacketView pkt(data + 6, length - 6);
			const uint32_t emote = pkt.Read4B(GA_T::EMOTE_VALUE_ID).value_or(0);
			const auto now = std::chrono::steady_clock::now();
			Logger::Log("social", "[Emote] guid=%s emote=%u inst=%u\n", session_guid_.c_str(), emote, (unsigned)assigned_instance_id_);   // trace
			if (emote == 0 || emote >= 0x3B || assigned_instance_id_ == 0 ||
			    now - last_emote_ < std::chrono::milliseconds(800)) break;
			last_emote_ = now;
			nlohmann::json j;
			j["type"]         = IpcProtocol::MSG_PLAYER_EMOTE;
			j["session_guid"] = session_guid_;
			j["emote"]        = emote;
			IpcServer::SendToInstance(assigned_instance_id_, j.dump());
			break;
		}
		case GA_U::UPDATE_CHAR_VISUAL_SETTINGS: {  // Cyber Cuts / Genolab screens (Appearance.hpp)
			PacketView pkt(data + 6, length - 6); Appearance::Request q;
			q.currency = pkt.Read4B(GA_T::CURRENCY_TYPE_VALUE_ID).value_or(0); q.price = pkt.Read4B(GA_T::PRICE).value_or(0);
			q.loot_table_id = pkt.Read4B(GA_T::LOOT_TABLE_ID).value_or(0); q.loot_table_item_id = pkt.Read4B(GA_T::LOOT_TABLE_ITEM_ID).value_or(0);
			q.item_id = pkt.Read4B(GA_T::ITEM_ID).value_or(0); q.hair_asm_id = pkt.Read4B(GA_T::HAIR_ASM_ID).value_or(0); q.head_asm_id = pkt.Read4B(GA_T::HEAD_ASM_ID).value_or(0);
			if (auto dw = pkt.ReadNBytes(GA_T::DWORDS); dw && dw->size() > 4) q.morph.assign(dw->begin() + 4, dw->end());
			const auto r = Appearance::Apply(selected_character_id_, q); if (r.ok) send_mail_notify(0, true);
			SystemMessageToCharacter(selected_character_id_, r.ok ? std::string("New look saved - it shows from your next map load.") : "Appearance change failed: " + r.reason + ".");
			break;
		}
		default:
			if (Social::Teams::HandlePacket(session_guid_, player_name, packet_type, PacketView(data + 6, length - 6))) break;   // strike force/raid/leaderboard/challenge
			Logger::Log("tcp", "[%s] Received unknown packet type: %04X, raw data:\n", Logger::GetTime(), packet_type);
			LogData(data, length);
			break;
	}

}

// MATCH_JOIN for the client queue list and for AvA Attack (HEX_CAPTURE), which joins a
// hidden AvA queue for the territory's facility. Teamed: leader queues the whole party.
void TcpSession::handle_match_join(uint32_t matchQueueId, uint32_t matchFilters) {
	matchQueueId = PvpParity::GateJoin(session_guid_, matchQueueId, matchFilters);  // Deserter, merc bracket, filter log
	if (matchQueueId == 0) return;
	MatchModeFilter::OnJoin(session_guid_, matchFilters);  // Merc game-type checkboxes
	// Teamed: only the leader manages the queue, and joining queues the
	// WHOLE team as one party.
	if (TeamService::IsTeamed(session_guid_)) {
		if (!TeamService::IsLeader(session_guid_)) {
			send_team_system_message(TeamService::MSG_QUEUE_LEADER_ONLY, "");
			return;
		}
		auto party = TeamService::BuildParty(session_guid_);
		if (!party) return;
		auto cfg = MatchmakingService::GetQueueConfig(matchQueueId);
		// DLC gate: the queue list already hides fully-locked queues,
		// but a crafted MATCH_JOIN must not bypass it. A team needs a
		// COMMON map — some pool map every member owns — else no pop
		// could ever seat the whole party (parties are atomic).
		if (cfg) {
			std::vector<std::vector<int64_t>> member_sets;
			for (const auto& member : party->members)
				member_sets.push_back(Database::GetInstalledDlcIds(member.user_id));
			bool locked = !cfg->map_pool.empty();
			for (const auto& e : cfg->map_pool) {
				bool all_own = true;
				for (const auto& set : member_sets)
					if (!mm::EntryPlayableWith(e, set)) { all_own = false; break; }
				if (all_own) { locked = false; break; }
			}
			if (locked) {
				Logger::Log("tcp", "[TcpSession] MATCH_JOIN: queue %u DLC-locked for team of %s\n",
					matchQueueId, session_guid_.c_str());
				ChatSession::SystemMessageToGuid(session_guid_,
					"*** Cannot queue: this queue needs DLC maps that you or a teammate don't have installed (see the launcher's DLC section). ***");
				return;
			}
		}
		// Queue level requirement - every member must meet it.
		if (cfg && cfg->level_min > 1) {
			for (const auto& member : party->members) {
				const auto si = PlayerSessionStore::GetByGuid(member.session_guid);
				const uint32_t lvl = si && si->selected_character_id
					? PlayerSessionStore::GetCharacterLevelXp(si->selected_character_id).level : 0;
				if (lvl < cfg->level_min) {
					Logger::Log("tcp", "[TcpSession] MATCH_JOIN: queue %u needs level %u, team member %s is %u\n",
						matchQueueId, cfg->level_min, member.session_guid.c_str(), lvl);
					ChatSession::SystemMessageToGuid(session_guid_,
						"*** Cannot queue: this mission requires level " + std::to_string(cfg->level_min) +
						" and a team member is level " + std::to_string(lvl) + ". ***");
					return;
				}
			}
		}
		if (cfg && cfg->team_policy == TeamPolicy::Block) {
			Logger::Log("tcp", "[TcpSession] MATCH_JOIN: queue %u blocks teams — leader %s\n",
				matchQueueId, session_guid_.c_str());
			return;
		}
		if (cfg && party->size() > mm::MaxTeamSize(*cfg)) {
			send_team_system_message(TeamService::MSG_TEAM_TOO_LARGE, "");
			return;
		}
		if (Social::Teams::QueueBlocked(session_guid_, cfg ? cfg->map_pool_id : 0)) return;   // strike force: no matchmade queues (32417)
		MatchmakingService::AddParty(matchQueueId, *party);
		TeamService::NotifyTeamQueued(party->party_id, matchQueueId,
			cfg ? cfg->name_msg_id : 0);
		return;
	}
	send_match_join_response(matchQueueId, matchFilters);
}

void TcpSession::send_match_join_response(uint32_t matchQueueId, uint32_t matchFilters) {
    // DLC gate: the queue list already hides fully-locked queues, but a
    // crafted MATCH_JOIN must not bypass it.
    auto cfg = MatchmakingService::GetQueueConfig(matchQueueId);
    if (cfg && !QueuePlayableWithDlc(*cfg, Database::GetInstalledDlcIds(user_id_))) {
        Logger::Log("tcp", "[TcpSession] MATCH_JOIN: queue %u DLC-locked for user %lld\n",
            matchQueueId, (long long)user_id_);
        ChatSession::SystemMessageToGuid(session_guid_,
            "*** Cannot queue: this queue needs DLC maps you don't have installed (see the launcher's DLC section). ***");
        return;
    }

    // Queue level requirement (GA 1.35 gates, ga_queues.level_min).
    if (cfg && cfg->level_min > 1) {
        const uint32_t lvl = selected_character_id_
            ? PlayerSessionStore::GetCharacterLevelXp(selected_character_id_).level : 0;
        if (lvl < cfg->level_min) {
            Logger::Log("tcp", "[TcpSession] MATCH_JOIN: queue %u needs level %u, char %lld is %u\n",
                matchQueueId, cfg->level_min, (long long)selected_character_id_, lvl);
            ChatSession::SystemMessageToGuid(session_guid_,
                "*** Cannot queue: this mission requires level " + std::to_string(cfg->level_min) +
                " (you are level " + std::to_string(lvl) + "). ***");
            return;
        }
    }

    current_match_queue_id_ = matchQueueId;

    // -togglesolomode preference — stamped here so the matchmaking rule can
    // give the player a private PARTY_LOCKED instance (CoopMatchRule).
    const bool solo_mode = Database::GetUserPreference(user_id_, "solo_mode") == "1";

    QueuedPlayer player;
    player.session_guid = session_guid_;
    player.profile_id = selected_profile_id_;
    player.user_id = user_id_;  // drives the queue's requires_pvp_verification gate
    player.solo_lock = solo_mode;
    player.joined_at = std::chrono::steady_clock::now();

    MatchmakingService::AddPlayer(matchQueueId, player);

    // Drive the AgentInfo HUD "IN QUEUE: <name>" panel + leave-queue keybind.
    send_match_queue_status(matchQueueId, cfg ? cfg->name_msg_id : 0);

    // Chat confirmation via the vt[+0x54] display route (opcode-agnostic).
    send_team_system_message(18306, "");  // "You have joined a match queue."

    // Solo-mode reminder. Group queues (min_players_to_pop > 1: merc/ddr/sr)
    // can't spawn a 1-player instance — the flag is inert there and the
    // player is matched normally (mirrors the CoopMatchRule gate).
    if (solo_mode) {
        const bool solo_capable = cfg && cfg->min_players_to_pop <= 1;
        ChatSession::SystemMessageToGuid(session_guid_, solo_capable
            ? "*** Solo mode is ON - your mission will be locked to you. "
              "Type -togglesolomode to turn it off. ***"
            : "*** Solo mode does not apply to this group queue — "
              "you will be matched normally. ***");
    }
}

void TcpSession::send_match_leave_response() {
    const bool was_queued = current_match_queue_id_ != 0;
    MatchmakingService::RemovePlayer(session_guid_);
    send_match_queue_status(0, 0);  // hide the HUD queue panel
    if (was_queued)
        send_team_system_message(18308, "");  // "You have left the match queue"
    current_match_queue_id_       = 0;
    // Also clear the invitation-side state in case MATCH_LEAVE arrived
    // between MATCH_INVITATION and MATCH_ACCEPT (player declined the popup
    // by leaving the queue). Otherwise pending_match_instance_id_ would
    // stick around and a later MATCH_ACCEPT could route them to a stale
    // instance.
    pending_match_instance_id_    = 0;
    pending_match_game_mode_.clear();
    pending_match_task_force_     = 1;
}

// The leader's MATCH_LEAVE for a team member. Members who
// already accepted (both ids cleared on GO_PLAY) are left alone.
void TcpSession::DeliverMatchLeave(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return;
    auto session = it->second.lock();
    if (!session) return;
    if (session->current_match_queue_id_ == 0 && session->pending_match_instance_id_ == 0) return;
    session->send_match_leave_response();
}

void TcpSession::send_match_invitation(int64_t instance_id, const std::string& game_mode, int task_force) {
    pending_match_instance_id_ = instance_id;
    pending_match_game_mode_ = game_mode;
    pending_match_task_force_ = task_force;

    std::vector<uint8_t> response;

    uint16_t packet_type = GA_U::MATCH_INVITATION;
    // 0xE4 routes through the SAME vt[+0x6c] handler as MATCH_JOIN before
    // showing the popup, and the client's get_int32_t ZEROES the stored
    // queue-state fields when absent — a bare frame wipes the HUD's
    // "IN QUEUE" state the instant the queue pops. Carry the fields.
    uint16_t item_count = 3;

    append(response, packet_type & 0xFF, packet_type >> 8);
    append(response, item_count & 0xFF, item_count >> 8);

    auto cfg = MatchmakingService::GetQueueConfig(current_match_queue_id_);
    Write4B(response, GA_T::MATCH_QUEUE_ID, current_match_queue_id_);
    Write4B(response, GA_T::NAME_MSG_ID, cfg ? cfg->name_msg_id : 0);
    Write4B(response, GA_T::PENALTY_SECONDS, 0);

    send_response(response);

    Logger::Log("tcp", "[TcpSession] Sent MATCH_INVITATION to %s (instance %lld)\n",
        player_name.c_str(), (long long)instance_id);
}

void TcpSession::DeliverMatchInvitation(const std::string& session_guid, int64_t instance_id, const std::string& game_mode, int task_force) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return;
    auto session = it->second.lock();
    if (!session) return;
    session->send_match_invitation(instance_id, game_mode, task_force);
}

void TcpSession::send_team_system_message(uint32_t msg_id, const std::string& player_name) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::TEAM_INVITE;
	uint16_t item_count = player_name.empty() ? 1 : 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MSG_ID, msg_id);
	if (!player_name.empty())
		WriteString(response, GA_T::PLAYER_NAME, player_name);

	send_response(response);
}

void TcpSession::send_team_invitation_popup(const std::string& leader_name) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::TEAM_INVITATION;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MSG_ID, TeamService::MSG_INVITATION_POPUP);
	WriteString(response, GA_T::LEADER_NAME, leader_name);

	send_response(response);
}

void TcpSession::send_match_queue_status(uint32_t queue_id, uint32_t name_msg_id,
                                         uint32_t penalty_seconds) {
	// S→C MATCH_JOIN (0xE1) → CGameClient vt[+0x6c] @ 0x1091fe40 stores
	// queue id / name msg id / deserter penalty at CGameClient+0xb0..0xc0;
	// TgUIPrimaryHUD_AgentInfo::UpdateQueueValues (0x114d6ae0) renders them
	// as "IN QUEUE: <name>" + leave-queue keybind (or DESERTER countdown).
	// All fields written explicitly — the client stores into persistent
	// fields, so absent fields would keep stale values. queue_id=0 hides.
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MATCH_JOIN;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MATCH_QUEUE_ID, queue_id);
	Write4B(response, GA_T::NAME_MSG_ID, name_msg_id);
	// Write4B(response, GA_T::STATUS_CODE, 608);
	Write4B(response, GA_T::PENALTY_SECONDS, penalty_seconds);

	Logger::Log("tcp", "[TcpSession] send_match_queue_status guid=%s queue=%u nameMsg=%u penalty=%u\n",
		session_guid_.c_str(), queue_id, name_msg_id, penalty_seconds);

	send_response(response);
}

void TcpSession::send_team_invite_expired() {
	std::vector<uint8_t> response;

	// The client's 0x4C handler reads no fields — it just drops the queued
	// team-invitation node and dismisses the popup.
	uint16_t packet_type = GA_U::TEAM_EXPIRED_INVITE;
	uint16_t item_count = 0;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	send_response(response);
}

void TcpSession::send_team_update(const TeamRoster& roster) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::TEAM_UPDATE;
	uint16_t item_count = 6;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Do NOT send SETTING_FLAGS (0x0474) — its presence flips the client
	// parser into the settings-only branch and the roster is ignored.
	Write4B(response, GA_T::TEAM_MODE, roster.team_mode);
	Write4B(response, GA_T::TEAM_ID, (uint32_t)roster.team_id);
	// Team +0x14. The map's 'is this hex mine' test (exe 0x10927740) compares it with the hex owner; 0 = every own hex looks foreign.
	Write4B(response, GA_T::OWNER_AGENCY_ID, roster.team_mode >= 2 ? (uint32_t)AvaParity::OwnerOf(selected_character_id_) : 0);
	Write4B(response, GA_T::OWNER_ALLIANCE_ID, 0);
	Write4B(response, GA_T::LEADER_CHARACTER_ID, (uint32_t)roster.leader_character_id);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, roster.rows.size() & 0xFF, (roster.rows.size() >> 8) & 0xFF);
	for (const auto& row : roster.rows) {
		append(response, 0x0B, 0x00);  // 11 fields per member
		Write4B(response, GA_T::CHARACTER_ID, (uint32_t)row.character_id);
		WriteString(response, GA_T::PLAYER_NAME, row.player_name);
		Write4B(response, GA_T::PROFILE_ID, row.profile_id);
		// 0xC2 is wire-string typed; the parser coerces digits via _wtoi.
		WriteString(response, GA_T::CLASS_MSG_ID, std::to_string(row.class_msg_id));
		Write4B(response, GA_T::STATUS_MSG_ID, row.status_msg_id);
		Write4B(response, GA_T::MAP_NAME_MSG_ID, row.map_name_msg_id);
		Write4B(response, GA_T::LEVEL, row.level);
		WriteFloat(response, GA_T::SKILL_RATING, row.skill_rating);
		Write1B(response, GA_T::OFFLINE_FLAG, row.offline ? 1 : 0);
		WriteString(response, GA_T::AGENCY_NAME, row.agency_name);
		WriteString(response, GA_T::ALLIANCE_NAME, row.alliance_name);
	}

	send_response(response);
}

void TcpSession::DeliverTeamUpdate(const std::string& session_guid,
                                   const TeamRoster& roster) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_update(roster);
}

bool TcpSession::DeliverRawPacket(const std::string& session_guid, std::vector<uint8_t> body) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return false;
	auto session = it->second.lock();
	if (!session) return false;
	session->send_response(body);
	return true;
}

void TcpSession::DeliverTeamSystemMessage(const std::string& session_guid,
                                          uint32_t msg_id, const std::string& player_name) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_system_message(msg_id, player_name);
}

void TcpSession::DeliverTeamInvitation(const std::string& session_guid,
                                       const std::string& leader_name) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_invitation_popup(leader_name);
}

void TcpSession::DeliverTeamInviteExpired(const std::string& session_guid) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_invite_expired();
}

void TcpSession::DeliverMatchQueueStatus(const std::string& session_guid,
                                         uint32_t queue_id, uint32_t name_msg_id) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->current_match_queue_id_ = queue_id;
	session->send_match_queue_status(queue_id, name_msg_id);
}

void TcpSession::DeliverMatchCancelled(const std::string& session_guid, const char* reason) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return;
    auto session = it->second.lock();
    if (!session) return;
    MatchmakingService::RemoveReadyMatchReservation(session_guid, reason);
    Logger::Log("tcp", "[TcpSession] Match cancelled for guid=%s (reason: %s) — clearing queue/match state\n",
        session_guid.c_str(), reason ? reason : "unspecified");
    session->current_match_queue_id_       = 0;
    session->pending_match_instance_id_    = 0;
    session->pending_match_game_mode_.clear();
    session->pending_match_task_force_     = 1;
    // GET_TICKET_INFO polling picks up CURRENT_MATCH_QUEUE_ID=0 for the
    // queue menu; the HUD panel needs the explicit clear frame.
    session->send_match_queue_status(0, 0);
}

void TcpSession::send_match_accept_response() {
    if (pending_match_instance_id_ == 0) {
        Logger::Log("tcp", "[TcpSession] MATCH_ACCEPT from %s but no pending instance\n", player_name.c_str());
        return;
    }

    auto instance = InstanceRegistry::GetInstanceById(pending_match_instance_id_);
    if (!instance || instance->state != "READY") {
        Logger::Log("tcp", "[TcpSession] MATCH_ACCEPT: instance %lld not ready\n",
            (long long)pending_match_instance_id_);
        MatchmakingService::RemoveReadyMatchReservation(
            pending_match_instance_id_, session_guid_, "match accept target not ready");
        pending_match_instance_id_ = 0;
        return;
    }

    Logger::Log("tcp", "[TcpSession] MATCH_ACCEPT from %s — registering with instance %lld then sending go_play\n",
        player_name.c_str(), (long long)instance->instance_id);

    // Build PLAYER_REGISTER JSON with full character state (same as initiate_player_register_and_go_play).
    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = pending_match_task_force_;

    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    // Mirror the skill-tree embedding from initiate_player_register_and_go_play
    // so a match-accept spawn gets the same skill effects as a home-map spawn.
    {
        const int item_profile_id =
            PlayerSessionStore::GetCurrentItemProfile(selected_character_id_);
        this->item_profile_id_ = item_profile_id;
        reg["current_item_profile_id"] = item_profile_id;
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(
                 selected_character_id_, item_profile_id)) {
            nlohmann::json row;
            row["skill_group_id"] = s.skill_group_id;
            row["skill_id"]       = s.skill_id;
            row["points"]         = s.points;
            skillsArr.push_back(row);
        }
        reg["skills"] = skillsArr;
        reg["last_respec_at"] = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
    }

    // Send PLAYER_REGISTER to the MISSION instance (not home map).
    int64_t target_instance_id = pending_match_instance_id_;
    int target_task_force = pending_match_task_force_;
    if (!IpcServer::SendToInstance(target_instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session for instance %lld — cannot register player %s\n",
            (long long)target_instance_id, player_name.c_str());
        MatchmakingService::RemoveReadyMatchReservation(
            target_instance_id, session_guid_, "match accept target has no IPC session");
        pending_match_instance_id_ = 0;
        pending_match_game_mode_.clear();
        return;
    }

    // Arm 60-second ACK-wait timer, then send go_play on success.
    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));

    auto timer = pending_ack_timer_;
    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer, target_instance_id, target_task_force](bool success, int pawn_id) {
            timer->cancel();
            if (!success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK failed for %s on mission instance\n",
                    session_guid_.c_str());
                MatchmakingService::RemoveReadyMatchReservation(
                    target_instance_id, session_guid_, "player register ack failed");
                pending_match_instance_id_ = 0;
                pending_match_game_mode_.clear();
                return;
            }

            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK success for %s pawn_id=%d — sending go_play to mission\n",
                session_guid_.c_str(), pawn_id);

            // The player now lives on the mission instance, not the home
            // instance. Keep assigned_instance_id_ in sync so any subsequent
            // control-server-→-DLL messages (e.g. PLAYER_ACTION /changeteam)
            // route to the right DLL.
            assigned_instance_id_ = target_instance_id;

            // Track player in mission instance
            InstanceRegistry::InsertInstancePlayer(
                target_instance_id, session_guid_, selected_character_id_, target_task_force,
                selected_profile_id_);
            MatchmakingService::RemoveReadyMatchReservation(
                target_instance_id, session_guid_, "player registered in mission");

            auto inst = InstanceRegistry::GetInstanceById(target_instance_id);
            if (!inst || inst->state != "READY") {
                Logger::Log("tcp", "[TcpSession] Mission instance %lld no longer ready\n",
                    (long long)target_instance_id);
                pending_match_instance_id_ = 0;
                pending_match_game_mode_.clear();
                return;
            }

            // Route through the single GSC_GO_PLAY builder so map_game_info
            // resolution applies on this path too. The original duplicate
            // had hardcoded HQ values (22845 / 0x050B / 5716) and was the
            // reason MATCH_ACCEPT-driven launches still saw the fallback.
            send_go_play_to_instance(*inst, target_task_force);

            pending_match_instance_id_ = 0;
            pending_match_game_mode_.clear();
            pending_match_task_force_ = 1;
            current_match_queue_id_ = 0;
            send_match_queue_status(0, 0);  // entering the match — hide HUD queue panel
        });

    pending_ack_timer_->async_wait([this, self, timer](std::error_code ec) {
        if (!ec) {
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK timeout for %s on mission instance\n",
                session_guid_.c_str());
            IpcServer::ClearPendingAck(session_guid_);
            MatchmakingService::RemoveReadyMatchReservation(
                pending_match_instance_id_, session_guid_, "player register ack timeout");
            pending_match_instance_id_ = 0;
            pending_match_game_mode_.clear();
        }
    });
}

void TcpSession::send_get_ticket_info_response() {

	auto queues = MatchmakingService::GetEnabledQueueConfigs();

	// DLC gating: hide queues whose entire (enabled) map pool requires packs
	// this account doesn't have installed.
	{
		const auto installed = Database::GetInstalledDlcIds(user_id_);
		std::vector<QueueConfig> visible;
		visible.reserve(queues.size());
		for (auto& cfg : queues) {
			if (QueuePlayableWithDlc(cfg, installed))
				visible.push_back(std::move(cfg));
		}
		queues = std::move(visible);
	}

	std::vector<uint8_t> response;

	const uint16_t packet_type = GA_U::GET_TICKET_INFO;
	const uint16_t item_count  = 3;  // TEAM_LEADER_FLAG + CURRENT_MATCH_QUEUE_ID + DATA_SET

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count  & 0xFF, item_count  >> 8);

	// Unteamed players queue for themselves; teamed non-leaders lose the
	// join button (only the leader queues the team).
	const bool can_queue = !TeamService::IsTeamed(session_guid_)
	                    || TeamService::IsLeader(session_guid_);
	Write1B(response, GA_T::TEAM_LEADER_FLAG, can_queue ? 0x1 : 0x0);
	Write4B(response, GA_T::CURRENT_MATCH_QUEUE_ID, current_match_queue_id_);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(queues.size() & 0xFF), (uint8_t)(queues.size() >> 8));

	for (const auto& cfg : queues) {
		TicketInfoEncoder::EncodeRecord(response, cfg, RuntimeStats::ForQueue(cfg.queue_id));
	}

	send_response(response);

	// ─── Reference templates for queue rows we haven't added yet. ────────────
	// Kept for documentation while implementing new queue types — copy the
	// magic numbers you need and seed them into ga_queues / ga_map_pool_entries.
	// Compiled out; the active encode path is TicketInfoEncoder above.
#if 0
		// MATCH_QUEUE_ID 5 — Low security (commonwealth missions, tier 1)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000005);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a1); // Low security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a0);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000404);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 6 — Medium security
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000006);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1f7); // Medium security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f6);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000405);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 7 — High security
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000007);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1f9); // High security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000406);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 8 — Maximum security (header only; rest of fields follow the same pattern as 5/6/7)
		// Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000008);
		// Write4B(response, GA_T::NAME_MSG_ID,    0x0000a1fa); // Maximum security

		// MATCH_QUEUE_ID 1 — Ultra max (Commonwealth Prime) — ALREADY SEEDED in ga_queues
		// MATCH_QUEUE_ID 2 — Mercenary / PvP                — ALREADY SEEDED in ga_queues

		// MATCH_QUEUE_ID 10 — Ultra max security (Sonoran Desert)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x0000000a);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9);
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005cb);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 11 — Ultra max security (Mining Province)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x0000000b);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9);
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c6);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 3 — Double Agent (timed/rotating queue, uses REMAINING_SECONDS)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000003);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1fc); // Double Agent
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1fb);
			Write4B(response, GA_T::ICON_ID, 0x000001c9);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x00000004);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000004);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000000e8);
			WriteFloat(response, GA_T::MAP_X, 10.0f);
			WriteFloat(response, GA_T::MAP_Y, 0.5f);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000004);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000004ec);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			// Write4B(response, GA_T::REMAINING_SECONDS, 0x00000001);
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 4 — Sonoran Raid (large-side raid queue)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000005ae);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000004);
			Write4B(response, GA_T::NAME_MSG_ID, 0x00008fdc); // Sonoran Raid
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000e7b0);
			Write4B(response, GA_T::ICON_ID, 0x000006b4);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x00000015);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000015);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x0000003c);
			Write4B(response, GA_T::TAB, 0x000000e7);
			Write4B(response, GA_T::MAP_X, 0x00);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x00000000);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000004eb);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			// Write4B(response, GA_T::REMAINING_SECONDS, 0x00000001);
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);
#endif
}

void TcpSession::send_character_inventory_response(int nPawnId) {
	// Stub — no active callers. Reimplement from ga_character_devices if needed.
	Logger::Log("tcp", "[TCP] send_character_inventory_response: STUB (no-op) pawnId=%d\n", nPawnId);
}

void TcpSession::send_inventory_response(int nPawnId, int64_t character_id) {
	// `item_profile_id_` tracks the active loadout slot — drives which rows
	// land in the equipped pass (the rest go to the bag pass).
	auto devices = PlayerSessionStore::GetDevicesForCharacter(
		character_id, (int)item_profile_id_);

	// Ships TWO blocks of records over SEND_INVENTORY (0x0182):
	//   1. Currently-equipped devices for `character_id`, with
	//      LOCATION_VALUE_ID=369 (EQUIPPED), DEVICE_ID=<inv_id> (= the
	//      backing pawn device's instance id), EQUIPPED_SLOT_VALUE_ID=<svid>,
	//      ACTIVE_FLAG=T.
	//   2. The rest of the account-scoped inventory pool visible to this
	//      character's class profile, with LOCATION_VALUE_ID=370 (ON_HAND),
	//      DEVICE_ID=0, EQUIPPED_SLOT_VALUE_ID=0, ACTIVE_FLAG=F.
	//
	// The DEVICE_ID field is the gate that decides whether the client tries
	// to bind this inventory record to a live actor on the pawn. The receiver
	// path (CGameClient::SendInventory @ 0x109ddb40) at the bottom of its
	// per-record loop scans the inventory manager's item array and runs
	//
	//     if (FUN_10a12f80(item) > 0 && FUN_10a14a50(item) == 0)
	//         debugf("Resubmitting an inventory item that could not find its device");
	//
	// FUN_10a12f80 is literally `return *(item + 0xb4);` and the value at
	// +0xb4 is written by FUN_10a12f90 from the marshal DEVICE_ID (0x0206).
	// FUN_10a14a50 iterates all actors and matches *(actor + 0x220) ==
	// *(item + 0xb4). For bag items there's no backing actor, so we send
	// DEVICE_ID=0 → +0xb4=0 → the gate skips the item entirely.
	//
	// LOCATION_VALUE_ID (marshal 0x0310) defaults to 0x172 (= 370 = ON_HAND)
	// on the client side when omitted or zero — see FUN_10a12340. The 369
	// constant in the equipped block above is the semantic mark; the actual
	// don't-resubmit knob is DEVICE_ID.
	//
	// Earlier attempt that triggered the every-frame resubmit loop sent the
	// bag pool with LOCATION_VALUE_ID=369 and DEVICE_ID=<inv_id>, which made
	// the client believe each bag item was currently equipped — the recovery
	// scan then resubmitted them all back to the server forever.

	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) {
		Logger::Log("tcp",
			"[TCP] send_inventory_response: GetCharacterById(%lld) returned null — skipping\n",
			character_id);
		return;
	}
	const int64_t user_id    = charInfo->user_id;
	const uint32_t profile   = charInfo->profile_id;

	if (devices.empty()) {
		Logger::Log("tcp", "[TCP] send_inventory_response: no equipped devices for charId=%lld pawnId=%d (will still ship bag pool)\n",
			character_id, nPawnId);
	}

	// Bag pool first (we need its size to write the outer DATA_SET count
	// before any record bodies). Filter out anything already covered by the
	// equipped pass — same item shipped twice would be a wasted record.
	std::set<int> equipped_inv_ids;
	for (const auto& d : devices) equipped_inv_ids.insert(d.inventory_id);
	const auto bag_all = PlayerSessionStore::GetInventoryForUser(user_id, profile);
	std::vector<InventoryRow> bag;
	bag.reserve(bag_all.size());
	for (const auto& row : bag_all) {
		if (equipped_inv_ids.count(row.inventory_id)) continue;
		bag.push_back(row);
	}

	// Crafting components ride the same packet. They MUST be here: the client
	// computes Collect (1429) quest progress by summing nInstanceCount over its
	// own inventory map (FUN_10a16310), so a component that isn't a real
	// inventory record leaves the quest log stuck at 0 forever.
	//
	// The matching half is r_ItemCount — the DLL's stamp sites count
	// ga_user_components too, and live drops restate it via the set_item_count
	// action. Ship records here without that and IsValid() fails, blanking
	// every equip slot (reference_equip_screen_is_valid_gate.md).
	const auto components = Database::GetAllComponents(user_id);

	const size_t equipped_count  = devices.size();
	const size_t bag_count       = bag.size();
	const size_t component_count = components.size();
	const size_t total_records   = equipped_count + bag_count + component_count;

	Logger::Log("tcp",
		"[TCP] send_inventory_response: charId=%lld pawnId=%d equipped=%zu bag=%zu components=%zu (bundled into one SEND_INVENTORY)\n",
		character_id, nPawnId, equipped_count, bag_count, component_count);

	// Ledger line on the same channel as every r_ItemCount stamp: this total is
	// exactly how many entries the client's map should end up with, so a stamp
	// that disagrees with the nearest preceding line here is the desync.
	Logger::Log("loot",
		"[Loot] shipped inventory: char=%lld equipped=%zu bag=%zu components=%zu total=%zu\n",
		(long long)character_id, equipped_count, bag_count, component_count, total_records);

	if (total_records == 0) return;

	std::vector<uint8_t> response;

	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // 2 top-level fields: PAWN_ID + DATA_SET

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(total_records & 0xFF), (uint8_t)(total_records >> 8));

	// --- Equipped block. 14 fields per record (12 scalars + 2 nested data_sets).
	//
	// nBlueprintId gates m_pAmBlueprint on the client (FUN_10a12740 sets it
	// from this field). The mod-letter [...] suffix is rendered only when
	// m_pAmBlueprint is non-null, so for any item carrying rolled mods we
	// must send a real blueprint_id whose created_item_id matches this
	// device. Items with no mods send 0 (no fake blueprint, no suffix).
	// When d.oc is set, picks a blueprint with override_name_msg_id != 0
	// so the client renders the "OC" name suffix.
	//
	// v76: cosmetic rows (item_id > 0, device_id = 0) marshal as their
	// item_id and ship DEVICE_ID=0 so the client's FUN_10a12f80 recovery
	// scan doesn't try to bind them to a live actor (cosmetics have no
	// ATgDevice). BLUEPRINT_ID stays 0 for cosmetics (no rolled mods).
	//
	// Armor rows have the same shape as cosmetics (item_id > 0, device_id =
	// 0, no backing actor) but DO carry mods — so they need a non-zero
	// BLUEPRINT_ID to make the client's FUN_10a13820 mod-application path
	// apply the rolled effects. Discriminator: armor has mods, cosmetics
	// don't. Universal blueprint works because the client only nullity-
	// checks the resolved blueprint pointer — the rendered `[…]` letters
	// come from each effect's `prop.ui_code`, not from blueprint metadata.
	//
	// Slot 14 RestDevice is not cosmetic. R self-heal needs DEVICE_ID to
	// point at the live ATgDevice inventory id; sending 0 makes R no-op.
	//
	// Crafted consumables (Insta-Heal: item 6300 -> device 6268) are item rows
	// that DO get a live ATgDevice (the spawn equip loop takes every row with
	// device_id > 0, invId = inventory_id). They need DEVICE_ID = inventory_id
	// too, or the client never sets m_nDeviceInstanceId on the inv object,
	// never registers the device at equip point 13, and key 9 does nothing.
	for (const auto& d : devices) {
		const bool itemRow       = d.item_id > 0;
		const bool isArmor       = itemRow && !d.mods.empty();
		const int marshalItemId  = itemRow ? d.item_id : d.device_id;
		const int marshalDeviceField = (!itemRow || d.device_id > 0) ? d.inventory_id : 0;
		const int blueprintId    =
			isArmor                    ? PlayerSessionStore::GetBlueprintIdForStatItem(d.item_id) :
			(itemRow || d.mods.empty()) ? 0
			                            : PlayerSessionStore::GetBlueprintIdForDevice(d.device_id, d.oc);

		append(response, 0x0E, 0x00);   // 14 fields

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, marshalItemId);
		Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
		Write4B(response, GA_T::BLUEPRINT_ID, blueprintId);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, d.quality);
		Write4B(response, GA_T::DURABILITY, Durability::ForWire(d.inventory_id, blueprintId));
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, marshalDeviceField);

		// DATA_SET_INVENTORY_STATE: {INVENTORY_ID, EFFECT_GROUP_ID} pairs for
		// rolled mods only. Client FUN_10a13820 @ 0x10a13820 appends matching
		// rows into the inventory object's effect-group list, driving the
		// `[...]` suffix in the UI. Built-in equip / fire-mode effects are
		// NOT shipped here — they come from the client's own asm.dat (via
		// Device.m_EquipEffect / m_pFireModeSetup). Mixing them in leaks
		// built-in props into the suffix.
		std::vector<int> effectGroups = d.mods;
		if (effectGroups.empty()) effectGroups.push_back(0); // preserve 1-row shape

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, (uint8_t)(effectGroups.size() & 0xFF), (uint8_t)(effectGroups.size() >> 8));
		for (int egid : effectGroups) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, egid);
		}

		// DATA_SET_CHARACTER_PROFILES: one row per build profile (1..5) with
		// the same EQUIPPED_SLOT_VALUE_ID. Each row populates one of the five
		// `invObj+0x68..+0x78` slot-table entries on the client. The equip-
		// screen UI's FixupWidgets path requires EVERY profile entry to be
		// non-zero — if ANY profile has 0, the slot renders blank for all
		// profiles (see reference_equip_screen_is_valid_gate.md).
		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x05, 0x00);
		for (int profileId = 1; profileId <= 5; ++profileId) {
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
			Write4B(response, GA_T::PROFILE_ID, (uint32_t)profileId);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, d.slot_value_id);
		}
	}

	// --- Bag block. 13 fields per record (12 scalars + 1 nested data_set —
	// no DATA_SET_CHARACTER_PROFILES because bag items aren't bound to a
	// slot). LOCATION_VALUE_ID=370 (ON_HAND), DEVICE_ID=0, ACTIVE_FLAG=F.
	// See top-of-function comment for why DEVICE_ID=0 is the don't-resubmit
	// gate.
	for (const auto& row : bag) AppendBagRecord(response, row);

	// --- Component block. Same 13-field bag shape; INSTANCE_COUNT carries the
	// stack size (what the client sums for Collect progress) and the inventory
	// id is synthesised — components live in ga_user_components, not
	// ga_players_inventory, so they have no row id to borrow.
	for (const auto& comp : components) {
		AppendComponentRecord(response, comp);
	}

	send_response(response);
}

// One ON_HAND bag record (see the bag block in send_inventory_response for why
// DEVICE_ID=0 / LOCATION_VALUE_ID=370).
void TcpSession::AppendBagRecord(std::vector<uint8_t>& response, const InventoryRow& row, uint32_t instance_count) {
	const bool   rowIsItem     = row.item_id > 0;
	const bool   rowIsArmor    = rowIsItem && !row.mods.empty();
	const int    marshalItemId = rowIsItem ? row.item_id : row.device_id;
	const int    blueprintId   =
		rowIsArmor                    ? PlayerSessionStore::GetBlueprintIdForStatItem(row.item_id) :
		(rowIsItem || row.mods.empty()) ? 0
		                                : PlayerSessionStore::GetBlueprintIdForDevice(row.device_id, row.oc);

	append(response, 0x0D, 0x00);   // 13 fields

	Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
	Write4B(response, GA_T::ITEM_ID, marshalItemId);
	Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
	Write4B(response, GA_T::BLUEPRINT_ID, blueprintId);
	Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, row.quality);
	Write4B(response, GA_T::DURABILITY, Durability::ForWire(row.inventory_id, blueprintId));
	WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
	WriteString(response, GA_T::BOUND_FLAG, AgencyBank::BagBoundFlag(row.item_id));
	Write4B(response, GA_T::LOCATION_VALUE_ID, 370);   // ON_HAND
	Write4B(response, GA_T::INSTANCE_COUNT, instance_count);
	WriteString(response, GA_T::ACTIVE_FLAG, "F");
	Write4B(response, GA_T::DEVICE_ID, 0);

	std::vector<int> effectGroups = row.mods;
	if (effectGroups.empty()) effectGroups.push_back(0);

	append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
	append(response, (uint8_t)(effectGroups.size() & 0xFF), (uint8_t)(effectGroups.size() >> 8));
	for (int egid : effectGroups) {
		append(response, 0x02, 0x00);
		Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
		Write4B(response, GA_T::EFFECT_GROUP_ID, egid);
	}
}

// Vendor purchase -> push ONLY the new rows as INV_REPLICATION_STATE=1
// adds, then restate r_ItemCount (each new map entry must be counted or
// ATgInventoryManager::IsValid() fails). Same shape as credit_loot_table's
// component path; a clear+rebuild here left the bag list at 0/0 until relog.
void TcpSession::send_inventory_added(int nPawnId, int64_t character_id,
                                      const std::vector<int64_t>& inventory_ids) {
	if (nPawnId == 0 || character_id == 0 || inventory_ids.empty()) return;
	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) return;
	const std::set<int64_t> wanted(inventory_ids.begin(), inventory_ids.end());
	std::vector<InventoryRow> rows;
	for (const auto& row : PlayerSessionStore::GetInventoryForUser(charInfo->user_id, charInfo->profile_id))
		if (wanted.count(row.inventory_id)) rows.push_back(row);
	if (!rows.empty()) {
		std::vector<uint8_t> response;
		append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
		append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
		Write4B(response, GA_T::PAWN_ID, nPawnId);
		append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
		append(response, (uint8_t)(rows.size() & 0xFF), (uint8_t)(rows.size() >> 8));
		for (const auto& row : rows) AppendBagRecord(response, row);
		send_response(response);
	}
	Logger::Log("loot", "[Loot] vendor add: char=%lld pawnId=%d sent %zu of %zu new row(s)\n",
		(long long)character_id, nPawnId, rows.size(), inventory_ids.size());
	sync_item_count(character_id);
}

// Vendor sell / destroy -> INV_REPLICATION_STATE=2 records (the
// client's removal path, see send_component_removals), then restate r_ItemCount.
void TcpSession::send_inventory_removed(int nPawnId, int64_t character_id,
                                        const std::vector<int64_t>& inventory_ids) {
	if (nPawnId == 0 || character_id == 0 || inventory_ids.empty()) return;
	std::vector<uint8_t> response;
	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
	Write4B(response, GA_T::PAWN_ID, nPawnId);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(inventory_ids.size() & 0xFF), (uint8_t)(inventory_ids.size() >> 8));
	for (int64_t id : inventory_ids) {
		append(response, 0x02, 0x00);
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // delete
		Write4B(response, GA_T::INVENTORY_ID, (uint32_t)id);
	}
	send_response(response);
	Logger::Log("loot", "[Loot] inventory removed: char=%lld pawnId=%d %zu row(s)\n",
		(long long)character_id, nPawnId, inventory_ids.size());
	sync_item_count(character_id);
}

// One SEND_INVENTORY record for a component stack. Shared by the full push and
// the incremental drop update so the two can never drift apart.
void TcpSession::AppendComponentRecord(std::vector<uint8_t>& response,
                                       const Database::ComponentRow& comp) {
	const uint32_t synthetic_inv_id = kComponentInventoryIdBase + (uint32_t)comp.item_id;

	append(response, 0x0D, 0x00);   // 13 fields

	Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
	Write4B(response, GA_T::ITEM_ID, comp.item_id);
	Write4B(response, GA_T::INVENTORY_ID, synthetic_inv_id);
	Write4B(response, GA_T::BLUEPRINT_ID, 0);
	Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, comp.quality_value_id);
	Write4B(response, GA_T::DURABILITY, 100);
	WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
	WriteString(response, GA_T::BOUND_FLAG, AgencyBank::BagBoundFlag(comp.item_id));
	Write4B(response, GA_T::LOCATION_VALUE_ID, 370);   // ON_HAND
	Write4B(response, GA_T::INSTANCE_COUNT, (uint32_t)comp.quantity);
	WriteString(response, GA_T::ACTIVE_FLAG, "F");
	// DEVICE_ID 0 — components have no ATgDevice, so the client's recovery scan
	// must not try to bind one (same reason cosmetics send 0). It also keeps
	// them out of the resubmit path that re-resolves the in-hand weapon.
	Write4B(response, GA_T::DEVICE_ID, 0);

	append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
	append(response, 0x01, 0x00);
		append(response, 0x02, 0x00);
		Write4B(response, GA_T::INVENTORY_ID, synthetic_inv_id);
		Write4B(response, GA_T::EFFECT_GROUP_ID, 0);
}

void TcpSession::send_inventory_clear(int nPawnId, int64_t character_id) {
	// Resolve user + class profile so we hit the same pool send_inventory_response does.
	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) return;
	const auto bag = PlayerSessionStore::GetInventoryForUser(charInfo->user_id, charInfo->profile_id);
	if (bag.empty()) return;

	// One STATE=2 record per invId, all bundled in a single SEND_INVENTORY.
	// Per-record body is tiny (2 fields = 12 bytes) so the bundled payload
	// for a fully-stocked account fits well under any reasonable cap; the
	// chunked-marker framing in send_response handles overflow if it ever
	// matters.
	const size_t count = bag.size();

	std::vector<uint8_t> response;
	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
	Write4B(response, GA_T::PAWN_ID, nPawnId);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(count & 0xFF), (uint8_t)(count >> 8));
	for (const auto& row : bag) {
		append(response, 0x02, 0x00);  // 2 fields per record
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // delete
		Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
	}
	send_response(response);

	Logger::Log("tcp",
		"[TCP] send_inventory_clear: charId=%lld pawnId=%d wiped=%zu entries (STATE=2 delete pass)\n",
		character_id, nPawnId, count);
}

void TcpSession::send_loadout_inventory_response(int nPawnId, const std::vector<LoadoutItem>& items) {
    if (items.empty()) {
        Logger::Log("tcp", "[TCP] send_loadout_inventory_response: empty loadout for pawnId=%d\n", nPawnId);
        return;
    }
    Logger::Log("tcp",
        "[TCP] send_loadout_inventory_response: %zu items for pawnId=%d (bundled into one SEND_INVENTORY)\n",
        items.size(), nPawnId);

    // Same per-record wire format as send_inventory_response, minus the DB
    // lookup. BLUEPRINT_ID is 0 (no rolled mods on bot devices, so no [...]
    // suffix needed). DATA_SET_INVENTORY_STATE carries a single placeholder
    // row with EFFECT_GROUP_ID=0 to preserve the 1-row shape — harmless when
    // BLUEPRINT_ID=0 (parser skips mod application).
    std::vector<uint8_t> response;

    append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
    append(response, 0x02, 0x00);   // 2 top-level fields: PAWN_ID + DATA_SET

    Write4B(response, GA_T::PAWN_ID, nPawnId);

    append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
    append(response, (uint8_t)(items.size() & 0xFF), (uint8_t)(items.size() >> 8));

    for (const auto& d : items) {
        append(response, 0x0E, 0x00);   // 14 fields per record

        Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
        Write4B(response, GA_T::ITEM_ID, d.device_id);
        Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
        Write4B(response, GA_T::BLUEPRINT_ID, 0);
        Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, d.quality);
        Write4B(response, GA_T::DURABILITY, 100);
        WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
        WriteString(response, GA_T::BOUND_FLAG, "T");
        Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
        Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
        WriteString(response, GA_T::ACTIVE_FLAG, "T");
        Write4B(response, GA_T::DEVICE_ID, d.inventory_id);

        append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
        append(response, 0x01, 0x00);
        append(response, 0x02, 0x00);
        Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
        Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

        append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
        append(response, 0x01, 0x00);
        append(response, 0x04, 0x00);
        Write4B(response, GA_T::CHARACTER_ID, 0);
        Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
        Write4B(response, GA_T::PROFILE_ID, 0x1);
        Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, d.slot_value_id);
    }

    send_response(response);
}

void TcpSession::send_beacon_pickup_response(int nPawnId, int nDeviceId, int nInventoryId, int nEquipSlotValueId, int nItemProfileId) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_INVENTORY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);  // 1 item in DATA_SET

		append(response, 0x0E, 0x00);  // 14 fields per item

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, nDeviceId);
		Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
		Write4B(response, GA_T::BLUEPRINT_ID, 0);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, nInventoryId);

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

		// One row per profile (1..5) with the SAME EQUIPPED_SLOT_VALUE_ID —
		// the FixupWidgets gate (see send_inventory_response) requires all 5
		// non-zero or the slot widget renders blank. A picked-up beacon is
		// bound to the PAWN, not to a loadout profile — same SVID on every
		// profile is the desired pawn-scoped behavior anyway.
		(void)nItemProfileId;  // no longer needed — kept in signature for the IPC carry
		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x05, 0x00);
			for (int p = 1; p <= 5; ++p) {
				append(response, 0x04, 0x00);
				Write4B(response, GA_T::CHARACTER_ID, 0);
				Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
				Write4B(response, GA_T::PROFILE_ID, (uint32_t)p);
				Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, (uint32_t)nEquipSlotValueId);
			}

	send_response(response);
}

void TcpSession::send_beacon_remove_response(int nPawnId, int nInventoryId) {
	std::vector<uint8_t> response;

	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);  // 2 top-level items: PAWN_ID + DATA_SET

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);  // 1 item in DATA_SET
		append(response, 0x02, 0x00);  // 2 fields
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // 2 = deleted
		Write4B(response, GA_T::INVENTORY_ID, nInventoryId);

	send_response(response);
}

// Let the player's instance run the map's quest Kismet for a
// decided transition (QUEST_KISMET; see IpcProtocol.hpp for link numbers).
void TcpSession::send_quest_kismet(int quest_id, int link, int requirement_id) {
	if (assigned_instance_id_ == 0) return;
	nlohmann::json m;
	m["type"]           = IpcProtocol::MSG_QUEST_KISMET;
	m["quest_id"]       = quest_id;
	m["link"]           = link;
	m["requirement_id"] = requirement_id;
	IpcServer::SendToInstance(assigned_instance_id_, m.dump());
}

void TcpSession::send_quest_accept_response(int nQuestId) {
	send_quest_kismet(nQuestId, 0, 0);
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x02, 0x00);  // 2 fields: QUEST_ID + ACCEPT_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write1B(response, GA_T::ACCEPT_FLAG, 0x01);

	send_response(response);
}

// Shared turn-in tail (QUEST_UPDATE turn-in + no-turn-in auto finish).
void TcpSession::finish_quest(int64_t character_id, int quest_id, int pawn_id, int reward_item_id) {
	// Spend the Collect components BEFORE completing, so a
	// failure to complete can't hand out a free reward. Stacks
	// that hit zero are deleted and reported back here.
	const auto depleted =
		Database::ConsumeQuestRequirementItems(user_id_, quest_id);

	PlayerSessionStore::Quests().Complete(character_id, quest_id);
	send_quest_complete_response(quest_id);
	schedule_quest_tracker_refresh();

	// The quest's own XP + credit reward (was never
	// paid). DB first, then the instance restates the pawn's
	// replicated xp/credits/level so the HUD updates live.
	// p0366: an optional "quest_reward" service may scale the quest xp
	// (xp_percent 0..300, default 100); no reply = 100, never blocks.
	int xp_percent = 100;
	std::string bonus_label;
	if (ServiceHooks::Enabled("quest_reward")) {
		const int64_t base_xp = Database::LookupQuestBaseXp(quest_id);
		if (base_xp > 0) {
			auto d = ServiceHooks::Call("quest_reward", {
				{"quest_id", quest_id}, {"char_id", character_id},
				{"level", Database::CharacterLevel(character_id)}, {"xp", base_xp}});
			if (d) {
				xp_percent  = (int)ServiceHooks::ClampInt(*d, "xp_percent", 100, 0, 300);
				bonus_label = ServiceHooks::ClampString(*d, "label", 64);
			}
		}
	}
	const auto cash = Database::AwardQuestXpCredits(character_id, quest_id, xp_percent);
	if (cash.xp > 0 || cash.credits > 0) {
		Logger::Log("quest", "[TcpSession] quest %d reward char=%lld -> +%lld xp +%lld credits (xp %d%%, +%lld bonus)\n",
			quest_id, (long long)character_id, (long long)cash.xp, (long long)cash.credits,
			xp_percent, (long long)cash.bonus_xp);
		using MF = ChatSession::MsgField;
		if (cash.xp > 0) ChatSession::SystemMsgIdToGuid(session_guid_, 22229, {MF::U(GA_T::XP_VALUE, cash.xp)});
		if (cash.credits > 0) ChatSession::SystemMsgIdToGuid(session_guid_, 55329, {MF::U(GA_T::CURRENCY_VALUE, cash.credits)});
		level_lines_if_up();
		if (cash.bonus_xp > 0 && !session_guid_.empty())
			ChatSession::SystemMessageToGuid(session_guid_, "Includes +" + std::to_string(cash.bonus_xp) + " bonus XP" +
				(bonus_label.empty() ? std::string() : " - " + bonus_label) + ".");
		if (pawn_id != 0 && assigned_instance_id_ != 0) {
			nlohmann::json qr;
			qr["type"]         = IpcProtocol::MSG_QUEST_REWARD;
			qr["pawn_id"]      = pawn_id;
			qr["character_id"] = character_id;
			IpcServer::SendToInstance(assigned_instance_id_, qr.dump());
		}
	}

	if (pawn_id != 0) {
		// Depleted stacks leave the client's map entirely (no
		// "0 Units" ghost in the bag list); survivors just get a
		// lower INSTANCE_COUNT.
		send_component_removals(pawn_id, depleted);
		send_component_refresh(pawn_id);
	}

	// Reward table. Several quests are the ONLY source of a
	// component another quest needs (quest 30's table 285 ->
	// item 6295, required by quest 28), so the reward roll is
	// load-bearing — it feeds the next quest's Collect stock.
	credit_loot_table(Database::GetQuestRewardLootTableId(quest_id),
	                  character_id, pawn_id, "quest reward", reward_item_id);

	// Restate r_ItemCount unconditionally. A turn-in can shrink
	// the map (removals), grow it (a first-time reward
	// component), or both in one go, and credit_loot_table only
	// syncs when something actually dropped. Absolute value, so
	// a redundant call costs nothing.
	if (pawn_id != 0) {
		sync_item_count(character_id);
	}
}

// Quest follow-ups (ALL QUESTS item). Loops because a grant can be a
// quest that finishes at once (bounded: each pass completes/grants new rows).
void TcpSession::run_quest_followups(int64_t character_id, int pawn_id) {
	for (int guard = 0; guard < 8; guard++) {
		bool changed = false;
		for (int q : Database::PendingQuestGrants(character_id)) {
			PlayerSessionStore::Quests().Accept(character_id, q);
			send_quest_accept_response(q);
			Logger::Log("quest", "[TcpSession] quest %d granted to char=%lld (follow-up)\n", q, (long long)character_id);
			changed = true;
		}
		for (int q : Database::ActiveQuestsWithoutTurnIn(character_id)) {
			if (!Database::AreQuestRequirementsMet(user_id_, character_id, q, nullptr)) continue;
			Logger::Log("quest", "[TcpSession] quest %d auto turn-in for char=%lld (no turn-in volume)\n", q, (long long)character_id);
			finish_quest(character_id, q, pawn_id);
			changed = true;
		}
		if (!changed) break;
		schedule_quest_tracker_refresh();
	}
}

void TcpSession::send_quest_complete_response(int nQuestId) {
	send_quest_kismet(nQuestId, 2, 0);
	std::vector<uint8_t> response;

	// Current time as a non-zero 64-bit timestamp — triggers the complete branch in LoadQuests.
	uint64_t ts = static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		// Dailies also carry the UTC day they were done in.
		const uint32_t tf = Database::QuestCompletedTimeframe(nQuestId, (int64_t)ts);
		append(response, tf ? 0x04 : 0x03, 0x00);  // QUEST_ID + COMPLETED_DATETIME + COMPLETE_FLAG [+ timeframe]
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		if (tf) Write4B(response, GA_T::COMPLETED_QUEST_TIMEFRAME_ID, tf);
		// COMPLETED_DATETIME (0x00D8): 8-byte value — non-zero triggers completion branch
		append(response, GA_T::COMPLETED_DATETIME & 0xFF, GA_T::COMPLETED_DATETIME >> 8);
		for (int i = 0; i < 8; i++) append(response, static_cast<uint8_t>((ts >> (i * 8)) & 0xFF));
		Write1B(response, GA_T::COMPLETE_FLAG, 0x01);

	send_response(response);
}

void TcpSession::send_quest_abandon_response(int nQuestId) {
	send_quest_kismet(nQuestId, 3, 0);
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x02, 0x00);  // 2 fields: QUEST_ID + ABANDON_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write1B(response, GA_T::ABANDON_FLAG, 0x01);

	send_response(response);
}

// Push the authoritative r_ItemCount to the DLL. Absolute, never a delta —
// see the call site in credit_loot_table.
//
// user + class profile come from the CHARACTER ROW, never from session state.
// send_inventory_response builds its record set from exactly this pair
// (charInfo->user_id / charInfo->profile_id) and the DLL's three stamp sites
// read the same pair out of ga_characters — so the count and the records it has
// to match are the same expression over the same inputs, by construction.
//
// This used to read selected_profile_id_, which is set during character
// create/select and is zero or stale on any session that reached gameplay
// another way. A wrong profile here does not drift the count by one: the
// `(profile_id = 0 OR profile_id = ?)` pool term recomputes the WHOLE total,
// so the first component drop of the session — the first time this function
// ever runs — replaces a correct r_ItemCount with a badly wrong one and blanks
// the inventory instantly.
void TcpSession::sync_item_count(int64_t character_id) {
	if (session_guid_.empty() || character_id == 0) return;

	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) {
		Logger::Log("loot",
			"[Loot] sync_item_count char=%lld: no character row — count NOT pushed\n",
			(long long)character_id);
		return;
	}

	const int total = Database::GetExpectedItemCount(charInfo->user_id,
	                                                 (int)charInfo->profile_id);
	if (total <= 0) return;

	nlohmann::json msg;
	msg["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
	msg["session_guid"] = session_guid_;
	msg["action"]       = "set_item_count";
	msg["args"]         = {{"total", total}};
	TcpSession::DeliverPlayerAction(session_guid_, msg);

	Logger::Log("loot",
		"[Loot] sync_item_count guid=%s char=%lld user=%lld profile=%d -> %d\n",
		session_guid_.c_str(), (long long)character_id,
		(long long)charInfo->user_id, (int)charInfo->profile_id, total);
}

// Re-send every component stack at its current quantity. Used after turn-in
// spends components. Updates existing entries only — a stack that hit zero is
// gone from the DB and goes out through send_component_removals instead.
// NOTIFY_SALVAGED_ITEMS 0x01E9 fills the bag's salvage result
// panel (client TgUIAgentProfile_Inventory subscribes to it, handler 0x1141cbb0):
// DATA_SET_INVENTORY_CHANGE rows = CHANGE_TYPE 1 (add) + an inventory record
// (same fields as the component push) + CHANGE_AMT. INSTANCE_COUNT carries the
// amount gained so the panel shows "+N", the bag keeps the full stack.
void TcpSession::send_salvaged_items(const std::map<int, int>& gained) {
	if (gained.empty() || user_id_ == 0) return;
	std::map<int, int> quality;
	for (const auto& c : Database::GetAllComponents(user_id_)) quality[c.item_id] = c.quality_value_id;

	std::vector<uint8_t> response;
	append(response, GA_U::NOTIFY_SALVAGED_ITEMS & 0xFF, GA_U::NOTIFY_SALVAGED_ITEMS >> 8);
	append(response, 0x01, 0x00);
	append(response, GA_T::DATA_SET_INVENTORY_CHANGE & 0xFF, GA_T::DATA_SET_INVENTORY_CHANGE >> 8);
	append(response, (uint8_t)(gained.size() & 0xFF), (uint8_t)(gained.size() >> 8));
	for (const auto& kv : gained) {
		Database::ComponentRow row;
		row.item_id = kv.first;
		row.quantity = kv.second;
		row.quality_value_id = quality.count(kv.first) ? quality[kv.first] : 0;
		std::vector<uint8_t> rec;
		AppendComponentRecord(rec, row);
		rec[0] = (uint8_t)(rec[0] + 2);   // + CHANGE_TYPE + CHANGE_AMT
		response.insert(response.end(), rec.begin(), rec.end());
		Write4B(response, GA_T::CHANGE_TYPE, 1);
		Write4B(response, GA_T::CHANGE_AMT, (uint32_t)kv.second);
	}
	send_response(response);
}

void TcpSession::send_component_refresh(int nPawnId) {
	if (nPawnId == 0 || user_id_ == 0) return;

	const auto components = Database::GetAllComponents(user_id_);
	if (components.empty()) return;

	std::vector<uint8_t> response;
	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
	Write4B(response, GA_T::PAWN_ID, nPawnId);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(components.size() & 0xFF), (uint8_t)(components.size() >> 8));
	for (const auto& c : components) AppendComponentRecord(response, c);
	send_response(response);
}

// Drop depleted component stacks from the client's inventory map.
//
// INV_REPLICATION_STATE=2 is the client's real removal path: CGameClient::
// SendInventory routes it to FUN_10a16190 → FUN_10a160e0, which erases the
// m_InventoryMap entry. Same record shape send_inventory_clear uses.
//
// The map shrinks by one per record, so the caller MUST restate r_ItemCount
// afterwards or IsValid() starts failing (blank equip screen).
void TcpSession::send_component_removals(int nPawnId, const std::vector<int>& item_ids) {
	if (nPawnId == 0 || item_ids.empty()) return;

	std::vector<uint8_t> response;
	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
	Write4B(response, GA_T::PAWN_ID, nPawnId);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(item_ids.size() & 0xFF), (uint8_t)(item_ids.size() >> 8));
	for (int item_id : item_ids) {
		append(response, 0x02, 0x00);  // 2 fields per record
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // delete
		Write4B(response, GA_T::INVENTORY_ID,
			kComponentInventoryIdBase + (uint32_t)item_id);
	}
	send_response(response);

	Logger::Log("loot", "[Loot] component removals: pawnId=%d dropped %zu depleted stack(s)\n",
		nPawnId, item_ids.size());
}

// Roll a loot table and hand the player whatever components drop.
//
// Three things have to happen together, and all three were needed before a
// Collect quest would actually track:
//
//   1. persist the stock  (ga_user_components — per USER, shared by the
//      account's characters),
//   2. push a SEND_INVENTORY record so the component exists in the CLIENT's
//      inventory map. The client computes Collect (1429) progress itself, by
//      summing nInstanceCount over that map (FUN_10a16310 via FUN_109ac070) —
//      it ignores the COUNT in a quest progress packet for this type. No
//      record, no progress, ever.
//   3. grow r_ItemCount when the drop creates a NEW map entry, or
//      ATgInventoryManager::IsValid() fails and the equip screen goes blank.
//      A drop that only grows an existing stack must NOT bump — the entry is
//      already there and only nInstanceCount changes.
//
// No quest progress packet is sent for a component: the client notices the
// inventory record on its own and raises the "N of M" notice itself. Pushing one
// too gave two toasts per drop.
//
// Components no quest wants are still granted — they are real stock, and a later
// quest may ask for them. Non-component drops are rolled and discarded inside
// RollLootTableComponents so the odds stay faithful to the table.
// 21310 Congratulations + sound 5223, and 25117 "more skill point(s)", which the client
// turns into its "You have unused skill points." alert (59445, 0x113ec712). Compared with
// the level last seen so every path that pays XP can call it, server or player hosted.
void TcpSession::level_lines_if_up() {
	const int64_t cid = selected_character_id_;
	if (cid == 0) return;
	const int level = (int)Database::CharacterLevel(cid);
	const bool known = level_seen_char_ == cid;
	const int was = level_seen_;
	level_seen_char_ = cid;
	level_seen_ = level;
	if (!known || level <= was) return;
	using MF = ChatSession::MsgField;
	ChatSession::SystemMsgIdToGuid(session_guid_, 21310, {MF::U(GA_T::LEVEL, level)});
	int points = 0;
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* st = nullptr;
	if (db && sqlite3_prepare_v2(db, "SELECT COALESCE(SUM(skill_points),0) FROM asm_data_set_xp_levels WHERE level > ?1 AND level <= ?2",
			-1, &st, nullptr) == SQLITE_OK && st) {
		sqlite3_bind_int(st, 1, was);
		sqlite3_bind_int(st, 2, level);
		if (sqlite3_step(st) == SQLITE_ROW) points = sqlite3_column_int(st, 0);
	}
	sqlite3_finalize(st);
	if (points > 0) ChatSession::SystemMsgIdToGuid(session_guid_, 25117, {MF::U(GA_T::SKILL_POINTS, points)});
}

void TcpSession::LevelLinesForCharacter(int64_t character_id) {
	std::shared_ptr<TcpSession> s;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& kv : g_sessions_) {
			auto c = kv.second.lock();
			if (c && character_id != 0 && c->selected_character_id_ == character_id) { s = c; break; }
		}
	}
	if (s) s->level_lines_if_up();
}

// 32849 Received item / 67235 Received item reward: chat line (+ sound 5221 on the former)
// and the client's ACQUIRED banner.
void TcpSession::received_item_line(int item_id, int count, const char* reason) {
	using MF = ChatSession::MsgField;
	const std::string name = MissionLoot::ItemName(item_id);
	if (name.empty()) return;
	const bool reward = reason != nullptr && std::strcmp(reason, "quest reward") == 0;
	ChatSession::SystemMsgIdToGuid(session_guid_, reward ? 67235 : 32849, {MF::U(GA_T::ITEM_ID, item_id),
		MF::S(GA_T::ITEM_NAME, name), MF::U(GA_T::INSTANCE_COUNT, count)});
}

void TcpSession::credit_loot_table(int loot_table_id, int64_t character_id, int pawn_id,
                                   const char* reason, int chosen_item_id) {
	if (loot_table_id <= 0 || character_id == 0) return;

	// Stock is credited to the character's OWNING account, resolved from the
	// character row — the same source sync_item_count and send_inventory_response
	// use. Session state is not consulted anywhere on this path.
	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) return;
	const int64_t owner_user_id = charInfo->user_id;
	if (owner_user_id == 0) return;

	std::map<int, int> bagDrops;
	std::vector<Database::AutoGear> autoGear;  // 0957
	const auto dropped = Database::RollLootTable(loot_table_id, bagDrops, chosen_item_id, &autoGear,
	                                            // 0959: quest rewards give the agent's class row only
	                                            std::strcmp(reason, "quest reward") == 0 ? charInfo->profile_id : 0u);
	if (dropped.empty() && bagDrops.empty()) return;

	// Roll first, but do NOT persist until we know the client can be told. A
	// grant the client never hears about leaves the DB ahead of its map, and the
	// next r_ItemCount stamp then counts a row that has no map entry — blank
	// equip screen with no event to trace it back to.
	if (pawn_id == 0) {
		Logger::Log("loot",
			"[Loot] %s table=%d char=%lld: %zu component(s) rolled but no pawn — "
			"drop DISCARDED (not persisted) so the DB can't lead the client\n",
			reason, loot_table_id, (long long)character_id, dropped.size() + bagDrops.size());
		return;
	}

	// Blueprint / flair drops -> bag rows, pushed as incremental
	// adds (send_inventory_added restates r_ItemCount itself).
	if (!bagDrops.empty()) {
		const auto ids = Database::GrantLootItems(owner_user_id, bagDrops);
		// 0957: auto-created gear gets the reward's quality + stats BEFORE the client is told
		EconParity::ApplyAutoGear(ids, autoGear);
		for (const auto& kv : bagDrops)
			Logger::Log("loot", "[Loot] %s table=%d char=%lld -> bag item %d x%d\n",
				reason, loot_table_id, (long long)character_id, kv.first, kv.second);
		if (!ids.empty()) send_inventory_added(pawn_id, character_id, ids);
		if (!ids.empty()) for (const auto& kv : bagDrops) received_item_line(kv.first, kv.second, reason);
	}

	if (dropped.empty()) return;
	const auto granted = Database::GrantComponents(owner_user_id, dropped);
	if (granted.empty()) return;

	// 2 — one incremental SEND_INVENTORY for the changed stacks. Never a
	// clear+rebuild: that re-resolves the in-hand device and drops the player
	// to melee mid-combat.
	{
		std::vector<uint8_t> response;
		append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
		append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
		Write4B(response, GA_T::PAWN_ID, pawn_id);
		append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
		append(response, (uint8_t)(granted.size() & 0xFF), (uint8_t)(granted.size() >> 8));
		for (const auto& g : granted) {
			AppendComponentRecord(response,
				Database::ComponentRow{ g.item_id, g.new_total, g.quality_value_id });
		}
		send_response(response);
	}

	// 3 — restate r_ItemCount as an ABSOLUTE total rather than nudging it by a
	// delta. A delta only has to be missed or double-applied once (a drop with
	// no pawn, a dropped IPC, two grants in one frame) for the count to drift
	// permanently and blank the equip screen. The total is recomputed from the
	// same expression the DLL's spawn / re-equip stamps use, so every send is
	// self-correcting.
	sync_item_count(character_id);
	for (const auto& kv : dropped) received_item_line(kv.first, kv.second, reason);

	// No quest progress packet here. The client raises its own "N of M" notice
	// off the inventory record above — pushing one as well produced two toasts
	// per drop. Kill (1428) credit still pushes its own, because nothing on the
	// client observes a kill.
	Logger::Log("loot",
		"[Loot] %s table=%d char=%lld -> %zu component stack(s) updated\n",
		reason, loot_table_id, (long long)character_id, granted.size());
}

// -components grant/take. Deliberately reuses the production helpers rather
// than reimplementing the wire format, so what this exercises is exactly what a
// drop / turn-in does.
std::string TcpSession::DeliverComponentDebug(const std::string& session_guid,
                                              int op_take, int item_id, int count) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it != g_sessions_.end()) {
            session = it->second.lock();
            if (!session) g_sessions_.erase(it);
        }
    }
    if (!session) return "components: no active game session";

    const int64_t character_id = session->selected_character_id_;
    const int     pawn_id      = session->last_pawn_id_;
    if (character_id == 0) return "components: no character selected";
    if (pawn_id == 0)      return "components: no spawned pawn yet";

    const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
    if (!charInfo) return "components: no character row";
    const int64_t user_id = charInfo->user_id;

    const int before = Database::GetComponentCount(user_id, item_id);

    if (!op_take) {
        const auto granted = Database::GrantComponents(user_id, {{item_id, count}});
        if (granted.empty()) return "components: grant failed";

        std::vector<uint8_t> response;
        session->append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
        session->append(response, 0x02, 0x00);
        session->Write4B(response, GA_T::PAWN_ID, pawn_id);
        session->append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
        session->append(response, 0x01, 0x00);
        session->AppendComponentRecord(response,
            Database::ComponentRow{ granted[0].item_id, granted[0].new_total,
                                    granted[0].quality_value_id });
        session->send_response(response);
        session->sync_item_count(character_id);

        char msg[192];
        snprintf(msg, sizeof(msg),
            "components: item %d %d -> %d (%s map entry)",
            item_id, before, granted[0].new_total,
            granted[0].new_row ? "NEW" : "existing");
        return msg;
    }

    // take
    if (before <= 0) return "components: you hold none of that item";
    const int removed = Database::TakeComponents(user_id, item_id, count);
    const int after   = Database::GetComponentCount(user_id, item_id);

    if (after <= 0) {
        // Stack emptied -> the entry must leave the client's map. This is the
        // exact path a turn-in takes, and the one the desync tracks.
        session->send_component_removals(pawn_id, { item_id });
    } else {
        session->send_component_refresh(pawn_id);
    }
    session->sync_item_count(character_id);

    char msg[192];
    snprintf(msg, sizeof(msg),
        "components: item %d %d -> %d (took %d, %s)",
        item_id, before, after, removed,
        after <= 0 ? "STATE=2 removal sent" : "count updated");
    return msg;
}

// Replay the character's persisted quest state to a freshly-connected client.
//
// Nothing else does this: the control server never answers GET_ALL_QUESTS and
// friends (the client reads the quest CATALOG from its own asm.dat), so without
// this push the client's tracker starts empty every login and quest givers
// re-offer quests the character already finished.
//
// Read off LoadQuests @ 0x109ad570:
//   - A quest with a NON-ZERO COMPLETED_DATETIME goes into the completed map;
//     that map is what stops a giver re-offering it.
//   - A quest with COMPLETED_DATETIME absent/zero gets an entry in the accepted
//     map — mere presence marks it accepted, ACCEPT_FLAG only drives the toast.
//   - ACCEPT_FLAG / COMPLETE_FLAG / ABANDON_FLAG are what trigger the
//     accept/complete popups and the outbound echo marshal at the end of the
//     handler, so a silent state replay must send NONE of them.
//   - Requirement counters replay through the nested DATA_SET_QUEST_REQUIREMENTS
//     array; PROGRESS is omitted so the counts are stored without firing the
//     "N of M" toast for every requirement at login.
//
// ONE PACKET PER QUEST, deliberately: LoadQuests initialises its
// COMPLETED_DATETIME / timeframe locals ONCE before the entry loop, not per
// entry. If a completed quest and an active quest shared a packet and the
// active entry simply omitted COMPLETED_DATETIME, the stale value from the
// previous entry could still be live and the active quest would be read as
// completed. Separate packets make that impossible regardless of how the
// marshal getters behave on a missing tag.
// Client UPDATE_QUEST_TIMEFRAMES handler (GlobalAgenda.exe 0x109102a0) stores
// DAILY_TIMEFRAME_ID / WEEKLY_TIMEFRAME_ID and, only when one of them CHANGED,
// calls the quest manager's tracker refresh (0x109abaf0 -> 0x113aa630(0,0) ->
// 0x113a2c10 -> TgUIPrimaryHUD_QuestTracking update 0x114e4c50): tracked quest
// -> its name + requirements; none tracked -> "No tracked assignment." (msg
// 59302) where the client wants it, else the panel is hidden. The ids live for
// the whole client process, so a sentinel goes first to force a change; the
// second packet leaves the real UTC day/week numbers in place.
void TcpSession::send_quest_timeframes_refresh() {
	const uint32_t days = static_cast<uint32_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count() / 86400);
	const uint32_t daily  = days + 1;
	const uint32_t weekly = days / 7 + 1;
	for (uint32_t d : { 0xFFFFFFFFu, daily }) {
		std::vector<uint8_t> response;
		append(response, GA_U::UPDATE_QUEST_TIMEFRAMES & 0xFF, GA_U::UPDATE_QUEST_TIMEFRAMES >> 8);
		append(response, 0x02, 0x00);  // 2 fields
		Write4B(response, GA_T::DAILY_TIMEFRAME_ID, d);
		Write4B(response, GA_T::WEEKLY_TIMEFRAME_ID, weekly);
		send_response(response);
	}
	Logger::Log("quest", "[TCP] quest timeframes refresh: char=%lld daily=%u weekly=%u\n",
		(long long)selected_character_id_, daily, weekly);
}

// The spawn-time refresh can land before the HUD's tracker panel exists or
// while the PC refresh debounce runs (0x113a2c10 skips then), leaving the upk
// placeholder "Quest Name" up (intro). Repeat it a few seconds later;
// it is a pure tracker redraw on the client, so extra copies are harmless.
void TcpSession::schedule_quest_tracker_refresh() {
	auto s = shared_from_this();
	for (int delay : {4, 15}) {
		auto t = std::make_shared<asio::steady_timer>(io_ctx_, std::chrono::seconds(delay));
		t->async_wait([s, t](const asio::error_code& ec) {
			if (!ec && s->selected_character_id_ != 0) s->send_quest_timeframes_refresh();
		});
	}
}

// The HUD's tracker panel only drops its placeholder ("Quest Name") once the
// client's quest manager is flagged loaded: a 0.1 s looping PC timer polls that
// flag (set only by LoadQuests 0x109ad570 = any QUEST_UPDATE) and, until it is
// set, every tracker refresh (0x113a2c10) is skipped. A fresh agent (intro) has
// no quests, so nothing ever set it; the flag is also per map's HUD lifetime in
// practice, so this goes out on every spawn. Zero entries = no other effect.
void TcpSession::send_quest_list_loaded() {
	std::vector<uint8_t> response;
	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS
	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x00, 0x00);  // 0 quest entries
	send_response(response);
	Logger::Log("quest", "[TCP] quest list loaded marker: char=%lld\n", (long long)selected_character_id_);
}

void TcpSession::send_quest_state_sync() {
	if (selected_character_id_ == 0) return;

	const auto states = Database::GetCharacterQuestStates(selected_character_id_);
	if (states.empty()) {
		Logger::Log("quest", "[TCP] quest state sync: char=%lld has no quests\n",
			(long long)selected_character_id_);
		send_quest_list_loaded();
		return;
	}

	int completed = 0, active = 0;

	for (const auto& s : states) {
		std::vector<uint8_t> response;
		append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
		append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

		append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
		append(response, 0x01, 0x00);  // 1 quest entry

		if (s.status == "complete") {
			// COMPLETED_DATETIME must be non-zero — it is the discriminator.
			// Fall back to "now" for legacy rows with a null completed_at,
			// otherwise the client reads the quest as still active.
			uint64_t ts = (uint64_t)s.completed_at;
			if (ts == 0) {
				ts = static_cast<uint64_t>(
					std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch()).count());
			}

			const uint32_t tf = Database::QuestCompletedTimeframe(s.quest_id, (int64_t)ts);
			append(response, tf ? 0x03 : 0x02, 0x00);  // QUEST_ID + COMPLETED_DATETIME [+ timeframe]
			Write4B(response, GA_T::QUEST_ID, s.quest_id);
			if (tf) Write4B(response, GA_T::COMPLETED_QUEST_TIMEFRAME_ID, tf);
			append(response, GA_T::COMPLETED_DATETIME & 0xFF, GA_T::COMPLETED_DATETIME >> 8);
			for (int i = 0; i < 8; i++) append(response, static_cast<uint8_t>((ts >> (i * 8)) & 0xFF));
			completed++;
		} else {
			const auto counts = Database::GetQuestRequirementCounts(
				user_id_, selected_character_id_, s.quest_id);

			if (counts.empty()) {
				append(response, 0x01, 0x00);  // QUEST_ID only
				Write4B(response, GA_T::QUEST_ID, s.quest_id);
			} else {
				append(response, 0x02, 0x00);  // QUEST_ID + requirement array
				Write4B(response, GA_T::QUEST_ID, s.quest_id);

				append(response, GA_T::DATA_SET_QUEST_REQUIREMENTS & 0xFF,
				                 GA_T::DATA_SET_QUEST_REQUIREMENTS >> 8);
				append(response, (uint8_t)(counts.size() & 0xFF), (uint8_t)(counts.size() >> 8));
				for (const auto& c : counts) {
					append(response, 0x02, 0x00);  // 2 fields
					Write4B(response, GA_T::QUEST_REQUIREMENT_ID, c.first);
					Write4B(response, GA_T::COUNT, c.second);
				}
			}
			active++;
		}

		send_response(response);
	}

	Logger::Log("quest",
		"[TCP] quest state sync: char=%lld replayed %d completed + %d active quest(s)\n",
		(long long)selected_character_id_, completed, active);

	// Follow-ups due from before they existed (e.g. 107 done before 137 was granted).
	run_quest_followups(selected_character_id_, last_pawn_id_);
}

// Per-requirement progress update. Shape taken from the client's QUEST_UPDATE
// handler (LoadQuests @ 0x109ad570):
//   - QUEST_ID identifies the tracked quest (created on demand if missing).
//   - QUEST_REQUIREMENT_ID + COUNT store the ABSOLUTE new count (the handler
//     assigns, it does not accumulate).
//   - PROGRESS is presence-gated: without it the count is stored silently and
//     neither the on-screen "N of M" nor the quest-log entry refreshes.
//   - COMPLETED_DATETIME must be ABSENT — any non-zero value routes the entry
//     into the quest-finished branch instead of the progress branch.
// The required total ("of 10") already lives on the client from the static
// GET_QUEST_REQUIREMENTS catalog, so only the current count travels here.
void TcpSession::send_quest_requirement_progress_response(int nQuestId, int nRequirementId,
                                                          int nCount) {
	send_quest_kismet(nQuestId, 0, nRequirementId);
	if (Database::AreQuestRequirementsMet(user_id_, selected_character_id_, nQuestId, nullptr))
		send_quest_kismet(nQuestId, 1, 0);
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x04, 0x00);  // 4 fields
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write4B(response, GA_T::QUEST_REQUIREMENT_ID, nRequirementId);
		Write4B(response, GA_T::COUNT, nCount);
		Write4B(response, GA_T::PROGRESS, nCount);

	send_response(response);
}

// 0x176 {DATA_SET_PLAYER_ACHIEVEMENTS 0x17F rows: ACHIEVEMENT_ID,
// ACHIEVED_DATETIME, MAX_NUMBER}. Row present = earned (handler 0x10915060).
void TcpSession::send_player_achievements_response() {
	const auto earned = AchievementService::GetEarned(selected_character_id_);
	std::vector<uint8_t> response;
	append(response, GA_U::SEND_PLAYER_ACHIEVEMENTS & 0xFF, GA_U::SEND_PLAYER_ACHIEVEMENTS >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: the data set
	append(response, GA_T::DATA_SET_PLAYER_ACHIEVEMENTS & 0xFF, GA_T::DATA_SET_PLAYER_ACHIEVEMENTS >> 8);
	append(response, earned.size() & 0xFF, (earned.size() >> 8) & 0xFF);
	uint32_t order = 0;
	for (const auto& e : earned) {
		append(response, 0x03, 0x00);  // 3 fields
		Write4B(response, GA_T::ACHIEVEMENT_ID, (uint32_t)e.achievement_id);
		WriteDouble(response, GA_T::ACHIEVED_DATETIME, (double)e.achieved_at);
		Write4B(response, GA_T::MAX_NUMBER, ++order);
	}
	send_response(response);
}

// 0x177 {ACHIEVEMENT_ID} -> "achievement earned" popup + chat line.
void TcpSession::send_achievement_notice(int nAchievementId) {
	std::vector<uint8_t> response;
	append(response, GA_U::PUSH_PLAYER_ACHIEVEMENT_NOTICE & 0xFF, GA_U::PUSH_PLAYER_ACHIEVEMENT_NOTICE >> 8);
	append(response, 0x01, 0x00);
	Write4B(response, GA_T::ACHIEVEMENT_ID, (uint32_t)nAchievementId);
	send_response(response);
}

void TcpSession::send_get_loot_table_items_by_id_filtered_response(int nLootTableId)
{
	// Vendor stock straight from the game's own tables: the loot table a vendor
	// volume points at (asm_data_set_ui_volumes.loot_table_id) and the item
	// prices in credits/tokens. Unknown/empty table -> empty lists.
	// A rotating vendor's empty LIVE table -> this week's set
	// (Shared/VendorRotation.hpp). LOOT_TABLE_ITEM_IDs stay the set's own; the
	// purchase names the LIVE table and is resolved the same way.
	const int nStockTableId = VendorRotation::Resolve(nLootTableId);
	std::vector<Database::VendorItem> items = Database::GetVendorItems(nStockTableId);
	// Agenda Points (1605) prices are served too: AP is earned in play
	// and the instance's VendorPurchase debits ga_users.agenda_points.
	uint16_t nPrices = 0;
	for (const auto& v : items) nPrices += (uint16_t)v.prices.size();

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Format read by the client's vendor scene (GlobalAgenda.exe 0x114b2ff0,
	// statically decoded): DATA_SET (0x010C) rows LOOT_TABLE_ITEM_ID, ITEM_ID,
	// CREATED_ITEM_ID, AUTO_CREATE_FLAG, MAX_QUALITY_VALUE_ID, SORT_ORDER
	// (+ optional ITEM_SPECIAL_TYPE_VALUE_ID), then a top-level DATA_SET_PRICES
	// with LOOT_TABLE_ITEM_ID, CURRENT_PRICE, PRICE, CURRENCY_TYPE_VALUE_ID.
	// AUTO_CREATE_FLAG is a sized 'y'/'n' string, not a UINT32 - sending it as
	// 4 bytes desynced the client's parse and left the vendor list empty.
	uint16_t nItems = (uint16_t)items.size();
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, nItems & 0xFF, nItems >> 8);
	for (const auto& v : items) {
		append(response, 0x06, 0x00);  // inner item count
		Write4B(response, GA_T::LOOT_TABLE_ITEM_ID, v.loot_table_item_id);
		Write4B(response, GA_T::ITEM_ID, v.item_id);
		Write4B(response, GA_T::CREATED_ITEM_ID, v.item_id);
		WriteNBytes(response, GA_T::AUTO_CREATE_FLAG, {0x01, 0x00, (uint8_t)(v.auto_create_flag ? 0x79 : 0x6e)});
		Write4B(response, GA_T::MAX_QUALITY_VALUE_ID, v.max_quality_value_id ? v.max_quality_value_id : 1165);
		Write4B(response, GA_T::SORT_ORDER, v.sort_order);
	}

	append(response, GA_T::DATA_SET_PRICES & 0xFF, GA_T::DATA_SET_PRICES >> 8);
	append(response, nPrices & 0xFF, nPrices >> 8);
	for (const auto& v : items) {
		for (const auto& p : v.prices) {
			append(response, 0x04, 0x00);  // inner item count
			Write4B(response, GA_T::LOOT_TABLE_ITEM_ID, v.loot_table_item_id);
			Write4B(response, GA_T::CURRENT_PRICE, p.price);
			Write4B(response, GA_T::PRICE, p.price);
			Write4B(response, GA_T::CURRENCY_TYPE_VALUE_ID, p.currency_type_value_id);
		}
	}

	Logger::Log("tcp", "[Vendor] loot table %d (stock %d) -> %d items, %d prices\n", nLootTableId, nStockTableId, (int)nItems, (int)nPrices);
	send_response(response);
}

// 0760: text of a billboard volume (client: BILBOARD_CONTENT handler, TgOmegaVolume key). billboard-<key>.txt in the server
// directory, written by a separate news service; no file = no reply, as before. The client's text buffer holds 0x2000 characters.
void TcpSession::send_billboard_content(int key)
{
	if (key <= 0) return;
	std::ifstream in("billboard-" + std::to_string(key) + ".txt");
	std::string text, line;
	while (std::getline(in, line) && text.size() + line.size() + 2 < 0x1000) text += line + "\r\n";
	Logger::Log("tcp", "[TCP] BILBOARD_CONTENT key=%d bytes=%d\n", key, (int)text.size());
	if (text.empty()) return;
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::BILBOARD_CONTENT, item_count = 2;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	Write4B(response, GA_T::BILBOARD_ID, (uint32_t)key);
	append(response, GA_T::DATA_SET_BILBOARD & 0xFF, GA_T::DATA_SET_BILBOARD >> 8);
	append(response, 1, 0);
	append(response, 1, 0);
	WriteString(response, GA_T::MESSAGE, text);
	send_response(response);
}

void TcpSession::send_player_skills_response()
{
	std::vector<uint8_t> response;

	// Authoritative skill allocation for the currently-selected character.
	// Ship EVERY profile's rows (each tagged with its own profile_id) so the
	// client's skill-tree widget can re-filter against r_nItemProfileId after
	// a profile switch without needing a fresh SEND_PLAYER_SKILLS. r_nItemProfileId
	// has no UC repnotify, so on switch the widget receives no callback —
	// previously the active-only ship meant the widget saw 0 rows for the new
	// profile until a close+reopen rebuilt its bindings.
	std::vector<SkillRow> skills;
	double respecUnix = 1.0;
	if (selected_character_id_ != 0) {
		skills = PlayerSessionStore::GetAllSkillsForCharacter(selected_character_id_);
		int64_t t = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
		if (t > 0) respecUnix = (double)t;
	}

	uint16_t packet_type = GA_U::SEND_PLAYER_SKILLS;
	// Top-level fields: LAST_RESPEC_DATETIME + DATA_SET_CHARACTER_SKILLS.
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// LAST_RESPEC_DATETIME is encoded as DATETIME (type 7 = 8 bytes / double).
	// The client parses it via CMarshal::get_datetime_2 — a 4-byte write here
	// would leak garbage into the next field.
	WriteDouble(response, GA_T::LAST_RESPEC_DATETIME, respecUnix);

	uint16_t count = (uint16_t)skills.size();
	append(response, GA_T::DATA_SET_CHARACTER_SKILLS & 0xFF, GA_T::DATA_SET_CHARACTER_SKILLS >> 8);
	append(response, count & 0xFF, count >> 8);
	for (const auto& s : skills) {
		// Client's receive-side lookup (FUN_1141f750) matches rows on THREE
		// keys: SKILL_GROUP_ID, SKILL_ID, and PROFILE_ID — matched against
		// character+0x1634 = r_nItemProfileId (NOT the class profile id).
		// Each row carries its OWN item_profile_id from the DB so all five
		// loadouts ship in one packet; the widget filters by active profile.
		append(response, 0x04, 0x00);
		Write4B(response, GA_T::SKILL_GROUP_ID,   (uint32_t)s.skill_group_id);
		Write4B(response, GA_T::SKILL_ID,         (uint32_t)s.skill_id);
		Write4B(response, GA_T::PROFILE_ID,       (uint32_t)s.item_profile_id);
		Write4B(response, GA_T::POINTS_ALLOCATED, (uint32_t)s.points);
	}

	Logger::Log("tcp", "[TCP] SEND_PLAYER_SKILLS: sent %d skills (all profiles) for charId=%lld active=%d\n",
		(int)skills.size(), (long long)selected_character_id_, (int)item_profile_id_);

	send_response(response);
}

// Rank helpers. Ranks are ordered rank_level ASC; level 0 = Leader, who always
// passes every permission check regardless of the stored bits.
namespace {
int agency_rank_level(const Database::AgencyInfo& info, int rank_id) {
	for (const auto& rk : info.ranks) if (rk.rank_id == rank_id) return rk.rank_level;
	return 9999;
}

const Database::AgencyMemberRow* agency_member_by_user(const Database::AgencyInfo& info,
                                                       int64_t user_id) {
	for (const auto& m : info.members) if (m.user_id == user_id) return &m;
	return nullptr;
}

bool agency_has_perm(const Database::AgencyInfo& info, int64_t user_id, uint32_t bit) {
	const auto* me = agency_member_by_user(info, user_id);
	if (!me) return false;
	for (const auto& rk : info.ranks) {
		if (rk.rank_id != me->rank_id) continue;
		return rk.rank_level == 0 || ((uint32_t)rk.permissions & bit) != 0;
	}
	return false;
}
}  // namespace

// Append the caller's agency as top-level TLV fields, in the exact shape the
// client's receive handler (TgAgencyData, reversed at client FUN_113cd3d0)
// consumes. The gate is ROSTER_FLAGS bit0 — WITHOUT it the client leaves
// m_bNoAgency=true and ignores the whole block. Agency/rank name is tag NAME
// (0x370), NOT AGENCY_NAME/RANK_NAME. Returns the top-level field count (0 if
// the caller has no agency).
int TcpSession::append_agency_payload(std::vector<uint8_t>& response)
{
	int64_t agency_id = Database::GetAgencyIdForUser(user_id_);
	if (agency_id == 0) return 0;

	auto info = Database::GetAgencyInfo(agency_id);
	if (!info) return 0;

	// bit0 = "core agency data present" → clears m_bNoAgency. (bit4 = inventory.)
	const bool with_bank = agency_bank_due(agency_id);
	Write4B(response,     GA_T::ROSTER_FLAGS,             with_bank ? 0x11 : 0x1);
	WriteString(response, GA_T::NAME,                     info->name);        // 0x370
	WriteString(response, GA_T::AGENCY_MOTD,              info->motd);
	WriteString(response, GA_T::AGENCY_INFORMATION,       info->information);
	WriteString(response, GA_T::RECRUITING_LISTING_TEXT,  info->recruiting_text);
	// Flags are WCHAR_STR on the wire; client GetFlag reads first char (y/Y/t/T=true).
	WriteString(response, GA_T::RECRUITING_FLAG,          info->recruiting ? "y" : "n");
	WriteString(response, GA_T::RECRUITING_SUB_ONLY_FLAG, info->sub_only ? "y" : "n");
	Write4B(response,     GA_T::AGENCY_ID,                (uint32_t)info->id);
	// PLAYER_ID at top level = the local player's member id (→ m_nLocalPlayerId,
	// drives LocalPlayerIsLeader). Must match this member's PLAYER_ID row below,
	// which carries the join-time snapshot character_id — so send the caller's
	// member-row id, NOT the character currently played. A mismatch leaves the
	// client unable to resolve the local member row (selection/permissions dead).
	const auto* self_row = agency_member_by_user(*info, user_id_);
	Write4B(response,     GA_T::PLAYER_ID,
		(uint32_t)(self_row ? self_row->character_id : selected_character_id_));
	Write4B(response,     GA_T::COUNT,                    (uint32_t)info->members.size());

	// DATA_SET_RANKS — rows: RANK_ID, RANK_LEVEL, PERMISSION_FLAGS, NAME(0x370).
	append(response, GA_T::DATA_SET_RANKS & 0xFF, GA_T::DATA_SET_RANKS >> 8);
	append(response, info->ranks.size() & 0xFF, (info->ranks.size() >> 8) & 0xFF);
	for (const auto& rk : info->ranks) {
		append(response, 0x04, 0x00);  // 4 fields
		Write4B(response,     GA_T::RANK_ID,          (uint32_t)rk.rank_id);
		Write4B(response,     GA_T::RANK_LEVEL,       (uint32_t)rk.rank_level);
		Write4B(response,     GA_T::PERMISSION_FLAGS, (uint32_t)rk.permissions);
		WriteString(response, GA_T::NAME,             rk.rank_name);
	}

	// Online members (user_id -> map + current class) in ONE query scoped to
	// this agency. The roster is polled every 2s per client, so this path stays
	// at a fixed 5 queries regardless of member count: agency id, meta, ranks,
	// members (profile_id joined in), online map.
	const std::map<int64_t, Database::OnlineAgencyMemberRow> online =
		Database::GetOnlineAgencyMembers(agency_id);

	// DATA_SET_MEMBERS — the row parser (client FUN_113ccbe0) reads, in order:
	// PLAYER_NAME 0x3C5, PLAYER_ID 0x3C3, RANK_ID 0x417, LEVEL 0x303,
	// PROFILE_ID 0x3DF, ACTIVE_FLAG 0x16, MAP_NAME_MSG_ID 0x327 (the Location
	// column), PUBLIC_COMMENT 0x3F3, OFFICER_COMMENT 0x386. It stores
	// ACTIVE_FLAG inverted (absent/false => "offline"), so online must send "y".
	std::vector<int64_t> member_ids;
	for (const auto& m : info->members) member_ids.push_back(m.character_id);
	const auto member_levels = PlayerSessionStore::GetCharacterLevels(member_ids);
	append(response, GA_T::DATA_SET_MEMBERS & 0xFF, GA_T::DATA_SET_MEMBERS >> 8);
	append(response, info->members.size() & 0xFF, (info->members.size() >> 8) & 0xFF);
	for (const auto& m : info->members) {
		auto it = online.find(m.user_id);
		const bool is_online = (it != online.end());

		uint32_t map_msg_id = 0;
		if (is_online) {
			if (auto rec = MapGameInfo::LookupByName(it->second.map_name))
				map_msg_id = rec->friendly_name_msg_id;
		}
		// A map with no friendly-name msg id would localize to "Unknown" too.
		const bool send_location = (map_msg_id != 0);

		// Offline members OMIT MAP_NAME_MSG_ID: the parser only localizes the id
		// when the field is present, so leaving it out keeps
		// sMemberLocationString at its "" default (a 0 id localizes to
		// "Unknown"). Field count is per-row, so 8 vs 9 is fine.
		append(response, send_location ? 0x09 : 0x08, 0x00);
		WriteString(response, GA_T::PLAYER_NAME,     m.player_name);
		Write4B(response,     GA_T::PLAYER_ID,       (uint32_t)m.character_id);
		Write4B(response,     GA_T::RANK_ID,         (uint32_t)m.rank_id);
		// The member agent's real level (ga_characters.xp).
		{
			auto lv = member_levels.find(m.character_id);
			Write4B(response, GA_T::LEVEL, lv != member_levels.end() ? lv->second : 1u);
		}
		// Class column: the character being played right now when online;
		// the join-time snapshot's class is only the offline fallback.
		Write4B(response,     GA_T::PROFILE_ID,
			is_online && it->second.profile_id != 0 ? it->second.profile_id
			                                        : m.profile_id);
		WriteString(response, GA_T::ACTIVE_FLAG,     is_online ? "y" : "n");
		if (send_location) Write4B(response, GA_T::MAP_NAME_MSG_ID, map_msg_id);
		WriteString(response, GA_T::PUBLIC_COMMENT,  m.public_comment);
		// Officer notes only go out to ranks allowed to read them — the client
		// hides the box, but don't ship the text to a client that can't show it.
		WriteString(response, GA_T::OFFICER_COMMENT,
			agency_has_perm(*info, user_id_, Database::AGENCY_PERM_VIEW_OFFICER_MSG)
				? m.officer_comment : std::string());
	}

	// ROSTER_FLAGS, NAME, MOTD, INFORMATION, RECRUITING_TEXT, RECRUITING_FLAG,
	// SUB_ONLY, AGENCY_ID, PLAYER_ID, COUNT, DATA_SET_RANKS, DATA_SET_MEMBERS + bank fields.
	return 12 + append_agency_bank_fields(response, agency_id, with_bank);
}

void TcpSession::send_agency_get_roster_response()
{
	std::vector<uint8_t> payload;
	int fields = append_agency_payload(payload);

	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::AGENCY_GET_ROSTER;

	if (fields == 0) {
		// "You are not in an agency." MSG_ID==0x37B9 makes the receiver set
		// m_bNoAgency + ClearData — needed to reset the UI after a disband
		// (the creation menu shows again). Harmless before first create too.
		uint16_t item_count = 1;
		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);
		Write4B(response, GA_T::MSG_ID, 0x37B9);
		send_response(response);
		return;
	}

	uint16_t item_count = (uint16_t)fields;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	WriteNBytesRaw(response, payload);

	send_response(response);
}

// AGENCY_GET_DIRECTORY 0xD0. Client handler 0x113f5630 (TgUIAgencyDirectory, event 0x4c)
// reads DATA_SET 0x10C rows: AGENCY_ID, AGENCY_NAME, PLAYER_COUNT (members), RECRUITING_LISTING_TEXT,
// ALLIANCE_NAME, COUNT (alliance members), RECRUITING_FLAG / SUB_ONLY_FLAG ("y"/"n"). Optional
// AGENCY_NAME in the request = search text.
void TcpSession::send_agency_get_directory_response(const PacketView& pkt)
{
	std::string filter = pkt.ReadString(GA_T::AGENCY_NAME).value_or("");
	auto wild = [](std::string s) {  // the client sends "%%" for an empty box = any
		s.erase(std::remove_if(s.begin(), s.end(), [](char c) { return c == '%' || c == '*'; }), s.end()); return s; };
	filter = wild(filter);
	auto rows = Database::GetAgencyDirectory(filter, 100);
	{  // the client filters nothing - the search form's other fields are applied here (0 = no limit)
		const bool want_rec = pkt.ReadString(GA_T::RECRUITING_FLAG).value_or("") == "y";
		const bool want_sub = pkt.ReadString(GA_T::RECRUITING_SUB_ONLY_FLAG).value_or("") == "y";
		const uint32_t amin = pkt.Read4B(GA_T::AGENCY_MIN).value_or(0), amax = pkt.Read4B(GA_T::AGENCY_MAX).value_or(0);
		const uint32_t lmin = pkt.Read4B(GA_T::ALLIANCE_MIN).value_or(0), lmax = pkt.Read4B(GA_T::ALLIANCE_MAX).value_or(0);
		std::string ally = pkt.ReadString(GA_T::ALLIANCE_NAME).value_or("");
		auto lower = [](std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; };
		ally = lower(wild(ally));
		const size_t before = rows.size();
		rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const Database::AgencyDirRow& r) {
			if (want_rec && !r.recruiting) return true;
			if (want_sub && !r.sub_only) return true;
			if (amin && (uint32_t)r.members < amin) return true;
			if (amax && (uint32_t)r.members > amax) return true;
			if (lmin && (uint32_t)r.alliance_members < lmin) return true;
			if (lmax && (uint32_t)r.alliance_members > lmax) return true;
			if (!ally.empty() && lower(r.alliance_name).find(ally) == std::string::npos) return true;
			return false; }), rows.end());
		Logger::Log("agency", "[TCP] AGENCY_GET_DIRECTORY name='%s' rec=%d sub=%d members %u-%u alliance '%s' %u-%u -> %zu of %zu row(s)\n",
			filter.c_str(), want_rec, want_sub, amin, amax, ally.c_str(), lmin, lmax, rows.size(), before);
	}

	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::AGENCY_GET_DIRECTORY;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 0x01, 0x00);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, rows.size() & 0xFF, (rows.size() >> 8) & 0xFF);
	for (const auto& r : rows) {
		append(response, 0x08, 0x00);
		Write4B(response,     GA_T::AGENCY_ID,                (uint32_t)r.id);
		WriteString(response, GA_T::AGENCY_NAME,              r.name);
		Write4B(response,     GA_T::PLAYER_COUNT,             (uint32_t)r.members);
		WriteString(response, GA_T::RECRUITING_LISTING_TEXT,  r.recruiting_text);
		WriteString(response, GA_T::ALLIANCE_NAME,            r.alliance_name);
		Write4B(response,     GA_T::COUNT,                    (uint32_t)r.alliance_members);
		WriteString(response, GA_T::RECRUITING_FLAG,          r.recruiting ? "y" : "n");
		WriteString(response, GA_T::RECRUITING_SUB_ONLY_FLAG, r.sub_only ? "y" : "n");
	}
	send_response(response);
}

void TcpSession::handle_agency_disband()
{
	int64_t agency_id = Database::GetAgencyIdForUser(user_id_);
	if (agency_id == 0) { send_agency_get_roster_response(); return; }

	auto agency = Database::GetAgencyInfo(agency_id);
	if (!agency) return;
	// Non-leader: this is a plain LEAVE — drop just my member row. (Only the
	// leader's "remove last member" confirms as a disband.)
	if (agency->leader_user_id != user_id_) {
		Logger::Log("agency", "[TCP] AGENCY leave: user %lld leaving agency %lld\n",
			(long long)user_id_, (long long)agency_id);
		Database::RemoveAgencyMember(agency_id, user_id_);
		// Announced after the delete so the leaver isn't in the audience.
		ChatSession::AgencyMessage(agency_id, player_name + " has left the agency.");
		// Alliance tab needs the explicit reset too — it only refreshes while open.
		send_agency_get_roster_response();
		send_alliance_member_list_response();
		return;
	}

	// Everyone else loses their agency too — collect them before the delete so
	// their panels can be reset without waiting for a relog.
	std::vector<int64_t> others;
	for (const auto& m : agency->members) {
		if (m.user_id != user_id_) others.push_back(m.user_id);
	}

	if (!AgencyBank::IsEmpty(agency_id)) { send_agency_message(AgencyBank::MSG_DISBAND_HAS_INV, player_name); return; }
	Database::DisbandAgency(agency_id);
	// Reset both panels: no-agency roster + no-alliance member list.
	send_agency_get_roster_response();
	send_alliance_member_list_response();
	for (int64_t uid : others) DeliverAgencyRefresh(uid);
}

void TcpSession::handle_agency_create(const PacketView& pkt)
{
	std::string name = pkt.ReadString(GA_T::AGENCY_NAME).value_or("");
	float r = pkt.ReadFloat(GA_T::LINEAR_COLOR_R).value_or(0.0f);
	float g = pkt.ReadFloat(GA_T::LINEAR_COLOR_G).value_or(0.0f);
	float b = pkt.ReadFloat(GA_T::LINEAR_COLOR_B).value_or(0.0f);

	Logger::Log("agency", "[TCP] AGENCY_CREATE name='%s' color=(%.3f,%.3f,%.3f) user=%lld\n",
		name.c_str(), r, g, b, (long long)user_id_);

	if (name.empty() || user_id_ <= 0) return;
	// Bounded, printable names only (they are shown to every player and on the site)
	if (name.size() > 32 || std::any_of(name.begin(), name.end(),
	                                    [](unsigned char c) { return c < 0x20 || c == 0x7f; })) {
		Logger::Log("agency", "[TCP] AGENCY_CREATE refused: name too long or has control characters\n");
		return;
	}

	int64_t agency_id = Database::CreateAgency(
		name, r, g, b, user_id_, selected_character_id_, player_name);
	if (agency_id == 0) {
		Logger::Log("agency", "[TCP] AGENCY_CREATE rejected (dup name or already in agency)\n");
		return;  // ponytail: no error-reply reversed yet; client stays on the
		         // creation panel. Add AGENCY_CREATE error path when observed.
	}

	// Push the agency data as a roster response so the client's TgAgencyData
	// receiver clears m_bNoAgency and the UI leaves the creation panel. The
	// menu's own 2s roster poll is the backup path.
	send_agency_get_roster_response();
}

// AGENCY_INVITE (0xB5) — invite a player by name, same shape as ALLIANCE_INVITE.
// Target must be online (the popup is a live push); pending state drives accept.
void TcpSession::handle_agency_invite(const PacketView& pkt)
{
	std::string target_name = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
	Logger::Log("agency", "[TCP] AGENCY_INVITE target='%s' user=%lld\n",
		target_name.c_str(), (long long)user_id_);
	if (target_name.empty() || user_id_ <= 0) return;

	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	if (my_agency == 0) { send_agency_message(14265, target_name); return; }
	auto agency = Database::GetAgencyInfo(my_agency);
	if (!agency) return;
	if (!agency_has_perm(*agency, user_id_, Database::AGENCY_PERM_INVITE)) {
		Logger::Log("agency", "[TCP] AGENCY_INVITE denied: user %lld lacks the invite permission\n",
			(long long)user_id_);
		send_agency_message(14258, target_name);  // "Your rank does not allow..."
		return;
	}

	int64_t target_user = 0;
	std::shared_ptr<TcpSession> target_session;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& kv : g_sessions_) {
			auto s = kv.second.lock();
			if (s && s->player_name == target_name && s->user_id_ > 0) {
				target_session = s;
				target_user = s->user_id_;
				break;
			}
		}
	}
	if (!target_session) {
		// Offline players can't be invited (the popup is a live push), and the
		// client's own message for that case is "character was not found".
		Logger::Log("agency", "[TCP] AGENCY_INVITE: '%s' not online\n", target_name.c_str());
		send_agency_message(14259, target_name);
		return;
	}
	if (target_user == user_id_) return;
	const int64_t their_agency = Database::GetAgencyIdForUser(target_user);
	if (their_agency != 0) {
		Logger::Log("agency", "[TCP] AGENCY_INVITE: '%s' already in an agency\n",
			target_name.c_str());
		// 14260 "is already a member." / 14262 "is a member of another agency."
		send_agency_message(their_agency == my_agency ? 14260 : 14262, target_name);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(pending_agency_mutex_);
		if (pending_agency_invites_.count(target_user)) {
			send_agency_message(14267, target_name);  // "has a pending invite."
			return;
		}
		pending_agency_invites_[target_user] = my_agency;
	}
	target_session->send_agency_invitation(agency->name, player_name);
	send_agency_message(14261, target_name);  // "Invite to join agency was sent to..."
}

void TcpSession::handle_agency_accept()
{
	int64_t agency_id = 0;
	{
		std::lock_guard<std::mutex> lock(pending_agency_mutex_);
		auto it = pending_agency_invites_.find(user_id_);
		if (it == pending_agency_invites_.end()) {
			Logger::Log("agency", "[TCP] AGENCY_ACCEPT: no pending invite for user %lld\n",
				(long long)user_id_);
			return;
		}
		agency_id = it->second;
		pending_agency_invites_.erase(it);
	}

	// rank_id 2 = the seeded "Member" rank (see CreateAgency).
	if (!Database::AddAgencyMember(agency_id, user_id_, selected_character_id_,
	                               player_name, 2)) {
		return;
	}
	Logger::Log("agency", "[TCP] AGENCY_ACCEPT: user %lld joined agency %lld\n",
		(long long)user_id_, (long long)agency_id);
	ChatSession::AgencyMessage(agency_id, player_name + " has joined the agency.");
	// Roster + alliance both: the agency I just joined may already be in an
	// alliance, and the Alliance tab won't refresh on its own until opened.
	send_agency_get_roster_response();
	send_alliance_member_list_response();
}

void TcpSession::handle_agency_join(const PacketView& pkt)
{
	int64_t agency_id = (int64_t)pkt.Read4B(GA_T::AGENCY_ID).value_or(0);
	Logger::Log("agency", "[TCP] AGENCY_JOIN agency=%lld user=%lld\n",
		(long long)agency_id, (long long)user_id_);
	if (agency_id <= 0 || user_id_ <= 0) return;
	if (Database::GetAgencyIdForUser(user_id_) != 0) return;  // already in one
	auto agency = Database::GetAgencyInfo(agency_id);
	if (!agency || !agency->recruiting) {
		Logger::Log("agency", "[TCP] AGENCY_JOIN: agency %lld missing or not recruiting\n",
			(long long)agency_id);
		return;
	}
	if (!Database::AddAgencyMember(agency_id, user_id_, selected_character_id_, player_name, 2))
		return;
	ChatSession::AgencyMessage(agency_id, player_name + " has joined the agency.");
	send_agency_get_roster_response();
	send_alliance_member_list_response();
}

// AGENCY_PROMOTE (0xBB) / AGENCY_DEMOTE (0xBC). Client sends the member's
// PLAYER_ID, which is the character_id we put in the DATA_SET_MEMBERS rows
// (UC: TgAgencyData::Promote/Demote(int nPlayerId)). Ranks are ordered by
// rank_level ASC (0 = Leader), so promote = previous rank in that list.
void TcpSession::handle_agency_rank_change(const PacketView& pkt, bool promote)
{
	const int64_t target_char = (int64_t)pkt.Read4B(GA_T::PLAYER_ID).value_or(0);
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	Logger::Log("agency", "[TCP] AGENCY_%s target_char=%lld user=%lld agency=%lld\n",
		promote ? "PROMOTE" : "DEMOTE", (long long)target_char,
		(long long)user_id_, (long long)my_agency);
	if (target_char == 0 || my_agency == 0) return;

	auto info = Database::GetAgencyInfo(my_agency);
	if (!info) return;

	const Database::AgencyMemberRow* me = nullptr;
	const Database::AgencyMemberRow* target = nullptr;
	for (const auto& m : info->members) {
		if (m.user_id == user_id_)          me = &m;
		if (m.character_id == target_char)  target = &m;
	}
	if (!me || !target) return;

	auto level_of = [&](int rank_id) {
		for (const auto& rk : info->ranks) if (rk.rank_id == rank_id) return rk.rank_level;
		return 9999;
	};
	// info->ranks comes back ordered by rank_level ASC.
	size_t idx = info->ranks.size();
	for (size_t i = 0; i < info->ranks.size(); ++i) {
		if (info->ranks[i].rank_id == target->rank_id) { idx = i; break; }
	}
	if (idx >= info->ranks.size()) return;
	// idx 0 = the rank_level 0 Leader rank — promoting INTO it is TransferLeader
	// (a separate native), so promote stops one below.
	if (promote && idx <= 1) return;
	if (!promote && idx + 1 >= info->ranks.size()) return; // already bottom rank
	const auto& new_rank = info->ranks[promote ? idx - 1 : idx + 1];

	if (!agency_has_perm(*info, user_id_, promote ? Database::AGENCY_PERM_PROMOTE
	                                              : Database::AGENCY_PERM_DEMOTE)) {
		Logger::Log("agency", "[TCP] AGENCY rank change denied: no permission\n");
		return;
	}
	// Only outrank-ers may act, and never onto/above your own rank.
	const int my_level = level_of(me->rank_id);
	if (my_level >= level_of(target->rank_id) || my_level >= new_rank.rank_level) {
		Logger::Log("agency", "[TCP] AGENCY rank change denied: my_level=%d target_level=%d new_level=%d\n",
			my_level, level_of(target->rank_id), new_rank.rank_level);
		return;
	}

	if (!Database::SetAgencyMemberRank(my_agency, target_char, new_rank.rank_id)) return;
	// The affected member (and everyone else) picks this up on the 2s roster poll.
	send_agency_get_roster_response();
}

// AGENCY_UPDATE_RANKS (0xCB) — the Management tab's rank editor Submit. Sends
// DATA_SET_RANKS (0x193) with EVERY rank row: RANK_ID(0x417), NAME(0x370),
// PERMISSION_FLAGS(0x3B0), RANK_LEVEL(0x418) — so add / rename / delete /
// permission edits all arrive as one replacement set. Leader only.
void TcpSession::handle_agency_update_ranks(const PacketView& pkt)
{
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	if (my_agency == 0) return;
	auto info = Database::GetAgencyInfo(my_agency);
	if (!info) return;

	const auto* me = agency_member_by_user(*info, user_id_);
	if (!me || agency_rank_level(*info, me->rank_id) != 0) {
		Logger::Log("agency", "[TCP] AGENCY_UPDATE_RANKS rejected: user %lld is not the leader\n",
			(long long)user_id_);
		return;
	}

	std::vector<Database::AgencyRankRow> ranks;
	for (const auto& row : pkt.GetDataSet(GA_T::DATA_SET_RANKS)) {
		Database::AgencyRankRow rk;
		rk.rank_id     = (int)row.Read4B(GA_T::RANK_ID).value_or(0);
		rk.rank_level  = (int)row.Read4B(GA_T::RANK_LEVEL).value_or(0);
		rk.permissions = (int)row.Read4B(GA_T::PERMISSION_FLAGS).value_or(0);
		rk.rank_name   = row.ReadString(GA_T::NAME).value_or("");
		ranks.push_back(std::move(rk));
	}
	Logger::Log("agency", "[TCP] AGENCY_UPDATE_RANKS user=%lld rows=%d\n",
		(long long)user_id_, (int)ranks.size());
	if (ranks.empty()) return;

	AgencyBank::NormalizeRanks(ranks);
	if (!Database::ReplaceAgencyRanks(my_agency, ranks)) return;
	send_agency_get_roster_response();
	for (int64_t uid : Database::GetAgencyMemberUserIds(my_agency)) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

// AGENCY_SET_NOTE (0xC4) — the four text edits share this opcode; which one is
// meant is told by the tag present (UC: TgAgencyData::SetMOTD(string),
// SetDescription(string), SetPublicComment/SetOfficerComment(int nPlayerId,
// string)). A PLAYER_ID alongside the text = a per-member note.
void TcpSession::handle_agency_set_note(const PacketView& pkt)
{
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	if (my_agency == 0) return;
	auto info = Database::GetAgencyInfo(my_agency);
	if (!info) return;

	const auto* me = agency_member_by_user(*info, user_id_);
	if (!me) return;

	auto motd        = pkt.ReadString(GA_T::AGENCY_MOTD);
	auto information = pkt.ReadString(GA_T::AGENCY_INFORMATION);
	auto public_note = pkt.ReadString(GA_T::PUBLIC_COMMENT);
	auto officer_note= pkt.ReadString(GA_T::OFFICER_COMMENT);
	// Fallback to the caller's OWN member row: comments are keyed by the
	// join-time snapshot character_id, not whichever character is being played.
	const int64_t target_char = (int64_t)pkt.Read4B(GA_T::PLAYER_ID)
		.value_or((uint32_t)me->character_id);

	Logger::Log("agency", "[TCP] AGENCY_SET_NOTE user=%lld motd=%d info=%d pub=%d off=%d target=%lld\n",
		(long long)user_id_, (int)motd.has_value(), (int)information.has_value(),
		(int)public_note.has_value(), (int)officer_note.has_value(), (long long)target_char);

	bool changed = false;
	if (motd && agency_has_perm(*info, user_id_, Database::AGENCY_PERM_EDIT_MOTD))
		changed |= Database::SetAgencyText(my_agency, true, *motd);
	if (information && agency_has_perm(*info, user_id_, Database::AGENCY_PERM_EDIT_DESCRIPTION))
		changed |= Database::SetAgencyText(my_agency, false, *information);
	if (public_note && agency_has_perm(*info, user_id_, Database::AGENCY_PERM_EDIT_PUBLIC_MSG))
		changed |= Database::SetAgencyMemberComment(my_agency, target_char, false, *public_note);
	if (officer_note && agency_has_perm(*info, user_id_, Database::AGENCY_PERM_EDIT_OFFICER_MSG))
		changed |= Database::SetAgencyMemberComment(my_agency, target_char, true, *officer_note);
	if (!changed) return;

	send_agency_get_roster_response();
	// MOTD/description are agency-wide — everyone else's panel shows them too.
	for (int64_t uid : Database::GetAgencyMemberUserIds(my_agency)) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

// AGENCY_UPDATE_RECRUITING (0xCA) — the Recruiting tab's Submit. Observed:
// RECRUITING_LISTING_TEXT(0x41F) + RECRUITING_FLAG(0x41E) +
// RECRUITING_SUB_ONLY_FLAG(0x420); both flags are "y"/"n" strings, same as the
// roster response direction.
void TcpSession::handle_agency_update_recruiting(const PacketView& pkt)
{
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	if (my_agency == 0) return;
	auto info = Database::GetAgencyInfo(my_agency);
	if (!info) return;

	auto flag_of = [&](uint16_t tag, bool fallback) {
		auto s = pkt.ReadString(tag);
		if (!s || s->empty()) return fallback;
		const char c = (*s)[0];
		return c == 'y' || c == 'Y' || c == 't' || c == 'T';
	};
	const std::string text = pkt.ReadString(GA_T::RECRUITING_LISTING_TEXT)
		.value_or(info->recruiting_text);
	const bool recruiting = flag_of(GA_T::RECRUITING_FLAG,          info->recruiting);
	const bool sub_only   = flag_of(GA_T::RECRUITING_SUB_ONLY_FLAG, info->sub_only);

	Logger::Log("agency", "[TCP] AGENCY_UPDATE_RECRUITING user=%lld recruiting=%d sub_only=%d text='%s'\n",
		(long long)user_id_, (int)recruiting, (int)sub_only, text.c_str());

	if (!Database::SetAgencyRecruiting(my_agency, text, recruiting, sub_only)) return;
	send_agency_get_roster_response();
	for (int64_t uid : Database::GetAgencyMemberUserIds(my_agency)) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

// AGENCY_SET_OWNER (0xC6) — Management tab's "Transfer Leader". The client
// resolves the typed name itself and sends the target's PLAYER_ID
// (= character_id). Leader takes the target's old rank; target takes rank 0.
void TcpSession::handle_agency_set_owner(const PacketView& pkt)
{
	const int64_t target_char = (int64_t)pkt.Read4B(GA_T::PLAYER_ID).value_or(0);
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	Logger::Log("agency", "[TCP] AGENCY_SET_OWNER target_char=%lld user=%lld agency=%lld\n",
		(long long)target_char, (long long)user_id_, (long long)my_agency);
	if (target_char == 0 || my_agency == 0) return;

	auto info = Database::GetAgencyInfo(my_agency);
	if (!info || info->leader_user_id != user_id_) {
		Logger::Log("agency", "[TCP] AGENCY_SET_OWNER rejected: user %lld is not the leader\n",
			(long long)user_id_);
		return;
	}
	const Database::AgencyMemberRow* target = nullptr;
	for (const auto& m : info->members) if (m.character_id == target_char) target = &m;
	if (!target || target->user_id == user_id_) return;

	// Leader drops to the rank just below rank_level 0 (ranks are level-ASC).
	const int demoted_rank = info->ranks.size() > 1 ? info->ranks[1].rank_id : 0;
	const int64_t new_leader_user = target->user_id;

	Database::SetAgencyMemberRank(my_agency, target_char, 0);
	Database::SetAgencyMemberRank(my_agency, selected_character_id_, demoted_rank);
	Database::SetAgencyLeader(my_agency, new_leader_user);

	send_agency_get_roster_response();
	for (int64_t uid : Database::GetAgencyMemberUserIds(my_agency)) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

// Kick a member: AGENCY_PLAYER_REMOVE (0xBF), or AGENCY_REMOVE (0xBD) carrying
// someone else's PLAYER_ID. Self-targeted / no target = leave-or-disband.
void TcpSession::handle_agency_kick(int64_t target_char)
{
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	Logger::Log("agency", "[TCP] AGENCY kick target_char=%lld user=%lld agency=%lld\n",
		(long long)target_char, (long long)user_id_, (long long)my_agency);
	if (target_char == 0 || my_agency == 0) return;

	auto info = Database::GetAgencyInfo(my_agency);
	if (!info) return;

	const Database::AgencyMemberRow* me = nullptr;
	const Database::AgencyMemberRow* target = nullptr;
	for (const auto& m : info->members) {
		if (m.user_id == user_id_)         me = &m;
		if (m.character_id == target_char) target = &m;
	}
	if (!me || !target) return;

	auto level_of = [&](int rank_id) {
		for (const auto& rk : info->ranks) if (rk.rank_id == rank_id) return rk.rank_level;
		return 9999;
	};
	if (!agency_has_perm(*info, user_id_, Database::AGENCY_PERM_KICK)) {
		Logger::Log("agency", "[TCP] AGENCY kick denied: no permission\n");
		return;
	}
	if (level_of(me->rank_id) >= level_of(target->rank_id)) {
		Logger::Log("agency", "[TCP] AGENCY kick denied: target outranks or equals kicker\n");
		return;
	}

	const int64_t kicked_user = target->user_id;
	const std::string kicked_name = target->player_name;
	if (!Database::RemoveAgencyMember(my_agency, kicked_user)) return;
	ChatSession::AgencyMessage(my_agency, kicked_name + " has left the agency.");
	send_agency_get_roster_response();
	// Push the reset to the kicked player too: their agency panel would clear on
	// its own roster poll, but the Alliance tab only refreshes while that tab is
	// open, so without this it stays stale until relog.
	DeliverAgencyRefresh(kicked_user);
}

// Push the caller's CURRENT agency + alliance state to whichever live session
// owns target_user_id (so it doubles as the "no agency"/"no alliance" reset).
// Needed whenever a player's agency or alliance changes without them asking —
// the roster poll self-heals the AgentInfo panel, but the Alliance tab only
// refreshes while it's open, so it stays stale until relog otherwise.
void TcpSession::DeliverAgencyRefresh(int64_t target_user_id)
{
	if (target_user_id <= 0) return;
	std::shared_ptr<TcpSession> s;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& kv : g_sessions_) {
			auto cand = kv.second.lock();
			if (cand && cand->user_id_ == target_user_id) { s = cand; break; }
		}
	}
	if (!s) return;
	s->send_agency_get_roster_response();
	s->send_alliance_member_list_response();
}

// Agency status/error line (the Invite tab's message label). The full set of
// templates exists in asm_data_set_msg_translations: 14258 no permission, 14259
// character not found, 14260 already a member, 14261 invite sent, 14262 member
// of another agency, 14265 you have no agency, 14267 has a pending invite.
// Carrier: echoed on AGENCY_INVITE (0xB5) — the request opcode — with MSG_ID +
// the @@player_name@@ token. Tested: renders in the Invite tab's label, an OK
// dialog, and the chat log. Do NOT reuse 0xB6, that one raises the HUD dialog.
void TcpSession::send_agency_message(uint32_t msg_id, const std::string& player_name_token)
{
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::AGENCY_INVITE;
	uint16_t item_count = 2;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	Write4B(response,     GA_T::MSG_ID,      msg_id);
	WriteString(response, GA_T::PLAYER_NAME, player_name_token);
	Logger::Log("agency", "[TCP] send agency message msg_id=%u token='%s' to user=%lld\n",
		msg_id, player_name_token.c_str(), (long long)user_id_);
	send_response(response);
}

void TcpSession::send_agency_invitation(const std::string& agency_name,
                                        const std::string& inviter_leader_name)
{
	// MSG_ID 14266 = "@@leader_name@@ has invited you to join agency
	// \"@@agency_name@@\"." — tokens substitute by field name.
	// NOT reversed at the client yet (unlike the roster/alliance receivers): the
	// client shows this as a HUD dialog — TgUIPrimaryHUD_DialogQuery.HUDDialogType
	// DialogInviteToAgency, the same widget the team invite (0x4B) drives — so the
	// shape mirrors send_team_invitation_popup (MSG_ID + LEADER_NAME), plus
	// AGENCY_NAME for the second token. Confirm on first test.
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::AGENCY_INVITATION;
	uint16_t item_count = 3;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	Write4B(response,     GA_T::MSG_ID,      14266);
	WriteString(response, GA_T::LEADER_NAME, inviter_leader_name);
	WriteString(response, GA_T::AGENCY_NAME, agency_name);
	Logger::Log("agency", "[TCP] send AGENCY_INVITATION agency='%s' from '%s' to user=%lld\n",
		agency_name.c_str(), inviter_leader_name.c_str(), (long long)user_id_);
	send_response(response);
}

// Alliance member-list payload, matching the client TgAllianceData receiver
// (reversed at client FUN_113ce850). Gate is ROSTER_FLAGS bit0 (clears
// m_bNoAlliance). Alliance name = NAME(0x370); owner agency name = AGENCY_NAME.
// Returns top-level field count (0 if the caller's agency has no alliance).
int TcpSession::append_alliance_payload(std::vector<uint8_t>& response)
{
	int64_t agency_id = Database::GetAgencyIdForUser(user_id_);
	if (agency_id == 0) return 0;
	int64_t alliance_id = Database::GetAllianceIdForAgency(agency_id);
	if (alliance_id == 0) return 0;

	auto info = Database::GetAllianceInfo(alliance_id);
	if (!info) return 0;

	// bit0 = core present (clears m_bNoAlliance); bit2 = member-agency data set
	// present (the receiver gates DATA_SET_AGENCIES on it). Client requests flags=5.
	Write4B(response,     GA_T::ROSTER_FLAGS,     0x5);
	WriteString(response, GA_T::NAME,             info->name);              // 0x370
	WriteString(response, GA_T::AGENCY_NAME,      info->owner_agency_name); // 0x24
	Write4B(response,     GA_T::ALLIANCE_ID,      (uint32_t)info->id);
	Write4B(response,     GA_T::OWNER_AGENCY_ID,  (uint32_t)info->owner_agency_id);
	Write4B(response,     GA_T::COUNT,            (uint32_t)info->members.size());
	// "Formed <date>" line. The client's DATETIME formatter (FUN_10955ab0) reads
	// this double as DAYS SINCE 1900-01-01 (input 0 -> 01/01/1900). Convert from
	// unix seconds: days = (unix + 2208988800) / 86400  (offset = 1900->1970 secs).
	const double kSecs1900to1970 = 2208988800.0;
	double days_since_1900 = ((double)info->created_at + kSecs1900to1970) / 86400.0;
	WriteDouble(response, GA_T::CREATED_DATETIME, days_since_1900);

	// DATA_SET_AGENCIES — rows: AGENCY_NAME, AGENCY_ID, PLAYER_COUNT, TERRITORY_COUNT.
	append(response, GA_T::DATA_SET_AGENCIES & 0xFF, GA_T::DATA_SET_AGENCIES >> 8);
	append(response, info->members.size() & 0xFF, (info->members.size() >> 8) & 0xFF);
	for (const auto& m : info->members) {
		append(response, 0x04, 0x00);  // 4 fields
		WriteString(response, GA_T::AGENCY_NAME,     m.agency_name);
		Write4B(response,     GA_T::AGENCY_ID,       (uint32_t)m.agency_id);
		Write4B(response,     GA_T::PLAYER_COUNT,    (uint32_t)m.member_count);
		Write4B(response,     GA_T::TERRITORY_COUNT, (uint32_t)m.territory_count);
	}

	// ROSTER_FLAGS, NAME, AGENCY_NAME, ALLIANCE_ID, OWNER_AGENCY_ID, COUNT,
	// CREATED_DATETIME, DATA_SET_AGENCIES.
	return 8;
}

// The client's alliance receiver needs a DEFINITIVE answer to show the Alliance
// tab: either the alliance data, or the explicit "no alliance" signal
// MSG_ID==0x624F (which sets m_bNoAlliance + runs ClearData, unlocking the
// in-tab Create panel). An empty response is ignored and leaves the tab blank.
void TcpSession::send_alliance_member_list_response()
{
	std::vector<uint8_t> payload;
	int fields = append_alliance_payload(payload);

	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::ALLIANCE_GET_MEMBER_LIST;

	if (fields == 0) {
		// "You are not in an alliance."
		uint16_t item_count = 1;
		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);
		Write4B(response, GA_T::MSG_ID, 0x624F);
		send_response(response);
		return;
	}

	uint16_t item_count = (uint16_t)fields;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	WriteNBytesRaw(response, payload);

	send_response(response);
}

void TcpSession::handle_alliance_create(const PacketView& pkt)
{
	// Observed tag is NAME (0x370); the other two are harmless fallbacks.
	std::string name = pkt.ReadString(GA_T::ALLIANCE_NAME)
		.value_or(pkt.ReadString(GA_T::NAME)
		.value_or(pkt.ReadString(GA_T::AGENCY_NAME).value_or("")));

	Logger::Log("agency", "[TCP] ALLIANCE_CREATE name='%s' user=%lld\n",
		name.c_str(), (long long)user_id_);

	if (name.empty() || user_id_ <= 0) return;

	// Must be in an agency AND be its leader to found an alliance.
	int64_t agency_id = Database::GetAgencyIdForUser(user_id_);
	if (agency_id == 0) {
		Logger::Log("agency", "[TCP] ALLIANCE_CREATE rejected: user %lld not in an agency\n",
			(long long)user_id_);
		return;
	}
	auto agency = Database::GetAgencyInfo(agency_id);
	if (!agency || agency->leader_user_id != user_id_) {
		Logger::Log("agency", "[TCP] ALLIANCE_CREATE rejected: user %lld not agency leader\n",
			(long long)user_id_);
		return;
	}

	int64_t alliance_id = Database::CreateAlliance(name, agency_id);
	if (alliance_id == 0) {
		Logger::Log("agency", "[TCP] ALLIANCE_CREATE rejected (dup name or agency already allied)\n");
		return;
	}

	// Push the member list so the client's TgAllianceData clears m_bNoAlliance.
	send_alliance_member_list_response();
	// My agency-mates just gained an alliance too.
	for (int64_t uid : Database::GetAgencyMemberUserIds(agency_id)) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}


void TcpSession::handle_alliance_disband()
{
	int64_t agency_id = Database::GetAgencyIdForUser(user_id_);
	int64_t alliance_id = agency_id ? Database::GetAllianceIdForAgency(agency_id) : 0;
	if (alliance_id == 0) { send_alliance_member_list_response(); return; }

	auto info = Database::GetAllianceInfo(alliance_id);
	if (!info || info->owner_agency_id != agency_id) {
		Logger::Log("agency", "[TCP] ALLIANCE_DISBAND rejected: agency %lld does not own alliance %lld\n",
			(long long)agency_id, (long long)alliance_id);
		return;  // only the owning agency can disband
	}

	// Collect everyone in the alliance BEFORE the delete — after it, the
	// membership rows are gone and there's no way to find them.
	auto affected = Database::GetAllianceMemberUserIds(alliance_id);
	Database::DisbandAlliance(alliance_id);
	send_alliance_member_list_response();
	// Push the reset to every other member agency's players: their Alliance tab
	// (and the AgentInfo line) would otherwise stay stale until relog.
	for (int64_t uid : affected) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

// ALLIANCE_REMOVE (0xD3) handles both "owner kicks a member agency" and "member
// agency leaves" — both send AGENCY_ID of the agency to drop.
void TcpSession::handle_alliance_remove(const PacketView& pkt)
{
	uint32_t target_agency = pkt.Read4B(GA_T::AGENCY_ID).value_or(0);
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	int64_t alliance_id = my_agency ? Database::GetAllianceIdForAgency(my_agency) : 0;
	Logger::Log("agency", "[TCP] ALLIANCE_REMOVE target_agency=%u my_agency=%lld alliance=%lld\n",
		target_agency, (long long)my_agency, (long long)alliance_id);
	if (target_agency == 0 || alliance_id == 0) return;

	auto info = Database::GetAllianceInfo(alliance_id);
	if (!info) return;

	const bool leaving = ((int64_t)target_agency == my_agency);
	const bool is_owner = (info->owner_agency_id == my_agency);
	if (!leaving && !is_owner) {
		Logger::Log("agency", "[TCP] ALLIANCE_REMOVE rejected: not owner and not self-leave\n");
		return;
	}
	if ((int64_t)target_agency == info->owner_agency_id) {
		// The owner agency can't be removed from its own alliance — that's a disband.
		Logger::Log("agency", "[TCP] ALLIANCE_REMOVE: target is owner agency; use disband\n");
		return;
	}
	// Target must actually be in THIS alliance.
	if (Database::GetAllianceIdForAgency((int64_t)target_agency) != alliance_id) {
		Logger::Log("agency", "[TCP] ALLIANCE_REMOVE: agency %u not in alliance %lld\n",
			target_agency, (long long)alliance_id);
		return;
	}

	// Both sides change: the dropped agency loses the alliance, the remaining
	// ones lose a row. Collect before the delete so the dropped agency's players
	// are still reachable through the alliance join.
	auto affected = Database::GetAllianceMemberUserIds(alliance_id);
	Database::RemoveAgencyFromAlliance((int64_t)target_agency);
	send_alliance_member_list_response();
	for (int64_t uid : affected) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

void TcpSession::handle_alliance_invite(const PacketView& pkt)
{
	std::string leader_name = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
	Logger::Log("agency", "[TCP] ALLIANCE_INVITE target-leader='%s' user=%lld\n",
		leader_name.c_str(), (long long)user_id_);
	if (leader_name.empty() || user_id_ <= 0) return;

	// I must own an alliance.
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	int64_t alliance_id = my_agency ? Database::GetAllianceIdForAgency(my_agency) : 0;
	if (alliance_id == 0) return;
	auto alliance = Database::GetAllianceInfo(alliance_id);
	if (!alliance || alliance->owner_agency_id != my_agency) {
		Logger::Log("agency", "[TCP] ALLIANCE_INVITE rejected: user %lld not alliance owner\n",
			(long long)user_id_);
		return;
	}

	// Resolve the target agency by its leader's player name.
	int64_t target_agency = Database::GetAgencyIdByLeaderName(leader_name);
	if (target_agency == 0 || target_agency == my_agency) {
		Logger::Log("agency", "[TCP] ALLIANCE_INVITE: no other agency led by '%s'\n",
			leader_name.c_str());
		return;
	}
	if (Database::GetAllianceIdForAgency(target_agency) != 0) {
		Logger::Log("agency", "[TCP] ALLIANCE_INVITE: target agency %lld already in an alliance\n",
			(long long)target_agency);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(pending_alliance_mutex_);
		pending_alliance_invites_[target_agency] = alliance_id;
	}
	auto target = Database::GetAgencyInfo(target_agency);
	int64_t target_user = target ? target->leader_user_id : 0;
	// @@leader_name@@ in the popup = the inviting leader (me).
	DeliverAllianceInvitation(target_user, alliance->name, player_name, alliance_id);
}

void TcpSession::handle_alliance_accept()
{
	int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	if (my_agency == 0) return;

	int64_t alliance_id = 0;
	{
		std::lock_guard<std::mutex> lock(pending_alliance_mutex_);
		auto it = pending_alliance_invites_.find(my_agency);
		if (it == pending_alliance_invites_.end()) {
			Logger::Log("agency", "[TCP] ALLIANCE_ACCEPT: no pending invite for agency %lld\n",
				(long long)my_agency);
			return;
		}
		alliance_id = it->second;
		pending_alliance_invites_.erase(it);
	}

	if (!Database::AddAgencyToAlliance(alliance_id, my_agency)) {
		Logger::Log("agency", "[TCP] ALLIANCE_ACCEPT: AddAgencyToAlliance(%lld,%lld) failed\n",
			(long long)alliance_id, (long long)my_agency);
		return;
	}
	Logger::Log("agency", "[TCP] ALLIANCE_ACCEPT: agency %lld joined alliance %lld\n",
		(long long)my_agency, (long long)alliance_id);
	send_alliance_member_list_response();
	// Everyone already in the alliance gains a member row — push it to them too.
	for (int64_t uid : Database::GetAllianceMemberUserIds(alliance_id)) {
		if (uid != user_id_) DeliverAgencyRefresh(uid);
	}
}

bool TcpSession::DeliverAllianceInvitation(int64_t target_user_id,
                                           const std::string& alliance_name,
                                           const std::string& inviter_leader_name,
                                           int64_t alliance_id)
{
	if (target_user_id <= 0) return false;
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	for (auto& kv : g_sessions_) {
		auto s = kv.second.lock();
		if (s && s->user_id_ == target_user_id) {
			s->send_alliance_invitation(alliance_name, inviter_leader_name, alliance_id);
			return true;
		}
	}
	Logger::Log("agency", "[TCP] DeliverAllianceInvitation: target user %lld not online\n",
		(long long)target_user_id);
	return false;
}

void TcpSession::send_alliance_invitation(const std::string& alliance_name,
                                          const std::string& inviter_leader_name,
                                          int64_t alliance_id)
{
	// The popup is a MSG_ID template + @@token@@ substitution (like team invites).
	// MSG_ID 25111 = "Invitation from @@leader_name@@ to join alliance
	// @@alliance_name@@" (asm_data_set_msg_translations). Tokens map by name:
	// @@leader_name@@ -> LEADER_NAME, @@alliance_name@@ -> ALLIANCE_NAME.
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::ALLIANCE_INVITATION;
	uint16_t item_count = 4;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	Write4B(response,     GA_T::MSG_ID,        25111);
	WriteString(response, GA_T::LEADER_NAME,   inviter_leader_name);
	WriteString(response, GA_T::ALLIANCE_NAME, alliance_name);
	Write4B(response,     GA_T::ALLIANCE_ID,   (uint32_t)alliance_id);
	Logger::Log("agency", "[TCP] send ALLIANCE_INVITATION alliance='%s' from '%s' to user=%lld\n",
		alliance_name.c_str(), inviter_leader_name.c_str(), (long long)user_id_);
	send_response(response);
}

void TcpSession::send_query_players_response(const PacketView& pkt)
{
	// CALLER_ID is the GObjObjects index of the client's TgDataSet; without
	// it MarshalReceived2 can't route the response (see player-search-tcp-protocol.md).
	const uint32_t caller_id = pkt.Read4B(GA_T::CALLER_ID).value_or(0);
	if (caller_id == 0) {
		Logger::Log("tcp", "[TCP] QUERY_PLAYERS without CALLER_ID, dropping\n");
		return;
	}

	// Filters — absent field = no constraint.
	auto name_filter     = pkt.ReadString(GA_T::PLAYER_NAME);
	auto agency_filter   = pkt.ReadString(GA_T::AGENCY_NAME);
	auto alliance_filter = pkt.ReadString(GA_T::ALLIANCE_NAME);
	auto level_min       = pkt.Read4B(GA_T::LEVEL_MIN);
	auto level_max       = pkt.Read4B(GA_T::LEVEL_MAX);
	auto profile_filter  = pkt.Read4B(GA_T::PROFILE_ID);
	auto team_filter     = pkt.Read1B(GA_T::TEAM_FLAG);
	auto mission_filter  = pkt.ReadString(GA_T::MISSION_FLAG);  // string flag "T"/"F"
	// SYS_AVA_TEAMS_OK (the Team menu's AvA checkbox) isn't tracked yet — ignored.

	auto to_lower = [](std::string s) {
		for (auto& c : s) c = (char)std::tolower((unsigned char)c);
		return s;
	};
	auto contains_ci = [&](const std::string& haystack, const std::string& needle) {
		return to_lower(haystack).find(to_lower(needle)) != std::string::npos;
	};

	// Real level per listed agent (ga_characters.xp) and
	// Combat Rating stars from it - was 50 + 5 stars for everyone.
	// send_response chunks big payloads, so this is a UI sanity bound: the
	// Team menu list scrolls, the standalone search menu shows 16 (paging
	// natives are no-op stubs in this client build).
	constexpr size_t   kMaxRows = 50;

	const auto players = InstanceRegistry::GetActiveSearchablePlayers();
	const auto affiliations = Database::GetAffiliationsByCharacter();

	std::vector<int64_t> player_ids;
	for (const auto& p : players) player_ids.push_back(p.character_id);
	const auto levels = PlayerSessionStore::GetCharacterLevels(player_ids);

	std::vector<uint8_t> rows;
	size_t row_count = 0;
	for (const auto& p : players) {
		if (row_count >= kMaxRows) break;
		uint32_t level = 1;
		if (auto lv = levels.find(p.character_id); lv != levels.end()) level = lv->second;
		const bool teamed = TeamService::IsTeamed(p.session_guid);
		Database::AffiliationRow aff;
		if (auto a = affiliations.find(p.character_id); a != affiliations.end())
			aff = a->second;
		if (name_filter && !contains_ci(p.player_name, *name_filter)) continue;
		if (agency_filter && !agency_filter->empty() &&
			!contains_ci(aff.agency_name, *agency_filter)) continue;
		if (alliance_filter && !alliance_filter->empty() &&
			!contains_ci(aff.alliance_name, *alliance_filter)) continue;
		if (level_min && level < *level_min) continue;
		if (level_max && level > *level_max) continue;
		if (profile_filter && p.profile_id != *profile_filter) continue;
		if (team_filter && teamed != (*team_filter != 0)) continue;
		if (mission_filter && p.in_mission != (*mission_filter == "T")) continue;

		uint32_t map_msg_id = 0;
		if (auto rec = MapGameInfo::LookupByName(p.map_name))
			map_msg_id = rec->friendly_name_msg_id;

		append(rows, 0x0A, 0x00);  // 10 fields per row
		Write4B(rows, GA_T::PLAYER_ID, (uint32_t)p.character_id);
		WriteString(rows, GA_T::PLAYER_NAME, p.player_name);
		WriteString(rows, GA_T::AGENCY_NAME, aff.agency_name);
		WriteString(rows, GA_T::ALLIANCE_NAME, aff.alliance_name);
		Write4B(rows, GA_T::LEVEL, level);
		Write4B(rows, GA_T::PROFILE_ID, p.profile_id);
		Write4B(rows, GA_T::MAP_NAME_MSG_ID, map_msg_id);
		// Always ship SKILL_RATING — the Team menu reads it into an
		// uninitialized stack float when absent.
		WriteFloat(rows, GA_T::SKILL_RATING, CombatRating::Stars(p.character_id, level));
		Write1B(rows, GA_T::TEAM_FLAG, teamed ? 1 : 0);
		WriteString(rows, GA_T::MISSION_FLAG, p.in_mission ? "T" : "F");
		++row_count;
	}

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::QUERY_PLAYERS;
	uint16_t item_count = 2;  // CALLER_ID + DATA_SET

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::CALLER_ID, caller_id);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, row_count & 0xFF, (row_count >> 8) & 0xFF);
	WriteNBytesRaw(response, rows);

	Logger::Log("tcp", "[TCP] QUERY_PLAYERS: caller_id=%u online=%d matched=%d\n",
		caller_id, (int)players.size(), (int)row_count);

	send_response(response);
}

void TcpSession::handle_player_command(const PacketView& pkt)
{
	if (session_guid_.empty()) return;

	// TEXT_VALUE is flagged TYPE_TCP_WCHAR_STR, but the chat path proves this
	// client ships narrow strings on the wire. Read narrow, fall back to wide.
	std::string raw = pkt.ReadString(GA_T::TEXT_VALUE).value_or("");
	if (raw.empty()) {
		if (auto wide = pkt.ReadWideString(GA_T::TEXT_VALUE)) {
			raw.assign(wide->begin(), wide->end());  // ASCII-only narrowing
		}
	}

	// The client forwards the command with its leading '/' intact on some
	// paths and stripped on others — tolerate both.
	size_t start = raw.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return;
	if (raw[start] == '/') start++;

	size_t sep = raw.find_first_of(" \t", start);
	const std::string token = raw.substr(start, sep == std::string::npos
		? std::string::npos : sep - start);
	std::string rest;
	if (sep != std::string::npos) {
		size_t body = raw.find_first_not_of(" \t", sep);
		if (body != std::string::npos) rest = raw.substr(body);
		while (!rest.empty() && (rest.back() == ' ' || rest.back() == '\t' ||
		                         rest.back() == '\r' || rest.back() == '\n')) {
			rest.pop_back();
		}
	}
	if (token.empty()) return;

	const auto channel = ChatCommand::ChannelForCommandToken(token);
	if (!channel) {
		std::string lower = token;
		for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (PlayerCommands::Run(session_guid_, lower, rest)) return;
		Logger::Log("chat-command",
			"[ChatCmd] PLAYER_COMMAND guid=%s token='%s' outcome=unknown\n",
			session_guid_.c_str(), token.c_str());
		ChatSession::SystemMessageToGuid(session_guid_,
			"*** Unknown command: /" + token + " ***");
		return;
	}
	if (rest.empty()) {
		ChatSession::SystemMessageToGuid(session_guid_,
			"*** Usage: /" + token + " <message> ***");
		return;
	}

	const bool sent = ChatSession::SendChannelMessageFrom(session_guid_, *channel, rest);
	Logger::Log("chat-command",
		"[ChatCmd] PLAYER_COMMAND guid=%s token='%s' channel=%u outcome=%s\n",
		session_guid_.c_str(), token.c_str(), *channel, sent ? "sent" : "no_chat_session");
	if (!sent) {
		ChatSession::SystemMessageToGuid(session_guid_,
			"*** Chat is not connected ***");
	}
}

void TcpSession::handle_friend_update(const PacketView& pkt)
{
	if (user_id_ == 0) return;
	const auto friend_id = pkt.Read4B(GA_T::FRIEND_PLAYER_ID);

	// Remove: PLAYER_NAME + IGNORE_FLAG + FRIEND_PLAYER_ID + DELETE_FLAG=1.
	if (pkt.Exists(GA_T::DELETE_FLAG)) {
		if (friend_id) {
			Database::RemoveFriend(user_id_, static_cast<int64_t>(*friend_id));
			Logger::Log("friends", "[Friends] user=%lld removed friend_id=%u\n",
				(long long)user_id_, *friend_id);
		}
		return;
	}

	// Note edit: NOTES + FRIEND_PLAYER_ID (no PLAYER_NAME).
	if (auto notes = pkt.ReadString(GA_T::NOTES)) {
		if (friend_id) {
			Database::SetFriendNotes(user_id_, static_cast<int64_t>(*friend_id), *notes);
			Logger::Log("friends", "[Friends] user=%lld set notes for friend_id=%u\n",
				(long long)user_id_, *friend_id);
		}
		return;
	}

	// Add: PLAYER_NAME + IGNORE_FLAG ("y" = ignore list, "n" = friend —
	// bools ride the wire as 1-char strings; client accepts Y/y/T/t as true).
	auto name = pkt.ReadString(GA_T::PLAYER_NAME);
	if (!name || name->empty()) return;
	const std::string flag = pkt.ReadString(GA_T::IGNORE_FLAG).value_or("n");
	const bool ignore = !flag.empty() &&
		(flag[0] == 'y' || flag[0] == 'Y' || flag[0] == 't' || flag[0] == 'T');
	if (Database::AddFriend(user_id_, *name, ignore)) {
		Logger::Log("friends", "[Friends] user=%lld added '%s' ignore=%d\n",
			(long long)user_id_, name->c_str(), ignore ? 1 : 0);
	} else {
		Logger::Log("friends", "[Friends] user=%lld add failed, no such player '%s'\n",
			(long long)user_id_, name->c_str());
		ChatSession::SystemMessageTo(player_name, "Player '" + *name + "' not found.");
	}
}

void TcpSession::send_friends_list()
{
	if (user_id_ == 0) return;
	const auto rows = Database::GetFriendsForUser(user_id_);

	// Online decoration (map / class / profile) from the same snapshot
	// QUERY_PLAYERS uses. A friend absent from it is shown offline.
	const auto online_players = InstanceRegistry::GetActiveSearchablePlayers();
	std::vector<int64_t> online_ids;
	for (const auto& p : online_players) online_ids.push_back(p.character_id);
	const auto online_levels = PlayerSessionStore::GetCharacterLevels(online_ids);

	std::vector<uint8_t> data_rows;
	uint16_t row_count = 0;
	for (const auto& r : rows) {
		if (r.ignore_flag) {
			append(data_rows, 0x04, 0x00);  // 4 fields
			WriteString(data_rows, GA_T::IGNORE_FLAG, "y");
			WriteString(data_rows, GA_T::PLAYER_NAME, r.friend_name);
			WriteString(data_rows, GA_T::NOTES, r.notes);
			Write4B(data_rows, GA_T::FRIEND_PLAYER_ID, static_cast<uint32_t>(r.friend_user_id));
			++row_count;
			continue;
		}

		const InstanceRegistry::SearchablePlayerRow* online = nullptr;
		for (const auto& p : online_players) {
			if (p.player_name == r.friend_name) { online = &p; break; }
		}

		if (online) {
			uint32_t map_msg_id = 0;
			if (auto rec = MapGameInfo::LookupByName(online->map_name))
				map_msg_id = rec->friendly_name_msg_id;
			append(data_rows, 0x09, 0x00);  // 9 fields
			WriteString(data_rows, GA_T::IGNORE_FLAG, "n");
			WriteString(data_rows, GA_T::PLAYER_NAME, r.friend_name);
			WriteString(data_rows, GA_T::NOTES, r.notes);
			Write4B(data_rows, GA_T::FRIEND_PLAYER_ID, static_cast<uint32_t>(r.friend_user_id));
			Write1B(data_rows, GA_T::OFFLINE_FLAG, 0);
			Write4B(data_rows, GA_T::MAP_NAME_MSG_ID, map_msg_id);
			// CLASS_MSG_ID is a WCHAR_STR on the wire; the client converts.
			WriteString(data_rows, GA_T::CLASS_MSG_ID,
				std::to_string(ResolveClassMsgId(online->profile_id, 22976, "FRIENDS_LIST")));
			Write4B(data_rows, GA_T::PROFILE_ID, online->profile_id);
			// LEVEL presence is what routes the row into the client's online
			// list; the friend's real agent level.
			{
				auto lv = online_levels.find(online->character_id);
				Write4B(data_rows, GA_T::LEVEL, lv != online_levels.end() ? lv->second : 1u);
			}
		} else {
			append(data_rows, 0x05, 0x00);  // 5 fields, no LEVEL → offline list
			WriteString(data_rows, GA_T::IGNORE_FLAG, "n");
			WriteString(data_rows, GA_T::PLAYER_NAME, r.friend_name);
			WriteString(data_rows, GA_T::NOTES, r.notes);
			Write4B(data_rows, GA_T::FRIEND_PLAYER_ID, static_cast<uint32_t>(r.friend_user_id));
			Write1B(data_rows, GA_T::OFFLINE_FLAG, 1);
		}
		++row_count;
	}

	// Reply is packet type FRIEND_GET_LIST carrying DATA_SET_FRIENDS — the
	// client dispatches 0x1AE to its friends-store populate handler, fires
	// the F3 UI refresh event, then feeds the same packet to the chat-side
	// friend/ignore name map.
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::FRIEND_GET_LIST;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	append(response, GA_T::DATA_SET_FRIENDS & 0xFF, GA_T::DATA_SET_FRIENDS >> 8);
	append(response, row_count & 0xFF, (row_count >> 8) & 0xFF);
	WriteNBytesRaw(response, data_rows);

	send_response(response);
}

void TcpSession::send_start_listen_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_START_LISTEN;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_player_update_client_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::PLAYER_UPDATE_CLIENT;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::SYS_SITE_ID, 0x2);
	Write4B(response, GA_T::SYS_AVA_TEAMS_OK, 0x1);

	send_response(response);
}

// Optional service event "character_login" {user_id, char_id, name = account}: the
// decision may carry system mails {"mail":[{from, subject, body, credits,
// items:[{item_id, quantity}]}]} delivered before the inbox is read.
// No service = nothing happens.
static void deliver_login_mail(int64_t user_id, int64_t character_id, const std::string& name)
{
	if (!ServiceHooks::Enabled("character_login")) return;
	auto d = ServiceHooks::Call("character_login", {{"user_id", user_id}, {"char_id", character_id}, {"name", name}});
	if (!d || !d->contains("mail") || !(*d)["mail"].is_array()) return;
	int n = 0;
	for (const auto& m : (*d)["mail"]) {
		if (!m.is_object() || ++n > 5) break;
		std::vector<MailService::NewItem> items;
		if (m.contains("items") && m["items"].is_array())
			for (const auto& it : m["items"]) {
				if (!it.is_object()) continue;
				MailService::NewItem ni;
				ni.item_id  = (int)ServiceHooks::ClampInt(it, "item_id", 0, 0, 0x7fffffff);
				ni.quantity = ServiceHooks::ClampInt(it, "quantity", 1, 1, 1000);
				items.push_back(ni);
			}
		const int64_t id = MailService::DeliverSystem(character_id, ServiceHooks::ClampString(m, "from", 32),
			ServiceHooks::ClampString(m, "subject", MailService::MAX_SUBJECT),
			ServiceHooks::ClampString(m, "body", MailService::MAX_BODY),
			ServiceHooks::ClampInt(m, "credits", 0, 0, MailService::MAX_CREDITS), items);
		Logger::Log("mail", "[%s] character_login mail -> char %lld: %s (id %lld)\n", Logger::GetTime(),
			(long long)character_id, id ? "delivered" : "REFUSED", (long long)id);
	}
}

void TcpSession::send_update_new_mail_count_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::UPDATE_NEW_MAIL_COUNT;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::NEW_MAIL_COUNT, MailService::UnreadCount(selected_character_id_));

	send_response(response);
}

// ── mail ──────────────────────────────────────
// Client side decoded from GlobalAgenda.exe: replies to
// 0x169 (inbox) and 0x16A/0x16C (one mail, matched by MAIL_ID) are a DATA_SET
// 0x10C of mail rows + COUNT; UTgUIMailMenu re-renders from them.
// A mail / auction attachment as a bag record (component stacks get a synthetic id).
static InventoryRow MailItemRow(const MailService::MailItem& it)
{
	InventoryRow row{};
	row.inventory_id = it.inventory_id > 0 ? (int)it.inventory_id
	                                       : (int)(MailService::MAIL_COMPONENT_BASE + (uint32_t)it.id);
	row.device_id = it.device_id;
	row.quality   = it.quality;
	row.oc        = it.oc != 0;
	row.item_id   = it.item_id;
	for (size_t p = 0, q; p < it.mods.size(); p = q + 1) {
		q = it.mods.find(',', p);
		if (q == std::string::npos) q = it.mods.size();
		if (q > p) row.mods.push_back(std::atoi(it.mods.substr(p, q - p).c_str()));
	}
	return row;
}

void TcpSession::send_mail_rows(uint16_t packet_type, const std::vector<MailService::MailRow>& rows)
{
	std::vector<uint8_t> response;
	uint16_t item_count = 2;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::COUNT, (uint32_t)rows.size());
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, rows.size() & 0xFF, (rows.size() >> 8) & 0xFF);
	for (const auto& m : rows) {
		const bool has_items = !m.items.empty();
		append(response, has_items ? 0x0D : 0x0B, 0x00);  // 11 fields per mail (+2 item sets)
		Write4B(response, GA_T::MAIL_ID, (uint32_t)m.id);
		Write4B(response, GA_T::MAIL_TYPE_VALUE_ID, m.mail_type);
		Write4B(response, GA_T::STATE_VALUE_ID, m.state);
		Write4B(response, GA_T::SOURCE_VALUE_ID, m.source);
		Write4B(response, GA_T::FROM_PLAYER_ID, (uint32_t)m.from_character_id);
		Write4B(response, GA_T::CURRENCY_VALUE, (uint32_t)m.credits);  // DOUBLE_OR_INT -> 4-byte int
		WriteString(response, GA_T::NAME, m.source == MailService::SOURCE_SYSTEM
		                                  ? (m.from_name.empty() ? std::string("schlumpy.net") : m.from_name)
		                                  : m.source == MailService::SOURCE_RETURNED ? std::string("Mail Service")
		                                  : m.from_name);
		WriteString(response, GA_T::SUBJECT, m.subject);
		WriteString(response, GA_T::MESSAGE, m.body);
		WriteDouble(response, GA_T::SENT_DATETIME, (double)m.sent_at);
		WriteDouble(response, GA_T::EXPIRE_DATETIME, (double)m.expire_at);
		if (!has_items) continue;
		// Attachments (reader 0x11479500): DATA_SET_MAIL_ITEMS rows go through the
		// generic inventory-record parser (0x10a12340, same 13-field bag shape) +
		// INSTANCE_COUNT; DATA_SET_INVENTORY_STATE rows {INVENTORY_ID, EFFECT_GROUP_ID}
		// sit beside it on the mail row.
		std::vector<InventoryRow> inv;
		for (const auto& it : m.items) inv.push_back(MailItemRow(it));
		append(response, GA_T::DATA_SET_MAIL_ITEMS & 0xFF, GA_T::DATA_SET_MAIL_ITEMS >> 8);
		append(response, inv.size() & 0xFF, (inv.size() >> 8) & 0xFF);
		for (size_t i = 0; i < inv.size(); ++i)
			AppendBagRecord(response, inv[i], (uint32_t)m.items[i].quantity);
		size_t state_rows = 0;
		for (const auto& row : inv) state_rows += row.mods.empty() ? 1 : row.mods.size();
		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, state_rows & 0xFF, (state_rows >> 8) & 0xFF);
		for (const auto& row : inv) {
			std::vector<int> eg = row.mods;
			if (eg.empty()) eg.push_back(0);
			for (int e : eg) {
				append(response, 0x02, 0x00);
				Write4B(response, GA_T::INVENTORY_ID, (uint32_t)row.inventory_id);
				Write4B(response, GA_T::EFFECT_GROUP_ID, (uint32_t)e);
			}
		}
	}
	send_response(response);
}

// Error popup / live wallet restate go through the agent's instance (the
// client shows alerts from the game connection; credits are replicated there).
void TcpSession::send_mail_notify(uint32_t msg_id, bool restate_wallet)
{
	if (assigned_instance_id_ == 0 || (msg_id == 0 && !restate_wallet)) return;
	nlohmann::json j;
	j["type"]         = IpcProtocol::MSG_MAIL_NOTIFY;
	j["session_guid"] = session_guid_;
	j["character_id"] = selected_character_id_;
	j["msg_id"]       = msg_id;
	j["wallet"]       = restate_wallet;
	IpcServer::SendToInstance(assigned_instance_id_, j.dump());
}

void TcpSession::DeliverNewMail(int64_t to_character_id)
{
	std::vector<std::shared_ptr<TcpSession>> targets;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& kv : g_sessions_)
			if (auto s = kv.second.lock())
				if (s->selected_character_id_ == to_character_id) targets.push_back(s);
	}
	Logger::Log("mail", "[%s] [Mail] new mail -> char %lld: pushed to %zu online session(s)\n",
		Logger::GetTime(), (long long)to_character_id, targets.size());
	for (auto& s : targets) {
		auto self = s;
		asio::post(s->io_ctx_, [self]() {
			self->send_update_new_mail_count_response();
			self->send_mail_rows(GA_U::GET_CHARACTER_MAILS, MailService::List(self->selected_character_id_));
		});
	}
}

// Bag side of a mail send / accept: gear rows leave or join the client's bag,
// component stacks are restated in full (same pushes vendor sell / salvage use).
// No pawn yet (not spawned) -> the next spawn's full inventory carries it.
void TcpSession::push_mail_bag_changes(const std::vector<int64_t>& removed,
                                       const std::vector<int64_t>& added, bool components,
                                       const std::vector<int>& depleted)
{
	const int pawn = last_pawn_id_;
	const int64_t me = selected_character_id_;
	if (pawn == 0 || me <= 0) return;
	if (!removed.empty()) send_inventory_removed(pawn, me, removed);
	if (!added.empty())   send_inventory_added(pawn, me, added);
	if (components) {
		send_component_removals(pawn, depleted);
		send_component_refresh(pawn);
		sync_item_count(me);
	}
}

void TcpSession::handle_mail_packet(uint16_t packet_type, const PacketView& pkt)
{
	const int64_t me = selected_character_id_;
	if (me <= 0) return;
	const int64_t mail_id = (int64_t)pkt.Read4B(GA_T::MAIL_ID).value_or(0);

	switch (packet_type) {
	case GA_U::GET_CHARACTER_MAILS:
		send_mail_rows(GA_U::GET_CHARACTER_MAILS, MailService::List(me));
		break;
	case GA_U::READ_MAIL: {
		MailService::MailRow row;
		if (MailService::MarkRead(me, mail_id, &row)) send_mail_rows(GA_U::READ_MAIL, {row});
		send_update_new_mail_count_response();
		break;
	}
	case GA_U::ACCEPT_MAIL: {
		const auto r = MailService::Accept(me, mail_id);
		if (r.ok) {
			send_mail_rows(GA_U::ACCEPT_MAIL, {r.row});
			send_mail_notify(0, true);
			push_mail_bag_changes({}, r.gained_rows, r.components_changed);
			if (r.cod_paid_to_character) DeliverNewMail(r.cod_paid_to_character);
		} else {
			send_mail_notify(r.error_msg_id, false);
		}
		break;
	}
	case GA_U::DELETE_MAIL:
		// no client handler for 0x16B: answer with the refreshed inbox
		if (!MailService::Delete(me, mail_id)) send_mail_notify(MailService::MSG_ALREADY_DONE, false);
		send_mail_rows(GA_U::GET_CHARACTER_MAILS, MailService::List(me));
		break;
	case GA_U::DECLINE_COD_MAIL:
		if (const int64_t sender = MailService::DeclineCod(me, mail_id)) DeliverNewMail(sender);
		else send_mail_notify(MailService::MSG_ALREADY_DONE, false);
		send_mail_rows(GA_U::GET_CHARACTER_MAILS, MailService::List(me));
		break;
	case GA_U::SEND_MAIL: {
		// attachment rows {INVENTORY_ID, INSTANCE_COUNT, ITEM_ID}; component stacks
		// carry the synthetic 0x40000000+item id and the count to send
		std::vector<MailService::Attachment> attachments;
		for (const auto& rec : pkt.GetDataSet(GA_T::DATA_SET)) {
			MailService::Attachment a;
			a.inventory_id = (int64_t)rec.Read4B(GA_T::INVENTORY_ID).value_or(0);
			a.count        = (int64_t)rec.Read4B(GA_T::INSTANCE_COUNT).value_or(1);
			if (a.count <= 0) a.count = 1;
			attachments.push_back(a);
		}
		const auto r = MailService::Send(me,
			pkt.ReadString(GA_T::NAME).value_or(""),
			pkt.ReadString(GA_T::SUBJECT).value_or(""),
			pkt.ReadString(GA_T::MESSAGE).value_or(""),
			(int64_t)(int32_t)pkt.Read4B(GA_T::CURRENCY_VALUE).value_or(0),
			pkt.Read4B(GA_T::MAIL_TYPE_VALUE_ID).value_or(MailService::TYPE_NORMAL),
			attachments);
		if (!r.ok) {
			send_mail_notify(r.error_msg_id, false);
			break;
		}
		send_mail_notify(0, true);
		push_mail_bag_changes(r.removed_rows, {}, r.components_changed, r.depleted_components);
		DeliverNewMail(r.to_character_id);
		break;
	}
	}
}

// ── Auction House ─────────────────────────────
// Client side decoded from GlobalAgenda.exe: browse
// (0x15B) and My Auctions (0x158 sells / 0x159 bids) rows = the bag record +
// NAME_MSG_ID, BUYOUT_PRICE, START_PRICE, AUCTION_ITEM_ID, AUCTION_TIME_LEFT
// (minutes), BID_AMOUNT. Replies to add/bid/buyout/cancel carry ERROR_CODE (a
// msg id the client shows as an alert) or the refreshed My Auctions list.
void TcpSession::send_auction_rows(uint16_t packet_type, uint16_t data_set,
                                   const std::vector<AuctionService::Listing>& rows,
                                   uint32_t total, bool count_in_first_row)
{
	std::vector<uint8_t> response;
	uint16_t item_count = 3;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	Write4B(response, GA_T::COUNT, total);

	const int64_t now = (int64_t)std::time(nullptr);
	std::vector<InventoryRow> inv;
	append(response, data_set & 0xFF, data_set >> 8);
	append(response, rows.size() & 0xFF, (rows.size() >> 8) & 0xFF);
	for (size_t i = 0; i < rows.size(); ++i) {
		const auto& l = rows[i];
		InventoryRow row{};
		if (!l.items.empty()) row = MailItemRow(l.items[0]);
		else row.item_id = l.item_id;
		inv.push_back(row);
		const size_t start = response.size();
		AppendBagRecord(response, row, (uint32_t)l.quantity);
		const bool with_count = count_in_first_row && i == 0;
		response[start] = (uint8_t)(13 + 6 + (with_count ? 1 : 0));
		Write4B(response, GA_T::NAME_MSG_ID, (uint32_t)l.name_msg_id);
		// My Auctions "Buy" column = the unit price the seller typed
		Write4B(response, GA_T::BUYOUT_PRICE, (uint32_t)(data_set == GA_T::DATA_SET_CHAR_AUCTION_SELL_ITEMS && l.quantity > 0
			? l.buyout_price / l.quantity : l.buyout_price));
		Write4B(response, GA_T::START_PRICE, (uint32_t)l.start_price);
		Write4B(response, GA_T::AUCTION_ITEM_ID, (uint32_t)l.id);
		Write4B(response, GA_T::AUCTION_TIME_LEFT, (uint32_t)std::max<int64_t>(0, (l.expire_at - now + 59) / 60));
		Write4B(response, GA_T::BID_AMOUNT, (uint32_t)l.bid);
		if (with_count) Write4B(response, GA_T::COUNT, total);
	}
	// DATA_SET_INVENTORY_STATE beside the rows (same as mail)
	size_t state_rows = 0;
	for (const auto& row : inv) state_rows += row.mods.empty() ? 1 : row.mods.size();
	append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
	append(response, state_rows & 0xFF, (state_rows >> 8) & 0xFF);
	for (const auto& row : inv) {
		std::vector<int> eg = row.mods;
		if (eg.empty()) eg.push_back(0);
		for (int e : eg) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, (uint32_t)row.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, (uint32_t)e);
		}
	}
	send_response(response);
}

// Market view (every category but Special Auction, request USER_DEFINED_FLAG "y"): client reader
// 0x1142e280 takes Quantity from ITEM_COUNT and Unit Price from AVG_PRICE; one row per item + quality
// (+ unit price when "Lowest price items only" is off).
void TcpSession::send_auction_market_rows(uint16_t packet_type, const std::vector<AuctionService::MarketRow>& rows, uint32_t total)
{
	std::vector<uint8_t> response;
	uint16_t item_count = 3;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);
	Write4B(response, GA_T::COUNT, total);
	std::vector<InventoryRow> inv;
	append(response, GA_T::DATA_SET_AUCTION_BROWSE_ITEMS & 0xFF, GA_T::DATA_SET_AUCTION_BROWSE_ITEMS >> 8);
	append(response, rows.size() & 0xFF, (rows.size() >> 8) & 0xFF);
	for (size_t i = 0; i < rows.size(); ++i) {
		const auto& m = rows[i];
		InventoryRow row{};
		if (!m.items.empty()) row = MailItemRow(m.items[0]);
		row.item_id = m.item_id;
		row.quality = m.quality;
		inv.push_back(row);
		const size_t start = response.size();
		AppendBagRecord(response, row, (uint32_t)m.count);
		const bool with_count = i == 0;
		response[start] = (uint8_t)(13 + 3 + (with_count ? 1 : 0));
		Write4B(response, GA_T::NAME_MSG_ID, (uint32_t)m.name_msg_id);
		Write4B(response, GA_T::ITEM_COUNT, (uint32_t)m.count);
		Write4B(response, GA_T::AVG_PRICE, (uint32_t)m.unit);
		if (with_count) Write4B(response, GA_T::COUNT, total);
	}
	size_t state_rows = 0;
	for (const auto& row : inv) state_rows += row.mods.empty() ? 1 : row.mods.size();
	append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
	append(response, state_rows & 0xFF, (state_rows >> 8) & 0xFF);
	for (const auto& row : inv) {
		std::vector<int> eg = row.mods;
		if (eg.empty()) eg.push_back(0);
		for (int e : eg) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, (uint32_t)row.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, (uint32_t)e);
		}
	}
	send_response(response);
}

void TcpSession::send_auction_error(uint16_t packet_type, uint32_t msg_id)
{
	std::vector<uint8_t> response;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 0x01, 0x00);
	Write4B(response, GA_T::ERROR_CODE, msg_id);
	send_response(response);
}

void TcpSession::handle_auction_packet(uint16_t packet_type, const PacketView& pkt)
{
	const int64_t me = selected_character_id_;
	if (me <= 0) return;
	for (int64_t c : AuctionService::SettleExpired()) DeliverNewMail(c);

	auto my_sells = [&](uint16_t op) {
		const auto rows = AuctionService::MySells(me);
		send_auction_rows(op, GA_T::DATA_SET_CHAR_AUCTION_SELL_ITEMS, rows, (uint32_t)rows.size(), false);
	};
	auto my_bids = [&](uint16_t op) {
		const auto rows = AuctionService::MyBids(me);
		send_auction_rows(op, GA_T::DATA_SET_CHAR_AUCTION_BID_ITEMS, rows, (uint32_t)rows.size(), false);
	};
	auto mail_out = [&](const AuctionService::Result& r) {
		std::vector<int64_t> seen;
		for (int64_t c : r.mailed)
			if (std::find(seen.begin(), seen.end(), c) == seen.end()) { seen.push_back(c); DeliverNewMail(c); }
	};
	const int64_t auction_id = (int64_t)pkt.Read4B(GA_T::AUCTION_ITEM_ID).value_or(0);

	switch (packet_type) {
	case GA_U::GET_CHARACTER_AUCTION_SELL_ITEMS: my_sells(packet_type); break;
	case GA_U::GET_CHARACTER_AUCTION_BID_ITEMS:  my_bids(packet_type);  break;
	case GA_U::GET_FILTERED_AUCTION_ITEMS: {
		AuctionService::Filter f;
		for (const auto& rec : pkt.GetDataSet(GA_T::DATA_SET))
			if (const auto t = rec.Read4B(GA_T::ITEM_TYPE_VALUE_ID))
				f.types.emplace_back(*t, rec.Read4B(GA_T::ITEM_SUBTYPE_VALUE_ID).value_or(0));
		f.quality = pkt.Read4B(GA_T::QUALITY_VALUE_ID).value_or(0);
		f.name    = pkt.ReadString(GA_T::NAME).value_or("");
		f.start_row = (int)pkt.Read4B(GA_T::START_ROW_NUMBER).value_or(0);
		f.end_row   = (int)pkt.Read4B(GA_T::END_ROW_NUMBER).value_or(0);
		uint32_t total = 0;
		if (pkt.ReadString(GA_T::USER_DEFINED_FLAG).value_or("") == "y") {
			f.lowest = pkt.ReadString(GA_T::ACTIVE_FLAG).has_value();
			const auto market = AuctionService::BrowseMarket(f, &total);
			send_auction_market_rows(packet_type, market, total);
			break;
		}
		const auto rows = AuctionService::Browse(f, &total);
		send_auction_rows(packet_type, GA_T::DATA_SET_AUCTION_BROWSE_ITEMS, rows, total, true);
		break;
	}
	case GA_U::ADD_AUCTION_ITEM: {
		const auto r = AuctionService::Add(me,
			(int64_t)pkt.Read4B(GA_T::INVENTORY_ID).value_or(0),
			(int64_t)pkt.Read4B(GA_T::INSTANCE_COUNT).value_or(1),
			(int64_t)(int32_t)pkt.Read4B(GA_T::START_PRICE).value_or(0),
			(int64_t)(int32_t)pkt.Read4B(GA_T::BUYOUT_PRICE).value_or(0),
			pkt.Read4B(GA_T::DURATION_VALUE_ID).value_or(0));
		if (!r.ok) { send_auction_error(packet_type, r.error_msg_id); break; }
		send_mail_notify(0, true);
		push_mail_bag_changes(r.removed_rows, {}, r.components_changed, r.depleted_components);
		my_sells(packet_type);
		break;
	}
	case GA_U::BID_AUCTION_ITEM:
	case GA_U::BUYOUT_AUCTION_ITEM: {
		// market view Buy: no auction id, item + quality + count at the unit price shown
		const auto r = packet_type == GA_U::BUYOUT_AUCTION_ITEM && auction_id == 0
			? AuctionService::BuyMarket(me, (int)pkt.Read4B(GA_T::ITEM_ID).value_or(0),
				(int)pkt.Read4B(GA_T::QUALITY_VALUE_ID).value_or(0),
				(int64_t)pkt.Read4B(GA_T::ITEM_COUNT).value_or(1), (int64_t)pkt.Read4B(GA_T::AVG_PRICE).value_or(0))
			: packet_type == GA_U::BUYOUT_AUCTION_ITEM
			? AuctionService::Buyout(me, auction_id)
			: AuctionService::Bid(me, auction_id, (int64_t)(int32_t)pkt.Read4B(GA_T::BID_AMOUNT).value_or(0));
		if (!r.ok) { send_auction_error(packet_type, r.error_msg_id); break; }
		send_mail_notify(0, true);
		my_bids(packet_type);
		mail_out(r);
		break;
	}
	case GA_U::CANCEL_AUCTION_ITEM: {
		const auto r = AuctionService::Cancel(me, auction_id);
		if (!r.ok) { send_auction_error(packet_type, r.error_msg_id); break; }
		my_sells(packet_type);
		mail_out(r);
		break;
	}
	}
}

// AvA / Conquest campaign map (TgUICampaignMenu_HexMap). Opening the
// map sends HEX_GET_ALL_ZONES once (exe 0x10922b00: hex manager +0xd8 == null) and
// the scene stays empty until this reply arrives. Client parse 0x10926d30:
// DATA_SET_HEX_ZONES {ZONE_OBJECT_ID, HEX_ZONE_ID, HEX_SEASON_NBR, NAME_MSG_ID,
// DESC_MSG_ID}, DATA_SET_ZONE_REGIONS {ZONE_OBJECT_ID, HEX_ZONE_REGION_ID,
// NAME_MSG_ID}, DATA_SET_TERRITORY_RESOURCES {ZONE_OBJECT_ID, HEX_TERRITORY_ID,
// ITEM_ID, POSITION}; then UI event 0x43 -> UpdateZoneList. The zone layout lived
// on Hi-Rez's backend (not in the game files), so we serve one zone of our own.
// AvA campaign seasons. The client has no HEX_CAMPAIGN_STATE 0x13B sender or
// listener (0x13B pushes in the exe are asserts / DATA_SET_DEPLOYABLES), so the campaign state
// rides on fields it already reads: HEX_SEASON_NBR (zone list) + REMAINING_SECONDS (zone data).
// Ours: weekly seasons from Mon 2026-10-26 00:00 UTC; territories are NOT wiped at a season end
// (few players), standings are kept per season in ga_ava_seasons.
struct AvaSeason { uint32_t nbr; uint32_t remaining; };
static AvaSeason ava_season(int64_t now = (int64_t)time(nullptr))
{
	const int64_t kEpoch = AvaParity::kSeasonEpoch, kLength = AvaParity::SeasonLength();
	const int64_t t = now > kEpoch ? now - kEpoch : 0;
	return {(uint32_t)(t / kLength + 1), (uint32_t)(kLength - t % kLength)};
}

void TcpSession::send_hex_get_all_zones_response()
{
	const uint32_t kSeasonNbr = ava_season().nbr;
	const int zones = AvaZones::Count();

	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::HEX_GET_ALL_ZONES;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 0x03, 0x00);  // 3 data sets

	append(response, GA_T::DATA_SET_HEX_ZONES & 0xFF, GA_T::DATA_SET_HEX_ZONES >> 8);
	append(response, zones, 0x00);
	for (int z = 1; z <= zones; ++z) {
		const AvaZones::Zone* zone = AvaZones::Find((uint32_t)z);
		append(response, 0x05, 0x00);
		Write4B(response, GA_T::ZONE_OBJECT_ID, zone->id);
		Write4B(response, GA_T::HEX_ZONE_ID, zone->id);
		Write4B(response, GA_T::HEX_SEASON_NBR, kSeasonNbr);
		Write4B(response, GA_T::NAME_MSG_ID, zone->name_msg);
		Write4B(response, GA_T::DESC_MSG_ID, zone->desc_msg);
	}

	const int regions = AvaParity::RegionCount() * zones;
	append(response, GA_T::DATA_SET_ZONE_REGIONS & 0xFF, GA_T::DATA_SET_ZONE_REGIONS >> 8);
	append(response, regions & 0xFF, regions >> 8);  // region rows (texts 59671-59675 'Region 1-5')
	for (int z = 1; z <= zones; ++z)
		for (int r = 1; r <= AvaParity::RegionCount(); ++r) {
			append(response, 0x03, 0x00);
			Write4B(response, GA_T::ZONE_OBJECT_ID, (uint32_t)z);
			Write4B(response, GA_T::HEX_ZONE_REGION_ID, r);
			Write4B(response, GA_T::NAME_MSG_ID, 59670 + r);
		}
	append(response, GA_T::DATA_SET_TERRITORY_RESOURCES & 0xFF, GA_T::DATA_SET_TERRITORY_RESOURCES >> 8);
	append(response, 0x00, 0x00);

	send_response(response);
}

// HEX_GET_ZONE_DATA reply. Selecting a zone in the campaign map sends
// HEX_GET_ZONE_DATA {ZONE_OBJECT_ID} (exe 0x10922b20). Reply handler 0x10920b90: flat
// marshal, ZONE_OBJECT_ID must match the request and RAID_PHASE must be ABSENT (present =
// raid-zone path); zone parse 0x10927be0: DATA_SET_AGENCIES {AGENCY_ID, NAME, LINEAR_COLOR_R/G/B},
// ZONE_OBJECT_ID, HEX_ZONE_ID, WIDTH, HEIGHT (WIDTH*HEIGHT territory slots), NAME_MSG_ID,
// DESC_MSG_ID, ROTATION_REMAINING_SECONDS, REMAINING_SECONDS, SETTING_FLAGS, then
// DATA_SET_HEX_TERRITORIES rows placed by POSITION (row parse 0x10927770: HEX_BUILDING_ID,
// BUILDING_TYPE/SUBTYPE_VALUE_ID, OWNER_AGENCY_ID, HEX_ZONE_REGION_ID, ...); then UI event 0x42.
// Layout is ours (Hi-Rez backend data is gone): 7x5 grid, all territories unowned, buildings
// from asm_data_set_hex_buildings.
// AvA War Zone layout, shared by HEX_GET_ZONE_DATA and HEX_CAPTURE.
namespace hexzone {
// The original zones were big, tall hex maps with their own outlines (Steam news 2010: "Hex 219 added to Region 2 of NA
// Sovereign zone"; stats pages archived 2012: 10-13 columns x 25-32 half rows). Ours: every zone on a 12x30 grid with the
// outlines and regions of AvaZones/AvaBoards.hpp; all other positions are blank tiles. Layout rows are also written to
// ga_ava_layout for ava-bots.
constexpr uint32_t kWidth = AvaZones::kW, kHeight = AvaZones::kH;
constexpr uint32_t kTerritoryVisible = 0x02000000;  // territory SETTING_FLAGS bit 25
struct HexBuilding { uint32_t id, type, subtype; };
constexpr HexBuilding kUnusable  = {1, 679, 0};
constexpr HexBuilding kEmpty     = {18, 677, 0};     // "No Facility"
// The stats pages list every hex of the current zones as No Facility: a facility is always built by its owner.
inline HexBuilding LayoutOf(uint32_t zone, uint32_t local) { return AvaZones::Region(zone, local) ? kEmpty : kUnusable; }
inline HexBuilding LayoutAt(uint32_t g) { return LayoutOf(AvaZones::ZoneOf(g), AvaZones::Local(g)); }
// Hidden AvA queues (Database::Init seed, enabled=0 so they stay off the queue list),
// one per facility; each pool holds that facility's HEX_AVA_* maps. 0 = not attackable.
inline uint32_t QueueForBuilding(uint32_t building_id) {
	switch (building_id) {
		case 17: return 40;  // Lab      -> HEX_AVA_Lab1-3, Push_Lab1, 2pt_Theft_Lab1
		case 14: return 41;  // Factory  -> HEX_AVA_Factory1-3, Push_Factory1, 2pt_Theft_Factory1
		case 13: return 42;  // Mine     -> HEX_AVA_Plant/Plant2/Plant3 (TGA_Mine1-3 load screens)
		case 24: case 25: return 40;  // Tech III / Tech I Lab (built/upgraded)
		case 27: case 29: return 41;  // Tech III / Tech I Factory
		case 26: case 28: return 42;  // Tech III / Tech I Mine
		case 15: return 43;  // Missile  -> HEX_AVA_Missile1
		case 3:  return 44;  // Base/HQ  -> HEX_AVA_Defense1/2
		case 18: return 45;  // No Facility -> HEX_AVA_Ticket_Neutral
		default: return 0;   // Unusable / Mountain
	}
}
}  // namespace hexzone

// Facilities built on a "No Facility" hex or upgraded (HEX_ADD_FACILITY) replace
// the layout building while the builder still holds the hex; on capture they lapse (destroyed).
static std::map<uint32_t, hexzone::HexBuilding> ava_built(sqlite3* db)
{
	std::map<uint32_t, hexzone::HexBuilding> m;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT b.position, b.hex_building_id, h.building_type_value_id, h.building_subtype_value_id "
	        "FROM ga_ava_buildings b JOIN ga_ava_territories t ON t.zone_id = b.zone_id AND t.position = b.position "
	        "AND t.owner_agency_id = b.owner_agency_id JOIN asm_data_set_hex_buildings h ON h.hex_building_id = b.hex_building_id "
	        "WHERE b.zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW)
			m[(uint32_t)sqlite3_column_int(s, 0)] = {(uint32_t)sqlite3_column_int(s, 1),
				(uint32_t)sqlite3_column_int(s, 2), (uint32_t)sqlite3_column_int(s, 3)};
		sqlite3_finalize(s);
	}
	return m;
}
static hexzone::HexBuilding ava_building(sqlite3* db, uint32_t pos)
{
	const auto m = ava_built(db);
	const auto it = m.find(pos);
	return it != m.end() ? it->second : hexzone::LayoutAt(pos);
}
uint32_t AvaParityLayoutAt(uint32_t pos)  // the map's own building (no player facility)
{ return hexzone::LayoutAt(pos).id; }
uint32_t AvaParityBuildingAt(uint32_t pos)
{ return AvaZones::Valid(pos) ? ava_building(Database::GetConnection(), pos).id : 0; }

// Production modifiers of every held hex. hq/adj in percent, mod = size inefficiency (soft cap) as a factor.
struct AvaHexMods { float hq = 0, adj = 0, mod = 1; };
static std::map<uint32_t, AvaHexMods> ava_production_mods(sqlite3* db)
{
	std::map<uint32_t, AvaHexMods> out;
	std::map<uint32_t, int64_t> owner_of;
	std::map<int64_t, std::vector<uint32_t>> hqs;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT position, owner_agency_id FROM ga_ava_territories WHERE zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW) owner_of[(uint32_t)sqlite3_column_int(s, 0)] = sqlite3_column_int64(s, 1);
		sqlite3_finalize(s);
	}
	if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, position FROM ga_ava_hq WHERE zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW) hqs[sqlite3_column_int64(s, 0)].push_back((uint32_t)sqlite3_column_int(s, 1));
		sqlite3_finalize(s);
	}
	std::set<uint32_t> connected;  // reachable from a finished HQ over the owner's own hexes
	for (const auto& [owner, list] : hqs)
		for (uint32_t hq : list) {
			const auto own = owner_of.find(hq);
			if (own == owner_of.end() || own->second != owner || AvaParity::HqProgress(hq) < 100) continue;
			std::vector<uint32_t> todo{hq};
			while (!todo.empty()) {
				const uint32_t p = todo.back();
				todo.pop_back();
				if (!connected.insert(p).second) continue;
				for (int d = 0; d < 6; ++d) {
					const int n = AvaShields::Neighbour(p, d);
					const auto o = n >= 0 ? owner_of.find((uint32_t)n) : owner_of.end();
					if (o != owner_of.end() && o->second == owner) todo.push_back((uint32_t)n);
				}
			}
		}
	const auto built = ava_built(db);
	auto subtype = [&](uint32_t p) { const auto b = built.find(p); return b != built.end() ? b->second.subtype : hexzone::LayoutAt(p).subtype; };
	const auto soft = AvaParity::SoftCapFactors(db);
	for (const auto& [pos, owner] : owner_of) {
		if (!AvaZones::Valid(pos)) continue;
		AvaHexMods m;
		if (!connected.count(pos)) m.hq = -50.f;
		const uint32_t st = subtype(pos);
		if (st == 1320 || st == 1321 || st == 1322)
			for (int d = 0; d < 6 && m.adj < 30.f; ++d) {
				const int n = AvaShields::Neighbour(pos, d);
				const auto o = n >= 0 ? owner_of.find((uint32_t)n) : owner_of.end();
				if (o != owner_of.end() && o->second == owner && subtype((uint32_t)n) == st) m.adj += 5.f;
			}
		if (const auto f = soft.find(owner); f != soft.end()) m.mod = (float)f->second;
		out[pos] = m;
	}
	return out;
}

// AvA facility production. Each owned territory accrues its building's
// asm_data_set_hex_buildings.output_value per hour since MAX(captured_at, settled_at):
// Mine (subtype 1322) -> resources, Factory (1320) -> production, Lab (1321) -> research
// (Tech I/II/III Mine 125/300/720, Factory 25/50/100, Lab 10/20/30). At most 7 days per settle.
void PlayerSessionStore::SettleAvaProduction()
{
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	const int64_t now = (int64_t)std::time(nullptr);
	std::map<uint32_t, std::pair<uint32_t, double>> out;  // hex building -> (subtype, output/h)
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT hex_building_id, building_subtype_value_id, output_value "
	        "FROM asm_data_set_hex_buildings", -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW)
			out[(uint32_t)sqlite3_column_int(s, 0)] = {(uint32_t)sqlite3_column_int(s, 1), sqlite3_column_double(s, 2)};
		sqlite3_finalize(s);
	}
	struct Add { double res = 0, prod = 0, sci = 0; };
	std::map<int64_t, Add> add;
	// A Mine with an assigned resource item (HEX_ASSIGN_RESOURCE_ITEM) mines
	// output_value / production_cost units of that item per hour into ga_ava_resources instead.
	struct MineRes { int64_t owner; uint32_t item; double cost; };
	std::map<uint32_t, MineRes> mine_res;
	std::map<std::pair<int64_t, uint32_t>, double> mined;
	if (sqlite3_prepare_v2(db, "SELECT r.position, r.owner_agency_id, r.item_id, i.production_cost FROM ga_ava_mine_resources r "
	        "JOIN asm_data_set_items i ON i.item_id = r.item_id WHERE r.zone_id = 1 AND i.production_cost > 0",
	        -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW)
			mine_res[(uint32_t)sqlite3_column_int(s, 0)] = {sqlite3_column_int64(s, 1),
				(uint32_t)sqlite3_column_int(s, 2), sqlite3_column_double(s, 3)};
		sqlite3_finalize(s);
	}
	const auto built = ava_built(db);
	const auto mods = ava_production_mods(db);
	if (sqlite3_prepare_v2(db, "SELECT position, owner_agency_id, MAX(captured_at, settled_at) "
	        "FROM ga_ava_territories WHERE zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW) {
			const uint32_t pos = (uint32_t)sqlite3_column_int(s, 0);
			if (!AvaZones::Valid(pos)) continue;
			const auto bt = built.find(pos);
			const auto it = out.find(bt != built.end() ? bt->second.id : hexzone::LayoutAt(pos).id);
			if (it == out.end() || it->second.second <= 0) continue;
			double hours = std::min<double>(std::max<int64_t>(0, now - sqlite3_column_int64(s, 2)), 7 * 86400) / 3600.0;
			if (const auto m = mods.find(pos); m != mods.end()) hours *= 1.0 + (m->second.hq + m->second.adj) / 100.0;  // HQ penalty + adjacency
			const int64_t owner = sqlite3_column_int64(s, 1);
			Add& a = add[owner];
			const auto mr = mine_res.find(pos);
			if (it->second.first == 1322 && mr != mine_res.end() && mr->second.owner == owner) {
				mined[{owner, mr->second.item}] += hours * it->second.second / mr->second.cost;
				continue;
			}
			switch (it->second.first) {
				case 1322: a.res  += hours * it->second.second; break;
				case 1320: a.prod += hours * it->second.second; break;
				case 1321: a.sci  += hours * it->second.second; break;
			}
		}
		sqlite3_finalize(s);
	}
	for (const auto& [o, fac] : AvaParity::SoftCapFactors(db)) {  // soft cap (client msg 59925)
		if (auto it = add.find(o); it != add.end()) { it->second.res *= fac; it->second.prod *= fac; it->second.sci *= fac; }
		for (auto& [key, units] : mined) if (key.first == o) units *= fac;
	}
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
	for (const auto& [owner, a] : add) {
		if (sqlite3_prepare_v2(db,
		        "INSERT INTO ga_ava_stock (owner_agency_id, resources, production, research) VALUES (?1, ?2, ?3, ?4) "
		        "ON CONFLICT(owner_agency_id) DO UPDATE SET resources = resources + ?2, "
		        "production = production + ?3, research = research + ?4", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_double(s, 2, a.res);
			sqlite3_bind_double(s, 3, a.prod);
			sqlite3_bind_double(s, 4, a.sci);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	}
	for (const auto& [key, units] : mined) {
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_resources (owner_agency_id, item_id, amount) VALUES (?1, ?2, ?3) "
		        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET amount = amount + ?3", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, key.first);
			sqlite3_bind_int(s, 2, (int)key.second);
			sqlite3_bind_double(s, 3, units);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	}
	if (sqlite3_prepare_v2(db, "UPDATE ga_ava_territories SET settled_at = MAX(settled_at, ?1) WHERE zone_id = 1",  // theft pause
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, now);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	AvaParity::LabBlueprints(db);  // lab blueprints
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
}

// Chat line on opening the AvA map: the stockpile of this agent's agency (or solo owner).
void TcpSession::send_ava_stock_line()
{
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty()) return;
	const int64_t stock_now = (int64_t)std::time(nullptr);  // the map asks for zone data on every tab: one line per 10 minutes
	if (stock_now - ava_stock_line_at_ < 600) return;
	ava_stock_line_at_ = stock_now;
	PlayerSessionStore::SettleAvaProduction();
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT t.owner_name, COUNT(*), k.resources, k.production, k.research, t.owner_agency_id "
	        "FROM ga_characters c LEFT JOIN ga_agency_members m ON m.user_id = c.user_id "
	        "JOIN ga_ava_territories t ON t.zone_id = 1 "
	        " AND (t.owner_agency_id = ?2 + c.id OR t.owner_agency_id = m.agency_id) "
	        "LEFT JOIN ga_ava_stock k ON k.owner_agency_id = t.owner_agency_id "
	        "WHERE c.id = ?1 GROUP BY t.owner_agency_id", -1, &s, nullptr) != SQLITE_OK) return;
	sqlite3_bind_int64(s, 1, selected_character_id_);
	sqlite3_bind_int64(s, 2, 0x40000000);
	if (sqlite3_step(s) == SQLITE_ROW) {
		const char* n = (const char*)sqlite3_column_text(s, 0);
		char line[256];
		snprintf(line, sizeof line, "AvA stockpile (%s, %d territories): %.0f resources, %.0f production, %.0f research",
			n ? n : "?", sqlite3_column_int(s, 1), sqlite3_column_double(s, 2),
			sqlite3_column_double(s, 3), sqlite3_column_double(s, 4));
		ChatSession::SystemMessageTo(player_name, line);
		// HQ (only while still held) + mined resource items.
		std::string extra;
		sqlite3_stmt* q = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT h.position FROM ga_ava_hq h JOIN ga_ava_territories t ON t.zone_id = h.zone_id "
		        "AND t.position = h.position AND t.owner_agency_id = h.owner_agency_id "
		        "WHERE h.owner_agency_id = ?1",
		        -1, &q, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(q, 1, sqlite3_column_int64(s, 5));
			if (sqlite3_step(q) == SQLITE_ROW) extra = "HQ: territory " + AvaZones::Label((uint32_t)sqlite3_column_int(q, 0));
			sqlite3_finalize(q);
		}
		if (sqlite3_prepare_v2(db, "SELECT COALESCE(m.message, 'item ' || r.item_id), r.amount FROM ga_ava_resources r "
		        "LEFT JOIN asm_data_set_items i ON i.item_id = r.item_id "
		        "LEFT JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id "
		        "WHERE r.owner_agency_id = ?1 "
		        "AND r.amount >= 0.01 ORDER BY r.amount DESC LIMIT 6", -1, &q, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(q, 1, sqlite3_column_int64(s, 5));
			while (sqlite3_step(q) == SQLITE_ROW) {
				char part[96];
				snprintf(part, sizeof part, "%s%.1f %s", extra.empty() ? "" : ", ", sqlite3_column_double(q, 1),
					(const char*)sqlite3_column_text(q, 0));
				extra += part;
			}
			sqlite3_finalize(q);
		}
		if (!extra.empty()) ChatSession::SystemMessageTo(player_name, "AvA: " + extra);
	}
	sqlite3_finalize(s);
	send_ava_campaign_line();
}

// Standings snapshot of `season` from the live territories (last write before the boundary = final).
static void ava_snapshot_standings(sqlite3* db, uint32_t season)
{
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "DELETE FROM ga_ava_seasons WHERE season = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)season);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_seasons (season, owner_agency_id, owner_name, territories, zone) "
	        "SELECT ?1, owner_agency_id, MAX(owner_name), COUNT(*), MIN(position) / 1000 + 1 FROM ga_ava_territories WHERE zone_id = 1 "
	        "GROUP BY owner_agency_id", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)season);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	AvaParity::SnapshotNetworth(db, season);  // networth
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
}

// The AvA map only asks for its zone when opened; these push the zone again to every agent who has looked at one this login.
void TcpSession::AvaPushZoneToViewers(uint32_t zone, const std::string& except)
{
	std::vector<std::shared_ptr<TcpSession>> list;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_)
			if (auto s = weak.lock()) if (s->ava_zone_asked_ && guid != except && (!zone || s->ava_zone_asked_ == zone)) list.push_back(s);
	}
	for (auto& s : list) s->send_hex_get_zone_data_response(s->ava_zone_asked_);
}
void TcpSession::AvaRegionTick(int64_t now)
{
	static int last = 0;
	AvaParity::ResolveBidWindows(now);  // pushes the zone when a window closed
	const int open = AvaParity::OpenRegion(now, nullptr);
	if (last && open != last) AvaPushZoneToViewers(0);
	last = open;
}
void TcpSession::AvaRestateBags()
{
	std::vector<std::shared_ptr<TcpSession>> list;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) if (auto s = weak.lock()) if (s->selected_character_id_) list.push_back(s);
	}
	for (auto& s : list) {
		s->send_inventory_clear(s->last_pawn_id_, s->selected_character_id_);
		s->send_inventory_response(s->last_pawn_id_, s->selected_character_id_);
		s->sync_item_count(s->selected_character_id_);
	}
}

// AvA season end. Called every few minutes: refreshes the
// current season's standings snapshot, then pays every finished, unpaid season once:
// owners ranked 1/2/3 by territories (ties: lower owner id) pay season_rank1/2/3 AP to
// each ACTIVE member (an agent of the account played any instance during that season).
// Solo owners (0x40000000 + char id) are a one-agent agency. Not scaled, not capped.
int TcpSession::AvaSeasonTick(int64_t now_ts)
{
	AvaParity::ResolveBidWindows(now_ts);  // resolve (season tick)
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	const int64_t kEpoch = AvaParity::kSeasonEpoch, kLength = AvaParity::SeasonLength();
	constexpr int64_t kSoloOwnerBase = 0x40000000;
	const AvaSeason now = ava_season(now_ts);
	// A longer season length renumbers seasons - drop snapshots of seasons that have not started yet
	sqlite3_exec(db, ("DELETE FROM ga_ava_seasons WHERE season > " + std::to_string(now.nbr) + " AND season NOT IN (SELECT season FROM ga_ava_season_payouts)").c_str(), nullptr, nullptr, nullptr);
	ava_snapshot_standings(db, now.nbr);
	const ApRewards::Settings& cfg = ApRewards::Get();
	const int64_t pay[3] = {cfg.season_rank1, cfg.season_rank2, cfg.season_rank3};
	std::vector<int> seasons;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT DISTINCT season FROM ga_ava_seasons WHERE season < ?1 "
	        "AND season NOT IN (SELECT season FROM ga_ava_season_payouts) ORDER BY season",
	        -1, &s, nullptr) != SQLITE_OK) return 0;
	sqlite3_bind_int(s, 1, (int)now.nbr);
	while (sqlite3_step(s) == SQLITE_ROW) seasons.push_back(sqlite3_column_int(s, 0));
	sqlite3_finalize(s);
	int paid_total = 0;
	for (int season : seasons) {
		// Claim first: a crash mid-payout never pays a season twice.
		if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO ga_ava_season_payouts (season, paid_at) VALUES (?1, ?2)",
		        -1, &s, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int(s, 1, season);
		sqlite3_bind_int64(s, 2, now_ts);
		sqlite3_step(s);
		sqlite3_finalize(s);
		if (sqlite3_changes(db) == 0) continue;
		const int64_t from = kEpoch + (int64_t)(season - 1) * kLength, to = from + kLength;
		std::vector<std::pair<int64_t, std::string>> top;  // best first within each zone
		std::vector<size_t> ranks;
		if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, owner_name, zone FROM ga_ava_seasons WHERE season = ?1 "
		        "AND territories > 0 ORDER BY zone, networth DESC, territories DESC, owner_agency_id", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, season);
			int zone = -1;
			size_t in_zone = 0;
			while (sqlite3_step(s) == SQLITE_ROW) {
				const char* n = (const char*)sqlite3_column_text(s, 1);
				if (sqlite3_column_int(s, 2) != zone) { zone = sqlite3_column_int(s, 2); in_zone = 0; }
				if (in_zone < 3) { top.emplace_back(sqlite3_column_int64(s, 0), n ? n : "?"); ranks.push_back(in_zone); }
				++in_zone;
			}
			sqlite3_finalize(s);
		}
		int agents = 0;
		for (size_t i = 0; i < top.size(); ++i) {
			const int64_t owner = top[i].first;
			const size_t rank = ranks[i];
			// Active members: per account, the agent with the latest instance join in the season.
			const char* sql = owner >= kSoloOwnerBase
				? "SELECT ip.character_id, MAX(ip.joined_at) FROM ga_instance_players ip "
				  "WHERE ip.character_id = ?1 - ?4 AND ip.joined_at >= ?2 AND ip.joined_at < ?3"
				: "SELECT c.id, MAX(ip.joined_at) FROM ga_agency_members m "
				  "JOIN ga_characters c ON c.user_id = m.user_id "
				  "JOIN ga_instance_players ip ON ip.character_id = c.id "
				  "WHERE m.agency_id = ?1 AND ip.joined_at >= ?2 AND ip.joined_at < ?3 AND m.joined_at <= ?3 - ?5 GROUP BY m.user_id";  // 7-day rule
			std::vector<int64_t> chars;
			if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) continue;
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_int64(s, 2, from);
			sqlite3_bind_int64(s, 3, to);
			if (owner >= kSoloOwnerBase) sqlite3_bind_int64(s, 4, kSoloOwnerBase);
			else sqlite3_bind_int64(s, 5, AvaParity::SeasonMemberSeconds());
			while (sqlite3_step(s) == SQLITE_ROW)
				if (sqlite3_column_type(s, 0) != SQLITE_NULL) chars.push_back(sqlite3_column_int64(s, 0));
			sqlite3_finalize(s);
			for (int64_t cid : chars) {
				const int64_t ap = ApRewards::Grant(cid, PlayerSessionStore::GetCharacterLevelXp(cid).level,
					"ava_season", pay[rank], false, false);
				if (ap > 0) ++agents;
				{ std::vector<std::pair<int64_t, int>> un; for (const auto& u : AchievementService::CreditAvaSeason(cid, (int)rank)) un.emplace_back(u.character_id, u.achievement_id);
				  if (!un.empty()) PushAchievementUnlocks(un); }  // AvA season achievements
			}
			Logger::Log("rewards", "[AP] AvA season %d rank %zu %s: %zu active agents x %lld AP\n",
				season, rank + 1, top[i].second.c_str(), chars.size(), (long long)pay[rank]);
		}
		AvaParity::GrantSeasonItems(season);  // season end items
		PlayerSessionStore::SettleAvaProduction();
		AvaParity::SeasonEnd(season, from, to);  // tokens + zone wipe
		AvaRestateBags();
		AvaPushZoneToViewers(0);
		if (sqlite3_prepare_v2(db, "UPDATE ga_ava_season_payouts SET agents = ?2 WHERE season = ?1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, season);
			sqlite3_bind_int(s, 2, agents);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		paid_total += agents;
	}
	return paid_total;
}

// Campaign season standings + timer line on opening the AvA map.
void TcpSession::send_ava_campaign_line()
{
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty()) return;
	const AvaSeason now = ava_season();
	ava_snapshot_standings(db, now.nbr);
	sqlite3_stmt* s = nullptr;
	auto leader = [&](uint32_t season) {
		std::string r;
		if (sqlite3_prepare_v2(db, "SELECT owner_name, territories FROM ga_ava_seasons WHERE season = ?1 AND zone = ?2 "
		        "ORDER BY networth DESC, territories DESC, owner_agency_id LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)season);
			sqlite3_bind_int(s, 2, (int)AvaZones::ViewOf(session_guid_));
			if (sqlite3_step(s) == SQLITE_ROW) {
				const char* n = (const char*)sqlite3_column_text(s, 0);
				r = std::string(n ? n : "?") + " (" + std::to_string(sqlite3_column_int(s, 1)) + " territories)";
			}
			sqlite3_finalize(s);
		}
		return r;
	};
	char line[256];
	const std::string lead = leader(now.nbr), last = now.nbr > 1 ? leader(now.nbr - 1) : "";
	snprintf(line, sizeof line, "AvA campaign season %u: %ud %uh left. Leader: %s.%s%s", now.nbr,
		now.remaining / 86400, now.remaining % 86400 / 3600, lead.empty() ? "nobody yet" : lead.c_str(),
		last.empty() ? "" : " Last season: ", last.c_str());
	ChatSession::SystemMessageTo(player_name, line);
}

// AvA owner + hex building subtype of a held territory (0 = not held).
static std::pair<int64_t, int> ava_territory_owner(sqlite3* db, uint32_t nPosition)
{
	std::pair<int64_t, int> r{0, 0};
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT t.owner_agency_id, b.building_subtype_value_id FROM ga_ava_territories t "
	        "LEFT JOIN asm_data_set_hex_buildings b ON b.hex_building_id = ?2 "
	        "WHERE t.zone_id = 1 AND t.position = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		sqlite3_bind_int(s, 2, (int)ava_building(db, nPosition).id);
		if (sqlite3_step(s) == SQLITE_ROW) r = {sqlite3_column_int64(s, 0), sqlite3_column_int(s, 1)};
		sqlite3_finalize(s);
	}
	return r;
}

// HEX_ASSIGN_RESOURCE_ITEM 0x18F: pick which AvA resource (item subtype 1347 "AvA Resources":
// Class 1-3 Ore, Unpentium, Neptunium, Therium, Elements, Power Cores, Foundations; the base
// items, not the per-region copies >= 6564) a held Mine extracts. ITEM_ID 0 = back to the
// generic resources pool. Settles first so past hours count under the old choice.
static std::string ava_assign_resource_core(sqlite3* db, int64_t want_owner, uint32_t nPosition, uint32_t nItemId);
void TcpSession::handle_hex_assign_resource(uint32_t nPosition, uint32_t nItemId)
{
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty()) return;
	if (!AvaZones::Valid(nPosition) || !owns_ava_territory(nPosition)) {
		send_team_system_message(17494, "");  // "Territory is not controlled by your agency."
		return;
	}
	ChatSession::SystemMessageTo(player_name, ava_assign_resource_core(db, ava_territory_owner(db, nPosition).first, nPosition, nItemId));
	send_hex_get_zone_data_response(AvaZones::ZoneOf(nPosition));
}
// Session-free part (also driven by the service-agents admin op "assign").
static std::string ava_assign_resource_core(sqlite3* db, int64_t want_owner, uint32_t nPosition, uint32_t nItemId)
{
	if (!AvaZones::Valid(nPosition)) return "AvA: bad territory.";
	const auto [owner, subtype] = ava_territory_owner(db, nPosition);
	if (owner == 0 || owner != want_owner) return "Territory is not controlled by your agency.";
	if (subtype == 1321) {  // Lab: research this blueprint (Add Task), 0 = back to rolling by Lab tier
		sqlite3_stmt* q = nullptr;
		int need = 0;
		int have = 0;
		std::string bp;
		if (nItemId != 0 && sqlite3_prepare_v2(db, "SELECT MIN(l.loot_table_id) - 152, COALESCE(m.message, ''), "
		        "(SELECT tech_level_value_id - 1334 FROM asm_data_set_hex_buildings WHERE hex_building_id = ?2) FROM asm_data_set_loot_table_items l "
		        "JOIN asm_data_set_items i ON i.item_id = l.item_id LEFT JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id "
		        "WHERE l.item_id = ?1 AND l.loot_table_id IN (153, 154, 155) AND i.item_type_value_id = 686", -1, &q, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(q, 1, (int)nItemId);
			sqlite3_bind_int(q, 2, (int)ava_building(db, nPosition).id);
			if (sqlite3_step(q) == SQLITE_ROW) {
				need = sqlite3_column_int(q, 0);
				const char* c = (const char*)sqlite3_column_text(q, 1);
				bp = c ? c : "";
				have = sqlite3_column_int(q, 2);
			}
			sqlite3_finalize(q);
		}
		if (nItemId != 0 && need == 0) return "AvA: a Lab cannot research that.";
		if (nItemId != 0 && need > have) return "AvA: this Lab's tech level is too low for that blueprint.";
		PlayerSessionStore::SettleAvaProduction();
		if (sqlite3_prepare_v2(db, nItemId ? "INSERT OR REPLACE INTO ga_ava_mine_resources (zone_id, position, owner_agency_id, item_id) VALUES (1, ?1, ?2, ?3)"
		        : "DELETE FROM ga_ava_mine_resources WHERE zone_id = 1 AND position = ?1 AND ?2 = ?2 AND ?3 = ?3", -1, &q, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(q, 1, (int)nPosition);
			sqlite3_bind_int64(q, 2, owner);
			sqlite3_bind_int(q, 3, (int)nItemId);
			sqlite3_step(q);
			sqlite3_finalize(q);
		}
		return nItemId ? "AvA Lab (territory " + std::to_string(AvaZones::Local(nPosition)) + ") now researches " + bp + "."
			: std::string("AvA Lab: back to researching by tech level.");
	}
	if (subtype != 1322) {
		return "AvA: resources can only be assigned to a Mine you hold.";
	}
	std::string name;
	double cost = 0;
	sqlite3_stmt* s = nullptr;
	if (nItemId != 0 && sqlite3_prepare_v2(db, "SELECT i.production_cost, COALESCE(m.message, '') FROM asm_data_set_items i "
	        "LEFT JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id "
	        "WHERE i.item_id = ?1 AND i.item_subtype_value_id = 1347 AND i.item_id < 6564 AND i.production_cost > 0",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nItemId);
		if (sqlite3_step(s) == SQLITE_ROW) {
			cost = sqlite3_column_double(s, 0);
			const char* t = (const char*)sqlite3_column_text(s, 1);
			name = t ? t : "";
		}
		sqlite3_finalize(s);
	}
	if (nItemId != 0 && cost <= 0) {
		return "AvA: that is not a resource a Mine can extract.";
	}
	PlayerSessionStore::SettleAvaProduction();
	if (nItemId == 0) {
		if (sqlite3_prepare_v2(db, "DELETE FROM ga_ava_mine_resources WHERE zone_id = 1 AND position = ?1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)nPosition);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		return "AvA Mine: back to the general resources stockpile.";
	}
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_mine_resources (zone_id, position, owner_agency_id, item_id) "
	        "VALUES (1, ?1, ?2, ?3)", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		sqlite3_bind_int64(s, 2, owner);
		sqlite3_bind_int(s, 3, (int)nItemId);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	double outh = 0;
	if (sqlite3_prepare_v2(db, "SELECT output_value FROM asm_data_set_hex_buildings WHERE hex_building_id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)ava_building(db, nPosition).id);
		if (sqlite3_step(s) == SQLITE_ROW) outh = sqlite3_column_double(s, 0);
		sqlite3_finalize(s);
	}
	char line[200];
	snprintf(line, sizeof line, "AvA Mine (territory %u) now extracts %s: %.2f per hour.",
		AvaZones::Local(nPosition), name.empty() ? "that resource" : name.c_str(), outh / cost);
	return line;
}

// HEX_ASSIGN_HEADQUARTERS 0x190: the agency (or solo owner) names one held territory its HQ;
// a new choice moves it. The HQ lapses when the territory is lost (checked on read).
void TcpSession::handle_hex_assign_headquarters(uint32_t nPosition)
{
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty()) return;
	if (!AvaZones::Valid(nPosition) || !owns_ava_territory(nPosition)) {
		send_team_system_message(17494, "");  // "Territory is not controlled by your agency."
		return;
	}
	const int64_t owner = ava_territory_owner(db, nPosition).first;
	if (owner == 0) return;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_hq (owner_agency_id, zone_id, position, set_at) "
	        "VALUES (?1, 1, ?2, ?3)", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		sqlite3_bind_int(s, 2, (int)nPosition);
		sqlite3_bind_int64(s, 3, (int64_t)std::time(nullptr));
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	char line[120];
	snprintf(line, sizeof line, "AvA: territory %u is now your headquarters.", AvaZones::Local(nPosition));
	ChatSession::SystemMessageTo(player_name, line);
}

// HEX_ADD_FACILITY 0x18C (rules are ours; the game files carry no build prices).
// INVENTORY_ID = a hex building id (Mine/Factory/Lab, any tech) or an AvA resource item 5106-5110.
// Held "No Facility" hex + a building id: build that type at Tech I for 1 Tech I Power Core (5106).
// Held Mine/Factory/Lab: upgrade one tech level - I->II for 1 Tech II Foundation (5107) + 1 Tech II
// Power Core (5108), II->III for 1 Tech III Foundation (5109) + 1 Tech III Power Core (5110), paid
// from ga_ava_resources (mined via HEX_ASSIGN_RESOURCE_ITEM). Stored in ga_ava_buildings; lapses on capture.
static std::string ava_add_facility_core(sqlite3* db, int64_t want_owner, uint32_t nPosition, uint32_t nId);
void TcpSession::handle_hex_add_facility(uint32_t nPosition, uint32_t nId)
{
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty()) return;
	if (const auto mc = AvaShields::AddFacility(selected_character_id_, nPosition, nId); mc.handled) {  // Missile Complex deed
		if (mc.msg_id) send_team_system_message(mc.msg_id, "");
		if (!mc.text.empty()) ChatSession::SystemMessageTo(player_name, mc.text);
		if (mc.zone_changed) {  // panel and bag show the new state right away
			send_hex_get_zone_data_response(AvaZones::ZoneOf(nPosition));
			send_inventory_clear(last_pawn_id_, selected_character_id_);
			send_inventory_response(last_pawn_id_, selected_character_id_);
			sync_item_count(selected_character_id_);
		}
		return;
	}
	if (!AvaZones::Valid(nPosition) || !owns_ava_territory(nPosition)) {
		send_team_system_message(17494, "");  // "Territory is not controlled by your agency."
		return;
	}
	ChatSession::SystemMessageTo(player_name, ava_add_facility_core(db, ava_territory_owner(db, nPosition).first, nPosition, nId));
	send_hex_get_zone_data_response(AvaZones::ZoneOf(nPosition));
}
// Session-free part (also driven by the service-agents admin op "facility").
static std::string ava_add_facility_core(sqlite3* db, int64_t want_owner, uint32_t nPosition, uint32_t nId)
{
	if (!AvaZones::Valid(nPosition)) return "AvA: bad territory.";
	const int64_t owner = ava_territory_owner(db, nPosition).first;
	if (owner == 0 || owner != want_owner) return "Territory is not controlled by your agency.";
	struct Ladder { uint32_t subtype; uint32_t ids[3]; };
	static const Ladder kLadders[] = {{1322, {28, 13, 26}}, {1320, {29, 14, 27}}, {1321, {25, 17, 24}}};
	const Ladder* req = nullptr;
	for (const auto& l : kLadders)
		for (uint32_t id : l.ids) if (id == nId) req = &l;
	if (!req && nId != 0 && (nId < 5106 || nId > 5110)) {
		return "AvA: that cannot be built on a territory.";
	}
	PlayerSessionStore::SettleAvaProduction();
	const hexzone::HexBuilding cur = ava_building(db, nPosition);
	uint32_t target = 0;
	std::vector<uint32_t> cost;
	if (cur.id == hexzone::kEmpty.id) {
		if (!req) {
			return "AvA: pick a Mine, Factory or Lab to build on this territory.";
		}
		target = req->ids[0];
		cost = {5106};
	} else {
		const Ladder* lad = nullptr;
		int tier = -1;
		for (const auto& l : kLadders)
			for (int i = 0; i < 3; ++i) if (l.ids[i] == cur.id) { lad = &l; tier = i; }
		if (!lad) {
			return "AvA: no facility can be built or upgraded on this territory.";
		}
		if (req && req != lad) {
			return "AvA: this territory already has a facility (one per territory).";
		}
		if (tier == 2) {
			return "AvA: this facility is already Tech III.";
		}
		target = lad->ids[tier + 1];
		cost = tier == 0 ? std::vector<uint32_t>{5107, 5108} : std::vector<uint32_t>{5109, 5110};
	}
	sqlite3_stmt* s = nullptr;
	auto resname = [&](uint32_t item) {
		std::string n;
		if (sqlite3_prepare_v2(db, "SELECT COALESCE(m.message, '') FROM asm_data_set_items i "
		        "LEFT JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id WHERE i.item_id = ?1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)item);
			if (sqlite3_step(s) == SQLITE_ROW) { const char* c = (const char*)sqlite3_column_text(s, 0); n = c ? c : ""; }
			sqlite3_finalize(s);
		}
		return n.empty() ? "item " + std::to_string(item) : n;
	};
	std::string need;
	for (uint32_t item : cost) {
		double have = 0;
		if (sqlite3_prepare_v2(db, "SELECT amount FROM ga_ava_resources WHERE owner_agency_id = ?1 AND item_id = ?2",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_int(s, 2, (int)item);
			if (sqlite3_step(s) == SQLITE_ROW) have = sqlite3_column_double(s, 0);
			sqlite3_finalize(s);
		}
		if (have + 1e-6 < 1.0) {
			char part[120];
			snprintf(part, sizeof part, "%s1 %s (have %.2f)", need.empty() ? "" : ", ", resname(item).c_str(), have);
			need += part;
		}
	}
	if (!need.empty()) {
		return "AvA: not enough resources - need " + need + ". Assign a Mine to mine them.";
	}
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
	for (uint32_t item : cost)
		if (sqlite3_prepare_v2(db, "UPDATE ga_ava_resources SET amount = amount - 1 WHERE owner_agency_id = ?1 AND item_id = ?2",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_int(s, 2, (int)item);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_buildings (zone_id, position, owner_agency_id, hex_building_id, built_at) "
	        "VALUES (1, ?1, ?2, ?3, ?4)", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		sqlite3_bind_int64(s, 2, owner);
		sqlite3_bind_int(s, 3, (int)target);
		sqlite3_bind_int64(s, 4, (int64_t)std::time(nullptr));
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	std::string bname;
	if (sqlite3_prepare_v2(db, "SELECT COALESCE(m.message, '') FROM asm_data_set_hex_buildings b "
	        "LEFT JOIN asm_data_set_msg_translations m ON m.msg_id = b.name_msg_id WHERE b.hex_building_id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)target);
		if (sqlite3_step(s) == SQLITE_ROW) { const char* c = (const char*)sqlite3_column_text(s, 0); bname = c ? c : ""; }
		sqlite3_finalize(s);
	}
	char line[200];
	snprintf(line, sizeof line, "AvA: territory %u is now a %s (reopen the map to see it).", AvaZones::Local(nPosition),
		bname.empty() ? "new facility" : bname.c_str());
	return line;
}
std::string AvaServiceAssignResource(int64_t owner, uint32_t pos, uint32_t item)
{ sqlite3* db = Database::GetConnection(); return db ? ava_assign_resource_core(db, owner, pos, item) : "no db"; }
std::string AvaServiceAddFacility(int64_t owner, uint32_t pos, uint32_t id)
{ sqlite3* db = Database::GetConnection(); return db ? ava_add_facility_core(db, owner, pos, id) : "no db"; }

// AvA factory queue. HEX_ADD_FACTORY_ITEM / HEX_REMOVE_FACTORY_ITEM
// {ZONE_OBJECT_ID, POSITION, INVENTORY_ID, INSTANCE_COUNT} (exe sender 0x10926890). A Factory
// (hex building subtype 1320) owned by the agent's agency (or solo id) turns the owner's
// ga_ava_stock.production into AvA items (type 1298: siege 1300, facility devices 1301,
// dropships 1302) at asm_data_set_items.production_cost each, stored in ga_ava_items.
// INVENTORY_ID is read as the item id. Remove = scrap built items for a full refund.
static std::string ava_factory_item_core(sqlite3* db, int64_t want_owner, uint32_t nPosition, uint32_t nItemId, uint32_t nCount, bool bRemove, bool* changed);
void TcpSession::handle_hex_factory_item(uint32_t nPosition, uint32_t nItemId, uint32_t nCount, bool bRemove)
{
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty()) return;
	if (nCount == 0 || nCount > 100) nCount = 1;
	if (!AvaZones::Valid(nPosition) || !owns_ava_territory(nPosition)) {
		send_team_system_message(17494, "");  // "Territory is not controlled by your agency."
		return;
	}
	bool changed = false;
	const int64_t ava_owner = ava_territory_owner(db, nPosition).first;
	const uint32_t bag_bp = bRemove ? 0 : AvaShields::BagBlueprint(selected_character_id_, nItemId);
	const auto stock_bp = [&](int delta) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3) "
		        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + ?3", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, ava_owner);
			sqlite3_bind_int(s, 2, (int)bag_bp);
			sqlite3_bind_int(s, 3, delta);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		sqlite3_exec(db, "DELETE FROM ga_ava_items WHERE count <= 0", nullptr, nullptr, nullptr);
	};
	if (bag_bp) stock_bp(1);  // the picked bag row goes back to the agency for the build (one row = one build)
	const std::string text = ava_factory_item_core(db, ava_owner, nPosition, bag_bp ? bag_bp : nItemId,
		bag_bp ? 1 : nCount, bRemove, &changed);
	if (bag_bp) {
		if (changed) {
			sqlite3_stmt* s = nullptr;
			if (sqlite3_prepare_v2(db, "DELETE FROM ga_players_inventory WHERE id = ?1", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, nItemId);
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
			send_inventory_clear(last_pawn_id_, selected_character_id_);
			send_inventory_response(last_pawn_id_, selected_character_id_);
			sync_item_count(selected_character_id_);
		} else stock_bp(-1);
	}
	if (!text.empty()) ChatSession::SystemMessageTo(player_name, text);
	if (changed) send_strike_force_inventory();
}
// Session-free part (also driven by the service-agents admin op "produce").
static std::string ava_factory_item_core(sqlite3* db, int64_t want_owner, uint32_t nPosition, uint32_t nItemId, uint32_t nCount, bool bRemove, bool* changed)
{
	if (nCount == 0 || nCount > 100) nCount = 1;
	if (!AvaZones::Valid(nPosition)) return "AvA: bad territory.";
	PlayerSessionStore::SettleAvaProduction();
	sqlite3_stmt* s = nullptr;
	int64_t owner = 0;
	int subtype = 0;
	if (sqlite3_prepare_v2(db, "SELECT t.owner_agency_id, b.building_subtype_value_id FROM ga_ava_territories t "
	        "LEFT JOIN asm_data_set_hex_buildings b ON b.hex_building_id = ?2 "
	        "WHERE t.zone_id = 1 AND t.position = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		sqlite3_bind_int(s, 2, (int)ava_building(db, nPosition).id);
		if (sqlite3_step(s) == SQLITE_ROW) { owner = sqlite3_column_int64(s, 0); subtype = sqlite3_column_int(s, 1); }
		sqlite3_finalize(s);
	}
	if (owner == 0 || owner != want_owner) return "Territory is not controlled by your agency.";
	if (subtype != 1320) return "AvA: only a Factory you hold can build items.";
	nItemId = AvaParity::FactoryTarget(nItemId);  // blueprint id -> item
	double cost = 0;
	if (sqlite3_prepare_v2(db, "SELECT production_cost FROM asm_data_set_items WHERE item_id = ?1 "
	        "AND ((item_type_value_id = 1298 AND item_subtype_value_id IN (1300, 1301, 1302)) OR (item_type_value_id = 1312 AND item_subtype_value_id = 1339) "
	        "OR item_id IN (5075, 5112, 5113, 5114, 5115, 5116, 5117)) AND production_cost > 0",  // Tactical Missiles, Missile Complex / Tech II-III deeds
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nItemId);
		if (sqlite3_step(s) == SQLITE_ROW) cost = sqlite3_column_double(s, 0);
		sqlite3_finalize(s);
	}
	if (cost <= 0) return "AvA: that item cannot be built in a Factory.";
	auto scalar = [&](const char* sql) {
		double v = 0;
		if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_int(s, 2, (int)nItemId);
			if (sqlite3_step(s) == SQLITE_ROW) v = sqlite3_column_double(s, 0);
			sqlite3_finalize(s);
		}
		return v;
	};
	const double have = scalar("SELECT production FROM ga_ava_stock WHERE owner_agency_id = ?1 AND ?2 = ?2");
	const double built = scalar("SELECT count FROM ga_ava_items WHERE owner_agency_id = ?1 AND item_id = ?2");
	if (bRemove && built < nCount) nCount = (uint32_t)built;
	if (bRemove && nCount == 0) return "";
	const double total = cost * nCount;
	if (!bRemove && have + 1e-6 < total) {
		char line[160];
		snprintf(line, sizeof line, "AvA: not enough production (need %.0f, have %.0f).", total, have);
		return line;
	}
	if (const auto g = AvaParity::FactoryRecipe(owner, nItemId, nCount, bRemove); g.refuse) return g.text;  // recipe
	const int delta = bRemove ? -(int)nCount : (int)nCount;
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
	if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_stock (owner_agency_id, production) VALUES (?1, ?2) "
	        "ON CONFLICT(owner_agency_id) DO UPDATE SET production = production + ?2", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		sqlite3_bind_double(s, 2, bRemove ? total : -total);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3) "
	        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + ?3", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		sqlite3_bind_int(s, 2, (int)nItemId);
		sqlite3_bind_int(s, 3, delta);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "DELETE FROM ga_ava_items WHERE count <= 0", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	AvaParity::BookNetworth(owner, bRemove ? -total : total);  // networth
	char line[200];
	snprintf(line, sizeof line, "AvA Factory: %s %u x item %u for %.0f production (now %.0f production, %.0f built).",
		bRemove ? "scrapped" : "built", nCount, nItemId, total, bRemove ? have + total : have - total, built + delta);
	if (changed) *changed = true;
	return line;
}
std::string AvaServiceFactoryItem(int64_t owner, uint32_t pos, uint32_t item, uint32_t count, bool remove, bool* changed)
{ sqlite3* db = Database::GetConnection(); return db ? ava_factory_item_core(db, owner, pos, item, count, remove, changed) : "no db"; }

static int64_t ava_owner_of_character(sqlite3* db, int64_t character_id);
void TcpSession::send_hex_get_zone_data_response(uint32_t nZoneObjectId)
{
	AvaParity::ResolveBidWindows((int64_t)std::time(nullptr));  // resolve (zone map)
	using namespace hexzone;
	const AvaZones::Zone* zone = AvaZones::Find(nZoneObjectId);
	if (!zone) zone = AvaZones::Find(1);
	AvaZones::SetView(session_guid_, zone->id);
	const uint32_t base = AvaZones::Base(zone->id);

	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::HEX_GET_ZONE_DATA;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 14, 0x00);  // top-level items

	// Owners (part 5d): ga_ava_territories, written by PlayerSessionStore::ResolveAvaMatch.
	struct Owner { uint32_t agency; std::string name; float r, g, b; };
	std::map<uint32_t, Owner> owner_at;          // position -> owner
	std::map<uint32_t, const Owner*> agencies;   // agency id -> one row each
	if (sqlite3* db = Database::GetConnection()) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT position, owner_agency_id, owner_name, color_r, color_g, color_b "
		        "FROM ga_ava_territories WHERE zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
			while (sqlite3_step(s) == SQLITE_ROW) {
				const uint32_t pos = (uint32_t)sqlite3_column_int(s, 0);
				if (AvaZones::ZoneOf(pos) != zone->id || AvaZones::Local(pos) >= kWidth * kHeight) continue;
				const char* n = (const char*)sqlite3_column_text(s, 2);
				owner_at[AvaZones::Local(pos)] = {(uint32_t)sqlite3_column_int64(s, 1), n ? n : "",
					(float)sqlite3_column_double(s, 3), (float)sqlite3_column_double(s, 4),
					(float)sqlite3_column_double(s, 5)};
			}
			sqlite3_finalize(s);
		}
	}
	std::map<uint32_t, AvaHexMods> hex_mods;
	if (sqlite3* db = Database::GetConnection()) hex_mods = ava_production_mods(db);
	// Standing bids -> BID_AMOUNT + HEX_BID_AGENCY_ID (bidder listed in DATA_SET_AGENCIES).
	struct Bid { uint32_t owner, amount; };
	std::map<uint32_t, Bid> bid_at;
	std::vector<Owner> bidders;
	if (sqlite3* db = Database::GetConnection()) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT b.position, b.owner_agency_id, b.amount, COALESCE(a.name, u.username, '?') FROM ga_ava_bids b "
		        "LEFT JOIN ga_characters c ON c.id = b.character_id LEFT JOIN ga_users u ON u.id = c.user_id "
		        "LEFT JOIN ga_agencies a ON a.id = b.owner_agency_id WHERE b.zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
			while (sqlite3_step(s) == SQLITE_ROW) {
				const uint32_t pos = (uint32_t)sqlite3_column_int(s, 0);
				if (AvaZones::ZoneOf(pos) != zone->id || AvaZones::Local(pos) >= kWidth * kHeight) continue;
				const uint32_t owner = (uint32_t)sqlite3_column_int64(s, 1);
				bid_at[AvaZones::Local(pos)] = {owner, (uint32_t)sqlite3_column_int64(s, 2)};
				const char* n = (const char*)sqlite3_column_text(s, 3);
				bidders.push_back({owner, n ? n : "?", 0.5f, 0.5f, 0.5f});
			}
			sqlite3_finalize(s);
		}
	}
	for (const auto& [pos, o] : owner_at) agencies.emplace(o.agency, &o);
	for (const auto& o : bidders) agencies.emplace(o.agency, &o);  // owners keep their colour

	append(response, GA_T::DATA_SET_AGENCIES & 0xFF, GA_T::DATA_SET_AGENCIES >> 8);
	append(response, agencies.size() & 0xFF, agencies.size() >> 8);
	for (const auto& [id, o] : agencies) {
		append(response, 0x05, 0x00);
		Write4B(response, GA_T::AGENCY_ID, id);
		WriteString(response, GA_T::NAME, o->name);
		WriteFloat(response, GA_T::LINEAR_COLOR_R, o->r);
		WriteFloat(response, GA_T::LINEAR_COLOR_G, o->g);
		WriteFloat(response, GA_T::LINEAR_COLOR_B, o->b);
	}
	Write4B(response, GA_T::ZONE_OBJECT_ID, nZoneObjectId);
	Write4B(response, GA_T::HEX_ZONE_ID, zone->id);
	Write4B(response, GA_T::WIDTH, kWidth);
	Write4B(response, GA_T::HEIGHT, kHeight);
	Write4B(response, GA_T::BACKGROUND_IMAGE_RES_ID, AvaZones::Background(zone->id));
	Write4B(response, GA_T::NAME_MSG_ID, zone->name_msg);
	Write4B(response, GA_T::DESC_MSG_ID, zone->desc_msg);
	// The client's AvA map header (exe 0x1143e7b0) shows
	// msg 55274 'Time until season begins:' + (zone +0x16 != 0) -> 55275 '...ends:' with the timer from
	// ROTATION_REMAINING_SECONDS (zone +0x28, getter 0x10926530); the 2nd bar (only when +0x16) shows 53749 'Time until
	// open:' + (+0x15) -> 53750 '...close:' with REMAINING_SECONDS (+0x2c). Parse 0x10927efb: SETTING_FLAGS bit 24 ->
	// +0x16 (season running), bit 3 -> +0x15 (zone open), bit 2 -> +0x14. We sent 0/0 -> 'season begins: 0 minutes'.
	// Ours: the War Zone is always open and closes only at the season-end wipe.
	Write4B(response, GA_T::ROTATION_REMAINING_SECONDS, ava_season().remaining);  // season timer (header)
	Write4B(response, GA_T::REMAINING_SECONDS, ava_season().remaining);  // zone closes at the season-end wipe
	Write4B(response, GA_T::SETTING_FLAGS, (1u << 24) | (1u << 3));  // season running + zone open

	const auto built = ava_built(Database::GetConnection());
	const auto open_fights = AvaParity::OpenFights(zone->id, selected_character_id_);
	std::map<uint32_t, int64_t> open_at;  // local position -> attacker
	for (const auto& f : open_fights) open_at[AvaZones::Local(f.position)] = f.attacker;
	// The agent's own fights: attacker -> battle button (0x13B, mode 2); defender -> the row mark gives Defend (mode 1).
	uint32_t own_attack = AvaZones::kBad;
	for (const auto& f : AvaParity::OwnFights(zone->id, selected_character_id_)) {
		open_at[AvaZones::Local(f.position)] = f.attacker;
		if (f.attacker == AvaParity::OwnerOf(selected_character_id_) && own_attack == AvaZones::kBad) own_attack = f.position;
	}
	const bool row_mark = MatchKey::SettingInt("ava_open_row_mark", 1) != 0;
	append(response, GA_T::DATA_SET_HEX_TERRITORIES & 0xFF, GA_T::DATA_SET_HEX_TERRITORIES >> 8);
	append(response, (kWidth * kHeight) & 0xFF, (kWidth * kHeight) >> 8);
	for (uint32_t pos = 0; pos < kWidth * kHeight; ++pos) {
		const auto bid = bid_at.find(pos);
		const int hq_progress = AvaParity::HqProgress(base + pos);  // HQ build time
		const int64_t region_secs = AvaParity::RegionSeconds(base + pos);  // region seconds
		const auto shield = AvaShields::RowFor(base + pos);  // shields
		const auto hex_mod = hex_mods.find(base + pos);
		append(response, (bid != bid_at.end() ? 0x09 : 0x07) + (hq_progress >= 0 ? 1 : 0) + (region_secs >= 0 ? 1 : 0)
			+ (hex_mod != hex_mods.end() ? 3 : 0)
			+ (shield.position_flags ? 1 : 0) + (shield.origin_flags ? 1 : 0) + (shield.progress >= 0 ? 1 : 0)
			+ (row_mark && open_at.count(pos) ? 2 : 0), 0x00);
		if (row_mark && open_at.count(pos)) { Write4B(response, GA_T::HEX_ATTACK_AGENCY_ID, (uint32_t)open_at[pos]); Write4B(response, GA_T::ATTACK_STRIKE_FORCE_ID, (uint32_t)open_at[pos]); }  // solo agents get the panel (exe 0x1143ff20 mode 5)
		if (shield.position_flags) Write4B(response, GA_T::SHIELD_POSITION_FLAGS, shield.position_flags);
		if (shield.origin_flags) Write4B(response, GA_T::SHIELD_ORIGIN_FLAGS, shield.origin_flags);
		if (shield.progress >= 0) Write4B(response, GA_T::SHIELD_PROGRESS, (uint32_t)shield.progress);
		if (hq_progress >= 0) Write4B(response, GA_T::HQ_PROGRESS, (uint32_t)hq_progress);
		if (hex_mod != hex_mods.end()) {
			WriteFloat(response, GA_T::HQ_BONUS, hex_mod->second.hq);
			WriteFloat(response, GA_T::ADJACENCY_BONUS, hex_mod->second.adj);
			WriteFloat(response, GA_T::PRODUCTION_MODIFIER, hex_mod->second.mod);
		}
		if (region_secs >= 0) Write4B(response, GA_T::REGION_SECONDS, (uint32_t)region_secs);
		// Territory SETTING_FLAGS bit 25 = tile shown. The hex widget copies it to the tile
		// (exe 0x10ac2c8f -> tile+0xb0 bit 2) and UpdateFilters (0x10ac69a0) hides every tile
		// without it - with 0 the AvA map stayed empty although the zone data was parsed.
		// Bit 23 = closed (exe 0x1143ffe9: no Bid button; 0x1143fb15: 'Time until open' instead of 'Time until close').
		const int hex_region = AvaParity::RegionOf(base + pos);
		constexpr uint32_t kHexIsHq = 1u << 19, kHexCanBeHq = 1u << 21;
				const uint32_t hq_flags = !owner_at.count(pos) ? 0u : hq_progress >= 0 ? kHexIsHq : kHexCanBeHq;
		Write4B(response, GA_T::SETTING_FLAGS, kTerritoryVisible | hq_flags |
			(hex_region && hex_region != AvaParity::OpenRegion((int64_t)std::time(nullptr), nullptr) ? 0x00800000u : 0u));
		if (bid != bid_at.end()) {
			Write4B(response, GA_T::BID_AMOUNT, bid->second.amount);
			Write4B(response, GA_T::HEX_BID_AGENCY_ID, bid->second.owner);
		}
		Write4B(response, GA_T::POSITION, pos);
		const auto bt = built.find(base + pos);
		const HexBuilding hb = bt != built.end() ? bt->second : LayoutAt(base + pos);
		Write4B(response, GA_T::HEX_BUILDING_ID, hb.id);
		Write4B(response, GA_T::BUILDING_TYPE_VALUE_ID, hb.type);
		Write4B(response, GA_T::BUILDING_SUBTYPE_VALUE_ID, hb.subtype);
		const auto it = owner_at.find(pos);
		Write4B(response, GA_T::OWNER_AGENCY_ID, it != owner_at.end() ? it->second.agency : 0);
		Write4B(response, GA_T::HEX_ZONE_REGION_ID, (uint32_t)AvaParity::RegionOf(base + pos));
	}

	// Add Task list (exe 0x1146db70): {ITEM_ID, TECH_LEVEL_VALUE_ID}, the popup filters by item type (Mine 1171, Lab 686)
	// and by tech <= the hex's. Ore: any Mine ("higher tier mines produce the same ore"). Blueprints: by Lab tier (wiki AVA "Lab").
	{
		std::vector<std::pair<uint32_t, uint32_t>> res;
		sqlite3* zdb = Database::GetConnection();
		sqlite3_stmt* zs = nullptr;
		if (zdb && sqlite3_prepare_v2(zdb, "SELECT item_id, 1335 FROM asm_data_set_items WHERE item_type_value_id = 1171 AND item_subtype_value_id = 1347 "
		        "AND item_id < 6564 AND production_cost > 0 UNION ALL SELECT l.item_id, 1182 + MIN(l.loot_table_id) FROM asm_data_set_loot_table_items l "
		        "JOIN asm_data_set_items i ON i.item_id = l.item_id WHERE l.loot_table_id IN (153, 154, 155) AND i.item_type_value_id = 686 GROUP BY l.item_id",
		        -1, &zs, nullptr) == SQLITE_OK) {
			while (sqlite3_step(zs) == SQLITE_ROW) res.push_back({(uint32_t)sqlite3_column_int(zs, 0), (uint32_t)sqlite3_column_int(zs, 1)});
			sqlite3_finalize(zs);
		}
		append(response, GA_T::DATA_SET_HEX_ZONE_RESOURCES & 0xFF, GA_T::DATA_SET_HEX_ZONE_RESOURCES >> 8);
		append(response, res.size() & 0xFF, res.size() >> 8);
		for (const auto& [item, tech] : res) {
			append(response, 2, 0x00);
			Write4B(response, GA_T::ITEM_ID, item);
			Write4B(response, GA_T::TECH_LEVEL_VALUE_ID, tech);
		}
		// Current Task of our own hexes (exe 0x10926210): {POSITION, RESOURCE_ITEM_ID, CURRENT_WIP}.
		std::vector<std::pair<uint32_t, uint32_t>> own;
		const int64_t zme = zdb ? ava_owner_of_character(zdb, selected_character_id_) : 0;
		if (zme && sqlite3_prepare_v2(zdb, "SELECT r.position, r.item_id FROM ga_ava_mine_resources r JOIN ga_ava_territories t ON t.zone_id = r.zone_id "
		        "AND t.position = r.position AND t.owner_agency_id = r.owner_agency_id WHERE r.owner_agency_id = ?1 AND r.position >= ?2 AND r.position < ?3",
		        -1, &zs, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(zs, 1, zme);
			sqlite3_bind_int64(zs, 2, base);
			sqlite3_bind_int64(zs, 3, (int64_t)base + kWidth * kHeight);
			while (sqlite3_step(zs) == SQLITE_ROW) own.push_back({(uint32_t)sqlite3_column_int(zs, 0) - base, (uint32_t)sqlite3_column_int(zs, 1)});
			sqlite3_finalize(zs);
		}
		append(response, GA_T::DATA_SET_PRIVATE & 0xFF, GA_T::DATA_SET_PRIVATE >> 8);
		append(response, own.size() & 0xFF, own.size() >> 8);
		for (const auto& [pos, item] : own) {
			append(response, 3, 0x00);
			Write4B(response, GA_T::POSITION, pos);
			Write4B(response, GA_T::RESOURCE_ITEM_ID, item);
			Write4B(response, GA_T::CURRENT_WIP, 0);
		}
	}

	send_response(response);
	// Any 0x13B hides Bid on every other hex (exe 0x11440175). Join is for agents without a strike force to lead.
	const bool sf_leader = TeamService::IsLeader(session_guid_) && Social::Teams::ModeFor(TeamService::GetTeamId(session_guid_)) >= 2;
	send_hex_campaign_state(zone->id, own_attack != AvaZones::kBad ? own_attack : (open_fights.empty() || sf_leader) ? AvaZones::kBad : open_fights.front().position);
}

// Combat Log tab. Dates come as "YYYYMMDD  H:MM:SS"; the row date is days since 1900-01-01 (exe 0x10955ab0).
static int64_t ava_log_day(const std::string& s, int64_t def)
{
	if (s.size() < 8) return def;
	std::tm tm{};
	tm.tm_year = std::atoi(s.substr(0, 4).c_str()) - 1900;
	tm.tm_mon = std::atoi(s.substr(4, 2).c_str()) - 1;
	tm.tm_mday = std::atoi(s.substr(6, 2).c_str());
	return (int64_t)timegm(&tm);
}
void TcpSession::send_ava_combat_log(uint32_t zone, const std::string& agency_like, uint32_t outcome, const std::string& from, const std::string& to)
{
	const int64_t now = (int64_t)std::time(nullptr);
	const auto rows = AvaParity::CombatLog(AvaZones::Find(zone) ? zone : 1, agency_like.empty() ? "%" : agency_like, outcome,
		ava_log_day(from, now - 86400), ava_log_day(to, now) + 86400);
	std::vector<uint8_t> response;
	const uint16_t packet_type = GA_U::GET_COMBAT_LOG;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 1, 0x00);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, rows.size() & 0xFF, (rows.size() >> 8) & 0xFF);
	for (const auto& r : rows) {
		append(response, 6, 0x00);
		WriteDouble(response, GA_T::EVENT_DATETIME, 25567.0 + (double)r.at / 86400.0);
		Write4B(response, GA_T::HEX_BUILDING_ID, r.building);
		Write4B(response, GA_T::POSITION, AvaZones::Local(r.position));
		WriteString(response, GA_T::OWNING_AGENCY_NAME, r.owner);
		WriteString(response, GA_T::ATTACKING_AGENCY_NAME, r.attacker);
		Write4B(response, GA_T::EVENT_VALUE_ID, r.outcome);
	}
	send_response(response);
}

// Combat Alerts tab: reply to HEX_GET_FULL_ACA (exe parser 0x10928660): the fights of the agent's own owner.
void TcpSession::send_hex_full_aca()
{
	const auto fights = AvaParity::OwnFights(0, selected_character_id_);
	const int64_t me = AvaParity::OwnerOf(selected_character_id_);
	std::map<int64_t, std::string> names;
	std::vector<uint8_t> response;
	const uint16_t packet_type = GA_U::HEX_GET_FULL_ACA;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 2, 0x00);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, fights.size() & 0xFF, (fights.size() >> 8) & 0xFF);
	for (const auto& f : fights) {
		names[f.attacker]; if (f.defender) names[f.defender];
		append(response, 9, 0x00);
		Write1B(response, GA_T::DEFENDING_FLAG, f.attacker == me ? 0 : 1);
		Write4B(response, GA_T::ZONE_OBJECT_ID, AvaZones::ZoneOf(f.position));
		Write4B(response, GA_T::POSITION, AvaZones::Local(f.position));
		Write4B(response, GA_T::HEX_BUILDING_ID, AvaParityBuildingAt(f.position));
		Write4B(response, GA_T::OWNER_AGENCY_ID, (uint32_t)f.defender);
		Write4B(response, GA_T::AGENCY_ID, (uint32_t)f.attacker);
		Write4B(response, GA_T::PLAYER_COUNT, 0);
		Write4B(response, GA_T::HEX_ATTACK_TYPE, 1);
		Write4B(response, GA_T::REMAINING_LAUNCH_SECONDS, f.secs_left > 0 ? (uint32_t)f.secs_left : 0);
	}
	append(response, GA_T::DATA_SET_AGENCIES & 0xFF, GA_T::DATA_SET_AGENCIES >> 8);
	append(response, names.size() & 0xFF, (names.size() >> 8) & 0xFF);
	for (const auto& [id, name] : names) {
		append(response, 2, 0x00);
		Write4B(response, GA_T::AGENCY_ID, (uint32_t)id);
		WriteString(response, GA_T::AGENCY_NAME, AvaParity::OwnerName(id));
	}
	send_response(response);
}

// HEX_CAMPAIGN_STATE 0x13B server -> client (exe parser 0x10928cb0 via 0x109120e0): the attack this agent is part of. The
// map's option panel shows the battle button on that hex (exe 0x1143ff20 mode 2), also for an agent without a team;
// the button sends HEX_GOTO_BATTLE, which has no fields, so the session remembers the hex.
void TcpSession::send_hex_campaign_state(uint32_t zone_id, uint32_t g)
{
	AvaZones::SetBattle(session_guid_, g);
	if (MatchKey::SettingInt("ava_campaign_state", 1) == 0) return;
	const bool on = g != AvaZones::kBad;
	std::vector<uint8_t> response;
	const uint16_t packet_type = GA_U::HEX_CAMPAIGN_STATE;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, on ? 4 : 2, 0x00);
	Write4B(response, GA_T::ZONE_OBJECT_ID, on ? zone_id : 0);
	Write4B(response, GA_T::POSITION, on ? AvaZones::Local(g) : 0);
	if (on) {
		Write4B(response, GA_T::HEX_BUILDING_ID, AvaParityBuildingAt(g));
		Write4B(response, GA_T::REMAINING_LAUNCH_SECONDS, 0);
	}
	send_response(response);
}

// AvA Attack gate. The campaign map only sends HEX_CAPTURE when the
// strike force holds a Dropship (exe 0x1143b3c0: some DATA_SET_STRIKE_FORCE_INVENTORY row
// whose item is type 1298 AvA / subtype 1302 Dropship, else msg 38846 "too much cargo").
// That list (team object 0x1199fbc8 +0x3c/+0x40) is filled only by TEAM_UPDATE's
// SETTING_FLAGS branch (0x1092a5a0 -> 0x1092a360): no roster DATA_SET there, just
// DATA_SET_STRIKE_FORCE_INVENTORY rows {INVENTORY_ID, ITEM_ID, INSTANCE_COUNT, TASK_TEAM}.
// Dropships were agency assets on live; until agencies exist everyone gets a Basic Dropship.
void TcpSession::send_strike_force_inventory()
{
	constexpr uint32_t kBasicDropship = 5111;
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::TEAM_UPDATE;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 2, 0x00);
	Write4B(response, GA_T::SETTING_FLAGS, 0);
	append(response, GA_T::DATA_SET_STRIKE_FORCE_INVENTORY & 0xFF, GA_T::DATA_SET_STRIKE_FORCE_INVENTORY >> 8);
	append(response, 1, 0x00);
	// The agency's (or solo id's) ga_ava_items; AvaCargo::InventoryRows marks
	// loaded rows TASK_TEAM 1 (cargo menu) and the ones kept home TASK_TEAM 0. Row ids: 1 = Basic Dropship.
	(void)kBasicDropship;
	const auto rows = AvaCargo::InventoryRows(selected_character_id_);
	response[response.size() - 2] = (uint8_t)(rows.size() & 0xFF);
	response[response.size() - 1] = (uint8_t)(rows.size() >> 8);
	for (const auto& r : rows) {
		append(response, 4, 0x00);
		Write4B(response, GA_T::INVENTORY_ID, r.inventory_id);
		Write4B(response, GA_T::ITEM_ID, r.item_id);
		Write4B(response, GA_T::INSTANCE_COUNT, r.count);
		Write4B(response, GA_T::TASK_TEAM, r.task_team);
	}
	send_response(response);
}

// HEX_CAPTURE / HEX_DEFEND {ZONE_OBJECT_ID, POSITION} (exe 0x1143b4d0 / 0x1143b440 ->
// 0x10926c80). Attack = join the hidden AvA queue of the territory's facility; the normal
// queue -> MATCH_INVITATION -> instance flow does the rest. Defend (part 5e) = the owner
// (their agency, or the agent's solo id 0x40000000 + character id) joins the same
// facility queue on task force 2, so it meets any attacker queued there.
void TcpSession::handle_hex_attack(uint32_t nZoneObjectId, uint32_t nPosition, bool bDefend)
{
	using namespace hexzone;
	if (bDefend && !owns_ava_territory(nPosition)) {
		send_team_system_message(17494, "");  // "Territory is not controlled by your agency."
		return;
	}
	if (const auto g = AvaParity::Check(selected_character_id_, nPosition, bDefend ? AvaParity::kDefend : AvaParity::kAttack); g.refuse) {
		if (g.msg_id) send_team_system_message(g.msg_id, "");
		if (!g.text.empty()) ChatSession::SystemMessageTo(player_name, g.text);
		return;
	}
	const uint32_t queue_id = AvaZones::Valid(nPosition) ? QueueForBuilding(ava_building(Database::GetConnection(), nPosition).id) : 0;
	Logger::Log("tcp", "[TcpSession] %s zone %u pos %u -> AvA queue %u (%s)\n",
		bDefend ? "HEX_DEFEND" : "HEX_CAPTURE", nZoneObjectId, nPosition, queue_id, player_name.c_str());
	if (queue_id == 0 || !MatchmakingService::GetQueueConfig(queue_id)) {
		send_team_system_message(17408, "");  // "Territory cannot be attacked, it is being secured."
		return;
	}
	if (current_match_queue_id_ != 0) {
		ChatSession::SystemMessageToGuid(session_guid_, "*** Leave your current queue before launching an AvA attack. ***");
		return;
	}
	if (!bDefend && ava_bid_blocks_attack(nPosition)) return;
	if (!bDefend) if (const auto ms = AvaShields::CheckAttack(selected_character_id_, nPosition, false); ms.refuse) {  // attack gate
		if (ms.msg_id) send_team_system_message(ms.msg_id, "");
		if (!ms.text.empty()) ChatSession::SystemMessageTo(player_name, ms.text);
		return;
	}
	MatchmakingService::SetForcedSide(session_guid_, bDefend ? 2 : 0);
	// Attackers and defenders of this territory share one match; a defender
	// waits for it (cs_settings ava_defend_hold_secs, default 300; -1 = no limit), then joins any.
	MatchKey::Set(session_guid_, (int64_t)nPosition + 1, bDefend ? MatchKey::SettingInt("ava_defend_hold_secs", 300) : 0);
	if (bDefend) ChatSession::SystemMessageTo(player_name, "Defending territory " + AvaZones::Label(nPosition) +
		": you join the attack on it as soon as it starts (or the match already running there).");
	handle_match_join(queue_id, 0);
	if (bDefend) return;  // holding the territory needs no bookkeeping
	// Remember the target so the AvA result (IpcServer MISSION_ENDED -> ResolveAvaMatch)
	// knows which territory a win captures. Leaving the queue leaves a stale row that the
	// next attack overwrites; a match resolve consumes it.
	if (current_match_queue_id_ == queue_id) {
		if (const auto ms = AvaShields::CheckAttack(selected_character_id_, nPosition, true); !ms.text.empty())  // missile launch
			ChatSession::SystemMessageTo(player_name, ms.text);
		if (sqlite3* db = Database::GetConnection()) {
			sqlite3_stmt* s = nullptr;
			if (sqlite3_prepare_v2(db,
			        "INSERT OR REPLACE INTO ga_ava_attacks (character_id, zone_id, position, queued_at, on_behalf_of) "
			        "VALUES (?, 1, ?, strftime('%s','now'), ?)", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, selected_character_id_);
				sqlite3_bind_int(s, 2, (int)nPosition);
				sqlite3_bind_int64(s, 3, AvaParity::OpenBidOwnerFor(nPosition, selected_character_id_));
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
		}
		for (const auto& n : AvaParity::AttackAlertRecipients(nPosition, selected_character_id_))  // (client text 39861)
			ChatSession::SystemMessageTo(n, "An agency territory is under attack! (territory " + AvaZones::Label(nPosition) + ")");
	}
}

// AvA territory bids. One standing (top) bid per territory in ga_ava_bids;
// the credits sit in escrow (taken from the bidder). A higher bid from another owner refunds
// the old one by system mail; cancel refunds too. The top bidder's owner (agency or solo id)
// alone may launch HEX_CAPTURE on that territory; launching it spends the bid. Amount rules
// are ours (no values in the files): minimum 1000 credits, +10% over the standing bid.
static int64_t ava_owner_of_character(sqlite3* db, int64_t character_id)
{
	sqlite3_stmt* s = nullptr;
	int64_t owner = 0;
	if (sqlite3_prepare_v2(db, "SELECT COALESCE(m.agency_id, ?2 + c.id) FROM ga_characters c "
	        "LEFT JOIN ga_agency_members m ON m.user_id = c.user_id WHERE c.id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		sqlite3_bind_int64(s, 2, 0x40000000);
		if (sqlite3_step(s) == SQLITE_ROW) owner = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	return owner;
}

struct AvaBid { bool found = false; int64_t character_id = 0, owner = 0, amount = 0; std::string name; };

static AvaBid ava_standing_bid(sqlite3* db, uint32_t nPosition)
{
	AvaBid b;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT b.character_id, b.owner_agency_id, b.amount, COALESCE(u.username, '?') "
	        "FROM ga_ava_bids b LEFT JOIN ga_characters c ON c.id = b.character_id "
	        "LEFT JOIN ga_users u ON u.id = c.user_id "
	        "WHERE b.zone_id = 1 AND b.position = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		if (sqlite3_step(s) == SQLITE_ROW) {
			b.found = true;
			b.character_id = sqlite3_column_int64(s, 0);
			b.owner = sqlite3_column_int64(s, 1);
			b.amount = sqlite3_column_int64(s, 2);
			const char* n = (const char*)sqlite3_column_text(s, 3);
			b.name = n ? n : "?";
		}
		sqlite3_finalize(s);
	}
	return b;
}

// Removes the standing bid of a territory and mails its credits back (inside the caller's transaction).
static bool ava_refund_bid(sqlite3* db, uint32_t nPosition, const AvaBid& b, const char* why)
{
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "DELETE FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1", -1, &s, nullptr) != SQLITE_OK)
		return false;
	sqlite3_bind_int(s, 1, (int)nPosition);
	const bool ok = sqlite3_step(s) == SQLITE_DONE;
	sqlite3_finalize(s);
	if (!ok) return false;
	char body[160];
	snprintf(body, sizeof body, "Your bid of %lld credits on AvA territory %u was returned: %s.",
		(long long)b.amount, AvaZones::Local(nPosition), why);
	return MailService::DeliverFromAuction(0, b.character_id,
		"AvA bid refund - territory " + AvaZones::Label(nPosition), body, b.amount) != 0;
}

// HEX_GET_AUCTION reply (exe 0x113e0187): one DATA_SET_HEX_TERRITORY_AUCTION row fills the bid panel. BID_AMOUNT = the price
// to beat: the client lets a bid through when it is higher or exactly 1000. Bids stay sealed, so it is the agency's own bid
// or the minimum. ATTACK_TYPE_FLAGS bit 0 Capture, bit 1 Steal; with one bit the panel hides the choice and uses it.
void TcpSession::send_hex_auction(uint32_t nPosition)
{
	if (!AvaZones::Valid(nPosition)) return;
	int64_t left = 0;
	const bool window = AvaParity::BidWindowOpen(nPosition, &left);
	const auto shield = AvaShields::RowFor(nPosition);
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::HEX_GET_AUCTION;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 1, 0x00);
	append(response, GA_T::DATA_SET_HEX_TERRITORY_AUCTION & 0xFF, GA_T::DATA_SET_HEX_TERRITORY_AUCTION >> 8);
	append(response, 1, 0x00);
	append(response, 5, 0x00);
	Write4B(response, GA_T::AUCTION_TIME_LEFT, window ? (uint32_t)left : 0);
	const int64_t mine = AvaParity::OwnSealedBid(selected_character_id_, nPosition);
	Write4B(response, GA_T::BID_AMOUNT, mine > 0 ? (uint32_t)mine : 1000);
	Write4B(response, GA_T::ATTACK_TYPE_FLAGS, AvaParity::AttackTypesFor(nPosition));
	Write4B(response, GA_T::SHIELD_ORIGIN_FLAGS, shield.origin_flags);
	Write1B(response, GA_T::BASE_PROTECTED_FLAG, 0);
	send_response(response);
}

// HEX_BID reply (exe 0x113e0120): ERROR_CODE > 0 shows that message in the panel, 0 closes it.
void TcpSession::send_hex_bid_reply(uint32_t msg_id)
{
	std::vector<uint8_t> response;
	uint16_t packet_type = GA_U::HEX_BID;
	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, 1, 0x00);
	Write4B(response, GA_T::ERROR_CODE, msg_id);
	send_response(response);
}

void TcpSession::handle_hex_bid(uint32_t nPosition, uint32_t nAmount, uint32_t nAttackType, bool bCancel)
{
	(void)nAttackType;
	sqlite3* db = Database::GetConnection();
	if (!db || player_name.empty() || selected_character_id_ == 0) return;
	if (!AvaZones::Valid(nPosition)) return;
	const int64_t me = ava_owner_of_character(db, selected_character_id_);
	if (me == 0) return;
	if (const auto g = bCancel ? AvaParity::Gate{} : AvaParity::Check(selected_character_id_, nPosition, AvaParity::kBid); g.refuse) {
		if (g.msg_id) send_team_system_message(g.msg_id, "");
		if (!g.text.empty()) ChatSession::SystemMessageTo(player_name, g.text);
		return;
	}
	if (const auto sb = AvaParity::SealedBid(selected_character_id_, nPosition, nAmount, nAttackType, bCancel); sb.handled) {
		AvaPushZoneToViewers(AvaZones::ZoneOf(nPosition), session_guid_);  // sealed bid
		if (sb.msg_id) send_team_system_message(sb.msg_id, ""); if (!sb.text.empty()) ChatSession::SystemMessageTo(player_name, sb.text); if (sb.wallet_changed || sb.msg_id) send_hex_bid_reply(sb.msg_id); if (sb.wallet_changed) send_mail_notify(0, true); return; }
	const AvaBid cur = ava_standing_bid(db, nPosition);
	int64_t refunded_to = 0;
	char line[200];

	if (bCancel) {
		if (!cur.found || cur.owner != me) {
			ChatSession::SystemMessageTo(player_name, "AvA: you have no bid on territory " + AvaZones::Label(nPosition) + ".");
			return;
		}
		sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
		if (!ava_refund_bid(db, nPosition, cur, "bid cancelled")) {
			sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
			return;
		}
		sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
		DeliverNewMail(cur.character_id);
		snprintf(line, sizeof line, "AvA: bid on territory %u cancelled, %lld credits returned by mail.",
			AvaZones::Local(nPosition), (long long)cur.amount);
		ChatSession::SystemMessageTo(player_name, line);
		return;
	}

	if (owns_ava_territory(nPosition)) {
		ChatSession::SystemMessageTo(player_name, "AvA: your agency already holds territory " + AvaZones::Label(nPosition) + ".");
		return;
	}
	const int64_t floor = cur.found ? cur.amount + (cur.amount + 9) / 10 : 1000;
	if ((int64_t)nAmount < floor || nAmount > 100000000u) {
		snprintf(line, sizeof line, "AvA: the minimum bid on territory %u is %lld credits%s.", AvaZones::Local(nPosition),
			(long long)floor, cur.found ? " (10% over the standing bid)" : "");
		ChatSession::SystemMessageTo(player_name, line);
		return;
	}

	sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
	bool ok = false;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "UPDATE ga_characters SET credits = credits - ?1 "
	        "WHERE id = ?2 AND deleted_at = 0 AND credits >= ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, nAmount);
		sqlite3_bind_int64(s, 2, selected_character_id_);
		ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
		sqlite3_finalize(s);
	}
	if (!ok) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		snprintf(line, sizeof line, "AvA: not enough credits for a %u credit bid.", nAmount);
		ChatSession::SystemMessageTo(player_name, line);
		return;
	}
	if (cur.found) {
		ok = ava_refund_bid(db, nPosition, cur, cur.owner == me ? "replaced by your higher bid" : "outbid");
		refunded_to = cur.character_id;
	}
	if (ok && sqlite3_prepare_v2(db, "INSERT INTO ga_ava_bids (zone_id, position, owner_agency_id, character_id, "
	        "amount, attack_type, placed_at) VALUES (1, ?1, ?2, ?3, ?4, ?5, strftime('%s','now'))", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		sqlite3_bind_int64(s, 2, me);
		sqlite3_bind_int64(s, 3, selected_character_id_);
		sqlite3_bind_int64(s, 4, nAmount);
		sqlite3_bind_int(s, 5, (int)nAttackType);
		ok = sqlite3_step(s) == SQLITE_DONE;
		sqlite3_finalize(s);
	} else ok = false;
	if (!ok) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		return;
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	if (refunded_to) DeliverNewMail(refunded_to);
	send_mail_notify(0, true);  // restate the wallet
	Logger::Log("tcp", "[TcpSession] AvA bid pos %u: %u credits by char %lld (owner %lld)%s\n", nPosition, nAmount,
		(long long)selected_character_id_, (long long)me, cur.found ? " (previous bid refunded)" : "");
	snprintf(line, sizeof line, "AvA: you hold the top bid on territory %u (%u credits). Only your agency can launch the "
		"attack there; launching it spends the bid.", AvaZones::Local(nPosition), nAmount);
	ChatSession::SystemMessageTo(player_name, line);
}

// HEX_CAPTURE gate: a standing bid reserves the attack for the bidder's owner; launching spends it.
bool TcpSession::ava_bid_blocks_attack(uint32_t nPosition)
{
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	const AvaBid cur = ava_standing_bid(db, nPosition);
	if (!cur.found) return false;
	if (cur.owner != ava_owner_of_character(db, selected_character_id_) &&
	    AvaParity::OpenBidOwnerFor(nPosition, selected_character_id_) == cur.owner) {  // opened won bid
		ChatSession::SystemMessageTo(player_name, "AvA: you join the attack on territory " + AvaZones::Label(nPosition) +
			" for the agency that won its auction - a win takes the territory for them, you keep your match rewards.");
		return false;  // the bid stays until the match result
	}
	if (cur.owner != ava_owner_of_character(db, selected_character_id_)) {
		char line[200];
		snprintf(line, sizeof line, "AvA: territory %u is reserved by %s's winning bid (%lld credits). Outbid it to attack.",
			AvaZones::Local(nPosition), cur.name.c_str(), (long long)cur.amount);
		ChatSession::SystemMessageTo(player_name, line);
		return true;
	}
	// Own won bid: spent by the match result (PlayerSessionStore), so a missed popup or Decline can launch again.
	return false;
}

// True when territory (zone 1, nPosition) belongs to this agent's agency, or to the
// agent alone (solo owner id 0x40000000 + character id, see ResolveAvaMatch).
bool TcpSession::owns_ava_territory(uint32_t nPosition)
{
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* s = nullptr;
	bool owned = false;
	if (db && sqlite3_prepare_v2(db,
	        "SELECT 1 FROM ga_ava_territories t, ga_characters c "
	        "LEFT JOIN ga_agency_members m ON m.user_id = c.user_id "
	        "WHERE t.zone_id = 1 AND t.position = ?1 AND c.id = ?2 "
	        "AND (t.owner_agency_id = ?3 + c.id OR t.owner_agency_id = m.agency_id)",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)nPosition);
		sqlite3_bind_int64(s, 2, selected_character_id_);
		sqlite3_bind_int64(s, 3, 0x40000000);
		owned = sqlite3_step(s) == SQLITE_ROW;
		sqlite3_finalize(s);
	}
	return owned;
}

void TcpSession::send_add_player_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::ADD_PLAYER_CHARACTER;
	uint16_t item_count = 6;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	Write4B(response, GA_T::PROFILE_ID, selected_profile_id_);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(selected_profile_id_).classTypeValueId);

	Write4B(response, GA_T::CLASS_MSG_ID,
		ResolveClassMsgId(selected_profile_id_, 22976, "ADD_PLAYER_CHARACTER"));
	Write4B(response, GA_T::HOME_MAP_GAME_ID, ResolveHomeMapGameId());

	send_response(response);
}

void TcpSession::send_select_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_SELECT_CHARACTER;
	uint16_t item_count = 13;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::ERROR_CODE, 0x0);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, ResolveHomeMapGameId());
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	const uint32_t profile_id = selected_profile_id_ != 0 ? selected_profile_id_ : GA_G::PROFILE_ID_ASSAULT;
	Write4B(response, GA_T::PROFILE_ID, profile_id);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(profile_id).classTypeValueId);
	WriteString(response, GA_T::CLASS_MSG_ID,
		std::to_string(ResolveClassMsgId(profile_id, 22976, "GSC_SELECT_CHARACTER")));
	// Agent progression: same level/xp fields as GSC_CHARACTER_LIST.
	const auto lx = PlayerSessionStore::GetCharacterLevelXp(selected_character_id_);
	Write4B(response, GA_T::CHARACTER_LEVEL, lx.level);
	Write4B(response, GA_T::CURRENT_LEVEL,   lx.level);
	Write4B(response, GA_T::XP_VALUE,        static_cast<uint32_t>(lx.xp));

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	Logger::Log("tcp", "[%s] send_select_character_response: charId=%lld profile=%u session_guid=%s\n",
		Logger::GetTime(), selected_character_id_, selected_profile_id_, session_guid_.c_str());

	// TODO Phase 8: get instance address from instance registry
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);

	send_response(response);
}

void TcpSession::send_change_map_prep_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::CHANGE_MAP_PREP;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_change_map_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHANGE_MAP;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_match_launch_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_LAUNCH_MAP_INSTANCE;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_go_play_tutorial_response()
{
	auto instance = InstanceRegistry::GetReadyHomeInstance();
	if (!instance) {
		Logger::Log("tcp", "[TcpSession] No READY home map instance for go_play_tutorial\n");
		return;
	}

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 10;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteIP(response, GA_T::HOST_NET_ADDR, instance->ip_address, instance->udp_port);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	WriteString(response, GA_T::MAP_FILENAME, "Inception_ALL");
	WriteString(response, GA_T::PARAMETERS, "?Game=TgGame.TgGame_Mission");
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 22845);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, 5289);
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);

	send_response(response);
}

void TcpSession::send_marshal_channel_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MARSHAL_CHANNEL;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));
	Write4B(response, GA_T::VERSION, 4869);
	WriteString(response, GA_T::TEXT_VALUE, player_name);

	send_response(response);
}

void TcpSession::send_go_play_response()
{
	auto instance = InstanceRegistry::GetReadyHomeInstance();
	if (!instance) {
		Logger::Log("tcp", "[TcpSession] No READY home map instance for go_play\n");
		return;
	}
	send_go_play_to_instance(*instance, home_task_force_);
}

void TcpSession::send_go_play_to_instance(const InstanceInfo& target, int task_force, uint32_t title_msg_id)
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint32_t map_type = Inspect::MapTypeFor(target.game_mode);  // X inspect
	if (!map_type) map_type = AssignmentMapType::ForQueue(target.queue_id);  // simplified end screen
	uint16_t item_count = map_type ? 14 : 13;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Resolve map metadata from the preloaded map_game_info registry. Falls
	// back to the legacy hardcoded HQ values if the map isn't in the table
	// (NULL map_name rows, manually-added maps, etc.) — preserves the old
	// behavior for any unseeded map.
	uint32_t friendly_name_msg_id          = 22845;   // Commonwealth HQ (legacy fallback)
	uint32_t map_game_id                   = 0x050B;  // legacy fallback
	uint32_t entry_background_image_res_id = 5716;    // legacy fallback
	if (auto info = MapGameInfo::LookupByName(target.map_name)) {
		friendly_name_msg_id          = info->friendly_name_msg_id;
		map_game_id                   = info->map_game_id;
		entry_background_image_res_id = info->entry_background_image_res_id;
	} else {
		Logger::Log("tcp", "[TcpSession] send_go_play_to_instance: no map_game_info row "
			"for map_name='%s'; falling back to HQ defaults\n", target.map_name.c_str());
	}
	if (title_msg_id != 0) friendly_name_msg_id = title_msg_id;

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteIP(response, GA_T::HOST_NET_ADDR, target.ip_address, target.udp_port);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	WriteString(response, GA_T::MAP_FILENAME, target.map_name);
	WriteString(response, GA_T::PARAMETERS, "?Game=" + target.game_mode);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, friendly_name_msg_id);
	Write4B(response, GA_T::MAP_GAME_ID, map_game_id);
	Write4B(response, GA_T::MAP_INSTANCE_ID, static_cast<uint32_t>(target.instance_id));
	Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, entry_background_image_res_id);
	Write4B(response, GA_T::TASK_FORCE, static_cast<uint32_t>(task_force));
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	if (map_type) Write4B(response, GA_T::MAP_TYPE, map_type);

	const uint32_t profile_id = selected_profile_id_ != 0 ? selected_profile_id_ : GA_G::PROFILE_ID_ASSAULT;
	const auto& class_config = GetClassConfig(profile_id);
	const uint32_t class_msg_id = ResolveClassMsgId(profile_id, 22976, "GSC_GO_PLAY");
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, class_config.classTypeValueId);
	Write4B(response, GA_T::CLASS_MSG_ID, class_msg_id);

	send_response(response);
	Logger::Log("tcp", "[TcpSession] Sent GSC_GO_PLAY → instance=%lld map=%s game_id=%u name_msg=%u bg_res=%u class_type=%u class_msg=%u tf=%d for %s\n",
		(long long)target.instance_id, target.map_name.c_str(),
		map_game_id, friendly_name_msg_id, entry_background_image_res_id,
		class_config.classTypeValueId, class_msg_id, task_force, session_guid_.c_str());
}

void TcpSession::send_instance_ready_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MATCH_LAUNCH_TIMEOUT;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::MSG_ID, 33528);

	send_response(response);
}


void TcpSession::send_character_list_response()
{
	auto characters = PlayerSessionStore::GetCharactersByUserId(user_id_);

	// Collect equipped cosmetics (helmet, suit, dyes, trail) for all characters.
	struct CosmeticEntry { uint32_t char_id; int item_id; int slot_value_id; };
	std::vector<CosmeticEntry> cosmetics;
	for (const auto& c : characters) {
		auto devices = PlayerSessionStore::GetDevicesForCharacter(c.id, c.current_item_profile_id);
		for (const auto& d : devices) {
			if (d.item_id > 0)
				cosmetics.push_back({ static_cast<uint32_t>(c.id), d.item_id, d.slot_value_id });
		}
	}

	// Head-morph poses, decoded from the (node index, weight) uint32 pairs the
	// client sent at creation. TgUICharacterSelectScene.UpdateCustomModel feeds
	// these to the preview model via PopulateCurrentMorphSettings — the skin
	// tone rides along with them (the head menu's colour sliders are morph
	// nodes: EHeadMorphGroups has HMG_Colors / HMG_HairColors), which is why
	// the assembly's SkinToneParameterId alone can't carry it.
	struct MorphEntry { uint32_t char_id; uint32_t node_index; uint32_t weight; };
	std::vector<MorphEntry> morphs;
	for (const auto& c : characters) {
		const size_t pairs = c.morph_data.size() / 8;
		for (size_t i = 0; i < pairs; ++i) {
			uint32_t idx = 0, weight = 0;
			std::memcpy(&idx,    c.morph_data.data() + i * 8,     4);
			std::memcpy(&weight, c.morph_data.data() + i * 8 + 4, 4);
			morphs.push_back({ static_cast<uint32_t>(c.id), idx, weight });
		}
	}

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 4;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name.c_str());

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, static_cast<uint8_t>(characters.size() & 0xFF),
	                 static_cast<uint8_t>(characters.size() >> 8));

	for (const auto& c : characters) {
		const auto lx = PlayerSessionStore::GetCharacterLevelXp(c.id);
		append(response, 0x0E, 0x00);  // inner item count = 14
		Write4B(response, GA_T::CHARACTER_ID,               static_cast<uint32_t>(c.id));
		Write4B(response, GA_T::CHARACTER_LEVEL,            lx.level);
		Write4B(response, GA_T::CURRENT_LEVEL,              lx.level);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL,     lx.level);
		Write4B(response, GA_T::PROFILE_ID,                 c.profile_id);
		Write4B(response, GA_T::CLASS_TYPE_VALUE_ID,        GetClassConfig(c.profile_id).classTypeValueId);
		Write4B(response, GA_T::HEAD_ASM_ID,                c.head_asm_id);
		Write4B(response, GA_T::HAIR_ASM_ID,                c.hair_asm_id);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID,       c.gender_type_value_id);
		// Per-character "Location" label (CharacterSelectEntry.cseLocation). We
		// don't track a last-played map, so send empty rather than a placeholder.
		WriteString(response, GA_T::MAP_NAME,               "");
		Write4B(response, GA_T::XP_VALUE,                   static_cast<uint32_t>(lx.xp));
		Write4B(response, GA_T::SKILL_GROUP_SET_ID,         GetClassConfig(c.profile_id).skillGroupSetId);
		Write4B(response, GA_T::SKIN_MATERIAL_PARAMETER_ID, c.skin_mat_param_id);
		Write4B(response, GA_T::EYE_MATERIAL_PARAMETER_ID,  c.eye_mat_param_id);
	}

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, static_cast<uint8_t>(cosmetics.size() & 0xFF),
	                 static_cast<uint8_t>(cosmetics.size() >> 8));
	for (const auto& entry : cosmetics) {
		append(response, 0x03, 0x00);  // 3 fields per record: CHARACTER_ID, ITEM_ID, EQUIPPED_SLOT_VALUE_ID
		Write4B(response, GA_T::CHARACTER_ID,           entry.char_id);
		Write4B(response, GA_T::ITEM_ID,                static_cast<uint32_t>(entry.item_id));
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, static_cast<uint32_t>(entry.slot_value_id));
	}

	append(response, GA_T::DATA_SET_CHAR_MORPH_SETTINGS & 0xFF, GA_T::DATA_SET_CHAR_MORPH_SETTINGS >> 8);
	append(response, static_cast<uint8_t>(morphs.size() & 0xFF),
	                 static_cast<uint8_t>(morphs.size() >> 8));
	for (const auto& m : morphs) {
		append(response, 0x03, 0x00);  // 3 fields: CHARACTER_ID, MORPH_NODE_INDEX, MORPH_NODE_WEIGHT
		Write4B(response, GA_T::CHARACTER_ID,      m.char_id);
		Write4B(response, GA_T::MORPH_NODE_INDEX,  m.node_index);
		Write4B(response, GA_T::MORPH_NODE_WEIGHT, m.weight);
	}

	send_response(response);
}

void TcpSession::send_character_list_response_mock()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name.c_str());

	// ==== arrayofenumblockarrays (010C) ====
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x04, 0x00);        // count elements

	// ASSAULT
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_ASSAULT).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_ASSAULT);

	// MEDIC
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x0000f69c);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_MEDIC);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_MEDIC).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Commonwealth HQ");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_MEDIC);

	// RECON
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_RECON);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_RECON).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_RECON);

	// ROBO
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ROBOTIC);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_ROBOTIC).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "alsdfslfjlj");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_ROBOTIC);

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, 0x00, 0x00);        // count elements

	send_response(response);
}

void TcpSession::send_character_list_queue_response(uint32_t position, uint32_t est_minutes,
                                                    uint32_t queue_length)
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// MIN_WAIT_SEC must be non-zero or the client treats it as "not queued".
	Write4B(response, GA_T::MIN_WAIT_SEC, est_minutes ? est_minutes : 1);
	Write4B(response, GA_T::ACTIVE_POSITION, position);
	Write4B(response, GA_T::ENTRY_NUMBER, queue_length);

	send_response(response);
}


void TcpSession::send_login_response_for(const std::string& response_player_name,
                                         const std::string& response_session_guid,
                                         bool success,
                                         const char* error_text,
                                         uint32_t error_msg_id)
{
	std::vector<uint8_t> response;

	// MSG_ID is what the client actually displays on failure (FinishLogin's
	// error path Translates it); ERROR_TEXT/DESC are not shown by the login menu.
	const bool write_msg_id = error_text && error_msg_id != 0;

	uint16_t packet_type = GA_U::GSC_USER_LOGIN_RESPONSE;
	uint16_t item_count = error_text ? 12 : 9;
	if (write_msg_id) item_count += 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write1B(response,     GA_T::SUCCESS, success ? 1 : 0);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(response_session_guid));
	Write4B(response,     GA_T::GAME_BITS, 0x0000000F);
	// NET_ACCESS_FLAGS is a TYPE_TCP_UINT64: written as 2 bytes the client read 8 and
	// swallowed AFK_TIMEOUT_SEC (flags = 0x001CF3F8). The client's feature gates test
	// its low dword (UTgUICraftScene.ValidateCraftingFeature = 0x200000, others in
	// 0x1FFF000 / 0x3F0); all on = Elite Agent for every account (free-to-play).
	Write8B(response,     GA_T::NET_ACCESS_FLAGS, 0x01FFF3F8ULL);
	Write4B(response,     GA_T::AFK_TIMEOUT_SEC, 0x00000384);
	WriteNBytes(response, GA_T::DISPLAY_EULA_FLAG, {0x01, 0x00, 0x6E});
	WriteString(response, GA_T::PLAYER_NAME, response_player_name.c_str());

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);        // count elements

	append(response, 0x02, 0x00);  // inner item count
	Write4B(response, GA_T::SYS_SITE_ID, 0x00000002);
	Write4B(response, GA_T::NAME_MSG_ID, 0x0000A20B);

	append(response, GA_T::DATA_SET_MAP_CONFIGS & 0xFF, GA_T::DATA_SET_MAP_CONFIGS >> 8);
	append(response, 0x01, 0x00);    // 1 item

	append(response, 0x02, 0x00);  // inner item count
	Write4B(response, GA_T::MAP_GAME_ID, 0x0000046B);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	if (error_text) {
		Write4B(response, GA_T::ERROR_CODE, 1);
		WriteString(response, GA_T::ERROR_TEXT, error_text);
		WriteString(response, GA_T::ERROR_DESC, error_text);
	}
	if (write_msg_id) {
		Write4B(response, GA_T::MSG_ID, error_msg_id);
	}

	send_response(response);
}

void TcpSession::send_login_rejected_response(const char* error_text, uint32_t msg_id)
{
	send_login_response_for("", "00000000000000000000000000000000", false, error_text, msg_id);
}

void TcpSession::send_login_response()
{
	send_login_response_for(player_name, session_guid_);
}

// ===========================================================================
// User moderation — login-bug spoof, live-kick, online snapshot.
// ===========================================================================

void TcpSession::ApplyBanSpoof() {
	if (s_ban_spoof_mode_ == "garbage") {
		std::vector<uint8_t> garbage(32);
		for (auto& b : garbage) b = static_cast<uint8_t>(std::rand() & 0xFF);
		enqueue_write(std::move(garbage));
		close_after_login_rejection();
		return;
	}
	// "silent" (default): do nothing. The TcpSession sits idle in do_read.
	// The client's own ~10s read timeout fires and surfaces "Unable to
	// connect to server" — matches the historical login-bug surface.
	if (s_ban_spoof_fallback_close_sec_ > 0) {
		auto self = shared_from_this();
		auto t = std::make_shared<asio::steady_timer>(io_ctx_);
		t->expires_after(std::chrono::seconds(s_ban_spoof_fallback_close_sec_));
		t->async_wait([self, t](std::error_code ec) {
			if (!ec) self->close_after_login_rejection();
		});
	}
}

void TcpSession::send_kick_to_bogus_map() {
	// Slim sibling of send_go_play_response() — same opcode + the minimum
	// set of fields the client expects on a GO_PLAY frame, but the map
	// name is one no client will resolve. Client tries to switch maps,
	// fails, and falls into its local disconnect path (which closes the
	// control TCP socket → existing do_read error branch sweeps the rest).
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteIP(response, GA_T::HOST_NET_ADDR, "999.999.999.999", 0);
	WriteString(response, GA_T::MAP_FILENAME, "Login_FreeAgent");


	send_response(response);

	Logger::Log("ban", "[ban] kick guid=%s user_id=%lld name=%s ip=%s\n",
		session_guid_.c_str(), (long long)user_id_,
		player_name.c_str(), ip_address_.c_str());
}

void TcpSession::arm_kick_fallback_timer() {
	if (s_kick_fallback_close_sec_ <= 0) return;
	auto self = shared_from_this();
	auto t = std::make_shared<asio::steady_timer>(io_ctx_);
	t->expires_after(std::chrono::seconds(s_kick_fallback_close_sec_));
	t->async_wait([self, t](std::error_code ec) {
		if (!ec) self->close_after_login_rejection();
	});
}

int TcpSession::KickSessionsForUser(int64_t user_id) {
	if (user_id <= 0) return 0;
	// Snapshot under lock — we'll mutate session state through
	// MatchmakingService / write the socket outside the lock.
	std::vector<std::shared_ptr<TcpSession>> targets;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) {
			auto s = weak.lock();
			if (s && s->user_id_ == user_id) targets.push_back(s);
		}
	}
	for (auto& s : targets) {
		MatchmakingService::RemovePlayer(s->session_guid_);
		s->send_kick_to_bogus_map();
		s->arm_kick_fallback_timer();
	}
	return (int)targets.size();
}

int TcpSession::KickSessionsForIp(const std::string& ip) {
	if (ip.empty()) return 0;
	std::vector<std::shared_ptr<TcpSession>> targets;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) {
			auto s = weak.lock();
			if (!s) continue;
			// ip_address_ is "1.2.3.4:port" — match the host octet quad,
			// not the port.
			if (s->ip_address_.rfind(ip + ":", 0) == 0) targets.push_back(s);
		}
	}
	for (auto& s : targets) {
		MatchmakingService::RemovePlayer(s->session_guid_);
		s->send_kick_to_bogus_map();
		s->arm_kick_fallback_timer();
	}
	return (int)targets.size();
}

void TcpSession::ForEachLiveSession(const std::function<void(const OnlineSnapshot&)>& visit) {
	std::vector<OnlineSnapshot> snaps;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) {
			auto s = weak.lock();
			if (!s) continue;
			OnlineSnapshot snap;
			snap.session_guid = s->session_guid_;
			snap.user_id      = s->user_id_;
			snap.username     = s->player_name;
			const std::string& ipp = s->ip_address_;
			auto colon = ipp.find(':');
			snap.ip = (colon == std::string::npos) ? ipp : ipp.substr(0, colon);
			snap.instance_id  = s->assigned_instance_id_;
			snap.login_at     = s->session_login_at_;
			snaps.push_back(std::move(snap));
		}
	}
	for (const auto& s : snaps) visit(s);
}


// ---- Agency applications. Client sender 0x10923030: AGENCY_APPLICATION 0x194 {ACTION_FLAGS,
// [AGENCY_ID (2,3) | CHARACTER_ID (4,5)], [APPLICATION_TEXT]}. Replies are 0x194 too: to the directory (actions 1-3)
// {STATUS_CODE 0/2/4/5, CURRENT_AGENCY_ID, [DATA_SET {AGENCY_ID, AGENCY_NAME}]}; to the Recruiting tab (4-6)
// DATA_SET rows {CHARACTER_ID, APPLICATION_DATETIME, APPLICATION_TEXT, NAME, PROFILE_ID, XP_VALUE, STAT_RANK}.
void TcpSession::send_agency_application_status()
{
	const int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	auto app = AgencyApply::Latest(user_id_);
	std::vector<uint8_t> r;
	const uint16_t pt = GA_U::AGENCY_APPLICATION;
	append(r, pt & 0xFF, pt >> 8);
	append(r, app ? 0x03 : 0x02, 0x00);
	Write4B(r, GA_T::STATUS_CODE, app ? (uint32_t)app->state : 0u);
	Write4B(r, GA_T::CURRENT_AGENCY_ID, (uint32_t)my_agency);
	if (app) {
		append(r, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
		append(r, 0x01, 0x00);
		append(r, 0x02, 0x00);
		Write4B(r, GA_T::AGENCY_ID, (uint32_t)app->agency_id);
		WriteString(r, GA_T::AGENCY_NAME, app->agency_name);
	}
	Logger::Log("agency", "[TCP] AGENCY_APPLICATION status -> user %lld state %d agency %lld\n",
		(long long)user_id_, app ? app->state : 0, app ? (long long)app->agency_id : 0LL);
	send_response(r);
}

void TcpSession::send_agency_applicants()
{
	const int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	const auto apps = my_agency ? AgencyApply::Pending(my_agency) : std::vector<AgencyApply::App>{};
	std::vector<uint8_t> r;
	const uint16_t pt = GA_U::AGENCY_APPLICATION;
	append(r, pt & 0xFF, pt >> 8);
	append(r, 0x01, 0x00);
	append(r, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(r, apps.size() & 0xFF, (apps.size() >> 8) & 0xFF);
	for (const auto& a : apps) {
		append(r, 0x07, 0x00);
		Write4B(r, GA_T::CHARACTER_ID, (uint32_t)a.character_id);
		WriteDouble(r, GA_T::APPLICATION_DATETIME, ((double)a.applied_at + 2208988800.0) / 86400.0);  // days since 1900 (as CREATED_DATETIME)
		WriteString(r, GA_T::APPLICATION_TEXT, a.text);
		WriteString(r, GA_T::NAME, a.name);
		Write4B(r, GA_T::PROFILE_ID, a.profile_id);
		Write4B(r, GA_T::XP_VALUE, (uint32_t)std::max<int64_t>(0, a.xp));
		WriteFloat(r, GA_T::STAT_RANK, 0.0f);
	}
	Logger::Log("agency", "[TCP] AGENCY_APPLICATION list -> user %lld agency %lld: %zu applicant(s)\n",
		(long long)user_id_, (long long)my_agency, apps.size());
	send_response(r);
}

void TcpSession::handle_agency_application(const PacketView& pkt)
{
	const uint32_t action = pkt.Read4B(GA_T::ACTION_FLAGS).value_or(0);
	const int64_t agency_id = (int64_t)pkt.Read4B(GA_T::AGENCY_ID).value_or(0);
	const int64_t char_id = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
	const std::string text = pkt.ReadString(GA_T::APPLICATION_TEXT).value_or("");
	Logger::Log("agency", "[TCP] AGENCY_APPLICATION action=%u agency=%lld char=%lld user=%lld\n",
		action, (long long)agency_id, (long long)char_id, (long long)user_id_);
	if (user_id_ <= 0) return;
	const int64_t my_agency = Database::GetAgencyIdForUser(user_id_);
	switch (action) {
	case 1: send_agency_application_status(); return;
	case 2: {
		auto target = agency_id > 0 ? Database::GetAgencyInfo(agency_id) : std::nullopt;
		if (my_agency != 0) {
			ChatSession::SystemMessageTo(player_name, "You are already in an agency - leave it before applying to another.");
		} else if (!target || !target->recruiting) {
			ChatSession::SystemMessageTo(player_name, "That agency is not recruiting right now.");
		} else if (AgencyApply::Apply(agency_id, user_id_, selected_character_id_, player_name, text)) {
			ChatSession::AgencyMessage(agency_id, player_name + " applied to join the agency (Agency > Recruiting).");
		}
		send_agency_application_status();
		return;
	}
	case 3: AgencyApply::Cancel(user_id_); send_agency_application_status(); return;
	case 4: case 5: {
		auto info = my_agency ? Database::GetAgencyInfo(my_agency) : std::nullopt;
		if (!info || !agency_has_perm(*info, user_id_, Database::AGENCY_PERM_INVITE)) {
			send_agency_message(14258, player_name);  // "Your rank does not allow..."
			send_agency_applicants();
			return;
		}
		auto app = AgencyApply::PendingFor(my_agency, char_id);
		if (!app) { send_agency_applicants(); return; }
		if (action == 5) {
			AgencyApply::SetState(app->id, AgencyApply::kDeclined);
			ChatSession::SystemMessageTo(app->name, "Your application to " + info->name + " was declined.");
		} else if (Database::GetAgencyIdForUser(app->user_id) != 0) {
			AgencyApply::SetState(app->id, AgencyApply::kDeclined);
			ChatSession::SystemMessageTo(player_name, app->name + " has joined another agency already.");
		} else if (Database::AddAgencyMember(my_agency, app->user_id, app->character_id, app->name, 2)) {
			AgencyApply::SetState(app->id, AgencyApply::kAccepted);
			ChatSession::AgencyMessage(my_agency, app->name + " has joined the agency.");
			DeliverAgencyRefresh(app->user_id);
			send_agency_get_roster_response();
		}
		send_agency_applicants();
		return;
	}
	case 6: {
		auto info = my_agency ? Database::GetAgencyInfo(my_agency) : std::nullopt;
		if (info && agency_has_perm(*info, user_id_, Database::AGENCY_PERM_INVITE)) send_agency_applicants();
		return;
	}
	default: return;
	}
}
