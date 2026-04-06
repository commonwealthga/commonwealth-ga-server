#include "src/GameServer/TgGame/TgGame_Ticket/LoadGameConfig/TgGame_Ticket__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Ticket__LoadGameConfig::Call(ATgGame_Ticket* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
