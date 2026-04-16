#include "src/GameServer/TgGame/TgEffectManager/ApplyDamage/TgEffectManager__ApplyDamage.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

// Logging-only: called from the naked trampoline with individual params.
// Combat-message emission lives in the ProcessEvent intercept for
// `Function TgGame.TgEffectManager.ApplyDamage` so we don't double-fire
// when UC dispatches into this native body.
void __cdecl TgEffectManager__ApplyDamage::LogCall(
		ATgEffectManager* pThis, int nDamage, AActor* aInstigator,
		int nAttackType, int nDamageType, int nEffectGroupCategory) {
	char thisName[256], instigatorName[256];
	strncpy(thisName, pThis ? pThis->GetFullName() : "null", sizeof(thisName) - 1);
	thisName[sizeof(thisName) - 1] = '\0';
	strncpy(instigatorName, aInstigator ? aInstigator->GetFullName() : "null", sizeof(instigatorName) - 1);
	instigatorName[sizeof(instigatorName) - 1] = '\0';

	Logger::Log(GetLogChannel(),
		"TgEffectManager::ApplyDamage: pThis=%s nDamage=%d instigator=%s attackType=%d damageType=%d effectGroupCategory=%d\n",
		thisName, nDamage, instigatorName, nAttackType, nDamageType, nEffectGroupCategory);
}

// Naked trampoline: logs, then JMPs to the original function reusing the
// caller's stack frame.  No C++ prolog/epilog, so no struct-copy issues.
//
// On entry (from the Detours trampoline, __thiscall convention):
//   ECX          = pThis (ATgEffectManager*)
//   [ESP+0x04]   = nDamage
//   [ESP+0x08]   = aInstigator
//   [ESP+0x0C]   = nAttackType
//   [ESP+0x10]   = nDamageType
//   [ESP+0x14]   = FImpactInfo  (96 bytes by value)
//   [ESP+0x74]   = nEffectGroupCategory
__attribute__((naked))
void __fastcall TgEffectManager__ApplyDamage::Call(ATgEffectManager* /*pThis*/, void* /*edx*/) {
	__asm__ __volatile__(
		// --- preserve every register the original will need ---
		"pushal\n"

		// --- build LogCall(__cdecl) args  (right-to-left) ---
		// After pushal ESP is 32 bytes below entry ESP.
		// entry offsets are relative to (current ESP + 32 + 4_retaddr) = ESP+36.
		// But note: entry [ESP+X] means [currentESP + 32 + X].
		"pushl 0x94(%%esp)\n"       // nEffectGroupCategory  (entry +0x74, +32 → +0x94)
		"pushl 0x34(%%esp)\n"       // nDamageType           (entry +0x10, +32+4 → +0x34)  (4 extra for 1 push above)
		"pushl 0x38(%%esp)\n"       // nAttackType           (entry +0x0C, +32+8 → +0x38)
		"pushl 0x3C(%%esp)\n"       // aInstigator           (entry +0x08, +32+12 → +0x3C)
		"pushl 0x40(%%esp)\n"       // nDamage               (entry +0x04, +32+16 → +0x40)
		"pushl %%ecx\n"             // pThis (still in ECX)

		"call %P0\n"               // LogCall (cdecl — we clean up)
		"addl $24, %%esp\n"         // pop 6 args × 4

		// --- restore regs and jump to original ---
		"popal\n"
		"jmp *%1\n"
		:
		: "i"(LogCall), "m"(m_original)
	);
}
