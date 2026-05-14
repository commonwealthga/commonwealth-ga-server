#pragma once

#include "src/pch.hpp"
#include <cstdint>

// Emits a CHAT_MESSAGE (TCP opcode 0x0073) marshal that hits the SAME client
// dispatcher path as TgPlayerController::AddKillAlert — the center-screen
// alert widget with the satisfying sound effect.
//
// Why CHAT_MESSAGE for kill alerts: the client's AddKillAlert (0x10964810)
// builds a CMarshal with DISPLAY_AS_ALERT/ALERT_PRIORITY/ALERT_TYPE/
// FLOAT_VALUE/NAME/PLAYER_NAME fields and routes it through vtable[0x734]
// (FUN_109649f0) → CMarshal__send_marshal → FUN_10901f00 (chat dispatcher
// at CGameClient+0xf0). FUN_109027e0, the network entry for opcode 0x73,
// also ends in FUN_10901f00 (param_1) when MSG_ID is present in the
// incoming marshal. So a server-sent CHAT_MESSAGE marshal with the same
// fields lights up the alert by piggybacking on the chat dispatcher — no
// new opcode, no client patches.
//
// Cursed but: this is what the original devs almost certainly used.
//
// References:
//   - reference_alert_dispatcher_734.md
//   - reference_client_tcp_handlers.md
class SendKillAlert {
public:
	// VictimName: who was killed (@@name@@ slot in the localization template).
	// PetName:    if non-null/non-empty, this was a pet kill — the killer's
	//             pet name fills @@player_name@@ and the message switches to
	//             "Your <pet> defeated <victim>". When null/empty, the
	//             direct-kill template "You defeated <victim>" is used.
	// Sends directly to `Connection`; caller has already resolved
	// "killer is a real player with a network connection".
	static void Call(UNetConnection* Connection, wchar_t* VictimName, wchar_t* PetName);
};
