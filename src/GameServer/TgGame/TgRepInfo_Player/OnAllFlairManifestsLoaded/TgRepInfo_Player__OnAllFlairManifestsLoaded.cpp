#include "src/GameServer/TgGame/TgRepInfo_Player/OnAllFlairManifestsLoaded/TgRepInfo_Player__OnAllFlairManifestsLoaded.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgRepInfo_Player__OnAllFlairManifestsLoaded::Call(ATgRepInfo_Player* RepInfo, void* edx) {
	LogCallBegin();
	CallOriginal(RepInfo, edx);
	LogCallEnd();
}
