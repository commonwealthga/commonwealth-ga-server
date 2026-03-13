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
