#include "src/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native (TgTeamBeaconManager__SpawnNewBeaconForTeam_notimplemented
// @ 0x109ee6b0). Called from CheckBeacon (vtable slot 0x374) when:
//   - r_Beacon is null/invalid
//   - No PRI carries the beacon (LoadInventoryBeacon found no holder)
//   - bAttemptRespawn arg is true
//
// Factory selection by *priority*: TgGame.s_nCurrentPriority starts at 1 and
// climbs as objectives unlock (TgGame.PostBeginPlay sets it to 1 + calls
// UnlockObjective(1); subsequent UnlockObjective calls advance it). Each
// TgBeaconFactory has m_nPriority (defaults to -1 = "any priority"):
//
//   1. Best:     factory with m_nPriority == game->s_nCurrentPriority
//                AND m_bBeaconExit AND !m_bIsFallback
//   2. Untiered: factory with m_nPriority == -1 (treated as always-eligible)
//   3. Fallback: factory with m_bIsFallback OR game->s_FallbackBeaconExit
//
// Picks the appropriate exit factory and delegates to its SpawnObject() —
// our SpawnObject hook handles the actual spawn + RegisterBeacon for exit
// factories when the manager already exists.

void __fastcall TgTeamBeaconManager__SpawnNewBeaconForTeam::Call(
	ATgTeamBeaconManager* mgr, void* /*edx*/)
{
	if (!mgr) return nullptr;

	ATgGame* game = (ATgGame*)Globals::Get().GGameInfo;
	const int currentPriority = game ? game->s_nCurrentPriority : -1;
	const int count = mgr->s_BeaconFactoryList.Num();

	Logger::Log("beacon",
		"TgTeamBeaconManager::SpawnNewBeaconForTeam mgr=0x%p tf=0x%p factories=%d "
		"currentPriority=%d r_Beacon=0x%p r_BeaconHolder=0x%p gameFallback=0x%p\n",
		mgr, mgr->r_TaskForce, count, currentPriority,
		mgr->r_Beacon, mgr->r_BeaconHolder,
		game ? game->s_FallbackBeaconExit : nullptr);

	// Defensive: CheckBeacon only routes to vtable[0x374] (us) when r_Beacon
	// is null AND no carrier was found. If r_Beacon is already set, somebody
	// invoked us out-of-band (or via a stale dispatch) — spawning a second
	// beacon would duplicate. Log and skip.
	if (mgr->r_Beacon) {
		Logger::Log("beacon",
			"  mgr already has r_Beacon=0x%p — skipping respawn\n", mgr->r_Beacon);
		return nullptr;
	}

	ATgBeaconFactory* tieredMatch = nullptr;
	ATgBeaconFactory* untiered    = nullptr;
	ATgBeaconFactory* fallback    = nullptr;

	for (int i = 0; i < count; ++i) {
		ATgBeaconFactory* f = mgr->s_BeaconFactoryList.Data[i];
		if (!f || !f->m_bBeaconExit) continue;

		if (f->m_bIsFallback) {
			if (!fallback) fallback = f;
			continue;
		}
		if (f->m_nPriority < 0) {
			if (!untiered) untiered = f;
			continue;
		}
		if (f->m_nPriority == currentPriority) {
			tieredMatch = f;  // first one wins
			break;
		}
	}

	ATgBeaconFactory* pick = tieredMatch
		? tieredMatch
		: (untiered ? untiered : fallback);

	// Last-ditch: TgGame's designated fallback factory (set by map / native
	// initialisation). Only honoured if it actually belongs to this manager's
	// task force; otherwise we'd spawn the defender beacon at the attacker's
	// fallback or vice versa.
	if (!pick && game && game->s_FallbackBeaconExit) {
		ATgBeaconFactory* gf = game->s_FallbackBeaconExit;
		if (mgr->r_TaskForce && gf->s_nTaskForce == mgr->r_TaskForce->r_nTaskForce) {
			pick = gf;
		}
	}

	if (!pick) {
		Logger::Log("beacon",
			"  no eligible exit factory for tf %d at priority %d — cannot respawn\n",
			mgr->r_TaskForce ? (int)mgr->r_TaskForce->r_nTaskForce : -1,
			currentPriority);
		return nullptr;
	}

	Logger::Log("beacon",
		"  picked factory 0x%p mapObjId=%d m_nPriority=%d m_bIsFallback=%d "
		"(matched %s) — delegating to SpawnObject\n",
		pick, pick->m_nMapObjectId, pick->m_nPriority, (int)pick->m_bIsFallback,
		(pick == tieredMatch) ? "priority" : (pick == untiered ? "untiered" : "fallback"));

	// Fresh team beacon = full health. s_fHealthPercent is only ever written
	// by pickup (TgDeploy_Beacon.uc:144 SetHealthPercent(GetSaveHealthPercent()))
	// and consumed by RegisterBeacon(bDeployed=false) on the next deploy;
	// nothing resets it, so a respawn here must not inherit a stale percent.
	mgr->s_fHealthPercent = 1.0f;

	TgBeaconFactory__SpawnObject::Call(pick, nullptr);
	return nullptr;
}
