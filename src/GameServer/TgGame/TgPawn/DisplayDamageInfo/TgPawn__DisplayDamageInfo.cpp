#include "src/GameServer/TgGame/TgPawn/DisplayDamageInfo/TgPawn__DisplayDamageInfo.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__DisplayDamageInfo::Call(void* Pawn, void* edx, void* Canvas, float x, float y, float z) {
	int count = Pawn ? *(int*)((char*)Pawn + 0xFFC) : -1;
	Logger::Log("damage-info",
		"DisplayDamageInfo CALLED pawn=%p canvas=%p args=(%.1f,%.1f,%.1f) count=%d\n",
		Pawn, Canvas, x, y, z, count);
	CallOriginal(Pawn, edx, Canvas, x, y, z);
}
