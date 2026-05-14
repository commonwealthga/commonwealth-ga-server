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
			AController* Controller = Game->s_AttackerReviveList.Data[i];
			if (Controller == nullptr) continue;

			// Defensive: reject anything that isn't a player controller. The
			// SDK's `eventRevive()` accessor uses TgPlayerController's UFunction
			// table, so calling it on a TgAIController dispatches the wrong
			// `Revive` (TgPlayerController.Dead.Revive instead of
			// TgAIController.Dead.Revive) — player-revive logic then runs on a
			// bot/turret context and crashes. RegisterForWaveRevive already
			// filters these out, but if some other code path ever pushes an
			// AIController onto the list this guard catches it.
			if (Controller->Class == nullptr ||
			    strstr(Controller->Class->GetFullName(), "PlayerController") == nullptr) {
				Logger::Log("revive",
					"  skipping[%d] %s (class=%s) — not a PlayerController\n",
					i, Controller->GetName(),
					Controller->Class ? Controller->Class->GetFullName() : "NULL");
				continue;
			}

			ATgPlayerController* PC = (ATgPlayerController*)Controller;
			if (Logger::IsChannelEnabled("revive")) {
				Logger::Log("revive", "  Reviving attacker[%d] %s pawn=%s\n",
					i, PC->GetName(),
					PC->Pawn ? PC->Pawn->GetName() : "NULL");

				if (PC->Pawn == nullptr) {
					Logger::Log("revive", "  WARNING: %s has NULL Pawn, eventRevive will likely fail\n", PC->GetName());
				}
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
