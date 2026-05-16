#include "src/GameServer/TgGame/TgGame_Ticket/AwardTickets/TgGame_Ticket__AwardTickets.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native. UC never calls this on the simple-Ticket path — all
// ticket accumulation happens directly in TickTicketsCalculation. Kept
// hooked as a passthrough so any binary caller (DualCTF derives from
// Ticket and may invoke it on flag capture) lands cleanly instead of in
// the stub.
void __fastcall TgGame_Ticket__AwardTickets::Call(ATgGame_Ticket* Game, void* edx) {
	if (Logger::IsChannelEnabled(GetLogChannel())) {
		Logger::Log(GetLogChannel(), "AwardTickets — called (no-op; work is in TickTicketsCalculation)\n");
	}
}
