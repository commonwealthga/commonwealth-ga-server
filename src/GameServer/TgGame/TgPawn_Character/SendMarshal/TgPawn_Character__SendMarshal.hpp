#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgPawn_Character marshal-send native @ 0x109c1000.
//
// Stripped to a bare `return;` in this build. This is the function the engine
// combat-message emitter (TgPawn_Character::SendCombatMessage @ 0x109dd9c0)
// dispatches its accumulated marshal through — `pawn->vtable[0x266](marshal)`
// at the end of the emitter body. With the stub no-op, the engine's
// accumulator + dedup + marshal-build runs to completion but the final wire
// send is a black hole, which is why damage numbers never naturally reach
// the client.
//
// Hook implements the missing wire send: route through the owning client's
// UClientConnection::SendMarshal, then FlushNet inline. This is the per-event
// + FlushNet pattern the user empirically verified delivers to the client
// without the channel-queue stalls we hit when we tried to batch marshals
// outside the engine path.
//
// Calling convention is __thiscall:
//   ECX        = ATgPawn_Character* (this = recipient pawn)
//   [ESP+0x4]  = CMarshal* (the marshal to send)
class TgPawn_Character__SendMarshal : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, void*),
	0x109c1000,
	TgPawn_Character__SendMarshal> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, void* Marshal);
};
