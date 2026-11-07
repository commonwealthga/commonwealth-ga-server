#pragma once

// Game DLL half of the AvA cargo (AvA tech in matches).
// Control server AVA_CARGO {instance_id, sides:[{task_force, items:[{item_id, deployable_id, count}]}]}
// -> Spawn(): every unit becomes its deployable (the item's "<X> Pack" / beacon,
// asm ref_deployable_id) owned by that task force, placed at the side's TgHexItemFactory spots
// (every AvA map has 28 of them in two groups of 14 "dropship stuff"; a spot belongs to the side
// whose TgTeamPlayerStart is nearest). Agents pick a pack up with the beacon key (B):
// TgDeployable.PickUpDeployable (UC) gives the pack's pickup device (asm pickup_device_id) to deploy.
// The original TgHexItemFactory/TgHexItemInstance natives are stripped; this re-creates
// HIS_Factory (waiting at its spot) -> HIS_PickedUp. ReportResult() (from BeginEndMission, before
// MISSION_ENDED) sends AVA_CARGO_RESULT {returns:[{task_force,item_id,count}]} = the packs still
// untouched at their spot -> the control server returns them to the agency stock; picked-up /
// placed / destroyed tech is used.
#include <string>

class APlayerController;
class ATgDeployable; struct FVector;
// AVA_CARGO items may carry bot_id instead of deployable_id = a siege vehicle (see below).

namespace AvaCargoDll {
void Spawn(const std::string& json);   // IpcClient, game thread
void ReportResult();                   // TgGame::BeginEndMission
// B on a pack. A pack spawned at its spot blocks the pawn (Bump, never Touch), so UC
// PickupNearestDeployable (TouchingActors walk) cannot find it - same gap as the team beacon fallback.
// ServerPickupPutdownDeployable: before = UntouchedCount() before the UC body; after it,
// PickupFallback() picks up the nearest own-side untouched pack within reach if UC took none.
int UntouchedCount();
bool PickupFallback(APlayerController* pc, int untouchedBefore);

// Siege vehicles. AVA_CARGO items with a bot_id (asm item subtype 1300 -> ref_bot_id:
// Vindicator/Tormentor = TgPawn_Siege*, Vandal = TgPawn_CTR) spawn as that bot pawn at the side's spots,
// crewable by the side (s_bIsCrewable + a TgCollisionProxy use volume = the Acquisition path):
// B -> UC PickupNearestBot -> native CrewBot -> HackedBy = the player's controller possesses the vehicle
// (drive, aim, fire its devices, its HP); B again -> ServerReleasePet -> back in the agent.
// Vehicle killed -> TgPawn.Died -> UnCrewed(true) (driver dies with it) -> used up.
// ReportResult: a vehicle still alive at the end goes back to the agency; a destroyed one is used.
// VehicleKeyBefore/After wrap the UC pickup body: leaving a vehicle must not grab a pack; B beside an
// own vehicle whose use volume the agent did not enter by walking (spawned around him) still crews it.
bool VehicleKeyBefore(APlayerController* pc);           // true = this B leaves a vehicle
bool VehicleKeyAfter(APlayerController* pc, bool wasDriving);   // true = handled (skip pack/beacon fallbacks)
// Packs of a side flagged bot_carriers (AVA_CARGO) can be taken by that side's bots.
int BotCarrierTf();                                     // 0 = no bot-carried side this match
ATgDeployable* BotClaimPack(int tf, const FVector& at, float maxDist, int pawnId, float now);   // nearest free pack, claimed
bool BotPackLive(ATgDeployable* d);
int BotTakePack(ATgDeployable* d);                      // picked up: returns deployable id (0 = gone), counts as used
void BotReleasePack(int pawnId);
void Tick();                                            // GameEngine tick: 1 Hz vehicle state log, no-op without vehicles
void DevVehicleHealth(int pct, int taskForce, int botId);   // dev only (admin dev-kill target vehicle_hp): HP to pct, 0 = destroy
}
