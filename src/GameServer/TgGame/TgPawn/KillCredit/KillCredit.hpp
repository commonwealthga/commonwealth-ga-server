#pragma once
#include "src/pch.hpp"
#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// Who gets kill XP / loot / quest credit for a dead bot.
// Missions keep the retail rule (every player on the killer's task force).
// Open zones are shared persistent instances where every player is on the same
// task force, so team-wide credit paid the whole zone for every kill; there
// only the agent who landed the killing hit (APawn::LastHitBy) and that agent's
// squad mates in the same zone (ga_squad_members) are credited.
namespace KillCredit {
	inline bool InOpenZone() {
		static int cached = -1;  // map is fixed for the life of the instance
		if (cached < 0) cached = MapGameInfo::IsOpenZone(Config::GetMapNameChar()) ? 1 : 0;
		return cached == 1;
	}

	inline int64_t CharacterIdOf(APawn* pawn) {
		for (auto& kv : GClientConnectionsData) {
			if (kv.second.bClosed || (APawn*)kv.second.Pawn != pawn) continue;
			const auto* info = kv.second.pPlayerInfo ? kv.second.pPlayerInfo : &kv.second.PlayerInfo;
			return info->selected_character_id;
		}
		return 0;  // bot / pet killer
	}

	inline bool Eligible(ATgPawn* deadPawn, ATgPawn* playerPawn) {
		if (!InOpenZone()) return true;
		AController* killer = deadPawn ? deadPawn->LastHitBy : nullptr;
		if (killer == nullptr || playerPawn == nullptr || killer->Pawn == nullptr) return false;
		if (killer->Pawn == (APawn*)playerPawn) return true;
		return Database::SameSquad(CharacterIdOf(killer->Pawn), CharacterIdOf((APawn*)playerPawn));
	}

	// Bots from a factory whose map_object_config row `no_kill_reward` = 1 give no
	// kill XP and no loot roll; quest kill credit still counts. Unknown factory or a
	// controller already gone -> normal rewards.
	inline bool NoKillReward(ATgPawn* deadPawn) {
		AController* c = deadPawn ? deadPawn->Controller : nullptr;
		if (c == nullptr || !ObjectClassCache::ClassNameContains((UObject*)c, "TgAIController")) return false;
		ATgBotFactory* f = ((ATgAIController*)c)->m_pFactory;
		return f != nullptr && MapObjectConfig::GetInt(f->m_nMapObjectId, "no_kill_reward", 0) == 1;
	}
}
