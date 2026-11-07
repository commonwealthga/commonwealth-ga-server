#pragma once

#include "src/pch.hpp"
#include "lib/nlohmann/json.hpp"

// Named groups of server-spawned bots at a world position
// (control-server admin actions spawn-group / despawn-group / group-status).
// Generic primitive: callers decide what to spawn, where and when.
namespace WorldGroups {

// SPAWN_GROUP payload (see IpcProtocol.hpp). Replaces an existing group with
// the same tag. Sends GROUP_STATUS back. "bar": true (+ "bar_link" tag prefix,
// default the tag) = boss fight bar on the first member (see BarValue).
void Spawn(const nlohmann::json& j);
// Destroys every live member of the group. Sends GROUP_STATUS back.
void Despawn(const std::string& tag);
// Sends GROUP_STATUS for the group.
void ReportStatus(const std::string& tag);
// Combat event from TgEffect::TrackStats (charId = player character after
// pet -> owner resolution; targetCharId = healed player's character or 0).
// Damage on a live member is tallied per character id; healing is tallied
// for characters that damaged the group. Reported as GROUP_STATUS contrib[].
// attacker = the damaging pawn (pet -> owner resolved); a damaged member of a
// passive group (see Control) ends the group's passive state and targets it.
void OnCombat(int64_t charId, AActor* target, int64_t targetCharId, float magnitude, bool isHeal,
              AActor* attacker = nullptr);
// GROUP_CONTROL payload (see IpcProtocol.hpp): passive return to the members'
// spawn spots with optional regen, ground speed scale, full heal; live HP/damage
// rescale (HP fraction kept), protection on/off (+ optional effect-group visual),
// regen up to a cap while not passive. Sends GROUP_STATUS back.
void Control(const nlohmann::json& j);
// Per-frame driver for passive groups (self-throttles to ~4 Hz).
void Tick();
// SetWhatToDoNext intercept (engage): members the drive sends somewhere get their
// action pick rewritten to "run to m_vTriggerLocation" (238/289) at that point.
void OverrideMovement(ATgAIController* aic, int* nMovementCode, int* nMoveDestination);
// TargetInLOS fallback for engaged world-group members (origOk = the native answer).
// True = treat the AI's enemy as in line of sight: a clear world trace from the member's eye
// (big scaled pawns' native eye trace fails) to the enemy's head or centre within 5000 uu.
// Also counts the native checks per member for group-status ("fire" diagnostics).
bool FireSight(ATgAIController* aic, bool origOk);
// Boss fight bar (groups spawned with "bar": true): true if pri is a bar
// boss's PRI; *value = the r_eBonusType marker this viewer gets (0xB.. = bar,
// 0xA.. = world boss but this viewer is not fighting it).
bool BarValue(ATgRepInfo_Player* pri, ATgPawn* viewer, int* value);
// True if a is a live member pawn of any spawned group (world boss, escort).
bool IsMember(AActor* a);
// A heal effect landed on a group member (TrackStats): with native_regen false that much
// HP rise is allowed (other rises are reverted).
void OnMemberHealed(AActor* member, float amount, ATgPawn* healer);
// Damage event where a group member hurt a player character (victimCharId):
// the victim counts as engaged with that group (status fighters[], heal credit).
void OnMemberHit(AActor* member, int64_t victimCharId);
// PLAY_FX payload (see IpcProtocol.hpp): one-shot sound cue(s), particle
// emitter and camera shake at a world position for players within radius.
void PlayFx(const nlohmann::json& j);
// PLACE_CHEST payload (see IpcProtocol.hpp): spawns a NEW TgChestActor on the
// ground at x,y,z under tag; uses are reported as GAME_EVENT chest_use.
void PlaceChest(const nlohmann::json& j);
// Hides + destroys the tagged chest (also done after life_s).
void ReleaseChest(const std::string& tag);
// Called from TgChestActor OpenChest: true if the chest is a placed one
// (use reported, chest stays closed).
bool OnChestUse(ATgChestActor* chest, ATgPawn* pawn);
// Per-frame life_s expiry for placed chests.
void TickChests();

} // namespace WorldGroups
