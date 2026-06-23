#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDeployableFactory__SpawnObject : public HookBase<
	void(__fastcall*)(ATgDeployableFactory*, void*),
	0x10a8c250,
	TgDeployableFactory__SpawnObject> {
public:
	static void __fastcall Call(ATgDeployableFactory* Obj, void* edx);
	static inline void __fastcall CallOriginal(ATgDeployableFactory* Obj, void* edx) {
		m_original(Obj, edx);
	};
};
