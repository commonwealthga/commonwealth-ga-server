#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/Utils/Logger/Logger.hpp"

ANavigationPoint* __fastcall TgGame__TgFindPlayerStart::Call(ATgGame* Game, void* edx, AController* Controller, FName IncomingName) {
	Logger::Log("debug", "TgGame__TgFindPlayerStart::Call");

	// if (Controller == nullptr) {
	// 	return m_original(Game, edx, Controller, IncomingName);
	// }
	// ATgPlayerController* PlayerController = (ATgPlayerController*)Controller;
	// if (PlayerController->PlayerReplicationInfo == nullptr) {
	// 	return m_original(Game, edx, Controller, IncomingName);
	// }
	//
	// ATgRepInfo_Player* PlayerRep = (ATgRepInfo_Player*)PlayerController->PlayerReplicationInfo;
	// if (PlayerRep->r_TaskForce == nullptr) {
	// 	return m_original(Game, edx, Controller, IncomingName);
	// }

	for (int i = 0; i < UObject::GObjObjects()->Num(); i++) {
		UObject* obj = UObject::GObjObjects()->Data[i];

		if (obj && strcmp(obj->Class->GetFullName(), "Class TgGame.TgTeamPlayerStart") == 0) {
			ATgTeamPlayerStart* playerStart = (ATgTeamPlayerStart*)obj;
			playerStart->Location.Z += 2000;
			Logger::Log("debug", "Player start found");

			return playerStart;
		}
	}

	Logger::Log("debug", "Player start not found");

	return m_original(Game, edx, Controller, IncomingName);
}

