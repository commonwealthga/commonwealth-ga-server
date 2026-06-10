#include "src/GameServer/TgGame/TgBotFactory/UseSpawnTable/TgBotFactory__UseSpawnTable.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Native is stripped. Semantics from the UC declaration + observed call
// sites: "rebuild the queue from the current nSpawnTableId." Delegate to
// ResetQueue with the current table id as the override so we reuse its
// queue+groups rebuild path.
void __fastcall TgBotFactory__UseSpawnTable::Call(ATgBotFactory* BotFactory, void* edx) {
	if (BotFactory == nullptr) return;
	Logger::Log("tgbotfactory",
		"[%s] %s UseSpawnTable mapObjectId=%d nSpawnTableId=%d\n",
		Logger::GetTime(), BotFactory->GetName(),
		BotFactory->m_nMapObjectId, BotFactory->nSpawnTableId);

	if (BotFactory->nSpawnTableId <= 0) {
		Logger::Log("tgbotfactory",
			"  UseSpawnTable called with nSpawnTableId=0 — nothing to do\n");
		return;
	}

	// "The table changed externally — rebuild now." ResetQueue rebuilds
	// unconditionally; pass the current id so a default-restore (ResetQueue 0
	// → nDefaultSpawnTableId) doesn't undo the external change.
	BotFactory->ResetQueue(BotFactory->nSpawnTableId);
}
