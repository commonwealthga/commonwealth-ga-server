#include "src/GameServer/TgGame/TgGame/SendMissionTimerEvent/TgGame__SendMissionTimerEvent.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TArray<int> TgGame__SendMissionTimerEvent::ActivateIndices;

void __fastcall TgGame__SendMissionTimerEvent::Call(ATgGame *Game, void *edx, int nEventId) {
	TArray<USequenceObject*> Events;
	TArray<int> Indices;

	Logger::Log("debug", "TgGame__SendMissionTimerEvent::Call - nEventId: %d\n", nEventId);

	Indices.Clear();
	Indices.Add(nEventId);

	if (!Globals::Get().GWorldInfo) {
		bool bFirstSkipped = false;
		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			if (UObject::GObjObjects()->Data[i]) {
				if (strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "WorldInfo TheWorld.PersistentLevel.WorldInfo") == 0) {
					if (!bFirstSkipped) {
						bFirstSkipped = true;
						continue;
					}

					Globals::Get().GWorldInfo = reinterpret_cast<AWorldInfo*>(UObject::GObjObjects()->Data[i]);
					break;
				}
			}
		}
	}

	AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;

	WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
		ClassPreloader::GetTgSeqEventMissionTimerClass(),
		1,
		&Events
	);

	for (int i = 0; i < Events.Num(); i++) {
		UTgSeqEvent_MissionTimer* Event = (UTgSeqEvent_MissionTimer*)Events.Data[i];
		Event->CheckActivate(Game, Game, 0, 0, Indices);
	}
	Logger::Log("debug", "TgGame__SendMissionTimerEvent::Call END\n", nEventId);
}

