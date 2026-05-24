#include "src/GameServer/TgGame/TgGame_Defense/TickWaveNodes/TgGame_Defense__TickWaveNodes.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__TickWaveNodes::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();

	if (Game == nullptr) {
		LogCallEnd();
		return;
	}

	if (Game->s_WaveSpawnerList.Num() == 0) {
		LogCallEnd();
		return;
	}

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo == nullptr) {
		LogCallEnd();
		return;
	}

	UClass* BotFactoryClass = ClassPreloader::GetClass("Class TgGame.TgBotFactory");
	const float Now = WorldInfo->TimeSeconds;
	int activatedNodes = 0;
	int kickedFactories = 0;

	for (int i = 0; i < Game->s_WaveSpawnerList.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* Spawner = Game->s_WaveSpawnerList.Data[i];
		// Logger::Log("gametimer", "Spawner try tick: m_nRoundNumber=%d m_fSpawnFrequency=%f m_fNextSpawnTime=%f\n", Spawner->m_nRoundNumber, Spawner->m_fSpawnFrequency, Spawner->m_fNextSpawnTime);
		if (Spawner == nullptr) continue;
		if (Spawner->m_nRoundNumber != Game->s_nRoundNumber) continue;
		if (Spawner->m_fNextSpawnTime > Now) continue;

		float Frequency = Spawner->m_fSpawnFrequency;
		if (Frequency <= 0.0f) Frequency = 1.0f;
		Spawner->m_fNextSpawnTime = Now + Frequency;

		int nodeFactories = 0;

		// int nRandomFactory = rand() % Spawner->Targets.Num();
		// UObject* Target = Spawner->Targets.Data[nRandomFactory];

		float percent = Game->NumPlayers / 10;
		int nMaxTargets = std::max(1, int(percent * Spawner->Targets.Num()));
		for (int j = 0; j < nMaxTargets; j++) {
			UObject* Target = Spawner->Targets.Data[rand() % Spawner->Targets.Num()];
			if (Target == nullptr) continue;
			if (BotFactoryClass != nullptr && !Target->IsA(BotFactoryClass)) continue;

			ATgBotFactory* Factory = (ATgBotFactory*)Target;
			// Factory->SpawnWave(rand() % 10 + 3);
			Factory->nPrevPriority = Factory->nPriority;
			Factory->nPriority = Game->s_nCurrentPriority;
			Factory->SpawnNextBot();
			Factory->nPriority = Factory->nPrevPriority;
			nodeFactories++;
			kickedFactories++;
		}

		Spawner->ActivateOutputLink(0);
		activatedNodes++;

		Logger::Log("gametimer",
			"TickWaveNodes: round=%d activated spawner %s factories=%d next=%.2f\n",
			Game->s_nRoundNumber, Spawner->GetName(), nodeFactories, Spawner->m_fNextSpawnTime);
	}

	if (activatedNodes > 0) {
		Logger::Log("gametimer",
			"TickWaveNodes: activated=%d kickedFactories=%d round=%d time=%.2f\n",
			activatedNodes, kickedFactories, Game->s_nRoundNumber, Now);
	}

	LogCallEnd();
}
