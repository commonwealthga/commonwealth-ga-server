#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Database/Database.hpp"

// Spawn-point picker for the stripped native `TgGame.TgFindPlayerStart`.
// UC entry: TgGame.uc:631 FindPlayerStart → TgFindPlayerStart.
//
// Filter (eligibility):
//   * bEnabled
//   * Task-force match — if both player and start carry a non-zero task force
//     they must agree; zero on either side means "team-agnostic, accept any".
//   * Priority gate — TgTeamPlayerStart.m_nPriority drives the forward-spawn
//     handoff: <= 0 means always available, > 0 means available only while
//     Game.s_nCurrentPriority matches. Mirrors TgLevelCamera.uc:53.
//     s_nCurrentPriority is advanced by TgGame::UnlockObjective when an
//     objective at the current priority completes.
//
// Ranking: m_fCurrentRating + (bPrimaryStart ? 100 : 0)  (TgStartPoint.uc:27).
// After picking, AdjustRating() (TgStartPoint.uc:49) decays the winner's
// rating so consecutive spawns rotate across eligible starts.
//
// Field seeding (m_nTaskForce, m_nPriority, nPrevPriority, m_nMinLevel,
// m_eCoalition) is handled by TgTeamPlayerStart::LoadObjectConfig pulling
// from map_object_config. If a map has no override rows, fields stay at
// their UC defaults and selection degrades gracefully.
//
// Checkpoints (PvE only): mission/tutorial Kismet SeqAct_Toggle turns
// later TgTeamPlayerStarts on (all but the first are bEnabled=False in the
// map) and does not always turn the first one off. Equal ratings kept the
// pick on the first start -> respawn at the mission start. In PvE the start
// enabled most recently wins; ties fall back to the rating ranking.

bool TgGame__TgFindPlayerStart::IsPvECheckpointMap(ATgGame* Game) {
	if (Game == nullptr || Game->m_GameType == 1) return false;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr) return false;
	return !GRI->r_bIsPVP && !GRI->r_bIsOpenWorld;
}

// The revive paths (TgGame RunWave, TgPlayerController.Release) call
// Dead.Revive on the same pawn; SpawnPlayerCharacter (the first checkpoint hook) is
// not on that path, so the checkpoint teleport has to run here.
void TgGame__TgFindPlayerStart::ZoneReviveMove(ATgGame* Game, AController* Controller, const char* why) {
	if (Controller == nullptr || Controller->Pawn == nullptr || Controller->Pawn->Health <= 0) return;
	ATgRepInfo_Game* GRI = Game ? (ATgRepInfo_Game*)Game->GameReplicationInfo : nullptr;
	if (GRI == nullptr || !GRI->r_bIsOpenWorld || s_ZoneDeaths.find(Controller) == s_ZoneDeaths.end()) return;
	ANavigationPoint* sp = PickZoneRespawn(Game, Controller);
	if (sp == nullptr) return;
	APawn* pawn = Controller->Pawn;
	FRotator rot = sp->Rotation; rot.Pitch = 0; rot.Roll = 0;
	const bool ok = pawn->SetLocation(sp->Location);
	if (ok) Controller->ClientSetLocation(pawn->Location, rot);
	Logger::Log("spawn", "TgFindPlayerStart: zone revive (%s) moved ok=%d to (%.0f,%.0f,%.0f)\n",
		why, (int)ok, pawn->Location.X, pawn->Location.Y, pawn->Location.Z);
}

