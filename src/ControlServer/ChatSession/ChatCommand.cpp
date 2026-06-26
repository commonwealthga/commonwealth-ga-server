#include "src/ControlServer/ChatSession/ChatCommand.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <vector>

#include "lib/nlohmann/json.hpp"

#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "src/Shared/IpcProtocol.hpp"

namespace ChatCommand {

namespace {

std::string TrimAscii(const std::string& s) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto begin = std::find_if_not(s.begin(), s.end(), is_space);
    auto end   = std::find_if_not(s.rbegin(), s.rend(), is_space).base();
    if (begin >= end) return {};
    return std::string(begin, end);
}

std::string LowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

const char* TargetName(ChangeTeamTarget t) {
    switch (t) {
        case ChangeTeamTarget::Toggle:    return "toggle";
        case ChangeTeamTarget::Attackers: return "attackers";
        case ChangeTeamTarget::Defenders: return "defenders";
    }
    return "?";
}

const char* SpawnTargetTeamName(SpawnTargetTeam t) {
    switch (t) {
        case SpawnTargetTeam::Friend: return "friend";
        case SpawnTargetTeam::Enemy:  return "enemy";
    }
    return "?";
}

// Difficulty token → scalar. Mirrors Config::GetDifficultyScalar() values:
//   low     = 1.00  (DIFFICULTY_VALUE_ID_LOW_SECURITY / NOVICE)
//   medium  = 1.25  (MEDIUM_SECURITY / ADEPT)
//   high    = 1.50  (HIGH_SECURITY / DOUBLE_AGENT / ADVANCED)
//   max     = 1.75  (MAXIMUM_SECURITY / EXPERT)
//   umax    = 2.00  (ULTRA_MAX_SECURITY)
// Returns 0.0 for unknown tokens (caller treats this as "not a difficulty
// token at all" — could be the bot_id then).
float DifficultyScalarFromToken(const std::string& tok_lower) {
    if (tok_lower == "low")    return 1.00f;
    if (tok_lower == "medium") return 1.25f;
    if (tok_lower == "high")   return 1.50f;
    if (tok_lower == "max")    return 1.75f;
    if (tok_lower == "umax")   return 2.00f;
    return 0.0f;
}

// Split a trimmed string on ASCII whitespace runs.
std::vector<std::string> SplitWs(const std::string& s) {
    std::vector<std::string> out;
    auto it = s.begin();
    while (it != s.end()) {
        while (it != s.end() && std::isspace(static_cast<unsigned char>(*it))) ++it;
        auto start = it;
        while (it != s.end() && !std::isspace(static_cast<unsigned char>(*it))) ++it;
        if (start != it) out.emplace_back(start, it);
    }
    return out;
}

// Parse a positive integer; returns nullopt on any non-digit / overflow / empty.
std::optional<int> ParseInt(const std::string& s) {
    if (s.empty()) return std::nullopt;
    int v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        if (v > (INT_MAX - (c - '0')) / 10) return std::nullopt;
        v = v * 10 + (c - '0');
    }
    return v;
}

// Resolve a <map> token that is EITHER a 1-based challenge-list number OR a map
// name (case-insensitive). nullopt if neither matches.
std::optional<MapGameRecord> ResolveMapToken(const std::string& token) {
    if (auto n = ParseInt(token)) {              // all-digit -> list index
        auto maps = MapGameInfo::GetChallengeMaps();
        if (*n >= 1 && static_cast<size_t>(*n) <= maps.size())
            return MapGameInfo::LookupByName(maps[*n - 1].map_name);
        return std::nullopt;
    }
    return MapGameInfo::LookupByName(token);     // otherwise treat as a name
}

} // namespace

