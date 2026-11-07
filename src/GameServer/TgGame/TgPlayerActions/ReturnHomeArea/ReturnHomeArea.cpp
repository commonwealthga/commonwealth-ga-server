#include "src/GameServer/TgGame/TgPlayerActions/ReturnHomeArea/ReturnHomeArea.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

namespace TgPlayerActions::ReturnHomeAreaCmd {

namespace {

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == guid) return kv.second.Pawn;
	}
	return nullptr;
}

// Teleport the pawn to the start FindPlayerStart picks for its controller.
// Shared by the VR-arena return area and the player -stuck command.
bool MoveToPlayerStart(ATgPawn_Character* pawn, const std::string& session_guid) {

	ATgGame* game = reinterpret_cast<ATgGame*>(pawn->WorldInfo->Game);
	FString incomingName;
	ANavigationPoint* start = game->FindPlayerStart(pawn->Controller, 0, incomingName);
	if (!start) {
		Logger::Log("tcp",
			"[ReturnHomeArea] guid=%s: FindPlayerStart returned null\n",
			session_guid.c_str());
		return false;
	}

	FRotator newRot = pawn->Rotation;
	newRot.Pitch = 0;
	newRot.Yaw = start->Rotation.Yaw;
	newRot.Roll = 0;

	if (!pawn->SetLocation(start->Location)) {
		Logger::Log("tcp",
			"[ReturnHomeArea] guid=%s: SetLocation failed start=%s loc=(%.0f,%.0f,%.0f)\n",
			session_guid.c_str(), ((UObject*)start)->GetName(),
			start->Location.X, start->Location.Y, start->Location.Z);
		return false;
	}

	// Mirror TgTeleporter.UsePlayerStart so this is a game-thread move, not a
	// fake reconnect; live connection cleanup killed the home NetDriver.
	pawn->Velocity = FVector(0, 0, 0);
	pawn->SetRotation(newRot);
	pawn->SetViewRotation(newRot);
	pawn->ClientSetRotation(newRot);
	pawn->Controller->ClientSetRotation(newRot, 1);
	pawn->Controller->MoveTimer = -1.0f;
	pawn->SetAnchor(start);
	pawn->SetMoveTarget(start);
	pawn->PlayTeleportEffect(false, true);
	ActorCache::CacheMapActors();
	pawn->eventModifyPawnPropertiesVolumeChanged();
	pawn->eventOmegaVolumePropertiesChanged();
	pawn->bNetDirty = 1;
	pawn->bForceNetUpdate = 1;

	Logger::Log("tcp",
		"[ReturnHomeArea] guid=%s: moved to %s loc=(%.0f,%.0f,%.0f) yaw=%d\n",
		session_guid.c_str(), ((UObject*)start)->GetName(),
		start->Location.X, start->Location.Y, start->Location.Z, newRot.Yaw);
	return true;
}

bool PawnReady(ATgPawn_Character* pawn) {
	return pawn && pawn->Controller && pawn->WorldInfo && pawn->WorldInfo->Game;
}

void Tell(ATgPawn_Character* pawn, const char* text) {
	if (!pawn || !pawn->Controller) return;
	const char* raw = pawn->Controller->Class ? pawn->Controller->Class->GetFullName() : nullptr;
	if (!raw || std::string(raw).find("PlayerController") == std::string::npos) return;
	APlayerController* PC = (APlayerController*)pawn->Controller;
	if (!PC->Player) return;
	const std::wstring w(text, text + std::strlen(text));
	SendAlert::SendText((UNetConnection*)PC->Player, w.c_str(), 1, 0, 5.0f);
}

} // namespace

void Execute(const std::string& session_guid) {
	const std::string mapName = Config::GetMapNameChar();
	if (mapName != "Dome3_VR_Arena_P") {
		Logger::Log("tcp",
			"[ReturnHomeArea] guid=%s: current map %s has no return-area handler\n",
			session_guid.c_str(), mapName.c_str());
		return;
	}

	ATgPawn_Character* pawn = FindPawnBySessionGuid(session_guid);
	if (!PawnReady(pawn)) {
		Logger::Log("tcp",
			"[ReturnHomeArea] guid=%s: missing pawn/controller/world/game\n",
			session_guid.c_str());
		return;
	}
	MoveToPlayerStart(pawn, session_guid);
}

void ExecuteStuck(const std::string& session_guid, bool pvp) {
	ATgPawn_Character* pawn = FindPawnBySessionGuid(session_guid);
	if (!PawnReady(pawn) || pawn->Health <= 0) {
		Logger::Log("tcp", "[Stuck] guid=%s: no live pawn\n", session_guid.c_str());
		return;
	}
	if (pvp) {
		Logger::Log("tcp", "[Stuck] guid=%s: refused in PvP\n", session_guid.c_str());
		Tell(pawn, "-stuck is not available in PvP matches");
		return;
	}
	Logger::Log("tcp", "[Stuck] guid=%s map=%s\n", session_guid.c_str(), Config::GetMapNameChar());
	Tell(pawn, MoveToPlayerStart(pawn, session_guid) ? "Moved you to a safe spawn point"
	                                                 : "-stuck failed here - please send a -bug report");
}

} // namespace TgPlayerActions::ReturnHomeAreaCmd
