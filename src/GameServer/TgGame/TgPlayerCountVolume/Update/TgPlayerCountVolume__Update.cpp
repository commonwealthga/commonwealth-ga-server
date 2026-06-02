#include "src/GameServer/TgGame/TgPlayerCountVolume/Update/TgPlayerCountVolume__Update.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerCountVolume__Update::Call(
    ATgPlayerCountVolume* Volume, void* edx, ATgPawn* Other) {
	(void)edx;
	if (!Volume || !Other) return;
	if (!Volume->Enabled) {
		Logger::Log("playercount",
			"Update skipped: volume=%p disabled\n", (void*)Volume);
		return;
	}

	// Brute-force override at the per-call entry point — the LoadGameConfig
	// pass that sets PlayerCountTarget=1 ran but didn't stick (observed
	// target=3 at touch time, meaning either the volume wasn't in the
	// ActorCache snapshot yet at LoadGameConfig time or something resets the
	// field afterwards, possibly kismet PostBeginPlay). Stomping it here
	// guarantees the threshold is 1 regardless of upstream order.
	// Placeholder until a dynamic resolver lands.
	if (Volume->PlayerCountTarget != 1) {
		Logger::Log("playercount",
			"Update: forcing PlayerCountTarget %d -> 1 on volume=%p\n",
			Volume->PlayerCountTarget, (void*)Volume);
		Volume->PlayerCountTarget = 1;
	}

	// Dedupe — UC Touch fires on every overlap edge, including re-entry after
	// micro-jitter. Players is the canonical "currently inside" set; we keep
	// it append-only (UnTouch doesn't remove, matching the one-shot threshold
	// semantics — once N have hit the volume in this match, the event has
	// already fired).
	APawn* candidate = (APawn*)Other;
	for (int i = 0; i < Volume->Players.Count; i++) {
		if (Volume->Players.Data[i] == candidate) {
			return;
		}
	}

	Volume->Players.Add(candidate);
	const int after = Volume->Players.Count;

	Logger::Log("playercount",
		"Update: volume=%p pawn=%p tf=%d target=%d msgId=%d -> %d player(s) inside\n",
		(void*)Volume, (void*)Other,
		Volume->TaskForceNumber, Volume->PlayerCountTarget, Volume->messageID,
		after);

	// Only fire on the exact crossing (after == target). The kismet binding is
	// "N players reached", so >N must not re-trigger; <N is still climbing.
	if (after != Volume->PlayerCountTarget) {
		return;
	}

	// 1) Fire TgSeqEvent_PlayerCountHit attached to this volume's Kismet
	//    output. ActivateIndex=0 (the event has a single default output pin).
	//    Instigator=the triggering pawn so kismet conditions that read the
	//    causer get a real actor.
	UClass* eventClass = ClassPreloader::GetClass("Class TgGame.TgSeqEvent_PlayerCountHit");
	if (eventClass) {
		Volume->TriggerEventClass(eventClass, (AActor*)Other, 0, 0, nullptr);
		Logger::Log("playercount",
			"Update: TgSeqEvent_PlayerCountHit fired on volume=%p (instigator pawn=%p)\n",
			(void*)Volume, (void*)Other);
	} else {
		Logger::Log("playercount",
			"Update: TgSeqEvent_PlayerCountHit class lookup failed — kismet pin not fired\n");
	}

	// 2) Broadcast the configured messageID to EVERY connected player (both
	//    teams) as a center-screen chat-alert via the SendAlert piggyback.
	//    AlertPriority=High, AlertType=Beneficial — this is a "your team made
	//    it" notification, intentionally attention-grabbing for both sides.
	if (Volume->messageID != 0) {
		SendAlert::Broadcast(Volume->messageID,
		                     /*priority=*/2,   // High
		                     /*type=*/    3,   // Beneficial
		                     /*duration=*/3.0f);
		Logger::Log("playercount",
			"Update: broadcast alert msgId=%d to all connected players\n",
			Volume->messageID);
	} else {
		Logger::Log("playercount",
			"Update: messageID=0, skipping alert broadcast\n");
	}
}
