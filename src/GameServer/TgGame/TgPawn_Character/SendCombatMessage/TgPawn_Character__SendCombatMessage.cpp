#include "src/GameServer/TgGame/TgPawn_Character/SendCombatMessage/TgPawn_Character__SendCombatMessage.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

void __fastcall TgPawn_Character__SendCombatMessage::Call(
		ATgPawn_Character* Pawn, void* /*edx*/,
		int nMsgId, int nIdAttacker, int nIdAssist,
		int nIdVictim, int nValueHealth, int nValueShield) {

	// Diagnostic: for the secondary AddDamageInfo dispatch on the client, the
	// client needs the VICTIM's PRI->r_PawnOwner (+0x690) to be non-null.
	// Log state of the recipient pawn's PRI so we can see whether server side
	// has r_PawnOwner set when we emit. If null on server → server never wired
	// it. If set on server → the issue is replication not reaching the client.
	ATgRepInfo_Player* recipientPRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	void* rPawnOwner = recipientPRI ? (void*)recipientPRI->r_PawnOwner : nullptr;
	int playerID = recipientPRI ? recipientPRI->PlayerID : -1;

	Logger::Log("combat-trace",
		"TgPawn_Character::SendCombatMessage pawn=%p msgId=%d ids=[%d,%d,%d] vals=[%d,%d] | "
		"recipientPRI=%p PlayerID=%d r_PawnOwner=%p (pawn==r_PawnOwner? %s)\n",
		Pawn, nMsgId, nIdAttacker, nIdAssist, nIdVictim, nValueHealth, nValueShield,
		recipientPRI, playerID, rPawnOwner,
		((void*)Pawn == rPawnOwner) ? "yes" : "NO");

	// Skip bots/AI — the game dispatches vtable[0x984] on every recipient pawn
	// including bots. On bots Controller is ATgAIController; APlayerController's
	// `Player` field at +0x354 is shared with AAIController's bitfield (bHunting,
	// bAdjustFromWalls, bReverseScriptedRoute at bits 0x1/0x2/0x4) — reading it
	// as UPlayer* yields tiny integers (e.g. 2), which crashes SendMarshal when
	// it dereferences `this+0x4fb4`.
	// bIsPlayer is unreliable here (this build sets it on bots), so filter by
	// class-name match — and defensively reject obviously-invalid Connection ptrs.
	if (!Pawn || !Pawn->Controller || !Pawn->Controller->Class) return;
	const char* controllerClass = Pawn->Controller->Class->GetFullName();
	if (!controllerClass || !strstr(controllerClass, "PlayerController")) return;
	APlayerController* PC = (APlayerController*)Pawn->Controller;
	UNetConnection* Connection = (UNetConnection*)PC->Player;
	if (!Connection || (uintptr_t)Connection < 0x10000) return;

	// Values in the record are ints but serialized as int16 on the wire.
	// Sign-truncate here to match the format the parser expects.
	SendCombatMessage::CallRaw(
		Connection,
		(uint16_t)nMsgId,
		(int16_t)nIdAttacker,
		(int16_t)nIdAssist,
		(int16_t)nIdVictim,
		(int16_t)nValueHealth,
		(int16_t)nValueShield);

	// Intentionally do NOT call original — its accumulator only flushes when
	// the buffer fills, so most combat events would never reach the client.
}
