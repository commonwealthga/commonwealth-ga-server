#include "src/GameServer/TgGame/TgGame_Ticket/TickTicketsCalculation/TgGame_Ticket__TickTicketsCalculation.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Ticket__TickTicketsCalculation::Call(ATgGame_Ticket* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
