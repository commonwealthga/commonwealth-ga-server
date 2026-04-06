#include "src/GameServer/TgGame/TgGame_Defense/SendNewRoundMessage/TgGame_Defense__SendNewRoundMessage.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__SendNewRoundMessage::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
