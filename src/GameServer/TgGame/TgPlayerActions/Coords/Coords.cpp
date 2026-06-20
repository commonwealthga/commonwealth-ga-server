#include "src/GameServer/TgGame/TgPlayerActions/Coords/Coords.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace TgPlayerActions::CoordsCmd {

namespace {

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == guid) {
			return kv.second.Pawn;
		}
	}
	return nullptr;
}

} // namespace

void Execute(const std::string& session_guid) {
	ATgPawn_Character* pawn = FindPawnBySessionGuid(session_guid);
	if (!pawn) {
		Logger::Log("coords", "coords guid=%s: no pawn\n", session_guid.c_str());
		return;
	}

	const float x = pawn->Location.X;
	const float y = pawn->Location.Y;
	const float z = pawn->Location.Z;

	char buf[96];
	std::snprintf(buf, sizeof(buf), "X=%.0f  Y=%.0f  Z=%.0f", x, y, z);
	Logger::Log("coords", "coords guid=%s: %s\n", session_guid.c_str(), buf);

	// On-screen center alert — PlayerControllers only (bots have no connection).
	if (!pawn->Controller) return;
	const char* raw = pawn->Controller->Class ? pawn->Controller->Class->GetFullName() : nullptr;
	const std::string ctrlClass(raw ? raw : "");
	if (ctrlClass.find("PlayerController") == std::string::npos) return;

	APlayerController* PC = (APlayerController*)pawn->Controller;
	if (!PC->Player) return;
	UNetConnection* Connection = (UNetConnection*)PC->Player;

	// ASCII zero-extend widen (coords text is ASCII-only).
	const std::wstring wmsg(buf, buf + std::strlen(buf));
	// priority 1 = Normal, type 0 = Regular; 5s so there's time to read/record.
	SendAlert::SendText(Connection, wmsg.c_str(), 1, 0, 5.0f);
}

} // namespace TgPlayerActions::CoordsCmd
