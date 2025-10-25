#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnBotById : public HookBase<
	ATgPawn*(__fastcall*)(ATgGame*, void*, int, FVector, FRotator, bool, ATgBotFactory*, bool, ATgPawn*, bool, UTgDeviceFire*, float),
	0x10ad9b70,
	TgGame__SpawnBotById> {
public:
	static std::map<int, int> m_spawnedBotIds;
	static inline char* GetPawnClassName(int nBotId) {
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
			case 5515: return "Class TgGame.TgPawn_Vulcan";
			case 6541: return "Class TgGame.TgPawn_Warlord";
			case 7845: return "Class TgGame.TgPawn_WaspSpawner";
			case 6009: return "Class TgGame.TgPawn_Widow";
			default: return "Class TgGame.TgPawn_Character";


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

