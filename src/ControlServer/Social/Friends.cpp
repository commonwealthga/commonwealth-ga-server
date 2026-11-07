// See Friends.hpp.
#include "src/ControlServer/Social/Friends.hpp"
#include "src/ControlServer/BugReports/BugReports.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/TcpSession/PacketView.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <map>
#include <memory>
#include <unordered_map>

namespace Social {
namespace Friends {

namespace {

constexpr uint32_t kMsgCameOnline   = 42225;   // "@@player_name@@ has come online."
constexpr uint32_t kMsgWentOffline  = 42226;   // "@@player_name@@ has gone offline."
constexpr uint32_t kSystemChannel   = 11;
constexpr int      kOfflineDelaySec = 20;      // map travel / chat reconnect inside this = no notice
constexpr int      kRefreshDelaySec = 12;      // F3 rows read the instance snapshot: wait for the join

std::string Lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

struct Stmt {
    sqlite3_stmt* s = nullptr;
    Stmt(const char* sql) {
        sqlite3* db = Database::GetConnection();
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("friends", "[Friends] prepare failed: %s\n", sqlite3_errmsg(db));
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    explicit operator bool() const { return s != nullptr; }
};

void EnsureSchema() {
    static bool done = false;
    if (done) return;
    done = true;
    const char* sql =
        "CREATE TABLE IF NOT EXISTS ga_invite_flags ("
        " user_id INTEGER PRIMARY KEY, flags INTEGER NOT NULL, updated_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS ga_chat_gags ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL, until_ts INTEGER NOT NULL,"
        " by_name TEXT NOT NULL DEFAULT '', reason TEXT NOT NULL DEFAULT '', created_at INTEGER NOT NULL);"
        "CREATE INDEX IF NOT EXISTS idx_ga_chat_gags_user ON ga_chat_gags(user_id, until_ts);";
    char* err = nullptr;
    if (sqlite3_exec(Database::GetConnection(), sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("friends", "[Friends] schema failed: %s\n", err ? err : "?");
        sqlite3_free(err);
    }
}

int64_t UserIdByName(const std::string& name) {
    Stmt st("SELECT id FROM ga_users WHERE lower(username) = lower(?) LIMIT 1");
    if (!st) return 0;
    sqlite3_bind_text(st.s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.s) == SQLITE_ROW ? sqlite3_column_int64(st.s, 0) : 0;
}

// Usernames that keep `name` on their FRIENDS list (not ignore).
std::vector<std::string> WatchersOf(const std::string& name) {
    std::vector<std::string> out;
    Stmt st("SELECT u.username FROM ga_friends f JOIN ga_users u ON u.id = f.user_id"
            " JOIN ga_users t ON t.id = f.friend_user_id"
            " WHERE lower(t.username) = lower(?) AND f.ignore_flag = 0");
    if (!st) return out;
    sqlite3_bind_text(st.s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(st.s) == SQLITE_ROW) {
        const unsigned char* t = sqlite3_column_text(st.s, 0);
        if (t) out.emplace_back(reinterpret_cast<const char*>(t));
    }
    return out;
}

// invitee keeps inviter on their friends list (not ignore)
bool IsFriendOf(const std::string& owner, const std::string& other) {
    Stmt st("SELECT 1 FROM ga_friends f JOIN ga_users u ON u.id = f.user_id JOIN ga_users t ON t.id = f.friend_user_id"
            " WHERE lower(u.username) = lower(?) AND lower(t.username) = lower(?) AND f.ignore_flag = 0 LIMIT 1");
    if (!st) return false;
    sqlite3_bind_text(st.s, 1, owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 2, other.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.s) == SQLITE_ROW;
}

bool SameAgency(const std::string& a, const std::string& b) {
    auto sa = PlayerSessionStore::GetByPlayerName(a);
    auto sb = PlayerSessionStore::GetByPlayerName(b);
    if (!sa || !sb || !sa->selected_character_id || !sb->selected_character_id) return false;
    const auto aff = Database::GetAffiliationsByCharacter();
    auto ia = aff.find(sa->selected_character_id), ib = aff.find(sb->selected_character_id);
    return ia != aff.end() && ib != aff.end() && !ia->second.agency_name.empty()
        && ia->second.agency_name == ib->second.agency_name;
}

// ---- wire helpers (narrow strings, as the rest of the chat path) ----
void Put2(std::vector<uint8_t>& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
void Put4(std::vector<uint8_t>& b, uint16_t t, uint32_t v) {
    Put2(b, t); for (int i = 0; i < 4; i++) b.push_back((v >> (8 * i)) & 0xFF);
}
void Put1(std::vector<uint8_t>& b, uint16_t t, uint8_t v) { Put2(b, t); b.push_back(v); }
void PutS(std::vector<uint8_t>& b, uint16_t t, const std::string& s) {
    Put2(b, t); Put2(b, static_cast<uint16_t>(s.size())); b.insert(b.end(), s.begin(), s.end());
}
std::vector<uint8_t> Chunk(const std::vector<uint8_t>& body) {
    std::vector<uint8_t> f; Put2(f, static_cast<uint16_t>(body.size())); f.insert(f.end(), body.begin(), body.end());
    return f;
}

// Original friend notice: announcement chat line with a localised msg id.
std::vector<uint8_t> PresenceFrame(uint32_t msg_id, const std::string& name) {
    std::vector<uint8_t> b;
    Put2(b, GA_U::CHAT_MESSAGE); Put2(b, 4);
    Put4(b, GA_T::CHAT_CH_TYPE, kSystemChannel);
    Put1(b, GA_T::CHAT_ANNOUNCEMENT_FLAG, 1);
    Put4(b, GA_T::MSG_ID, msg_id);
    PutS(b, GA_T::PLAYER_NAME, name);
    return Chunk(b);
}

// pending offline notices (name lower -> timer); a rejoin inside the window cancels it silently
std::map<std::string, std::shared_ptr<asio::steady_timer>> g_pending_offline;

void Notify(const std::string& name, bool online) {
    const auto watchers = WatchersOf(name);
    int sent = 0;
    for (const auto& w : watchers) {
        auto s = PlayerSessionStore::GetByPlayerName(w);
        if (!s) continue;   // watcher offline
        if (ChatSession::DeliverToPlayer(w, PresenceFrame(online ? kMsgCameOnline : kMsgWentOffline, name))) sent++;
    }
    Logger::Log("friends", "[Friends] presence '%s' %s -> %d/%zu watcher(s) told\n",
        name.c_str(), online ? "online" : "offline", sent, watchers.size());
}

void RefreshWatchers(const std::string& name) {
    int n = 0;
    for (const auto& w : WatchersOf(name)) {
        if (auto s = PlayerSessionStore::GetByPlayerName(w)) {
            if (TcpSession::RefreshFriendsList(s->session_guid)) n++;
        }
    }
    Logger::Log("friends", "[Friends] F3 refresh for watchers of '%s': %d session(s)\n", name.c_str(), n);
}

void RefreshLater(const std::string& name, const asio::any_io_executor& ex) {
    auto t = std::make_shared<asio::steady_timer>(ex, std::chrono::seconds(kRefreshDelaySec));
    t->async_wait([t, name](std::error_code ec) { if (!ec) RefreshWatchers(name); });
}

// ---- AFK ----
std::unordered_map<std::string, std::string> g_afk;   // lower name -> display name

} // namespace

void LogFriendPacket(const char* what, int64_t user_id, const std::string& player_name) {
    Logger::Log("friends", "[Friends] %s user=%lld player='%s'\n", what, (long long)user_id, player_name.c_str());
}

void OnOnline(const std::string& name, const asio::any_io_executor& ex) {
    if (name.empty()) return;
    EnsureSchema();
    auto it = g_pending_offline.find(Lower(name));
    if (it != g_pending_offline.end()) {           // came back inside the window: no offline, no online
        it->second->cancel();
        g_pending_offline.erase(it);
        Logger::Log("friends", "[Friends] presence '%s' reconnect inside %ds, no notice\n", name.c_str(), kOfflineDelaySec);
        RefreshLater(name, ex);
        return;
    }
    Notify(name, true);
    RefreshWatchers(name);
    RefreshLater(name, ex);      // again once the agent is in a map (map/class/level on the row)
}

void OnOffline(const std::string& name, const asio::any_io_executor& ex) {
    if (name.empty()) return;
    g_afk.erase(Lower(name));
    const std::string key = Lower(name);
    auto t = std::make_shared<asio::steady_timer>(ex, std::chrono::seconds(kOfflineDelaySec));
    g_pending_offline[key] = t;
    t->async_wait([t, name, key](std::error_code ec) {
        if (ec) return;
        auto it = g_pending_offline.find(key);
        if (it == g_pending_offline.end() || it->second != t) return;
        g_pending_offline.erase(it);
        if (PlayerSessionStore::GetByPlayerName(name)) {   // still logged in on the game socket (chat-only drop)
            Logger::Log("friends", "[Friends] presence '%s' chat left but session alive, no notice\n", name.c_str());
            return;
        }
        Notify(name, false);
        RefreshWatchers(name);
    });
}

// ---- /afk ----
void ToggleAfk(const std::string& guid) {
    auto s = PlayerSessionStore::GetByGuid(guid);
    if (!s || s->player_name.empty()) return;
    const std::string key = Lower(s->player_name);
    if (g_afk.erase(key)) {
        ChatSession::SystemMessageToGuid(guid, "You are no longer AFK.");
        Logger::Log("friends", "[Friends] afk off '%s' (/afk)\n", s->player_name.c_str());
    } else {
        g_afk[key] = s->player_name;
        ChatSession::SystemMessageToGuid(guid, "You are now AFK. Type /afk again or chat to come back.");
        Logger::Log("friends", "[Friends] afk on '%s'\n", s->player_name.c_str());
    }
}

void SetAfk(const std::string& guid, bool afk) {
    auto s = PlayerSessionStore::GetByGuid(guid);
    if (!s || s->player_name.empty()) {
        Logger::Log("friends", "[Friends] afk_state %d: no session for guid %s\n", (int)afk, guid.c_str());
        return;
    }
    const std::string key = Lower(s->player_name);
    const bool was = g_afk.count(key) != 0;
    if (afk == was) return;   // game server echoes; nothing changed
    if (afk) {
        g_afk[key] = s->player_name;
        ChatSession::SystemMessageToGuid(guid, "You are now AFK.");
    } else {
        g_afk.erase(key);
        ChatSession::SystemMessageToGuid(guid, "You are no longer AFK.");
    }
    Logger::Log("friends", "[Friends] afk %s '%s' (game ServerToggleAFK)\n", afk ? "on" : "off", s->player_name.c_str());
}

void Activity(const std::string& name) {
    if (g_afk.empty() || name.empty()) return;
    if (g_afk.erase(Lower(name))) {
        ChatSession::SystemMessageTo(name, "You are no longer AFK.");
        Logger::Log("friends", "[Friends] afk off '%s' (activity)\n", name.c_str());
    }
}

bool IsAfk(const std::string& name) { return !g_afk.empty() && g_afk.count(Lower(name)) != 0; }

std::string AfkAutoReply(const std::string& target) {
    if (!IsAfk(target)) return "";
    return g_afk[Lower(target)] + " - AFK";    // client string "%s - AFK"
}

// ---- invite flags ----
void SetInviteFlags(int64_t user_id, uint32_t flags) {
    if (user_id <= 0) return;
    EnsureSchema();
    Stmt st("INSERT INTO ga_invite_flags(user_id, flags, updated_at) VALUES(?,?,strftime('%s','now'))"
            " ON CONFLICT(user_id) DO UPDATE SET flags = excluded.flags, updated_at = excluded.updated_at");
    if (!st) return;
    sqlite3_bind_int64(st.s, 1, user_id);
    sqlite3_bind_int64(st.s, 2, static_cast<int64_t>(flags));
    sqlite3_step(st.s);
}

uint32_t GetInviteFlags(int64_t user_id) {
    EnsureSchema();
    Stmt st("SELECT flags FROM ga_invite_flags WHERE user_id = ?");
    if (!st) return kInviteDefault;
    sqlite3_bind_int64(st.s, 1, user_id);
    return sqlite3_step(st.s) == SQLITE_ROW ? static_cast<uint32_t>(sqlite3_column_int64(st.s, 0)) : kInviteDefault;
}

void HandleSetInviteFlags(int64_t user_id, const PacketView& pkt) {
    auto f = pkt.Read4B(GA_T::INVITE_FLAGS);
    Logger::Log("friends", "[Friends] SET_INVITE_FLAGS user=%lld flags=%s0x%X\n",
        (long long)user_id, f ? "" : "(missing) ", f ? *f : 0u);
    if (f) SetInviteFlags(user_id, *f);
}

std::vector<uint8_t> BuildInviteFlagsReply(int64_t user_id) {
    std::vector<uint8_t> b;
    Put2(b, GA_U::GET_INVITE_FLAGS); Put2(b, 1);
    const uint32_t f = GetInviteFlags(user_id);
    Put4(b, GA_T::INVITE_FLAGS, f);
    Logger::Log("friends", "[Friends] GET_INVITE_FLAGS user=%lld -> 0x%X\n", (long long)user_id, f);
    return b;
}

void InvitesCommand(const std::string& guid, const std::string& args) {
    auto s = PlayerSessionStore::GetByGuid(guid);
    if (!s || s->user_id <= 0) return;
    std::vector<std::string> w;
    { std::string cur; for (char c : args + " ") { if (c == ' ' || c == '\t') { if (!cur.empty()) w.push_back(Lower(cur)); cur.clear(); } else cur += c; } }
    struct K { const char* name; uint32_t bit; } kinds[] = { {"agency", 1u}, {"alliance", 2u}, {"team", 4u}, {"strikeforce", 8u} };
    uint32_t f = GetInviteFlags(s->user_id);
    if (!w.empty()) {
        std::string what = w.size() >= 2 ? w[0] : "all", onoff = w.back();
        if (onoff != "on" && onoff != "off") {
            ChatSession::SystemMessageToGuid(guid, "Usage: -invites [team|agency|alliance|strikeforce|all] on|off");
            return;
        }
        uint32_t mask = 0;
        for (const auto& k : kinds) if (what == "all" || what == k.name || (what == "sf" && k.bit == 8u)) mask |= k.bit;
        if (!mask) { ChatSession::SystemMessageToGuid(guid, "Usage: -invites [team|agency|alliance|strikeforce|all] on|off"); return; }
        f = (onoff == "on") ? (f | mask) : (f & ~mask);
        SetInviteFlags(s->user_id, f);
        Logger::Log("friends", "[Friends] -invites '%s' %s %s -> flags 0x%X\n", s->player_name.c_str(), what.c_str(), onoff.c_str(), f);
    }
    std::string line = "Invites from anyone:";
    for (const auto& k : kinds) line += std::string(" ") + k.name + "=" + ((f & k.bit) ? "on" : "off");
    ChatSession::SystemMessageToGuid(guid, line + " (friends can always invite)");
}

bool InviteAllowed(const std::string& invitee, const std::string& inviter, InviteKind kind) {
    const int64_t uid = UserIdByName(invitee);
    if (uid <= 0) return true;
    const uint32_t f = GetInviteFlags(uid);
    uint32_t bit = 0;
    switch (kind) {
        case InviteKind::Agency:      bit = 1u << 0; break;
        case InviteKind::Alliance:    bit = 1u << 1; break;
        case InviteKind::Team:        bit = 1u << 2; break;
        case InviteKind::StrikeForce: bit = 1u << 3; break;
    }
    if (f & bit) return true;
    // "from anyone" off: friends (and agency mates for team invites) still get through
    if (IsFriendOf(invitee, inviter)) return true;
    if (kind == InviteKind::Team && SameAgency(invitee, inviter)) return true;
    Logger::Log("friends", "[Friends] invite blocked by flags: '%s' -> '%s' kind=%d flags=0x%X\n",
        inviter.c_str(), invitee.c_str(), static_cast<int>(kind), f);
    return false;
}

// ---- report / gag ----
void HandlePlayerReported(const std::string& guid, const std::string& reporter, const PacketView& pkt) {
    std::string target = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
    std::string text;
    for (uint16_t t : { (uint16_t)GA_T::MESSAGE, (uint16_t)GA_T::TEXT_VALUE, (uint16_t)GA_T::NOTES }) {
        if (auto s = pkt.ReadString(t)) { if (!s->empty()) { text = *s; break; } }
    }
    if (text.empty()) text = "(reported from the in-game report button)";
    Logger::Log("friends", "[Friends] PLAYER_REPORTED by '%s' target='%s' text='%.80s'\n",
        reporter.c_str(), target.c_str(), text.c_str());
    auto r = BugReports::Submit("player", guid, reporter, target, text);
    if (!r.reply.empty()) ChatSession::SystemMessageToGuid(guid, r.reply);
}

bool Gag(const std::string& target, int minutes, const std::string& by) {
    EnsureSchema();
    const int64_t uid = UserIdByName(target);
    if (uid <= 0) return false;
    if (minutes <= 0) minutes = 10;
    if (minutes > 7 * 24 * 60) minutes = 7 * 24 * 60;
    Stmt st("INSERT INTO ga_chat_gags(user_id, until_ts, by_name, created_at) VALUES(?, strftime('%s','now') + ?, ?, strftime('%s','now'))");
    if (!st) return false;
    sqlite3_bind_int64(st.s, 1, uid);
    sqlite3_bind_int64(st.s, 2, static_cast<int64_t>(minutes) * 60);
    sqlite3_bind_text(st.s, 3, by.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(st.s);
    Logger::Log("friends", "[Friends] gag '%s' %d min by '%s'\n", target.c_str(), minutes, by.c_str());
    ChatSession::SystemMessageTo(target, "*** You have been muted for " + std::to_string(minutes) + " minute(s) ***");
    return true;
}

void HandleGag(int64_t gm_user_id, const std::string& gm_name, const PacketView& pkt) {
    const std::string target = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
    const bool gm = gm_user_id > 0 && (Database::UserHasRole(gm_user_id, "gm") || Database::UserHasRole(gm_user_id, "dev"));
    Logger::Log("friends", "[Friends] GAG_PLAYER from '%s' target='%s' gm=%d\n", gm_name.c_str(), target.c_str(), gm ? 1 : 0);
    if (!gm) { ChatSession::SystemMessageTo(gm_name, "*** Only staff can mute players ***"); return; }
    int minutes = 10;
    for (uint16_t t : { (uint16_t)GA_T::ACTIVE_DURATION, (uint16_t)GA_T::DURATION_VALUE_ID }) {
        if (auto v = pkt.Read4B(t)) { minutes = static_cast<int>(*v); break; }
    }
    if (target.empty() || !Gag(target, minutes, gm_name))
        ChatSession::SystemMessageTo(gm_name, "*** Player '" + target + "' not found ***");
    else
        ChatSession::SystemMessageTo(gm_name, target + " muted for " + std::to_string(minutes) + " minute(s).");
}

void GagCommand(const std::string& guid, const std::string& args, bool ungag) {
    auto s = PlayerSessionStore::GetByGuid(guid);
    if (!s) return;
    const int64_t uid = s->user_id;
    const bool gm = uid > 0 && (Database::UserHasRole(uid, "gm") || Database::UserHasRole(uid, "dev"));
    std::vector<std::string> w;
    { std::string cur; for (char c : args + " ") { if (c == ' ' || c == '\t') { if (!cur.empty()) w.push_back(cur); cur.clear(); } else cur += c; } }
    Logger::Log("friends", "[Friends] -%s from '%s' args='%.60s' gm=%d\n", ungag ? "ungag" : "gag", s->player_name.c_str(), args.c_str(), gm ? 1 : 0);
    if (!gm) { ChatSession::SystemMessageToGuid(guid, "*** Only staff can mute players ***"); return; }
    if (w.empty()) { ChatSession::SystemMessageToGuid(guid, ungag ? "Usage: -ungag <player>" : "Usage: -gag <player> [minutes, default 10]"); return; }
    if (ungag) {
        EnsureSchema();
        const int64_t tid = UserIdByName(w[0]);
        Stmt st("DELETE FROM ga_chat_gags WHERE user_id = ?");
        if (tid <= 0 || !st) { ChatSession::SystemMessageToGuid(guid, "*** Player '" + w[0] + "' not found ***"); return; }
        sqlite3_bind_int64(st.s, 1, tid); sqlite3_step(st.s);
        Logger::Log("friends", "[Friends] ungag '%s' by '%s'\n", w[0].c_str(), s->player_name.c_str());
        ChatSession::SystemMessageTo(w[0], "*** You can chat again ***");
        ChatSession::SystemMessageToGuid(guid, w[0] + " can chat again.");
        return;
    }
    int minutes = 10;
    if (w.size() >= 2) { try { minutes = std::stoi(w[1]); } catch (...) {} }
    if (!Gag(w[0], minutes, s->player_name)) ChatSession::SystemMessageToGuid(guid, "*** Player '" + w[0] + "' not found ***");
    else ChatSession::SystemMessageToGuid(guid, w[0] + " muted for " + std::to_string(minutes <= 0 ? 10 : std::min(minutes, 7 * 24 * 60)) + " minute(s).");
}

bool IsGagged(const std::string& name, std::string* reply) {
    EnsureSchema();
    Stmt st("SELECT max(g.until_ts) - strftime('%s','now') FROM ga_chat_gags g JOIN ga_users u ON u.id = g.user_id"
            " WHERE lower(u.username) = lower(?) AND g.until_ts > strftime('%s','now')");
    if (!st) return false;
    sqlite3_bind_text(st.s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st.s) != SQLITE_ROW || sqlite3_column_type(st.s, 0) == SQLITE_NULL) return false;
    const int64_t left = sqlite3_column_int64(st.s, 0);
    if (reply) *reply = "*** You are muted for " + std::to_string((left + 59) / 60) + " more minute(s) ***";
    return left > 0;
}

} // namespace Friends
} // namespace Social
