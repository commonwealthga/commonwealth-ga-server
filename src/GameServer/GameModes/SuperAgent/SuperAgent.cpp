#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

namespace {

const char* kChannel = "superagent";

// Boss-room "alarm adds" factories are identified by this spawn table id at map
// load; a capture-% hook re-tables them (e.g. to 41 = bosses) and unleashes.
constexpr int kBossAddsTable = 59;

// AddObjectivePointToList @ 0x109f0580 — inserts an objective into
// GRI->r_Objectives, kept sorted ASCENDING by nPriority, and marks the GRI
// replicated-dirty. Re-inserting a moved objective fixes its HUD position.
typedef void(__fastcall* AddObjectivePointToList_t)(ATgRepInfo_Game*, void*, ATgMissionObjective*);
const AddObjectivePointToList_t AddObjectivePointToList =
	reinterpret_cast<AddObjectivePointToList_t>(0x109f0580);

// ---- match-lifetime runtime state (reset each Init) ----
ATgGame*          s_Game           = nullptr;
ATgRepInfo_Game*  s_GRI            = nullptr;
bool s_aWasLocked  = true;    // A starts locked; 1->0 means "boss killed"
bool s_bossKilled  = false;
bool s_aCaptured   = false;

// Boss-room adds factories (spawn table kBossAddsTable at map load) — the 25%
// hook re-tables these to bosses. s_AlarmFactories is EVERY alarm-gated factory
// on the map, used by the global escape alarm.
std::vector<ATgBotFactory*> s_AddFactories;
std::vector<ATgBotFactory*> s_AlarmFactories;
AActor* s_AlarmOriginator = nullptr;
int     s_AlarmId         = 0;

// Escape-phase spawn-table remap (current table -> escape table), authored in
// AuthorMission(). Applied to every normal factory when A is captured.
std::map<int, int> s_EscapeRemap;

// Escape-phase periodic GLOBAL alarm, authored via Escape::PeriodicAlarm().
// Fires at a random interval in [min,max]; max<=0 disables. s_nextEscapeAlarm is
// the absolute WorldInfo.TimeSeconds of the next scheduled alarm.
float s_escapeAlarmMin  = 0.0f;
float s_escapeAlarmMax  = 0.0f;
float s_nextEscapeAlarm = 0.0f;

// Spacing for the on-capture respawn so the whole map's roster doesn't appear in
// one frame (PRIs are always-relevant — a burst spikes replication).
constexpr float kRespawnBaseDelay = 0.5f;   // first factory's first spawn
constexpr float kRespawnStagger   = 0.5f;   // added per factory

// Team-gated geometry, bucketed by nearest spawn at Init.
//   defender* : nearest to the defender (boss-side) spawn  -> flipped on boss kill
//   residual* : everything that isn't nearest to either spawn -> flipped on A capture
//   (the attacker-nearest gate is left alone — players already own that side)
std::vector<ATgTeamBlocker*> s_DefenderBlockers;
std::vector<ATgDoorMarker*>  s_DefenderMarkers;
std::vector<ATgTeamBlocker*> s_ResidualBlockers;
std::vector<ATgDoorMarker*>  s_ResidualMarkers;

// 1<->2 task-force/team swap (other values pass through unchanged).
int inv(int v) { return v == 1 ? 2 : (v == 2 ? 1 : v); }

float Dist2(const FVector& a, const FVector& b) {
	const float dx = a.X - b.X, dy = a.Y - b.Y, dz = a.Z - b.Z;
	return dx * dx + dy * dy + dz * dz;
}

// Uniform random in [lo, hi]. rand() is process-seeded by the bot-factory code
// at map load; good enough for ambush jitter.
float RandRange(float lo, float hi) {
	if (hi <= lo) return lo;
	return lo + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (hi - lo);
}

// A pawn counts toward capture ONLY if a real human drives it. bIsPlayer / bBot
// are unreliable on this build; the controller class is the reliable signal —
// henchmen / pets are AI-controlled, so they never satisfy this and thus can
// never drive (or win) a capture.
bool IsHumanPawn(ATgPawn* pawn) {
	if (!pawn || !pawn->Controller) return false;
	return ObjectClassCache::ClassNameContains(pawn->Controller, "PlayerController");
}

// Human player pawns currently inside the capture cylinder (henchmen excluded).
int HumansOnPoint(ATgMissionObjective_Proximity* Obj) {
	ATgCollisionProxy* proxy = Obj->s_CollisionProxy;
	if (!proxy) return 0;
	int n = 0;
	for (int i = 0; i < proxy->m_NearByPlayers.Count; i++) {
		if (IsHumanPawn(proxy->m_NearByPlayers.Data[i])) n++;
	}
	return n;
}

// How many human players are in the match (live TF1 roster, dead or alive).
// Read each tick, so player join/leave needs no bookkeeping.
int HumanRosterCount() {
	ATgRepInfo_TaskForce* tf1 = GTeamsData.Attackers;
	if (!tf1) return 0;
	int roster = 0;
	for (int i = 0; i < tf1->m_TeamPlayers.Count; i++) {
		ATgRepInfo_Player* pri = tf1->m_TeamPlayers.Data[i].pPrep;
		if (pri && !pri->bBot) roster++;
	}
	return roster;
}

// requireAllPlayers gate: EVERY human must be on the point. A dead / absent
// human has no pawn in the zone, so any gap freezes capture — the "everyone
// here or nothing" pressure.
bool AllHumansPresent(ATgMissionObjective_Proximity* Obj) {
	const int roster = HumanRosterCount();
	return roster > 0 && HumansOnPoint(Obj) >= roster;
}

// Default gate: at least one human must be on the point. Henchmen / bots alone
// can never drive (or win) a capture.
bool AtLeastOneHumanPresent(ATgMissionObjective_Proximity* Obj) {
	return HumansOnPoint(Obj) >= 1;
}

// ---- geometry flips ----

void FlipBlocker(ATgTeamBlocker* b) {
	if (!b) return;
	b->m_nTeam      = inv(b->m_nTeam);
	b->m_eCoalition = (unsigned char)inv(b->m_eCoalition);
}

void FlipMarker(ATgDoorMarker* d) {
	if (!d) return;
	d->m_nTeam                 = inv(d->m_nTeam);
	d->m_eCoalition            = (unsigned char)inv(d->m_eCoalition);
	d->m_nTeamThatControlsDoor = inv(d->m_nTeamThatControlsDoor);
}

// Swap every player start's task force (1<->2) and zero its priority so the
// now-TF1 (formerly defender, boss-side) starts are eligible for the rest of
// the match. After this, dying players respawn by the boss room. Blockers are
// server-side only, so no client message is needed for any of these flips.
void FlipSpawnPoints() {
	int flipped = 0;
	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (!start) continue;
		const int tf = (int)start->m_nTaskForce;
		if (tf != 1 && tf != 2) continue;
		start->m_nTaskForce = (unsigned char)inv(tf);
		start->m_nPriority  = 0;
		flipped++;
	}
	SuperAgent::Log("flip spawn points: %d starts swapped\n", flipped);
}

