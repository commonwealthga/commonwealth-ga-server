#include "src/ControlServer/ChatSession/PlayerCommands.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <cstdio>
#include <ctime>
#include <unordered_map>
#include "src/ControlServer/Social/Friends.hpp"
#include "src/ControlServer/Social/Teams.hpp"
#include "src/ControlServer/AvaCargo/AvaCargo.hpp"

namespace PlayerCommands {

namespace {

constexpr int64_t kStuckCooldownSeconds = 300;
constexpr size_t  kWhoMaxNames = 30;

std::unordered_map<int64_t, int64_t> g_last_stuck;  // user_id -> epoch

void Say(const std::string& guid, const std::string& text) {
    ChatSession::SystemMessageToGuid(guid, text);
}

std::string Duration(int64_t secs) {
    if (secs < 0) secs = 0;
    char buf[48];
    const int64_t h = secs / 3600, m = (secs % 3600) / 60;
    if (h > 0) std::snprintf(buf, sizeof(buf), "%lldh %02lldm", (long long)h, (long long)m);
    else       std::snprintf(buf, sizeof(buf), "%lldm", (long long)m);
    return buf;
}

void Help(const std::string& guid) {
    Say(guid, "Commands on schlumpy.net (type them in chat):");
    Say(guid, "  -who  online players | -played  your play time | -loc  where you are");
    Say(guid, "  -stuck  move to a spawn point (5 min cooldown, not in PvP)");
    Say(guid, "  -bug <text>  send a bug report | -report <player> <text>  report a player");
    Say(guid, "  -classes  team class counts | -changeteam  switch team (PvP)");
    Say(guid, "  -afk  away from keyboard | -invites team off  only friends can invite you (also agency/all, on)");
    Say(guid, "  -raid  strike force leader: make your strike force a raid group (T > Manage)");
    Say(guid, "  -cargo  AvA dropship cargo (-cargo load / -cargo unload)");
    Say(guid, "  Game slash commands like /w /invite /friend /agencyinvite work as usual.");
}

void Who(const std::string& guid) {
    std::vector<std::string> shown;
    size_t total = 0;
    for (const auto& n : ChatSession::OnlinePlayerNames()) {
        if (n.size() >= 2 && (n[0] == 'z' || n[0] == 'Z') && (n[1] == 'z' || n[1] == 'Z')) continue;  // test accounts
        total++;
        if (shown.size() < kWhoMaxNames) shown.push_back(n + (Social::Friends::IsAfk(n) ? " (AFK)" : ""));
    }
    Say(guid, "Players online: " + std::to_string(total));
    std::string line;
    for (const auto& n : shown) {
        if (!line.empty() && line.size() + n.size() > 90) { Say(guid, "  " + line); line.clear(); }
        line += line.empty() ? n : ", " + n;
    }
    if (total > shown.size()) line += " and " + std::to_string(total - shown.size()) + " more";
    if (!line.empty()) Say(guid, "  " + line);
}

void Played(const std::string& guid) {
    auto info = PlayerSessionStore::GetByGuid(guid);
    if (!info || !info->selected_character_id) { Say(guid, "*** No agent selected ***"); return; }
    double secs = 0; int64_t matches = 0;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT COALESCE(SUM(time_played_seconds),0), COUNT(DISTINCT instance_id)"
            " FROM ga_match_player_stats WHERE character_id = ?", -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(st, 1, info->selected_character_id);
        if (sqlite3_step(st) == SQLITE_ROW) {
            secs = sqlite3_column_double(st, 0);
            matches = sqlite3_column_int64(st, 1);
        }
    }
    sqlite3_finalize(st);
    Say(guid, "Time in missions/matches on this agent: " + Duration((int64_t)secs)
        + " (" + std::to_string(matches) + " played)");
    const int64_t session = ChatSession::SessionSecondsForGuid(guid);
    if (session >= 0) Say(guid, "This session: " + Duration(session));
}

void Loc(const std::string& guid) {
    auto at = InstanceRegistry::GetInstancePlayerTaskForce(guid);
    std::string map = "?";
    if (at) {
        if (auto inst = InstanceRegistry::GetInstanceById(at->first)) map = inst->map_name;
        Say(guid, "Location: " + map + " (instance " + std::to_string(at->first) + ") - coordinates shown on screen");
        ChatCommand::DispatchCoords(guid);
    } else {
        Say(guid, "*** You are not in a map yet ***");
    }
}

void Stuck(const std::string& guid) {
    auto info = PlayerSessionStore::GetByGuid(guid);
    if (!info || !info->user_id) return;
    if (!InstanceRegistry::GetInstancePlayerTaskForce(guid)) {
        Say(guid, "*** You are not in a map yet ***");
        return;
    }
    const int64_t now = static_cast<int64_t>(std::time(nullptr));
    auto it = g_last_stuck.find(info->user_id);
    if (it != g_last_stuck.end() && now - it->second < kStuckCooldownSeconds) {
        Say(guid, "*** -stuck can be used again in "
            + std::to_string(kStuckCooldownSeconds - (now - it->second)) + " s ***");
        return;
    }
    g_last_stuck[info->user_id] = now;
    Say(guid, "Moving you to a spawn point...");
    ChatCommand::DispatchStuck(guid);
}

} // namespace

bool Run(const std::string& guid, const std::string& name, const std::string& rest) {
    (void)rest;
    if (guid.empty() || !Known(name)) return false;
    Logger::Log("chat-command", "[ChatCmd] player command guid=%s name=%s\n", guid.c_str(), name.c_str());
    if (name == "help" || name == "commands") Help(guid);
    else if (name == "who")                     Who(guid);
    else if (name == "played")                  Played(guid);
    else if (name == "loc")                     Loc(guid);
    else if (name == "stuck" || name == "unstuck") Stuck(guid);
    else if (name == "afk")                     Social::Friends::ToggleAfk(guid);
    else if (name == "raid")                    Social::Teams::RaidCommand(guid);
    else if (name == "cargo") {   // AvA dropship cargo
        const auto info = PlayerSessionStore::GetByGuid(guid);
        if (info && info->selected_character_id) Say(guid, AvaCargo::Command(info->selected_character_id, rest));
    }
    else if (name == "invites")                 Social::Friends::InvitesCommand(guid, rest);
    else if (name == "gag" || name == "ungag")  Social::Friends::GagCommand(guid, rest, name == "ungag");
    else if (name == "support" || name == "petition")
        Say(guid, "*** Hi-Rez support is gone - use -bug <text> or -report <player> <text> ***");
    else
        Say(guid, "*** That command is not available on this server (type -help) ***");
    return true;
}

} // namespace PlayerCommands
