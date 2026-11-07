#include "src/GameServer/TgGame/TgRepInfo_Game/CheckIsEnemy/TgRepInfo_Game__CheckIsEnemy.hpp"
#include "src/GameServer/Utils/BotSight/BotSight.hpp"

int __fastcall TgRepInfo_Game__CheckIsEnemy::Call(ATgRepInfo_Game* pThis, void* edx, AActor* a, AActor* b) {
	if (BotSight::IsHostilePair(a, b)) return 1;
	return CallOriginal(pThis, edx, a, b);
}

int __fastcall TgRepInfo_GameOpenWorld__CheckIsEnemy::Call(ATgRepInfo_Game* pThis, void* edx, AActor* a, AActor* b) {
	if (BotSight::IsHostilePair(a, b)) return 1;
	return CallOriginal(pThis, edx, a, b);
}
