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
