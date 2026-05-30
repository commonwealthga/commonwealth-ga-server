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
			// Cache the class name once — same buffer is used by every
			// GetName/GetFullName call so we must copy before any second call.
			const char* clsRaw = Controller->Class ? Controller->Class->GetFullName() : nullptr;
			std::string clsName = clsRaw ? clsRaw : "NULL";
			if (Controller->Class == nullptr ||
			    clsName.find("PlayerController") == std::string::npos) {
				const char* ctrlRaw = Controller->GetName();
				std::string ctrlName = ctrlRaw ? ctrlRaw : "NULL";
				Logger::Log("revive",
					"  skipping[%d] %s (class=%s) — not a PlayerController\n",
					i, ctrlName.c_str(), clsName.c_str());
				continue;
			}

			ATgPlayerController* PC = (ATgPlayerController*)Controller;
			if (Logger::IsChannelEnabled("revive")) {
				const char* pcRaw = PC->GetName();
				std::string pcName = pcRaw ? pcRaw : "NULL";
				const char* pawnRaw = PC->Pawn ? PC->Pawn->GetName() : nullptr;
				std::string pawnName = pawnRaw ? pawnRaw : "NULL";
				Logger::Log("revive", "  Reviving attacker[%d] %s pawn=%s\n",
					i, pcName.c_str(), pawnName.c_str());

				if (PC->Pawn == nullptr) {
					Logger::Log("revive", "  WARNING: %s has NULL Pawn, eventRevive will likely fail\n", pcName.c_str());
				}
			}

			PC->eventRevive();
		}

		// Reset count to 0 instead of Clear() to avoid freeing the malloc'd buffer.
		Game->s_AttackerReviveList.Count = 0;
	}

	// Re-arm repeating timer for the next wave
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr && GRI->r_nSecsToAutoReleaseAttackers > 0) {
		Actor__SetTimer::SetTimer(Game, (float)GRI->r_nSecsToAutoReleaseAttackers, true, FName("ReviveAttackersTimer"), nullptr);
	}

	LogCallEnd();
}
