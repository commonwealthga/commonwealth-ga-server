#include "src/GameServer/TgGame/TgGame_Control/SendCountdownRemainingMessages/TgGame_Control__SendCountdownRemainingMessages.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Control__SendCountdownRemainingMessages::Call(ATgGame_Control* Game, void* edx, float oldTimeRemaining, float newTimeRemaining) {
	LogCallBegin();

	// The native client/server build used localized combat-message ids for
	// threshold announcements here. The extracted UC only exposes the timing
	// hook, not those ids. Keep gameplay authoritative in
	// TickCountdownCalculation and avoid sending guessed message ids.
	const int oldWholeSeconds = (int)oldTimeRemaining;
	const int newWholeSeconds = (int)newTimeRemaining;
	if (oldWholeSeconds != newWholeSeconds &&
	    (newWholeSeconds == 30 || newWholeSeconds == 20 ||
	     newWholeSeconds == 10 || newWholeSeconds <= 5)) {
		Logger::Log("control",
			"SendCountdownRemainingMessages: countdown crossed %d seconds\n",
			newWholeSeconds);
	}

	LogCallEnd();
}
