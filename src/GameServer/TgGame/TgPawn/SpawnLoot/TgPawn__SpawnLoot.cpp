#include "src/GameServer/TgGame/TgPawn/SpawnLoot/TgPawn__SpawnLoot.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>

// Reimplementation of UTgPawn::SpawnLoot. The native @ 0x109c0a90 is stripped
// (decompiles to a single `ret`) so the UC chain
//   TgPawn.Died → SpawnLoot(killerTaskForce)
// is a no-op, and no PvP death pickup is ever produced.
//
// We pick a single hard-coded DeathDrop item (1509), spawn a TgDroppedItem at
// the dying pawn's location, set the replicated r_nItemId, and mirror UC's
// client-side ReplicatedEvent path on the server so the GiveTo cycle works
// when a picker pawn touches the actor.
//
// Item 1509 selection rationale (verified against asm_data_set_items +
// asm_data_set_item_effect_groups):
//   item_type_value_id = 724 ("Dropped (DeathDrop)")
//   time_to_live_secs  = 60
//   type-264 effect groups:
//     4152 — prop 51 (HEALTH) +35% + prop 243 (POWERPOOL) +50
//     7587 / 7820 / 16660 — prop 140 +99
//   Only DeathDrop item in the DB with BOTH health and power restore.

// asm_data_set_items.id (the row picked above). Centralized so future tuning
// can swap the drop class without touching the spawn plumbing.
constexpr int kDeathDropItemId = 1509;

void __fastcall TgPawn__SpawnLoot::Call(ATgPawn* Pawn, void* /*edx*/, ATgRepInfo_TaskForce* /*tf*/) {
	if (!Pawn) return;

	// Gate 1: PvP modes only. r_bIsPVP lives on ATgRepInfo_Game (subclass of
	// AGameReplicationInfo) at 0x0264 bit 0x4 and is set externally by the
	// game-mode setup path (user handles that wiring). PvE deaths don't drop.
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal(
		(UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (!WI || !WI->GRI) return;
	ATgRepInfo_Game* TgGri = reinterpret_cast<ATgRepInfo_Game*>(WI->GRI);
	if (!TgGri->r_bIsPVP) return;

	// Gate 2: only player deaths drop. AI/bots/turrets dying do not produce
	// pickups. Per memory rule: bIsPlayer is unreliable on this build —
	// classify by Controller's class name instead.
	if (!Pawn->Controller || !Pawn->Controller->Class) return;
	const char* ctrlNameRaw = Pawn->Controller->Class->GetFullName();
	const std::string ctrlName(ctrlNameRaw ? ctrlNameRaw : "<null>");
	if (ctrlName.find("PlayerController") == std::string::npos) return;

	UClass* dropClass = ClassPreloader::GetClass("Class TgGame.TgDroppedItem");
	if (!dropClass) {
		Logger::Log("debug",
			"[SpawnLoot] TgDroppedItem class not loaded — drop skipped\n");
		return;
	}

	// Spawn slightly above the body. TgDroppedItem's collision cylinder is
	// 30r/20h (DefaultProperties) — lifting 30u keeps the pickup floating
	// just above the corpse so a chasing player can touch it on approach
	// without having to crouch over the body.
	FVector spawnLoc = Pawn->Location;
	spawnLoc.Z += 30.0f;
	FRotator spawnRot = {0, 0, 0};

	ATgDroppedItem* drop = reinterpret_cast<ATgDroppedItem*>(Pawn->Spawn(
		dropClass, /*SpawnOwner=*/nullptr, FName(),
		spawnLoc, spawnRot, /*ActorTemplate=*/nullptr,
		/*bNoCollisionFail=*/1));
	if (!drop) {
		Logger::Log("debug",
			"[SpawnLoot] Spawn(TgDroppedItem) returned null at (%.1f,%.1f,%.1f)\n",
			spawnLoc.X, spawnLoc.Y, spawnLoc.Z);
		return;
	}

	// Setting Instigator to the dying pawn lines up with UC's ValidTouch
	// "Other == Instigator" check (suppresses pickup while the dying player
	// is bouncing upward post-death). Doesn't restrict pickup to anyone in
	// particular — UC's CanPickupDroppedItem handles the broader gating.
	drop->Instigator = reinterpret_cast<APawn*>(Pawn);

	// Replicated trigger. Setting r_nItemId fires UC's ReplicatedEvent on the
	// client, which calls ApplyItemSetup (our hook → mesh + lifespan + populated
	// s_EffectGroupList) and GotoState('Pickup').
	drop->r_nItemId    = kDeathDropItemId;
	// drop->bNetInitial  = 1;
	// drop->bNetDirty    = 1;

	// Mirror the same path on the SERVER. UC's ReplicatedEvent only fires
	// client-side, but the GiveTo loop (which restores health/power) runs
	// server-side and needs s_EffectGroupList populated to do any work.
	// Likewise we need to enter state 'Pickup' so the Touch event handler
	// becomes active server-side. ApplyItemSetup is idempotent — the client
	// firing it again post-replication will see the list already populated
	// and skip.
	drop->ApplyItemSetup();
	drop->GotoState(FName("Pickup"), FName(), 0, 0);

	Logger::Log("debug",
		"[SpawnLoot] dropped item_id=%d at (%.1f,%.1f,%.1f) actor=%p victim=%s\n",
		kDeathDropItemId, spawnLoc.X, spawnLoc.Y, spawnLoc.Z,
		(void*)drop, ctrlName.c_str());
}
