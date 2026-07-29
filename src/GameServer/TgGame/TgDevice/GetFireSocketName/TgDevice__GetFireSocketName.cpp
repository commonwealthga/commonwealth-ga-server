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

	const int max = FireSockets::GetShotOriginSocketMax(
		model, (int)Device->CurrentFireMode, (int)Device->r_eEquippedAt);

	// Retail ProjectileFire (TgDevice.uc:1884-1890) runs UpdateIndex() BEFORE
	// FlashFireNoSim, so the client muzzle flash renders at the MuzzleFlash-
	// group socket of the NEXT cycle index. Spawn the projectile from that
	// exact socket. Index-shifting ShotOrigin instead is wrong: the two
	// groups' display_order was authored independently per weapon (Shrike
	// slot 200 ShotOrigin is order-reversed vs. its MuzzleFlash rows — the
	// retail data pre-compensates the engine quirk). Instant/arcing fire
	// flash pre-increment — untouched.
	FName resolved(0);
	UTgDeviceFire* fireMode =
		((int)Device->CurrentFireMode < Device->m_FireMode.Count)
			? Device->m_FireMode.Data[(int)Device->CurrentFireMode] : nullptr;
	if (fireMode && fireMode->m_nFireType == 1 /*EWFT_Projectile*/ && max > 1) {
		const int flashIndex = (Device->m_nSocketIndex % max) + 1;
		FName flashSocket = FireSockets::GetMuzzleFlashSocketName(
			model, (int)Device->CurrentFireMode, flashIndex,
			(int)Device->r_eEquippedAt);
		// Only usable if the trace native can resolve it (synth SOI carries
		// every mesh socket; a miss would collapse the trace to body center).
		if (flashSocket.Index != 0 && Pawn->m_TgSocketOffsetInfo) {
			UTgSocketOffsetInfo* soi = Pawn->m_TgSocketOffsetInfo;
			for (int i = 0; i < soi->m_SocketOffsets.Count; i++) {
				const FName& n = soi->m_SocketOffsets.Data[i].SocketName;
				// unknownData00 = the FName instance-Number half ("_01" split)
				if (n.Index == flashSocket.Index &&
				    *(const int*)n.unknownData00 ==
				    *(const int*)flashSocket.unknownData00) {
					resolved = flashSocket;
					break;
				}
			}
		}
	}

	// Same query the stock native runs — only the asmId source differs
	// (pawn body asm instead of c_DeviceForm->c_Mesh's asm).
	if (resolved.Index == 0) {
		resolved = FireSockets::GetShotOriginSocketName(
			model, (int)Device->CurrentFireMode, Device->m_nSocketIndex,
			(int)Device->r_eEquippedAt);
	}
	if (resolved.Index != 0) {
		*outName = resolved;
	}

	// Retail side effect: refresh the cycle size + mark it calculated.
	Device->m_nSocketMax = max;
	Device->m_bSocketMaxCalculated = 1;

	return ret;
}
