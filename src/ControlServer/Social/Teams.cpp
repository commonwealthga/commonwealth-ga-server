#include <set>
// See Teams.hpp for the client wire format.
#include "src/ControlServer/Social/Teams.hpp"
#include "src/ControlServer/PvpParity/PvpParity.hpp"
#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/AvaZones/AvaZones.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/TcpSession/PacketView.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

namespace Social {
namespace Teams {
namespace {

// ---- message ids (asm_data_set_msg_translations) ----
constexpr uint32_t MSG_TEAM_FULL          = 20422; // "The team is full."
constexpr uint32_t MSG_SF_ALREADY         = 30041; // "Your team is already a strike force."
constexpr uint32_t MSG_SF_CONVERTED       = 30578; // "Your team has been converted to a strike force."
constexpr uint32_t MSG_SF_CANNOT_CREATE   = 30197; // "Unable to create a strike force."
constexpr uint32_t MSG_SF_NEED_AGENCY8    = 52167; // "You must have eight members in your agency to create a strikeforce."
constexpr uint32_t MSG_SF_NO_QUEUE        = 32417; // "...converted to a strike force.  You cannot join a matchmade queue."
constexpr uint32_t MSG_NOT_ON_SF          = 30059; // "You are not on a strike force."
constexpr uint32_t MSG_RAID_ALREADY       = 36043; // "Your team is already set as a raid group."
constexpr uint32_t MSG_NOT_RAID_LEADER    = 30714; // "You are not the raid leader."
constexpr uint32_t MSG_NOT_IN_RAID        = 30756; // "You must be in a raid to do this."
constexpr uint32_t MSG_PROMOTED_RAID_LDR  = 27160; // "@@PLAYER_NAME@@ has been promoted to raid leader."
constexpr uint32_t MSG_NOT_ONLINE         = 17787; // "Player @@PLAYER_NAME@@ not found online. ..."
constexpr uint32_t MSG_CH_PENDING         = 62679; // "You already have a pending challenge"
constexpr uint32_t MSG_CH_TARGET_BUSY     = 62681; // "The team you challenged is being challenged by someone else"
constexpr uint32_t MSG_CH_EXPIRED         = 62692; // "The challenge invite has expired"
constexpr uint32_t MSG_CH_SELF            = 63831; // "You cannot challenge yourself"
constexpr uint32_t MSG_CH_ACCEPTED        = 64062; // "You have accepted the challenge, stand-by..."
constexpr uint32_t MSG_CH_DECLINED        = 64107; // "The challenge was declined"
constexpr uint32_t MSG_CH_IN_MISSION      = 64975; // "You may not issue challenges while in a mission."
constexpr uint32_t MSG_CH_YOU_CHALLENGE   = 66484; // "You challenge @@PLAYER_NAME@@."
constexpr uint32_t MSG_NOT_LEADER         = 18305; // "You are not the team leader, only leader can manage the match queue."

constexpr int kMaxTaskTeams      = 6;     // TgUIRaidMenu RM_MAX_TEAM_COUNT
constexpr int kChallengeSeconds  = 60;

// ---- tiny TLV writers (same wire encoding as TcpSession's members) ----
void P(std::vector<uint8_t>& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
void W4(std::vector<uint8_t>& b, uint16_t t, uint32_t v) { P(b, t); for (int i = 0; i < 4; ++i) b.push_back((v >> (8 * i)) & 0xFF); }
void W1(std::vector<uint8_t>& b, uint16_t t, uint8_t v) { P(b, t); b.push_back(v); }
void WS(std::vector<uint8_t>& b, uint16_t t, const std::string& s) {
    P(b, t); const size_t n = std::min<size_t>(s.size(), 255); P(b, (uint16_t)n); b.insert(b.end(), s.begin(), s.begin() + n); }
void WD(std::vector<uint8_t>& b, uint16_t t, double d) { P(b, t); uint64_t r; std::memcpy(&r, &d, 8); for (int i = 0; i < 8; ++i) b.push_back((r >> (8 * i)) & 0xFF); }
std::vector<uint8_t> Head(uint16_t type, uint16_t items) { std::vector<uint8_t> b; P(b, type); P(b, items); return b; }

// Generic status line: same packet id + MSG_ID [+ PLAYER_NAME] -> client vt+0x54 shows the message.
void Status(const std::string& guid, uint16_t type, uint32_t msg, const std::string& name = "") {
    auto b = Head(type, name.empty() ? 1 : 2);
    W4(b, GA_T::MSG_ID, msg);
    if (!name.empty()) WS(b, GA_T::PLAYER_NAME, name);
    TcpSession::DeliverRawPacket(guid, b);
}

// ---- rules (ga_team_rules key/value; defaults = original game) ----
struct Rules { int sf_min_agency = 0; int sf_blocks_queue = 1; int squad_max = 10; int task_team_max = 10; };
std::mutex rules_mu;
Rules rules_cache;
int64_t rules_at = 0;

Rules GetRules() {
    std::lock_guard<std::mutex> l(rules_mu);
    const int64_t now = (int64_t)time(nullptr);
    if (rules_at && now - rules_at < 30) return rules_cache;
    rules_at = now;
    sqlite3* db = Database::GetConnection();
    if (!db) return rules_cache;
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ga_team_rules (key TEXT PRIMARY KEY, value INTEGER NOT NULL);"
                     "INSERT OR IGNORE INTO ga_team_rules VALUES ('sf_min_agency_members', 0), ('sf_blocks_queue', 1),"
                     " ('squad_max', 10), ('task_team_max', 10);"
                     // Ship the strike force agency minimum at 0; one-time, so a later raise sticks
                     "UPDATE ga_team_rules SET value = 0 WHERE key = 'sf_min_agency_members' AND value = 8"
                     " AND NOT EXISTS (SELECT 1 FROM ga_team_rules WHERE key = 'mig_0473_sf0');"
                     "INSERT OR IGNORE INTO ga_team_rules VALUES ('mig_0473_sf0', 1);", nullptr, nullptr, nullptr);
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT key, value FROM ga_team_rules", -1, &s, nullptr) == SQLITE_OK) {
        while (sqlite3_step(s) == SQLITE_ROW) {
            const char* k = (const char*)sqlite3_column_text(s, 0);
            const int v = sqlite3_column_int(s, 1);
            if (!k) continue;
            if (!strcmp(k, "sf_min_agency_members")) rules_cache.sf_min_agency = std::max(0, v);
            else if (!strcmp(k, "sf_blocks_queue")) rules_cache.sf_blocks_queue = v;
            else if (!strcmp(k, "squad_max")) rules_cache.squad_max = std::clamp(v, 2, 60);
            else if (!strcmp(k, "task_team_max")) rules_cache.task_team_max = std::clamp(v, 1, 12);
        }
        sqlite3_finalize(s);
    }
    return rules_cache;
}

// ---- group state per team id ----
struct Group {
    uint32_t mode = 1;                       // 2 strike force, 3 raid group
    std::map<int64_t, int> tt;               // character_id -> task team 1..6
    std::map<int, int64_t> tt_leader;        // task team -> leader character_id
};
std::mutex mu;
std::map<uint64_t, Group> groups;

struct Challenge {
    std::string from_guid, from_name, to_guid, to_name;
    uint32_t map_game_id = 0; bool challenger_defends = false; uint32_t site = 0;
    int64_t expires = 0;
};
std::vector<Challenge> challenges;           // guarded by mu

int64_t Now() { return (int64_t)time(nullptr); }

size_t TeamSize(const std::string& guid) {
    TeamSnapshot t;
    return TeamService::Snapshot(guid, t) ? t.members.size() : 1;
}

// ---- settings-branch push: task team + task team leader for every member ----
// Client parser 0x1092a360: a DATA_SET row WITHOUT CHARACTER_ID sets the "current"
// TASK_TEAM + LEADER_CHARACTER_ID (+ optional POSITION hex); the rows WITH CHARACTER_ID that follow
// are members of that task team (member+0x26 = current TASK_TEAM, leader flag = current leader == id).
void PushTaskTeams(const TeamSnapshot& t) {
    std::map<int, std::vector<int64_t>> by_team;
    std::map<int, int64_t> leaders;
    uint32_t mode = 1;
    {
        std::lock_guard<std::mutex> l(mu);
        auto it = groups.find(t.id);
        if (it == groups.end()) return;
        mode = it->second.mode;
        for (const auto& m : t.members) {
            auto tt = it->second.tt.find(m.character_id);
            by_team[tt == it->second.tt.end() ? 1 : tt->second].push_back(m.character_id);
        }
        leaders = it->second.tt_leader;
    }
    if (mode < 3 || by_team.empty()) return;    // Raid teams exist only in a raid group (a strike force is one squad)
    size_t nrows = 0;
    for (auto& [team, ids] : by_team) nrows += 1 + ids.size();
    auto b = Head(GA_U::TEAM_UPDATE, 2);
    W4(b, GA_T::SETTING_FLAGS, 0);          // presence = settings branch; bit 0x10 (defender side) off
    P(b, GA_T::DATA_SET); P(b, (uint16_t)nrows);
    for (auto& [team, ids] : by_team) {
        P(b, 2);
        W4(b, GA_T::TASK_TEAM, (uint32_t)team);
        W4(b, GA_T::LEADER_CHARACTER_ID, (uint32_t)(leaders.count(team) ? leaders[team] : 0));
        for (int64_t cid : ids) { P(b, 1); W4(b, GA_T::CHARACTER_ID, (uint32_t)cid); }
    }
    for (const auto& m : t.members)
        if (!m.offline && !m.session_guid.empty()) TcpSession::DeliverRawPacket(m.session_guid, b);
}

// Reconcile task teams with the live roster (caller holds mu).
void ReconcileLocked(Group& g, const TeamSnapshot& t, const Rules& r) {
    std::map<int64_t, bool> present;
    for (const auto& m : t.members) present[m.character_id] = true;
    for (auto it = g.tt.begin(); it != g.tt.end();) it = present.count(it->first) ? std::next(it) : g.tt.erase(it);
    auto count = [&](int team) { int n = 0; for (auto& [c, tt] : g.tt) n += tt == team; return n; };
    for (const auto& m : t.members) {
        if (g.tt.count(m.character_id)) continue;
        int team = 1;
        if (g.mode >= 3) while (team < kMaxTaskTeams && count(team) >= r.task_team_max) ++team;
        g.tt[m.character_id] = team;
    }
    if (g.mode < 3) for (auto& [c, tt] : g.tt) tt = 1;       // strike force = one task team
    // Raid leader = team leader = leader of task team 1 (msg 51558). Keep them in team 1.
    if (t.leader_character_id) {
        const int lt = g.tt[t.leader_character_id];
        if (lt != 1) {
            // swap with the old team-1 leader (or any team-1 member) so sizes stay the same
            int64_t other = 0;
            if (auto ol = g.tt_leader.find(1); ol != g.tt_leader.end() && g.tt.count(ol->second) && g.tt[ol->second] == 1) other = ol->second;
            if (!other) for (auto& [c, tt] : g.tt) if (tt == 1 && c != t.leader_character_id) { other = c; break; }
            if (other) g.tt[other] = lt;
            g.tt[t.leader_character_id] = 1;
        }
        g.tt_leader[1] = t.leader_character_id;
    }
    // every non-empty task team has a leader who is in it
    for (int team = 1; team <= kMaxTaskTeams; ++team) {
        auto ld = g.tt_leader.find(team);
        const bool ok = ld != g.tt_leader.end() && g.tt.count(ld->second) && g.tt[ld->second] == team;
        if (ok) continue;
        g.tt_leader.erase(team);
        for (const auto& m : t.members)
            if (g.tt[m.character_id] == team) { g.tt_leader[team] = m.character_id; break; }
    }
}

int64_t AgencyMemberCount(const std::string& guid) {
    auto s = PlayerSessionStore::GetByGuid(guid);
    if (!s || !s->user_id) return 0;
    const int64_t agency = Database::GetAgencyIdForUser(s->user_id);
    return agency ? (int64_t)Database::GetAgencyMemberUserIds(agency).size() : 0;
}

bool InMission(const std::string& guid) {
    auto where = InstanceRegistry::GetInstancePlayerTaskForce(guid);
    if (!where) return false;
    auto inst = InstanceRegistry::GetInstanceById(where->first);
    return inst && !inst->is_home_map;
}

// ---- 0x48 Convert To Strikeforce ----
void CreateStrikeForce(const std::string& guid, const std::string& name) {
    const Rules r = GetRules();
    TeamSnapshot t;
    const bool teamed = TeamService::Snapshot(guid, t);
    if (teamed && t.leader_guid != guid) { Status(guid, GA_U::TEAM_CREATE_STRIKE_FORCE, MSG_SF_CANNOT_CREATE); return; }
    if (teamed && ModeFor(t.id) >= 2) { Status(guid, GA_U::TEAM_CREATE_STRIKE_FORCE, MSG_SF_ALREADY); return; }
    const int64_t agency_n = AgencyMemberCount(guid);
    if (agency_n < r.sf_min_agency) {
        Logger::Log("team", "[teams] %s strike force refused: agency members %lld < %d\n", name.c_str(), (long long)agency_n, r.sf_min_agency);
        Status(guid, GA_U::TEAM_CREATE_STRIKE_FORCE, MSG_SF_NEED_AGENCY8);
        return;
    }
    if (r.sf_blocks_queue) TeamService::DequeueForCompositionChange(guid);   // strike forces leave matchmade queues
    const uint64_t id = teamed ? t.id : TeamService::EnsureTeamFor(guid);
    if (!id) { Status(guid, GA_U::TEAM_CREATE_STRIKE_FORCE, MSG_SF_CANNOT_CREATE); return; }
    { std::lock_guard<std::mutex> l(mu); groups[id].mode = 2; }
    Logger::Log("team", "[teams] team #%llu converted to strike force by %s\n", (unsigned long long)id, name.c_str());
    TeamService::Snapshot(guid, t);
    for (const auto& m : t.members) Status(m.session_guid, GA_U::TEAM_CREATE_STRIKE_FORCE, r.sf_blocks_queue ? MSG_SF_NO_QUEUE : MSG_SF_CONVERTED);
    TeamService::Rebroadcast(id);
}

// ---- 0x49 Convert To Raid ----
void MakeRaidGroup(const std::string& guid, const std::string& name) {
    TeamSnapshot t;
    if (!TeamService::Snapshot(guid, t)) { Status(guid, GA_U::TEAM_MAKE_RAID_GROUP, MSG_NOT_ON_SF); return; }
    const uint32_t mode = ModeFor(t.id);
    if (mode >= 3) { Status(guid, GA_U::TEAM_MAKE_RAID_GROUP, MSG_RAID_ALREADY); return; }
    if (mode < 2)  { Status(guid, GA_U::TEAM_MAKE_RAID_GROUP, MSG_NOT_ON_SF); return; }
    if (t.leader_guid != guid) { Status(guid, GA_U::TEAM_MAKE_RAID_GROUP, MSG_NOT_RAID_LEADER); return; }
    { std::lock_guard<std::mutex> l(mu); auto& g = groups[t.id]; g.mode = 3; g.tt_leader.clear(); g.tt_leader[1] = t.leader_character_id; }
    Logger::Log("team", "[teams] team #%llu is now a raid group (%s)\n", (unsigned long long)t.id, name.c_str());
    for (const auto& m : t.members)
        ChatSession::SystemMessageToGuid(m.session_guid, "*** " + name + " made the strike force a raid group (T > Manage: move players into raid teams 1-6) ***");
    TeamService::Rebroadcast(t.id);
}

// Raid leader check for 0x13F / 0x140. Fills t. false = status already sent.
bool RaidLeaderGate(const std::string& guid, uint16_t type, TeamSnapshot& t) {
    if (!TeamService::Snapshot(guid, t) || ModeFor(t.id) < 3) { Status(guid, type, MSG_NOT_IN_RAID); return false; }
    if (t.leader_guid != guid) { Status(guid, type, MSG_NOT_RAID_LEADER); return false; }
    return true;
}

bool Has(const TeamSnapshot& t, int64_t cid) {
    return std::any_of(t.members.begin(), t.members.end(), [&](const TeamSnapshot::Member& m) { return m.character_id == cid; });
}

// ---- 0x13F RAID_MOVE_PLAYER {CHARACTER_ID, TASK_TEAM} | {CHARACTER_ID, TO_CHARACTER_ID} ----
void RaidMove(const std::string& guid, const PacketView& pkt) {
    TeamSnapshot t;
    if (!RaidLeaderGate(guid, GA_U::RAID_MOVE_PLAYER, t)) return;
    const int64_t cid = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
    const auto to_cid = pkt.Read4B(GA_T::TO_CHARACTER_ID);
    const auto to_tt = pkt.Read4B(GA_T::TASK_TEAM);
    Logger::Log("team", "[teams] RAID_MOVE team #%llu char=%lld task_team=%d to_char=%d\n", (unsigned long long)t.id,
        (long long)cid, to_tt ? (int)*to_tt : -1, to_cid ? (int)*to_cid : -1);
    if (!Has(t, cid)) return;
    const Rules r = GetRules();
    {
        std::lock_guard<std::mutex> l(mu);
        auto& g = groups[t.id];
        ReconcileLocked(g, t, r);
        if (to_cid && Has(t, (int64_t)*to_cid)) {
            std::swap(g.tt[cid], g.tt[(int64_t)*to_cid]);
        } else if (to_tt) {
            const int team = (int)*to_tt;
            if (team < 1 || team > kMaxTaskTeams) return;
            int n = 0; for (auto& [c, tt] : g.tt) n += (tt == team && c != cid);
            if (n >= r.task_team_max) { Status(guid, GA_U::RAID_MOVE_PLAYER, MSG_TEAM_FULL); return; }
            g.tt[cid] = team;
        } else return;
        if (cid == t.leader_character_id && g.tt[cid] != 1) {
            // the raid leader always heads team 1 (51558): moving them = undo
            g.tt[cid] = 1;
        }
        ReconcileLocked(g, t, r);
    }
    PushTaskTeams(t);
}

// ---- 0x140 RAID_PROMOTE_PLAYER {CHARACTER_ID}: leader of that player's raid team ----
void RaidPromote(const std::string& guid, const PacketView& pkt) {
    TeamSnapshot t;
    if (!RaidLeaderGate(guid, GA_U::RAID_PROMOTE_PLAYER, t)) return;
    const int64_t cid = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
    if (!Has(t, cid)) return;
    int team = 1;
    {
        std::lock_guard<std::mutex> l(mu);
        auto& g = groups[t.id];
        ReconcileLocked(g, t, GetRules());
        team = g.tt[cid];
        if (team != 1) g.tt_leader[team] = cid;
    }
    Logger::Log("team", "[teams] RAID_PROMOTE team #%llu char=%lld task_team=%d\n", (unsigned long long)t.id, (long long)cid, team);
    if (team == 1) {
        // leader of raid team 1 = raid leader = team leader
        std::string nm;
        for (const auto& m : t.members) if (m.character_id == cid) nm = m.player_name;
        TeamService::PromoteLeader(guid, cid);
        for (const auto& m : t.members) Status(m.session_guid, GA_U::RAID_PROMOTE_PLAYER, MSG_PROMOTED_RAID_LDR, nm);
        return;   // PromoteLeader rebroadcasts (-> AfterRoster)
    }
    PushTaskTeams(t);   // client posts 26936 "is now leader of raid team N"
}

// ---- 0x1D9 HEX_GET_LEADERBOARD {HEX_SEASON_NBR} -> DATA_SET {AGENCY_ID, RANK, AGENCY_NAME, NET_WORTH, TERRITORY_COUNT, ESTIMATED_OUTPUT}
void Leaderboard(const std::string& guid, const PacketView& pkt) {
    const uint32_t season = pkt.Read4B(GA_T::HEX_SEASON_NBR).value_or(0);
    struct Row { int64_t id = 0; std::string name; int64_t terr = 0; double worth = 0; int64_t out = 0; };
    std::map<int64_t, Row> rows;
    sqlite3* db = Database::GetConnection();
    auto q = [&](const char* sql, auto&& fn, int bind = -1) {
        sqlite3_stmt* s = nullptr;
        if (!db || sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return;
        if (bind >= 0) sqlite3_bind_int(s, 1, bind);
        while (sqlite3_step(s) == SQLITE_ROW) fn(s);
        sqlite3_finalize(s);
    };
    auto txt = [](sqlite3_stmt* s, int c) { auto p = (const char*)sqlite3_column_text(s, c); return std::string(p ? p : ""); };
    bool archived = false;
    const int view = (int)AvaZones::ViewOf(guid);
    if (season) q("SELECT owner_agency_id, owner_name, territories, networth, zone FROM ga_ava_seasons WHERE season = ?", [&](sqlite3_stmt* s) {
        archived = true; if (sqlite3_column_int(s, 4) != view) return; auto& r = rows[sqlite3_column_int64(s, 0)]; r.id = sqlite3_column_int64(s, 0); r.name = txt(s, 1); r.terr = sqlite3_column_int64(s, 2); r.worth = sqlite3_column_double(s, 3); }, (int)season);
    if (!archived) {
        // live: every agency without territory elsewhere, plus solo owners of territories (id 0x40000000 + character id)
        std::set<int64_t> elsewhere;
        q("SELECT id, name FROM ga_agencies", [&](sqlite3_stmt* s) { auto& r = rows[sqlite3_column_int64(s, 0)]; r.id = sqlite3_column_int64(s, 0); r.name = txt(s, 1); });
        q("SELECT owner_agency_id, MAX(owner_name), COUNT(*), MIN(position) / 1000 + 1 FROM ga_ava_territories GROUP BY owner_agency_id", [&](sqlite3_stmt* s) {
            if (sqlite3_column_int(s, 3) != view) { rows.erase(sqlite3_column_int64(s, 0)); elsewhere.insert(sqlite3_column_int64(s, 0)); return; }
            auto& r = rows[sqlite3_column_int64(s, 0)]; r.id = sqlite3_column_int64(s, 0); if (r.name.empty()) r.name = txt(s, 1); r.terr = sqlite3_column_int64(s, 2); });
        for (auto& [id, r] : rows) r.worth = AvaParity::Networth(id, AvaParity::CurrentSeason((int64_t)time(nullptr)));
        q("SELECT b.owner_agency_id, SUM(h.output_value) FROM ga_ava_buildings b JOIN asm_data_set_hex_buildings h "
          "ON h.hex_building_id = b.hex_building_id GROUP BY b.owner_agency_id", [&](sqlite3_stmt* s) {
            if (auto it = rows.find(sqlite3_column_int64(s, 0)); it != rows.end()) it->second.out = sqlite3_column_int64(s, 1); });
        (void)elsewhere;
    }
    std::vector<Row> v;
    for (auto& [id, r] : rows) if (!r.name.empty()) v.push_back(r);
    std::sort(v.begin(), v.end(), [](const Row& a, const Row& b) {
        if (a.worth != b.worth) return a.worth > b.worth;  // networth first (1.36)
        if (a.terr != b.terr) return a.terr > b.terr;
        if (a.out != b.out) return a.out > b.out;
        return a.name < b.name; });
    if (v.size() > 100) v.resize(100);
    auto b = Head(GA_U::HEX_GET_LEADERBOARD, 1);
    P(b, GA_T::DATA_SET); P(b, (uint16_t)v.size());
    uint32_t rank = 0;
    for (const auto& r : v) {
        P(b, 6);
        W4(b, GA_T::AGENCY_ID, (uint32_t)r.id);
        W4(b, GA_T::RANK, ++rank);
        // The client's leaderboard name cell holds 15 characters; a longer name (bot agency
        // 'Crimson Vanguard' = 16) showed as a blank row. Shorten instead of losing it.
        WS(b, GA_T::AGENCY_NAME, r.name.size() > 15 ? r.name.substr(0, 14) + "." : r.name);
        W4(b, GA_T::NET_WORTH, (uint32_t)(int32_t)std::llround(r.worth));   // Client type 2 = 4-byte int (a double shifted the row)
        W4(b, GA_T::TERRITORY_COUNT, (uint32_t)r.terr);
        W4(b, GA_T::ESTIMATED_OUTPUT, (uint32_t)r.out);
    }
    Logger::Log("team", "[teams] HEX_GET_LEADERBOARD season=%u rows=%zu%s\n", season, v.size(), archived ? " (archived season)" : "");
    TcpSession::DeliverRawPacket(guid, b);
}

// ---- 0x1F3 CHALLENGE_REQUEST ----
void ChallengeStatus(const std::string& guid, uint32_t msg, const std::string& name = "") {
    auto b = Head(GA_U::CHALLENGE_REQUEST, name.empty() ? 2 : 3);
    W1(b, GA_T::SUCCESS, 1);                 // SUCCESS present -> status line, not the popup
    W4(b, GA_T::MSG_ID, msg);
    if (!name.empty()) WS(b, GA_T::PLAYER_NAME, name);
    TcpSession::DeliverRawPacket(guid, b);
}

void Challenge_(const std::string& guid, const std::string& name, const PacketView& pkt) {
    if (auto acc = pkt.Read1B(GA_T::ACCEPT_FLAG)) {
        Challenge c; bool found = false;
        {
            std::lock_guard<std::mutex> l(mu);
            auto it = std::find_if(challenges.begin(), challenges.end(), [&](const Challenge& x) { return x.to_guid == guid; });
            if (it != challenges.end()) { c = *it; challenges.erase(it); found = true; }
        }
        TcpSession::DeliverRawPacket(guid, Head(GA_U::CHALLENGE_EXPIRED_INVITE, 0));   // dismiss the popup node
        if (!found || c.expires < Now()) { ChallengeStatus(guid, MSG_CH_EXPIRED); return; }
        Logger::Log("team", "[teams] challenge %s -> %s map=%u %s\n", c.from_name.c_str(), c.to_name.c_str(), c.map_game_id, *acc ? "ACCEPTED" : "declined");
        if (!*acc) { ChallengeStatus(c.from_guid, MSG_CH_DECLINED); return; }
        ChallengeStatus(guid, MSG_CH_ACCEPTED);
        ChallengeStatus(c.from_guid, MSG_CH_ACCEPTED);
        PvpParity::LaunchChallenge(c.from_guid, c.from_name, guid, c.to_name, c.map_game_id, c.challenger_defends);
        return;
    }
    const std::string target = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
    const uint32_t map_game_id = pkt.Read4B(GA_T::MAP_GAME_ID).value_or(0);
    const bool defend = pkt.Read1B(GA_T::DEFENSE_FLAG).value_or(0) != 0;
    const uint32_t site = pkt.Read4B(GA_T::SYS_SITE_ID).value_or(0);
    Logger::Log("team", "[teams] CHALLENGE_REQUEST from %s target='%s' map=%u defend=%d site=%u\n", name.c_str(), target.c_str(), map_game_id, defend, site);
    if (target.empty()) return;
    auto ieq = [](const std::string& a, const std::string& b) { return a.size() == b.size() &&
        std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return std::tolower((unsigned char)x) == std::tolower((unsigned char)y); }); };
    if (ieq(target, name)) { ChallengeStatus(guid, MSG_CH_SELF); return; }
    auto to = PlayerSessionStore::GetByPlayerName(target);
    if (!to) { ChallengeStatus(guid, MSG_NOT_ONLINE, target); return; }
    if (InMission(guid)) { ChallengeStatus(guid, MSG_CH_IN_MISSION); return; }
    TeamSnapshot mine, theirs;
    if (TeamService::Snapshot(guid, mine) && mine.leader_guid != guid) { ChallengeStatus(guid, MSG_NOT_LEADER); return; }
    if (TeamService::Snapshot(to->session_guid, theirs) && theirs.leader_guid != to->session_guid) {
        ChatSession::SystemMessageToGuid(guid, "*** " + to->player_name + " is not a team leader - challenge their team leader ***");
        return;
    }
    Challenge c{guid, name, to->session_guid, to->player_name, map_game_id, defend, site, Now() + kChallengeSeconds};
    {
        std::lock_guard<std::mutex> l(mu);
        challenges.erase(std::remove_if(challenges.begin(), challenges.end(), [](const Challenge& x) { return x.expires < Now(); }), challenges.end());
        for (const auto& x : challenges) {
            if (x.from_guid == guid) { c.expires = 0; break; }
            if (x.to_guid == to->session_guid || x.from_guid == to->session_guid) { c.expires = -1; break; }
        }
        if (c.expires > 0) challenges.push_back(c);
    }
    if (c.expires == 0)  { ChallengeStatus(guid, MSG_CH_PENDING); return; }
    if (c.expires == -1) { ChallengeStatus(guid, MSG_CH_TARGET_BUSY); return; }
    const uint32_t n_from = (uint32_t)TeamSize(guid), n_to = (uint32_t)TeamSize(to->session_guid);
    auto b = Head(GA_U::CHALLENGE_REQUEST, 6);      // no SUCCESS -> TgUIChallengePopup
    W4(b, GA_T::REMAINING_SECONDS, kChallengeSeconds);
    W4(b, GA_T::MAP_GAME_ID, map_game_id);
    WS(b, GA_T::PLAYER_NAME, defend ? to->player_name : name);    // "%s <=> %s" attacker first
    WS(b, GA_T::LEADER_NAME, defend ? name : to->player_name);
    W4(b, GA_T::ATTACK_COUNT, defend ? n_to : n_from);
    W4(b, GA_T::DEFEND_COUNT, defend ? n_from : n_to);
    TcpSession::DeliverRawPacket(to->session_guid, b);
    ChallengeStatus(guid, MSG_CH_YOU_CHALLENGE, to->player_name);
}

} // namespace

