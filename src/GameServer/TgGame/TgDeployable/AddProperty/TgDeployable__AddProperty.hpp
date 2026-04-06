#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDeployable__AddProperty : public HookBase<
	void(__fastcall*)(ATgDeployable*, void*, int, float, float, float, float),
	0x10a194e0,
	TgDeployable__AddProperty> {
public:
	static void __fastcall Call(ATgDeployable* Deployable, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax);
	static inline void __fastcall CallOriginal(ATgDeployable* Deployable, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax) {
		m_original(Deployable, edx, nPropId, fBase, fRaw, FMin, FMax);
	};
};
