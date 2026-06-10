#include "src/GameServer/TgGame/TgProj_Bot/SpawnBot/TgProj_Bot__SpawnBot.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/ApplyPlayerModsToPet.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of the stripped TgProj_Bot::SpawnBot native (0x10a19c10,
// stub returning 0). UC flow: grenade bounces, settles (FreeGrenade.HitWall
// sets bBounce=false at speed<20 on a floor), TgProj_Bot.HitWall calls
// SpawnTheBot(Location) -> this native -> Destroy().
//
// Mirrors TgDeviceFire::SpawnPet's post-spawn wiring (owner, buffs, team,
// lifespan); the bot id comes from the projectile config instead of the
// fire mode (asm_data_set_projectiles.spawn_bot_id, e.g. Spider Grenades
// projectile 29 -> bot 765, 3 projectiles per throw via prop 287).
ATgPawn* __fastcall TgProj_Bot__SpawnBot::Call(ATgProj_Bot* pThis, void* edx, FVector vLocation) {
	if (!pThis) return nullptr;

	ATgPawn* pawn = (ATgPawn*)pThis->Instigator;
	if (!pawn) {
		Logger::Log("pet_spawn", "TgProj_Bot::SpawnBot: null Instigator — bailing\n");
		return nullptr;
	}

	// s_nSpawnBotId is filled by the intact InitializeProjectile native from
	// asm_data_set_projectiles.spawn_bot_id — same mechanism the SpawnDeployable
	// hook relies on for s_nSpawnDeployableId. Fire-mode setup +0x2C (the mode
	// row's bot_id, same offset SpawnPet reads) is the fallback.
	int botId = pThis->s_nSpawnBotId;
	if (botId == 0) {
		UTgDeviceFire* fireMode = pThis->s_LastDefaultMode;
		if (fireMode && fireMode->m_pFireModeSetup.Dummy) {
			botId = *(int*)((char*)fireMode->m_pFireModeSetup.Dummy + 0x2C);
			Logger::Log("pet_spawn",
				"TgProj_Bot::SpawnBot: botId resolved via fire-mode setup: %d\n", botId);
		}
	}
	if (botId == 0) {
		Logger::Log("pet_spawn",
			"TgProj_Bot::SpawnBot: no spawn_bot_id on projectile or fire mode — bailing\n");
		return nullptr;
	}

	// vLocation is the grenade's resting point (≈ ground contact). Pawn Location
	// is the cylinder CENTER — lift by halfHeight + 5uu floor buffer, same as
	// SpawnPet / SpawnNextBot, or the bot sinks into the terrain.
	float radius = 0.f, halfHeight = 0.f;
	TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
	FVector spawnLoc = vLocation;
	spawnLoc.Z += halfHeight + 5.0f;

	// Yaw-only facing from the projectile; the chase AI re-orients immediately.
	FRotator rot;
	rot.Pitch = 0;
	rot.Yaw   = pThis->Rotation.Yaw;
	rot.Roll  = 0;

	ATgDevice*     srcDevice   = (ATgDevice*)pThis->r_Owner;
	UTgDeviceFire* srcFireMode = pThis->s_OwnerFireMode;
	// Same fallback the SpawnDeployable hook uses for its fire-mode resolution.
	// A null fire mode here degrades silently: no lifespan lookup AND the
	// SpawnBotById pack-limit gate falls back to single-r_Pet replacement,
	// which suicides each prior spider as the next one lands.
	if (!srcFireMode) srcFireMode = pThis->s_LastDefaultMode;
	const int deviceInstId = srcDevice ? srcDevice->r_nDeviceInstanceId : 0;
	ATgGame* game = reinterpret_cast<ATgGame*>(Globals::Get().GGameInfo);

	// bIgnoreCollision=true: the grenade can settle flush against walls/props
	// where the bot cylinder would overlap; the bot should still spawn there.
	ATgPawn* BotPawn = TgGame__SpawnBotById::Call(game, nullptr, botId, spawnLoc, rot,
		/*bKillController=*/false, /*pFactory=*/nullptr, /*bIgnoreCollision=*/true,
		/*pOwnerPawn=*/pawn, /*bIsDecoy=*/false, srcFireMode, /*fDeployAnimLength=*/0.0f);

	if (!BotPawn) {
		Logger::Log("pet_spawn",
			"TgProj_Bot::SpawnBot: SpawnBotById returned null for botId=%d at (%.1f,%.1f,%.1f)\n",
			botId, spawnLoc.X, spawnLoc.Y, spawnLoc.Z);
		return nullptr;
	}

	// Bake the thrower's pet buffs (Pet Health / Pet Damage mods & skills) into
	// the bot's s_Properties — same canonical producer SpawnPet uses.
	ApplyPlayerModsToPet::Apply(pawn, BotPawn, deviceInstId);

	// Team propagation — mirrors TgDeviceFire::SpawnPet (playtested pattern).
	ATgRepInfo_Player* BotRep  = (ATgRepInfo_Player*)BotPawn->PlayerReplicationInfo;
	ATgRepInfo_Player* PawnRep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
	if (BotRep && PawnRep) {
		BotRep->r_TaskForce = PawnRep->r_TaskForce;
		BotRep->Team        = PawnRep->Team;
		BotRep->SetTeam(PawnRep->r_TaskForce);
		BotPawn->NotifyTeamChanged();
	}

	// Lifespan: prop 354 PET_LIFESPAN on the throwing device-mode, scaled by the
	// thrower's prop-355 buff registry. 0 (no DB row, e.g. Spider Grenades) =
	// lives until killed or replaced via the prop-154 pack limit. One-shot
	// 'Suicide' timer instead of raw LifeSpan so natural expiry plays death FX.
	if (srcFireMode) {
		const float baseLifeSpan =
			TgProj_Deployable__SpawnDeployable::GetPetLifeSpanSecs(srcFireMode->m_nId);
		const float lifeSpan =
			TgProj_Deployable__SpawnDeployable::ApplyPetLifeSpanBuff(pawn, baseLifeSpan, deviceInstId);
		if (lifeSpan > 0.0f) {
			Actor__SetTimer::SetTimer(BotPawn, lifeSpan, /*bLoop=*/false, FName("Suicide"), nullptr);
		}
	}

	if (Logger::IsChannelEnabled("pet_spawn")) {
		const char* clsRaw = BotPawn->Class ? BotPawn->Class->GetFullName() : nullptr;
		const std::string cls(clsRaw ? clsRaw : "<null>");
		Logger::Log("pet_spawn",
			"TgProj_Bot::SpawnBot: spawned botId=%d 0x%p class=%s at (%.1f,%.1f,%.1f) owner=0x%p devInst=%d mode=%d\n",
			botId, BotPawn, cls.c_str(), spawnLoc.X, spawnLoc.Y, spawnLoc.Z,
			pawn, deviceInstId, srcFireMode ? srcFireMode->m_nId : 0);
	}

	return BotPawn;
}
