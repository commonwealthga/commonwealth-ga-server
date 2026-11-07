#pragma once

class ATgGame;
class ATgGame_Ticket;
class ATgMissionObjective;
class ATgPawn;
struct FVector;

// Acquisition (TgGame_DualCTF) on the CTR_DuelStrike maps — the original 1.4
// mode: each team's robot waits on a pickup platform in the ENEMY base; a
// player crews it (key B = PickupPutdownDeployable -> TgPawn.PickupNearestBot
// -> CrewBot) and drives it home onto the team's capture platform
// (TgFlagCaptureVolume). First team to kCapsToWin captures wins; at time-out
// the team with more captures wins.
//
// The binary still has the UnrealScript side (CTFBot RequestFlagSpawn /
// AddFlagToObjectiveBot / OnFlagPickedUp / CaptureFlag / OnFlagConsumed,
// FlagCaptureVolume Touch / CanCapture) and the real CrewBot,
// IsCarryingPickupFlag and RemoveFlagFromPlayField natives. Stripped (empty)
// natives rebuilt here, patched into the TgBaseObjective_CTFBot vtable (the
// stubs are folded `ret`s shared with unrelated classes, so no detours):
//   +0x37c SetObjectiveActive   +0x3e8 SpawnObjectiveBot (spawn, then the
//   binary's own setup)   +0x3f4/0x3f8/0x3fc/0x400 SendFlag{PickedUp,Captured,
//   Spawned,Consumed}Message (announcer + scoring).
// Capture volumes (setupFlagCapVolume is stripped too) get their team from
// map_object_config s_n_task_force, else "the team whose robot is NOT parked
// next to this platform". Channel: "acq".
namespace Acquisition {
	// TgMissionObjective::RegisterSelf hook: patch the CTFBot vtable once.
	void OnRegisterObjective(ATgMissionObjective* Obj);
	// TgGame_Ticket::LoadGameConfig hook (after it): DualCTF caps-to-win.
	void OnLoadGameConfig(ATgGame* Game);
	// TgGame_Ticket::TickTicketsCalculation hook: true = DualCTF handled it
	// (scoring is per capture, not per second).
	bool OnTickTickets(ATgGame_Ticket* Game);
	// TgGame::LockoutObjectives hook: true = skip (DualCTF robots are two
	// independent objectives; the Mission priority lockout would lock + despawn
	// the other team's robot the moment one is picked up).
	bool SkipLockout(ATgGame* Game);
	// ~1 Hz from GameEngine::Tick: capture-volume setup + robot respawn net.
	void Tick();
	// Dev teleport shortcut (IpcClient DEV_TELEPORT): if `Player` crews a
	// robot, move the robot instead (the crew rides inside it). True = moved.
	bool DevTeleportCrewed(ATgPawn* Player, const FVector& Loc);
}
