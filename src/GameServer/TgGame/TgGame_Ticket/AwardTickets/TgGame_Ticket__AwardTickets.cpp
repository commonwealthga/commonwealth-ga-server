#include "src/GameServer/TgGame/TgGame_Ticket/AwardTickets/TgGame_Ticket__AwardTickets.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Ticket__AwardTickets::Call(ATgGame_Ticket* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
