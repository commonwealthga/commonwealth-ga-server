#include "src/GameServer/TgGame/TgGame/SendMissionTimerEvent/TgGame__SendMissionTimerEvent.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Maps/CtrRecursiveDoors/CtrRecursiveDoors.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TArray<int> TgGame__SendMissionTimerEvent::ActivateIndices;

void __fastcall TgGame__SendMissionTimerEvent::Call(ATgGame *Game, void *edx, int nEventId) {
	LogCallBegin();

	Logger::Log(GetLogChannel(), "nEventId: %d\n", nEventId);
	if (Logger::IsChannelEnabled("gametimer")) {
		const std::string gameName = ((UObject*)Game)->GetFullName();
		const std::string gameClass = Game->Class ? Game->Class->GetFullName() : "<null-class>";
		const std::string stateName = Game->GetStateName().GetName();
		ATgRepInfo_Game* GRI = Game->GameReplicationInfo ? (ATgRepInfo_Game*)Game->GameReplicationInfo : nullptr;
		Logger::Log("gametimer",
			"SendMissionTimerEvent(%d): game=%s class=%s state=%s timerState=%d mission=%.2f gameMission=%.2f "
			"GRI{round=%d/%d mtState=%d rem=%.2f remaining=%d limit=%d}\n",
			nEventId,
			gameName.c_str(),
			gameClass.c_str(),
			stateName.c_str(),
			(int)Game->m_eTimerState,
			Game->m_fMissionTime,
			Game->m_fGameMissionTime,
			GRI ? GRI->r_nRoundNumber : -1,
			GRI ? GRI->r_nMaxRoundNumber : -1,
			GRI ? (int)GRI->r_nMissionTimerState : -1,
			GRI ? GRI->r_fMissionRemainingTime : -1.0f,
			GRI ? GRI->RemainingTime : -1,
			GRI ? GRI->TimeLimit : -1);
	}

	TArray<USequenceObject*> Events;
	TArray<int> Indices;

	Indices.Clear();
	Indices.Add(nEventId);

	// if (!Globals::Get().GWorldInfo) {
	// 	bool bFirstSkipped = false;
	// 	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
	// 		if (UObject::GObjObjects()->Data[i]) {
	// 			if (strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "WorldInfo TheWorld.PersistentLevel.WorldInfo") == 0) {
	// 				if (!bFirstSkipped) {
	// 					bFirstSkipped = true;
	// 					continue;
	// 				}
	//
	// 				Globals::Get().GWorldInfo = reinterpret_cast<AWorldInfo*>(UObject::GObjObjects()->Data[i]);
	// 				break;
	// 			}
	// 		}
	// 	}
	// }

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	// AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;

	WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
		ClassPreloader::GetTgSeqEventMissionTimerClass(),
		1,
		&Events
	);

	for (int i = 0; i < Events.Num(); i++) {
		UTgSeqEvent_MissionTimer* Event = (UTgSeqEvent_MissionTimer*)Events.Data[i];
		Event->CheckActivate(Game, Game, 0, 0, Indices);
	}

	Logger::Log("gametimer", "SendMissionTimerEvent(%d): activated %d MissionTimer sequence events\n", nEventId, Events.Num());

	// MissionStarted (1): clear CTR_Recursive_P's unbound attacker spawn door,
	// matching when the defender door's matinee opens. Leaves were cached at
	// init (TgGame::InitGameRepInfo) so nothing scans here.
	if (nEventId == 1 && Config::GetMapNameChar() == std::string("CTR_Recursive_P")) {
		CtrRecursiveDoors::OpenAttackerSpawnDoor();
	}

	LogCallEnd();
}