// Fired once, when the boss dies (A unlocks). Open the boss-room exit for the
// (still TF1) players and move their respawns up to the boss side.
void OnBossKilled() {
	FlipSpawnPoints();
	for (ATgTeamBlocker* b : s_DefenderBlockers) FlipBlocker(b);
	for (ATgDoorMarker*  d : s_DefenderMarkers)  FlipMarker(d);
	SuperAgent::Log("boss killed: flipped %zu defender blockers, %zu defender markers\n",
		s_DefenderBlockers.size(), s_DefenderMarkers.size());
}

// Clear and re-spawn every NORMAL (non-pet, non-alarm) bot factory, applying
// the escape-phase spawn-table remap. Alarm factories are intentionally left
// alone — they stay alarm-gated and are driven by the periodic escape alarm.
// The hard count reset is required: SpawnNextBot's active-cap brake
// (nCurrentCount >= nActiveCount) would otherwise block the respawn because the
// just-killed bots' counts only drop asynchronously via BotDied.
void RespawnAllFactories() {
	int n = 0;
	for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgBotFactory")) {
		ATgBotFactory* f = (ATgBotFactory*)obj;
		if (!f) continue;
		if (ObjectClassCache::ClassNameContains(f, "TgBotFactorySpawnable")) continue;  // pets
		if (f->bSpawnOnAlarm) continue;   // alarm factories stay alarm-gated

		const int cur = f->nSpawnTableId;
		const auto it = s_EscapeRemap.find(cur);
		const int newTable = (it != s_EscapeRemap.end()) ? it->second : cur;

		// SpawnBotById drops every bot from a factory whose nPriority>0 and
		// != s_nCurrentPriority (the mission has advanced past 1 by now). Set the
		// "any phase" sentinel -1 so these spawn here AND keep spawning BotDied
		// replacements through the escape.
		f->nPriority = -1;
		f->bAutoSpawn = 1;
		f->bBulkSpawn = 0;                // drip the roster one-at-a-time, not all at once
		f->eventKillBots(1);              // despawn current roster
		f->nCurrentCount = 0;             // clear the active-cap brake
		for (int i = 0; i < f->m_SpawnGroups.Num(); i++) {
			if (f->m_SpawnGroups.Data) f->m_SpawnGroups.Data[i].nCurrentCount = 0;
		}
		TgBotFactory__ResetQueue::Call(f, nullptr, newTable);
		// Don't spawn now — stagger each factory's FIRST spawn so the whole map's
		// roster doesn't replicate in one frame. From there, bBulkSpawn=0 +
		// fSpawnDelay keep each factory's roster dripping out over time.
		Actor__SetTimer::SetTimer((AActor*)f, kRespawnBaseDelay + n * kRespawnStagger,
			/*bLoop=*/false, FName("SpawnNextBot"), nullptr);
		n++;
	}
	SuperAgent::Log("escape: staggered respawn of %d normal factories (alarm/pet skipped)\n", n);
}

