#include "src/GameServer/TgGame/TgPawn_Character/SendMarshal/TgPawn_Character__SendMarshal.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include <cstring>

// Connection field offsets used by the inline-drain accounting below. See
// decompiled/Engine/UNetConnection/UNetConnection__BandwidthMechanics.md.
static constexpr int kConnQueuedBytesOffset    = 0x138;  // running bandwidth deficit
static constexpr int kConnMarshalChannelOffset = 0x4FB4; // UMarshalChannel*
// UMarshalChannel::Tick vtable slot — +0x130 in bytes = slot 76 in pointer-sized
// indexing. See decompiled/Engine/UMarshalChannel/UMarshalChannel__Tick/.
static constexpr int kMarshalChannelTickSlot   = 0x130 / 4;

void __fastcall TgPawn_Character__SendMarshal::Call(
	ATgPawn_Character* Pawn, void* /*edx*/, void* Marshal)
{
	if (!Pawn || !Marshal || !Pawn->s_nCharacterId) return;

	APlayerController* PC = (APlayerController*)Pawn->Controller;
	UNetConnection* Connection = (UNetConnection*)PC->Player;
	if (!Connection || (uintptr_t)Connection < 0x10000) return;

	// Enqueue into the marshal channel's outbound queue (chan+0xA8).
	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, Marshal);

	// Force-drain the marshal channel inline so the marshal we just enqueued
	// goes onto the wire in this call instead of waiting for the next
	// UNetConnection::Tick — which under PvP load may not drain marshals at
	// all because UWorld::ServerTickClients has already saturated the
	// per-connection bandwidth budget. UMarshalChannel::Tick gates on
	// IsNetReady, so we temporarily fake a huge unused-budget bank for the
	// duration of the call, then restore QueuedBytes verbatim.
	//
	// We do NOT try to account for the marshal-channel bandwidth cost in the
	// restored QueuedBytes. UChannel::IsReadyToSend calls IsNetReady() with
	// no args, so the Saturate parameter is whatever happens to be in EDX —
	// when that's truthy, IsNetReady writes `QueuedBytes = -BunchBytes`,
	// clobbering our -1B sentinel. Computing `sent = post - fakeBank` in
	// that path produces ≈ +1B "sent bytes", and writing that back
	// permanently exhausts the connection's budget — actor rep stops
	// keeping up forever after, which manifests as massive lag.
	//
	// Marshals are small (tens to hundreds of bytes), 200 KB/s grant is
	// huge, and the in-engine accounting was already fuzzy. Skipping the
	// cost accounting for marshals is a benign accuracy loss.
	//
	// Hook on IsReadyToSend was tried first and crashed (Detours-trampoline
	// interaction with the function's compact entry); doing the bypass at
	// the caller site avoids touching any engine function entry.
	void* MarshalChannel = *(void**)((char*)Connection + kConnMarshalChannelOffset);
	if (MarshalChannel) {
		volatile int32_t* QueuedBytesPtr =
			(volatile int32_t*)((char*)Connection + kConnQueuedBytesOffset);
		constexpr int32_t kFakeBank = -1000000000; // 1 GB of headroom
		const int32_t QbOriginal = *QueuedBytesPtr;
		*QueuedBytesPtr = kFakeBank;

		typedef void(__thiscall* TickFn)(void*);
		TickFn Tick = (TickFn)((*(void***)MarshalChannel)[kMarshalChannelTickSlot]);
		Tick(MarshalChannel);

		*QueuedBytesPtr = QbOriginal;
	}

	NetConnection__FlushNet::CallOriginal(Connection);
}
