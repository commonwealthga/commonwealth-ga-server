#pragma once

#include "src/Shared/BuildVersion.hpp"

// NodeMessages.hpp -- control-plane message types on the master <-> node link.
//
// These ride inside sealed frames with kind = kFrameControl and stream = 0. Game
// instance IPC does NOT travel as these messages: it goes over per-instance
// tunnel streams with kind = kFrameInstance, byte for byte, and the master's
// existing IpcServer dispatcher handles it unchanged.

namespace NodeMsg {

// ---- node -> master -------------------------------------------------------

// Heartbeat. Sent every kStatusPeriodMs; also the first thing a node sends after
// the handshake, so the master learns its instance list immediately.
//   { type, ts, instances: [instance_id...], slots_used, slots_total,
//     content_hash, agent_version, dll_name, dll_hash }
constexpr const char* kNodeStatus = "NODE_STATUS";

// The node forked the process and knows the port it bound.
//   { type, instance_id, udp_port, pid }
constexpr const char* kInstanceSpawning = "INSTANCE_SPAWNING";

// The node could not start the instance. Sent as early as it can tell (child
// died, or no IPC tunnel established within its deadline).
//   { type, instance_id, reason }
constexpr const char* kInstanceFailed = "INSTANCE_FAILED";

// The instance process is gone.
//   { type, instance_id, exit_code }
constexpr const char* kInstanceExited = "INSTANCE_EXITED";

// ---- master -> node ------------------------------------------------------

// Sent once, right after the handshake. Carries the values the node needs to
// answer its own instances locally while the link is down.
//   { type, master_ts, device_stats_enabled, effectiveness_enabled,
//     spawn_timeout_s }
constexpr const char* kWelcome = "NODE_WELCOME";

// Start an instance. Everything the local spawner needs is in here so the node
// never has to ask the master a question mid-spawn.
//   { type, instance_id, map_name, game_mode, udp_port, seed,
//     difficulty_kind ("default"|"id"|"scalar"), difficulty_value_id,
//     difficulty_hp, difficulty_dmg, queue_id, is_home_map }
constexpr const char* kInstanceSpawn = "INSTANCE_SPAWN";

// Stop an instance (master shutdown, admin stop, idle reap, or the node's own
// idle timer being overruled).
//   { type, instance_id, reason, grace_seconds }
constexpr const char* kInstanceStop = "INSTANCE_STOP";

// Master -> node: the immutable game data (asm_*/map_*/mod_*) the node needs to
// run matches, as a content-addressed SQLite file. The hash travels in
// NODE_WELCOME so the node knows before it asks for anything.
//   NODE_WELCOME adds: { content_hash, content_size }
//
// Also carries the two non-content artifacts a node may be behind on, which are
// offered the same way (hash announced here, fetched with kFetchContent):
//   NODE_WELCOME adds: { agent_offer_hash, agent_offer_size,
//                        dll_offer_hash, dll_offer_size }
// An empty offer hash means "not offered" (no artifact, or auto-update off).
// Master -> node: the artifacts on offer changed (a new proxy DLL was dropped in
// while the master kept running). Same field names as the NODE_WELCOME block, so
// the node handles both with one path. An empty hash means "nothing on offer".
//   { type, agent_offer_hash, agent_offer_size, dll_offer_hash, dll_offer_size }
constexpr const char* kArtifactOffer = "NODE_ARTIFACT_OFFER";

constexpr const char* kFetchContent = "NODE_FETCH_CONTENT";
//   { type, kind ("content"|"agent"|"dll"), hash, offset, len }
constexpr const char* kContentChunk = "NODE_CONTENT_CHUNK";
//   { type, kind, hash, offset, data (base64), eof, error? }

// Largest chunk a node may ask for. base64 inflates by 4/3, and the result has to
// fit one sealed frame (NodeLink::kMaxFrameSize = 1 MiB).
constexpr int kMaxContentChunk = 256 * 1024;

// Mutable per-player rows. Master -> node for direction "down" (sent immediately
// before the instance is told about the player); node -> master for "up" (pushed
// periodically and at instance end). Same body either way:
//   { type, instance_id, user_id, character_id, direction,
//     tables: { <table>: { columns: [...], rows: [[...], ...] } } }
constexpr const char* kPlayerBundle = "PLAYER_BUNDLE";

// ---- both directions -----------------------------------------------------

// Tells the peer why something was refused. Sent during the handshake and for
// malformed control messages.
constexpr const char* kError = "NODE_ERROR";

// Liveness probe / reply. NODE_STATUS is the primary heartbeat; ping exists so a
// node can prove the link is alive during a long quiet period without sending a
// full status.
constexpr const char* kPing = "NODE_PING";
constexpr const char* kPong = "NODE_PONG";

// ---- identity ------------------------------------------------------------

// Reported by a node in NODE_STATUS so the master can tell two node builds apart.
// Bump when a node's behaviour changes in a way an operator needs to know about.
// The version policy that acts on a mismatch (drain, never break a live match)
// lands with artifact delivery in phase 4; until then this is visibility only.
// Build identity, from git at build time (scripts/gen_version.py) -- not a
// hand-maintained number. Master and node run the same binary, so both report
// this and the master compares a node's report against its own.
constexpr const char* kAgentVersion = BuildVersion::Describe();

// ---- timing --------------------------------------------------------------

// How often a node pushes NODE_STATUS.
constexpr int kStatusPeriodMs = 5000;

// A node with no status/heartbeat for this long is STALE (still usable, but not
// chosen for new work), and DEAD (link torn down) at twice this.
constexpr int kStaleAfterMs = 15000;

}  // namespace NodeMsg