// Periodic GLOBAL escape alarm: ambush every player from the alarm-gated factory
// nearest to them (its own roster), like the normal-mission alarm. Respects each
// factory's active cap (no count reset) so ambushers replenish as they die
// rather than piling up. Also fires the native alarm for the siren/kismet.
void FireGlobalAlarm() {
	ATgRepInfo_TaskForce* tf1 = GTeamsData.Attackers;
	if (!tf1 || s_AlarmFactories.empty()) return;

	std::vector<ATgBotFactory*> fired;   // dedupe: one wave per factory per alarm
	for (int i = 0; i < tf1->m_TeamPlayers.Count; i++) {
		ATgRepInfo_Player* pri = tf1->m_TeamPlayers.Data[i].pPrep;
		if (!pri || pri->bBot) continue;
		ATgPawn* pawn = pri->r_PawnOwner;
		if (!IsHumanPawn(pawn)) continue;   // alive human only

		ATgBotFactory* nearest = nullptr; float best = 0.0f;
		for (ATgBotFactory* f : s_AlarmFactories) {
			if (!f) continue;
			const float d = Dist2(f->Location, pawn->Location);
			if (!nearest || d < best) { best = d; nearest = f; }
		}
		if (!nearest) continue;
		bool already = false;
		for (ATgBotFactory* g : fired) if (g == nearest) { already = true; break; }
		if (already) continue;
		fired.push_back(nearest);

		// Skip if it's already at its active cap from a recent ambush.
		if (nearest->nActiveCount > 0 && nearest->nCurrentCount >= nearest->nActiveCount) continue;

		nearest->nPriority = -1;   // phase-gate: eligible in the escape phase
		const unsigned long savedAuto = nearest->bAutoSpawn;
		nearest->bAutoSpawn = 1;
		TgBotFactory__ResetQueue::Call(nearest, nullptr, 0);   // own/default table
		TgBotFactory__SpawnNextBot::Call(nearest, nullptr);
		nearest->bAutoSpawn = savedAuto;
	}

	// Siren / AlarmBots kismet for the alarm feel.
	if (s_Game && s_AlarmOriginator) {
		TgGame__ActivateAlarm::Call(s_Game, nullptr, s_AlarmOriginator, s_AlarmId, 0, 0, 0);
	}
	SuperAgent::Log("global alarm: %zu ambush wave(s)\n", fired.size());
}

