#include "src/GameServer/TgGame/TgDevice/GetFireSocketName/TgDevice__GetFireSocketName.hpp"
#include "src/Database/SocketCycle/SocketCycle.hpp"

FName* __fastcall TgDevice__GetFireSocketName::Call(ATgDevice* Device, void* edx, FName* outName) {
    FName* ret = CallOriginal(Device, edx, outName);

    if (!Device || !outName) return ret;
    if (outName->Index != 0) return ret;

    APawn* Instigator = Device->Instigator;
    if (!Instigator) return ret;

    ATgPawn* TgPawn = reinterpret_cast<ATgPawn*>(Instigator);
    int equipPoint  = (int)Device->r_eEquippedAt;

    int count = SocketCycle::LookupOriginSocketCount(TgPawn, equipPoint);
    if (count == 0) return ret;

    int idx = Device->m_nSocketIndex - 1;
    if (idx < 0) idx = 0;
    if (idx >= count) idx = count - 1;

    FName resolved = SocketCycle::GetOriginSocketName(TgPawn, equipPoint, idx);
    if (resolved.Index != 0) {
        *outName = resolved;
    }
    return ret;
}