ParseResult TryParseChatCommand(const std::string& message_text) {
    ParseResult out;

    std::string trimmed = TrimAscii(message_text);
    // Commands use a leading '-' (the GA chat box appears to swallow '/'-prefixed
    // text client-side, which is why the existing commands all use '-'). Accept a
    // leading '/' too so /challenge works if the client does forward it; both
    // sigils normalise to the same command name below. Unrecognised '-'/'/' text
    // still falls through to broadcast as ordinary chat.
    if (trimmed.empty() || (trimmed[0] != '-' && trimmed[0] != '/')) {
        return out;
    }

    // Split on first whitespace.
    auto ws = std::find_if(trimmed.begin(), trimmed.end(),
        [](unsigned char c) { return std::isspace(c) != 0; });
    std::string cmd_name = LowerAscii(std::string(trimmed.begin(), ws));
    std::string rest;
    if (ws != trimmed.end()) {
        rest = TrimAscii(std::string(ws, trimmed.end()));
    }

    if (cmd_name == "-changeteam") {
        // We own this command from here on — suppress broadcast either way.
        out.recognized = true;
        out.suppress_broadcast = true;

        std::string rest_lower = LowerAscii(rest);
        if (rest_lower.empty()) {
            out.change_team = ChangeTeamTarget::Toggle;
        } else if (rest_lower == "attackers") {
            out.change_team = ChangeTeamTarget::Attackers;
        } else if (rest_lower == "defenders") {
            out.change_team = ChangeTeamTarget::Defenders;
        }
        // else: change_team stays nullopt -> silent reject. suppress_broadcast still true.
        return out;
    }

    if (cmd_name == "-coords") {
        // No args — report the player's current XYZ.
        out.recognized = true;
        out.suppress_broadcast = true;
        out.coords = true;
        return out;
    }

    if (cmd_name == "-challenge" || cmd_name == "/challenge") {
        // -challenge                        -> show the numbered map list
        // -challenge maps | list            -> show the numbered map list
        // -challenge <person> <map>         -> side defaults to attackers
        // -challenge <person> <map> <side>  -> explicit side (attackers/defenders)
        // <map> is a list NUMBER or a map name. Extra trailing tokens (e.g. a
        // stray region) are ignored — there is no server-location argument.
        out.recognized = true;
        out.suppress_broadcast = true;

        std::vector<std::string> tokens = SplitWs(rest);
        if (tokens.size() < 2 ||
            LowerAscii(tokens[0]) == "maps" || LowerAscii(tokens[0]) == "list") {
            out.challenge_list = true;  // bare / keyword / missing map -> show list
            return out;
        }

        ChallengeArgs args;
        args.target_name   = tokens[0];
        args.map_token     = tokens[1];
        args.challenger_tf = 1;  // default = attackers
        if (tokens.size() >= 3) {
            const std::string side = LowerAscii(tokens[2]);
            if (side == "defenders" || side == "defender" || side == "d")
                args.challenger_tf = 2;
            // attackers/attacker/a (or any other ignored token) -> default tf 1
        }
        out.challenge = args;
        return out;
    }

    if (cmd_name == "-spawnfriend" || cmd_name == "-spawnenemy" ||
        cmd_name == "-spawnhenchman") {
        // -spawnfriend    [low|medium|high|max|umax] <bot_id>
        // -spawnenemy     [low|medium|high|max|umax] <bot_id>
        // -spawnhenchman  [low|medium|high|max|umax] <bot_id>
        //   (= -spawnfriend + henchman flag, player becomes its leader)
        // Difficulty token is optional; bare form falls back to the map's
        // current difficulty in the DLL (scalar=0 sentinel).
        out.recognized = true;
        out.suppress_broadcast = true;

        std::vector<std::string> tokens = SplitWs(rest);
        if (tokens.empty() || tokens.size() > 2) return out;

        SpawnTargetArgs args;
        args.team = (cmd_name == "-spawnenemy")
                        ? SpawnTargetTeam::Enemy
                        : SpawnTargetTeam::Friend;
        args.henchman = (cmd_name == "-spawnhenchman");

        std::optional<int> bot_id;
        if (tokens.size() == 1) {
            // Just <bot_id> — leave difficulty at 0 (use map default).
            bot_id = ParseInt(tokens[0]);
        } else {
            // <difficulty> <bot_id>
            const float scalar = DifficultyScalarFromToken(LowerAscii(tokens[0]));
            if (scalar == 0.0f) return out;  // unknown difficulty token
            args.difficulty_scalar = scalar;
            bot_id = ParseInt(tokens[1]);
        }
        if (!bot_id || *bot_id <= 0) return out;
        args.bot_id = *bot_id;

        out.spawn_target = args;
        return out;
    }

    if (cmd_name == "-deployfriend" || cmd_name == "-deployenemy") {
        // -deployfriend <deployable_id>
        // -deployenemy  <deployable_id>
        out.recognized = true;
        out.suppress_broadcast = true;

        std::vector<std::string> tokens = SplitWs(rest);
        if (tokens.size() != 1) return out;

        std::optional<int> dep_id = ParseInt(tokens[0]);
        if (!dep_id || *dep_id <= 0) return out;

        DeployTargetArgs args;
        args.deployable_id = *dep_id;
        args.team = (cmd_name == "-deployfriend")
                        ? SpawnTargetTeam::Friend
                        : SpawnTargetTeam::Enemy;
        out.deploy_target = args;
        return out;
    }

    if (cmd_name == "-possess") {
        out.recognized = true;
        out.suppress_broadcast = true;
        if (rest.empty()) out.possess = true;
        return out;
    }

    if (cmd_name == "-unpossess") {
        out.recognized = true;
        out.suppress_broadcast = true;
        if (rest.empty()) out.unpossess = true;
        return out;
    }

    if (cmd_name == "-reload-queues") {
        out.recognized = true;
        out.suppress_broadcast = true;
        // No args; trailing junk silently ignored (recognized + suppressed).
        if (rest.empty()) out.reload_queues = true;
        return out;
    }

    if (cmd_name == "-topdown") {
        // -topdown            -> toggle, default lift
        // -topdown <lift_z>   -> toggle, explicit lift in world units (cm)
        out.recognized = true;
        out.suppress_broadcast = true;
        TopDownArgs args;
        if (!rest.empty()) {
            try {
                args.lift_z = std::stof(rest);
            } catch (...) {
                // Bad arg — silent reject (suppress_broadcast stays true).
                return out;
            }
        }
        out.topdown = args;
        return out;
    }

    // Other "-" text — pass through to broadcast as ordinary chat.
    return out;
}

