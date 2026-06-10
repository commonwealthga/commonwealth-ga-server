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

		// Skip the whole pick loop if this spawner has no targets — otherwise
		// `rand() % Spawner->Targets.Num()` below is an integer divide-by-zero.
		const int targetCount = Spawner->Targets.Num();
		if (targetCount <= 0) continue;
		// float division — NumPlayers/10 in int math is 0 for any normal
		// lobby, pinning every tick to a single factory regardless of players.
		float percent = static_cast<float>(Game->NumPlayers) / 10.0f;
		int nMaxTargets = std::max(1, int(percent * targetCount));
		for (int j = 0; j < nMaxTargets; j++) {
			UObject* Target = Spawner->Targets.Data[rand() % targetCount];
			if (Target == nullptr || Target->Class == nullptr) continue;
			// IsA is unreliable on this build (feedback_no_isa_use_classname_strstr.md);
			// match by class-name string instead. "TgBotFactory" prefix catches
			// the class and any subclasses (e.g. TgBotFactory_*).
			const char* tcRaw = Target->Class->GetFullName();
			if (!tcRaw) continue;
			const std::string tcName = tcRaw;
			if (tcName.find("TgBotFactory") == std::string::npos) continue;

			ATgBotFactory* Factory = (ATgBotFactory*)Target;
			// Defense factories ship dormant (b_auto_spawn=0 override, v113) so
			// PostBeginPlay doesn't dump 55 rosters at map load. Wake the
			// factory UC-OnToggle-style; start a fresh wave roster only when
			// the previous one finished spawning — m_fSpawnFrequency then
			// paces wave STARTS, while alive enemies accumulate only if the
			// players can't kill fast enough (the intended losing pressure).
			Factory->bAutoSpawn = 1;
			if (Factory->m_SpawnQueue.Num() == 0) {
				Factory->ResetQueue(0);
			}
			Factory->SpawnNextBot();
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
