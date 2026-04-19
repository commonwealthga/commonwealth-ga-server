#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgDeployable__InitializeDefaultProps : public HookBase<
	void(__fastcall*)(ATgDeployable*, void*),
	0x10a194d0,
	TgDeployable__InitializeDefaultProps> {
public:
	static void __fastcall Call(ATgDeployable* Deployable, void* edx);
	static inline void __fastcall CallOriginal(ATgDeployable* Deployable, void* edx) {
		m_original(Deployable, edx);
	};

	// Construct a UTgProperty, push it onto Deployable->s_Properties, then
	// call native SetProperty so the ApplyProperty vtable slot pushes the
	// value into any engine-owned mirror field.
	static UTgProperty* InitializeProperty(
		ATgDeployable* Deployable, int nPropertyId,
		float fBase, float fRaw, float fMinimum, float fMaximum);
};