// ================= public =================
// The client's Convert To Raid buttons are hidden in every GA_Menu_Team scene (dead UI),
// so the strike force leader types -raid; same checks + result as the 0x49 packet.
void RaidCommand(const std::string& session_guid) {
    auto s = PlayerSessionStore::GetByGuid(session_guid);
    if (s) MakeRaidGroup(session_guid, s->player_name);
}

uint32_t ModeFor(uint64_t team_id) {
    std::lock_guard<std::mutex> l(mu);
    auto it = groups.find(team_id);
    return it == groups.end() ? 1 : it->second.mode;
}

size_t MaxMembers(uint64_t team_id) {
    const Rules r = GetRules();
    return ModeFor(team_id) >= 3 ? (size_t)(kMaxTaskTeams * r.task_team_max) : (size_t)r.squad_max;
}

size_t DisbandBelow(uint64_t team_id) { return ModeFor(team_id) >= 2 ? 0 : 1; }

void Forget(uint64_t team_id) {
    std::lock_guard<std::mutex> l(mu);
    groups.erase(team_id);
}

void AfterRoster(uint64_t team_id) {
    TeamSnapshot t;
    if (!TeamService::SnapshotById(team_id, t)) { Forget(team_id); return; }
    {
        std::lock_guard<std::mutex> l(mu);
        auto it = groups.find(team_id);
        if (it == groups.end() || it->second.mode < 2) return;
        ReconcileLocked(it->second, t, GetRules());
    }
    PushTaskTeams(t);
}

