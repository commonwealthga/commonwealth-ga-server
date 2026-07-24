#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBeaconFactory__SpawnObject : public HookBase<
	void(__fastcall*)(ATgBeaconFactory*, void*),
	0x10A8C260,
	TgBeaconFactory__SpawnObject> {
public:
	// Spawn one inert entrance-pad marker (the map-icon actor) at baseLoc.
	// Entrance recipe: deployableId 48, replication wiring. zLift=true =
	// retail beacon-factory pad (plain cylinder Z lift above baseLoc);
	// zLift=false = nav-point marker: downward ground trace from baseLoc,
	// pad rested on the hit surface — lifted along the hit normal and
	// rotation surface-aligned like a player-placed deployable (nav points
	// float at pawn-center height and sit on slopes).
	// factory may be null (SuperAgent ambush-point markers); tf may be null
	// (marker stays taskforce-neutral, like a pre-taskforce retail entrance).
	// Deliberately NO EffectManager / health init / beacon-manager wiring —
	// indestructible, inert, replicated.
	static ATgDeploy_BeaconEntrance* SpawnEntranceMarker(AActor* spawner,
		const FVector& baseLoc, const FRotator& rot,
		ATgBeaconFactory* factory, ATgRepInfo_TaskForce* tf, bool zLift);

	static void __fastcall Call(ATgBeaconFactory* BeaconFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBeaconFactory* BeaconFactory, void* edx) {
		m_original(BeaconFactory, edx);
	}
};

