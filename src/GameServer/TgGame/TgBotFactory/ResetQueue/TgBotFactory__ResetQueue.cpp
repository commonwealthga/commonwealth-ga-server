#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__ResetQueue::Call(ATgBotFactory *BotFactory, void *edx, int nOverrideSpawnTableId) {
	Logger::Log("tgbotfactory", "[%s] %s ResetQueue(%d) mapObjectId=%d\n", Logger::GetTime(), BotFactory->GetName(), nOverrideSpawnTableId, BotFactory->m_nMapObjectId);

	if (nOverrideSpawnTableId > 0) {
		BotFactory->nSpawnTableId = nOverrideSpawnTableId;
	}
	
	BotFactory->nCurrentCount = 0;

	BotFactory->s_nCurListIndex = -1;
	BotFactory->s_nCurLocationIndex = -1;
}