void TgGame__TgFindPlayerStart::CheckpointRespawn(ATgGame* Game, AController* Controller, const char* why) {
	ZoneReviveMove(Game, Controller, why);
	if (Game == nullptr || Controller == nullptr || Game->bGameEnded || !IsPvECheckpointMap(Game)) return;
	ATgPawn* pawn = (ATgPawn*)Controller->Pawn;
	if (pawn == nullptr || pawn->Health <= 0) return;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Controller->PlayerReplicationInfo;
	ATgRepInfo_TaskForce* tf = pri ? pri->r_TaskForce : nullptr;
	ATgTeamBeaconManager* mgr = tf ? tf->r_BeaconManager : nullptr;
	ATgDeploy_Beacon* beacon = mgr ? mgr->r_Beacon : nullptr;
	ATgBeaconFactory* factory = beacon ? (ATgBeaconFactory*)beacon->s_DeployFactory : nullptr;
	if (factory != nullptr && beacon->m_bIsDeployed) {
		Logger::Log("spawn",
			"SpawnPlayerCharacter: checkpoint respawn (%s) -> beacon factory mapObjId=%d prio=%d (%.0f,%.0f,%.0f) from (%.0f,%.0f,%.0f)\n",
			why, factory->m_nMapObjectId, factory->m_nPriority, beacon->Location.X, beacon->Location.Y, beacon->Location.Z,
			pawn->Location.X, pawn->Location.Y, pawn->Location.Z);
		pawn->eventTriggerBeaconEntrance();
	} else {
		Logger::Log("spawn", "TgFindPlayerStart: no checkpoint on revive (%s) mgr=%p beacon=%p factory=%p deployed=%d at (%.0f,%.0f,%.0f)\n",
			why, (void*)mgr, (void*)beacon, (void*)factory, beacon ? (int)beacon->m_bIsDeployed : -1,
			pawn->Location.X, pawn->Location.Y, pawn->Location.Z);
	}
}

bool TgGame__TgFindPlayerStart::DevToggleFactory(int mapObjId) {
	for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgBeaconFactory")) {
		ATgBeaconFactory* f = (ATgBeaconFactory*)obj;
		if (f == nullptr || f->m_nMapObjectId != mapObjId) continue;
		f->s_bAutoSpawn = 1;
		TgBeaconFactory__SpawnObject::Call(f, nullptr);
		Logger::Log("spawn", "TgFindPlayerStart: [DEV] toggle beacon factory mapObjId=%d exit=%d prio=%d tf=%d on (%.0f,%.0f,%.0f)\n",
			mapObjId, (int)f->m_bBeaconExit, f->m_nPriority, (int)f->s_nTaskForce, f->Location.X, f->Location.Y, f->Location.Z);
		return true;
	}
	Logger::Log("spawn", "TgFindPlayerStart: [DEV] toggle: no beacon factory mapObjId=%d\n", mapObjId);
	return false;
}

static bool IsOpenWorldGame(ATgGame* Game) {
	ATgRepInfo_Game* GRI = Game ? (ATgRepInfo_Game*)Game->GameReplicationInfo : nullptr;
	return GRI != nullptr && GRI->r_bIsOpenWorld;
}

void TgGame__TgFindPlayerStart::NoteDeath(ATgPawn* Pawn) {
	if (Pawn == nullptr || Pawn->Controller == nullptr || Pawn->WorldInfo == nullptr) return;
	if (!IsOpenWorldGame((ATgGame*)Pawn->WorldInfo->Game)) return;
	if (!ObjectClassCache::ClassNameContains(Pawn->Controller, "PlayerController")) return;
	int64_t charId = 0;
	for (auto& kv : GClientConnectionsData) {
		if ((ATgPawn*)kv.second.Pawn != Pawn || kv.second.bClosed) continue;
		if (kv.second.pPlayerInfo) charId = kv.second.pPlayerInfo->selected_character_id;
		break;
	}
	if (s_ZoneDeaths.size() > 512) s_ZoneDeaths.clear();
	s_ZoneDeaths[Pawn->Controller] = ZoneDeath{ Pawn->Location, charId };
	Logger::Log("spawn", "TgFindPlayerStart: zone death char=%lld at (%.0f,%.0f,%.0f)\n",
		(long long)charId, Pawn->Location.X, Pawn->Location.Y, Pawn->Location.Z);
}

