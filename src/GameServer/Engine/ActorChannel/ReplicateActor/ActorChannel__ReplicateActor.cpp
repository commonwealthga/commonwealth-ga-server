#include "src/GameServer/Engine/ActorChannel/ReplicateActor/ActorChannel__ReplicateActor.hpp"
#include "src/GameServer/Cosmetics/BrokenSuitSwap.hpp"
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

	// r_CustomCharacterAssembly lives on player character pawns AND their PRIs
	// (the PRI mirror drives the client's UpdateCharacterAssetRefs preloading —
	// per-viewer cosmetic swaps must cover both or the viewer still preloads
	// the hidden cosmetic's assets). Bail cheaply for every other actor/channel
	// (the overwhelming common case on this path).
	FCustomCharacterAssembly* assembly = nullptr;
	AController* ownerCtrl = nullptr;
	ATgPawn_Character* pawn = nullptr;
	int wearerProfileId = 0;  // r_nProfileId of the wearer — picks the class-default body for "hide" swaps
	bool wearerIsBot = false; // mission-dressed bots are exempt from the suit-pref swaps
	if (actor) {
		if (ObjectClassCache::ClassNameContains((UObject*)actor, "TgPawn_Character")) {
			pawn = (ATgPawn_Character*)actor;
			assembly = &pawn->r_CustomCharacterAssembly;
			ownerCtrl = pawn->Controller;
			wearerProfileId = pawn->r_nProfileId;
			wearerIsBot = pawn->r_bIsBot != 0;
		} else if (ObjectClassCache::ClassNameContains((UObject*)actor, "TgRepInfo_Player")) {
			auto* pri = (ATgRepInfo_Player*)actor;
			assembly = &pri->r_CustomCharacterAssembly;
			ownerCtrl = (AController*)pri->Owner;  // PRI's Owner is its controller
			wearerProfileId = pri->r_nProfileId;
			wearerIsBot = pri->bBot != 0;  // SpawnBotById stamps bBot on bot PRIs
		}
	}
	if (!assembly) {
		CallOriginal(Channel, edx);
		return;
	}

	void* conn = *(void**)((char*)Channel + kOff_Channel_Connection);

	// Ownership test mirrors the engine's own bNetOwner logic inside
	// ReplicateActor: this connection owns the actor iff the controlling
	// PlayerController's Player (its UNetConnection) IS this channel's
	// Connection. Guarded on a real PlayerController so bot AIControllers
	// (no Player field) never get dereferenced.
	bool isOwner = false;
	if (conn && ownerCtrl &&
	    ObjectClassCache::ClassNameContains((UObject*)ownerCtrl, "PlayerController")) {
		UPlayer* ownerConn = ((APlayerController*)ownerCtrl)->Player;
		isOwner = (ownerConn && (void*)ownerConn == conn);
	}

	// Both swaps below present altered values to this one connection only,
	// then restore the real values before the next connection's pass / any UC
	// read. The assembly re-replicates to a connection as a delta only when it
	// actually changes, so the swap costs nothing per tick in steady state.
	bool trailSuppressed = false;
	int savedTrail = 0;
	bool suitSwapped = false;
	BrokenSuitSwap::SavedFields savedSuit;

	if (!isOwner) {
		// Suppressed jetpack trails: hidden from every non-owner connection
		// (pawn only — the trail renders off the pawn's copy; a bot flaunting
		// one is simply hidden from everyone).
		if (pawn && IsSuppressedTrail(assembly->JetpackTrailId)) {
			savedTrail = assembly->JetpackTrailId;
			assembly->JetpackTrailId = 0;
			trailSuppressed = true;
		}
		// Broken-suit replacement for viewers who disabled them. Cheap
		// mapped-cosmetic check first — the pref lookup only runs for actors
		// actually wearing a mapped cosmetic.
	}

	// Hide-all ("-toggleallsuits 0") outranks the broken-suit swap map, but
	// never applies to the viewer's OWN pawn/PRI — nobody wants to look naked
	// themselves (own broken suits still fall through to the swap branch).
	// Mission-dressed BOTS (the Agenderp) are exempt from both prefs: their
	// cosmetics are authored spectacle, always replicated as-is. Cheap
	// field/table checks first — the per-connection pref lookup only runs for
	// actors actually wearing something.
	if (!wearerIsBot) {
		if (!isOwner &&
			BrokenSuitSwap::HasAnyCosmetic(*assembly) &&
			BrokenSuitSwap::ViewerHidesAllSuits(conn)) {
			suitSwapped = BrokenSuitSwap::HideAll(*assembly, wearerProfileId, savedSuit);
		} else if (BrokenSuitSwap::HasMappedCosmetic(*assembly) &&
			BrokenSuitSwap::ViewerHidesBrokenSuits(conn)) {
			suitSwapped = BrokenSuitSwap::ApplyReplacements(*assembly, wearerProfileId, savedSuit);
		}
	}

	CallOriginal(Channel, edx);

	if (trailSuppressed) assembly->JetpackTrailId = savedTrail;
	if (suitSwapped) BrokenSuitSwap::RestoreAssembly(*assembly, savedSuit);
}
