#pragma once

#include "src/Utils/HookBase.hpp"

// StaticConstructObject wrapper specialized for UTgEffectGroup @ 0x109a90b0.
// Mirrors TgInventoryObject_Device__ConstructInventoryObject (0x10a18bc0) — same
// 9-arg shape, just with a different cached class pointer (DAT_119a3fe0 →
// UTgEffectGroup class). Forwards to UObject::StaticConstructObject after the
// IsChildOf assertion path, so passing the correct UClass is the caller's job.
//
// Header-only: we only ever need CallOriginal, so HookBase's m_original default
// (initialized to the address itself) is enough — no Detour install required.
class TgEffectGroup__ConstructEffectGroup : public HookBase<
	void*(__cdecl*)(void*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, void*),
	0x109a90b0,
	TgEffectGroup__ConstructEffectGroup> {
public:
	static void* Call(UClass* Class, int32_t Outer, int32_t param3, int32_t param4, int32_t param5, int32_t param6, int32_t param7, int32_t param8, void* param9);
	static inline void* CallOriginal(UClass* Class, int32_t Outer, int32_t param3, int32_t param4, int32_t param5, int32_t param6, int32_t param7, int32_t param8, void* param9) {
		return m_original(Class, Outer, param3, param4, param5, param6, param7, param8, param9);
	}
};
