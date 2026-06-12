#include "src/GameServer/TgGame/TgGame_Control/CalcDefenderReviveTime/TgGame_Control__CalcDefenderReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Control mode never shipped; its m_fObjectiveRespawnTimeAdjust skew is
// unknowable. Use the base TgGame calculation.
float __fastcall TgGame_Control__CalcDefenderReviveTime::Call(ATgGame_Control* Game, void* edx) {
	return TgGame__CalcDefenderReviveTime::Call((ATgGame*)Game, edx);
}
