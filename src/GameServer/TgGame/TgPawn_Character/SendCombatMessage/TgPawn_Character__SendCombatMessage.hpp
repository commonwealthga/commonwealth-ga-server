#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgPawn_Character::SendCombatMessage @ 0x109dd9c0 — the per-recipient combat
// message dispatch. Called by the native TgGame::SendCombatMessage body
// (FUN_10a74a60) once for each pawn that should see the event (attacker
// pawn, victim pawn, etc.) via vtable slot 0x984.
//
// Calling convention is __thiscall:
//   ECX         = ATgPawn_Character* (this = recipient pawn)
//   [ESP+0x04]  = 24-byte record struct passed by value:
//                 { int nMsgId, int nIdAttacker, int nIdAssist,
//                   int nIdVictim, int nValueHealth, int nValueShield }
//   RET 0x18 (cleans the 24 bytes on return).
//
// The original body only flushes when an internal accumulator fills
// (21 records / 101 indices), so relying on it means most events never reach
// the client. Our detour skips the original and emits a single-record packet
// immediately via SendCombatMessage::CallRaw.
class TgPawn_Character__SendCombatMessage : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, int, int, int, int, int, int),
	0x109dd9c0,
	TgPawn_Character__SendCombatMessage> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx,
	                            int nMsgId, int nIdAttacker, int nIdAssist,
	                            int nIdVictim, int nValueHealth, int nValueShield);
};
