#include "src/GameServer/TgGame/TgEffectManager/GetSkillBasedEffectGroup/TgEffectManager__GetSkillBasedEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UTgEffectManager::GetSkillBasedEffectGroup — original is a _notimplemented
// stub @ 0x10a6ef50. UC's `TgDeviceFire.ApplyEffectType` iterates this for any
// effect group type != 261 to pick up skill-based buffs/effects, and the stub
// returning without setting the `out int nIndex` produces undefined UC loop
// behaviour (can infinite-loop, can return garbage pointers that downstream
// `SubmitEffect` feeds into `TgPawn.ProcessEffect`). Matches our other search
// pattern: iterate `s_SkillBasedEffectGroups` (TArray @ +0x474), match on
// `m_nType`, write resultIdx or -1 via the out-param.
UTgEffectGroup* __fastcall TgEffectManager__GetSkillBasedEffectGroup::Call(ATgEffectManager* Manager, void* edx, int nType, int* nIndex) {
	if (!Manager || !nIndex) {
		if (nIndex) *nIndex = -1;
		return nullptr;
	}

	TArray<UTgEffectGroup*>& list = Manager->s_SkillBasedEffectGroups;
	int startIdx = (*nIndex >= 0) ? *nIndex : 0;
	for (int i = startIdx; i < list.Count; i++) {
		UTgEffectGroup* g = list.Data[i];
		if (g && g->m_nType == nType) {
			*nIndex = i;
			return g;
		}
	}
	*nIndex = -1;
	return nullptr;
}
