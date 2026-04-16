#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Misc/CMarshal/SetBinBlob/CMarshal__SetBinBlob.hpp"
#include "src/GameServer/Misc/CMarshal/Destroy/CMarshal__Destroy.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include <cstring>

// Wire format — CMarshal key 0x71 (BIN_BLOB):
//   u8 N=1, u8 M=1,
//   record: { u16 eventType, s16 attacker, s16 assist, s16 victim, s16 health, s16 shield },
//   index:  u8 0
#pragma pack(push, 1)
struct CombatMessagePayload {
	uint8_t  numRecords;
	uint8_t  numIndices;
	uint16_t nEventType;
	int16_t  nIdAttacker;
	int16_t  nIdAssist;
	int16_t  nIdVictim;
	int16_t  nValueHealth;
	int16_t  nValueShield;
	uint8_t  index;
};
#pragma pack(pop)
static_assert(sizeof(CombatMessagePayload) == 15, "CombatMessagePayload must be exactly 15 bytes");

// Core packet construction + send. Shared by both Call variants.
static void EmitPacket(UNetConnection* Connection, const CombatMessagePayload& payload) {
	uint8_t MarshalStorage[0x80] = {0};
	void* Marshal = MarshalStorage;
	CMarshalObject__Create::CallOriginal(Marshal);
	*(void**)Marshal = CMarshalObject__Create::CMarshal_vftable;
	*(uint16_t*)((uint8_t*)Marshal + 0x26) = 0x9F;
	CMarshal__SetBinBlob::CallOriginal(Marshal, nullptr, 0x71, &payload, sizeof(payload));
	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, Marshal);
	NetConnection__FlushNet::CallOriginal(Connection);
	CMarshal__Destroy::CallOriginal(Marshal);
}

void SendCombatMessage::Call(
	ATgPawn* RecipientPawn,
	ATgPawn* SourcePawn,
	ATgPawn* TargetPawn,
	int Amount,
	Type MessageType)
{
	if (!RecipientPawn || !TargetPawn || Amount <= 0) return;

	// Robust bot filter — see TgPawn_Character__SendCombatMessage.cpp for why
	// bIsPlayer isn't trustworthy in this build. Class-name match + pointer
	// sanity-check on Connection.
	if (!RecipientPawn->Controller || !RecipientPawn->Controller->Class) return;
	const char* controllerClass = RecipientPawn->Controller->Class->GetFullName();
	if (!controllerClass || !strstr(controllerClass, "PlayerController")) return;
	APlayerController* PC = (APlayerController*)RecipientPawn->Controller;
	UNetConnection* Connection = (UNetConnection*)PC->Player;
	if (!Connection || (uintptr_t)Connection < 0x10000) return;

	APlayerReplicationInfo* TargetPRI = TargetPawn->PlayerReplicationInfo;
	if (!TargetPRI) return;
	int16_t TargetId = (int16_t)TargetPRI->PlayerID;

	int16_t SourceId = 0;
	if (SourcePawn && SourcePawn->PlayerReplicationInfo) {
		SourceId = (int16_t)SourcePawn->PlayerReplicationInfo->PlayerID;
	}

	CombatMessagePayload payload;
	payload.numRecords   = 1;
	payload.numIndices   = 1;
	payload.nEventType   = (uint16_t)MessageType;
	payload.nIdAttacker  = SourceId;
	payload.nIdAssist    = 0;
	payload.nIdVictim    = TargetId;
	payload.nValueHealth = (int16_t)Amount;
	payload.nValueShield = 0;
	payload.index        = 0;

	EmitPacket(Connection, payload);
}

void SendCombatMessage::CallRaw(
	UNetConnection* Connection,
	uint16_t nEventType,
	int16_t nIdAttacker,
	int16_t nIdAssist,
	int16_t nIdVictim,
	int16_t nValueHealth,
	int16_t nValueShield)
{
	if (!Connection) return;

	CombatMessagePayload payload;
	payload.numRecords   = 1;
	payload.numIndices   = 1;
	payload.nEventType   = nEventType;
	payload.nIdAttacker  = nIdAttacker;
	payload.nIdAssist    = nIdAssist;
	payload.nIdVictim    = nIdVictim;
	payload.nValueHealth = nValueHealth;
	payload.nValueShield = nValueShield;
	payload.index        = 0;

	EmitPacket(Connection, payload);
}
