#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UC declaration: `native function TgEffect CloneEffect();`
// Stripped stub at 0x10a6f2a0 (returns 0).
//
// Construct a fresh UTgEffect of the same concrete class as `this` and copy
// all per-instance fields. Used by UTgEffectGroup::CloneEffectGroup (also a
// stripped native, also reimplemented in this codebase) to produce
// independent effect instances when a group is cloned: each application
// target needs its own m_fCurrent / m_fBase / etc., or apply/remove math
// across simultaneous applications collides on shared template state.
class TgEffect__CloneEffect : public HookBase<
	UTgEffect*(__fastcall*)(UTgEffect*, void*),
	0x10a6f2a0,
	TgEffect__CloneEffect> {
public:
	static UTgEffect* __fastcall Call(UTgEffect* effect, void* edx);
	static inline UTgEffect* __fastcall CallOriginal(UTgEffect* effect, void* edx) {
		return m_original(effect, edx);
	};
};
