#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class UObject__ProcessEvent : public HookBase<
	void(__fastcall*)(UObject*, void*, UFunction*, void*, void*),
	0x11347C20,
	UObject__ProcessEvent> {
public:
	static bool bDisableGarbageCollection;
	static void __fastcall Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result);
	static inline void __fastcall CallOriginal(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {
		m_original(Object, edx, Function, Params, Result);
	};
};

