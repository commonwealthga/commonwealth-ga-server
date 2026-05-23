#include "src/GameServer/Combat/CombatMessageFlusher/CombatMessageFlusher.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include "src/GameServer/Misc/CMarshal/Destroy/CMarshal__Destroy.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

// ─── Engine internals (called raw — no Detours wrapper needed) ──────────────
//
// FUN_109faeb0: void __thiscall(void* acc, void* marshal)
//   Serializes the accumulator into the marshal at offset 0x26 (opcode 0x9F)
//   via FUN_10938500(marshal, 0x71, payload, payloadSize), then resets the
//   accumulator (records_end := records_begin, indices_end := indices_begin).
//   No-ops if records_count == 0, so empty accumulators are safe to pass.
typedef void (__thiscall *SerializeAccumulatorFn)(void* acc, void* marshal);
static const SerializeAccumulatorFn kSerializeAccumulator =
	(SerializeAccumulatorFn)0x109faeb0;

// Pawn marshal-send vtable slot. *(pawn) + 0x998 in the engine emitter's
// decompile, / sizeof(void*) = 0x266 in pointer-sized slots. 0x109c1000
// is the stripped stub on this class; TgPawn_Character__SendMarshal hooks
// it to do the real UClientConnection::SendMarshal + FlushNet.
constexpr int kPawnSendMarshalVtableSlot = 0x266;
typedef void (__thiscall *PawnSendMarshalFn)(void* pawn, void* marshal);

// Accumulator layout (verified against FUN_109fbc80 / FUN_109faeb0):
//   +0x00..0x07  inner serializer header
//   +0x08        records buffer begin (TArray-style)
//   +0x0c        records buffer end
//   +0x10        records buffer capacity end
//   +0x18        indices buffer begin
//   +0x1c        indices buffer end
//   +0x20        indices buffer capacity end
static inline bool HasPendingRecords(void* acc) {
	if (acc == nullptr) return false;
	uint8_t* p = static_cast<uint8_t*>(acc);
	void* begin = *(void**)(p + 0x08);
	void* end   = *(void**)(p + 0x0c);
	return begin != end;
}

// Flushes one pawn's accumulator. Mirrors the marshal lifecycle the engine
// emitter at 0x109dd9c0 uses for its threshold-triggered flush.
static void FlushPawn(ATgPawn_Character* Pawn) {
	void* acc = reinterpret_cast<void*>(Pawn->s_pCombatMessages.Dummy);
	if (!HasPendingRecords(acc)) return;

	// 0x80 is comfortably above CMarshal's real footprint (~0x40); the engine
	// emitter uses a 0x24-byte stack frame, but oversizing is harmless.
	alignas(8) uint8_t MarshalStorage[0x80] = {0};
	void* Marshal = MarshalStorage;
	CMarshalObject__Create::CallOriginal(Marshal);
	*(void**)Marshal = CMarshalObject__Create::CMarshal_vftable;

	kSerializeAccumulator(acc, Marshal);

	auto** vtable = *(void***)Pawn;
	auto fn = (PawnSendMarshalFn)vtable[kPawnSendMarshalVtableSlot];
	fn(Pawn, Marshal);

	CMarshal__Destroy::CallOriginal(Marshal);
}

// Interval between per-pawn flushes, in seconds of WorldInfo.TimeSeconds.
// 0.1s ≈ 10Hz — fast enough that damage numbers feel responsive, slow
// enough that bursty fights still coalesce identical hits inside the
// engine's dedup before we drain.
constexpr float kFlushIntervalSeconds = 0.2f;

static AWorldInfo* GetWorldInfoSafe() {
	if (Globals::Get().GWorld == nullptr) return nullptr;
	return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

}  // anonymous namespace

void CombatMessageFlusher::Tick() {
	AWorldInfo* WI = GetWorldInfoSafe();
	if (WI == nullptr) return;
	const float now = WI->TimeSeconds;

	// Per-pawn throttle off s_fLastCombatMessageProcess (+0x1648) — the field
	// the original engine clearly intended for this. Per-pawn (vs. a single
	// global timer) spreads marshal sends across the interval window and
	// keeps us from re-walking pawns that just engine-flushed via the
	// 21-record / 101-index threshold.
	for (auto& kv : GClientConnectionsData) {
		ClientConnectionData& cd = kv.second;
		if (cd.bClosed) continue;
		ATgPawn_Character* Pawn = cd.Pawn;
		if (Pawn == nullptr) continue;

		// Default-initialized to 0.0f at pawn construction, so first damage
		// against a freshly-spawned pawn fires on the very next tick. The
		// `now < field` branch defends against WorldInfo.TimeSeconds rolling
		// backwards across a map travel — without it, a pawn that survived
		// the transition (or one whose field outlived a reset) could go
		// permanently silent until the clock caught back up.
		const float since = now - Pawn->s_fLastCombatMessageProcess;
		if (since >= 0.0f && since < kFlushIntervalSeconds) continue;

		// Only bump the timestamp when there was actually something to send.
		// On an empty accumulator we cheap-out at the HasPendingRecords gate
		// inside FlushPawn; leaving the field alone means the next damage
		// event still gets serviced on the very next tick.
		void* acc = reinterpret_cast<void*>(Pawn->s_pCombatMessages.Dummy);
		if (!HasPendingRecords(acc)) continue;

		FlushPawn(Pawn);
		Pawn->s_fLastCombatMessageProcess = now;
	}
}