void DispatchTeamMove(int64_t instance_id, const std::string& session_guid, int new_tf,
                      bool is_autobalance) {
    // asio io_context is single-threaded, so the DB write + dispatch can't
    // interleave with a matchmaker decision.
    InstanceRegistry::UpdateInstancePlayerTaskForce(instance_id, session_guid, new_tf);

    // Always dispatch the EXPLICIT side — never "toggle" — so the DLL doesn't
    // re-resolve from a pawn whose team might change between dispatch and receipt.
    const char* explicit_target = (new_tf == 1) ? "attackers" : "defenders";

    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "change_team";
    payload["args"]         = { {"target", explicit_target}, {"autobalance", is_autobalance} };

    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] DispatchTeamMove dispatch_failed guid=%s new_tf=%d autobalance=%d instance=%lld\n",
            session_guid.c_str(), new_tf, (int)is_autobalance, (long long)instance_id);
    }
}

void DispatchChangeTeam(ChangeTeamTarget target, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchChangeTeam dropped: empty session_guid\n");
        return;
    }

    // Resolve current (instance, tf) from the control-server DB. If the
    // player isn't in any active instance, the DLL would drop the action
    // anyway (no pawn) — skip with a log.
    auto lookup = InstanceRegistry::GetInstancePlayerTaskForce(session_guid);
    if (!lookup) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-changeteam outcome=ignored details=no_active_instance_player\n",
            session_guid.c_str());
        return;
    }
    const int64_t instance_id = lookup->first;
    const int     old_tf      = lookup->second;

    int new_tf = old_tf;
    switch (target) {
        case ChangeTeamTarget::Toggle:    new_tf = (old_tf == 1) ? 2 : 1; break;
        case ChangeTeamTarget::Attackers: new_tf = 1; break;
        case ChangeTeamTarget::Defenders: new_tf = 2; break;
    }
    if (new_tf == old_tf) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-changeteam target=%s outcome=no-op details=already_on_tf_%d\n",
            session_guid.c_str(), TargetName(target), old_tf);
        return;
    }

    // Shared apply path: DB update + explicit change_team PLAYER_ACTION.
    DispatchTeamMove(instance_id, session_guid, new_tf);
}

