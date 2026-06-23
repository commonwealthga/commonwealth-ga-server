#include "src/GameServer/TgGame/TgGame_Defense/UnlockObjective/TgGame_Defense__UnlockObjective.hpp"
#include "src/GameServer/TgGame/TgGame/UnlockObjective/TgGame__UnlockObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgGame_Defense overrides the stripped TgGame::UnlockObjective native. The
// prior reimpl unlocked EVERY registered objective regardless of nPriority,
// which broke the boss round: UC TgMissionObjective_Bot.UpdateObjectiveStatus(8)
// calls Game.UnlockObjective(nPriority+1) when the boss dies, and "unlock all"
// re-unlocked Bancroft (priority 1) → his objective re-initialised ("mission
// restarts"). Defense's objective progression is plain priority-matched
// unlocking — identical to the base game's — so delegate to the base
// reimplementation (priority filter + capture-only-once skip + per-priority bot
// factory activation + the bGameEnded bail that suppresses the stale boss-death
// re-unlock). No Defense-specific unlock behaviour is required; waves are driven
// separately by TickWaveNodes.
void __fastcall TgGame_Defense__UnlockObjective::Call(ATgGame_Defense* Game, void* edx, int nPriority) {
	LogCallBegin();
	TgGame__UnlockObjective::Call((ATgGame*)Game, edx, nPriority);
	LogCallEnd();
}
