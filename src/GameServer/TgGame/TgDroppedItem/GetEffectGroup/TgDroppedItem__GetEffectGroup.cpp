#include "src/GameServer/TgGame/TgDroppedItem/GetEffectGroup/TgDroppedItem__GetEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UC contract (TgDroppedItem.uc:GiveTo):
//   nIndex starts at 0; UC calls GetEffectGroup(264, nIndex) in a loop.
//   - On match: return the effect group; UC then increments nIndex and re-calls.
//     We update *nIndex to the position WHERE we found the match (so UC's
//     post-increment moves past it).
//   - On no more matches: set *nIndex = -1 and return null; UC's loop guard
//     `if(nIndex >= 0)` exits and the actor self-destructs.
UTgEffectGroup* __fastcall TgDroppedItem__GetEffectGroup::Call(
	ATgDroppedItem* Item, void* /*edx*/, int nType, int* nIndex) {

	if (!Item || !nIndex) {
		if (nIndex) *nIndex = -1;
		return nullptr;
	}

	const int start = *nIndex;
	const int count = Item->s_EffectGroupList.Count;
	if (start < 0 || start >= count || Item->s_EffectGroupList.Data == nullptr) {
		*nIndex = -1;
		return nullptr;
	}

	for (int i = start; i < count; ++i) {
		UTgEffectGroup* eg = Item->s_EffectGroupList.Data[i];
		if (eg && eg->m_nType == nType) {
			*nIndex = i;
			return eg;
		}
	}

	*nIndex = -1;
	return nullptr;
}
