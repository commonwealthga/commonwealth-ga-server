#include "src/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/GameServer/Misc/CMarshal/SetWcharT/CMarshal__SetWcharT.hpp"
#include "src/GameServer/Misc/HeapAllocator/Allocate/HeapAllocator__Allocate.hpp"
#include "src/GameServer/Misc/CMarshalRowSet/InsertRow/CMarshalRowSet__InsertRow.hpp"
#include "src/GameServer/Misc/CMarshalRow/Initialize/CMarshalRow__Initialize.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/Logger/Logger.hpp"


void CGameClient__SendMapRandomSMSettingsMarshal::Call(UNetConnection* Connection, void* MarshalPtr) {

	// ManageRandomSMActors adds names of explicitly-selected actors to the DATA_SET,
	// but type-0 "companion" actors also get ToggleDisplay(1) server-side without
	// their names being included. Add them here so the client shows them too.
	uint32_t dataSet = 0;
	CMarshal__GetArray::CallOriginal(MarshalPtr, nullptr, GA_T::DATA_SET, &dataSet);
	if (dataSet != 0) {
		UClass* smActorClass = ATgRandomSMActor::StaticClass();
		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			if (!obj || !obj->IsA(smActorClass)) continue;

			ATgRandomSMActor* actor = reinterpret_cast<ATgRandomSMActor*>(obj);

			// Only process type-0 companion actors
			if (actor->m_nType != 0) continue;

			// bCollideActors is at offset 0xB0, bit 0x08000000.
			// ToggleDisplay(true) calls SetCollision(true,...), ToggleDisplay(false) calls SetCollision(false,...).
			// Enabled companions will have bCollideActors set.
			bool bEnabled = (*(uint32_t*)((char*)actor + 0xB0)) & 0x08000000;
			if (!bEnabled) continue;

			// Allocate and initialize a new row
			void* rowBuf = HeapAllocator__Allocate::CallOriginal(0x24);
			if (!rowBuf) continue;
			void* row = CMarshalRow__Initialize::CallOriginal(rowBuf);

			// Get the full actor name including FName instance number suffix (e.g. "TgRandomSMActor_23").
			// actor->GetName() only returns the base FName string without the number suffix,
			// which would fail the _wcsicmp comparison in RefreshRandomSMSettings.
			int nameBuffer[3] = {0, 0, 0};
			((void(__fastcall*)(void*, void*, int*))0x1090d1d0)(actor, nullptr, nameBuffer);
			wchar_t* wname = (wchar_t*)nameBuffer[0];

			if (wname) {
				CMarshal__SetWcharT::CallOriginal(row, nullptr, GA_T::NAME, wname);
				CMarshalRowSet__InsertRow::CallOriginal((void*)dataSet, nullptr, row);
				Logger::Log("debug", "Added companion SM actor to marshal: %ls\n", wname);
			}

			// Free the FString buffer (appFree @ 0x112c18c0 resets FString struct and frees Data)
			((void(__fastcall*)(int*))0x112c18c0)(nameBuffer);
		}
	}

	// Send all chained marshals
	void* CurrentMarshal = MarshalPtr;
	while (CurrentMarshal) {
		ClientConnection__SendMarshal::Call(Connection, nullptr, CurrentMarshal);
		CurrentMarshal = *(void**)((char*)CurrentMarshal + 0x2f4);
	}
	NetConnection__FlushNet::CallOriginal(Connection);

}