// Fired once, when A is captured. Open the path back to the original spawn (B),
// refresh every normal factory's roster (harder, per the escape remap), and
// start the periodic escape alarm clock.
void OnACaptured() {
	for (ATgTeamBlocker* b : s_ResidualBlockers) FlipBlocker(b);
	for (ATgDoorMarker*  d : s_ResidualMarkers)  FlipMarker(d);
	SuperAgent::Log("A captured: flipped %zu residual blockers, %zu residual markers\n",
		s_ResidualBlockers.size(), s_ResidualMarkers.size());
	RespawnAllFactories();
	// Arm the first periodic global alarm at a random offset.
	const float now = (s_Game && s_Game->WorldInfo) ? s_Game->WorldInfo->TimeSeconds : 0.0f;
	s_nextEscapeAlarm = now + RandRange(s_escapeAlarmMin, s_escapeAlarmMax);
}

// Bucket every door marker / team blocker by which spawn it sits nearest. The
// single closest of each type to each spawn is that spawn's "gate"; everything
// else is residual. The attacker gate is cached only to exclude it from residual.
void CacheGeometry(ATgTeamPlayerStart* attackerStart, ATgTeamPlayerStart* defenderStart) {
	s_DefenderBlockers.clear(); s_DefenderMarkers.clear();
	s_ResidualBlockers.clear(); s_ResidualMarkers.clear();
	if (!attackerStart || !defenderStart) return;
	const FVector aLoc = attackerStart->Location;
	const FVector dLoc = defenderStart->Location;

	// Blockers.
	{
		std::vector<ATgTeamBlocker*> all;
		for (UObject* obj : ObjectCache::FindAllByClassSubstr("TgTeamBlocker")) {
			ATgTeamBlocker* b = (ATgTeamBlocker*)obj;
			if (b) all.push_back(b);
		}
		ATgTeamBlocker* nearA = nullptr; float bestA = 0;
		ATgTeamBlocker* nearD = nullptr; float bestD = 0;
		for (ATgTeamBlocker* b : all) {
			const float da = Dist2(b->Location, aLoc), dd = Dist2(b->Location, dLoc);
			if (!nearA || da < bestA) { bestA = da; nearA = b; }
			if (!nearD || dd < bestD) { bestD = dd; nearD = b; }
		}
		if (nearD) s_DefenderBlockers.push_back(nearD);
		for (ATgTeamBlocker* b : all) {
			if (b == nearA || b == nearD) continue;
			s_ResidualBlockers.push_back(b);
		}
	}
	// Door markers.
	{
		std::vector<ATgDoorMarker*> all;
		for (UObject* obj : ObjectCache::FindAllByClassSubstr("TgDoorMarker")) {
			ATgDoorMarker* d = (ATgDoorMarker*)obj;
			if (d) all.push_back(d);
		}
		ATgDoorMarker* nearA = nullptr; float bestA = 0;
		ATgDoorMarker* nearD = nullptr; float bestD = 0;
		for (ATgDoorMarker* d : all) {
			const float da = Dist2(d->Location, aLoc), dd = Dist2(d->Location, dLoc);
			if (!nearA || da < bestA) { bestA = da; nearA = d; }
			if (!nearD || dd < bestD) { bestD = dd; nearD = d; }
		}
		if (nearD) s_DefenderMarkers.push_back(nearD);
		for (ATgDoorMarker* d : all) {
			if (d == nearA || d == nearD) continue;
			s_ResidualMarkers.push_back(d);
		}
	}
	SuperAgent::Log("geometry: defender(%zu blk,%zu mrk) residual(%zu blk,%zu mrk)\n",
		s_DefenderBlockers.size(), s_DefenderMarkers.size(),
		s_ResidualBlockers.size(), s_ResidualMarkers.size());
}

