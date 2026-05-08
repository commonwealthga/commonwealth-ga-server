#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Direct native call to TgPawn::GetBuffIndex (vtable[0x560], FUN_109cd890).
// We CANNOT use the SDK wrapper `Pawn->GetBuffIndex(...)` — its stub at
// TgGame_functions.cpp:44095 forgets to copy the input `*nIndex` into the
// parms struct, so the native sees uninitialized stack garbage as the loop
// start index. With find-or-add, garbage past array-end skips existing
// entries → appends fresh on every call → m_EffectBuffInfo grows unbounded
// and the BuffFormula sums every duplicate, applying the same modifier N
// times. Direct native call below keeps `nIndex=0` end-to-end.
//
// __thiscall: ECX = this; rest on stack. __fastcall with a junk edx maps
// cleanly to that.
typedef void (__fastcall* GetBuffIndexNativeFn)(
	ATgPawn* /*this*/, void* /*edx*/,
	int /*propId*/, int /*catCode*/, int /*skillId*/, int /*devInst*/,
	int /*bAddIfNotExists*/, int* /*startIdx_inout*/);
static const GetBuffIndexNativeFn GetBuffIndexNative = (GetBuffIndexNativeFn)0x109cd890;

// Reimplementation of TgPawn::ApplyBuff (UC native; binary stub @ 0x109bf7b0
// is empty so the buff registry never gets populated by the original code).
//
// Adds (or updates / removes) a single FBuffInfo entry in `pawn->m_EffectBuffInfo`
// (TArray @ +0x100C). The original GetPropertyValueById path on TgDeviceFire
// reads this registry — via vtable[0x568]=FUN_109cd6f0 (aggregator) and
// vtable[0x56C]=FUN_109cd4a0 (formula) — when computing fire-mode params, so
// any rolled mod we register here flows through to gameplay automatically.
//
// Slot mapping verified by decompiling FUN_109cd4a0 (the formula function):
//   v1 = base * (1 + ΣfItemPercent /100) + ΣfItemMod                                 // ITEM
//   v2 = v1   * (1 + ΣfSkillPercent/100) + ΣfSkillMod                                 // SKILL
//   v3 = v2   * (1 + (ΣfSelfPercent + ΣfPercent)/100) + ΣfSelfMod + ΣfMod             // Self+Generic
//
// Source-type → slot routing follows the field naming directly:
//   BUFF_SOURCE_TYPE_SKILL      (0) → fSkill*
//   BUFF_SOURCE_TYPE_ITEM       (1) → fItem*    ← rolled mods land here
//   BUFF_SOURCE_TYPE_EFFICIENCY (2) → f*         (generic catch-all)
//   BUFF_SOURCE_TYPE_SELF       (3) → fSelf*
//   BUFF_SOURCE_TYPE_OTHER      (4) → f*         (generic catch-all)
//
// Calc method 68 (PERC_INCREASE) / 69 (PERC_DECREASE) → percent slot.
// Calc method 67 (ADD)           / 70 (SUBTRACT)      → absolute slot.
// 69 and 70 are sign-flipped variants of 68 and 67.
//
// bRemove flips the sign once more (subtract the previously-added delta).

void __fastcall TgPawn__ApplyBuff::Call(ATgPawn* Pawn, void* /*edx*/, FBuffHeader BuffFilter,
                                        int nCalcMethodCode, float fAmount, unsigned long bRemove,
                                        unsigned char buffSourceType) {
	if (!Pawn) return;

	// Find existing matching entry, or append a fresh one when not removing.
	// Direct native call (see typedef above) — bypasses the broken SDK wrapper
	// that left the start index as uninitialized stack garbage.
	int index = 0;
	GetBuffIndexNative(Pawn, /*edx=*/nullptr,
	                   BuffFilter.nPropId, BuffFilter.nReqCategoryCode,
	                   BuffFilter.nReqSkillId, BuffFilter.nReqDeviceInstId,
	                   /*bAddIfNotExists=*/(bRemove ? 0 : 1),
	                   &index);
	if (index < 0) return;
	if (Pawn->m_EffectBuffInfo.Data == nullptr) return;
	if (index >= Pawn->m_EffectBuffInfo.Num()) return;

	FBuffInfo& entry = Pawn->m_EffectBuffInfo.Data[index];

	const bool isPercent = (nCalcMethodCode == 68 || nCalcMethodCode == 69);
	float delta = fAmount;
	if (nCalcMethodCode == 69 || nCalcMethodCode == 70) delta = -delta;
	if (bRemove) delta = -delta;

	float* slot = nullptr;
	switch (buffSourceType) {
		case 0: slot = isPercent ? &entry.fSkillPercentModifier : &entry.fSkillModifier; break;
		case 1: slot = isPercent ? &entry.fItemPercentModifier  : &entry.fItemModifier;  break;
		case 3: slot = isPercent ? &entry.fSelfPercentModifier  : &entry.fSelfModifier;  break;
		case 2:
		case 4:
		default: slot = isPercent ? &entry.fPercentModifier : &entry.fModifier;          break;
	}

	if (slot) {
		const float prev = *slot;
		*slot += delta;
		Logger::Log("inventory",
			"ApplyBuff: pawn=0x%p prop=%d cm=%d amount=%.2f src=%u remove=%lu  slot %.2f -> %.2f  (entry idx=%d)\n",
			Pawn, BuffFilter.nPropId, nCalcMethodCode, fAmount, (unsigned)buffSourceType, bRemove,
			prev, *slot, index);
	}

	// Mirror the entry to the owning client. UC's TgPawn::ApplyBuff (stripped
	// here, that's why we reimplement it) ends with a `ClientSendBuff(entry)`
	// RPC — the client uses it to populate its own m_EffectBuffInfo and then
	// runs OnDeviceBuffChange, which is what recomputes things like the
	// crosshair tightness from buffed accuracy. Without this, server-side
	// math is correct but the client never sees the buff (visible symptom:
	// no crosshair change inside a buff station radius).
	//
	// SDK wrapper for ClientSendBuff is safe to use — single 0x30 FBuffInfo
	// parm with no bitfields, so the wrapper's memcpy delivers it intact.
	Pawn->eventClientSendBuff(entry);
}
