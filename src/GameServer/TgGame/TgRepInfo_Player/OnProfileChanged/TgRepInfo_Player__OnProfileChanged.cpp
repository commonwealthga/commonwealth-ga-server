#include "src/GameServer/TgGame/TgRepInfo_Player/OnProfileChanged/TgRepInfo_Player__OnProfileChanged.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgRepInfo_Player__OnProfileChanged::Call(ATgRepInfo_Player* RepInfo, void* edx) {
	LogCallBegin();
	CallOriginal(RepInfo, edx);
	LogCallEnd();
}