// Cache the boss-room adds factories (spawn table kBossAddsTable, driven by the
// 25% hook) and EVERY alarm-gated factory on the map (driven by the global
// escape alarm), plus a single alarm originator/id for the siren.
void CacheAddFactories() {
	s_AddFactories.clear();
	s_AlarmFactories.clear();
	s_AlarmOriginator = nullptr;
	s_AlarmId = 0;
	for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgBotFactory")) {
		ATgBotFactory* f = (ATgBotFactory*)obj;
		if (!f) continue;
		if (ObjectClassCache::ClassNameContains(f, "TgBotFactorySpawnable")) continue;
		if (f->bSpawnOnAlarm) {
			s_AlarmFactories.push_back(f);
			if (!s_AlarmOriginator) {
				s_AlarmOriginator = (AActor*)f;
				s_AlarmId = f->nGlobalAlarmId;
			}
		}
		if (f->nSpawnTableId == kBossAddsTable) s_AddFactories.push_back(f);
	}
	SuperAgent::Log("factories: %zu boss-room (table %d), %zu alarm-gated (global)\n",
		s_AddFactories.size(), kBossAddsTable, s_AlarmFactories.size());
}

// Pull `boss` out of GRI->r_Objectives (shift the tail up), then re-insert it
// via AddObjectivePointToList so it lands in nPriority order. The list is only
// re-sorted on insert and the boss's priority was changed AFTER its map-load
// insert, so without this it keeps its stale (last) slot on the HUD.
void ReorderBossFirst(ATgRepInfo_Game* GRI, ATgMissionObjective* boss) {
	if (!GRI || !boss) return;
	const int kMax = 0x4B;  // r_Objectives is ATgMissionObjective*[75]
	int idx = -1;
	for (int i = 0; i < kMax; i++) {
		if (GRI->r_Objectives[i] == boss) { idx = i; break; }
		if (GRI->r_Objectives[i] == nullptr) break;
	}
	if (idx >= 0) {
		for (int i = idx; i < kMax - 1; i++) GRI->r_Objectives[i] = GRI->r_Objectives[i + 1];
		GRI->r_Objectives[kMax - 1] = nullptr;
	}
	AddObjectivePointToList(GRI, nullptr, boss);   // re-insert sorted + mark dirty
}

