#include "src/ControlServer/ChatSession/ChatCommand.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <vector>

#include "lib/nlohmann/json.hpp"

#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
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

    if (cmd_name == "-classes") {
        // No args — per-team class counts of the sender's instance.
        out.recognized = true;
        out.suppress_broadcast = true;
        if (rest.empty()) out.class_counts = true;
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

    if (cmd_name == "-togglesolomode") {
        // -togglesolomode     -> toggle current preference
        // -togglesolomode 1   -> enable (missions lock to the player)
        // -togglesolomode 0   -> disable
        out.recognized = true;
        out.suppress_broadcast = true;
        ToggleSoloModeArgs args;
        if (!rest.empty()) {
            if (rest == "0")      args.mode = 0;
            else if (rest == "1") args.mode = 1;
            else return out;  // bad arg — silent reject
        }
        out.toggle_solo_mode = args;
        return out;
    }

    if (cmd_name == "-enabledlc" || cmd_name == "-disabledlc") {
        // -enabledlc <identifier>  -> mark the pack installed for this account
        // -disabledlc <identifier> -> mark it not installed
        // Bare command -> ExecuteSetDlc replies with the available packs.
        out.recognized = true;
        out.suppress_broadcast = true;
        SetDlcArgs args;
        args.installed = (cmd_name == "-enabledlc");
        if (!rest.empty()) {
            std::vector<std::string> tokens = SplitWs(rest);
            if (tokens.size() != 1) return out;  // identifiers have no spaces — silent reject
            args.identifier = tokens[0];
        }
        out.set_dlc = args;
        return out;
    }

    if (cmd_name == "-fx") {
        // -fx            -> re-show / re-apply current entry
        // -fx next|prev  -> step
        // -fx <n>        -> jump to entry n (1-based)
        // -fx pawn|own   -> switch delivery route
        // -fx off        -> stop
        out.recognized = true;
        out.suppress_broadcast = true;
        FxBrowseArgs args;
        if (!rest.empty()) {
            const std::string tok = LowerAscii(rest);
            if (tok == "next" || tok == "n")      args.action = "next";
            else if (tok == "prev" || tok == "p") args.action = "prev";
            else if (tok == "off")                args.action = "off";
            else if (tok == "pawn")               args.action = "pawn";
            else if (tok == "own")                args.action = "own";
            else {
                std::optional<int> n = ParseInt(tok);
                if (!n || *n < 1) return out;  // bad arg — silent reject
                args.action = "jump";
                args.index  = *n;
            }
        }
        out.fx_browse = args;
        return out;
    }

    if (cmd_name == "-markers") {
        // -markers        -> toggle
        // -markers on|1   -> enable
        // -markers off|0  -> disable
        out.recognized = true;
        out.suppress_broadcast = true;
        // -markers                       -> toggle (defaults: glow route, all)
        // -markers off                   -> off
        // -markers all|attackers|defenders -> pick who gets highlighted
        // -markers glow|foreman          -> pick delivery route
        // -markers <uu>                  -> foreman-route near-cull radius
        MarkersArgs args;
        if (!rest.empty()) {
            const std::string tok = LowerAscii(rest);
            if (tok == "off" || tok == "0")            args.mode = "off";
            else if (tok == "on" || tok == "1" || tok == "all") args.mode = "all";
            else if (tok == "attack" || tok == "attackers")     args.mode = "attackers";
            else if (tok == "defense" || tok == "defence"
                     || tok == "defenders")                     args.mode = "defenders";
            // Relative to the spectator's own -spectate team assignment.
            else if (tok == "friendly" || tok == "friends"
                     || tok == "mine" || tok == "own")          args.mode = "friendly";
            else if (tok == "enemy" || tok == "enemies"
                     || tok == "them" || tok == "theirs")       args.mode = "enemy";
            else if (tok == "glow")                    args.mode = "glow";
            else if (tok == "foreman")                 args.mode = "foreman";
            else {
                try {
                    const float uu = std::stof(tok);
                    // Reject nonsense rather than let a typo cull the map.
                    if (uu <= 0.0f || uu > 100000.0f) return out;
                    args.mode = "distance";
                    args.min_distance_uu = uu;
                } catch (...) {
                    return out;  // bad arg — silent reject
                }
            }
        }
        out.markers = args;
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

    if (cmd_name == "-spectate") {
        // -spectate                      -> list (instance_id stays 0)
        // -spectate <instance_id>        -> join request, teamless
        // -spectate <instance_id> <team> -> join request, cosmetic team assignment
        out.recognized = true;
        out.suppress_broadcast = true;

        SpectateArgs args;
        if (!rest.empty()) {
            std::vector<std::string> tokens = SplitWs(rest);
            if (tokens.empty() || tokens.size() > 2) return out;

            std::optional<int> instance_id = ParseInt(tokens[0]);
            if (!instance_id || *instance_id <= 0) return out;  // bad arg -> silent reject
            args.instance_id = *instance_id;

            if (tokens.size() == 2) {
                const std::string teamTok = LowerAscii(tokens[1]);
                if (teamTok == "attack" || teamTok == "attackers") {
                    args.team = SpectateTeam::Attackers;
                } else if (teamTok == "defense" || teamTok == "defence" || teamTok == "defenders") {
                    args.team = SpectateTeam::Defenders;
                } else {
                    return out;  // unknown team token -> silent reject
                }
            }
        }
        out.spectate = args;
        return out;
    }

    if (cmd_name == "-unspectate") {
        // No args — leave the currently-spectated instance, return home.
        out.recognized = true;
        out.suppress_broadcast = true;
        if (rest.empty()) out.unspectate = true;
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

    // Manual -changeteam is disabled in matchmade Mercenary matches — players
    // were using it to stack teams. Autobalance moves don't come through here
    // (IpcServer calls DispatchTeamMove directly), so rebalancing still works.
    if (auto inst = InstanceRegistry::GetInstanceById(instance_id); inst && inst->queue_id != 0) {
        auto queue_cfg = MatchmakingService::GetQueueConfig(inst->queue_id);
        if (queue_cfg && queue_cfg->name == "merc") {
            Logger::Log("chat-command",
                "[ChatCmd] guid=%s command=-changeteam outcome=denied details=merc_queue_match instance=%lld\n",
                session_guid.c_str(), (long long)instance_id);
            ChatSession::SystemMessageToGuid(session_guid,
                "*** -changeteam is disabled in Mercenary matches ***");
            return;
        }
    }

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

void ExecuteClassCounts(const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] ExecuteClassCounts dropped: empty session_guid\n");
        return;
    }

    auto lookup = InstanceRegistry::GetInstancePlayerTaskForce(session_guid);
    if (!lookup) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-classes outcome=ignored details=no_active_instance_player\n",
            session_guid.c_str());
        ChatSession::SystemMessageToGuid(session_guid, "*** You are not in an instance ***");
        return;
    }
    const int64_t instance_id = lookup->first;
    const int     own_tf      = lookup->second;

    // Index: 0=assault 1=medic 2=recon 3=robotics.
    int own[4] = {0, 0, 0, 0};
    int enemy[4] = {0, 0, 0, 0};
    const auto rows = InstanceRegistry::GetActivePlayersForInstance(instance_id);
    for (const auto& row : rows) {
        int idx = -1;
        switch (row.profile_id) {
            case 680: idx = 0; break;  // PROFILE_ASSAULT
            case 567: idx = 1; break;  // PROFILE_MEDIC
            case 681: idx = 2; break;  // PROFILE_RECON
            case 679: idx = 3; break;  // PROFILE_ROBOTICS
            default: break;
        }
        if (idx < 0) continue;
        if (row.task_force == own_tf) own[idx]++;
        else                          enemy[idx]++;
    }

    char line[64];
    std::snprintf(line, sizeof(line), "Your team: %d/%d/%d/%d",
                  own[0], own[1], own[2], own[3]);
    ChatSession::SystemMessageToGuid(session_guid, line);
    std::snprintf(line, sizeof(line), "Enemy team: %d/%d/%d/%d",
                  enemy[0], enemy[1], enemy[2], enemy[3]);
    ChatSession::SystemMessageToGuid(session_guid, line);

    Logger::Log("chat-command",
        "[ChatCmd] guid=%s command=-classes outcome=sent instance=%lld tf=%d "
        "own=%d/%d/%d/%d enemy=%d/%d/%d/%d roster=%zu\n",
        session_guid.c_str(), (long long)instance_id, own_tf,
        own[0], own[1], own[2], own[3],
        enemy[0], enemy[1], enemy[2], enemy[3], rows.size());
}

