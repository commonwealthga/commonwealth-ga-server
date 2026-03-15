#include "src/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Set before every dispatch into CGameClient__MarshalReceived so the handler
// can identify which UNetConnection the incoming marshal arrived on.
void* GCurrentMarshalConnection = nullptr;

void MarshalChannel__MarshalReceived::Call(UMarshalChannel* MarshalChannel, void* edx, void* InBunch) {

	LogCallBegin();

	// UNetConnection is at MarshalChannel+0x3c (confirmed from TgMarshalChannel__MarshalReceived decompile).
	GCurrentMarshalConnection = *(void**)((char*)MarshalChannel + 0x3c);

	CallOriginal(MarshalChannel, edx, InBunch);

	GCurrentMarshalConnection = nullptr;

	LogCallEnd();
}

