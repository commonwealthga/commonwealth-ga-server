#include "src/GameServer/TgGame/TgPlayerController/ServerObama/TgPlayerController__ServerObama.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerObama::Call(ATgPlayerController* Controller, void* edx, int nCurrency, int nCharId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nCurrency, nCharId);
	LogCallEnd();
}
