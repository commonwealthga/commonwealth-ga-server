#include "src/GameServer/TgGame/TgAIController/SpawnPets/TgAIController__SpawnPets.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <map>

namespace {

// Seconds between pet spawns. Retail pacing lived in the stripped native
// body; 1s matches the spawner's spit-one-pet animation cadence.
constexpr float kPetSpawnInterval = 1.0f;

// Next allowed spawn time per spawner pawn, keyed by r_nPawnId.
std::map<int, float> s_nextSpawnTime;

}  // namespace

// Reimplementation of the stripped TgAIController::SpawnPets native.
//
// Flow (verified against the binary):
//   1. UC ExecuteAction case 1516 calls THIS native each think. Nothing else
//      ever calls InitSpawnPets — the action-record parser (FUN_1094c420)
//      stores alarm_bot_spawn_table_id at record+0x50 (m_pAction points at
//      the current record) and no intact native reads it, so the retail
//      SpawnPets body did the InitSpawnPets call itself.
//   2. InitSpawnPets (INTACT on RecursiveSpawner / WaspSpawner / Dismantler /
//      AttackTransport — byte-identical, class-agnostic copies; we call the
//      RecursiveSpawner copy directly) lazily spawns the pawn's personal
//      TgBotFactorySpawnable into s_PetBotFactory: ResetQueue(tableId) on
//      first creation AND on table change, LocationList from
//      m_DeployPetsSocketNames sockets (or own location), s_nTaskForce from
//      the pawn, Owner = pawn, m_bIgnoreCollisionOnSpawn=1.
//   3. We then drive the factory. ResetQueue builds rand[gmin..gmax] pending
//      entries (Tick Incubator: 3/5/7 by HP phase); each paced call here
//      drains one; the INTACT BotDied appends a timed replacement per pet
//      death — steady-state "keep N pets out".
void __fastcall TgAIController__SpawnPets::Call(ATgAIController* AIC, void* edx) {
	if (!AIC || !AIC->Pawn) return;
	if (!ObjectClassCache::ClassNameContains(AIC->Pawn, "TgPawn")) return;
	ATgPawn* pawn = (ATgPawn*)AIC->Pawn;

	void* action = (void*)AIC->m_pAction.Dummy;
	const int tableId = action ? *(int*)((char*)action + 0x50) : 0;

	if (tableId > 0) {
		const bool firstInit = (pawn->s_PetBotFactory == nullptr);
		// Intact InitSpawnPets template (RecursiveSpawner copy) — creates the
		// factory on first call, ResetQueue()s it when the table id changes.
		using InitSpawnPetsFn = void(__fastcall*)(ATgPawn*, void*, int);
		((InitSpawnPetsFn)0x109dcde0)(pawn, nullptr, tableId);
		if (firstInit) {
			Logger::Log("pet_spawn",
				"TgAIController::SpawnPets: pawn=0x%p InitSpawnPets(table=%d) -> factory=0x%p\n",
				(void*)pawn, tableId, (void*)pawn->s_PetBotFactory);
		}
	}

	ATgBotFactorySpawnable* factory = pawn->s_PetBotFactory;
	if (!factory) {
		Logger::Log("pet_spawn",
			"TgAIController::SpawnPets: pawn=0x%p no factory (action=0x%p table=%d) — skipping\n",
			(void*)pawn, action, tableId);
		return;
	}

	// This hook is the sole pacemaker for pet factories: SpawnNextBot skips
	// its SetTimer self-chain for TgBotFactorySpawnable, so pets emerge one
	// per interval (matching the spawner's spit animation) instead of the
	// whole group at once. Keyed by r_nPawnId, never by pointer.
	const float now = pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f;
	float& nextAt = s_nextSpawnTime[pawn->r_nPawnId];
	if (nextAt > now + kPetSpawnInterval) nextAt = 0.0f;  // stale (map travel reset TimeSeconds)
	if (now < nextAt) return;
	nextAt = now + kPetSpawnInterval;

	TgBotFactory__SpawnNextBot::Call((ATgBotFactory*)factory, nullptr);
}
