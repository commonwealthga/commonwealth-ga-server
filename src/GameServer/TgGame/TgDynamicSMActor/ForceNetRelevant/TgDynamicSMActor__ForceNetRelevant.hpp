#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDynamicSMActor__ForceNetRelevant : public HookBase<
	void(__fastcall*)(ATgDynamicSMActor*, void*),
	0x109aee40,
	TgDynamicSMActor__ForceNetRelevant> {
public:
	static void __fastcall Call(ATgDynamicSMActor* Actor, void* edx);
	static inline void __fastcall CallOriginal(ATgDynamicSMActor* Actor, void* edx) {
		m_original(Actor, edx);
	};
};
