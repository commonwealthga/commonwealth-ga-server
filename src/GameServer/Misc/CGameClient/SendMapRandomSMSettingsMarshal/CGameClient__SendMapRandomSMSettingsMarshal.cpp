#include "src/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/GameServer/Misc/CMarshal/SetWcharT/CMarshal__SetWcharT.hpp"
#include "src/GameServer/Misc/HeapAllocator/Allocate/HeapAllocator__Allocate.hpp"
#include "src/GameServer/Misc/CMarshalRowSet/InsertRow/CMarshalRowSet__InsertRow.hpp"
#include "src/GameServer/Misc/CMarshalRow/Initialize/CMarshalRow__Initialize.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/Utils/Logger/Logger.hpp"


// Ship the marshal that TgRandomSMManager::ManageRandomSMActors @ 0x109b8f40
// built into gamerep->s_pRandomSMSettings over the UDP marshal channel as
// opcode MAP_RANDOMSM_SETTINGS (0x0157). The client's CGameClient dispatcher
// stashes the marshal and invokes LocalPC->RefreshRandomSMSettings, which
// hides every TgRandomSMActor in the world and re-enables only the ones
// whose full FName (suffix included) is present in the DATA_SET.
//
// The native populates the DATA_SET with names of EXPLICITLY-SELECTED actors
// (m_nType > 0) only. Companion actors (m_nType == 0) get ToggleDisplay(1)
// server-side but their names never make it into the marshal, so we append
// them here — otherwise they'd remain hidden on the client and present as
// invisible collision geometry.
void CGameClient__SendMapRandomSMSettingsMarshal::Call(UNetConnection* Connection, void* MarshalPtr) {
	if (Connection == nullptr || MarshalPtr == nullptr) {
		Logger::Log("randomsm", "SendMapRandomSMSettingsMarshal: skip — Connection=%p MarshalPtr=%p\n",
			Connection, MarshalPtr);
		return;
	}

	uint32_t dataSet = 0;
	CMarshal__GetArray::CallOriginal(MarshalPtr, nullptr, GA_T::DATA_SET, &dataSet);

	int companionsAdded = 0;
	if (dataSet != 0) {
		ActorCache::CacheMapActors();
		for (ATgRandomSMActor* actor : ActorCache::RandomSMActors) {
			if (actor == nullptr) continue;

			// m_nType is at +0x1FC per SDK. Companion bucket = type 0.
			if (actor->m_nType != 0) continue;

			// AActor::bCollideActors lives at +0xB0 bit 0x08000000 (CPF_Net).
			// ToggleDisplay(1) → SetCollision(true,...) sets this bit.
			// ManageRandomSMActors only flips it on companions whose Owner
			// field (+0x9C) points at a selected actor, so this filter is a
			// clean "include client-relevant companions only" test.
			const bool bEnabled = (*(uint32_t*)((char*)actor + 0xB0)) & 0x08000000;
			if (!bEnabled) continue;

			// FName-with-suffix (e.g. "TgRandomSMActor_23"). The SDK
			// GetName() wrapper drops the suffix; the client matches via
			// _wcsicmp on the suffixed form. UObject::GetName @ 0x1090d1d0
			// fills an FString — caller owns the buffer.
			int nameBuffer[3] = {0, 0, 0};
			((void(__fastcall*)(void*, void*, int*))0x1090d1d0)(actor, nullptr, nameBuffer);
			wchar_t* wname = (wchar_t*)nameBuffer[0];

			if (wname) {
				void* rowBuf = HeapAllocator__Allocate::CallOriginal(0x24);
				if (rowBuf) {
					void* row = CMarshalRow__Initialize::CallOriginal(rowBuf);
					CMarshal__SetWcharT::CallOriginal(row, nullptr, GA_T::NAME, wname);
					CMarshalRowSet__InsertRow::CallOriginal((void*)dataSet, nullptr, row);
					companionsAdded++;
					Logger::Log("randomsm", "  + companion %ls\n", wname);
				}
			}

			// appFree the FString buffer that UObject::GetName allocated.
			((void(__fastcall*)(int*))0x112c18c0)(nameBuffer);
		}
	}

	Logger::Log("randomsm",
		"SendMapRandomSMSettingsMarshal: Connection=%p MarshalPtr=%p dataSet=0x%08X companionsAdded=%d cachedActors=%d\n",
		Connection, MarshalPtr, dataSet, companionsAdded, (int)ActorCache::RandomSMActors.size());

	ClientConnection__SendMarshal::Call(Connection, nullptr, MarshalPtr);
	NetConnection__FlushNet::CallOriginal(Connection);
}

