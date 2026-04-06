#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgMeshAssembly__ForceNetRelevant : public HookBase<
	void(__fastcall*)(ATgMeshAssembly*, void*),
	0x109aee70,
	TgMeshAssembly__ForceNetRelevant> {
public:
	static void __fastcall Call(ATgMeshAssembly* Actor, void* edx);
	static inline void __fastcall CallOriginal(ATgMeshAssembly* Actor, void* edx) {
		m_original(Actor, edx);
	};
};
