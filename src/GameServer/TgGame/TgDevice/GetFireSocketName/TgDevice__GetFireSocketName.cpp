#include "src/GameServer/TgGame/TgDevice/GetFireSocketName/TgDevice__GetFireSocketName.hpp"
#include "src/GameServer/Utils/FireSockets/FireSockets.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

FName* __fastcall TgDevice__GetFireSocketName::Call(ATgDevice* Device, void* edx, FName* outName) {
	FName* ret = CallOriginal(Device, edx, outName);

	if (!Device || !outName) return ret;
	if (outName->Index != 0) return ret;  // stock form/mesh path resolved it

	APawn* Instigator = Device->Instigator;
	if (!Instigator) return ret;
	if (!ObjectClassCache::ClassNameContains(Instigator, "TgPawn")) return ret;
	ATgPawn* Pawn = reinterpret_cast<ATgPawn*>(Instigator);

	// TgDevice.uc:1813 hazard: ProjectileFire takes the mesh-socket branch
	// whenever `Mesh != none && GetFireSocketName() != 'None'`. A mesh
	// component with no SkeletalMesh would yield a zero SocketLocation —
	// don't hand the UC a name it can't resolve.
	if (Pawn->Mesh && !Pawn->Mesh->SkeletalMesh) return ret;

	void* model = FireSockets::GetMeshModel(Pawn->r_nBodyMeshAsmId);
	if (!model) return ret;

	// Same query the stock native runs — only the asmId source differs
	// (pawn body asm instead of c_DeviceForm->c_Mesh's asm).
	FName resolved = FireSockets::GetShotOriginSocketName(
		model, (int)Device->CurrentFireMode, Device->m_nSocketIndex,
		(int)Device->r_eEquippedAt);
	if (resolved.Index != 0) {
		*outName = resolved;
	}

	// Retail side effect: refresh the cycle size + mark it calculated.
	Device->m_nSocketMax = FireSockets::GetShotOriginSocketMax(
		model, (int)Device->CurrentFireMode, (int)Device->r_eEquippedAt);
	Device->m_bSocketMaxCalculated = 1;

	return ret;
}
