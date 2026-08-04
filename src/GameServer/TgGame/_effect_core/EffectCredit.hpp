#pragma once

#include "src/pch.hpp"
#include "src/GameServer/TgGame/_effect_core/OriginResolver.hpp"
#include "src/GameServer/TgGame/_effect_core/DeviceLookup.hpp"
#include "src/GameServer/Stats/DeviceStats.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// EffectCredit — shared resolution of "who gets scoreboard credit for this
// effect group, and which player-equipped device produced it". Factored out
// of CleanseTracking when power-restore and buff-window recording needed the
// identical logic. Mirrors TrackStats conventions: pawn directly (pet →
// r_Owner), deployable → its deploying pawn / spawning device.
namespace EffectCredit {

inline ATgPawn* ResolveCreditPawn(AActor* inst) {
	if (!inst) return nullptr;
	if (ObjectClassCache::ClassNameContains(inst, "TgPawn")) {
		ATgPawn* p = static_cast<ATgPawn*>(inst);
		return p->r_Owner ? p->r_Owner : p;
	}
	if (ObjectClassCache::ClassNameContains(inst, "TgDeploy")) {
		APawn* deployer = inst->Instigator;
		if (deployer && ObjectClassCache::ClassNameContains(deployer, "TgPawn")) {
			return static_cast<ATgPawn*>(deployer);
		}
	}
	return nullptr;
}

// Effect group → asm device id. Canonical instance-id path first
// (OriginResolver + the pawn's equipped-device scan), then the deployable
// r_Owner fallback TrackStats uses when the fire mode is deployable-owned.
// Returns 0 when unresolvable.
inline int ResolveDeviceId(UTgEffectGroup* g, ATgPawn* creditPawn) {
	if (!g) return 0;
	OriginResolver::DeviceOrigin origin = OriginResolver::Resolve(g);
	if (origin.instId != 0 && creditPawn) {
		ATgDevice* d = DeviceLookup::DeviceByInstanceId(creditPawn, origin.instId);
		if (d && d->r_nDeviceId > 0) return d->r_nDeviceId;
	}
	AActor* inst = g->m_Instigator;
	if (inst && ObjectClassCache::ClassNameContains(inst, "TgDeploy")) {
		ATgDeployable* dep = static_cast<ATgDeployable*>(inst);
		if (dep->r_Owner && dep->r_Owner->r_nDeviceId > 0) {
			return dep->r_Owner->r_nDeviceId;
		}
		return DeviceStats::DeviceIdFromDeployableId(dep->r_nDeployableId);
	}
	return 0;
}

}  // namespace EffectCredit
