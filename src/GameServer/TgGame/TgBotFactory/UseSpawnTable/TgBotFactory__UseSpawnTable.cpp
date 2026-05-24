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

	// ResetQueue's override path is the rebuild we want. Pass current id so
	// the "override != current" guard doesn't short-circuit — we want a full
	// rebuild even if the id matches, because the caller is signalling "I
	// changed the table externally, rebuild now."
	const int currentTable = BotFactory->nSpawnTableId;
	BotFactory->nSpawnTableId = 0;       // force the != check to fire
	BotFactory->ResetQueue(currentTable);
}
