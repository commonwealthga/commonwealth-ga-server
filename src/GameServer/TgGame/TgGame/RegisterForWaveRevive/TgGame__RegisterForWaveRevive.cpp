#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/SDK/SdkHeaders.h"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__RegisterForWaveRevive::Call(ATgGame *Game, void *edx, AController *Controller) {
	if (Controller == nullptr) {
		return;
	}

	// Players always; AI only when henchman-flagged (mirrors the gate in
	// TgAIController.Dead.ReviveTimer). Deployed turrets/pets respawn through
	// their own lifecycle, not the wave.
	bool isPlayer = ObjectClassCache::ClassNameContains(Controller, "PlayerController");
	if (!isPlayer) {
		ATgPawn* Pawn = (ATgPawn*)Controller->Pawn;
		bool isHenchman = ObjectClassCache::ClassNameContains(Controller, "TgAIController")
			&& Pawn != nullptr && ObjectClassCache::ClassNameContains(Pawn, "TgPawn")
			&& Pawn->r_bIsHenchman;
		if (!isHenchman) {
			if (Logger::IsChannelEnabled("revive")) {
				const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
				const std::string clsName = ObjectClassCache::GetClassName((UObject*)Controller);
				Logger::Log("revive",
					"RegisterForWaveRevive: rejecting %s (class=%s) — not a player or henchman\n",
					ctrlName.c_str(), clsName.c_str());
			}
			return;
		}
	}

	ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)Controller->PlayerReplicationInfo;
	if (RepInfo == nullptr || RepInfo->r_TaskForce == nullptr) {
		if (Logger::IsChannelEnabled("revive")) {
			const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
			Logger::Log("revive", "RegisterForWaveRevive: %s has no PRI/TaskForce — skipping\n",
				ctrlName.c_str());
		}
		return;
	}
	bool isAttacker = RepInfo->r_TaskForce->IsAttacker();

	TArray<AController*>& List = isAttacker ? Game->s_AttackerReviveList : Game->s_DefenderReviveList;
	for (int i = 0; i < List.Num(); i++) {
		if (List.Data[i] == Controller) {
			return; // already queued for this wave
		}
	}
	List.Add(Controller);

	if (Logger::IsChannelEnabled("revive")) {
		const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
		Logger::Log("revive", "RegisterForWaveRevive: %s -> %sList (count=%d)\n",
			ctrlName.c_str(), isAttacker ? "Attacker" : "Defender", List.Count);
	}

	// Safety net: PostBeginPlay arms the wave timers, but if this side's timer
	// isn't running, arm it. Skipped in custom (round-based) revive modes.
	if (Game->s_UseCustomReviveTimer) {
		return;
	}
	FName TimerName = isAttacker ? FName("ReviveAttackersTimer") : FName("ReviveDefendersTimer");
	float remaining = Game->GetRemainingTimeForTimer(TimerName, nullptr);
	if (remaining < 0.0f) {
		float next = isAttacker
			? TgGame__CalcAttackerReviveTime::Call(Game, nullptr)
			: TgGame__CalcDefenderReviveTime::Call(Game, nullptr);
		if (next > 0.0f) {
			Logger::Log("revive", "%s wave timer not running — arming (%.0f sec)\n",
				isAttacker ? "attacker" : "defender", next);
			ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
			if (GRI != nullptr) {
				if (isAttacker) {
					GRI->r_nSecsToAutoReleaseAttackers = (int)next;
				} else {
					GRI->r_nSecsToAutoReleaseDefenders = (int)next;
				}
			}
			Actor__SetTimer::SetTimer(Game, next, false, TimerName, nullptr);
		}
	}
}
