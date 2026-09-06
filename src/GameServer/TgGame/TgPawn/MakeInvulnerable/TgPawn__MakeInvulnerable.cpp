#include "src/GameServer/TgGame/TgPawn/MakeInvulnerable/TgPawn__MakeInvulnerable.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgPawn::MakeInvulnerable — stripped native @ 0x109bfcc0 (decompiles to
// `return;`). UC calls it with 3.0s from:
//   * TgGame.uc:565            — respawn
//   * TgPawn.uc:4837           — post-death reset (GotoState('Auto'))
//   * TgPawn.uc:9200           — GotoBeacon (beacon teleport)
//   * TgGame_Arena.uc:404      — arena respawn
//
// There is no UC-side revert timer and no `bInvulnerable` field, so the whole
// lifecycle lived inside the native. Reconstructed from the two mechanisms the
// game actually uses to make a pawn untouchable:
//
//   1. Protection properties. TgPawn.TakeDamage has NO invulnerability check —
//      mitigation happens upstream in TgEffectDamage.ProtectionModifier ->
//      TgEffectGroup.CalcProtection, which computes
//      `m_fRaw / attacking device's attack_rating`. 1441 of 1446 device modes
//      ship attack_rating=100, so +100 on a protection prop is a 100% cut.
//      Effect group 8698 is the shipped "Invulnerable" payload (device 2801,
//      YA_Volume_Invulnerable): +100 on all ten protection props (+200 on
//      Biological / Ignite), class TgGame.TgEffect (direct m_fRaw modifier,
//      which is what CalcProtection reads), 3.0s lifetime.
//      Damage then lands as 0 and the client renders combat-message template
//      0xA489 = "*Immune*" instead of a number.
//
//   2. r_nInvulnerableCount — NOT used here (see the disabled retag below).
//      TgEffectManager.uc:267 increments it for any applied group whose
//      category is 862 ("Invulnerable"); IsInvulnerable() (0x10a6f1a0) is
//      `count > 0`. It gates TgCollisionProxy instant-kills
//      (m_bIgnoreInvulnerablePlayers), but ALSO fails every device StartFire
//      at TgDevice.uc:1067 — so routing spawn protection through 862 leaves
//      the player unable to use any device. No shipped group does that:
//      8698 ships category 302.
//
// So: build 8698, stretch its lifetime to fLength, and push it through the
// canonical ProcessEffect path — mechanism 1 only. BuildEffectGroup
// constructs a fresh object per call, so nothing here can leak into the
// device-2801 volume path that also builds 8698.

namespace {
	// Shipped "Invulnerable" payload — see header comment.
	constexpr int kInvulnEffectGroupId = 8698;
	// effect_group_type_value_id 264 = Hit, matching the DB row for 8698.
	constexpr int kInvulnEffectGroupType = 264;
	// asm valid_value 862 = "Invulnerable" — the category TgEffectManager
	// counts into r_nInvulnerableCount.
	constexpr int kCategoryInvulnerable = 862;
}

void __fastcall TgPawn__MakeInvulnerable::Call(ATgPawn* Pawn, void* edx, float fLength) {
	LogCallBegin();

	if (!Pawn || fLength <= 0.0f) { LogCallEnd(); return; }
	// Server-authoritative only — the effect manager's applied list is
	// replicated, not simulated.
	if (Pawn->Role != 3 /*ROLE_Authority*/) {
		Logger::Log("invuln", "[MakeInvulnerable] pawn=%p skipped: Role=%d (want 3)\n",
			(void*)Pawn, (int)Pawn->Role);
		LogCallEnd();
		return;
	}

	ATgEffectManager* mgr = Pawn->r_EffectManager;
	if (!mgr) {
		Logger::Log("invuln", "[MakeInvulnerable] pawn=%p has no r_EffectManager\n", (void*)Pawn);
		LogCallEnd();
		return;
	}

	UTgEffectGroup* group = BuildEffectGroup(kInvulnEffectGroupId, kInvulnEffectGroupType);
	if (!group) {
		Logger::Log("invuln", "[MakeInvulnerable] BuildEffectGroup(%d) failed\n",
			kInvulnEffectGroupId);
		LogCallEnd();
		return;
	}

	// DB row ships 3.0s; the caller owns the duration.
	group->m_fLifeTime     = fLength;
	// Retag to 862 DISABLED 2026-09-06. Bucketing into r_nInvulnerableCount
	// makes IsInvulnerable() true, and TgDevice.uc:1067 fails EVERY device
	// StartFire while that holds — spawn protection left players unable to use
	// any device. The original server never blocked devices in spawn, and no
	// shipped effect group routes spawn protection through 862 (8698 ships
	// category 302). The protection-property half below is the whole
	// mechanism; 862 additionally gated instant-kill hazards
	// (TgCollisionProxy), which does not apply on a spawn pad.
	// group->m_nCategoryCode = kCategoryInvulnerable;

	FImpactInfo impact{};
	mgr->eventProcessEffect(group, /*bRemove=*/0, /*Buffers=*/nullptr,
		/*aInstigator=*/(AActor*)Pawn, impact);

	Logger::Log("invuln",
		"[MakeInvulnerable] pawn=%p egId=%d lifetime=%.2fs invulnCount=%d appliedGroups=%d\n",
		(void*)Pawn, kInvulnEffectGroupId, fLength,
		mgr->r_nInvulnerableCount,
		mgr->s_AppliedEffectGroups.Data ? mgr->s_AppliedEffectGroups.Count : -1);

	LogCallEnd();
}
