#include "src/GameServer/Engine/ActorChannel/ReplicateActor/ActorChannel__ReplicateActor.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// Jetpack-trail cosmetics that pollute other players' screens. We hide these
// from every NON-owner connection; the owner keeps seeing their own trail.
// Values are JetpackTrailId (= the equipped game item_id, asm_data_set_items
// item_type 1612; resolved via asm_data_set_msg_translations):
//   7515 = Trail - Rings, 7523 = Trail - Fireworks, 7640 = Trail - Golden Spiral
static inline bool IsSuppressedTrail(int trailId) {
	return trailId == 7515 || trailId == 7523 || trailId == 7640;
}

// UActorChannel field offsets for THIS binary. NOT the stock UE3 layout: the
// 0x10d8c070 decompile shows two throttle fields at +0x68/+0x6C inserted ahead
// of World/Actor, shifting them +0x08 from stock. Verified against the
// decompile (+0x74 is the pointer passed to GetName and whose +0xB4 flags are
// the bNetDirty/bNetOwner bits; +0x3C is used as Connection -> +0x70 NetDriver,
// +0xBC PackageMap). UActorChannel/UNetConnection are opaque blobs in the SDK,
// so these can't be expressed as named fields.
static constexpr unsigned kOff_Channel_Connection = 0x3C;  // UChannel::Connection (UNetConnection*)
static constexpr unsigned kOff_Channel_Actor      = 0x74;  // UActorChannel::Actor (AActor*)

void __fastcall ActorChannel__ReplicateActor::Call(void* Channel, void* edx) {
	AActor* actor = *(AActor**)((char*)Channel + kOff_Channel_Actor);

	// Only player character pawns carry r_CustomCharacterAssembly. Bail cheaply
	// for every other actor/channel (the overwhelming common case on this path).
	if (actor && ObjectClassCache::ClassNameContains((UObject*)actor, "TgPawn_Character")) {
		ATgPawn_Character* pawn = (ATgPawn_Character*)actor;
		const int trailId = pawn->r_CustomCharacterAssembly.JetpackTrailId;
		if (IsSuppressedTrail(trailId)) {
			void* conn = *(void**)((char*)Channel + kOff_Channel_Connection);

			// Ownership test mirrors the engine's own bNetOwner logic inside
			// ReplicateActor: this connection owns the pawn iff the pawn's
			// controlling PlayerController's Player (its UNetConnection) IS this
			// channel's Connection. Guarded on a real PlayerController so bot
			// AIControllers (no Player field) never get dereferenced — a bot
			// flaunting one of these trails is simply hidden from everyone.
			bool isOwner = false;
			AController* ctrl = pawn->Controller;
			if (conn && ctrl &&
			    ObjectClassCache::ClassNameContains((UObject*)ctrl, "PlayerController")) {
				UPlayer* ownerConn = ((APlayerController*)ctrl)->Player;
				isOwner = (ownerConn && (void*)ownerConn == conn);
			}

			if (!isOwner) {
				// Present "no trail" (item 0 = unequipped) to this non-owner
				// connection only, then restore the real value before the next
				// connection's pass / any UC read. The assembly re-replicates to
				// this connection as a delta only when it actually changes, so
				// the swap costs nothing per tick in steady state.
				pawn->r_CustomCharacterAssembly.JetpackTrailId = 0;
				CallOriginal(Channel, edx);
				pawn->r_CustomCharacterAssembly.JetpackTrailId = trailId;
				return;
			}
		}
	}

	CallOriginal(Channel, edx);
}
