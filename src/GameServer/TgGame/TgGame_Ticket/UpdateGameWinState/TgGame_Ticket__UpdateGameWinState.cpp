#include "src/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Ticket__UpdateGameWinState::Call(ATgGame_Ticket* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
