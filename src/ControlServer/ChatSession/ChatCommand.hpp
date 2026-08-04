#pragma once

#include <cstdint>
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
    // -spawnhenchman: friend-team spawn additionally marked as a henchman
    // (pawn r_bIsHenchman) with the requesting player as leader (m_pOwner).
    bool henchman = false;
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

// Optional team for -spectate <id> [team]. Purely cosmetic — assigns
// PRI.r_TaskForce/Team so the client's own same-team HUD checks (health bars)
// resolve, WITHOUT clearing bOnlySpectator/bIsSpectator/bOutOfLives. The
// spectator still never spawns a pawn: no model, no collision, no scoreboard
// entry, no world interaction. None = today's teamless behavior.
enum class SpectateTeam {
    None,
    Attackers,
    Defenders,
};

// -spectate                       -> list running (non-home-map) instances
//                                    the caller may join as a spectator.
// -spectate <instance_id>         -> join that instance as a teamless spectator.
// -spectate <instance_id> <team>  -> join, cosmetically assigned to <team>
//                                    ("attack"/"attackers" or
//                                    "defense"/"defence"/"defenders") so that
//                                    team's health bars render. Still no pawn.
// All forms are permission-gated (ga_user_roles "spectator") and handled
// entirely on the control server — see ChatSession::HandleSpectateCommand.
struct SpectateArgs {
    int64_t instance_id = 0;  // 0 = list request
    SpectateTeam team = SpectateTeam::None;
};

// -togglebrokensuits [1|0]: per-user preference for seeing stutter-prone
// ("broken") cosmetic suits on other players. 1 = show originals, 0 = show
// the replacement cosmetics, omitted = toggle the current value. Persisted in
// ga_user_preferences and enforced DLL-side at replication time.
// -toggleallsuits [1|0] rides the same struct with all = true: 0 hides EVERY
// suit/helmet/flair on OTHER players for this viewer (stutter triage; own
// character exempt), 1 restores normal.
struct ToggleBrokenSuitsArgs {
    int mode = -1;    // -1 = toggle, 0 = off, 1 = on
    bool all = false; // true = -toggleallsuits variant
};

// -togglesolomode [1|0]: per-user matchmaking preference. When enabled, a PvE
// mission the player queues into spawns as their own PARTY_LOCKED instance —
// nobody else is routed in (they can still be invited, like team missions).
// Persisted in ga_user_preferences ("solo_mode") and handled entirely on the
// control server — no DLL/IPC involvement.
struct ToggleSoloModeArgs {
    int mode = -1;    // -1 = toggle, 0 = off, 1 = on
};

// -enabledlc <identifier> / -disabledlc <identifier>: mark a DLC map pack
// installed / not installed for the calling account (ga_user_dlc via
// Database::SetUserDlc — same flag the dashboard users page toggles). The
// launcher instructs players to run this after installing a pack; manual
// patchers can self-enable the same way. Enabling without actually having the
// files is the player's own problem (they won't be able to load the maps).
// Bare "-enabledlc"/"-disabledlc" replies with the available identifiers.
// Handled entirely on the control server — no DLL/IPC involvement.
struct SetDlcArgs {
    std::string identifier;  // empty = list available packs
    bool installed = false;
};

// -markers [off|all|attackers|defenders|friendly|enemy|glow|foreman|<uu>]:
// SPECTATOR-ONLY team highlight, visible only to the spectator who has it on.
// Auto-enabled on spectator join, so live use needs no command at all. Purely
// visual — the DLL pushes effect group 1469 (FX 131, a silent material swap)
// through SetEffectRep on its own owner-only effect managers. See Markers.hpp.
struct MarkersArgs {
    // "toggle" (default), "off", "all", "attackers", "defenders",
    // "friendly", "enemy", "glow", "foreman", or "distance". String rather
    // than an enum because it rides the JSON args blob straight through to
    // the DLL.
    //
    // "friendly"/"enemy" are resolved DLL-side against the spectator's own
    // PRI.r_TaskForce — the cosmetic team seeded by `-spectate <id> <team>` —
    // so they follow how the caller joined. A teamless spectator has no
    // r_TaskForce and both degrade to "all".
    std::string mode = "toggle";
    // `-markers <uu>`: near-cull radius in UE3 world units. Pawns closer than
    // this to the spectator's view point are not marked, because at close
    // range the world-scaled FX fills the screen. 0 = leave unchanged / use
    // the DLL default. Applied live on an already-enabled session.
    float min_distance_uu = 0.0f;
};

// -fx [next|prev|<n>|off|pawn|own]: step through candidate special-FX on the
// pawn the caller is currently viewing, so a marker look can be picked by eye.
// Dev/preview tooling — see FxBrowse.hpp.
struct FxBrowseArgs {
    // "show" (default), "next", "prev", "jump", "off", "pawn", "own".
    std::string action = "show";
    int index = 0;  // only meaningful for action == "jump"
};

struct ParseResult {
    // True if the message was a /-prefixed slash command attempt that we own
    // (currently: "-changeteam", "-spawnfriend", "-spawnenemy", "-possess",
    // "-unpossess", "-topdown", "-reload-queues", "-spectate",
    // "-unspectate", "-togglebrokensuits", "-toggleallsuits",
    // "-togglesolomode", "-enabledlc", "-disabledlc").
    // False for ordinary chat and for slash commands we don't recognize.
    bool recognized = false;

