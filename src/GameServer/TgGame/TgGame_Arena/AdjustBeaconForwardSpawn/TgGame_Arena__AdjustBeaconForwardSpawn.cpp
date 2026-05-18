#include "src/GameServer/TgGame/TgGame_Arena/AdjustBeaconForwardSpawn/TgGame_Arena__AdjustBeaconForwardSpawn.hpp"
#include "src/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgGame_Arena overrides the AdjustBeaconForwardSpawn native; UC virtual
// dispatch from TgGame_PointRotation lands here (PointRotation has no
// further override). The original ATgGame_Arena native is stripped, so
// CallOriginal would no-op. Delegate to our TgGame::AdjustBeaconForwardSpawn
// reimpl so arena/point-rotation games get the same lifecycle behavior as
// the base ATgGame path. (Caller is a TgGame_Arena*, which IS-A ATgGame*.)
void __fastcall TgGame_Arena__AdjustBeaconForwardSpawn::Call(ATgGame_Arena* Game, void* edx, int nPriority) {
	LogCallBegin();
	TgGame__AdjustBeaconForwardSpawn::Call((ATgGame*)Game, edx, nPriority);
	LogCallEnd();
}
