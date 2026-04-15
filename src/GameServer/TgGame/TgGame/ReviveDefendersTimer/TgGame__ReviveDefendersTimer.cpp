#include "src/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Revives all defenders in the wave revive queue and re-arms the repeating timer.
void __fastcall TgGame__ReviveDefendersTimer::Call(ATgGame *Game, void *edx) {
	LogCallBegin();

	if (Game->s_DefenderReviveList.Data != nullptr && Game->s_DefenderReviveList.Num() > 0) {
		int count = Game->s_DefenderReviveList.Num();
		Logger::Log("revive", "ReviveDefendersTimer: reviving %d defenders\n", count);
		for (int i = 0; i < count; i++) {
			ATgPlayerController* PC = (ATgPlayerController*)Game->s_DefenderReviveList.Data[i];
			if (PC == nullptr) continue;

			Logger::Log("revive", "  Reviving defender[%d] %s pawn=%s\n",
				i, PC->GetName(),
				PC->Pawn ? PC->Pawn->GetName() : "NULL");

			if (PC->Pawn == nullptr) {
				Logger::Log("revive", "  WARNING: %s has NULL Pawn, eventRevive will likely fail\n", PC->GetName());
			}

			PC->eventRevive();
		}

		// Reset count to 0 instead of Clear() to avoid freeing the malloc'd buffer.
		*(int*)((char*)Game + 0x428) = 0;  // s_DefenderReviveList.Count = 0
	}

	// Re-arm repeating timer for the next wave
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr && GRI->r_nSecsToAutoReleaseDefenders > 0) {
		Actor__SetTimer::SetTimer(Game, (float)GRI->r_nSecsToAutoReleaseDefenders, true, FName("ReviveDefendersTimer"), nullptr);
	}

	LogCallEnd();
}
