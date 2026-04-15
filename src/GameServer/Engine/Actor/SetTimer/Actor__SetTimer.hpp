#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// native final function SetTimer(float InRate, optional bool bLoop,
//                                optional Name inTimerFunc, optional Object inObj);
// Address: 0x10c7ffc0  |  RET 0x14 = 20 bytes stack cleanup
// Stack params: float(4) + uint(4) + FName(4+4) + UObject*(4) = 20
// FName is passed as two ints (Index, Number) to avoid GCC hidden-pointer ABI mismatch.
class Actor__SetTimer : public HookBase<
	void(__fastcall*)(AActor*, void*, float, unsigned long, int, int, UObject*),
	0x10c7ffc0,
	Actor__SetTimer> {
public:
	static void __fastcall Call(AActor* Actor, void* edx, float InRate, unsigned long bLoop, int NameIndex, int NameNumber, UObject* inObj) {
		CallOriginal(Actor, edx, InRate, bLoop, NameIndex, NameNumber, inObj);
	}
	static inline void __fastcall CallOriginal(AActor* Actor, void* edx, float InRate, unsigned long bLoop, int NameIndex, int NameNumber, UObject* inObj) {
		LogCallBegin();
		m_original(Actor, edx, InRate, bLoop, NameIndex, NameNumber, inObj);
		LogCallEnd();
	}

	// Convenience wrapper that accepts FName
	static inline void SetTimer(AActor* Actor, float InRate, bool bLoop, FName TimerFunc, UObject* inObj = nullptr) {
		int idx, num;
		memcpy(&idx, &TimerFunc, 4);
		memcpy(&num, (char*)&TimerFunc + 4, 4);
		CallOriginal(Actor, nullptr, InRate, bLoop ? 1 : 0, idx, num, inObj);
	}
};
