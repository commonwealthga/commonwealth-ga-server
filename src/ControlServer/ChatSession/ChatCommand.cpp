#include "src/ControlServer/ChatSession/ChatCommand.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <vector>

#include "lib/nlohmann/json.hpp"

#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
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

} // namespace

std::optional<uint32_t> ChannelForCommandToken(const std::string& token) {
    const std::string t = LowerAscii(token);
    // Channel ids: see handoff.md §1. Raid (6) is deliberately absent — no
    // raid-group concept exists server-side, so an unknown-command reply beats
    // silently sending into a channel nobody is scoped to.
    if (t == "l"    || t == "local")    return 4;
    if (t == "a"    || t == "agency")   return 2;
    if (t == "al"   || t == "alliance") return 3;
    if (t == "t"    || t == "team")     return 5;
    if (t == "c"    || t == "city")     return 7;
    if (t == "i"    || t == "instance") return 1;
    if (t == "tr"   || t == "trade")    return 12;
    if (t == "lfg")                     return 13;
    return std::nullopt;
}

ParseResult TryParseChatCommand(const std::string& message_text) {
    ParseResult out;

    std::string trimmed = TrimAscii(message_text);
    if (trimmed.empty() || trimmed[0] != '-') {
        // Not a slash command at all.
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

    if (cmd_name == "-fullheal") {
        // No args — heal the player's pawn to full (DLL gates map + cooldown).
        out.recognized = true;
        out.suppress_broadcast = true;
        if (rest.empty()) out.fullheal = true;
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

    if (cmd_name == "-announce") {
        out.recognized = true;
        out.suppress_broadcast = true;
        // Empty text -> nullopt (silent reject). Permission is the caller's
        // job — parsing does not know who sent this.
        if (!rest.empty()) out.announce = rest;
        return out;
    }

    if (cmd_name == "-togglebrokensuits" || cmd_name == "-toggleallsuits") {
        // -togglebrokensuits     -> toggle current preference
        // -togglebrokensuits 1   -> show broken suits (default)
        // -togglebrokensuits 0   -> replace broken suits
        // -toggleallsuits [1|0]  -> same, but 0 hides ALL suits/helmets/flairs
        out.recognized = true;
        out.suppress_broadcast = true;
        ToggleBrokenSuitsArgs args;
        args.all = (cmd_name == "-toggleallsuits");
        if (!rest.empty()) {
            if (rest == "0")      args.mode = 0;
            else if (rest == "1") args.mode = 1;
            else return out;  // bad arg — silent reject
        }
        out.toggle_broken_suits = args;
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
void DispatchFullHeal(const std::string& session_guid)  { DispatchSimpleAction("fullheal",  session_guid); }

void DispatchToggleBrokenSuits(const ToggleBrokenSuitsArgs& args,
                               const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchToggleBrokenSuits dropped: empty session_guid\n");
        return;
    }
    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = args.all ? "toggle_all_suits" : "toggle_broken_suits";
    payload["args"]         = { {"mode", args.mode} };
    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=%s mode=%d outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(),
            args.all ? "-toggleallsuits" : "-togglebrokensuits", args.mode);
    }
}

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

} // namespace ChatCommand
