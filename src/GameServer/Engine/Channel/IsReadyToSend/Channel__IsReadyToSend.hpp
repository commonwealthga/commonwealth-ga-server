#pragma once

#include "src/Utils/HookBase.hpp"

// UChannel::IsReadyToSend @ 0x10d87e60
//
// Two-stage gate consulted by UMarshalChannel::Tick (and the per-actor probe
// in UWorld::ServerTickClients):
//   1. chan->NumOutRec > 126 → false   (reliable sliding window full)
//   2. else NetConnection->IsNetReady() → bandwidth budget gate
//
// Why we hook this:
//
// UWorld::ServerTickClients walks every relevant actor per connection and
// replicates each one, burning through QueuedBytes (+0x138) via PreSend +
// EnsurePacketRoom + FlushNet. Under PvP load the budget routinely goes
// positive, which means IsNetReady returns false for the rest of the tick.
//
// UMarshalChannel::Tick then runs and bails on the very first IsReadyToSend
// check. The marshal queue at chan+0xA8 grows unbounded (MarshalQueue::Grow
// has no cap). When action briefly pauses, the per-tick replenisher catches
// QueuedBytes back to ≤ 0, IsNetReady flips true, and seconds of accumulated
// combat marshals dump out in one burst — the user sees damage numbers and
// combat alerts vanish during fights and then all show up at once. Bumping
// CurrentNetSpeed (we already raised it to 200 KB/s) only postpones this;
// 10-player PvP relevance still saturates per-tick budget.
//
// Combat marshal bunches are unreliable (UMarshalChannel::Tick builds bunches
// via FOutBunch's default-bReliable=0 ctor and never sets bReliable). They
// don't enter the OutRec list, don't bump NumOutRec, don't participate in
// ACK retransmit. Letting them past the bandwidth gate is safe — the 4999-
// byte per-tick cap inside UMarshalChannel::Tick still bounds drain to
// ~150 KB/s/connection, and the per-bunch 500-byte budget keeps single
// packets small.
//
// For non-MarshalChannel callers (the UActorChannel probe in ServerTickClients
// at the top of the inner replication loop), the original two-stage gate
// runs unchanged — actor rep continues to throttle on bandwidth as designed.
//
// Identification is by vtable compare: UMarshalChannel vtable lives at
// 0x116e1560 (confirmed in decompiled/TgGame/_combat_message_pipeline.md
// § "UMarshalChannel vtable"). All UMarshalChannel instances share that
// pointer, and no other class shares it.
class Channel__IsReadyToSend : public HookBase<
	int(__fastcall*)(void*),
	0x10d87e60,
	Channel__IsReadyToSend> {
public:
	static int __fastcall Call(void* Channel);
	static inline int __fastcall CallOriginal(void* Channel) {
		return m_original(Channel);
	}
};
