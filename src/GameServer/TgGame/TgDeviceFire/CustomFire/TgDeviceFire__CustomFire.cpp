#include "src/GameServer/TgGame/TgDeviceFire/CustomFire/TgDeviceFire__CustomFire.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/Deploy/TgDeviceFire__Deploy.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/Utils/Logger/Logger.hpp"

// native function CustomFire();
// TgDevice::CustomFire() calls this on the current fire mode when Role==Authority and
// m_nFireType==2 (custom). We dispatch based on m_nAttackType (+0x88).
void __fastcall TgDeviceFire__CustomFire::Call(UTgDeviceFire* pThis, void* edx) {
	if (!pThis) return;

	int attackType = pThis->m_nAttackType;
	// Bomb diagnostic: log every CustomFire invocation with attack_type so we
	// can tell whether bombs even reach this entry point. Bombs use
	// TgDeviceFireOffhand (res 681, attack_type=85 "Instant Ranged Attack")
	// which — if it dispatches through CustomFire — would land in the default
	// branch. If it dispatches via ProjectileFire/ArcingFire instead, this log
	// never fires for bombs and we know to look elsewhere.
	ATgDevice* device = (ATgDevice*)pThis->m_Owner;
	Logger::Log("bomb",
		"[CustomFire entry] attackType=%d  fireType=%d  device=%s  fireMode.class=%s\n",
		attackType, (int)pThis->m_nFireType,
		device ? (device->GetFullName()) : "<null>",
		pThis->Class ? pThis->Class->GetFullName() : "<null>");

	switch (attackType) {
		case CONST_TGTT_ATTACK_PLACE_DEPLOYABLE:   // 209
		case CONST_TGTT_ATTACK_INSTANT_DEPLOYABLE: // 342
		case CONST_TGTT_ATTACK_SELF_DEPLOYABLE:    // 876
			TgDeviceFire__Deploy::Call(pThis, edx);
			break;

		case CONST_TGTT_ATTACK_SPAWN_PET:  // 306
			TgDeviceFire__SpawnPet::Call(pThis, edx, TRUE);
			break;

		case CONST_TGTT_ATTACK_INSTANT_BOT: // 817
			TgDeviceFire__SpawnPet::Call(pThis, edx, FALSE);
			break;

		default:
			Logger::Log(GetLogChannel(), "TgDeviceFire::CustomFire: unhandled attackType=%d\n", attackType);
			break;
	}
}
