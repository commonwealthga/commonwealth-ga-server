#include "src/GameServer/TgGame/TgDevice/CalcFireSocketIndexMax/TgDevice__CalcFireSocketIndexMax.hpp"

// One-time-resolved FName index for "ShotOrigin". UE3 FNames are interned
// in a global table; the same string always resolves to the same Index.
// We cache to avoid re-running FName lookup every UpdateIndex call.
static int s_ShotOriginNameIdx = -1;

void __fastcall TgDevice__CalcFireSocketIndexMax::Call(ATgDevice* Device, void* edx) {
    CallOriginal(Device, edx);

    // Cheap exits first.
    if (!Device) return;
    if (Device->m_nSocketMax > 1) return;  // original lookup succeeded — leave it alone

    APawn* Instigator = Device->Instigator;
    if (!Instigator) return;

    // m_TgSocketOffsetInfo lives at TgPawn+0x1268. The SDK declares it on
    // ATgPawn_Character — same offset on any TgPawn that owns one.
    UTgSocketOffsetInfo* info = *(UTgSocketOffsetInfo**)((char*)Instigator + 0x1268);
    if (!info) return;

    // Lazy-init the "ShotOrigin" name index. Skipping Number comparison: any
    // numbered duplicates ("ShotOrigin_0", "ShotOrigin_1" with the same base
    // name) would also be valid socket entries we want to count.
    if (s_ShotOriginNameIdx < 0) {
        s_ShotOriginNameIdx = FName("ShotOrigin").Index;
    }

    int count = 0;
    for (int i = 0; i < info->m_SocketOffsets.Count; i++) {
        FSocketOffsetInfo& entry = info->m_SocketOffsets.Data[i];
        if (entry.SocketName.Index == s_ShotOriginNameIdx) {
            count++;
        }
    }

    if (count > 1) {
        Device->m_nSocketMax = count;
    }
}
