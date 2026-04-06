#include "src/GameServer/TgGame/TgBotFactory/UseSpawnTable/TgBotFactory__UseSpawnTable.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__UseSpawnTable::Call(ATgBotFactory* Factory, void* edx) {
	LogCallBegin();
	CallOriginal(Factory, edx);
	LogCallEnd();
}
