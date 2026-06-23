#include "src/GameServer/TgGame/TgGame_Defense/TickWaveNodes/TgGame_Defense__TickWaveNodes.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ClearQueue/TgBotFactory__ClearQueue.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <set>

namespace {

// Last round we ran the park pass for. Single-threaded in-game state; harmless
// across matches (the pass is idempotent and only touches still-awake factories).
int s_lastParkedRound = -1;

// Defense spawn rosters are tuned for a full ~10-player lobby, and Defense
// factories ship with nActiveCount=0 (no concurrency cap) — so a woken factory
// bulk-dumps its entire roster. At low player counts that floods the map. Cap
// each woken factory's concurrent-alive count, scaled by player count, so small
// lobbies get proportionally fewer enemies on the field. This is wave-spawner
// level only: it writes the factory's existing nActiveCount brake (honored by
// SpawnNextBot) — no change to shared TgBotFactory spawn logic, so other game
// modes are unaffected. The per-player rate is chosen so a full lobby's cap
// exceeds any roster (≈ original unlimited feel) while solo stays light.
constexpr int kAlivePerPlayerPerFactory = 3;

int PlayerScaledActiveCap(int numPlayers) {
	const int players = numPlayers > 0 ? numPlayers : 1;
	return std::max(1, players * kAlivePerPlayerPerFactory);
}

// On a round change, stop factories that belong only to earlier rounds from
// self-spawning. A factory keeps spawning on its own SpawnNextBot/BotDied
// SetTimer loop once woken (bAutoSpawn=1) — nothing else clears it — so without
// this every prior round's factories accumulate. Parking = bAutoSpawn=0 (kills
// the self-reschedule + BotDied respawn) + ClearQueue (drop pending). We do NOT
// KillBots: already-spawned enemies persist by design (prior-wave pressure).
// Shared factories that also belong to the new round are kept awake.
void ParkStaleRoundFactories(ATgGame_Defense* Game, int curRound) {
	std::set<UObject*> keep;
	for (int i = 0; i < Game->s_WaveSpawnerList.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* sp = Game->s_WaveSpawnerList.Data[i];
		if (sp == nullptr || sp->m_nRoundNumber != curRound) continue;
		for (int t = 0; t < sp->Targets.Num(); t++) {
			if (sp->Targets.Data[t] != nullptr) keep.insert(sp->Targets.Data[t]);
		}
	}

	int parked = 0;
	for (int i = 0; i < Game->s_WaveSpawnerList.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* sp = Game->s_WaveSpawnerList.Data[i];
		if (sp == nullptr || sp->m_nRoundNumber == curRound) continue;
		for (int t = 0; t < sp->Targets.Num(); t++) {
			UObject* obj = sp->Targets.Data[t];
			if (obj == nullptr || keep.count(obj) > 0) continue;
			ATgBotFactory* Factory = (ATgBotFactory*)obj;
			if (!Factory->bAutoSpawn) continue;  // already parked
			Factory->bAutoSpawn = 0;
			TgBotFactory__ClearQueue::Call(Factory, nullptr);
			parked++;
		}
	}

	if (parked > 0) {
		Logger::Log("gametimer",
			"TickWaveNodes: round=%d parked %d factory(ies) from earlier rounds\n",
			curRound, parked);
	}
}

}  // namespace

