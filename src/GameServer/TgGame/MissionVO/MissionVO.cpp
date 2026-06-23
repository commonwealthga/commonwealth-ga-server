#include "src/GameServer/TgGame/MissionVO/MissionVO.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

using MapDumpWriters::FNameStr;

int MissionVO::CauseEvent(const char* name) {
	if (name == nullptr) return 0;

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal(
		(UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo == nullptr || WorldInfo->GetGameSequence() == nullptr) {
		Logger::Log("missionvo", "CauseEvent('%s'): no game sequence\n", name);
		return 0;
	}

	// The console events are playerOnly=True, so CheckActivate's player-owned
	// gate rejects a non-player instigator (e.g. the game object). Use a real
	// PlayerController — mirrors ServerCauseEvent's (self=PC, Pawn). Any PC works;
	// the VO plays for everyone (SeqAct_PlaySound RPCs every client).
	AActor* instigator = nullptr;
	for (AController* C = WorldInfo->ControllerList; C != nullptr; C = C->NextController) {
		if (ObjectClassCache::ClassNameContains((UObject*)C, "PlayerController")) {
			instigator = (AActor*)C;
			break;
		}
	}
	if (instigator == nullptr) {
		Logger::Log("missionvo",
			"CauseEvent('%s'): no PlayerController instigator (no players) — skipped\n", name);
		return 0;
	}

	UClass* consoleEvtClass = ClassPreloader::GetClass("Class Engine.SeqEvent_Console");
	if (consoleEvtClass == nullptr) {
		Logger::Log("missionvo", "CauseEvent('%s'): SeqEvent_Console class not preloaded\n", name);
		return 0;
	}

	TArray<USequenceObject*> Events;
	WorldInfo->GetGameSequence()->FindSeqObjectsByClass(consoleEvtClass, 1, &Events);

	TArray<int> Indices;  // empty → activate all outputs (SeqEvent_Console has one)

	int fired = 0;
	for (int i = 0; i < Events.Num(); i++) {
		USeqEvent_Console* Evt = (USeqEvent_Console*)Events.Data[i];
		if (Evt == nullptr) continue;
		const char* en = FNameStr(Evt->ConsoleEventName);
		if (en == nullptr || strcmp(en, name) != 0) continue;
		Evt->CheckActivate(instigator, instigator, 0, 0, Indices);
		fired++;
	}

	Logger::Log("missionvo",
		"CauseEvent('%s'): scanned %d SeqEvent_Console, activated %d (instigator=%p)\n",
		name, Events.Num(), fired, (void*)instigator);
	return fired;
}
