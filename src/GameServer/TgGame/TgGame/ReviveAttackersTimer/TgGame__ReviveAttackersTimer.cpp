#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Revives all attackers in the wave revive queue and re-arms the repeating timer.
void __fastcall TgGame__ReviveAttackersTimer::Call(ATgGame *Game, void *edx) {
	LogCallBegin();

	if (Game->s_AttackerReviveList.Data != nullptr && Game->s_AttackerReviveList.Num() > 0) {
		int count = Game->s_AttackerReviveList.Num();
		Logger::Log("revive", "ReviveAttackersTimer: reviving %d attackers\n", count);
		for (int i = 0; i < count; i++) {
			ATgPlayerController* PC = (ATgPlayerController*)Game->s_AttackerReviveList.Data[i];
			if (PC == nullptr) continue;

			Logger::Log("revive", "  Reviving attacker[%d] %s pawn=%s\n",
				i, PC->GetName(),
				PC->Pawn ? PC->Pawn->GetName() : "NULL");

			if (PC->Pawn == nullptr) {
				Logger::Log("revive", "  WARNING: %s has NULL Pawn, eventRevive will likely fail\n", PC->GetName());
			}

			PC->eventRevive();
		}

		// Reset count to 0 instead of Clear() to avoid freeing the malloc'd buffer.
		*(int*)((char*)Game + 0x41C) = 0;  // s_AttackerReviveList.Count = 0
	}

	// Re-arm repeating timer for the next wave
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr && GRI->r_nSecsToAutoReleaseAttackers > 0) {
		Actor__SetTimer::SetTimer(Game, (float)GRI->r_nSecsToAutoReleaseAttackers, true, FName("ReviveAttackersTimer"), nullptr);
	}

	LogCallEnd();
}