// Spawn one proximity objective from a CapturePoint config at loc/rot, register
// it into the GRI, and store it back on the config. See the prior design notes
// for the bNoDelete / replication / nObjectiveId gotchas.
ATgMissionObjective_Proximity* SpawnPoint(
	ATgGame* Game, ATgRepInfo_Game* GRI, UClass* cls,
	const FVector& loc, const FRotator& rot, SuperAgent::CapturePoint& cfg) {

	ATgMissionObjective_Proximity* Obj =
		(ATgMissionObjective_Proximity*)Game->Spawn(cls, (AActor*)Game, FName(), loc, rot, nullptr, 1);
	if (!Obj) {
		SuperAgent::Log("SpawnPoint FAILED (def=%d priority=%d)\n", cfg.objectiveDefId, cfg.priority);
		return nullptr;
	}

	// Ground-snap so the objective's visual mesh sits on the floor rather than
	// at the (often elevated) anchor actor's height. bTraceActors=1 so static
	// meshes count as floor, not just BSP.
	{
		FVector start = loc; start.Z += 64.0f;
		FVector end   = loc; end.Z   -= 8192.0f;
		FVector hitLoc, hitNorm;
		FTraceHitInfo hitInfo;
		std::memset(&hitInfo, 0, sizeof(hitInfo));
		AActor* ground = Obj->Trace(end, start, 1, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo);
		if (ground) {
			FVector g = loc; g.Z = hitLoc.Z;
			Obj->SetLocation(g);
		}
	}

	// Plain dynamic actor (dynamic NetGUID + spawn command). bNoDelete/bStatic
	// would make it a net-startup actor the client can't resolve -> never appears.
	Obj->bNoDelete = 0;
	Obj->bStatic   = 0;

	Obj->nObjectiveId           = cfg.objectiveDefId;
	Obj->nPriority              = cfg.priority;
	Obj->nDefaultOwnerTaskForce = cfg.ownerTaskForce;
	Obj->r_nOwnerTaskForce      = cfg.ownerTaskForce;
	Obj->m_nCurrOwnerTaskforce  = cfg.ownerTaskForce;
	Obj->m_bCaptureOnlyOnce     = 1;
	Obj->r_bIsLocked            = 1;   // unlocked later by the priority chain
	Obj->m_fTimeToCapture       = cfg.timeToCapture;

	// Replication recipe (mirrors TgGame::LoadGameConfig for map-baked objectives).
	Obj->Role                          = 3;   // ROLE_Authority
	Obj->RemoteRole                    = 1;   // ROLE_SimulatedProxy
	Obj->bAlwaysRelevant               = 1;
	Obj->bNetInitial                   = 1;
	Obj->bNetDirty                     = 1;
	Obj->bForceNetUpdate               = 1;
	Obj->bOnlyDirtyReplication         = 0;
	Obj->bSkipActorPropertyReplication = 0;
	if (Game->WorldInfo) Obj->SetOwner((AActor*)Game->WorldInfo);

	// Capture cylinder (RegisterSelf is idempotent). Sized tall so floor-level
	// players are inside it even with the objective anchored high.
	TgMissionObjective__RegisterSelf::Call((ATgMissionObjective*)Obj, nullptr);
	if (Obj->s_CollisionProxy) {
		UCylinderComponent* cyl = (UCylinderComponent*)Obj->s_CollisionProxy->CollisionComponent;
		if (cyl) cyl->SetCylinderSize(cfg.cylRadius, cfg.cylHalfHeight);
	}

	AddObjectivePointToList(GRI, nullptr, (ATgMissionObjective*)Obj);

	cfg.obj = Obj;
	SuperAgent::Log("spawned point def=%d priority=%d ownerTF=%d time=%.0f loc=(%.0f,%.0f,%.0f)\n",
		cfg.objectiveDefId, cfg.priority, cfg.ownerTaskForce, cfg.timeToCapture, loc.X, loc.Y, loc.Z);
	return Obj;
}

}  // namespace

