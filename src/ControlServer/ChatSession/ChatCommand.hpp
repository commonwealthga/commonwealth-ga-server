#pragma once

#include <optional>
#include <string>

namespace ChatCommand {

enum class ChangeTeamTarget {
    Toggle,
    Attackers,
    Defenders,
};

// "Friend" = the requesting player's task force; "Enemy" = the opposing
// task force. The DLL resolves this against the player's pawn at exec time
// — the control server never needs to know who's attacking vs defending.
enum class SpawnTargetTeam {
    Friend,
    Enemy,
};

struct SpawnTargetArgs {
    int bot_id = 0;
    SpawnTargetTeam team = SpawnTargetTeam::Friend;
    // 0.0 = unspecified → DLL falls back to Config::GetDifficultyScalar()
    // (the map's current difficulty). Otherwise the scalar token mapping:
    //   low=1.0  medium=1.25  high=1.5  max=1.75  umax=2.0
    // Whether scalar is overridden or default, both HP and outgoing-damage
    // scaling are applied (mirrors enemy bot-factory spawn behavior).
    float difficulty_scalar = 0.0f;
};

// -deployfriend / -deployenemy: drop a deployable in front of the player on
// either their own team or the opposing team. Same Friend/Enemy semantics as
// SpawnTargetTeam; the DLL resolves which task force that is at exec time.
struct DeployTargetArgs {
    int deployable_id = 0;
    SpawnTargetTeam team = SpawnTargetTeam::Friend;
};

struct TopDownArgs {
    // World-units to lift the pawn by; 0 means "use the DLL-side default".
    float lift_z = 0.0f;
};

struct ParseResult {
    // True if the message was a /-prefixed slash command attempt that we own
    // (currently: "-changeteam", "-spawnfriend", "-spawnenemy", "-possess",
    // "-unpossess", "-topdown", "-reload-queues"). False for ordinary chat and
    // for slash commands we don't recognize.
    bool recognized = false;

    // True if the message must NOT be re-broadcast as ordinary chat.
    // Always true when recognized. False for unrecognized chat — pass-through.
    bool suppress_broadcast = false;

    // Only populated when recognized AND the args parsed cleanly.
    std::optional<ChangeTeamTarget> change_team;
    std::optional<SpawnTargetArgs>  spawn_target;
    std::optional<DeployTargetArgs> deploy_target;
    std::optional<TopDownArgs>      topdown;

    // No-arg toggles. Flag is set when recognized + parsed cleanly.
    bool possess   = false;
    bool unpossess = false;

    // -reload-queues — re-read ga_queues + ga_map_pool_entries. Handled
    // entirely on the control server; no PLAYER_ACTION IPC dispatched.
    bool reload_queues = false;
};

// Parse a chat MESSAGE string. Trims leading/trailing whitespace, recognises
// "-changeteam" (optional arg "attackers" | "defenders"; bare -> Toggle),
// "-spawnfriend [low|medium|high|max|umax] <bot_id>", and
// "-spawnenemy  [low|medium|high|max|umax] <bot_id>". Unrecognised args ->
// recognised=true, suppress_broadcast=true, payload=nullopt (silent reject).
ParseResult TryParseChatCommand(const std::string& message_text);

// Send the parsed -changeteam command to the game DLL via IPC.
// Logs on the "chat-command" channel; returns nothing — failures are silent
// to the user per design.
void DispatchChangeTeam(ChangeTeamTarget target, const std::string& session_guid);

// Send the parsed -spawnfriend / -spawnenemy command to the game DLL via IPC.
// Same delivery path as DispatchChangeTeam (PLAYER_ACTION over the per-session
// DLL IPC). The DLL resolves Friend/Enemy against the requesting player's
// task force.
void DispatchSpawnTarget(const SpawnTargetArgs& args, const std::string& session_guid);

// Send the parsed -deployfriend / -deployenemy command to the game DLL via IPC.
// Drops a deployable in front of the player on the chosen team.
void DispatchDeployTarget(const DeployTargetArgs& args, const std::string& session_guid);

// Send -possess and -unpossess to the game DLL. Same delivery path.
void DispatchPossess(const std::string& session_guid);
void DispatchUnpossess(const std::string& session_guid);

// Send -topdown to the game DLL. Toggles top-down view in the DLL — repeated
// invocations alternate enter/restore. lift_z=0 means "use the DLL default".
void DispatchTopDown(const TopDownArgs& args, const std::string& session_guid);

} // namespace ChatCommand