    // True if the message must NOT be re-broadcast as ordinary chat.
    // Always true when recognized. False for unrecognized chat — pass-through.
    bool suppress_broadcast = false;

    // Only populated when recognized AND the args parsed cleanly.
    std::optional<ChangeTeamTarget> change_team;
    std::optional<SpawnTargetArgs>  spawn_target;
    std::optional<DeployTargetArgs> deploy_target;
    std::optional<TopDownArgs>      topdown;
    std::optional<SpectateArgs>     spectate;
    std::optional<ToggleBrokenSuitsArgs> toggle_broken_suits;
    std::optional<ToggleSoloModeArgs>    toggle_solo_mode;
    std::optional<SetDlcArgs>            set_dlc;
    std::optional<MarkersArgs>      markers;
    std::optional<FxBrowseArgs>     fx_browse;

    // No-arg toggles. Flag is set when recognized + parsed cleanly.
    bool possess   = false;
    bool unpossess = false;

    // -unspectate — leave the instance currently being spectated and return
    // to the home map. No args. Entirely control-server-side, same as
    // -spectate — see ChatSession::HandleUnspectateCommand /
    // TcpSession::DeliverSpectateExit.
    bool unspectate = false;

    // -coords — report the player's current XYZ (map-prep tooling). No args.
    bool coords = false;

    // -fullheal — restore the player's pawn to full health (VR arena / 1v1
    // practice; DLL enforces map gate + per-player cooldown). No args.
    bool fullheal = false;

    // -classes — per-team class counts (assault/medic/recon/robotics) of the
    // sender's current instance, replied privately on the System channel.
    // Handled entirely on the control server. No args.
    bool class_counts = false;

    // -reload-queues — re-read ga_queues + ga_map_pool_entries. Handled
    // entirely on the control server; no PLAYER_ACTION IPC dispatched.
    bool reload_queues = false;

    // -announce <text> — server-wide announcement on chat channel 20, the one
    // channel the client's tab filter cannot exclude. Holds the announcement
    // text (never empty when set). Handled entirely on the control server;
    // caller must check the sender is permitted before acting on it.
    std::optional<std::string> announce;
};

// Resolve a slash-command token (no leading '/', any case) to the chat channel
// it targets. Returns nullopt for tokens that aren't channel commands.
//
// Covers the commands the client forwards to us as PLAYER_COMMAND (0x019F)
// because they aren't in its own name->id map — /t, /l, /a, /al, /rg — plus
// aliases of our own for channels whose combo-box label the client hardcodes
// (City is shown as "/1", so /c and /city are ours to define). Commands the
// client handles itself (/w, /gl, /i) never reach us and are absent here.
std::optional<uint32_t> ChannelForCommandToken(const std::string& token);

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

// Apply a single team move to an already-active player: update the control DB
// task_force, then dispatch an explicit change_team PLAYER_ACTION to the
// player's DLL (which flips the pawn's TaskForce + teleports to the new team's
// player start). Shared by -changeteam and the matchmaking auto-rebalance.
// new_tf is 1 (attackers) or 2 (defenders). is_autobalance=true flags an
// auto-rebalance move so the DLL shows the player an "autobalanced" alert.
void DispatchTeamMove(int64_t instance_id, const std::string& session_guid, int new_tf,
                      bool is_autobalance = false);

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

// Send -coords to the game DLL. Same delivery path; the DLL logs + shows the
// requesting player's current XYZ.
void DispatchCoords(const std::string& session_guid);

// Send -fullheal to the game DLL. Same delivery path; map gate + cooldown
// are enforced DLL-side.
void DispatchFullHeal(const std::string& session_guid);

// -classes: count classes per team in the sender's instance (from
// ga_instance_players — same data Local chat scoping uses) and reply with two
// private System-channel lines ("Your team: a/m/r/b" / "Enemy team: ...").
// Handled entirely on the control server; no PLAYER_ACTION IPC.
void ExecuteClassCounts(const std::string& session_guid);

// Send -topdown to the game DLL. Toggles top-down view in the DLL — repeated
// invocations alternate enter/restore. lift_z=0 means "use the DLL default".
void DispatchTopDown(const TopDownArgs& args, const std::string& session_guid);

// Send -markers to the game DLL. The DLL owns the enable set and the refresh
// timer; repeated bare invocations alternate on/off.
void DispatchMarkers(const MarkersArgs& args, const std::string& session_guid);

// Send -fx to the game DLL. The DLL owns the candidate table and the cursor.
void DispatchFxBrowse(const FxBrowseArgs& args, const std::string& session_guid);

// Send -togglebrokensuits to the game DLL. The DLL owns the preference
// (ga_user_preferences read/write + in-memory cache used at replication).
void DispatchToggleBrokenSuits(const ToggleBrokenSuitsArgs& args,
                               const std::string& session_guid);

// -togglesolomode: persist the preference (ga_user_preferences "solo_mode"),
// update any live queue entry, and reply privately with the new state.
// Handled entirely on the control server; no PLAYER_ACTION IPC.
void ExecuteToggleSoloMode(const ToggleSoloModeArgs& args,
                           const std::string& session_guid);

// -enabledlc / -disabledlc: flip the caller's ga_user_dlc installed flag for
// one pack and reply privately; empty/unknown identifier replies with the
// catalog. DLC-gated queues appear/disappear on the next GET_TICKET_INFO
// poll. Handled entirely on the control server; no PLAYER_ACTION IPC.
void ExecuteSetDlc(const SetDlcArgs& args, const std::string& session_guid);

} // namespace ChatCommand
