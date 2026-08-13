#include "src/GameServer/TgGame/TgPlayerActions/SetSpawnTable/SetSpawnTable.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace TgPlayerActions::SetSpawnTableCmd {

namespace {

constexpr const char* kCommandName = "-setspawntable";

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == guid) {
			return kv.second.Pawn;
		}
	}
	return nullptr;
}

// Chat-line reply to the requesting player (SendText renders on the Instance
// channel, so the text stays readable while walking the map).
void Reply(const std::string& guid, const std::string& text) {
	ATgPawn_Character* Pawn = FindPawnBySessionGuid(guid);
	if (!Pawn || !Pawn->Controller) return;
	if (!ObjectClassCache::ClassNameContains(Pawn->Controller, "PlayerController")) return;

	APlayerController* PC = (APlayerController*)Pawn->Controller;
	if (!PC->Player) return;

	const std::wstring wmsg(text.begin(), text.end());  // ASCII-only text
	SendAlert::SendText((UNetConnection*)PC->Player, wmsg.c_str(), 1, 0, 8.0f);
}

void Audit(const std::string& guid, const std::string& outcome, const std::string& details) {
	IpcClient::SendChatCommandAudit(guid, kCommandName, outcome, details);
}

// Every bot factory in the map, including ATgBotFactorySpawnable.
ATgBotFactory* FindFactoryByMapObjectId(int mapObjectId) {
	const std::vector<UObject*> objs = ObjectCache::FindAllByClassSubstr("TgBotFactory");
	for (UObject* obj : objs) {
		ATgBotFactory* f = (ATgBotFactory*)obj;
		if (f != nullptr && f->m_nMapObjectId == mapObjectId) return f;
	}
	return nullptr;
}

} // namespace

void Execute(const std::string& session_guid, int map_object_id, int spawn_table_id) {
	if (!kEnabled) {
		Logger::Log("chat-command",
			"[ChatCmd][DLL] setspawntable guid=%s: disabled (kEnabled=false)\n",
			session_guid.c_str());
		Reply(session_guid, "-setspawntable is disabled");
		Audit(session_guid, "ignored", "disabled");
		return;
	}

	ATgBotFactory* Factory = FindFactoryByMapObjectId(map_object_id);
	if (Factory == nullptr) {
		Logger::Log("chat-command",
			"[ChatCmd][DLL] setspawntable guid=%s: no bot factory with mapObjectId=%d\n",
			session_guid.c_str(), map_object_id);
		Reply(session_guid, "No bot factory with mapObjectId " + std::to_string(map_object_id));
		Audit(session_guid, "ignored",
			"no factory mapObjectId=" + std::to_string(map_object_id));
		return;
	}

	const int oldTable = Factory->nSpawnTableId;
	const int aliveBefore = Factory->nCurrentCount;

	// Suicide the live roster first so what stands in the world afterwards came
	// from the NEW table only. KillBots(false) runs Pawn.Suicide(), so the
	// intact BotDied unwinds nCurrentCount / m_SpawnGroups against the OLD
	// groups before ResetQueue replaces them.
	Factory->eventKillBots(0);

	// Stamp both: ResetQueue(0) (encounter restart, alarm reset) resolves the
	// table from nDefaultSpawnTableId, so leaving it stale would revert the
	// override on the next external reset.
	Factory->nDefaultSpawnTableId = spawn_table_id;
	Factory->nSpawnTableId        = spawn_table_id;

	// Rebuild the queue from the new table — SpawnNextBot drains it on the
	// factory's next tick.
	TgBotFactory__ResetQueue::Call(Factory, nullptr, spawn_table_id);

	const float fx = Factory->Location.X;
	const float fy = Factory->Location.Y;
	const float fz = Factory->Location.Z;

	float dist = -1.0f;
	ATgPawn_Character* Pawn = FindPawnBySessionGuid(session_guid);
	if (Pawn != nullptr) {
		const float dx = Pawn->Location.X - fx;
		const float dy = Pawn->Location.Y - fy;
		const float dz = Pawn->Location.Z - fz;
		dist = std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	const char* rawName = Factory->GetName();
	const std::string factoryName(rawName ? rawName : "<null>");

	char buf[224];
	std::snprintf(buf, sizeof(buf),
		"Factory %d (%s): table %d -> %d, killed %d, queued %d @ X=%.0f Y=%.0f Z=%.0f d=%.0f",
		map_object_id, factoryName.c_str(), oldTable, spawn_table_id,
		aliveBefore, Factory->m_SpawnQueue.Num(), fx, fy, fz, dist);

	Logger::Log("chat-command",
		"[ChatCmd][DLL] setspawntable guid=%s: %s\n", session_guid.c_str(), buf);
	Reply(session_guid, buf);
	Audit(session_guid, "applied", buf);
}

} // namespace TgPlayerActions::SetSpawnTableCmd
