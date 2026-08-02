#pragma once

class ATgGame;
class ATgMissionObjective;

// Custom Point-Rotation mode on the repurposed CTR_* maps. At match init, seeds
// one KOTH objective (asm objective id 345 "01_ROTATION_Large") at each surveyed
// point from CtrObjectives::ForMap, all at nPriority=1 so the stock
// TgGame_PointRotation rotation picks them at random, and neutralizes the maps'
// baked CTR objectives so only ours rotate.
//
// No-op unless the instance is running as TgGame_PointRotation on a CTR map that
// has surveyed points — so the original CTR mode (DualCTF/Mission) is untouched.
// Channel: "ctrrot".
namespace CtrPointRotation {
	void Init(ATgGame* Game);

	// Called from TgMissionObjective::RegisterSelf for EVERY objective. On a CTR
	// rotation map, the stock CTR objectives (CTFBot, etc.) run their PostBeginPlay
	// — and thus AddToList + RegisterSelf — AFTER Init has already seeded our points
	// and cleared the list, so they slip back into GRI->m_MissionObjectives and get
	// picked + auto-captured by the rotation. This removes any such late straggler
	// from the rotation list and disables it. No-op until Init has finished seeding
	// (so our own points, which register during Init, are never excluded) and on
	// any non-CTR map.
	void ExcludeLateStockObjective(ATgMissionObjective* Obj);

	// True once Init has seeded rotation points on the current map — i.e. the
	// custom CTR rotation variant is running. Gates the manual announcer alerts
	// (rotation banner / activation countdown) that retail Rot_* maps already
	// get from their own kismet.
	bool IsActive();

	// Retail Rot_* maps bake a TgBeaconFactory pair (teleport entrance pad +
	// pickupable exit beacon) inside each team's spawn room; the CTR maps have
	// none. Spawns the four factories relative to each team's TgTeamPlayerStart
	// and kicks the retail init sequence (PopulateBeaconFactoryList, entrance
	// SpawnObject, CheckBeacon → exit beacon). Must run AFTER the taskforces'
	// eventPostInit (beacon managers exist) — called from InitGameRepInfo.
	// No-op unless Init seeded this map.
	void InitBeacons(ATgGame* Game);
}
