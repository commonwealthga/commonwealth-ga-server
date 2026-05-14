#pragma once

#include <optional>
#include <string>

namespace ChatCommand {

enum class ChangeTeamTarget {
    Toggle,
    Attackers,
    Defenders,
};

enum class SpawnBotTeam {
    Attackers,
    Defenders,
};

struct SpawnBotArgs {
    int bot_id = 0;
    SpawnBotTeam team = SpawnBotTeam::Attackers;
};

struct ParseResult {
    // True if the message was a /-prefixed slash command attempt that we own
    // (currently: "-changeteam", "-spawnbot", "-possess", "-unpossess"). False
    // for ordinary chat and for slash commands we don't recognize.
    bool recognized = false;

    // True if the message must NOT be re-broadcast as ordinary chat.
    // Always true when recognized. False for unrecognized chat — pass-through.
    bool suppress_broadcast = false;

    // Only populated when recognized AND the args parsed cleanly.
    std::optional<ChangeTeamTarget> change_team;
    std::optional<SpawnBotArgs>     spawn_bot;

    // No-arg toggles. Flag is set when recognized + parsed cleanly.
    bool possess   = false;
    bool unpossess = false;
};

// Parse a chat MESSAGE string. Trims leading/trailing whitespace, recognises
// "-changeteam" (optional arg "attackers" | "defenders"; bare -> Toggle) and
// "-spawnbot <bot_id> <attackers|defenders>". Unrecognised args -> recognised=
// true, suppress_broadcast=true, payload=nullopt (silent reject).
ParseResult TryParseChatCommand(const std::string& message_text);

// Send the parsed -changeteam command to the game DLL via IPC.
// Logs on the "chat-command" channel; returns nothing — failures are silent
// to the user per design.
void DispatchChangeTeam(ChangeTeamTarget target, const std::string& session_guid);

// Send the parsed -spawnbot command to the game DLL via IPC. Same delivery
// path as DispatchChangeTeam (PLAYER_ACTION over the per-session DLL IPC).
void DispatchSpawnBot(const SpawnBotArgs& args, const std::string& session_guid);

// Send -possess and -unpossess to the game DLL. Same delivery path.
void DispatchPossess(const std::string& session_guid);
void DispatchUnpossess(const std::string& session_guid);

} // namespace ChatCommand