bool QueueBlocked(const std::string& session_guid, uint32_t map_pool_id) {
    if (map_pool_id >= 20 && map_pool_id <= 25) return false;      // AvA facility queues: that is what strike forces are for
    TeamSnapshot t;
    if (!TeamService::Snapshot(session_guid, t) || ModeFor(t.id) < 2) return false;
    if (!GetRules().sf_blocks_queue) return false;
    Status(session_guid, GA_U::TEAM_CREATE_STRIKE_FORCE, MSG_SF_NO_QUEUE);
    return true;
}

bool HandlePacket(const std::string& guid, const std::string& name, uint16_t type, const PacketView& pkt) {
    if (guid.empty()) return false;
    switch (type) {
        case GA_U::TEAM_CREATE_STRIKE_FORCE: CreateStrikeForce(guid, name); return true;
        case GA_U::TEAM_MAKE_RAID_GROUP:     MakeRaidGroup(guid, name); return true;
        case GA_U::RAID_MOVE_PLAYER:         RaidMove(guid, pkt); return true;
        case GA_U::RAID_PROMOTE_PLAYER:      RaidPromote(guid, pkt); return true;
        case GA_U::HEX_GET_LEADERBOARD:      Leaderboard(guid, pkt); return true;
        case GA_U::CHALLENGE_REQUEST:        Challenge_(guid, name, pkt); return true;
        case GA_U::TEAM_MERGE:
        case GA_U::TEAM_MERGE_ACCEPT:
            // no sender in this client build; strike forces grow by normal team invites
            Logger::Log("team", "[teams] TEAM_MERGE 0x%04X from %s ignored (unused by client)\n", type, name.c_str());
            return true;
        default: return false;
    }
}

