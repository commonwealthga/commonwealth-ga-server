#include "src/GameServer/TgGame/TgGame/UpdateMissionTimerEventWinVar/TgGame__UpdateMissionTimerEventWinVar.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__UpdateMissionTimerEventWinVar::Call(ATgGame* Game, void* edx) {
	LogCallBegin();

	if (Game == nullptr) {
		LogCallEnd();
		return;
	}

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo == nullptr || WorldInfo->GetGameSequence() == nullptr) {
		LogCallEnd();
		return;
	}

	const unsigned long bChallengerWon = (Game->m_GameWinState == 2) ? 1u : 0u;

	TArray<USequenceObject*> Events;
	WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
		ClassPreloader::GetTgSeqEventMissionTimerClass(),
		1,
		&Events
	);

	for (int i = 0; i < Events.Num(); i++) {
		UTgSeqEvent_MissionTimer* Event = (UTgSeqEvent_MissionTimer*)Events.Data[i];
		if (Event == nullptr) continue;
		Event->UpdateChallengerWonValue(bChallengerWon);
	}

	Logger::Log("gametimer",
		"UpdateMissionTimerEventWinVar: ChallengerWon=%lu from m_GameWinState=%u, events=%d\n",
		bChallengerWon, (unsigned)Game->m_GameWinState, Events.Num());

	LogCallEnd();
}
