#include "src/GameServer/TgGame/TgPlayerActions/ReturnHomeArea/ReturnHomeArea.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::ReturnHomeAreaCmd {

namespace {

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == guid) return kv.second.Pawn;
	}
	return nullptr;
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
	if (!pawn || !pawn->Controller || !pawn->WorldInfo || !pawn->WorldInfo->Game) {
		Logger::Log("tcp",
			"[ReturnHomeArea] guid=%s: missing pawn/controller/world/game\n",
			session_guid.c_str());
		return;
	}

	ATgGame* game = reinterpret_cast<ATgGame*>(pawn->WorldInfo->Game);
	FString incomingName;
	ANavigationPoint* start = game->FindPlayerStart(pawn->Controller, 0, incomingName);
	if (!start) {
		Logger::Log("tcp",
			"[ReturnHomeArea] guid=%s: FindPlayerStart returned null\n",
			session_guid.c_str());
		return;
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
		return;
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
}

} // namespace TgPlayerActions::ReturnHomeAreaCmd