ANavigationPoint* TgGame__TgFindPlayerStart::PickZoneRespawn(ATgGame* Game, AController* Controller) {
	if (Controller == nullptr || !IsOpenWorldGame(Game)) return nullptr;
	auto it = s_ZoneDeaths.find(Controller);
	if (it == s_ZoneDeaths.end()) return nullptr;
	const ZoneDeath death = it->second;
	s_ZoneDeaths.erase(it);
	static UClass* startClass = (UClass*)ObjectCache::Find("Class TgGame.TgStartPoint");
	TArray<UObject*>* objects = UObject::GObjObjects();
	ATgStartPoint* best = nullptr;
	float bestD2 = 0.0f;
	int seen = 0;
	for (int i = 0; startClass != nullptr && objects != nullptr && i < objects->Count; i++) {
		UObject* obj = objects->Data[i];
		if (obj == nullptr || obj->Class != startClass) continue;
		const char* nm = obj->GetName();
		if (nm == nullptr || strncmp(nm, "Default__", 9) == 0) continue;
		ATgStartPoint* sp = (ATgStartPoint*)obj;
		if (!sp->bEnabled || sp->TgPortalNetwork == nullptr) continue;   // beacon network points only
		seen++;
		const int group = sp->AvailabilityQuestGroupId;
		if (group > 0 && (death.charId <= 0 || !Database::QuestGroupSatisfied(death.charId, group))) continue;
		const float dx = sp->Location.X - death.loc.X, dy = sp->Location.Y - death.loc.Y, dz = sp->Location.Z - death.loc.Z;
		const float d2 = dx * dx + dy * dy + dz * dz;
		if (best == nullptr || d2 < bestD2) { best = sp; bestD2 = d2; }
	}
	if (best == nullptr) {
		Logger::Log("spawn", "TgFindPlayerStart: zone respawn char=%lld: no unlocked beacon start (%d on networks), stock pick\n",
			(long long)death.charId, seen);
		return nullptr;
	}
	s_ChosenYaw[Controller] = best->Rotation.Yaw;
	Logger::Log("spawn", "TgFindPlayerStart: zone respawn char=%lld died (%.0f,%.0f,%.0f) -> %s mapObjId=%d group=%d (%.0f,%.0f,%.0f)\n",
		(long long)death.charId, death.loc.X, death.loc.Y, death.loc.Z, ((UObject*)best)->GetName(), best->m_nMapObjectId,
		best->AvailabilityQuestGroupId, best->Location.X, best->Location.Y, best->Location.Z);
	return best;
}

