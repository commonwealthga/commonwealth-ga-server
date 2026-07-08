#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/ApplyPlayerModsToPet.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Engine/World/GetGameInfo/World__GetGameInfo.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include <string>

// BOT_TYPE_VALUE_ID (BotConfig+0x5C) that marks a spawned pet as a decoy.
// UC: const TG_BOT_TYPE_DECOY = 628.
static constexpr int TG_BOT_TYPE_DECOY = 628;

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

	if (Logger::IsChannelEnabled(GetLogChannel())) {
		// GetFullName shared static buffer — second call clobbers first.
		const char* devRaw = device ? device->GetFullName() : nullptr;
		std::string devName = devRaw ? devRaw : "<null>";
		const char* pawnRaw = pawn ? pawn->GetFullName() : nullptr;
		std::string pawnName = pawnRaw ? pawnRaw : "null";
		Logger::Log(GetLogChannel(), "TgDeviceFire::SpawnPet: bPet=%d device=%s pawn=%s petId=%d\n",
			(int)bPet,
			devName.c_str(),
			pawnName.c_str(),
			petId);
	}


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

    FRotator rot;
    rot.Pitch = 0;
    rot.Yaw   = spawnRot.Yaw;
    rot.Roll  = 0;

	// Decoy detection is by the spawned bot's BOT_TYPE_VALUE_ID (== 628
	// TG_BOT_TYPE_DECOY), NOT bPet: the Decoy offhand (device 2129) fires as a
	// normal SPAWN_PET (attack_type 306, bPet=TRUE) exactly like turrets/drones.
	// The decoy mimicry is applied below on the spawned pawn.
	ATgPawn* PetPawn = TgGame__SpawnBotById::Call(game, nullptr, petId, spawnLocation, spawnRot, false, nullptr, true, pawn, false, pThis, 0.0f);
	// ATgPawn* PetPawn = (ATgPawn*)game->SpawnBotById(petId, spawnLocation, spawnRot, false, nullptr, true, pawn, false, pThis, 0.0f);

	if (!PetPawn) {
		Logger::Log("pet_spawn",
			"TgDeviceFire::SpawnPet: SpawnBotById returned null for petId=%d at (%.1f,%.1f,%.1f) — spawn rejected (collision? out-of-world?)\n",
			petId, spawnLocation.X, spawnLocation.Y, spawnLocation.Z);
	}
	if (PetPawn) {
		// Tag the pet with its owner so KillAllOwned can find and destroy it
		// on team-change / profile-switch via the PawnList walk.
		PetPawn->Instigator = pawn;

		if (Logger::IsChannelEnabled("pet_spawn")) {
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
		}
		// PetPawn->r_bInitialIsEnemy = 0;

		// Bridge the deploying player's pet-related buff registry to the pet's
		// own s_Properties so player skills (Drone Damage / Pet Range / etc.)
		// actually scale the pet's stats. The pet's TgDeviceFire reads the
		// PET's m_EffectBuffInfo at fire time, not the player's, so without
		// this baking step those skills silently no-op. Mirrors
		// ApplyPlayerModsToDeployable's design for deployables.
		ApplyPlayerModsToPet::Apply(pawn, PetPawn, device ? device->r_nDeviceInstanceId : 0);

		// --- Turret flicker fix (ask 1) + pet-range-mod fix (ask 2) ---
		// Runs right after ApplyPlayerModsToPet. A turret is stationary, so its
		// effective engagement range is min(detection, weapon reach).
		//
		// (1) Flicker: clamp detection (SightRadius) down to the effective gun
		//     range so the turret never acquires a target it can't yet shoot
		//     (the ~10% "lock on / give up" band). min() keeps the sensor cap for
		//     long-gun turrets (Proto Rocket, gun 16000 >> sensor 2560).
		//
		// (2) Pet-range mods (the OG bug): a +X% Pet Range device mod lands an
		//     owner prop-381 buff SCOPED to the mod's device instance
		//     (nReqDeviceInstId > 0). But the turret's range query
		//     (UTgDeviceFire::GetPropertyValueById path A) asks the owner for prop
		//     381 GENERICALLY (devInst=0), so it only matches WILDCARD entries —
		//     skills like Heavy Artillery (devInst=0) work, device-scoped mod
		//     entries never apply. The ScaleTargetProperties bridge writes to the
		//     PET (path B), which range doesn't use, so it can't fix it either.
		//     Fix at the source the query actually reads: scale the pet's OWN
		//     weapon range base by the device-scoped mod %. Heavy Artillery still
		//     compounds on top (path A reads m_fBase). Self-contained — the pet
		//     weapon is a fresh per-turret instance that dies with the turret, so
		//     no owner/CDO pollution. Wildcard (devInst=0) entries are excluded
		//     here since path A already applies them.
		// (Earlier WaitForInventoryThenDoPostPawnSetup hook never fired for
		// turrets: that UC path is gated to custom characters / vanity pets.)
		if (ObjectClassCache::ClassNameContains(PetPawn, "Turret") && PetPawn->Controller) {
			const int srcDevInst = device ? device->r_nDeviceInstanceId : 0;

			// Combined factor from owner pet-range mods scoped to THIS deploy
			// device — the entries path A's generic query misses.
			float modFactor = 1.0f;
			if (pawn->m_EffectBuffInfo.Data) {
				for (int i = 0; i < pawn->m_EffectBuffInfo.Num(); ++i) {
					const FBuffInfo& e = pawn->m_EffectBuffInfo.Data[i];
					if (e.BuffHeader.nPropId != 381) continue;
					const int dev = e.BuffHeader.nReqDeviceInstId;
					if (dev <= 0 || dev != srcDevInst) continue;  // skip wildcard (path A) + other devices
					const float pct = e.fSkillPercentModifier + e.fItemPercentModifier
					                + e.fSelfPercentModifier  + e.fPercentModifier;
					if (pct != 0.0f) modFactor *= (1.0f + pct / 100.0f);
				}
			}

			// Scale the pet weapon's RANGE property (prop 5, m_nRangeIndex) by the
			// mod %. m_fBase is path A's input, m_fRaw the no-buff fallback — scale
			// both. GetMaxRangedDistance reads this.
			if (modFactor != 1.0f) {
				for (int slot = 0; slot < 0x19; ++slot) {
					ATgDevice* dev = PetPawn->m_EquippedDevices[slot];
					if (!dev) continue;
					for (int fm = 0; fm < dev->m_FireMode.Count; ++fm) {
						UTgDeviceFire* fire = dev->m_FireMode.Data[fm];
						if (!fire) continue;
						const int ri = fire->m_nRangeIndex;
						if (ri < 0 || ri >= fire->m_Properties.Count) continue;
						UTgProperty* rp = fire->m_Properties.Data[ri];
						if (!rp) continue;
						rp->m_fBase *= modFactor;
						rp->m_fRaw  *= modFactor;
					}
				}
			}

			ATgAIController* aic = (ATgAIController*)PetPawn->Controller;
			const float gunRange = aic->GetMaxRangedDistance();   // now includes mod (device) + Heavy Artillery (path A)
			const float designedSight = PetPawn->SightRadius;
			// Detection cap = designed sensor range, also scaled by the pet-range
			// mod so a mod that pushes firing past the sensor isn't re-capped.
			const float sensorCap = designedSight * modFactor;
			PetPawn->SightRadius = (gunRange > 0.0f && gunRange < sensorCap) ? gunRange : sensorCap;
			// "turret" channel: confirms the clamp AND whether a range modifier
			// reaches the turret. The pet-range pipeline only sees owner buffs that
			// sit in m_EffectBuffInfo as prop 381 (PET_RANGE_MODIFIER) -- that is
			// how Heavy Artillery works (BridgeOwnerEntriesToPet re-targets 381->114
			// onto the pet weapon). Dump the owner prop-381 entries: none with the
			// [rrr] +15% Pet Range mod equipped => the mod never reaches the owner
			// buff registry (the OG bug); an entry with a non-matching devInst =>
			// the bridge devInst filter drops it.
			if (Logger::IsChannelEnabled("turret")) {
				int n381 = 0;
				if (pawn->m_EffectBuffInfo.Data) {
					for (int i = 0; i < pawn->m_EffectBuffInfo.Num(); ++i) {
						const FBuffInfo& e = pawn->m_EffectBuffInfo.Data[i];
						if (e.BuffHeader.nPropId != 381) continue;
						++n381;
						Logger::Log("turret",
							"  owner prop381(PetRange) entry: devInst=%d skillPct=%.3f itemPct=%.3f selfPct=%.3f otherPct=%.3f\n",
							e.BuffHeader.nReqDeviceInstId,
							e.fSkillPercentModifier, e.fItemPercentModifier,
							e.fSelfPercentModifier, e.fPercentModifier);
					}
				}
				const std::string turretCls(PetPawn->Class ? PetPawn->Class->GetFullName() : "<no-class>");
				const char* rawOwner = pawn ? pawn->GetFullName() : nullptr;
				const std::string ownerName(rawOwner ? rawOwner : "<null>");
				Logger::Log("turret",
					"SpawnPet turret=%s owner=%s srcDevInst=%d modFactor=%.3f designedSight=%.0f gunRange=%.0f -> SightRadius=%.0f prop381_entries=%d\n",
					turretCls.c_str(), ownerName.c_str(), srcDevInst, modFactor,
					designedSight, gunRange, PetPawn->SightRadius, n381);
			}
		}

		ATgRepInfo_Player* PetRep = (ATgRepInfo_Player*)PetPawn->PlayerReplicationInfo;
		ATgRepInfo_Player* PawnRep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
		PetRep->r_TaskForce = PawnRep->r_TaskForce;
		PetRep->Team = PawnRep->Team;
		PetRep->SetTeam(PawnRep->r_TaskForce);
		PetPawn->NotifyTeamChanged();

		// --- Decoy: mimic the deploying player's appearance ---
		// The Decoy offhand (device 2129) fires as a normal SPAWN_PET
		// (attack_type 306, bPet=TRUE) just like turrets/drones, so bPet can't
		// distinguish it. The spawned bot's BOT_TYPE_VALUE_ID (BotConfig+0x5C)
		// == 628 (TG_BOT_TYPE_DECOY) is the discriminator. Copy the owner's
		// custom-character assembly (Suit/Head/Hair/Helmet mesh + skin/dye/flair,
		// incl. bValidCustomAssembly) onto the decoy pawn AND its PRI so the
		// client's OnCustomAssemblyChanged -> IsCustomCharacter path rebuilds the
		// decoy from the player's cosmetics (same mechanism SpawnBotPawn uses to
		// render a bot as a custom character). r_bIsDecoy also drives the
		// server-side taunt/threat mechanic (TgDeviceFire::IsDecoyEffect).
		// Assembly + head-mesh writes are at ATgPawn_Character offsets, so gate
		// them on both pawns actually being Characters; r_bIsDecoy / body-mesh /
		// profile-type live on base ATgPawn.
		void* botCfg = (void*)CAmBot__LoadBotMarshal::m_BotPointers[petId];
		const int botTypeValueId = botCfg ? *(int*)((char*)botCfg + 0x5C) : 0;
		const bool isDecoy = (botTypeValueId == TG_BOT_TYPE_DECOY);
		if (isDecoy) {
			PetPawn->r_bIsDecoy            = 1;
			PetPawn->r_nProfileTypeValueId = pawn->r_nProfileTypeValueId;
			PetPawn->r_nBodyMeshAsmId      = pawn->r_nBodyMeshAsmId;
			const bool decoyIsChar = ObjectClassCache::ClassNameContains(PetPawn, "TgPawn_Character");
			const bool ownerIsChar = ObjectClassCache::ClassNameContains(pawn, "TgPawn_Character");
			if (decoyIsChar && ownerIsChar) {
				ATgPawn_Character* ownerChar = (ATgPawn_Character*)pawn;
				ATgPawn_Character* decoyChar = (ATgPawn_Character*)PetPawn;
				decoyChar->r_CustomCharacterAssembly = ownerChar->r_CustomCharacterAssembly;
				decoyChar->r_nHeadMeshAsmId          = ownerChar->r_nHeadMeshAsmId;
				if (PetRep) PetRep->r_CustomCharacterAssembly = ownerChar->r_CustomCharacterAssembly;
			}
			// Nameplate: the client renders "<PlayerName> - Decoy" for r_bIsDecoy
			// pawns. The decoy bot's PRI defaults to the bot's own name ("Decoy"),
			// giving "Decoy - Decoy" — replace it with the owner's name so it reads
			// "<Owner> - Decoy". Mirror SpawnPlayerCharacter's FString handoff:
			// eventSetPlayerName memcpys our FString into its Parms and ProcessEvent
			// may free the Data, so pass a COPY and null its Data before scope exit
			// (never hand it PawnRep's own PlayerName.Data — that would free the
			// owner's name).
			if (PetRep && PawnRep && PawnRep->PlayerName.Data) {
				FString ownerName(PawnRep->PlayerName.Data);
				PetRep->eventSetPlayerName(ownerName);
				ownerName.Data = nullptr;
				ownerName.Count = ownerName.Max = 0;
			}

			PetPawn->bNetDirty       = 1;
			PetPawn->bForceNetUpdate = 1;
			if (PetRep) { PetRep->bNetDirty = 1; PetRep->bForceNetUpdate = 1; }

			// Melee mimicry: give the decoy the recon's ES1 melee weapon (engine
			// equip point 1) and set it in-hand so the already-engaging follow-AI
			// swings it. The weapon's dye rides along in r_CustomCharacterAssembly
			// (DyeList[3]=WeaponColor, [4]=WeaponEmissive), copied above. No damage
			// results — r_bIsDecoy makes TgDeviceFire::IsDecoyEffect short-circuit
			// SubmitEffect. Uses the crash-safe single-device equip helper (NOT the
			// landmined GiveDeviceById). Done LAST in this block, after all local
			// reads, since EquipInHandDevice ends in UpdateClientDevices.
			ATgDevice* ownerMelee = pawn->m_EquippedDevices[1];
			if (ownerMelee && ownerMelee->r_nDeviceId != 0) {
				TgGame__SpawnBotById::EquipInHandDevice(
					PetPawn, PetRep, ownerMelee->r_nDeviceId,
					/*equipPoint=*/1, ownerMelee->r_nQualityValueId, GA_G::TGDT_MELEE);
			}
		}

		// Pet auto-despawn timer. Prop 354 TGPID_PET_LIFESPAN on the spawn-pet
		// device-mode (drones 10-20s typical; turrets generally lack the row →
		// no time limit, the limit-enforcement system replaces them instead).
		//
		// We schedule a one-shot timer firing UC's `Suicide` event rather than
		// writing `PetPawn->LifeSpan` directly — raw `LifeSpan` expiry calls
		// the engine's bare `Destroy()` with NO FX (silent vanish). `Suicide`
		// runs TakeDamage(Health, DmgType_Suicided) → Died → PlayDying →
		// PlayDyingEffects → `Mesh.FxActivateIndependant('PawnDied', …)`, the
		// same combat-death FX route the engine uses for regular kills.
		//
		// (Why not `Despawn`? `Despawn` sets r_eDeathReason=1 which routes
		// PlayDyingEffects to the `'Despawned'` FX group instead — that group
		// is for kismet-driven cleanup and isn't authored on most drone
		// meshes, so calling Despawn produced a silent vanish. `Suicide` →
		// `'PawnDied'` is authored on every bot mesh.)
		if (pThis) {
			// Raw DB read of prop 354 (PET_LIFESPAN) on the spawn-pet
			// device-mode, then scaled by the deploying pawn's prop-355
			// (PET_LIFESPAN_MODIFIER) buff registry — same pattern as the
			// deploy-time chain a few lines down. Without the buff scale,
			// skills like Drones 631/797/798 and Infiltration 837 silently
			// no-op against the lifespan.
			const float baseLifeSpan =
				TgProj_Deployable__SpawnDeployable::GetPetLifeSpanSecs(pThis->m_nId);
			const float petLifeSpan =
				TgProj_Deployable__SpawnDeployable::ApplyPetLifeSpanBuff(
					pawn, baseLifeSpan,
					device ? device->r_nDeviceInstanceId : 0);
			if (petLifeSpan > 0.0f) {
				Actor__SetTimer::SetTimer(PetPawn, petLifeSpan,
					/*bLoop=*/false, FName("Suicide"), nullptr);
				Logger::Log("pet_spawn",
					"TgDeviceFire::SpawnPet: lifespan set petPawn=0x%p botId=%d mode=%d "
					"-> SetTimer(base=%.2fs buffed=%.2fs, 'Suicide') for natural-expire death FX\n",
					PetPawn, petId, pThis->m_nId, baseLifeSpan, petLifeSpan);
			}
		}

		// PetPawn->Role = 3;
		// PetPawn->RemoteRole = 1;
		// // PetPawn->bNetDirty = 1;
		// PetPawn->bNetInitial = 1;
		// // PetPawn->bForceNetUpdate = 1;
		// PetPawn->bSkipActorPropertyReplication = 0;
		// // PetPawn->bAlwaysRelevant = 1;
		//
		// PetRep->Role = 3;
		// PetRep->RemoteRole = 1;
		// // PetRep->bNetDirty = 1;
		// PetRep->bNetInitial = 1;
		// // PetRep->bForceNetUpdate = 1;
		// PetRep->bSkipActorPropertyReplication = 0;
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

				// Write the deploy-time fields BEFORE StartDeploy(). UC's
				// `Deploy.BeginState` runs synchronously inside GotoState('Deploy')
				// and bails to DeployComplete() (instant deploy) when
				// r_fTimeToDeploySecs == 0. ExtractDeployTimeFromMyAnimation
				// returns 0 server-side because the mesh's anim asset isn't
				// loaded, so without seeding the field first the turret
				// snap-deploys regardless of the DB value.
				//
				// prop 279 ("Time To Deploy (secs)") lives on the spawn-pet
				// device-mode keyed by this pet's bot_id — 1s personal turret,
				// 25–60s big emplaced turrets. Authoritative; overrides any
				// anim-length fallback.
				//
				// Don't write r_fInitDeployTime here — UC sets it to
				// WorldInfo.TimeSeconds inside Deploy.BeginState (it's a
				// timestamp, not a duration). r_fCurrentDeployTime and
				// r_bIsDeployed reset to fresh values for a clean deploy cycle.
				float petDeploySecs = TgProj_Deployable__SpawnDeployable::ApplyDeployTimeBuff(
					pawn,
					TgProj_Deployable__SpawnDeployable::GetPetDeployTimeSecs(petId),
					device ? device->r_nDeviceInstanceId : 0);
				turret->r_fTimeToDeploySecs  = petDeploySecs;
				turret->r_fCurrentDeployTime = 0.0f;
				turret->r_bIsDeployed        = 0;
				// turret->r_nPhysicalType = 861;
				// turret->r_bIsBot = 1;
				// turret->s_bInvisibleToPets = 0;
				// turret->s_bCanSeePets = 1;
				// turret->r_bIsHacked = 0;
				// turret->r_bIsHacking = 0;
				// turret->r_bIsDecoy = 0;
				// turret->m_bIsInvisibleToAI = 0;
				// turret->s_bIsCrewable = 0;


				// Henchman + r_Owner is what TgAIController::IsEnemy uses to decide
				// hostility for a pet-pawn. With r_bIsHenchman=1, the IsEnemy native
				// (0x10a80ff0) takes the henchman branch and returns
				// `Actor::IsEnemy(this, target->r_Owner)`; with r_bIsHenchman=0 it
				// falls through to the standard parent IsEnemy which compares PRI
				// teams — but our pet's PRI team isn't reliably propagated from the
				// deployer at the moment enemy AI evaluates it, so the assassin
				// never promotes the turret to Controller::Enemy and EMP action
				// 10907 never fires.
				//
				// The wave-revive crash that originally motivated commenting this
				// out is already defended against in
				// RegisterForWaveRevive/Revive*Timer (see
				// reference_wave_revive_henchman_collision.md). Re-enabled here so
				// enemy AI correctly engages player-deployed turrets.
				// turret->r_bIsHenchman = 1;
				// turret->r_Owner = pawn;

				// Drive deploy via the canonical posture transition (1=HIBERNATE/
				// stowed → 0=DEFAULT/deployed) instead of calling StartDeploy()
				// directly. Reason: clients receive the deploy through
				// `ReplicatedEvent('r_ePosture')` → `SetDeployStateBasedOnPosture()` →
				// `StartDeploy()` → `GotoState('Deploy')` → client-side
				// `Deploy.BeginState` runs `FxActivateGroup('Deploying')` and the
				// posture-blend anim node animates from stowed pose toward deployed.
				// Calling `StartDeploy()` directly only replicates the state name —
				// the visual buildup FX never fire client-side and the turret stays
				// invisible until DeployComplete swaps to the deployed mesh.
				//
				// Step 1: mark stowed. r_ePosture replicates so clients lock in the
				// stowed pose as the blend source.
				// turret->r_ePosture = 1;  // TG_POSTURE_HIBERNATE
				// turret->bNetDirty            = 1;
				// turret->bForceNetUpdate      = 1;

				// Step 2: transition to deployed. SetPosture handles the 1→0 path
				// (super-call into TgPawn::SetPosture replicates r_ePosture, then
				// SetDeployStateBasedOnPosture sees the transition and calls
				// StartDeploy on both server and client). Use the SDK
				// eventSetPosture wrapper — it dispatches via ProcessEvent so the
				// UC override on TgPawn_Turret is what runs.
				turret->eventSetPosture(/*ePosture=*/0, /*fRateScale=*/1.0f, /*bIgnoreTransition=*/0);

				// Pin rotation everywhere the AI rotation tick reads from. UC's
				// TgAIController.uc:2080–2093 picks DesiredRotation from one of
				// `m_rFixedDirection` (when `m_fFixedFOV != 1.0` and no target)
				// or `m_rSpawnDirection` (when at spawn location with move-code
				// 241). Without seeding those, the AI tick reasserts whatever
				// default Possess() left in them — typically (0,0,0) world-X —
				// and snaps the turret to a "random" facing the moment it ticks.
				//
				// Pawn->Rotation/DesiredRotation alone aren't enough: bForceDesiredRotation
				// gets re-set true on the AI side every tick, then the rotation
				// tick blends Pawn->Rotation toward Controller->DesiredRotation
				// (which the AI just overwrote from m_rSpawnDirection).
				//
				// Setting m_rSpawnDirection + m_rFixedDirection on the controller
				// (and m_vSpawnLocation so the AtLocation check in branch 241
				// passes) makes both AI paths face our spawnRot. Focus = null
				// keeps any auto-lookat off.
				//
				// 2026-05-08: confirmed the pin does NOT suppress idle scanning
				// (with pin removed, AI defaults m_rFixedDirection to (0,0,0)
				// world-X — same fixed-pin behavior, just to a wrong direction).
				// Idle scanning is anim-tree-driven, not AI-driven.
				PetPawn->Rotation         = rot;
				PetPawn->DesiredRotation  = rot;
				if (PetPawn->Controller) {
					ATgAIController* aic = (ATgAIController*)PetPawn->Controller;
					aic->Rotation           = rot;
					// aic->Focus              = nullptr;
					aic->DesiredRotation    = rot;
					aic->m_rFixedDirection  = rot;
					aic->m_rSpawnDirection  = rot;
					aic->m_vSpawnLocation   = PetPawn->Location;
					// aic->bReplicateMovement = 1;
				}
				// PetPawn->Rotation.Pitch = 50;
				// PetPawn->bReplicateMovement = 1;
				// PetPawn->bNetDirty       = 1;
				// PetPawn->bForceNetUpdate = 1;
				Logger::Log("pet_spawn",
					"TgDeviceFire::SpawnPet: rotation locked to (%d,%d,%d) pitch-zeroed "
					"(pawn + AI m_rFixedDirection/m_rSpawnDirection); spawnRot was (%d,%d,%d)\n",
					rot.Pitch, rot.Yaw, rot.Roll,
					spawnRot.Pitch, spawnRot.Yaw, spawnRot.Roll);

				Logger::Log("pet_spawn",
					"TgDeviceFire::SpawnPet: StartDeploy() dispatched on %s  botId=%d  prop279=%.2fs\n"
					"                        r_fTimeToDeploySecs=%.2f  r_fInitDeployTime=%.2f\n"
					"                        r_fCurrentDeployTime=%.2f  r_bIsDeployed=%d  r_ePosture=%d\n"
					"                        m_fDefaultDeployAnimLength=%.2f  m_fDeployAnimLength=%.2f\n",
					clsName, petId, petDeploySecs,
					turret->r_fTimeToDeploySecs, turret->r_fInitDeployTime,
					turret->r_fCurrentDeployTime, (int)turret->r_bIsDeployed,
					(int)turret->r_ePosture,
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

