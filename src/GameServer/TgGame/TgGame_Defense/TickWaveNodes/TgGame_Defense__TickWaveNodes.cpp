#include "src/GameServer/TgGame/TgGame_Defense/TickWaveNodes/TgGame_Defense__TickWaveNodes.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstdlib>
#include <string>

// Vanilla wave model, reconstructed from the Raid_DomeCityDefense_P kismet:
// every m_fSpawnFrequency seconds a due node spawns the full roster of ONE
// factory from Targets (round-robin in array order) and pulses "Activated".
// Evidence:
//  - "Wave 4 Juggernaut": 3 factories, freq 80, VO per pulse -> one
//    juggernaut gate per pulse (t=0/80/160 of the 210s round), each
//    announced. All-targets-per-pulse would spawn 9 juggernauts; one bot
//    per pulse would spawn none (the juggernaut is the roster's last entry).
//  - "Wave 3 Juggernaut": its only Target is the map's dummy/trash factory
//    (13809); the real juggernauts come from the node's Activated chain
//    (SeqCond_Increment capped at 2 -> SeqAct_Toggle on factory 13653), so
//    wave 3 gets exactly two: pulse 1 (round start) and pulse 2 (+90s).
//  - "Wave 4 Minions" lists factory 13649 twice — duplicate Targets entries
//    weight the round-robin, which only makes sense one-per-pulse.
// No player-count scaling: retail defense had none; pacing comes from the
// node frequencies and per-difficulty spawn-table rosters.

std::map<void*, TgGame_Defense__TickWaveNodes::NodeState>
	TgGame_Defense__TickWaveNodes::s_nodeState;

void TgGame_Defense__TickWaveNodes::ResetNodeState() {
	s_nodeState.clear();
}

void TgGame_Defense__TickWaveNodes::SetNodeFrequency(void* Spawner, float fMin, float fMax) {
	NodeState& st = s_nodeState[Spawner];
	st.fMinFreq = fMin;
	st.fMaxFreq = fMax;
	st.nCursor  = 0;
}

namespace {

// Retail evaluated the node's "Spawn Frequency" SeqVar per read, so a
// SeqVar_RandomFloat link (e.g. wave-1 minions 11-18s) re-rolls every pulse.
float RollFrequency(UTgSeqAct_DefenseWaveSpawner* Spawner) {
	auto it = TgGame_Defense__TickWaveNodes::s_nodeState.find(Spawner);
	if (it != TgGame_Defense__TickWaveNodes::s_nodeState.end() &&
	    it->second.fMaxFreq > 0.0f) {
		const float lo = it->second.fMinFreq;
		const float hi = it->second.fMaxFreq;
		if (hi > lo) {
			return lo + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (hi - lo);
		}
		return hi;
	}
	return Spawner->m_fSpawnFrequency > 0.0f ? Spawner->m_fSpawnFrequency : 1.0f;
}

// TEMP DIAGNOSTIC (channel "bosshud") — Solar Farm: boss objective never shows
// on the client HUD even after it spawns. Dumps the two replicated feeds the
// client HUD can read (GRI r_Objectives + per-taskforce r_CurrActiveObjective)
// and each connection's confirmed-visible level list (UNetConnection+0x4F64,
// maintained by ServerUpdateLevelVisibility). If the Boss sublevel names never
// appear in a client's visible list, the engine can never map the objective
// refs for that connection (NetSerializeItem Mapped=false → StillDirty retry
// forever) and the HUD stays empty — streaming-sync problem, not HUD logic.
void DumpBossHudState(ATgGame_Defense* Game, float Now) {
	static float s_fNextDump = 0.0f;
	if (!Logger::IsChannelEnabled("bosshud") || Now < s_fNextDump) return;
	s_fNextDump = Now + 20.0f;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr) {
		for (int i = 0; i < 0x4B; i++) {
			ATgMissionObjective* O = GRI->r_Objectives[i];
			if (O == nullptr) continue;
			Logger::Log("bosshud",
				"r_Objectives[%d]: %s mid=%d prio=%d enabled=%d active=%d locked=%d status=%d\n",
				i, ObjectClassCache::GetClassName((UObject*)O).c_str(),
				O->m_nMapObjectId, O->nPriority,
				(int)O->bEnabled, (int)O->r_bIsActive, (int)O->r_bIsLocked,
				(int)O->r_eStatus);
		}
	}
	ATgMissionObjective* defGoal = GTeamsData.Defenders
		? GTeamsData.Defenders->r_CurrActiveObjective : nullptr;
	ATgMissionObjective* attGoal = GTeamsData.Attackers
		? GTeamsData.Attackers->r_CurrActiveObjective : nullptr;
	Logger::Log("bosshud", "r_CurrActiveObjective: defenders mid=%d attackers mid=%d\n",
		defGoal ? defGoal->m_nMapObjectId : -1,
		attGoal ? attGoal->m_nMapObjectId : -1);