ANavigationPoint* __fastcall TgGame__TgFindPlayerStart::Call(ATgGame* Game, void* edx, AController* Controller, FString* IncomingName) {
	LogCallBegin();

	ActorCache::CacheMapActors();

	if (ActorCache::PlayerStarts.empty()) {
		if (ANavigationPoint* zoneStart = PickZoneRespawn(Game, Controller)) {
			LogCallEnd();
			return zoneStart;
		}
		// Hub maps (GT_CITY) carry no TgTeamPlayerStart; the stock fallback
		// lands agents on a travel door. Use the map's own PlayerStart.
		if (Game != nullptr && Game->m_GameType == 1) {
			for (UObject* obj : ObjectCache::FindAllByClass("Class Engine.PlayerStart")) {
				APlayerStart* start = (APlayerStart*)obj;
				if (!start->bEnabled) continue;
				if (Controller != nullptr) s_ChosenYaw[Controller] = start->Rotation.Yaw;
				Logger::Log("spawn", "TgFindPlayerStart: city start %s (%.1f,%.1f,%.1f)\n",
					obj->GetName(), start->Location.X, start->Location.Y, start->Location.Z);
				LogCallEnd();
				return start;
			}
			// The Dome's only PlayerStart (PlayerStart_2, beside Dalton Bancroft) lives in the
			// Kismet-streamed sublevel DomeCity_V3_MAIN. It loads after ObjectCache's first scan into a
			// reused GObjObjects slot below the scan mark, so the cached class list never holds it and
			// every arrival fell through to the stripped native (= the Sonoran door). Walk the live
			// object array once by class pointer instead.
			static UClass* playerStartClass = (UClass*)ObjectCache::Find("Class Engine.PlayerStart");
			TArray<UObject*>* objects = UObject::GObjObjects();
			for (int i = 0; playerStartClass != nullptr && objects != nullptr && i < objects->Count; i++) {
				UObject* obj = objects->Data[i];
				if (obj == nullptr || obj->Class != playerStartClass) continue;
				APlayerStart* start = (APlayerStart*)obj;
				const char* nm = obj->GetName();
				if (!start->bEnabled || nm == nullptr || strncmp(nm, "Default__", 9) == 0) continue;
				if (Controller != nullptr) s_ChosenYaw[Controller] = start->Rotation.Yaw;
				Logger::Log("spawn", "TgFindPlayerStart: city start %s (%.1f,%.1f,%.1f) via object scan\n",
					obj->GetName(), start->Location.X, start->Location.Y, start->Location.Z);
				LogCallEnd();
				return start;
			}
		}
		Logger::Log("spawn", "TgFindPlayerStart: no player starts cached, falling back to original\n");
		LogCallEnd();
		return CallOriginal(Game, edx, Controller, IncomingName);
	}

	unsigned char playerTaskForce = 0;
	if (Controller != nullptr && Controller->PlayerReplicationInfo != nullptr) {
		ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)Controller->PlayerReplicationInfo;
		if (RepInfo->r_TaskForce != nullptr) {
			playerTaskForce = RepInfo->r_TaskForce->r_nTaskForce;
		}
	}

	const int currentPriority = Game ? Game->s_nCurrentPriority : 0;
	const bool logEnabled = Logger::IsChannelEnabled("spawn");
	const bool pveCheckpoints = IsPvECheckpointMap(Game);
	const bool firstPick = s_EnabledSeq.empty();

	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (start == nullptr) continue;
		auto seqIt = s_EnabledSeq.find(start);
		if (!start->bEnabled) {
			if (seqIt != s_EnabledSeq.end()) s_EnabledSeq.erase(seqIt);
		} else if (seqIt == s_EnabledSeq.end()) {
			const int seq = firstPick ? 0 : s_NextEnabledSeq++;
			s_EnabledSeq[start] = seq;
			if (seq > 0) {
				Logger::Log("spawn", "TgFindPlayerStart: checkpoint start %s mapObjId=%d enabled (seq %d)\n",
					((UObject*)start)->GetName(), start->m_nMapObjectId, seq);
			}
		}
	}

	if (logEnabled) {
		Logger::Log("spawn",
			"TgFindPlayerStart: %zu candidate(s), playerTaskForce=%d, s_nCurrentPriority=%d\n",
			ActorCache::PlayerStarts.size(), playerTaskForce, currentPriority);
	}

	ATgTeamPlayerStart* best = nullptr;
	float bestRating = -1.0f;
	int bestSeq = -1;

	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (start == nullptr) continue;

		const float rating = start->m_fCurrentRating + (start->bPrimaryStart ? 100.0f : 0.0f);

		const bool enabled       = start->bEnabled;
		const bool taskForceOk   = playerTaskForce == 0 || start->m_nTaskForce == 0 || start->m_nTaskForce == playerTaskForce;
		const bool priorityOk    = start->m_nPriority <= 0 || start->m_nPriority == currentPriority;
		const bool eligible      = enabled && taskForceOk && priorityOk;

		if (logEnabled) {
			Logger::Log("spawn",
				"  %s mapObjId=%d enabled=%d tf=%d prio=%d rating=%.1f primary=%d eligible=%d loc=(%.0f,%.0f,%.0f)\n",
				((UObject*)start)->GetName(),
				start->m_nMapObjectId,
				enabled ? 1 : 0,
				(int)start->m_nTaskForce,
				start->m_nPriority,
				rating,
				start->bPrimaryStart ? 1 : 0,
				eligible ? 1 : 0,
				start->Location.X, start->Location.Y, start->Location.Z);
		}

		if (!eligible) continue;
		const int seq = pveCheckpoints ? s_EnabledSeq[start] : 0;
		if (seq > bestSeq || (seq == bestSeq && rating > bestRating)) {
			bestSeq = seq;
			bestRating = rating;
			best = start;
		}
	}

	if (best == nullptr) {
		Logger::Log("spawn", "TgFindPlayerStart: no eligible start, falling back to original\n");
		LogCallEnd();
		return CallOriginal(Game, edx, Controller, IncomingName);
	}

	if (best->m_fDecreaseRate != 0.0f) {
		best->m_fCurrentRating -= best->m_fDecreaseRate;
		if (best->m_fCurrentRating < best->m_fResetRating) {
			best->m_fCurrentRating = best->m_fStartRating;
		}
	}

	if (Controller != nullptr) {
		s_ChosenYaw[Controller] = best->Rotation.Yaw;
	}

	if (logEnabled) {
		Logger::Log("spawn",
			"TgFindPlayerStart: chose %s mapObjId=%d tf=%d prio=%d rating=%.1f yaw=%d seq=%d\n",
			((UObject*)best)->GetName(), best->m_nMapObjectId,
			(int)best->m_nTaskForce, best->m_nPriority, bestRating, best->Rotation.Yaw, bestSeq);
	}

	LogCallEnd();
	return best;
}
