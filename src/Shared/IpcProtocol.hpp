#pragma once

#include <string>
#include <cstdint>

// IpcProtocol.hpp -- Shared IPC message type constants and wire format documentation.
// ARCH: This file must compile on both i686 (DLL, Windows/Wine) and x86_64 (control server, Linux).
//       Include ONLY standard C++ headers. Never include pch.hpp, windows.h, or SDK headers.

namespace IpcProtocol {

// ---------------------------------------------------------------------------
// Message type strings (used in JSON "type" field)
// ---------------------------------------------------------------------------

constexpr const char* MSG_PING = "PING";
constexpr const char* MSG_PONG = "PONG";

constexpr const char* MSG_PLAYER_REGISTER     = "PLAYER_REGISTER";
constexpr const char* MSG_PLAYER_REGISTER_ACK = "PLAYER_REGISTER_ACK";

// Sent by the control server when the client emits PLAYER_LEAVE_GAME (0x0200)
// over TCP. The game DLL looks up the UDP UNetConnection by session_guid and
// invokes NetConnection__Cleanup so the stale Pawn/PlayerController and the
// ClientConnections entry are torn down immediately rather than waiting for
// the engine's 30s ConnectionTimeout — without this, picking a character
// again after Disconnect routes the new HELLO packets through a stale
// USOCK_Open connection and the client times out back to login.
constexpr const char* MSG_PLAYER_LEAVE = "PLAYER_LEAVE";

// Control server → game DLL. Force-drop a player's UDP connection whose
// client can no longer be trusted to quiesce first (control TCP socket died,
// re-login takeover evicted the session). Unlike MSG_PLAYER_LEAVE this does
// NOT drive NetConnection__Cleanup — that crashes on a still-live connection —
// it flips the connection to USOCK_Closed so the engine's own TickDispatch
// reap tears it down this frame (which then emits PLAYER_LEFT as normal).
// Fields: session_guid, register_token (0 = match any).
constexpr const char* MSG_PLAYER_CLOSE = "PLAYER_CLOSE";

constexpr const char* MSG_INSTANCE_HELLO     = "INSTANCE_HELLO";
// ACK fields: accepted, stats_enabled (master — false on home maps), plus
// the recording toggles device_stats_enabled / effectiveness_enabled
// (control-server.json, both default true; the DLL also defaults absent
// fields to true so an older control server keeps full recording).
constexpr const char* MSG_INSTANCE_HELLO_ACK = "INSTANCE_HELLO_ACK";
constexpr const char* MSG_INSTANCE_READY     = "INSTANCE_READY";

constexpr const char* MSG_GAME_EVENT      = "GAME_EVENT";
constexpr const char* MSG_ADMIN_ACTION    = "ADMIN_ACTION";
constexpr const char* MSG_ADMIN_ACTION_ACK = "ADMIN_ACTION_ACK";

constexpr const char* MSG_PLAYER_JOINED  = "PLAYER_JOINED";
constexpr const char* MSG_PLAYER_LEFT    = "PLAYER_LEFT";
constexpr const char* MSG_INSTANCE_EMPTY = "INSTANCE_EMPTY";

// Sent by a mission instance at end-of-mission to ask the control server to
// pre-warm a home map instance. The control server reuses the same on-demand
// spawn callback the login flow uses; this just front-runs it so the home
// map is READY by the time the player clicks "End Mission" (which fires
// GSC_CHANGE_INSTANCE{0,0} → control server routes them home).
constexpr const char* MSG_NEED_HOME_MAP = "NEED_HOME_MAP";

// Sent by a mission instance from inside BeginEndMission. Tells the control
// server (a) stamp end_mission_at on the ga_instances row, and (b) flip any
// DRAFTING successor instance for this parent to READY. After this fires,
// GSC_CHANGE_INSTANCE{0,0} from any player in this mission consults the
// stamped end_mission_at + the queue's continue_in_queue config to decide
// whether to route them home or to the successor.
//   { "type": "MISSION_ENDED", "instance_id": <int64> }
constexpr const char* MSG_MISSION_ENDED = "MISSION_ENDED";

// DEV shortcut (control server -> instance), only sent for the
// token-gated admin action "dev-end-mission" when config dev_controls=true.
// Ends the running mission through the normal BeginEndMission flow.
//   { "type": "DEV_END_MISSION", "result": "win"|"lose" }
constexpr const char* MSG_DEV_END_MISSION = "DEV_END_MISSION";

// DEV shortcut (control server -> instance), admin action
// "dev-teleport", dev_controls only. Moves every player pawn in the instance.
//   { "type": "DEV_TELEPORT", "x": f, "y": f, "z": f, "yaw": i }
constexpr const char* MSG_DEV_TELEPORT = "DEV_TELEPORT";
// DEV shortcut (control server -> instance), admin action
// "dev-kill", dev_controls only. Kills every player pawn in the instance
// through the normal suicide/death path (death screen, release, revive tests).
//   { "type": "DEV_KILL" }
constexpr const char* MSG_DEV_KILL = "DEV_KILL";
// Control server -> instance, admin actions "spawn-group" / "despawn-group" /
// "group-status": a named group of server-spawned bots at a world position.
//   { "type": "SPAWN_GROUP", "tag": s, "x": f, "y": f, "z": f, "yaw": i,
//     "radius": f, "task_force": 1|2, "hp_scale": f, "dmg_scale": f,
//     "bots": [ { "bot_id": i, "count": i }, ... ] }
//   { "type": "DESPAWN_GROUP", "tag": s }   { "type": "GROUP_STATUS_REQUEST", "tag": s }
constexpr const char* MSG_SPAWN_GROUP          = "SPAWN_GROUP";
constexpr const char* MSG_DESPAWN_GROUP        = "DESPAWN_GROUP";
constexpr const char* MSG_GROUP_STATUS_REQUEST = "GROUP_STATUS_REQUEST";
// Control server -> instance, admin action "group-control" on a spawned group:
//   { "type": "GROUP_CONTROL", "tag": s, "passive": b, "regen_frac_s": f,
//     "home_radius": f, "settle_s": f, "max_return_s": f, "speed_scale": f, "heal_full": b }
// passive=true: members drop their targets and stay passive (regen_frac_s of max HP
// per second) until a member takes damage from a player (the group targets that
// player again, HP kept) or every member stood within home_radius of its spawn spot
// for settle_s (then full HP). Members not home after max_return_s are moved there.
// "hp_scale": f = max HP = spawn base x f, current HP keeps its fraction; "dmg_scale": f =
// outgoing damage = spawn base x f; "protect": b = members take no damage (shipped
// Invulnerable effect group 8698, clients show *Immune*) + optional display-only
// "aura_group": effect group id while protected; "regen_cap_frac": f with regen_frac_s
// = regen while NOT passive up to f x max HP (0 = off); "area_radius": f = radius for
// status "near" (spawn-group takes area_radius too, default 6000).
// Dev only (dev_controls): "dev_damage_frac": f, "dev_credit_char": n = take f of max HP
// off each member as damage by character n (0 HP -> member dies; none while protected).
constexpr const char* MSG_GROUP_CONTROL        = "GROUP_CONTROL";
// Control server -> instance, admin action "play-fx": one-shot effect at a
// world position for players within radius (sounds = SoundCue paths, played
// at the first live member of source_tag if given; emitter = ParticleSystem
// path; effect_groups = up to 3 effect group ids queued one-shot on that member,
// whose FX each client plays from its own data; fire_fx = one device fire
// entry in that member's replicated flash queue, so each client plays the
// equipped device's own fire/impact FX at x,y,z - nothing fires on the server).
//   { "type": "PLAY_FX", "x": f, "y": f, "z": f, "radius": f, "source_tag": s,
//     "sounds": [s, ...], "emitter": s, "emitter_life": f, "effect_groups": [i, ...],
//     "fire_fx": { "device_id": i, "mode": i (fire mode index, 0..7) } }
constexpr const char* MSG_PLAY_FX = "PLAY_FX";
// Control server -> instance, admin action "set-world-flag": set one replicated
// world-state flag on the instance's game replication info.
//   { "type": "SET_WORLD_FLAG", "flag": "defense_alarm", "value": 0|1 }
constexpr const char* MSG_SET_WORLD_FLAG = "SET_WORLD_FLAG";
// Control server -> instance, admin action "place-chest": move one of the
// level's placed chests (assembly name preferred) to x,y,z for life_s
// seconds (default 300, max 3600). Each Use sends GAME_EVENT chest_use
// {tag, character_id}; GAME_EVENT chest_status {tag, placed, reason} on
// place / release / expiry / no chest in the level.
//   { "type": "PLACE_CHEST", "tag": s, "x": f, "y": f, "z": f, "yaw": f,
//     "assembly": s, "life_s": f }
constexpr const char* MSG_PLACE_CHEST = "PLACE_CHEST";
// Control server -> instance, admin action "release-chest": { "tag": s }.
constexpr const char* MSG_RELEASE_CHEST = "RELEASE_CHEST";
// Instance -> control server: state of a group after spawn / despawn / request.
//   { "type": "GROUP_STATUS", "tag": s, "alive": i, "spawned": i,
//     "members": [ { "bot_id": i, "hp": i, "hp_max": i, "x": f, "y": f, "z": f, "target": b }, ... ],
//     "passive": b, "interrupts": i, "settled": i (passive returns that ended at home),
//     "contrib": [ {char_id, dmg, heal} ], "protected": b, "near": i, "near_ids": [char_id],
//     "fighters": [ {char_id, ago (s since last fight action, <= 60)} ] }
constexpr const char* MSG_GROUP_STATUS = "GROUP_STATUS";
// Control server -> instance, admin action "team-bot": add/remove a
// dressed class bot on a task force, or ask for the roster.
//   { "type": "TEAM_BOT", "op": "add", "task_force": 1|2, "class": s, "tag": s }
//   { "type": "TEAM_BOT", "op": "remove", "tag": s, "slot": i (0 = all with tag), "now": b }
//   { "type": "TEAM_BOT", "op": "roster" }
// remove without "now" = the bot leaves at its next death (no respawn).
// The instance answers every op with TEAM_ROSTER:
//   { "type": "TEAM_ROSTER", "map": s, "pvp": b, "result": s,
//     "teams": [ { "task_force": i, "players": i, "bots": i } ],
//     "bots": [ { "slot": i, "tag": s, "class": s, "task_force": i, "alive": b, "leaving": b } ] }
constexpr const char* MSG_TEAM_BOT    = "TEAM_BOT";
constexpr const char* MSG_TEAM_ROSTER = "TEAM_ROSTER";
// Standalone (-node) instance -> control server: economy event forwarded from an
// instance that runs with its own local DB.
//   { "type": "NODE_CLAIM", "instance_id": i, "kind": "kill"|"consume",
//     "user_id": i, "character_id": i, "bot_id": i (kill), "inventory_id": i (consume),
//     "seq": i (per-instance, increasing), "t": <seconds since instance start> }
constexpr const char* MSG_NODE_CLAIM = "NODE_CLAIM";
// Control server -> instance after a craft_item GAME_EVENT
//   { "type": "CRAFT_RESULT", "pawn_id": i, "ok": b }
constexpr const char* MSG_CRAFT_RESULT = "CRAFT_RESULT";
// Control server -> instance after a quest turn-in paid xp/credits
//   { "type": "QUEST_REWARD", "pawn_id": i, "character_id": i }
// The instance re-reads the wallet from the DB and restates the pawn.
constexpr const char* MSG_QUEST_REWARD = "QUEST_REWARD";
// Control server -> instance after it decided a quest request
//   { "type": "QUEST_KISMET", "quest_id": i, "link": i, "requirement_id": i }
// requirement_id 0 -> TgSeqEvent_QuestUpdated output link (0 accepted,
// 1 requirements complete, 2 turned in, 3 abandoned); > 0 ->
// TgSeqEvent_QuestRequirementUpdated for that requirement.
constexpr const char* MSG_QUEST_KISMET = "QUEST_KISMET";
// Control server -> instance after a mail action
//   { "type": "MAIL_NOTIFY", "session_guid": s, "character_id": i, "msg_id": u, "wallet": b }
// msg_id != 0 -> alert popup on that player's connection; wallet -> restate credits.
constexpr const char* MSG_MAIL_NOTIFY = "MAIL_NOTIFY";
// Control server -> instance, client /emote (PLAYER_EMOTE 0x1C4)
//   { "type": "PLAYER_EMOTE", "session_guid": s, "emote": u }  -> TgPawn.SetEmote(emote, 0)
constexpr const char* MSG_PLAYER_EMOTE = "PLAYER_EMOTE";

// One row for ga_match_events, sent by the DLL the moment the event
// happens. Control server stamps wall-clock ts at insert. KILL messages
// carry an optional "assists" array; the server inserts the KILL row
// first and writes its rowid into each ASSIST row's detail column.
// All identity fields optional (0/absent = NULL): actor_user_id,
// actor_character_id, actor_bot_id, actor_task_force, target_*,
// owner_user_id, owner_character_id, device_id, detail, flags.
//   { "type": "MATCH_EVENT", "instance_id": <int64>, "game_time": <float>,
//     "event_type": "KILL", ..., "assists": [{...}] }
constexpr const char* MSG_MATCH_EVENT = "MATCH_EVENT";

// Absolute per-stint totals for ga_match_player_stats. Upsert keyed
// (instance_id, character_id, task_force) — idempotent, resend-safe.
//   { "type": "MATCH_STATS", "instance_id": <int64>, "user_id": <int64>,
//     "character_id": <int64>, "task_force": <int>, "scores": [11 ints],
//     "capture_seconds": <float>, "contest_seconds": <float>,
//     "objective_captures": <int>, "beacon_spawns_provided": <int>,
//     "beacon_spawns_used": <int>, "beacons_destroyed": <int>,
//     "time_played_seconds": <float> }
constexpr const char* MSG_MATCH_STATS = "MATCH_STATS";

// Absolute per-device totals for ga_match_device_stats — the server-side
// record of the same counters DeviceStats replicates to the end-of-mission
// Device Stats tab. One message per (character, task force, device) at leave
// and at FlushAll. Upsert keyed (instance_id, character_id, task_force,
// device_id) — idempotent, resend-safe. No DPM/HPM: those are rates derived
// from ga_match_player_stats.time_played_seconds, not stored.
//   { "type": "MATCH_DEVICE_STATS", "instance_id": <int64>,
//     "user_id": <int64>, "character_id": <int64>, "task_force": <int>,
//     "device_id": <int>, "damage": <int>, "healing": <int>,
//     "player_kills": <int>, "bot_kills": <int>,
//     "debuffs_removed": <int>, "overheal": <int>, "uses": <int>,
//     "power_restored": <int>, "power_wasted": <int>,
//     "buffed_damage_dealt": <int>, "protected_damage_taken": <int>,
//     "rescues": <int> }
// debuffs_removed = effect groups stripped by this device's property-140
// cleanses; overheal = heal magnitude clamped away on full-health targets;
// uses = DeviceFiring activations; power_restored/power_wasted = prop-243
// Power Pool split at the pool cap; buffed_damage_dealt / protected_damage_
// taken = damage dealt/taken by pawns inside this device's tracked buff
// window (BuffWindowTracking's table); rescues = under-25% conditional
// deliveries (Triage's own group, or a Savior trigger this device carried).
// boost_targets / boost_overwrites / boost_wasted_secs = tracked-boost
// reach and newest-wins overwrite waste. Every fresh boost application
// emits a BOOST_APPLY MATCH_EVENT and each overwrite a BOOST_OVERWRITE
// (actor = the overwriter, owner = the medic whose ticks were lost,
// detail = seconds remaining) — a cast's APPLY minus OVERWRITE rows =
// its fresh coverage. Allowlisted activatables (group heals + boosts)
// additionally emit timestamped DEVICE_USED MATCH_EVENT rows for
// per-cast joins.
constexpr const char* MSG_MATCH_DEVICE_STATS = "MATCH_DEVICE_STATS";

// Sent by a mission instance at the game-mode-specific pre-warm trigger
// (e.g. when one team passes the 50% threshold, or a 60s-remaining alert
// fires — exact trigger TBD per game mode). Asks the control server to
// spawn a successor instance for the same queue and bind it to this
// mission via predecessor_instance_id. Idempotent — control server dedupes
// against existing DRAFTING/READY successors of the same parent.
//   { "type": "REQUEST_SUCCESSOR", "instance_id": <int64> }
constexpr const char* MSG_REQUEST_SUCCESSOR = "REQUEST_SUCCESSOR";

// Sent by a mission instance ~5s before the setup timer ends. Asks the control
// server to rebalance the already-spawned roster using the queue's taskforce
// policy and dispatch any required team moves back over PLAYER_ACTION. Control
// server no-ops for non-balanced (pinned) queues. One-shot per setup phase.
//   { "type": "REQUEST_REBALANCE", "instance_id": <int64> }
constexpr const char* MSG_REQUEST_REBALANCE = "REQUEST_REBALANCE";

// Sent when a player Uses a Map Transition omega volume (volume type 1255).
// map_game_id is the destination carried by the volume's asm_data_set_ui_volumes
// row (+0x40 on the unmarshalled struct). The control server validates it
// (map_game_info.gameplay_type_value_id must be 1554, "PVE- Open Zone"),
// finds or spawns the single shared instance for that map, and routes the
// player there with PLAYER_REGISTER + GSC_GO_PLAY. Idempotent — spamming Use
// is deduped both per-map (one spawn) and per-session (one poll in flight).
//   { "type": "REQUEST_TRAVEL", "instance_id": <int64>,
//     "session_guid": <string>, "map_game_id": <int> }
constexpr const char* MSG_REQUEST_TRAVEL = "REQUEST_TRAVEL";

// Control server → game DLL. Ask the DLL to emit a bare ga_match_events row
// (no actor/target identity) back over MATCH_EVENT, stamped with the DLL's
// game_time. Routed through the DLL — instead of inserted directly — so the
// row's position in the event stream matches the DLL-emitted events around
// it. Used for AUTOBALANCE_START / AUTOBALANCE_END markers bracketing a
// rebalance move batch (detail = number of moves dispatched).
//   { "type": "EMIT_MATCH_EVENT", "event_type": "AUTOBALANCE_START",
//     "detail": <int64> }
constexpr const char* MSG_EMIT_MATCH_EVENT = "EMIT_MATCH_EVENT";

constexpr const char* MSG_PLAYER_ACTION = "PLAYER_ACTION";

// Sent by a mission instance on a per-pawn rate-limited timer (not on every
// engine tick) for whichever real-player pawns are alive. Purely transient
// live state for the spectator broadcast overlay — control server holds the
// latest one per (instance_id, session_guid) in memory only, no DB write.
// effect_ids are the pawn's currently-applied UTgEffectGroup ids
// (s_AppliedEffectGroups + s_SkillBasedEffectGroups), used to show active
// buffs (Frenzy, Protection Wave, Sensor Boost, etc.) on the overlay.
//   { "type": "PAWN_HEALTH_SNAPSHOT", "instance_id": <int64>,
//     "session_guid": <string>, "task_force": <int 0|1|2>,
//     "health": <int>, "health_max": <int>, "effect_ids": [<int>, ...] }
constexpr const char* MSG_PAWN_HEALTH_SNAPSHOT = "PAWN_HEALTH_SNAPSHOT";

// Sent by a mission instance on the same ~1/sec rate-limited timer basis as
// PAWN_HEALTH_SNAPSHOT, but once per instance (there's one ATgGame per
// match, not one per pawn). "mode" is derived server-side primarily from
// map_game_info.gameplay_type_value_id (the real differentiator between
// modes sharing one TgGame class -- e.g. TgGame_Mission covers both Breach
// and solo/PvE missions), falling back to class-name-only detection when a
// map has no map_game_info row (older/local DB). Modes with no requested
// overlay treatment (Home, stock CTR/DualCTF, City, OpenWorldPVE/PVP, CTF)
// never push at all. Fields are ADDITIVE, not mutually exclusive by mode:
//   - mission_remaining_seconds/mission_timer_state: EVERY mode, always
//     present. The engine's own single authoritative countdown
//     (TgGame__MissionTimeRemaining.cpp), not derived from map_game_info's
//     static mission_time_secs -- that's just the initial configured
//     length; this live value already accounts for extensions/overtime/
//     pauses a static value never could. mission_timer_state is
//     TgGame.ETgMissionTimerState (TGMTS_*): 0 unset, 1 setup, 2 running,
//     3 overtime, 4 complete, 5 paused, 6 custom (Arena/Defense's round/
//     wave timer -- for raid this is the CURRENT WAVE's countdown, not
//     total match time, since raid has no single continuous match clock).
//   - attacker_points/defender_points/points_to_win: ticket AND koth (both
//     track a first-to-N team score via ATgRepInfo_TaskForce::r_nCurrentPointCount).
//   - active_index: koth (PointRotation) only -- array position of the
//     current rotation point in "objectives", -1 before the first pick
//     (INSTANCE_RESTORE continues on that point).
//   - round_current/round_max: raid only (shown as "Wave" on the overlay,
//     not "Round" -- same underlying s_nRoundNumber/s_nMaxRoundNumber).
//   - boss_health/boss_health_max/boss_name: raid, pve, superagent.
//   - friendly_health/friendly_health_max/friendly_name: raid only -- a
//     defender-owned NPC/reactor the defenders are protecting (e.g.
//     "Bancroft" on Raid_DomeCityDefense_P), separate from the boss.
//   - objectives: ticket/koth/payload/breach/superagent (every mode except
//     raid and plain pve) -- a priority-ordered list of every
//     ATgMissionObjective on the game's GameReplicationInfo --
//     id/name/priority/locked/active/owner_taskforce plus a 0..1
//     capture_pct (m_fCurrCaptureTime / m_fTimeToCapture, unclamped same as
//     the server's own SuperAgent capture-bar math). NOTE: id, name, and
//     priority are NOT guaranteed unique per point -- e.g. every koth
//     rotation point shares the same nObjectiveId/nPriority/name by design
//     (see CtrPointRotation.cpp), and at least some Payload maps reuse one
//     id across all their checkpoints too -- consumers must key UI elements
//     by array position, never by id/priority, and must fall back to
//     numbering when multiple concurrently-shown points share a name.
//   ticket: { "mode":"ticket", "attacker_points":<int>, "defender_points":<int>,
//     "points_to_win":<int>, "objectives":[...] }
//   raid:   { "mode":"raid", "round_current":<int>, "round_max":<int>,
//     "boss_health":<int>, "boss_health_max":<int>, "boss_name":<string>,
//     "friendly_health":<int>, "friendly_health_max":<int>, "friendly_name":<string> }
//   koth:   { "mode":"koth", "attacker_points":<int>, "defender_points":<int>,
//     "points_to_win":<int>, "objectives":[...] }
//   pve:        { "mode":"pve", "boss_health":<int>, "boss_health_max":<int>, "boss_name":<string> }
//   superagent: { "mode":"superagent", "boss_health":<int>, "boss_health_max":<int>,
//     "boss_name":<string>, "objectives":[...] }  -- boss then 2 sequential captures
//   payload/breach: { "mode":"payload"|"breach", "objectives":[...] }
//   every mode, optional: "players":[{"user_id","character_id","task_force",
//     "scores":[11 ints]}] = live PRI r_Scores (host migration carries these).
constexpr const char* MSG_MISSION_PROGRESS_SNAPSHOT = "MISSION_PROGRESS_SNAPSHOT";

// Control server -> instance: carry a match over from another instance of the
// same map + mode. "progress" is a MISSION_PROGRESS_SNAPSHOT body (above);
// the instance applies it once its mission timer runs (InstanceRestore).
// progress.players seeds each arriving player's scoreboard (MatchStats carry).
//   { "type": "INSTANCE_RESTORE", "progress": { ... } }
constexpr const char* MSG_INSTANCE_RESTORE = "INSTANCE_RESTORE";

// Link owner -> instance: push one MISSION_PROGRESS_SNAPSHOT on the next game
// tick (outside the normal interval), marked "final": true. Sent right before
// an instance is stopped on purpose so the last state is exact.
//   { "type": "INSTANCE_SNAPSHOT_REQUEST" }
constexpr const char* MSG_INSTANCE_SNAPSHOT_REQUEST = "INSTANCE_SNAPSHOT_REQUEST";

// ---------------------------------------------------------------------------
// IPC transport
// ---------------------------------------------------------------------------

// Default port for the IPC TCP socket (DLL connects, control server listens).
// Configurable via CLI arg --ipc-port on the control server side.
constexpr uint16_t DEFAULT_IPC_PORT = 9010;

// ---------------------------------------------------------------------------
// GUID format documentation
// ---------------------------------------------------------------------------
//
// Session GUIDs are 32-character lowercase hex strings with NO dashes.
// Example: "a3f2c1d8e4b06912345678abcdef1234"
//
// Generated by TcpSession::GenerateSessionGuid() which XORs timestamp + counter
// bytes through a random seed and formats as %02x.
//
// Never convert to/from UUID dash format -- use the 32-hex-char form everywhere
// (PlayerRegistry lookups, IPC messages, log output).

} // namespace IpcProtocol
