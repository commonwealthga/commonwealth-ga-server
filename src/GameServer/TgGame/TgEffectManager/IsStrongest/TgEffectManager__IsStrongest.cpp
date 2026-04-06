#include "src/GameServer/TgGame/TgEffectManager/IsStrongest/TgEffectManager__IsStrongest.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgEffectManager__IsStrongest::Call(ATgEffectManager* Manager, void* edx, UTgEffectGroup* eg, unsigned long bConsiderLifetime) {
	LogCallBegin();
	bool result = CallOriginal(Manager, edx, eg, bConsiderLifetime);
	LogCallEnd();
	return result;
}
