#pragma once

#include "src/pch.hpp"
#include <cstdint>

// Emits a COMBAT_MESSAGE (TCP opcode 0x9F) to a single client.
// The client-side handler (CGameClient::OnCombatMessage @ 0x10919020) resolves
// actors from sign-encoded IDs (positive = PlayerId on ATgRepInfo_Player+0x204,
// negative = DeployableId on ATgRepInfo_Deployable+0x1DC), then calls
// ATgPawn::AddDamageInfo (vtable[0x8D8]) on the victim pawn, causing the
// floating number to appear above that pawn on the recipient's screen.
//
// See reference_combat_message_packet.md for the full wire format and
// dispatch table.
class SendCombatMessage {
public:
	// Event IDs cross-referenced from UC: TgEffectDamage.uc:57 / TgEffectHeal.uc:208.
	// These are the exact codes the original game dispatches for each case.
	enum class Type : uint16_t {
		DAMAGE = 0x2AD3,  // 10963 — regular damage, target still alive (TgEffectDamage)
		KILL   = 0x2AD4,  // 10964 — killing blow
		HEAL   = 0x2AD5,  // 10965 — health heal (TgEffectHeal)
		SHIELD_HEAL = 0x3E89, // 16009 — shield heal
	};

	// RecipientPawn: whose client receives the packet (number shows on their screen).
	// SourcePawn:    attacker/healer (may be null → environmental/unknown source).
	// TargetPawn:    whose pawn the number floats above (required, must have replicated PRI).
	// Amount:        positive integer shown in the number.
	// No-ops silently if recipient has no remote connection (AI/bot pawn).
	static void Call(
		ATgPawn* RecipientPawn,
		ATgPawn* SourcePawn,
		ATgPawn* TargetPawn,
		int Amount,
		Type MessageType);

	// Direct raw-ID variant. Sends to the given connection with the exact
	// record the game's own emitter would have sent. Used when hooking
	// TgPawn_Character::SendCombatMessage where the IDs are already computed
	// (sign-encoded per the wire format: positive = PlayerId, negative =
	// DeployableId). No pawn lookup — caller supplies the wire record as-is.
	static void CallRaw(
		UNetConnection* Connection,
		uint16_t nEventType,
		int16_t nIdAttacker,
		int16_t nIdAssist,
		int16_t nIdVictim,
		int16_t nValueHealth,
		int16_t nValueShield);
};