void DispatchDeployTarget(const DeployTargetArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchDeployTarget dropped: empty session_guid\n");
        return;
    }

    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "deploy_target";
    payload["args"]         = {
        {"deployable_id", args.deployable_id},
        {"team",          SpawnTargetTeamName(args.team)},
    };

    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-deploy%s deployable_id=%d outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(), SpawnTargetTeamName(args.team), args.deployable_id);
    }
}

void DispatchSpawnTarget(const SpawnTargetArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchSpawnTarget dropped: empty session_guid\n");
        return;
    }

    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "spawn_target";
    payload["args"]         = {
        {"bot_id",            args.bot_id},
        {"team",              SpawnTargetTeamName(args.team)},
        {"difficulty_scalar", args.difficulty_scalar},
        {"henchman",          args.henchman},
    };

    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-spawn%s bot_id=%d scalar=%.2f outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(), SpawnTargetTeamName(args.team),
            args.bot_id, args.difficulty_scalar);
    }
}

static void DispatchSimpleAction(const std::string& action_name, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] Dispatch%s dropped: empty session_guid\n", action_name.c_str());
        return;
    }
    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = action_name;
    payload["args"]         = nlohmann::json::object();
    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-%s outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(), action_name.c_str());
    }
}

void DispatchPossess(const std::string& session_guid)   { DispatchSimpleAction("possess",   session_guid); }
void DispatchUnpossess(const std::string& session_guid) { DispatchSimpleAction("unpossess", session_guid); }
void DispatchCoords(const std::string& session_guid)    { DispatchSimpleAction("coords",    session_guid); }

void DispatchTopDown(const TopDownArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchTopDown dropped: empty session_guid\n");
        return;
    }
    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "topdown";
    payload["args"]         = { {"lift_z", args.lift_z} };
    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-topdown lift_z=%.0f outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(), args.lift_z);
    }
}

namespace { ChallengeLauncher g_challenge_launcher; }

void SetChallengeLauncher(ChallengeLauncher launcher) {
    g_challenge_launcher = std::move(launcher);
}

void SendChallengeMapList(const ChallengeReply& reply) {
    auto maps = MapGameInfo::GetChallengeMaps();
    reply("Usage: -challenge <player> <number> [defenders]");
    if (maps.empty()) {
        reply("(no maps available)");
        return;
    }
    // Pack 4 maps per message.
    // Regular maps: strip mode prefix ("CONTROL: Seaside" -> "Seaside").
    // AvA maps: strip "HEX_AVA_" prefix, underscores to spaces, space before digits.
    auto shortName = [](const std::string& display_name) -> std::string {
        auto pos = display_name.find(": ");
        if (pos != std::string::npos) return display_name.substr(pos + 2);
        if (display_name.size() > 8 && display_name.substr(0, 8) == "HEX_AVA_") {
            std::string s = display_name.substr(8);
            if (s.size() > 2 && s.substr(s.size() - 2) == "_P")
                s = s.substr(0, s.size() - 2);
            std::string out;
            for (size_t i = 0; i < s.size(); i++) {
                if (s[i] == '_') { out += ' '; continue; }
                if (std::isdigit(static_cast<unsigned char>(s[i])) && i > 0
                        && s[i-1] != '_' && s[i-1] != ' ')
                    out += ' ';
                out += s[i];
            }
            return out;
        }
        return display_name;
    };
    std::string line;
    int col = 0;
    for (const auto& m : maps) {
        if (!line.empty()) line += "  ";
        line += std::to_string(m.number) + "." + shortName(m.display_name);
        if (++col == 4) {
            reply(line);
            line.clear();
            col = 0;
        }
    }
    if (!line.empty()) reply(line);
}

