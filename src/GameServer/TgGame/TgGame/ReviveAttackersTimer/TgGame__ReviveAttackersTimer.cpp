#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Revives everyone on one side's wave list, then re-arms the wave timer with
// a freshly computed interval (CalcAttacker/DefenderReviveTime).
void TgGame__ReviveAttackersTimer::RunWave(ATgGame* Game, bool bAttackers) {
	TArray<AController*>& List = bAttackers ? Game->s_AttackerReviveList : Game->s_DefenderReviveList;
	const char* side = bAttackers ? "attacker" : "defender";

	// Snapshot + clear before dispatching: Dead.Revive calls
	// UnRegisterForWaveRevive, which would mutate the list mid-iteration.
	std::vector<AController*> wave;
	for (int i = 0; i < List.Num(); i++) {
		if (List.Data[i] != nullptr) wave.push_back(List.Data[i]);
	}
	List.Count = 0;

	if (!wave.empty()) {
		Logger::Log("revive", "RunWave: reviving %d %ss\n", (int)wave.size(), side);
	}

	for (AController* Controller : wave) {
		if (Controller->bDeleteMe) {
			Logger::Log("revive", "  skipping deleted controller %p\n", Controller);
			continue;
		}
		// Drop anyone no longer in Dead (revived elsewhere, match over, …).
		if (strcmp(Controller->GetStateName().GetName(), "Dead") != 0) {
			if (Logger::IsChannelEnabled("revive")) {
				const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
				Logger::Log("revive", "  skipping %s — not in state Dead\n", ctrlName.c_str());
			}
			continue;
		}
		if (Controller->Pawn == nullptr) {
			if (Logger::IsChannelEnabled("revive")) {
				const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
				Logger::Log("revive", "  skipping %s — no Pawn\n", ctrlName.c_str());
			}
			continue;
		}

		if (Logger::IsChannelEnabled("revive")) {
			const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
			const std::string pawnName(Controller->Pawn->GetName() ? Controller->Pawn->GetName() : "<null>");
			Logger::Log("revive", "  reviving %s %s pawn=%s\n", side, ctrlName.c_str(), pawnName.c_str());
		}

		if (ObjectClassCache::ClassNameContains(Controller, "PlayerController")) {
			((ATgPlayerController*)Controller)->eventRevive();
		} else if (ObjectClassCache::ClassNameContains(Controller, "TgAIController")) {
			// SDK ATgAIController::eventRevive resolves the empty class-level
			// Revive; the real body is the Dead-state override.
			static UFunction* pFnAIRevive = nullptr;
			if (pFnAIRevive == nullptr) {
				pFnAIRevive = (UFunction*)ObjectCache::Find("Function TgAIController.Dead.Revive");
			}
			if (pFnAIRevive != nullptr) {
				ATgAIController_eventRevive_Parms Parms;
				Controller->ProcessEvent(pFnAIRevive, &Parms, nullptr);
			} else {
				Logger::Log("revive", "  ERROR: Function TgAIController.Dead.Revive not found\n");
			}
		} else {
			if (Logger::IsChannelEnabled("revive")) {
				const std::string clsName = ObjectClassCache::GetClassName((UObject*)Controller);
				Logger::Log("revive", "  skipping unsupported controller class %s\n", clsName.c_str());
			}
		}
	}

	// Re-arm the wave unless this mode runs custom (round-based) revive timing
	// (plain TgGame_Arena: revives happen at round boundaries via direct calls).
	if (Game->s_UseCustomReviveTimer) {
		return;
	}

	float next = bAttackers
		? TgGame__CalcAttackerReviveTime::Call(Game, nullptr)
		: TgGame__CalcDefenderReviveTime::Call(Game, nullptr);
	if (next <= 0.0f) {
		Logger::Log("revive", "RunWave: %s interval <= 0, wave timer not re-armed\n", side);
		return;
	}

	// Keep the replicated interval in sync — repnotify drives the client's
	// looping HUD respawn timer.
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr) {
		if (bAttackers) {
			GRI->r_nSecsToAutoReleaseAttackers = (int)next;
		} else {
			GRI->r_nSecsToAutoReleaseDefenders = (int)next;
		}
	}

	Actor__SetTimer::SetTimer(Game, next, false,
		bAttackers ? FName("ReviveAttackersTimer") : FName("ReviveDefendersTimer"), nullptr);
}

void __fastcall TgGame__ReviveAttackersTimer::Call(ATgGame *Game, void *edx) {
	LogCallBegin();
	RunWave(Game, true);
	LogCallEnd();
}
