#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/ApplyPlayerModsToDeployable.hpp"
#include "src/GameServer/TgGame/_effect_core/ScaleTargetProperties.hpp"

// Thin wrapper preserving the spawn-site call signature used by
// TgDeviceFire__Deploy.cpp and TgProj_Deployable__SpawnDeployable.cpp.
// All scaling logic lives in `_effect_core/ScaleTargetProperties` — see
// `.planning/2026-05-26-producer-pure-canonical-design.md`.
void ApplyPlayerModsToDeployable::Apply(ATgPawn* pawn, ATgDeployable* deployable,
                                        int sourceDeviceInstId) {
    ScaleTargetProperties::Apply(pawn, deployable, sourceDeviceInstId);
}
