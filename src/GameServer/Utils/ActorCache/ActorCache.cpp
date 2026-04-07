#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool ActorCache::bCached = false;
std::vector<ATgTeamPlayerStart*> ActorCache::PlayerStarts;
std::vector<ATgMissionObjective*> ActorCache::MissionObjectives;
std::vector<ATgRandomSMActor*> ActorCache::RandomSMActors;
ATgBotFactory* ActorCache::BotFactory = nullptr;

void ActorCache::CacheMapActors() {
	if (bCached) return;
	bCached = true;

	for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgTeamPlayerStart")) {
		PlayerStarts.push_back((ATgTeamPlayerStart*)obj);
	}

	// Use substr match to catch TgMissionObjective and all subclasses (_Proximity, _Escort, _Bot, etc.)
	for (UObject* obj : ObjectCache::FindAllByClassSubstr("Class TgGame.TgMissionObjective")) {
		MissionObjectives.push_back((ATgMissionObjective*)obj);
	}

	for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgRandomSMActor")) {
		RandomSMActors.push_back((ATgRandomSMActor*)obj);
	}

	const auto& factories = ObjectCache::FindAllByClass("Class TgGame.TgBotFactory");
	if (!factories.empty()) {
		BotFactory = (ATgBotFactory*)factories[0];
	}

	Logger::Log("debug", "ActorCache: %d PlayerStarts, %d MissionObjectives, %d RandomSMActors, BotFactory=%p\n",
		(int)PlayerStarts.size(), (int)MissionObjectives.size(), (int)RandomSMActors.size(), BotFactory);
}
