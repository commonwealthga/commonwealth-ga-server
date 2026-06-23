#include "src/GameServer/TgGame/TgGame/UnlockObjective/TgGame__UnlockObjective.hpp"
#include "src/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>
#include <functional>
#include <set>

// Called with nPriority=1 at PostBeginPlay and from TgGame_Arena::ObjectiveUnlock.
// Unlocks and activates all objectives matching the given priority.
// Delegates to eventUnlockObjective which handles: r_bIsLocked, SetObjectiveActive,
// ForceNetRelevant, and TriggerEventClass(TgSeqEvent_ObjectiveStatusChanged, 4).
//
// Skips objectives that are flagged `m_bCaptureOnlyOnce` and have already been
// captured once (`r_bHasBeenCapturedOnce`). Without this gate, the UC path
// in TgMissionObjective::UpdateObjectiveStatus:522-525 re-unlocks the previous
// objective on a status-7 transition (defender pushed attacker capture back
// to zero on the CURRENT point), which incorrectly reactivates already-finished
// permanent objectives in TgGame_Mission-style sequential capture missions.
void __fastcall TgGame__UnlockObjective::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();

	// No objective unlocks once the match is over. The Defense boss/escort end
	// path runs UC TgMissionObjective_Bot.UpdateObjectiveStatus(8) →
	// Game.UnlockObjective(nPriority+1) AFTER CheckWinGame has already fired
	// BeginEndMission (bGameEnded=1); without this gate that re-unlocks the
	// next-priority objective (Bancroft) when the boss dies → "mission restarts".
	if (Game != nullptr && Game->bGameEnded) {
		Logger::Log(GetLogChannel(),
			"UnlockObjective(%d): match over (bGameEnded) — suppressing unlock\n", nPriority);
		LogCallEnd();
		return;
	}

	// One-shot dump of the alarm-event registry built by BuildAlarmsArray
	// at TgGame.BeginPlay. By the time UnlockObjective(1) fires from
	// BeginPlay this is already populated. Logs per-event Originator,
	// trigger limits, output-link target counts and the linked op names so
	// we can see what Kismet has wired to each alarm event.
	// static bool s_alarmDumpDone = false;
	// if (!s_alarmDumpDone) {
	// 	s_alarmDumpDone = true;
	// 	Logger::Log("alarm",
	// 		"=== s_SeqEventAlarmBots dump (events=%d, acts=%d, actorFactories=%d) ===\n",
	// 		Game->s_SeqEventAlarmBots.Count, Game->s_SeqActAlarmBots.Count,
	// 		Game->s_ActorFactories.Count);
	//
	// 	// Recursive Kismet chain walker. Indented by depth; capped at depth
	// 	// 4 to avoid runaway with cyclic graphs. Prints LinkedOp's full name,
	// 	// class, and the LinkDesc each output is labelled with (so SeqAct_Delay
	// 	// shows "Finished" → next op, etc.). Cycle guard via a visited set.
	// 	std::set<USequenceOp*> visited;
	// 	std::function<void(USequenceOp*, int)> walk = [&](USequenceOp* op, int depth) {
	// 		if (op == nullptr || depth > 4) return;
	// 		if (!visited.insert(op).second) {
	// 			std::string ind(depth * 2, ' ');
	// 			Logger::Log("alarm", "%s(already visited)\n", ind.c_str());
	// 			return;
	// 		}
	// 		std::string ind(depth * 2, ' ');
	// 		for (int j = 0; j < op->OutputLinks.Count; j++) {
	// 			const FSeqOpOutputLink& ol = op->OutputLinks.Data[j];
	// 			Logger::Log("alarm", "%sout[%d] '%s' linkedTargets=%d\n",
	// 				ind.c_str(), j,
	// 				ol.LinkDesc.Data ? ol.LinkDesc.Data : "", ol.Links.Count);
	// 			for (int k = 0; k < ol.Links.Count; k++) {
	// 				USequenceOp* target = ol.Links.Data[k].LinkedOp;
	// 				if (target == nullptr) {
	// 					Logger::Log("alarm", "%s  link[%d] <null>\n", ind.c_str(), k);
	// 					continue;
	// 				}
	// 				const char* tnRaw = target->GetFullName();
	// 				const std::string tn(tnRaw ? tnRaw : "<null>");
	// 				const char* tcRaw = (target->Class ? target->Class->GetFullName() : nullptr);
	// 				const std::string tc(tcRaw ? tcRaw : "<null>");
	// 				Logger::Log("alarm", "%s  link[%d] -> %s  [class=%s]\n",
	// 					ind.c_str(), k, tn.c_str(), tc.c_str());
	// 				walk(target, depth + 1);
	// 			}
	// 		}
	// 	};
	//
	// 	for (int i = 0; i < Game->s_SeqEventAlarmBots.Count; i++) {
	// 		UTgSeqEvent_AlarmBots* ev = Game->s_SeqEventAlarmBots.Data[i];
	// 		if (ev == nullptr) { Logger::Log("alarm", "  [%d] <null>\n", i); continue; }
	// 		const char* evNameRaw = ev->GetFullName();
	// 		const std::string evName(evNameRaw ? evNameRaw : "<null>");
	// 		AActor* orig = ev->Originator;
	// 		std::string origName = "<null>";
	// 		std::string origClass = "<null>";
	// 		if (orig != nullptr) {
	// 			const char* on = orig->GetName();
	// 			origName = on ? on : "<null>";
	// 			if (orig->Class != nullptr) {
	// 				const char* oc = orig->Class->GetFullName();
	// 				origClass = oc ? oc : "<null>";
	// 			}
	// 		}
	// 		Logger::Log("alarm",
	// 			"  [%d] %s  originator=%s (%s)  bEnabled=%d MaxTriggerCount=%d\n",
	// 			i, evName.c_str(), origName.c_str(), origClass.c_str(),
	// 			(int)ev->bEnabled, ev->MaxTriggerCount);
	// 		visited.clear();
	// 		walk(ev, 1);
	// 	}
	// 	for (int i = 0; i < Game->s_SeqActAlarmBots.Count; i++) {
	// 		UTgSeqAct_AlarmBots* act = Game->s_SeqActAlarmBots.Data[i];
	// 		if (act == nullptr) { Logger::Log("alarm", "  act[%d] <null>\n", i); continue; }
	// 		const char* anRaw = act->GetFullName();
	// 		const std::string an(anRaw ? anRaw : "<null>");
	// 		Logger::Log("alarm", "  act[%d] %s\n", i, an.c_str());
	// 	}
	// 	Logger::Log("alarm", "=== end alarm dump ===\n");
	// }

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		LogCallEnd();
		return;
	}

	// Pre-scan for eligibility BEFORE any state mutation. We need to detect:
	//   (a) nPriority <= 0 — defender-pushback bogus call from UC
	//       UpdateObjectiveStatus:524 (`UnlockObjective(nPriority - 1)`).
	//       Status-7 fires when defenders push attacker capture back to zero
	//       on the CURRENT point — leaving mid-capture triggers it. For the
	//       first objective this becomes UnlockObjective(0). Priority 0 isn't
	//       a real game state (no priority-0 objectives, no priority-0 beacon
	//       factories); letting ABFS run with it destroys every entrance/exit
	//       on the map and there's nothing to respawn.
	//   (b) all matching objectives are capture-only-once + already captured.
	// Either case is a no-op — must not roll s_nCurrentPriority back or trip
	// ABFS.
	if (nPriority <= 0) {
		Logger::Log(GetLogChannel(),
			"UnlockObjective(%d): non-positive priority — bailing (defender-pushback status-7 path)\n",
			nPriority);
		LogCallEnd();
		return;
	}
	int matched = 0, eligible = 0;
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr || Obj->nPriority != nPriority) continue;
		matched++;
		if (!(Obj->m_bCaptureOnlyOnce && Obj->r_bHasBeenCapturedOnce)) eligible++;
	}
	if (matched > 0 && eligible == 0) {
		Logger::Log(GetLogChannel(),
			"UnlockObjective(%d): all %d matching objectives are capture-only-once + captured — bailing (no priority mutation, no ABFS)\n",
			nPriority, matched);
		LogCallEnd();
		return;
	}

	Game->s_nPrevPriority = Game->s_nCurrentPriority;
	Game->s_nCurrentPriority = nPriority;

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr || Obj->nPriority != nPriority) continue;

		if (Obj->m_bCaptureOnlyOnce && Obj->r_bHasBeenCapturedOnce) {
			Logger::Log(GetLogChannel(),
				"Skipping unlock of %s — already captured + flagged capture-only-once\n",
				Obj->GetFullName());
			continue;
		}

		Logger::Log(GetLogChannel(), "Unlocking objective %s\n", Obj->GetFullName());
		Obj->eventUnlockObjective(1);  // bActivateObjective=true
	}

	// Drive forward-spawn lifecycle: destroys stale-tier exit/entrance beacons
	// and spawns fresh ones at the new priority. UC never calls this native
	// (it's only declared, never invoked) and the original ATgGame native is
	// stripped — wiring it from here is the single source of truth for
	// priority-advancement beacon updates. Skipped at the bootstrap call
	// (prev priority was 0 — no factories have spawned anything yet);
	// SpawnObject's own priority filter handles the initial state.
	if (GRI->r_bIsPVP) {
		if (Game->s_nPrevPriority > 0 && Game->s_nPrevPriority != nPriority) {
			TgGame__AdjustBeaconForwardSpawn::Call(Game, nullptr, nPriority);
		}
	}

	// Activate TgBotFactory actors whose nPriority matches the newly-unlocked
	// phase. UC PostBeginPlay only auto-spawns priority==0 factories at boot
	// (TgBotFactory.uc:135); everything else relies on UnlockObjective to
	// fire SpawnNextBot. The original native presumably did this; we replace
	// the stripped behaviour here so each phase actually populates with bots.
	//
	// s_ActorFactories holds every TgActorFactory subclass (beacon, bot, …).
	// Class-name filter via GetFullName + strstr per the project's no-IsA
	// convention. We also gate on bAutoSpawn and !bSpawnOnAlarm to mirror
	// PostBeginPlay's auto-spawn predicate.
	int activated = 0;
	for (int i = 0; i < Game->s_ActorFactories.Count; i++) {
		ATgActorFactory* Factory = Game->s_ActorFactories.Data[i];
		if (Factory == nullptr || Factory->Class == nullptr) continue;
		const char* rawName = Factory->Class->GetFullName();
		if (rawName == nullptr) continue;
		const std::string className(rawName);
		if (className.find("TgBotFactory") == std::string::npos) continue;

		ATgBotFactory* Bot = (ATgBotFactory*)Factory;
		if (Bot->nPriority != nPriority) continue;
		if (!Bot->bAutoSpawn || Bot->bSpawnOnAlarm) continue;

		Logger::Log("tgbotfactory",
			"UnlockObjective(%d): activating bot factory %s (mapObjectId=%d nSpawnTableId=%d)\n",
			nPriority, Bot->GetName(), Bot->m_nMapObjectId, Bot->nSpawnTableId);
		Bot->ResetQueue(0);
		Bot->SpawnNextBot();
		activated++;
	}
	if (activated > 0) {
		Logger::Log("tgbotfactory",
			"UnlockObjective(%d): activated %d bot factories\n", nPriority, activated);
	}

	LogCallEnd();
}


