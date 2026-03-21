#include "src/GameServer/TgGame/TgGame/SetObjectivesOvertimeNotify/TgGame__SetObjectivesOvertimeNotify.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called when entering overtime. Notifies all active objectives that overtime has begun.
// Objectives can use this to trigger Kismet events or state changes.
void __fastcall TgGame__SetObjectivesOvertimeNotify::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	// No-op for PointRotation: PointRotation never enters overtime
	// (UpdateShouldWait always sets m_bShouldWait=false in the base state).
	LogCallEnd();
}


