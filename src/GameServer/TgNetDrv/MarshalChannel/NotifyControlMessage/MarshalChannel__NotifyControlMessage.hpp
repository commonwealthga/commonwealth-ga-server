#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class MarshalChannel__NotifyControlMessage : public HookBase<
	void(__fastcall*)(void*, void*, UNetConnection*, void*),
	0x109FA6E0,
	MarshalChannel__NotifyControlMessage> {
public:
	static void __fastcall Call(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch);
	static inline void __fastcall CallOriginal(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch) {
		return m_original(MarshalChannel, edx, Connection, InBunch);
	};

	static inline std::string GuidToHex(const FGuid& g) {
		char buf[33];
		snprintf(buf, sizeof(buf),
		   "%08X%08X%08X%08X",
		   g.A, g.B, g.C, g.D);
		return std::string(buf);
	};

	static void HandlePlayerConnected(UNetConnection* Connection, ATgPlayerController* Controller);
};

