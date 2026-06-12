#include "src/GameServer/TgGame/TgGame_Control/CalcAttackerReviveTime/TgGame_Control__CalcAttackerReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Control mode never shipped; its m_fObjectiveRespawnTimeAdjust skew is
// unknowable. Use the base TgGame calculation.
float __fastcall TgGame_Control__CalcAttackerReviveTime::Call(ATgGame_Control* Game, void* edx) {
	return TgGame__CalcAttackerReviveTime::Call((ATgGame*)Game, edx);
}
