#include "src/ControlServer/ChatSession/ChatCommand.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <vector>

#include "lib/nlohmann/json.hpp"

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

const char* SpawnBotTeamName(SpawnBotTeam t) {
    switch (t) {
        case SpawnBotTeam::Attackers: return "attackers";
        case SpawnBotTeam::Defenders: return "defenders";
    }
    return "?";
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

    if (cmd_name == "-spawnbot") {
        // -spawnbot <bot_id> <attackers|defenders>
        out.recognized = true;
        out.suppress_broadcast = true;

        std::vector<std::string> tokens = SplitWs(rest);
        if (tokens.size() != 2) return out;

        std::optional<int> bot_id = ParseInt(tokens[0]);
        if (!bot_id || *bot_id <= 0) return out;

        std::string team_lower = LowerAscii(tokens[1]);
        SpawnBotArgs args;
        args.bot_id = *bot_id;
        if (team_lower == "attackers") {
            args.team = SpawnBotTeam::Attackers;
        } else if (team_lower == "defenders") {
            args.team = SpawnBotTeam::Defenders;
        } else {
            return out;
        }
        out.spawn_bot = args;
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

    // Other "-" text — pass through to broadcast as ordinary chat.
    return out;
}

void DispatchChangeTeam(ChangeTeamTarget target, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchChangeTeam dropped: empty session_guid\n");
        return;
    }

    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "change_team";
    payload["args"]         = { {"target", TargetName(target)} };

    Logger::Log("chat-command", "[ChatCmd] /changeteam guid=%s target=%s -> dispatch\n",
        session_guid.c_str(), TargetName(target));

    (void)TcpSession::DeliverPlayerAction(session_guid, payload);
    // Return value ignored: silent on failure per spec; DeliverPlayerAction logs internally.
}

void DispatchSpawnBot(const SpawnBotArgs& args, const std::string& session_guid) {
    if (session_guid.empty()) {
        Logger::Log("chat-command", "[ChatCmd] DispatchSpawnBot dropped: empty session_guid\n");
        return;
    }

    nlohmann::json payload;
    payload["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
    payload["session_guid"] = session_guid;
    payload["action"]       = "spawn_bot";
    payload["args"]         = {
        {"bot_id", args.bot_id},
        {"team",   SpawnBotTeamName(args.team)},
    };

    Logger::Log("chat-command", "[ChatCmd] /spawnbot guid=%s bot_id=%d team=%s -> dispatch\n",
        session_guid.c_str(), args.bot_id, SpawnBotTeamName(args.team));

    (void)TcpSession::DeliverPlayerAction(session_guid, payload);
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
    Logger::Log("chat-command", "[ChatCmd] /%s guid=%s -> dispatch\n",
        action_name.c_str(), session_guid.c_str());
    (void)TcpSession::DeliverPlayerAction(session_guid, payload);
}

void DispatchPossess(const std::string& session_guid)   { DispatchSimpleAction("possess",   session_guid); }
void DispatchUnpossess(const std::string& session_guid) { DispatchSimpleAction("unpossess", session_guid); }

} // namespace ChatCommand