	UWorld* World = (UWorld*)Globals::Get().GWorld;
	if (World != nullptr && World->NetDriver != nullptr) {
		for (int c = 0; c < World->NetDriver->ClientConnections.Num(); c++) {
			UNetConnection* Conn = World->NetDriver->ClientConnections.Data[c];
			if (Conn == nullptr || (uintptr_t)Conn < 0x10000) continue;
			// ClientVisibleLevelNames — TArray<FName> at +0x4F64 (offset from
			// the binary's ServerUpdateLevelVisibility impl, 0x10c7b150).
			TArray<FName>* Visible = (TArray<FName>*)((char*)Conn + 0x4F64);
			std::string names;
			for (int n = 0; n < Visible->Num(); n++) {
				const char* raw = Visible->Data[n].GetName();
				names += raw ? raw : "<null>";
				names += " ";
			}
			Logger::Log("bosshud", "conn[%d] visibleLevels(%d): %s\n",
				c, Visible->Num(), names.c_str());
		}
	}
}

}  // namespace

void __fastcall TgGame_Defense__TickWaveNodes::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();

	if (Game == nullptr || Game->s_WaveSpawnerList.Num() == 0) {
		LogCallEnd();
		return;
	}

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WorldInfo == nullptr) {
		LogCallEnd();
		return;
	}

	const float Now = WorldInfo->TimeSeconds;

	DumpBossHudState(Game, Now);

	for (int i = 0; i < Game->s_WaveSpawnerList.Num(); i++) {
		UTgSeqAct_DefenseWaveSpawner* Spawner = Game->s_WaveSpawnerList.Data[i];
		if (Spawner == nullptr) continue;
		if (Spawner->m_nRoundNumber != Game->s_nRoundNumber) continue;
		if (Spawner->m_fNextSpawnTime > Now) continue;

		const float Frequency = RollFrequency(Spawner);
		Spawner->m_fNextSpawnTime = Now + Frequency;

		// ONE factory per pulse, round-robin in Targets order (targets are
		// pre-filtered to TgBotFactory by CacheKismetConfiguration).
		ATgBotFactory* Factory = nullptr;
		const int targetCount = Spawner->Targets.Num();
		if (targetCount > 0 && Spawner->Targets.Data != nullptr) {
			int& cursor = s_nodeState[Spawner].nCursor;
			for (int probe = 0; probe < targetCount; probe++) {
				UObject* Target = Spawner->Targets.Data[cursor % targetCount];
				cursor = (cursor + 1) % targetCount;
				if (Target != nullptr) {
					Factory = (ATgBotFactory*)Target;
					break;
				}
			}
		}

		if (Factory != nullptr) {
			// Wake UC-OnToggle-style. Defense factories ship dormant
			// (b_auto_spawn=0 override, v113) so PostBeginPlay doesn't dump
			// rosters at map load. Roll a fresh roster only when the previous
			// one finished spawning — a factory hit again while still
			// draining just continues (never discard a pending juggernaut).
			Factory->bAutoSpawn = 1;
			if (Factory->m_SpawnQueue.Num() == 0) {
				Factory->ResetQueue(0);
			}
			Factory->SpawnNextBot();
		}

		// Pulse the "Activated" output AND queue the op into its parent
		// sequence. ActivateOutputLink only sets the impulse; ExecuteActiveOps
		// propagates it only for ops in ActiveSequenceOps (mirrors
		// USequenceEvent::ActivateEvent, the working CheckActivate path).
		// Downstream chains: juggernaut/dismantler/wave announce VO, the
		// wave-3 juggernaut counter->toggle, one-shot HUD warnings.
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

		// GetName() returns a shared static buffer — copy before the next call.
		const std::string spawnerName(Spawner->GetName());
		Logger::Log("gametimer",
			"TickWaveNodes: round=%d pulsed %s factory=%s(mid=%d) queue=%d next=+%.1fs\n",
			Game->s_nRoundNumber, spawnerName.c_str(),
			Factory != nullptr ? Factory->GetName() : "<none>",
			Factory != nullptr ? Factory->m_nMapObjectId : 0,
			Factory != nullptr ? Factory->m_SpawnQueue.Num() : 0,
			Frequency);
	}

	LogCallEnd();
}