namespace SuperAgent {

CapturePoint A;
CapturePoint B;

void CapturePoint::at(float pct, std::function<void()> action) {
	hooks.push_back({pct, false, std::move(action)});
}

void Log(const char* fmt, ...) {
	char buf[512];
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	Logger::Log(kChannel, "%s", buf);
}

namespace Adds {

void Unleash(int tableId) {
	int n = 0;
	for (ATgBotFactory* f : s_AddFactories) {
		if (!f) continue;
		if (tableId > 0) f->nSpawnTableId = tableId;
		// SpawnBotById drops every bot from a factory whose nPriority>0 and
		// != s_nCurrentPriority. The boss-room factories are nPriority=1 (boss
		// phase), so once the mission advances they spawn nothing. Set the "any
		// phase" sentinel -1 so they fire in every phase from here on.
		f->nPriority = -1;
		// Spawn this wave's FULL roster, unconditionally, ADDING to whatever's
		// already alive — previous waves must NOT despawn. So: no KillBots, no
		// count reset. Instead, for the duration of this call, disable the
		// active-cap brake (nActiveCount=0 -> the `nActiveCount>0 && ...` guard is
		// skipped) and force bulk drain so the whole roster lands in one pass
		// (a single-spawn factory would otherwise drop all but the first bot once
		// bAutoSpawn is restored). Restore all three afterward.
		const unsigned long savedAuto   = f->bAutoSpawn;
		const unsigned long savedBulk   = f->bBulkSpawn;
		const int           savedActive = f->nActiveCount;
		f->bAutoSpawn   = 1;
		f->bBulkSpawn   = 1;
		f->nActiveCount = 0;
		TgBotFactory__ResetQueue::Call(f, nullptr, tableId);
		TgBotFactory__SpawnNextBot::Call(f, nullptr);
		f->nActiveCount = savedActive;
		f->bBulkSpawn   = savedBulk;
		f->bAutoSpawn   = savedAuto;
		n++;
	}
	Log("Adds::Unleash(%d): %d factories\n", tableId, n);
}

void Alarm() {
	if (!s_Game || !s_AlarmOriginator) { Log("Adds::Alarm: no game/originator\n"); return; }
	// Empty FString (Data/Count/Max = 0) avoids the FString allocator hazard.
	TgGame__ActivateAlarm::Call(s_Game, nullptr, s_AlarmOriginator, s_AlarmId, 0, 0, 0);
	Log("Adds::Alarm: fired alarmId=%d\n", s_AlarmId);
}

}  // namespace Adds

namespace Escape {

void Remap(int oldTable, int newTable) {
	s_EscapeRemap[oldTable] = newTable;
}

void PeriodicAlarm(float minSecs, float maxSecs) {
	s_escapeAlarmMin = minSecs;
	s_escapeAlarmMax = maxSecs;
}

}  // namespace Escape

bool IsActive() {
	return Config::GetDifficultyValueId() == GA_G::DIFFICULTY_VALUE_ID_CUSTOM_SUPER_AGENT;
}

void Init(ATgGame* Game) {
	if (!IsActive() || !Game) return;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (!GRI) { Log("Init: no GameReplicationInfo — skipping\n"); return; }

	ActorCache::CacheMapActors();

	// Boss objective: anchor for A + forced to priority 1 so the chain is
	// boss(1) -> A(2) -> B(3) and IsFinalObjective only ever picks B.
	ATgMissionObjective* boss = nullptr;
	for (ATgMissionObjective* obj : ActorCache::MissionObjectives) {
		if (obj && ObjectClassCache::ClassNameContains(obj, "TgMissionObjective_Bot")) { boss = obj; break; }
	}
	if (!boss) { Log("Init: no TgMissionObjective_Bot — not a boss map, skipping\n"); return; }

	// Spawn anchors: A on the boss, B on the TF1 (attacker / dropship) start.
	ATgTeamPlayerStart* attackerStart = nullptr;
	ATgTeamPlayerStart* defenderStart = nullptr;
	for (ATgTeamPlayerStart* start : ActorCache::PlayerStarts) {
		if (!start) continue;
		if (!attackerStart && start->m_nTaskForce == 1) attackerStart = start;
		if (!defenderStart && start->m_nTaskForce == 2) defenderStart = start;
	}
	if (!attackerStart) { Log("Init: no TF1 TgTeamPlayerStart — skipping\n"); return; }

	UClass* cls = ClassPreloader::GetClass("Class TgGame.TgMissionObjective_Proximity");
	if (!cls) { Log("Init: could not resolve TgMissionObjective_Proximity class\n"); return; }

	// Reset runtime state and (re-)author the mission for this match.
	s_Game = Game;
	s_GRI  = GRI;
	s_aWasLocked = true;
	s_bossKilled = false;
	s_aCaptured  = false;
	s_EscapeRemap.clear();
	s_escapeAlarmMin  = 0.0f;
	s_escapeAlarmMax  = 0.0f;
	s_nextEscapeAlarm = 0.0f;
	A.hooks.clear(); A.obj = nullptr;
	B.hooks.clear(); B.obj = nullptr;
	AuthorMission();

	boss->nPriority = 1;

	// Cache geometry + adds factories BEFORE the boss-room factories get re-tabled.
	CacheGeometry(attackerStart, defenderStart);
	CacheAddFactories();

	// SpawnActor refuses bStatic/bNoDelete CDOs; clear them around the spawns.
	AActor* cdo = (AActor*)ObjectCache::Find(
		"TgMissionObjective_Proximity TgGame.Default__TgMissionObjective_Proximity");
	unsigned long savedNoDelete = 0, savedStatic = 0;
	if (cdo) {
		savedNoDelete = cdo->bNoDelete; savedStatic = cdo->bStatic;
		cdo->bNoDelete = 0; cdo->bStatic = 0;
	} else {
		Log("Init: could not find proximity CDO — spawn will likely fail\n");
	}

	SpawnPoint(Game, GRI, cls, boss->Location, boss->Rotation, A);
	SpawnPoint(Game, GRI, cls, attackerStart->Location, attackerStart->Rotation, B);

	if (cdo) { cdo->bNoDelete = savedNoDelete; cdo->bStatic = savedStatic; }

	// Boss is now priority 1 but sits in its stale (last) HUD slot — fix it.
	ReorderBossFirst(GRI, boss);

	Log("Init complete: boss priority=1, A=%p, B=%p\n", (void*)A.obj, (void*)B.obj);
}

bool ShouldRunCapture(ATgMissionObjective_Proximity* Obj) {
	if (!IsActive() || !Obj) return true;
	CapturePoint* p = (Obj == A.obj) ? &A : (Obj == B.obj) ? &B : nullptr;
	if (!p) return true;   // not one of our points — don't interfere
	// Every Super Agent point requires real humans on it. Henchmen/pets are TF1
	// attackers too, so without this they'd drive (and B, the win point, could be
	// captured by) a henchman alone.
	return p->requireAllPlayers ? AllHumansPresent(Obj) : AtLeastOneHumanPresent(Obj);
}

// Driven off A's Tick (A.obj ticks every frame for the whole match, even after
// capture and even when the capture math is frozen — the dispatcher calls this
// unconditionally). All per-frame mission logic lives here.
void OnCaptureTick(ATgMissionObjective_Proximity* Obj) {
	if (!IsActive() || Obj != A.obj || !A.obj) return;

	// Boss killed = A's lock flag flips 1 -> 0 (boss's UnlockObjective).
	if (s_aWasLocked && !A.obj->r_bIsLocked) {
		s_aWasLocked = false;
		if (!s_bossKilled) { s_bossKilled = true; Log("boss killed (A unlocked)\n"); OnBossKilled(); }
	}

	// Capture-% hooks: fire each once, the first time the bar crosses its pct.
	const float pct = A.obj->m_fTimeToCapture > 0.0f
		? A.obj->m_fCurrCaptureTime / A.obj->m_fTimeToCapture : 0.0f;
	for (CapturePoint::Hook& h : A.hooks) {
		if (!h.fired && pct >= h.pct) {
			h.fired = true;
			Log("hook fired at %.0f%%\n", h.pct * 100.0f);
			if (h.action) h.action();
		}
	}

	// A captured (status 8 = TGMOS_ATTACKER_CAPTURED) -> open the path back to B.
	if (!s_aCaptured && A.obj->r_eStatus == 8) {
		s_aCaptured = true;
		Log("A captured\n");
		OnACaptured();
	}

	// Escape phase: fire a GLOBAL alarm at a random interval to keep the run-back
	// tense — players get ambushed from whichever alarm factory is nearest them.
	if (s_aCaptured && s_escapeAlarmMax > 0.0f && s_Game && s_Game->WorldInfo) {
		const float now = s_Game->WorldInfo->TimeSeconds;
		if (now >= s_nextEscapeAlarm) {
			FireGlobalAlarm();
			s_nextEscapeAlarm = now + RandRange(s_escapeAlarmMin, s_escapeAlarmMax);
		}
	}
}

}  // namespace SuperAgent
