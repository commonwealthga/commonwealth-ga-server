#include "src/GameServer/TgGame/TgDevice/CalcFireSocketIndexMax/TgDevice__CalcFireSocketIndexMax.hpp"
#include "src/GameServer/Utils/FireSockets/FireSockets.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

void __fastcall TgDevice__CalcFireSocketIndexMax::Call(ATgDevice* Device, void* edx) {
    CallOriginal(Device, edx);

    if (!Device) return;

    APawn* Instigator = Device->Instigator;
    if (!Instigator) return;
    if (!ObjectClassCache::ClassNameContains(Instigator, "TgPawn")) return;
    ATgPawn* Pawn = reinterpret_cast<ATgPawn*>(Instigator);

    // Catch-all SOI population for pawns that didn't go through SpawnBotById
    // (no-op when already populated / no asset bound). UpdateIndex calls this
    // every shot, so any firing pawn gets its SocketOffsetInfo by shot 2 at
    // the latest; SpawnBotById covers bots from shot 1.
    FireSockets::EnsurePopulated(Pawn);

    if (Device->m_nSocketMax > 1) return;  // stock (or prior) lookup succeeded

    void* model = FireSockets::GetMeshModel(Pawn->r_nBodyMeshAsmId);
    if (!model) return;

    int max = FireSockets::GetShotOriginSocketMax(
        model, (int)Device->CurrentFireMode, (int)Device->r_eEquippedAt);
    if (max > 1) {
        Device->m_nSocketMax = max;
    }
}
