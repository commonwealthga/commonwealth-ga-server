#include "src/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.hpp"

float __fastcall TgGame__MissionTimeRemaining::Call(ATgGame *Game, void *edx) {
	return 240.0f - Game->s_MissionTimeAccumulator;
}

