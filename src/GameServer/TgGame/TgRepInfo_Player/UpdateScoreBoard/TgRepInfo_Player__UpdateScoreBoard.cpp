#include "src/GameServer/TgGame/TgRepInfo_Player/UpdateScoreBoard/TgRepInfo_Player__UpdateScoreBoard.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgRepInfo_Player__UpdateScoreBoard::Call(ATgRepInfo_Player* RepInfo, void* edx) {
	LogCallBegin();
	CallOriginal(RepInfo, edx);
	LogCallEnd();
}
