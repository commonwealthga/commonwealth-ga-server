#include "src/GameServer/TgGame/TgDevice/ServerDetonate/TgDevice__ServerDetonate.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// On remote detonation, reinstate retail's StartFire activation behaviour
// (LifeSpan = s_fPersistTime) for the grenades this detonation just fired.
//
// The native already re-fired them (StartFire -> DeviceFiring -> damage). We
// only need to schedule the despawn the spawn-time s_bIsActivated=1 latch
// suppressed. Matching mirrors the native's own scan: a deployable belongs to
// this detonation when r_Owner == this device AND r_nOwnerFireMode == nFireMode.
//
// Scope/safety:
//   - s_fPersistTime <= 0 → skip. Those despawn through DeviceFired's DestroyIt
//     already (the m_bConsumedOnFire && s_fPersistTime==0 gate), so they're not
//     affected by the latch.
//   - LifeSpan > 0 → skip. Already counting down from an earlier detonate; not
//     re-extended, so a rapid re-detonate can't keep the grenade alive past its
//     first deadline (matches retail's one-shot latch).
//   - Raw LifeSpan (not DestroyIt) is the faithful path: the explosion FX
//     already played during the fire, so the spent grenade should simply vanish
//     — same as retail's StartFire LifeSpan write, which Destroy()s silently.
bool __fastcall TgDevice__ServerDetonate::Call(ATgDevice* Device, void* edx, int nFireMode) {
    const bool ret = CallOriginal(Device, edx, nFireMode);
    if (!Device) return ret;

    AWorldInfo* worldInfo = World__GetWorldInfo::CallOriginal(
        (UWorld*)Globals::Get().GWorld, nullptr, 0);
    if (!worldInfo || !worldInfo->GRI) return ret;
    ATgRepInfo_Game* gri = (ATgRepInfo_Game*)worldInfo->GRI;

    for (int i = 0; i < gri->m_Deployables.Count; ++i) {
        ATgDeployable* dep = gri->m_Deployables.Data[i];
        if (!dep) continue;
        if (dep->r_Owner != Device) continue;
        if (dep->r_nOwnerFireMode != nFireMode) continue;
        if (dep->m_bInDestroyedState) continue;
        if (dep->s_fPersistTime <= 0.0f) continue;
        if (dep->LifeSpan > 0.0f) continue;

        dep->LifeSpan = dep->s_fPersistTime;
        Logger::Log("detonate",
            "[ServerDetonate] dep=0x%p id=%d fireMode=%d  LifeSpan <- persist=%.2fs (explode + despawn)\n",
            dep, dep->r_nDeployableId, nFireMode, dep->s_fPersistTime);
    }
    return ret;
}