std::optional<std::unordered_set<std::string>> ChatRecipients(uint32_t channel, const std::string& sender_guid) {
    if (channel != 5 && channel != 6) return std::nullopt;
    TeamSnapshot t;
    const bool teamed = TeamService::Snapshot(sender_guid, t);
    const uint32_t mode = teamed ? ModeFor(t.id) : 0;
    if (channel == 6) {
        std::unordered_set<std::string> out;
        if (mode >= 3) for (const auto& m : t.members) if (!m.session_guid.empty()) out.insert(m.session_guid);
        return out;                                                 // empty = "You must be in a raid"
    }
    if (mode < 3) return std::nullopt;                              // plain team / strike force: existing team scope
    std::unordered_set<std::string> out;
    std::lock_guard<std::mutex> l(mu);
    auto it = groups.find(t.id);
    if (it == groups.end()) return std::nullopt;
    int64_t me = 0;
    for (const auto& m : t.members) if (m.session_guid == sender_guid) me = m.character_id;
    const int my_tt = it->second.tt.count(me) ? it->second.tt.at(me) : 1;
    for (const auto& m : t.members) {
        auto tt = it->second.tt.find(m.character_id);
        if ((tt == it->second.tt.end() ? 1 : tt->second) == my_tt && !m.session_guid.empty()) out.insert(m.session_guid);
    }
    return out;
}

