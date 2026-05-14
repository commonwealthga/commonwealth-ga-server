#include "src/GameServer/TgGame/TgEffect/CloneEffect/TgEffect__CloneEffect.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// ConstructObject<UTgEffect> — IsChildOf-validates the requested UClass
// against UTgEffect::StaticClass(), so passing TgEffectDamage / TgEffectHeal /
// etc. works without us having to find their individual ConstructObject
// addresses.
typedef void*(__cdecl* ConstructEffectFn)(void*, int, int, int, unsigned, unsigned, int, int, int*);
static const ConstructEffectFn ConstructEffectNative = (ConstructEffectFn)0x10a73f30;

// Per-class instance size (from SDK headers). UE3's UStruct::PropertiesSize
// would give this at runtime but the SDK hides it behind UnknownData blobs;
// the table is small (6 known concrete subclasses) and stable.
//
// Memcpy past PropertiesSize would write into the next allocation, so
// undersize is the bug to avoid; oversize would just copy padding.
static int GetEffectInstanceSize(UTgEffect* effect) {
	if (!effect || !effect->Class) return 0;
	const char* name = effect->Class->GetFullName();
	if (!name) return 0;
	if (strstr(name, "TgEffectDamage")     != nullptr) return 0x80;
	if (strstr(name, "TgEffectHeal")       != nullptr) return 0x74;
	// TgEffectSensor, TgEffectVisibility, TgEffect, TgEffectBuff — all 0x70.
	return 0x70;
}

// Reimplementation of TgEffect::CloneEffect — original native @ 0x10a6f2a0 is
// the empty stripped stub `_notimplemented`. UC declares the prototype but no
// UC bytecode call site exists; the call site that mattered historically was
// inside the original UTgEffectGroup::CloneEffectGroup (also stripped), which
// walked m_Effects calling CloneEffect on each. Our reimplemented
// CloneEffectGroup used to inline this work; we hoist it here so any binary-
// internal caller that does manage to land at 0x10a6f2a0 gets correct behavior
// rather than nullptr — same defense-in-depth as filling out other stripped
// natives we don't strictly need.
//
// Field copy strategy: ConstructObject<UTgEffect> populates the UObject
// header (Class, Name, Outer, etc.) for the fresh instance, so we skip 0x00..
// 0x3C. UTgEffect's persistent state starts at 0x3C (m_EffectGroup) and runs
// through `m_KnockbackZMultiplier` at 0x6C (size 0x4) = total 0x70 for the
// base class. Subclasses extend.
//
// Caller (CloneEffectGroup) re-wires `effClone->m_EffectGroup` to point at
// the new clone group and force-asserts `m_bRemovable=true` after we return
// — those are GROUP-relative concerns and don't belong in the per-effect
// clone primitive.
UTgEffect* __fastcall TgEffect__CloneEffect::Call(UTgEffect* effect, void* /*edx*/) {
	if (!effect || !effect->Class) return nullptr;

	int instSize = GetEffectInstanceSize(effect);
	if (instSize <= 0x3C) {
		Logger::Log("effects",
			"[CLONE-EFFECT] unknown size for class=%s — returning nullptr\n",
			effect->Class->GetFullName());
		return nullptr;
	}

	UTgEffect* clone = (UTgEffect*)ConstructEffectNative(
		effect->Class, -1, 0, 0, 0, 0, 0, 0, nullptr);
	if (!clone) {
		Logger::Log("effects", "[CLONE-EFFECT] ConstructObject failed for class=%s\n",
			effect->Class->GetFullName());
		return nullptr;
	}

	// Range 0x3C..instSize covers everything beyond the UObject header for
	// the concrete subclass. Inline storage (no TArrays / FStrings in
	// UTgEffect), so shallow memcpy is correct.
	std::memcpy((uint8_t*)clone + 0x3C, (uint8_t*)effect + 0x3C, instSize - 0x3C);

	return clone;
}
