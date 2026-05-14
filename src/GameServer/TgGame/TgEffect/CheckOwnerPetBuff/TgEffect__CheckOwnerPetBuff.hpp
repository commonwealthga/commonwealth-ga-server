#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UC declaration:
//   `native function CheckOwnerPetBuff(int nPropertyId, float fCurrAmount, out float fNewAmount);`
// Stripped stub at 0x10a6f290 (returns nothing — leaves fNewAmount unchanged).
//
// Called from TgEffectDamage.uc:130 with nPropertyId=350 (Pet Damage Modifier).
// When a pet bot deals damage, the OWNING pawn's prop-350 buff (from skills like
// "Mechanical Whisperer" or rolled mods on the spawning device) should scale the
// damage. The pet's own m_EffectBuffInfo doesn't carry the owner's skills, so the
// scaling has to query the owner pawn directly.
//
// With the stub being a no-op, pet damage never gets owner-side prop-350 scaling.
class TgEffect__CheckOwnerPetBuff : public HookBase<
	void(__fastcall*)(UTgEffect*, void*, int, float, float*),
	0x10a6f290,
	TgEffect__CheckOwnerPetBuff> {
public:
	static void __fastcall Call(UTgEffect* effect, void* edx, int nPropertyId, float fCurrAmount, float* fNewAmount);
	static inline void __fastcall CallOriginal(UTgEffect* effect, void* edx, int nPropertyId, float fCurrAmount, float* fNewAmount) {
		m_original(effect, edx, nPropertyId, fCurrAmount, fNewAmount);
	};
};
