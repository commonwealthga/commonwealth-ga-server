#include "src/GameServer/TgGame/TgEffectManager/SetEffectRep/TgEffectManager__SetEffectRep.hpp"
#include "src/GameServer/TgGame/TgDeviceVolume/setupDevice/TgDeviceVolume__setupDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgEffectManager::SetEffectRep — the native @ 0x10a6ffe0 is INTACT (it does
// Branch A queue-push vs Branch B managed-list-slot refresh correctly; see
// decompiled/.../ATgEffectManager/INDEX.md and canonical §0/§3). The rebuild is
// a THIN HOOK: call the original (+ diagnostic logging) and nothing else.
//
// REMOVED vs the prior implementation (all of it was re-deriving / fighting the
// intact native): managed-slot suppression for type 264/505, the queue-push
// gates, the beacon-5716 / stealth-621 special cases, and the
// EffectDisplacementMarker. The native already routes impact FX through
// r_EventQueue and manages the 16 HUD slots; with UC pre-adding the clone to
// s_AppliedEffectGroups the native takes Branch B and same-frame realloc is
// handled by its own refcount + form reconciliation (canonical Q4).
//
// REMOVED (2026-05-22): the buff side-channel. class_res_id-157 effects are now
// constructed as REAL TgEffectBuff instances (TgEffectBuff is force-loaded at
// startup — see EngineLoad::PreloadClass). Buff registration therefore happens
// canonically inside the intact UC `UTgEffectGroup.ApplyEffects` →
// `TgEffectBuff.ApplyEffect` → `ApplyBuff` → `ATgPawn::ApplyBuff`, independent of
// SetEffectRep. The old `ShouldRouteToBuff`/`ApplyBuffEffectFromHook` bolt-on
// (and its remove-side twin in UObject__ProcessEvent) are deleted.

int __fastcall TgEffectManager__SetEffectRep::Call(ATgEffectManager* Manager, void* edx,
		int nEffectGroupID, float fTime, unsigned long bIsBuff, int nHealthChange) {
	// VR heal pad: UC computes bIsBuff via TgEffectGroup.IsBuff(), whose default
	// branch is `m_Instigator != none && !m_Instigator.IsEnemy(m_Target)` — the
	// pad's instigator is the team-0 TgDeviceVolume, which fails IsEnemy, so the
	// heal reps as a debuff. The client packs this bit into the queue entry
	// (bit 24) and sets TgEffectForm.c_bIsDebuff from it, inverting the FX tint
	// (yellow self / green enemy instead of the Medical Station's green/yellow).
	// A heal is a buff regardless of instigator — force the bit for the pad's
	// heal group ids.
	if (!bIsBuff && DomeVrHealPad::IsPadHealEffectGroup(nEffectGroupID)) {
		bIsBuff = 1;
		Logger::Log("healpad", "[SetEffectRep] forced bIsBuff for egId=%d\n", nEffectGroupID);
	}
	// The intact native does all replication (queue / managed-list / forms).
	const int ret = CallOriginal(Manager, edx, nEffectGroupID, fTime, bIsBuff, nHealthChange);

	if (Logger::IsChannelEnabled("effects")) {
		Logger::Log("effects",
			"[SET-REP] egId=%d fTime=%.2f bIsBuff=%lu hpΔ=%d -> %s(slot=%d)\n",
			nEffectGroupID, fTime, bIsBuff, nHealthChange,
			(ret == -1) ? "A:queue" : "B:managed", ret);
	}

	return ret;
}