bool HandleRaidAlert(const std::string& sender_guid, const std::string& sender_name, const uint8_t* frame, size_t length) {
    if (length < 6) return false;
    PacketView pkt(frame + 6, length - 6);
    if (pkt.Read4B(GA_T::CHAT_CH_TYPE).value_or(0) != 6) return false;
    if (!pkt.Exists(GA_T::DISPLAY_AS_ALERT) && !pkt.Exists(GA_T::TEXT_VALUE)) return false;   // plain /rg line -> normal path
    auto to = ChatRecipients(6, sender_guid);
    if (!to || to->empty()) { ChatSession::SystemMessageToGuid(sender_guid, "*** You must be in a raid to do this. ***"); return true; }
    const std::string text = pkt.ReadString(GA_T::TEXT_VALUE).value_or("");
    // Relay the client's own alert frame unchanged (same as /alert in ChatChannels): the
    // receivers show the centre-screen raid warning (MSG_ID 28485 "@@text_value@@").
    const std::vector<uint8_t> raw(frame, frame + length);
    size_t n = 0;
    for (const auto& g : *to) if (auto s = PlayerSessionStore::GetByGuid(g)) n += ChatSession::DeliverToPlayer(s->player_name, raw);
    Logger::Log("team", "[teams] raid warning from %s to %zu member(s): %s\n", sender_name.c_str(), n, text.c_str());
    return true;
}

} // namespace Teams
} // namespace Social
