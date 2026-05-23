#pragma once

#include "src/pch.hpp"
#include <cstdint>

// Emits a COMBAT_MESSAGE (TCP opcode 0x9F) to a recipient pawn.
//
// Builds a single-record marshal and dispatches it through the recipient
// pawn's vtable slot 0x266 — the same slot the engine's own combat-message
// emitter (TgPawn_Character::SendCombatMessage @ 0x109dd9c0) uses at the
// end of its accumulator flush. That slot is a stripped `return;` stub in
// this build; we hook it via TgPawn_Character__SendMarshal to provide the
// missing UClientConnection::SendMarshal + FlushNet send.
//
// Used by call sites that need to emit combat messages outside the natural
// UC effect-damage chain: objective-point popups, mission progress, deploy-
// able heal/damage broadcasts, etc. UC-triggered events go through the
// engine emitter on their own and land in the same hook.
class SendCombatMessage {
public:
	enum class Type : uint16_t {
		DAMAGE      = 0x2AD3,
		KILL        = 0x2AD4,
		HEAL        = 0x2AD5,
		SHIELD_HEAL = 0x3E89,
		// Blue ^N^ floating-number popup over the recipient pawn for
		// objective-points credit. nValueHealth carries the int value into
		// both the popup template and the chat-log template.
		OBJ_POINTS  = 0x59F4,
	};

	// RecipientPawn: whose client receives the packet (number renders on their screen).
	// SourcePawn:    attacker/healer (may be null → environmental/unknown source).
	// TargetPawn:    whose pawn the number floats above (must have a replicated PRI).
	// Amount:        positive integer shown in the popup.
	// No-ops silently if recipient isn't player-controlled.
	static void Call(
		ATgPawn* RecipientPawn,
		ATgPawn* SourcePawn,
		ATgPawn* TargetPawn,
		int Amount,
		Type MessageType);

	// Direct raw-ID variant for callers that already have wire IDs (e.g.
	// deployable broadcast, replicated property mirror). Same dispatch path
	// as Call — just skips the pawn→ID resolution.
	static void CallRaw(
		ATgPawn* RecipientPawn,
		uint16_t nEventType,
		int16_t nIdAttacker,
		int16_t nIdAssist,
		int16_t nIdVictim,
		int16_t nValueHealth,
		int16_t nValueShield);
};
