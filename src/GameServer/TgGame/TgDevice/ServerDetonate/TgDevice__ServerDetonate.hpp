#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::ServerDetonate — native at 0x10a20580 (INTACT). On right-click the
// client RPCs ServerDoRemoteDetonation -> ServerDetonate, which re-fires every
// deployable this device placed (StartFire -> DeviceFiring -> damage) and every
// in-flight projectile.
//
// The damage works, but detonated grenades never disappear: in retail StartFire
// also runs the one-shot s_bIsActivated latch that sets LifeSpan = s_fPersistTime
// so the grenade despawns shortly after exploding. Our SpawnDeployable presets
// s_bIsActivated=1 at spawn (to stop time-despawn deployables from raw-LifeSpan
// vanishing the instant they land), which consumes that latch -> StartFire never
// writes LifeSpan, and DeviceFired's DestroyIt is gated off too (s_fPersistTime
// != 0). Net result: AfterShock Launcher grenades are immortal and can be
// re-detonated for repeated damage.
//
// This hook reinstates retail's LifeSpan = s_fPersistTime, scoped to the
// grenades this detonation just fired. See the .cpp for the full chain.
class TgDevice__ServerDetonate : public HookBase<
    bool(__fastcall*)(ATgDevice*, void*, int),
    0x10a20580,
    TgDevice__ServerDetonate> {
public:
    static bool __fastcall Call(ATgDevice* Device, void* edx, int nFireMode);
    static inline bool __fastcall CallOriginal(ATgDevice* Device, void* edx, int nFireMode) {
        return m_original(Device, edx, nFireMode);
    }
};
