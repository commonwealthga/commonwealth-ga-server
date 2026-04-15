#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmItem__LoadItemMarshal::bPopulateDatabaseItems = false;
std::map<uint32_t, int> CAmItem__LoadItemMarshal::m_ItemPointers;

void __fastcall CAmItem__LoadItemMarshal::Call(void* CAmItemRow, void* edx, void* Marshal) {

	CallOriginal(CAmItemRow, edx, Marshal);

	m_ItemPointers[CMarshal__GetByte::m_values[GA_T::GA_T::ITEM_ID]] = (int)(CAmItemRow);

	// DB population moved to AsmDataCapture::WalkItems (driven by
	// CMarshal__GetArray::Call for DATA_SET_ITEMS).  bPopulateDatabaseItems is kept
	// for source-compat but no longer does anything.
	//
	// if (bPopulateDatabaseItems) {
	//     // ... original inline INSERT kept in git history ...
	// }
}

