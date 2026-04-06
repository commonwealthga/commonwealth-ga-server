#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgEffectManager__RemoveEffectGroup::Call(ATgEffectManager* Manager, void* edx, UTgEffectGroup* EffectGroup) {
	LogCallBegin();
	bool result = CallOriginal(Manager, edx, EffectGroup);
	LogCallEnd();
	return result;
}
