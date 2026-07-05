#include "src/GameServer/Cosmetics/JetpackReload.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

constexpr int kJetpackSlot = 5;  // SVID_JETPACK engine equip point (ES5 Travel)

}  // namespace

void JetpackReload::MarkJetpackFormDirty(ATgPawn* Pawn) {
	if (Pawn == nullptr) return;
	ATgDevice* jet = Pawn->m_EquippedDevices[kJetpackSlot];
	if (jet == nullptr) return;  // no jetpack equipped

	// Epoch in bits 24..26, cycling 1..7 so the value is always nonzero,
	// always != the real invId, and always != the previous fake value.
	static int g_epoch = 0;
	const int fakeId = jet->r_nDeviceInstanceId | (((g_epoch % 7) + 1) << 24);
	++g_epoch;

	Pawn->r_EquipDeviceInfo[kJetpackSlot].nDeviceId         = jet->r_nDeviceId;
	Pawn->r_EquipDeviceInfo[kJetpackSlot].nDeviceInstanceId = fakeId;
	Pawn->r_EquipDeviceInfo[kJetpackSlot].nQualityValueId   = jet->r_nQualityValueId;
	Pawn->bNetDirty       = 1;
	Pawn->bForceNetUpdate = 1;

	Logger::Log("loadout",
		"[JetpackReload] slot 5 instance id %d -> %d (dev=%d) — owner form rebuild on next DeviceFormChanged\n",
		jet->r_nDeviceInstanceId, fakeId, jet->r_nDeviceId);
}
