#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Placement trace helper (FUN_10a1b7e0) — same one TgDeviceFire::Deploy uses.
// Client UpdateDeployModeStatus calls this with the bot's collision cylinder
// as extent, ffCheck=0 for pet/turret attack types.
static constexpr uintptr_t FN_PLACEMENT_TRACE = 0x10a1b7e0;
using PlaceFn = void*(__cdecl*)(float*, int*, float*, float*, char, char);

// native function SpawnPet(bool bPet);
// Called by CustomFire() for pet/bot attack types. Should retrieve the bot ID from the fire
// mode's assembly data (m_pFireModeSetup / m_pAmSetup) then delegate to TgGame::SpawnBotById.
// The spawn data offsets within m_pFireModeSetup are not yet mapped; stubbed for now.
void __fastcall TgDeviceFire__SpawnPet::Call(UTgDeviceFire* pThis, void* edx, BOOL bPet) {
	if (!pThis) return;

	ATgDevice* device = (ATgDevice*)pThis->m_Owner;
	if (!device) return;

	ATgPawn* pawn = (ATgPawn*)device->Instigator;
	if (!pawn) {
		// Matches the null-check in TgDeviceFire::Deploy (sibling of this
		// hook). AI-spawned pets whose fire-mode runs before Instigator is
		// wired would null-deref pawn->Location below. Bail early.
		Logger::Log(GetLogChannel(),
			"TgDeviceFire::SpawnPet: device has no Instigator, bailing\n");
		return;
	}

	char* pFireModeSetup = (char*)pThis->m_pFireModeSetup.Dummy;
	int petId = *(int*)(pFireModeSetup + 0x2C);

	Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: bPet=%d device=%s pawn=%s petId=%d\n",
		(int)bPet,
		device->GetFullName(),
		pawn ? pawn->GetFullName() : "null",
		petId);


	ATgGame* game = reinterpret_cast<ATgGame*>(Globals::Get().GGameInfo);

	// Resolve placement the same way the client's UpdateDeployModeStatus ghost
	// preview does for pet/turret attack types (0x132 / 0x331): aim-trace with
	// the TURRET's collision cylinder as extent, ffCheck=0 (turrets don't
	// respect force fields).  Without the cylinder extent the trace lands ~30uu
	// from the player and SpawnBotById rejects the placement for overlap — the
	// real cylinder (radius ~22 for personal turret) pushes the contact point
	// out by the footprint, matching the ghost preview.
	//
	// Earlier iteration used pawn->Location + Z200 as an unconditional fallback
	// because the zero-extent trace was unreliable; that spawned the turret
	// floating above the player regardless of aim.
	FVector spawnLocation = pawn->Location;
	FVector spawnNormal   = { 0.f, 0.f, 1.f };
	{
		float radius = 0.f, halfHeight = 0.f;
		TgGame__SpawnBotById::GetBotCollisionCylinder(petId, &radius, &halfHeight);
		FVector ext = { radius, radius, halfHeight };
		void* hit = ((PlaceFn)FN_PLACEMENT_TRACE)(
			&ext.X, (int*)pawn, &spawnLocation.X, &spawnNormal.X, 1, 0);
		if (hit) {
			// Trace returns ground contact; pawn Location is cylinder center,
			// so lift by halfHeight plus the 5uu floor buffer (_DAT_1197edd8)
			// used by the game's placement trace — same adjustment applied in
			// SpawnDeployableActor for deployables.  Without the +5 the turret
			// visual sits slightly sunk into terrain.
			spawnLocation.Z += halfHeight + 5.0f;
		} else {
			// Trace failed (aiming at sky / out of range) — fall back to the
			// known-working spawn-above-player position so the turret still
			// appears.  bIgnoreCollision=true on the SpawnBotById call below
			// handles residual overlap with the player's cylinder.
			spawnLocation   = pawn->Location;
			spawnLocation.Z += 200.0f;
			spawnNormal     = { 0.f, 0.f, 1.f };
		}
	}

	Logger::Log("pet_spawn",
		"TgDeviceFire::SpawnPet: placement resolved to (%.1f, %.1f, %.1f)  pawn at (%.1f, %.1f, %.1f)\n",
		spawnLocation.X, spawnLocation.Y, spawnLocation.Z,
		pawn->Location.X, pawn->Location.Y, pawn->Location.Z);

	// bIgnoreCollision=true — without this, SpawnBotById rejects the spawn
	// when the turret's collision cylinder overlaps the player's.  Humanoid
	// pets tolerated the overlap at +Z200, but TgPawn_TurretPlasma's shape
	// is different and the overlap check fails.  Turrets are stationary, so
	// any brief spawn-time overlap is harmless.
	//
	// Rotation: use Controller->Rotation (view/aim), not pawn->Rotation (body).
	// Body rotation lags / snaps during 3rd-person movement, producing turrets
	// that face a direction the player wasn't looking at.  Zero out pitch/roll
	// so the turret stands upright.
	FRotator spawnRot = pawn->Controller ? pawn->Controller->Rotation : pawn->Rotation;
	spawnRot.Pitch = 0;
	spawnRot.Roll  = 0;
	ATgPawn* PetPawn = (ATgPawn*)game->SpawnBotById(petId, spawnLocation, spawnRot, true, nullptr, true, pawn, false, pThis, 0.0f);
	if (!PetPawn) {
		Logger::Log("pet_spawn",
			"TgDeviceFire::SpawnPet: SpawnBotById returned null for petId=%d at (%.1f,%.1f,%.1f) — spawn rejected (collision? out-of-world?)\n",
			petId, spawnLocation.X, spawnLocation.Y, spawnLocation.Z);
	}
	if (PetPawn) {
		Logger::Log("pet_spawn",
			"TgDeviceFire::SpawnPet: pet spawned 0x%p class=%s at (%.1f,%.1f,%.1f)\n",
			PetPawn,
			PetPawn->Class ? PetPawn->Class->GetFullName() : "<null>",
			PetPawn->Location.X, PetPawn->Location.Y, PetPawn->Location.Z);
		Logger::Log("pet_spawn",
			"TgDeviceFire::SpawnPet: rotation check — spawnRot=(%d,%d,%d) pawn.Rotation=(%d,%d,%d) ctrl.Rotation=(%d,%d,%d)\n",
			spawnRot.Pitch, spawnRot.Yaw, spawnRot.Roll,
			PetPawn->Rotation.Pitch, PetPawn->Rotation.Yaw, PetPawn->Rotation.Roll,
			PetPawn->Controller ? PetPawn->Controller->Rotation.Pitch : 0,
			PetPawn->Controller ? PetPawn->Controller->Rotation.Yaw   : 0,
			PetPawn->Controller ? PetPawn->Controller->Rotation.Roll  : 0);
		ATgRepInfo_Player* PetRep = (ATgRepInfo_Player*)PetPawn->PlayerReplicationInfo;
		ATgRepInfo_Player* PawnRep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
		PetRep->r_TaskForce = PawnRep->r_TaskForce;
		PetRep->Team = PawnRep->Team;
		PetRep->SetTeam(PawnRep->r_TaskForce);
		PetPawn->NotifyTeamChanged();

		PetPawn->Role = 3;
		PetPawn->RemoteRole = 1;
		PetPawn->bNetDirty = 1;
		PetPawn->bNetInitial = 1;
		PetPawn->bForceNetUpdate = 1;
		PetPawn->bSkipActorPropertyReplication = 0;
		// PetPawn->bAlwaysRelevant = 1;

		PetRep->Role = 3;
		PetRep->RemoteRole = 1;
		PetRep->bNetDirty = 1;
		PetRep->bNetInitial = 1;
		PetRep->bForceNetUpdate = 1;
		PetRep->bSkipActorPropertyReplication = 0;
		// PetRep->bAlwaysRelevant = 1;

		// Kick off the deploy animation+state sequence on TgPawn_Turret pets.
		// Without this call the UC deploy timer never starts, r_bIsDeployed
		// (CPF_Net, +0x1C54 bit0) stays false forever, and the client renders
		// the turret locked in the "deploying" animation even though combat AI
		// fires regardless of the flag. Class check via GetFullName() prefix —
		// ATgPawn_Turret, ATgPawn_TurretAVAFlak, ATgPawn_TurretAVARocket all
		// inherit StartDeploy.
		if (PetPawn->Class) {
			const char* clsName = PetPawn->Class->GetFullName();
			if (clsName && strstr(clsName, "TgPawn_Turret") != nullptr) {
				ATgPawn_Turret* turret = (ATgPawn_Turret*)PetPawn;
				turret->StartDeploy();

				// UC StartDeploy pulls the deploy-time from the mesh's
				// "Deploy" animation via ExtractDeployTimeFromMyAnimation. On
				// our server the mesh's anim asset isn't always loaded, so
				// that extraction returns 0 — the deploy completes instantly
				// and the client observes a turret that spawns "almost fully
				// deployed" with no animation phase. Force a sensible default
				// so the deploy takes visible time; the client's posture-blend
				// anim node drives the visual based on these replicated fields.
				if (turret->r_fTimeToDeploySecs <= 0.0f) {
					turret->r_fTimeToDeploySecs  = 2.0f;
					turret->r_fInitDeployTime    = 2.0f;
					turret->r_fCurrentDeployTime = 0.0f;
					turret->r_bIsDeployed        = 0;
					turret->bNetDirty            = 1;
					turret->bForceNetUpdate      = 1;
				}

				// Force rotation again AFTER StartDeploy — the UC deploy path
				// can overwrite the pawn's rotation (AI init / Possess aftermath /
				// turret-specific BeginPlay), and we want the turret locked on the
				// player's aim at the moment of deploy.  Pin the controller,
				// DesiredRotation, and Rotation so nothing drifts during the
				// 2-second deploy window.
				PetPawn->Rotation         = spawnRot;
				PetPawn->DesiredRotation  = spawnRot;
				if (PetPawn->Controller) {
					PetPawn->Controller->Rotation = spawnRot;
					PetPawn->Controller->Focus    = nullptr;  // clear any AI focus target
				}
				PetPawn->bNetDirty       = 1;
				PetPawn->bForceNetUpdate = 1;
				Logger::Log("pet_spawn",
					"TgDeviceFire::SpawnPet: rotation locked to (%d,%d,%d) post-StartDeploy\n",
					spawnRot.Pitch, spawnRot.Yaw, spawnRot.Roll);

				Logger::Log("pet_spawn",
					"TgDeviceFire::SpawnPet: StartDeploy() dispatched on %s\n"
					"                        r_fTimeToDeploySecs=%.2f  r_fInitDeployTime=%.2f\n"
					"                        r_fCurrentDeployTime=%.2f  r_bIsDeployed=%d\n"
					"                        m_fDefaultDeployAnimLength=%.2f  m_fDeployAnimLength=%.2f\n",
					clsName,
					turret->r_fTimeToDeploySecs, turret->r_fInitDeployTime,
					turret->r_fCurrentDeployTime, (int)turret->r_bIsDeployed,
					turret->m_fDefaultDeployAnimLength, turret->m_fDeployAnimLength);
			}
		}
	}

	// ATgPawn* PetPawn = reinterpret_cast<ATgPawn*>(game->SpawnBotById(petId, pawn->Location, pawn->Rotation, true, nullptr, false, pawn, false, pThis, 0.0f));
	// if (PetPawn) {
	// 	Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: PetPawn=%s\n", PetPawn ? PetPawn->GetFullName() : "null");
	// } else {
	// 	Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: PetPawn=null\n");
	// }


	// Logger::DumpMemory("firespawnpet_m_pAmSetup", (void*)pThis->m_pAmSetup.Dummy, 0x1000);
	// Logger::DumpMemory("firespawnpet_m_pFireModeSetup", (void*)pThis->m_pFireModeSetup.Dummy, 0x1000);
	

	// TODO: read spawn bot ID from pThis->m_pFireModeSetup (offsets unknown).
	// Expected flow:
	//   int nBotId = *(int*)((char*)pThis->m_pFireModeSetup.Dummy + OFFSET_SPAWN_ENTITY_ID);
	//   ATgGame* game = (ATgGame*)Globals::Get().GWorld->...;
	//   game->SpawnBotById(nBotId, pawn->Location, pawn->Rotation,
	//                      false, nullptr, false, pawn, !bPet, pThis, 0.f);
}

