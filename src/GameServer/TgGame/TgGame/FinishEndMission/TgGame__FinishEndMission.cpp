#include "src/GameServer/TgGame/TgGame/FinishEndMission/TgGame__FinishEndMission.hpp"
#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native — fired by the 20s `SetTimer('FinishEndMission')` armed in
// BeginEndMission. Real engine implementation would route to the next map /
// return to lobby; Phase 1 leaves it as a log-only stub so we can verify the
// end-mission screen pipeline without coupling it to map travel. Players sit
// on the end-mission scene until they disconnect — fine for first test.
bool __fastcall TgGame__FinishEndMission::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	// Two-phase end flow: a deferred end screen consumes the first timer fire.
	if (ConsumePendingEndScreen(Game)) {
		LogCallEnd();
		return true;
	}
	Logger::Log("endmission",
		"FinishEndMission fired (no-op stub — map travel deferred to Phase 2)\n");
	LogCallEnd();
	return true;
}
