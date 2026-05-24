#include "src/GameServer/TgGame/TgGame_Defense/CacheKismetConfiguration/TgGame_Defense__CacheKismetConfiguration.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__CacheKismetConfiguration::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();

	if (Game == nullptr) {
		LogCallEnd();
		return;
	}

	Game->s_WaveSpawnerList.Clear();

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo == nullptr || WorldInfo->GetGameSequence() == nullptr) {
		Logger::Log("gametimer", "CacheKismetConfiguration: no WorldInfo/game sequence\n");
		LogCallEnd();
		return;
	}

	TArray<USequenceObject*> Nodes;
	WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
		ClassPreloader::GetClass("Class TgGame.TgSeqAct_DefenseWaveSpawner"),
		1,
		&Nodes);

	struct spawnerDef {
		int round;
		float frequency;
		std::vector<int> factories;
	};

	
	std::map<int, spawnerDef> spawnerDefs;

	spawnerDefs[0] = spawnerDef { // Wave 1 Support
		1,
		30.0f,
		std::vector<int> {
			13632,
			13630,
			13638,
			13629,
			13633,
			13634,
			13635,
			13636,
			13637,
			13639,
			13640,
			13641,
			13642,
			13643,
		}
	};

	spawnerDefs[1] = spawnerDef { // Wave 5 support
		5,
		10.0f,
		std::vector<int> {
			13664,
			13665,
			13650,
			13673,
			13691,
			13692,
			13694,
			13700,
			13703,
			13704,
			13708,
			13709,
		}
	};

	spawnerDefs[2] = spawnerDef { // Wave 1 Minions
		1,
		10.0f,
		std::vector<int> {
			13632,
			13630,
			13638,
			13629,
			13633,
			13634,
			13635,
			13636,
			13637,
			13639,
			13640,
			13641,
			13642,
			13643,
		}
	};

	spawnerDefs[3] = spawnerDef { // Wave 4 Juggernaut
		4,
		60.0f,
		std::vector<int> {
			13803, // wave 4 juggernaut
			13804, // wave 4 juggernaut

			13805, // wave 4 juggernaut
			13806, // wave 4 juggernaut
		}
	};

	spawnerDefs[4] = spawnerDef { // Wave 2 Minions
		2,
		10.0f,
		std::vector<int> {
			13645,
			13644,
			13646,
			13647,
			13648,
			13649,
			13651,
			13652,
		}
	};

	spawnerDefs[5] = spawnerDef { // Wave 2 support
		2,
		10.0f,
		std::vector<int> {
			13645,
			13644,
			13646,
			13647,
			13648,
			13649,
			13651,
			13652,
		}
	};

	spawnerDefs[6] = spawnerDef { // Wave 4 support
		4,
		10.0f,
		std::vector<int> {
			13664,
			13665,
			13650,
			13673,
			13691,
			13692,
			13694,
			13700,
			13703,
			13704,
			13708,
			13709,
		}
	};

	spawnerDefs[7] = spawnerDef { // Wave 3 support
		3,
		10.0f,
		std::vector<int> {
			13658,
			13653,
			13654,
			13655,
			13656,
			13657,
			13659,
			13660,
			13661,
			13662,
		}
	};

	spawnerDefs[8] = spawnerDef { // Wave 3 Minions
		3,
		10.0f,
		std::vector<int> {
			13658,
			13653,
			13654,
			13655,
			13656,
			13657,
			13659,
			13660,
			13661,
			13662,
		}
	};

	spawnerDefs[9] = spawnerDef { // Wave 3 Juggernaut Node called...
		3,
		60.0f,
		std::vector<int> {
			13809, // wave 3 juggernaut
		}
	};

	spawnerDefs[10] = spawnerDef { // Wave 4 Minions
		4,
		10.0f,
		std::vector<int> {
			13664,
			13665,
			13650,
			13673,
			13691,
			13692,
			13694,
			13700,
			13703,
			13704,
			13708,
			13709,
		}
	};

	spawnerDefs[11] = spawnerDef { // Wave 4 Minions
		5,
		10.0f,
		std::vector<int> {
			13664,
			13665,
			13650,
			13673,
			13691,
			13692,
			13694,
			13700,
			13703,
			13704,
			13708,
			13709,
		}
	};




	for (int i = 0; i < Nodes.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* Spawner = (UTgSeqAct_DefenseWaveSpawner*)Nodes.Data[i];
		if (Spawner == nullptr) continue;

		std::string map_name = Config::GetMapNameChar();

		if (map_name == "Raid_DomeCityDefense_P") {

			spawnerDef spawnerDef = spawnerDefs[i];
			for (int j = 0; j < spawnerDef.factories.size(); j++) {
				int factory = spawnerDef.factories[j];

				for (int k = 0; k < Game->s_ActorFactories.Num(); k++) {
					ATgActorFactory* Factory = Game->s_ActorFactories.Data[k];
					if (Factory == nullptr) continue;
					if (Factory->m_nMapObjectId == factory) {
						Spawner->Targets.Add(Factory);
					}
				}
			}

			Spawner->m_nRoundNumber = spawnerDef.round;
			Spawner->m_fSpawnFrequency = spawnerDef.frequency;
			Spawner->m_fNextSpawnTime = 0.0f;

			// std::map<int, int> index_to_round = {
			// 	{0, 1}, // Wave 1 Support
			// 	{1, 5}, // Wave 5 support
			// 	{2, 1}, // Wave 1 Minions
			// 	{3, 4}, // Wave 4 Juggernaut
			// 	{4, 2}, // Wave 2 Minions
			// 	{5, 2}, // Wave 2 support
			// 	{6, 4}, // Wave 4 support
			// 	{7, 3}, // Wave 3 support
			// 	{8, 3}, // Wave 3 Minions
			// 	{9, 3}, // Wave 3 Juggernaut Node called...
			// 	{10, 4}, // Wave 4 Minions
			// 	{11, 5}, // Wave 5 minion
			// };
			//
			// std::vector<int> wave1_factories = {
			// 	13632,
			// 	13630,
			// 	13638,
			// 	13629,
			// 	13633,
			// 	13634,
			// 	13635,
			// 	13636,
			// 	13637,
			// 	13639,
			// 	13640,
			// 	13641,
			// 	13642,
			// 	13643,
			//
			// 	13802, // wave 1 initial spawns
			// 	13810, // wave 1 initial spawns
			//
			// };
			// std::vector<int> wave2_factories = {
			// 	13645,
			// 	13644,
			// 	13646,
			// 	13647,
			// 	13648,
			// 	13649,
			// 	13651,
			// 	13652,
			// };
			// std::vector<int> wave3_factories = {
			// 	13658,
			// 	13653,
			// 	13654,
			// 	13655,
			// 	13656,
			// 	13657,
			// 	13659,
			// 	13660,
			// 	13661,
			// 	13662,
			//
			// 	13809, // wave 3 juggernaut
			// };
			// std::vector<int> wave4_factories = {
			// 	13664,
			// 	13665,
			// 	13650,
			// 	13673,
			// 	13691,
			// 	13692,
			// 	13694,
			// 	13700,
			// 	13703,
			// 	13704,
			// 	13708,
			// 	13709,
			//
			// 	13803, // wave 4 juggernaut
			// 	13804, // wave 4 juggernaut
			//
			// 	13805, // wave 4 juggernaut
			// 	13806, // wave 4 juggernaut
		}

		Game->s_WaveSpawnerList.Add(Spawner);

		Logger::Log("gametimer", "Spawner cached: m_nRoundNumber=%d m_fSpawnFrequency=%f m_fNextSpawnTime=%f\n", Spawner->m_nRoundNumber, Spawner->m_fSpawnFrequency, Spawner->m_fNextSpawnTime);
	}

	Logger::Log("gametimer",
		"CacheKismetConfiguration: cached %d defense wave spawner nodes\n",
		Game->s_WaveSpawnerList.Num());

	LogCallEnd();
}
