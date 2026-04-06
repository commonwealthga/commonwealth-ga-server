#include "src/GameServer/TgGame/TgEffectManager/GetSkillBasedEffectGroup/TgEffectManager__GetSkillBasedEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

UTgEffectGroup* __fastcall TgEffectManager__GetSkillBasedEffectGroup::Call(ATgEffectManager* Manager, void* edx, int nType, int* nIndex) {
	LogCallBegin();
	UTgEffectGroup* result = CallOriginal(Manager, edx, nType, nIndex);
	LogCallEnd();
	return result;
}
