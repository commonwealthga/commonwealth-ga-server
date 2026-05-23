#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Misc/CMarshal/SetBinBlob/CMarshal__SetBinBlob.hpp"
#include "src/GameServer/Misc/CMarshal/Destroy/CMarshal__Destroy.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include <cstring>

// ─── Wire format (CMarshal key 0x71 BIN_BLOB, one record per marshal) ────────
//   u8 numRecords (1), u8 numIndices (1)
//   CombatRecord (12 bytes)
//   u8 recordIdx (0)
//
// Dispatch: pawn->vtable[0x266](marshal). That slot is the engine's per-
// pawn marshal-send native at 0x109c1000 — a stripped `return;` stub on its
// own, but hooked by TgPawn_Character__SendMarshal to route through
// UClientConnection::SendMarshal + FlushNet. Going through the vtable rather
// than calling the hook's CallOriginal directly means UC-triggered combat
// messages (which use the engine's accumulator at 0x109dd9c0 and dispatch
// through the same slot) and our wrapper calls share one and only one send
// implementation.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

#pragma pack(push, 1)
struct CombatRecord {
	uint16_t nEventType;
	int16_t  nIdAttacker;
	int16_t  nIdAssist;
	int16_t  nIdVictim;
	int16_t  nValueHealth;
	int16_t  nValueShield;
};
#pragma pack(pop)
static_assert(sizeof(CombatRecord) == 12, "CombatRecord must be exactly 12 bytes");

// Pawn vtable slot for the per-pawn marshal-send native. Offset 0x998 in
// the engine emitter's decompile (`*pawn + 0x998`), / 4 = 0x266 in
// pointer-sized slots.
constexpr int kPawnSendMarshalVtableSlot = 0x266;

typedef void (__thiscall *PawnSendMarshalFn)(void* this_pawn, void* marshal);

static void SendMarshalToPawn(ATgPawn* Pawn, void* Marshal) {
	auto** vtable = *(void***)Pawn;
	auto fn = (PawnSendMarshalFn)vtable[kPawnSendMarshalVtableSlot];
	fn(Pawn, Marshal);
}

static void EmitRecord(ATgPawn* Pawn, const CombatRecord& rec) {
	uint8_t payload[2 + sizeof(CombatRecord) + 1];
	uint8_t* p = payload;
	*p++ = 1;  // numRecords
	*p++ = 1;  // numIndices
	memcpy(p, &rec, sizeof(CombatRecord));
	p += sizeof(CombatRecord);
	*p++ = 0;  // index of the one record
	const size_t payloadSize = (size_t)(p - payload);

	uint8_t MarshalStorage[0x80] = {0};
	void* Marshal = MarshalStorage;
	CMarshalObject__Create::CallOriginal(Marshal);
	*(void**)Marshal = CMarshalObject__Create::CMarshal_vftable;
	*(uint16_t*)((uint8_t*)Marshal + 0x26) = 0x9F;
	CMarshal__SetBinBlob::CallOriginal(Marshal, nullptr, 0x71, payload, payloadSize);
	SendMarshalToPawn(Pawn, Marshal);
	CMarshal__Destroy::CallOriginal(Marshal);
}

}  // anonymous namespace

void SendCombatMessage::Call(
	ATgPawn* RecipientPawn,
	ATgPawn* SourcePawn,
	ATgPawn* TargetPawn,
	int Amount,
	Type MessageType)
{
	if (!RecipientPawn || !TargetPawn || Amount <= 0) return;

	// Player-only filter happens in the hook on 0x109c1000 — we can dispatch
	// blindly here. PRI on the target is required to resolve victim ID.
	APlayerReplicationInfo* TargetPRI = TargetPawn->PlayerReplicationInfo;
	if (!TargetPRI) return;

	CombatRecord rec;
	rec.nEventType   = (uint16_t)MessageType;
	rec.nIdAttacker  = (SourcePawn && SourcePawn->PlayerReplicationInfo)
		? (int16_t)SourcePawn->PlayerReplicationInfo->PlayerID : 0;
	rec.nIdAssist    = 0;
	rec.nIdVictim    = (int16_t)TargetPRI->PlayerID;
	rec.nValueHealth = (int16_t)Amount;
	rec.nValueShield = 0;

	EmitRecord(RecipientPawn, rec);
}

void SendCombatMessage::CallRaw(
	ATgPawn* RecipientPawn,
	uint16_t nEventType,
	int16_t nIdAttacker,
	int16_t nIdAssist,
	int16_t nIdVictim,
	int16_t nValueHealth,
	int16_t nValueShield)
{
	if (!RecipientPawn) return;

	CombatRecord rec;
	rec.nEventType   = nEventType;
	rec.nIdAttacker  = nIdAttacker;
	rec.nIdAssist    = nIdAssist;
	rec.nIdVictim    = nIdVictim;
	rec.nValueHealth = nValueHealth;
	rec.nValueShield = nValueShield;

	EmitRecord(RecipientPawn, rec);
}