void ExecuteToggleSoloMode(const ToggleSoloModeArgs& args,
                           const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] ExecuteToggleSoloMode dropped: empty session_guid\n");
        return;
    }

    auto info = PlayerSessionStore::GetByGuid(session_guid);
    const int64_t user_id = info ? info->user_id : 0;
    if (user_id <= 0) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-togglesolomode outcome=ignored details=no_user_id\n",
            session_guid.c_str());
        return;
    }

    bool enable;
    if (args.mode == -1)
        enable = Database::GetUserPreference(user_id, "solo_mode") != "1";
    else
        enable = args.mode == 1;

    Database::SetUserPreference(user_id, "solo_mode", enable ? "1" : "0");
    MatchmakingService::SetSoloLockForQueuedPlayer(session_guid, enable);

    ChatSession::SystemMessageToGuid(session_guid, enable
        ? "*** Solo mode enabled - missions you enter will be locked to you. "
          "Type -togglesolomode to turn it off. ***"
        : "*** Solo mode disabled. ***");

    Logger::Log("chat-command",
        "[ChatCmd] guid=%s command=-togglesolomode outcome=set user=%lld enabled=%d\n",
        session_guid.c_str(), (long long)user_id, enable ? 1 : 0);
}

void ExecuteSetDlc(const SetDlcArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] ExecuteSetDlc dropped: empty session_guid\n");
        return;
    }

    auto info = PlayerSessionStore::GetByGuid(session_guid);
    const int64_t user_id = info ? info->user_id : 0;
    if (user_id <= 0) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-%sdlc outcome=ignored details=no_user_id\n",
            session_guid.c_str(), args.installed ? "enable" : "disable");
        return;
    }

    const auto catalog = Database::GetAllDlc();
    auto list_catalog = [&](const char* prefix) {
        std::string line = prefix;
        if (catalog.empty()) {
            line += " (no DLC packs configured on this server)";
        } else {
            const auto installed = Database::GetInstalledDlcIds(user_id);
            for (size_t i = 0; i < catalog.size(); ++i) {
                if (i) line += ", ";
                line += catalog[i].identifier;
                for (int64_t id : installed)
                    if (id == catalog[i].dlc_id) { line += " (enabled)"; break; }
            }
        }
        line += " ***";
        ChatSession::SystemMessageToGuid(session_guid, line);
    };

    if (args.identifier.empty()) {
        list_catalog("*** Usage: -enabledlc <identifier> / -disabledlc <identifier>. Available:");
        return;
    }

    const Database::DlcRow* row = nullptr;
    for (const auto& c : catalog)
        if (c.identifier == args.identifier) { row = &c; break; }
    if (!row || !Database::SetUserDlc(user_id, args.identifier, args.installed)) {
        list_catalog(("*** Unknown DLC '" + args.identifier + "'. Available:").c_str());
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-%sdlc outcome=rejected user=%lld identifier='%s'\n",
            session_guid.c_str(), args.installed ? "enable" : "disable",
            (long long)user_id, args.identifier.c_str());
        return;
    }

    // Queue visibility follows on the next GET_TICKET_INFO poll — no relog.
    ChatSession::SystemMessageToGuid(session_guid, args.installed
        ? "*** DLC '" + row->name + "' enabled - its queues will appear in your queue list. "
          "Make sure the map files are actually installed (launcher DLC section), "
          "or those maps will fail to load. Type -disabledlc " + row->identifier + " to turn it off. ***"
        : "*** DLC '" + row->name + "' disabled - its queues will be hidden again. ***");

    Logger::Log("chat-command",
        "[ChatCmd] guid=%s command=-%sdlc outcome=set user=%lld identifier='%s' installed=%d\n",
        session_guid.c_str(), args.installed ? "enable" : "disable",
        (long long)user_id, args.identifier.c_str(), args.installed ? 1 : 0);
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

void DispatchFxBrowse(const FxBrowseArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchFxBrowse dropped: empty session_guid\n");
        return;
    }
    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "fx_browse";
    payload["args"]         = { {"fx_action", args.action}, {"index", args.index} };
    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-fx action=%s outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(), args.action.c_str());
    }
}

void DispatchMarkers(const MarkersArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchMarkers dropped: empty session_guid\n");
        return;
    }
    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "markers";
    payload["args"]         = { {"mode", args.mode},
                                {"min_distance_uu", args.min_distance_uu} };
    const bool sent = TcpSession::DeliverPlayerAction(session_guid, payload);
    if (!sent) {
        Logger::Log("chat-command",
            "[ChatCmd] guid=%s command=-markers mode=%s nearcull=%.0f outcome=ignored details=dispatch_failed\n",
            session_guid.c_str(), args.mode.c_str(), args.min_distance_uu);
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
