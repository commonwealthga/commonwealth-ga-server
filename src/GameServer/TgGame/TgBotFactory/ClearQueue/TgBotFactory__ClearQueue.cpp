#include "src/GameServer/TgGame/TgBotFactory/ClearQueue/TgBotFactory__ClearQueue.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native. UC caller: TgBotFactory.Despawn() (encounter wind-down:
// bAutoSpawn=false → ClearQueue() → KillBots(true)). Drops every pending
// spawn so a parked factory has nothing scheduled; the next StartEncounter /
// OnToggle / alarm calls ResetQueue to rebuild.
void __fastcall TgBotFactory__ClearQueue::Call(ATgBotFactory* Factory, void* edx) {
	if (Factory == nullptr) return;
	Logger::Log("tgbotfactory",
		"[%s] %s ClearQueue mapObjectId=%d (dropping %d pending entries)\n",
		Logger::GetTime(), Factory->GetName(), Factory->m_nMapObjectId,
		Factory->m_SpawnQueue.Num());
	Factory->m_SpawnQueue.Clear();
}
