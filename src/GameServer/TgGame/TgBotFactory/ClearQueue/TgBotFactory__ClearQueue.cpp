#include "src/GameServer/TgGame/TgBotFactory/ClearQueue/TgBotFactory__ClearQueue.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__ClearQueue::Call(ATgBotFactory* Factory, void* edx) {
	LogCallBegin();
	CallOriginal(Factory, edx);
	LogCallEnd();
}