void DispatchChallenge(const ChallengeArgs& args, const std::string& challenger_guid,
                       const ChallengeReply& reply) {
    Logger::Log("challenge",
        "[Challenge] DispatchChallenge target='%s' map_token='%s' challenger_tf=%d challenger_guid=%s\n",
        args.target_name.c_str(), args.map_token.c_str(), args.challenger_tf,
        challenger_guid.c_str());

    if (challenger_guid.empty()) {
        Logger::Log("challenge", "[Challenge] dropped: empty challenger session_guid\n");
        return;
    }

    // Resolve <map> first (number or name) so a typo can hint the list.
    auto rec = ResolveMapToken(args.map_token);
    if (!rec) {
        reply("Unknown map '" + args.map_token + "'. Type -challenge to list maps.");
        Logger::Log("challenge",
            "[Challenge] guid=%s map_token='%s' outcome=ignored details=unknown_map\n",
            challenger_guid.c_str(), args.map_token.c_str());
        return;
    }

    // Resolve <person> to an online session (case-insensitive name match over
    // players currently inside any running instance — incl. the home map).
    const std::string want = LowerAscii(args.target_name);
    std::string target_guid;
    for (const auto& row : InstanceRegistry::GetActiveSearchablePlayers()) {
        if (LowerAscii(row.player_name) == want) { target_guid = row.session_guid; break; }
    }
    if (target_guid.empty()) {
        reply("Player '" + args.target_name + "' is not online.");
        Logger::Log("challenge",
            "[Challenge] guid=%s target='%s' outcome=ignored details=target_not_online\n",
            challenger_guid.c_str(), args.target_name.c_str());
        return;
    }
    if (target_guid == challenger_guid) {
        reply("You cannot challenge yourself.");
        Logger::Log("challenge",
            "[Challenge] guid=%s outcome=ignored details=self_challenge\n", challenger_guid.c_str());
        return;
    }

    // Pull WHOLE TEAMS: the challenger's team takes their chosen side, the
    // target's team the opposite. Solo players resolve to a single-guid list.
    std::vector<std::string> challenger_team = TeamService::GetTeamMemberGuids(challenger_guid);
    std::vector<std::string> target_team     = TeamService::GetTeamMemberGuids(target_guid);

    // Same-team guard: can't challenge a teammate (would seat a player on both
    // sides). self_challenge is handled above; a DIFFERENT teammate resolving
    // into the challenger's roster is rejected here.
    for (const auto& g : challenger_team) {
        if (g == target_guid) {
            reply("You cannot challenge a teammate.");
            Logger::Log("challenge",
                "[Challenge] guid=%s target='%s' outcome=ignored details=same_team\n",
                challenger_guid.c_str(), args.target_name.c_str());
            return;
        }
    }

    if (!g_challenge_launcher) {
        Logger::Log("challenge",
            "[Challenge] guid=%s outcome=ignored details=no_launcher_registered\n",
            challenger_guid.c_str());
        return;
    }

    Logger::Log("challenge",
        "[Challenge] resolved challenger_team=%zu target='%s' target_team=%zu map=%s mode=%s -- invoking launcher\n",
        challenger_team.size(), args.target_name.c_str(), target_team.size(),
        rec->map_name.c_str(), rec->game_class.c_str());

    const bool ok = g_challenge_launcher(
        challenger_team, args.challenger_tf, target_team, rec->map_name, rec->game_class);

    const char* side = (args.challenger_tf == 2) ? "defenders" : "attackers";
    if (ok) {
        const std::string& shown = rec->friendly_name.empty() ? rec->map_name : rec->friendly_name;
        reply("Challenge sent to " + args.target_name + " on " + shown +
              " (you: " + side + "). Waiting for them to accept...");
    } else {
        reply("Could not start the challenge match. Try again.");
    }
    Logger::Log("challenge",
        "[Challenge] guid=%s target='%s' map=%s challenger_tf=%d challengers=%zu targets=%zu outcome=%s\n",
        challenger_guid.c_str(), args.target_name.c_str(), rec->map_name.c_str(),
        args.challenger_tf, challenger_team.size(), target_team.size(),
        ok ? "instance_spawning" : "spawn_failed");
}

} // namespace ChatCommand