void __fastcall TgGame_Defense__TickWaveNodes::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();

	if (Game == nullptr) {
		LogCallEnd();
		return;
	}

	if (Game->s_WaveSpawnerList.Num() == 0) {
		LogCallEnd();
		return;
	}

	// Round advanced: park factories that belong only to earlier rounds so they
	// stop pumping bots forever. Idempotent — runs once per round transition.
	if (Game->s_nRoundNumber != s_lastParkedRound) {
		s_lastParkedRound = Game->s_nRoundNumber;
		ParkStaleRoundFactories(Game, Game->s_nRoundNumber);
	}

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo == nullptr) {
		LogCallEnd();
		return;
	}

	const float Now = WorldInfo->TimeSeconds;
	int activatedNodes = 0;
	int kickedFactories = 0;

	for (int i = 0; i < Game->s_WaveSpawnerList.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* Spawner = Game->s_WaveSpawnerList.Data[i];
		// Logger::Log("gametimer", "Spawner try tick: m_nRoundNumber=%d m_fSpawnFrequency=%f m_fNextSpawnTime=%f\n", Spawner->m_nRoundNumber, Spawner->m_fSpawnFrequency, Spawner->m_fNextSpawnTime);
		if (Spawner == nullptr) continue;
		if (Spawner->m_nRoundNumber != Game->s_nRoundNumber) continue;
		if (Spawner->m_fNextSpawnTime > Now) continue;

		float Frequency = Spawner->m_fSpawnFrequency;
		if (Frequency <= 0.0f) Frequency = 1.0f;
		Spawner->m_fNextSpawnTime = Now + Frequency;

		int nodeFactories = 0;

		// int nRandomFactory = rand() % Spawner->Targets.Num();
		// UObject* Target = Spawner->Targets.Data[nRandomFactory];

		// Skip the whole pick loop if this spawner has no targets — otherwise
		// `rand() % Spawner->Targets.Num()` below is an integer divide-by-zero.
		const int targetCount = Spawner->Targets.Num();
		if (targetCount <= 0) continue;
		// float division — NumPlayers/10 in int math is 0 for any normal
		// lobby, pinning every tick to a single factory regardless of players.
		float percent = static_cast<float>(Game->NumPlayers) / 10.0f;
		int nMaxTargets = std::max(1, int(percent * targetCount));
		for (int j = 0; j < nMaxTargets; j++) {
			UObject* Target = Spawner->Targets.Data[rand() % targetCount];
			if (Target == nullptr || Target->Class == nullptr) continue;
			// IsA is unreliable on this build (feedback_no_isa_use_classname_strstr.md);
			// match by class-name string instead. "TgBotFactory" prefix catches
			// the class and any subclasses (e.g. TgBotFactory_*).
			const char* tcRaw = Target->Class->GetFullName();
			if (!tcRaw) continue;
			const std::string tcName = tcRaw;
			if (tcName.find("TgBotFactory") == std::string::npos) continue;

			ATgBotFactory* Factory = (ATgBotFactory*)Target;
			// Defense factories ship dormant (b_auto_spawn=0 override, v113) so
			// PostBeginPlay doesn't dump 55 rosters at map load. Wake the
			// factory UC-OnToggle-style; start a fresh wave roster only when
			// the previous one finished spawning — m_fSpawnFrequency then
			// paces wave STARTS, while alive enemies accumulate only if the
			// players can't kill fast enough (the intended losing pressure).
			Factory->bAutoSpawn = 1;
			// Player-scaled concurrent-alive cap (see PlayerScaledActiveCap) so
			// small lobbies aren't buried under full-lobby-sized rosters.
			// Factory->nActiveCount = PlayerScaledActiveCap(Game->NumPlayers);
			if (Factory->m_SpawnQueue.Num() == 0) {
				Factory->ResetQueue(0);
			}
			Factory->SpawnNextBot();
			nodeFactories++;
			kickedFactories++;
		}

		// Pulse the "Activated" output AND queue the op into its parent sequence.
		// ActivateOutputLink only sets the impulse; ExecuteActiveOps propagates it
		// only for ops in ActiveSequenceOps (mirrors USequenceEvent::ActivateEvent,
		// the working CheckActivate path). Without the queue the announce-VO chain
		// (juggernaut/dismantler/wave callouts) never fired.
		Spawner->ActivateOutputLink(0);
		USequence* ParentSeq = ((USequenceObject*)Spawner)->ParentSequence;
		if (ParentSeq != nullptr) {
			Spawner->bActive = 1;
			bool bAlreadyQueued = false;
			for (int q = 0; q < ParentSeq->ActiveSequenceOps.Num(); q++) {
				if (ParentSeq->ActiveSequenceOps.Data[q] == (USequenceOp*)Spawner) { bAlreadyQueued = true; break; }
			}
			if (!bAlreadyQueued) ParentSeq->ActiveSequenceOps.Add((USequenceOp*)Spawner);
		}
		activatedNodes++;

		Logger::Log("gametimer",
			"TickWaveNodes: round=%d activated spawner %s factories=%d next=%.2f\n",
			Game->s_nRoundNumber, Spawner->GetName(), nodeFactories, Spawner->m_fNextSpawnTime);
	}

	if (activatedNodes > 0) {
		Logger::Log("gametimer",
			"TickWaveNodes: activated=%d kickedFactories=%d round=%d time=%.2f\n",
			activatedNodes, kickedFactories, Game->s_nRoundNumber, Now);
	}

	LogCallEnd();
}
