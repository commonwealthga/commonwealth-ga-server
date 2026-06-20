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
}
