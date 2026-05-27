#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/ApplyPlayerModsToPet.hpp"
#include "src/GameServer/TgGame/_effect_core/ScaleTargetProperties.hpp"

// Thin wrapper preserving the spawn-site call signature used by
// TgDeviceFire__SpawnPet.cpp. All scaling logic lives in
// `_effect_core/ScaleTargetProperties` — see
// `.planning/2026-05-26-producer-pure-canonical-design.md`.
void ApplyPlayerModsToPet::Apply(ATgPawn* deployingPawn, ATgPawn* petPawn,
                                 int sourceDeviceInstId) {
    ScaleTargetProperties::Apply(deployingPawn, petPawn, sourceDeviceInstId);
}
