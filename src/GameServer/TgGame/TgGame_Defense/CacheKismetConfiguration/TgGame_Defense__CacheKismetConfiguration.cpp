#include "src/GameServer/TgGame/TgGame_Defense/CacheKismetConfiguration/TgGame_Defense__CacheKismetConfiguration.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstdlib>
#include <string>

// Build the wave-spawner roster straight from each TgSeqAct_DefenseWaveSpawner's
// kismet variable links — the canonical source the stripped native read:
//   "Round Number"    → SeqVar_Int.IntValue          → m_nRoundNumber
//   "Spawn Frequency" → SeqVar_RandomFloat[/Float]    → m_fSpawnFrequency
//   "Bot Factories"   → one SeqVar_Object per factory → Targets
// No hardcoded per-map tables and no reliance on FindSeqObjectsByClass ordering,
// so this works on every defense map. TickWaveNodes then gates by round and
// drains the Target factories.
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

	for (int i = 0; i < Nodes.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* Spawner = (UTgSeqAct_DefenseWaveSpawner*)Nodes.Data[i];
		if (Spawner == nullptr) continue;

		int   round        = 0;
		float frequency    = 0.0f;
		int   factoryCount = 0;

		Spawner->Targets.Clear();

		for (int v = 0; v < Spawner->VariableLinks.Num(); v++) {
			const FSeqVarLink& vl = Spawner->VariableLinks.Data[v];

			char desc[64] = {0};
			if (vl.LinkDesc.Data && vl.LinkDesc.Count > 0) {
				wcstombs(desc, vl.LinkDesc.Data, sizeof(desc) - 1);
			}
			const std::string d(desc);

			if (d == "Round Number") {
				for (int k = 0; k < vl.LinkedVariables.Num(); k++) {
					USeqVar_Int* iv = (USeqVar_Int*)vl.LinkedVariables.Data[k];
					if (iv != nullptr) { round = iv->IntValue; break; }
				}
			} else if (d == "Spawn Frequency") {
				for (int k = 0; k < vl.LinkedVariables.Num(); k++) {
					USequenceVariable* var = vl.LinkedVariables.Data[k];
					if (var == nullptr) continue;
					float f = ((USeqVar_Float*)var)->FloatValue;
					// RandomFloat publishes into FloatValue only when evaluated;
					// at cache time it's usually 0, so derive from [Min,Max].
					if (f <= 0.0f && ObjectClassCache::ClassNameContains(var, "SeqVar_RandomFloat")) {
						USeqVar_RandomFloat* rf = (USeqVar_RandomFloat*)var;
						if (rf->Max > 0.0f)      f = (rf->Min + rf->Max) * 0.5f;
						else if (rf->Min > 0.0f) f = rf->Min;
					}
					frequency = f;
					break;
				}
			} else if (d == "Bot Factories") {
				for (int k = 0; k < vl.LinkedVariables.Num(); k++) {
					USeqVar_Object* ov = (USeqVar_Object*)vl.LinkedVariables.Data[k];
					if (ov == nullptr || ov->ObjValue == nullptr) continue;
					UObject* obj = ov->ObjValue;
					if (obj->Class == nullptr) continue;
					// IsA is unreliable on this build — match by class-name string.
					if (!ObjectClassCache::ClassNameContains(obj, "TgBotFactory")) continue;
					Spawner->Targets.Add(obj);
					factoryCount++;
				}
			}
		}

		Spawner->m_nRoundNumber    = round;
		Spawner->m_fSpawnFrequency = frequency;
		Spawner->m_fNextSpawnTime  = 0.0f;

		Game->s_WaveSpawnerList.Add(Spawner);

		Logger::Log("gametimer",
			"Spawner cached (varlink): round=%d freq=%.2f factories=%d\n",
			round, frequency, factoryCount);
	}

	Logger::Log("gametimer",
		"CacheKismetConfiguration: cached %d defense wave spawner nodes (varlink-driven)\n",
		Game->s_WaveSpawnerList.Num());

	LogCallEnd();
}
