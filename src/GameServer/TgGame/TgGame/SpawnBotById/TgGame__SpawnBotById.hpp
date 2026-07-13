#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/Database/Database.hpp"

class TgGame__SpawnBotById : public HookBase<
	ATgPawn*(__fastcall*)(ATgGame*, void*, int, FVector, FRotator, bool, ATgBotFactory*, bool, ATgPawn*, bool, UTgDeviceFire*, float),
	0x10ad9b70,
	TgGame__SpawnBotById> {
public:
	static std::map<int, int> m_spawnedBotIds;
	// bot_id → fully-qualified UClass name; filled from DB on first miss.
	static inline std::unordered_map<int, std::string> m_dbClassCache;

	// Per-bot collision data pulled from asm_data_set_assembly_meshes via
	// asm_data_set_bots.body_asm_id. Cached so we don't hit SQLite on every
	// spawn / posture change. Mirrors what the native SetCollisionFromMesh
	// (0x109be6d0) writes onto the pawn — verbatim from Ghidra:
	//   primary cylinder (CollisionCylinder.SetCylinderSize):
	//     asm.+0x98 = collision_height (half-extent)
	//     asm.+0x9c = collision_radius
	//   m_fStandingHeight/Radius: same as primary (used by UnCrouch restore)
	//   CrouchHeight  ← asm.+0xa4 (crouch_height)
	//   CrouchRadius  ← asm.+0x9c (reuses collision_radius)
	//   m_fTargetCylinderHeight/Radius ← asm.+0xa8 / +0xac (hit_collision_*)
	// The primary cylinder is INTENTIONALLY tall for hover-style bots (Support
	// Scanner: collision_h=70 → 140uu cylinder bottom-on-ground keeps the
	// chassis 70uu above ground; falls naturally off edges, resumes apparent
	// hover when cylinder bottom touches ground again). hit_collision_* is a
	// SEPARATE field for the AI-aim TargetCylinder (m_TargetComponent), NOT
	// for the primary cylinder — they coexist on Scanner asm (70×35 primary,
	// 30×30 target) precisely because the primary IS the height-illusion leg
	// and the target is the tight body the AI aims at.
	struct BotCollisionData {
		float collision_radius;       // primary cylinder radius (= CrouchRadius, StandingRadius)
		float collision_halfHeight;   // primary cylinder half-height (= StandingHeight)
		float crouch_height;          // 0 if not set
		float hit_collision_height;   // 0 if not set; → m_fTargetCylinderHeight
		float hit_collision_radius;   // 0 if not set; → m_fTargetCylinderRadius
	};
	static inline std::unordered_map<int, BotCollisionData> m_dbCollisionCache;

	// DB lookup + cache; returns const ref to the cached row. UC default on
	// TgPawn.CollisionCylinder (Height=46, Radius=25) is the fallback when
	// no asm row is found — every pawn subclass inherits this unchanged.
	static const BotCollisionData& GetBotCollisionData(int nBotId) {
		auto it = m_dbCollisionCache.find(nBotId);
		if (it != m_dbCollisionCache.end()) return it->second;

		BotCollisionData data{};
		data.collision_radius     = 25.0f;   // TgPawn.uc:10554
		data.collision_halfHeight = 46.0f;   // TgPawn.uc:10553
		data.crouch_height        = 0.0f;
		data.hit_collision_height = 0.0f;
		data.hit_collision_radius = 0.0f;

		sqlite3* db = Database::GetConnection();
		if (db) {
			sqlite3_stmt* stmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT am.collision_radius, am.collision_height, "
				"       am.crouch_height, "
				"       am.hit_collision_radius, am.hit_collision_height "
				"FROM asm_data_set_bots b "
				"JOIN asm_data_set_assembly_meshes am ON b.body_asm_id = am.asm_id "
				"WHERE b.bot_id = ? LIMIT 1;",
				-1, &stmt, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(stmt, 1, nBotId);
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					const float dbCR  = (float)sqlite3_column_double(stmt, 0);
					const float dbCH  = (float)sqlite3_column_double(stmt, 1);
					const float dbXH  = (float)sqlite3_column_double(stmt, 2);  // crouch_height
					const float dbHR  = (float)sqlite3_column_double(stmt, 3);  // hit_collision_radius
					const float dbHH  = (float)sqlite3_column_double(stmt, 4);  // hit_collision_height

					// asm collision_height is HALF-height (UCylinderComponent
					// convention — the native SetCollisionFromMesh passes the
					// asm value straight into UCylinderComponent.CollisionHeight
					// at +0x1c8 with no scaling). Sanity check: human asm 2562
					// = 46 → full 92uu = standard UE3 character.
					if (dbCR > 0.0f) data.collision_radius     = dbCR;
					if (dbCH > 0.0f) data.collision_halfHeight = dbCH;
					if (dbHH > 0.0f) data.hit_collision_height = dbHH;
					if (dbHR > 0.0f) data.hit_collision_radius = dbHR;
					if (dbXH > 0.0f) data.crouch_height        = dbXH;
				}
				sqlite3_finalize(stmt);
			}
		}

		auto [ins, _] = m_dbCollisionCache.emplace(nBotId, data);
		return ins->second;
	}

	// asm_data_set_bots.destroy_on_owner_death_flag — whether this bot is
	// cascade-killed when its owner pawn dies. Set on pets/drones (Grizzly,
	// Hornet, Lockdown, Harrier, Eye, decoys), clear on emplaced turrets and
	// standalone map enemies. Read by the intact native KillOwnedBots
	// (0x109e5090), fired from TgPawn.PlayDying at the moment of owner death —
	// it only kills bots with r_bIsBot + r_Owner==dead-pawn + THIS flag. Cached.
	static bool GetDestroyOnOwnerDeathFlag(int nBotId) {
		static std::unordered_map<int, bool> cache;
		auto it = cache.find(nBotId);
		if (it != cache.end()) return it->second;

		bool flag = false;
		sqlite3* db = Database::GetConnection();
		if (db) {
			sqlite3_stmt* stmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT destroy_on_owner_death_flag FROM asm_data_set_bots "
				"WHERE bot_id = ? LIMIT 1;",
				-1, &stmt, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(stmt, 1, nBotId);
				if (sqlite3_step(stmt) == SQLITE_ROW)
					flag = sqlite3_column_int(stmt, 0) != 0;
				sqlite3_finalize(stmt);
			}
		}
		cache[nBotId] = flag;
		return flag;
	}

	// Helper used by spawn-Z-lift callers (SpawnNextBot, SpawnObjectiveBot,
	// SpawnPet, SpawnWave, PlayerActions::SpawnBot). Returns the primary
	// cylinder dimensions — same as what ApplyBotCollisionData installs.
	static void GetBotCollisionCylinder(int nBotId, float* outRadius, float* outHalfHeight) {
		const BotCollisionData& d = GetBotCollisionData(nBotId);
		*outRadius     = d.collision_radius;
		*outHalfHeight = d.collision_halfHeight;
	}

	// Full apply — installs the cylinder and writes the auxiliary pawn fields
	// the native SetCollisionFromMesh would have populated. Call from spawn
	// paths after Spawn() returns; safe to call before or after ApplyPawnSetup
	// (our r_nBodyMeshAsmId-set ordering makes the native a no-op in our flow,
	// so we are the only writer).
	static void ApplyBotCollisionData(ATgPawn* Bot, int nBotId) {
		if (!Bot) return;
		const BotCollisionData& d = GetBotCollisionData(nBotId);

		// Primary cylinder — uses the RAW collision_* values. For Support
		// Scanner this installs 35×70 (140uu full), making the cylinder act
		// as a tall invisible "leg" — bottom on the ground, top at the visible
		// chassis. That is what produces the "hover at fixed elevation" look
		// while still letting gravity carry the bot off ledges.
		if (d.collision_radius > 0.0f && d.collision_halfHeight > 0.0f) {
			Bot->SetCollisionSize(d.collision_radius, d.collision_halfHeight);
		}

		// BaseEyeHeight lift — when the UC class default leaves the eye INSIDE
		// the collision cylinder (BaseEyeHeight < collision_halfHeight), the
		// LOS trace inside `TgPawn::CanSeeActor` / `TgAIController::TargetInLOS`
		// originates from `(Pawn.X, Pawn.Y, Pawn.Z + BaseEyeHeight)` — i.e. at
		// the cylinder's X/Y center, Z below the cylinder top. A trace from
		// that origin to a close-range side-target clips the turret's OWN
		// cylinder wall on exit and the trace fails to reach the target,
		// silently narrowing the apparent vision arc at close range while
		// still working at long range (where the same off-axis angle has a
		// shallow exit and clears the cylinder).
		//
		// Personal Turret is the worst case: UC default BaseEyeHeight=10,
		// asm collision_height=26 (half-height) → eye sits 16uu below the
		// cylinder top, deep inside the body. Lifting to halfHeight+1 puts
		// the trace origin just above the cylinder top so horizontal traces
		// to targets never re-enter the self-cylinder.
		//
		// Data-driven via collision_halfHeight; only fires when the current
		// value is too low. For TgPawn_Character bots (UC default 38, half=46)
		// this bumps eye from 38 → 47 — a minor 9uu shift that keeps the eye
		// just above the head. The trace-arithmetic fix is the same regardless
		// of class.
		//
		// Eye-lift target = top of the ACTUAL collision cylinder. When the asm
		// row carries a collision_height, SetCollisionSize just installed it
		// above and d.collision_halfHeight == the live value. But Auto Cannon /
		// Proto Rocket / Rocket Turret have collision_height=0 in their asm mesh
		// row, so SetCollisionSize never ran and the old `collision_halfHeight >
		// 0` gate skipped the lift entirely — leaving their AI eye at the
		// TgPawn_Turret UC default BaseEyeHeight=10, a full 25uu inside the
		// inherited 35-half-height cylinder. That sunk eye is why the Auto Cannon
		// "refuses to lock on": the TargetInLOS / CanSeeActor trace fires from
		// inside the body and fails on close & off-axis targets. Read the live
		// cylinder height as the fallback so those turrets get lifted too.
		const float cylTop = d.collision_halfHeight > 0.0f
			? d.collision_halfHeight
			: (Bot->CylinderComponent ? Bot->CylinderComponent->CollisionHeight : 0.0f);
		if (cylTop > 0.0f && Bot->BaseEyeHeight < cylTop) {
			Bot->BaseEyeHeight = cylTop + 1.0f;
			Bot->EyeHeight     = Bot->BaseEyeHeight;
		}

		// CrouchHeight/Radius — engine's Crouch()/UnCrouch() reads these to
		// swap the cylinder when posture changes. Mirror the native: the
		// crouch cylinder reuses collision_radius (no separate crouch_radius
		// field exists in the asm row).
		if (d.crouch_height > 0.0f) {
			Bot->CrouchHeight = d.crouch_height;
		}
		if (d.collision_radius > 0.0f) {
			Bot->CrouchRadius = d.collision_radius;
		}

		// m_fTargetCylinderHeight/Radius — dimensions of the SEPARATE AI-aim
		// TargetCylinder component (m_TargetComponent). NOT the primary
		// cylinder. Code that asks "how big is the pawn's tight hit body?"
		// reads these.
		if (d.hit_collision_height > 0.0f) {
			Bot->m_fTargetCylinderHeight = d.hit_collision_height;
		}
		if (d.hit_collision_radius > 0.0f) {
			Bot->m_fTargetCylinderRadius = d.hit_collision_radius;
		}

		// m_fStandingHeight/Radius hold the un-crouched cylinder values so
		// UnCrouch() restores the same cylinder we installed.
		Bot->m_fStandingHeight = (d.collision_halfHeight > 0.0f)
			? d.collision_halfHeight : 46.0f;
		Bot->m_fStandingRadius = (d.collision_radius > 0.0f)
			? d.collision_radius : 25.0f;
	}

	// DB-backed resolver: joins asm_data_set_bots.pawn_class_res_id to
	// asm_data_set_resources.res_id to recover the UC class name. Falls back
	// to empty string if the bot row is missing or the class isn't in the
	// resources table. Cached per bot_id.
	static const char* LookupPawnClassFromDb(int nBotId) {
		auto it = m_dbClassCache.find(nBotId);
		if (it != m_dbClassCache.end()) {
			return it->second.empty() ? nullptr : it->second.c_str();
		}
		std::string result;
		sqlite3* db = Database::GetConnection();
		if (db) {
			sqlite3_stmt* stmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT r.name FROM asm_data_set_bots b "
				"JOIN asm_data_set_resources r ON b.pawn_class_res_id = r.res_id "
				"WHERE b.bot_id = ? LIMIT 1;",
				-1, &stmt, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(stmt, 1, nBotId);
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					const unsigned char* name = sqlite3_column_text(stmt, 0);
					if (name) {
						// UC full-name convention expected by ClassPreloader::GetClass
						// is "Class <FullPath>" (e.g. "Class TgGame.TgPawn_Turret").
						result  = "Class ";
						result += reinterpret_cast<const char*>(name);
					}
				}
				sqlite3_finalize(stmt);
			}
		}
		auto [ins, _] = m_dbClassCache.emplace(nBotId, std::move(result));
		return ins->second.empty() ? nullptr : ins->second.c_str();
	}
	static void GiveDeviceById(
		ATgPawn* Pawn,
		ATgRepInfo_Player* PlayerReplicationInfo,
		int deviceId,
		int slotUsedValueId,
		int equipPoint,
		int qualityValueId,
		bool isOffHand,
		bool isHandDevice,
		int deviceType,
		int nInventoryId
	);
	static void GiveDevicesFromBotConfig(ATgPawn* Bot, ATgRepInfo_Player* BotRepInfo, int nBotId);
	// Crash-safe post-spawn equip of ONE in-hand device (decoy melee mimicry).
	// Mirrors GiveDevicesFromBotConfig's proven per-device wiring and ends with
	// UpdateClientDevices as the FINAL call — never add code after that call that
	// reads spilled locals (that ordering is exactly what makes the legacy
	// GiveDeviceById crash: EBX clobbered across UpdateClientDevices). See
	// project_spawnbotbyid-frame-trap.
	static void EquipInHandDevice(ATgPawn* Pawn, ATgRepInfo_Player* PRI,
		int deviceId, int equipPoint, int quality, int deviceType);
	static inline char* GetPawnClassName(int nBotId) {

		const char* dbClass = LookupPawnClassFromDb(nBotId);
		return const_cast<char*>(dbClass ? dbClass : "Class TgGame.TgPawn_Character");

		switch (nBotId) {
			case 5951: return "Class TgGame.TgPawn_Ambush";
			case 4562: return "Class TgGame.TgPawn_AndroidMinion";
			case 6881: return "Class TgGame.TgPawn_AttackTransport";
			case 4521: return "Class TgGame.TgPawn_Boss_Destroyer";
			case 6563: return "Class TgGame.TgPawn_Brawler";
			case 4981: return "Class TgGame.TgPawn_CTR";
			case 106: return "Class TgGame.TgPawn_Character";
			case 7847: return "Class TgGame.TgPawn_ColonyEye";
			case 6791: return "Class TgGame.TgPawn_Destructible";
			case 1072: return "Class TgGame.TgPawn_Detonator";
			case 7309: return "Class TgGame.TgPawn_Dismantler";
			case 6081: return "Class TgGame.TgPawn_DuneCommander";
			case 5655: return "Class TgGame.TgPawn_Elite_Alchemist";
			case 4929: return "Class TgGame.TgPawn_Elite_Assassin";
			case 6265: return "Class TgGame.TgPawn_EscortRobot";
			case 8112: return "Class TgGame.TgPawn_FlyingBoss";
			case 4879: return "Class TgGame.TgPawn_GroundPetA";
			case 5052: return "Class TgGame.TgPawn_Guardian";
			case 923: return "Class TgGame.TgPawn_Hover";
			case 4538: return "Class TgGame.TgPawn_HoverShieldSphere";
			case 4966: return "Class TgGame.TgPawn_Hunter";
			case 6878: return "Class TgGame.TgPawn_Inquisitor";
			case 5486: return "Class TgGame.TgPawn_Iris";
			case 7090: return "Class TgGame.TgPawn_Juggernaut";
			case 6087: return "Class TgGame.TgPawn_Marauder";
			case 7844: return "Class TgGame.TgPawn_NewWasp";
			case 5294: return "Class TgGame.TgPawn_Reaper";
			case 7310: return "Class TgGame.TgPawn_RecursiveSpawner";
			case 105: return "Class TgGame.TgPawn_Robot";
			case 4650: return "Class TgGame.TgPawn_Scanner";
			case 6654: return "Class TgGame.TgPawn_ScannerRecursive";
			case 5950: return "Class TgGame.TgPawn_Sniper";
			case 6600: return "Class TgGame.TgPawn_SonoranCommander";
			case 4816: return "Class TgGame.TgPawn_SupportForeman";
			case 5293: return "Class TgGame.TgPawn_Switchblade";
			case 7846: return "Class TgGame.TgPawn_Tentacle";
			case 5500: return "Class TgGame.TgPawn_ThinkTank";
			case 4595: return "Class TgGame.TgPawn_Turret";
			case 5802: return "Class TgGame.TgPawn_TurretAVAFlak";
			case 5834: return "Class TgGame.TgPawn_TurretAVARocket";
			case 4240: return "Class TgGame.TgPawn_TurretFlak";
			case 6204: return "Class TgGame.TgPawn_TurretFlame";
			case 475: return "Class TgGame.TgPawn_TurretPlasma";
			case 5205: return "Class TgGame.TgPawn_Vanguard";
			case 6909: return "Class TgGame.TgPawn_VanityPet";
			case 1657: return "Class TgGame.TgPawn_VanityPet";
			case 5515: return "Class TgGame.TgPawn_Vulcan";
			case 6541: return "Class TgGame.TgPawn_Warlord";
			case 7845: return "Class TgGame.TgPawn_WaspSpawner";
			case 6009: return "Class TgGame.TgPawn_Widow";
			default: {
				// Fall back to DB-backed resolution: asm_data_set_bots has the
				// correct pawn_class_res_id for every known bot, joined against
				// asm_data_set_resources. This covers turret pets (bot 1316 →
				// TgPawn_TurretPlasma) and every other bot not in the switch
				// above without having to manually maintain every case.
				const char* dbClass = LookupPawnClassFromDb(nBotId);
				return const_cast<char*>(dbClass ? dbClass : "Class TgGame.TgPawn_Character");
			}


			// todo:
			// TgPawn.uc
			// TgPawn_AVCompositeWalker.uc
			// TgPawn_Boss.uc
			// TgPawn_Interact_NPC.uc
			// TgPawn_NPC.uc
			// TgPawn_Raptor.uc
			// TgPawn_Remote.uc
			// TgPawn_Siege.uc
			// TgPawn_SiegeBarrage.uc
			// TgPawn_SiegeHover.uc
			// TgPawn_SiegeRapidFire.uc
			// TgPawn_TreadRobot.uc
			// TgPawn_UberWalker.uc

			// 725	ARM_Guard,Bolonov,Boss_Champion,Corrupted_Tribesman,Dalton_Bancroft,Dome_City_Guard,Dummy,Kanar_Tribesman,Kanar_Tribesman_F,Legion_Raider,Legion_Soldier,Legion_Templar,MobileMiner_Blaster,MobileMiner_Grinder,SDZone_CatherynIves,SDZone_CommanderRiggs,SDZone_Elkas_Scout,SDZone_Guillermo,SDZone_Kanar_Laborer,SDZone_Kanar_Scout,SDZone_Kanar_Warrior,SDZone_Legion_Grunt,SDZone_Legion_Gunner
			// 755	Boss_Core_Phase2,Boss_Electrocutioner,Boss_Magma_Lord
			// 809	Boss_Viking
			// 1009	Blaster_Minion,Introduction_Sentinel_A,Machinegun_Minion,Minion_Sentinel_A,Minion_Sentinel_B,SDZone_BabylonDefender,zzMinion_Ballista_A
			// 2349	Commonwealth_Dreadnought_MK1A
			// 5010	Perf_Robotics
			// 5307	Mech_Siege_Support
			// 5347	Vindicator_Siege_I
			// 5348	Vindicator_Dark,Vindicator_Siege_II
			// 5764	Tormentor_Siege_I


		}
	};
	static ATgPawn* __fastcall Call(
		ATgGame* Game,
		void* edx,
		int nBotId,
		FVector vLocation,
		FRotator rRotation,
		bool bKillController,
		ATgBotFactory* pFactory,
		bool bIgnoreCollision,
		ATgPawn* pOwnerPawn,
		bool bIsDecoy,
		UTgDeviceFire* deviceFire,
		float fDeployAnimLength
	);
	static inline ATgPawn* __fastcall CallOriginal(
		ATgGame* Game,
		void* edx,
		int nBotId,
		FVector vLocation,
		FRotator rRotation,
		bool bKillController,
		ATgBotFactory* pFactory,
		bool bIgnoreCollision,
		ATgPawn* pOwnerPawn,
		bool bIsDecoy,
		UTgDeviceFire* deviceFire,
		float fDeployAnimLength
	) {
		return m_original(Game, edx, nBotId, vLocation, rRotation, bKillController, pFactory, bIgnoreCollision, pOwnerPawn, bIsDecoy, deviceFire, fDeployAnimLength);
	};
};

