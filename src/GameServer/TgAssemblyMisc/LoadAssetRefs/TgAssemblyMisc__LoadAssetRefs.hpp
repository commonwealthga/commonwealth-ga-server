#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Diagnostic hook on the asset-reference loader at 0x10a2bc70
// (the unnamed function in TgAssemblyMisc.cpp that iterates a form's
// m_AssetReference TMap and calls LoadObject on each entry; crashes with
// "newObj" at TgAssemblyMisc.cpp:253 when any LoadObject returns null).
//
// Native signature is __thiscall with 3 stack params:
//   undefined4 __thiscall FUN_10a2bc70(void* this, int param_1,
//                                      void* param_2, int param_3)
// where:
//   this       → asset-ref container (struct fields at +0x10 state, +0x14
//                ref-list TMap, +0x18 m_pObjectReferencer, +0x20/+0x24 name)
//   param_1    → caller discriminator hardcoded by each caller:
//                  0x10a97300 = UTgDeviceForm::Init  (FUN_10a99190)
//                  0x10a2b020 = wrapper loop          (FUN_10a2c0a0)
//                  other     = TgPlayerController::ClientLoadDevices /
//                              TgRepInfo_Player::UpdateDeviceAssetRefs
//   param_2    → context/outer object (form, device, player-rep-info)
//   param_3    → small int (0 at all observed call sites)
//   return     → *(int*)(this + 0x10), i.e. the final state
//
// Logs every invocation + return so we can tell which deployable's form is
// triggering the crash; pairs with Core__LoadObject which logs null returns.
class TgAssemblyMisc__LoadAssetRefs : public HookBase<
	int(__fastcall*)(void*, void*, int, void*, int),
	0x10a2bc70,
	TgAssemblyMisc__LoadAssetRefs> {
public:
	static int __fastcall Call(void* pThis, void* edx, int param_1, void* param_2, int param_3);
	static inline int __fastcall CallOriginal(void* pThis, void* edx, int param_1, void* param_2, int param_3) {
		return m_original(pThis, edx, param_1, param_2, param_3);
	}

	// Thread-local flag that Core__LoadObject checks to enable detailed
	// per-call logging only while we're inside this function, keeping log
	// volume manageable.
	static thread_local int s_nestLevel;
};
