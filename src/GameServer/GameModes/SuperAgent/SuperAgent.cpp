#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Audio/BotVoice/BotVoice.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgGame/ActivateAlarm/TgGame__ActivateAlarm.hpp"
#include "src/GameServer/TgGame/TgAIController/InitBehavior/TgAIController__InitBehavior.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <map>
#include <set>
#include <vector>

namespace {

const char* kChannel = "superagent";

// Boss-room "alarm adds" factories are identified by this spawn table id at map
// load; a capture-% hook re-tables them (e.g. to 41 = bosses) and unleashes.
constexpr int kBossAddsTable = 59;

// Boss-room INITIAL adds (pre-spawned, normal non-alarm factories) always carry
// this spawn table id — inside the room, so never the outside factory.
constexpr int kBossInitialAddsTable = 46;

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

// The NORMAL (non-pet, non-alarm) factory whose spawn nav point sits nearest a
// boss-room ENTRANCE volume (one-way ATgModifyPawnPropertiesVolume facing the
// boss — see CacheAddFactories). Driven only by Adds::UnleashOutside: its bots
// spawn outside and march in toward A. s_EntranceVolume is the volume that won
// the pairing — leg 1 of the march aims THERE, because pathfinding straight to
// A can route a bot into the one-way EXIT, which blocks inward movement and
// strands it. From the entrance doorway the inward path is the natural one.
ATgBotFactory* s_OutsideFactory = nullptr;
ATgModifyPawnPropertiesVolume* s_EntranceVolume = nullptr;

// Bots on leg 1 (spawn -> entrance) of the outside march, keyed by pawnId.
// Pointers re-validated by r_nPawnId on use (recycled-slot guard, same pattern
// as SpawnNextBot's escort drones). OnCaptureTick retargets each to A once it
// is within kEntranceReachDist of the entrance volume, and REVERTS the seeded
// quarry aggro there so normal behavior picks its own targets inside.
// pendingSwitchSecs: targetSwitchSecs to arm at the doorway handoff (0 = none).
// Target switching must NOT run during the march — SetTargetActor gives the
// behavior a combat target, which is what appears to hold it out of the patrol
// phase (the reason kSeedQuarryAggro is off). Switching only starts once the
// bot is inside and the patrol drive has done its job.
struct OutsideMarcher {
	ATgPawn* pawn; ATgPawn* quarry; int quarryId; float pendingSwitchSecs;
	float nextPaceReassert;   // patrol actions re-apply their own pace; re-force run
};
constexpr float kPaceReassertSecs = 0.5f;
std::map<int, OutsideMarcher> s_OutsideMarchers;
constexpr float kEntranceReachDist = 600.0f;
constexpr float kQuarryThreatSeed  = 1000.0f;

// Aggro seeding (SetTargetActor + AddThreat on the human nearest A) did NOT
// move the elites either (playtest 2026-07-20) — and a held target may be what
// keeps their behavior out of the patrol phase, so it's off. The patrol drive
// below replaced it; flip this to re-enable for experiments.
constexpr bool kSeedQuarryAggro = false;

// Patrol anchor INSIDE the boss room for the outside march — the drive that
// works for trigger-blind behaviors: patrol is the core idle behavior every
// bot runs. Must be a BAKED nav point (a runtime-spawned node isn't linked
// into the nav network, pathfinding to it fails); the boss-room adds
// factories' LocationList navs are in the room. Cached at Init.
ANavigationPoint* s_BossRoomNav = nullptr;

// Bots running the erratic target-switch tell (UnleashOpts::targetSwitchSecs),
// keyed by pawnId. Re-validated by r_nPawnId, dropped when the pawn dies.
struct TargetSwitcher { ATgPawn* pawn; float interval; float nextSwitch; };
std::map<int, TargetSwitcher> s_TargetSwitchers;

// A DRIPPED UnleashOutside (spawnDelay>0): the factory trickles the roster out
// over time, so its bots can't be staged in one synchronous pass — instead each
// capture tick (ProcessPendingMarches) stages whichever bots have appeared
// since. `seen` holds every pawnId already staged (seeded with the pre-existing
// bots so a later unleash never re-routes survivors). Leader/escort wiring is
// deferred: a follower can drip in before its leader, so it waits in
// `waitingEscorts` until the leader appears. Expires once the factory queue is
// drained and no new bot has appeared for kMarchDripGrace.
constexpr float kMarchDripGrace = 3.0f;
struct PendingMarch {
	ATgBotFactory*                        f;
	std::vector<SuperAgent::UnleashGroup> groups;
	AActor*                               leg1;
	ATgPawn*                  quarry;
	std::set<int>             seen;
	ATgPawn*                  leader;
	std::vector<int>          waitingEscorts;  // escort pawnIds awaiting the leader
	bool                      alerted;
	int                       tableId;
	float                     lastActivity;
};
std::vector<PendingMarch> s_pendingMarches;

// Bots barking VO on a timer (UnleashOpts::voCueIds), keyed by pawnId.
// r_nBotSoundCueId is a REPNOTIFY (TgPawn.uc:861, in the V2 rep list): the
// client fires PlayBotSound(Abs(id)) only when the replicated value CHANGES,
// so replaying the same slot back to back needs the value to differ. We
// alternate the SIGN — which is exactly why the UC takes Abs() (TgPawn.uc:5702).
struct Barker {
	ATgPawn* pawn;
	std::vector<int> cues;
	float minSecs;
	float maxSecs;
	float nextBark;
	int   sign;      // flips every bark so repnotify always re-fires
	float pitch;     // 0 = plain replicated path; else Kismet_ClientPlaySound
};
std::map<int, Barker> s_Barkers;

// Escape-phase spawn-table remap (current table -> escape table), authored in
// AuthorMission(). Applied to every normal factory when A is captured.
std::map<int, int> s_EscapeRemap;

// Escape-phase periodic GLOBAL alarm, authored via Escape::PeriodicAlarm().
// Fires at a random interval in [min,max]; max<=0 disables. s_nextEscapeAlarm is
// the absolute WorldInfo.TimeSeconds of the next scheduled alarm.
float s_escapeAlarmMin  = 0.0f;
float s_escapeAlarmMax  = 0.0f;
float s_nextEscapeAlarm = 0.0f;

// Heads-up toast this long before each escape global alarm fires, so the ambush
// doesn't feel like it came from nowhere. One warning per scheduled alarm.
constexpr float kEscapeAlarmWarnSecs = 5.0f;
bool s_escapeAlarmWarned = false;

// Weighted pool of escape-alarm flavours (Escape::PeriodicAlarm overload). Empty
// = legacy single-table behaviour. s_pendingWaveIdx is the wave chosen for the
// NEXT alarm — locked in when the alarm is scheduled so the telegraph toast can
// show that wave's text; -1 = legacy/none.
std::vector<SuperAgent::Escape::AlarmWave> s_escapeWaves;
int s_pendingWaveIdx = -1;

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

// Roll one wave index from s_escapeWaves by weight. -1 if the pool is empty or
// every weight is non-positive.
static int PickEscapeWave() {
	float total = 0.0f;
	for (const SuperAgent::Escape::AlarmWave& w : s_escapeWaves) if (w.weight > 0.0f) total += w.weight;
	if (total <= 0.0f) return -1;
	float r = RandRange(0.0f, total);
	for (size_t i = 0; i < s_escapeWaves.size(); i++) {
		if (s_escapeWaves[i].weight <= 0.0f) continue;
		r -= s_escapeWaves[i].weight;
		if (r <= 0.0f) return (int)i;
	}
	return (int)s_escapeWaves.size() - 1;   // FP slop guard
}

// Schedule the next escape alarm and lock in which wave it will be, so the
// telegraph (fired kEscapeAlarmWarnSecs earlier) matches the wave that lands.
static void ArmNextEscapeAlarm(float now) {
	s_nextEscapeAlarm   = now + RandRange(s_escapeAlarmMin, s_escapeAlarmMax);
	s_pendingWaveIdx    = PickEscapeWave();
	s_escapeAlarmWarned = false;
}

// UE3 rotator (65536 units = 360°) -> unit direction vector.
FVector RotToDir(const FRotator& r) {
	const float k = 6.2831853f / 65536.0f;
	const float cp = cosf(r.Pitch * k), sp = sinf(r.Pitch * k);
	FVector v;
	v.X = cp * cosf(r.Yaw * k);
	v.Y = cp * sinf(r.Yaw * k);
	v.Z = sp;
	return v;
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

// Aggro seed for the outside march: the living human nearest point A (they're
// holding it). Null when nobody qualifies (e.g. mid team wipe).
ATgPawn* PickQuarry() {
	if (!SuperAgent::A.obj) return nullptr;
	ATgRepInfo_TaskForce* tf1 = GTeamsData.Attackers;
	if (!tf1) return nullptr;
	ATgPawn* quarry = nullptr;
	float best = 0.0f;
	for (int i = 0; i < tf1->m_TeamPlayers.Count; i++) {
		ATgRepInfo_Player* pri = tf1->m_TeamPlayers.Data[i].pPrep;
		if (!pri || pri->bBot) continue;
		ATgPawn* pw = pri->r_PawnOwner;
		if (!IsHumanPawn(pw) || pw->Health <= 0) continue;
		const float d = Dist2(pw->Location, SuperAgent::A.obj->Location);
		if (!quarry || d < best) { quarry = pw; best = d; }
	}
	return quarry;
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

// Fired once, when the boss dies (A unlocks). Move player respawns up to the
// boss side so deaths during the capture phase respawn nearby.
//
// The boss-room exit gate is deliberately NOT flipped here — see OnACaptured.
// Flipping it open at boss-kill leaves it blocking TF2 for the whole capture
// phase, and the outside-unleash adds spend that phase pathing INTO the flipped
// wall and piling up in the doorway (bodyblocking any player shoved into it).
// Disabling/moving the gate is a dead end (playtest 2026-07-21: SetCollision,
// SetCollisionType, and teleporting it below the map all left the block intact
// — it isn't standard actor collision). Instead we leave the gate in its
// original orientation (blocking the still-boxed-in players, who have no reason
// to leave mid-capture) so the adds path in through the exit freely, and only
// flip it open at capture-complete when players actually need to get out.
void OnBossKilled() {
	FlipSpawnPoints();
	SuperAgent::Log("boss killed: spawn points flipped (exit gate flip deferred to A capture)\n");
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
		// The outside factory hosts the CUSTOM UnleashOutside bots (the Agenderp +
		// its escort, small waves, etc.). KillBots+respawn here would wipe those
		// hand-authored enemies AND re-spawn the raw table roster (a bare helot +
		// field medics) with none of the opts/behavior/march — polluting the
		// escape. Leave it untouched so the custom bots survive the capture; it's
		// bRespawn=0 and non-alarm, so it stays inert for the rest of the match.
		if (f == s_OutsideFactory) continue;

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


// Fired once, when A is captured. Open the path back to the original spawn (B),
// refresh every normal factory's roster (harder, per the escape remap), and
// start the periodic escape alarm clock.
void OnACaptured() {
	// Now — not at boss-kill — flip the boss-room exit gate open for the (TF1)
	// players so they can leave for the escape. Deferring to here keeps the gate
	// blocking players (not the outside adds) through the whole capture phase, so
	// the adds never pile up in the exit doorway. See OnBossKilled.
	for (ATgTeamBlocker* b : s_DefenderBlockers) FlipBlocker(b);
	for (ATgDoorMarker*  d : s_DefenderMarkers)  FlipMarker(d);
	for (ATgTeamBlocker* b : s_ResidualBlockers) FlipBlocker(b);
	for (ATgDoorMarker*  d : s_ResidualMarkers)  FlipMarker(d);
	SuperAgent::Log("A captured: flipped %zu defender + %zu residual blockers, %zu defender + %zu residual markers\n",
		s_DefenderBlockers.size(), s_ResidualBlockers.size(),
		s_DefenderMarkers.size(), s_ResidualMarkers.size());
	// Capture phase over — drop any in-progress dripped march and PARK the outside
	// factory (bAutoSpawn=0) so a half-finished drip can't dribble unstaged bots
	// into the escape. RespawnAllFactories skips this factory (keeps the custom
	// Agenderp/escort alive), so parking is what stops it for good.
	s_pendingMarches.clear();
	if (s_OutsideFactory) s_OutsideFactory->bAutoSpawn = 0;
	RespawnAllFactories();
	// Arm the first periodic global alarm at a random offset (also picks the
	// wave that first alarm will be, so its telegraph text is right).
	const float now = (s_Game && s_Game->WorldInfo) ? s_Game->WorldInfo->TimeSeconds : 0.0f;
	ArmNextEscapeAlarm(now);
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
// 25% hook), EVERY alarm-gated factory on the map (driven by the global escape
// alarm) plus a single alarm originator/id for the siren, and the "outside"
// factory: the normal factory nearest the boss objective, for UnleashOutside.
void CacheAddFactories(ATgMissionObjective* boss) {
	s_AddFactories.clear();
	s_AlarmFactories.clear();
	s_AlarmOriginator = nullptr;
	s_AlarmId = 0;
	s_OutsideFactory = nullptr;
	s_EntranceVolume = nullptr;
	s_BossRoomNav = nullptr;
	s_OutsideMarchers.clear();
	s_TargetSwitchers.clear();
	s_pendingMarches.clear();
	s_Barkers.clear();

	// Outside-factory anchor: the boss-room ENTRANCE. There is no team blocker
	// on it (it's freely walkable inward) — the entrance is marked by a ONE-WAY
	// volume: ATgModifyPawnPropertiesVolume with m_bOneWayMovement whose
	// m_vOnewWay (sic — FRotator) faces the boss objective. Defender spawn
	// rooms use one-way volumes too, ALSO facing the boss (their exits point
	// out into the map), so facing alone can't separate them. What can: the
	// real entrance sits right beside the boss-room EXIT (the anti-facing
	// volume), while spawn-room exits sit elsewhere. So: collect facing
	// candidates + anti-facing exits, then keep the candidate nearest an exit.
	// Each volume is logged so a wrong resolve is diagnosable from the Init log.
	std::vector<ATgModifyPawnPropertiesVolume*> entrances;
	if (boss) {
		std::vector<ATgModifyPawnPropertiesVolume*> exits;
		for (UObject* obj : ObjectCache::FindAllByClassSubstr("TgModifyPawnPropertiesVolume")) {
			ATgModifyPawnPropertiesVolume* v = (ATgModifyPawnPropertiesVolume*)obj;
			if (!v || !v->m_bOneWayMovement) continue;
			const FVector dir = RotToDir(v->m_vOnewWay);
			const float tx = boss->Location.X - v->Location.X;
			const float ty = boss->Location.Y - v->Location.Y;
			const float tz = boss->Location.Z - v->Location.Z;
			const float len = sqrtf(tx * tx + ty * ty + tz * tz);
			if (len <= 1.0f) continue;
			const float dot = (dir.X * tx + dir.Y * ty + dir.Z * tz) / len;
			// ±60° leeway on both cones — circular boss rooms skew the angles.
			const bool facing = dot > 0.5f;
			const bool exit   = dot < -0.5f;
			SuperAgent::Log("one-way volume %p: dot=%.2f dist=%.0f -> %s\n",
				(void*)v, dot, len,
				facing ? "ENTRANCE candidate" : (exit ? "EXIT" : "rejected"));
			if (facing) entrances.push_back(v);
			if (exit)   exits.push_back(v);
		}
		// Multiple candidates (real entrance + spawn-room exits): keep the one
		// nearest a boss-room exit volume.
		if (entrances.size() > 1 && !exits.empty()) {
			ATgModifyPawnPropertiesVolume* pick = nullptr;
			float pickDist = 0.0f;
			for (ATgModifyPawnPropertiesVolume* c : entrances) {
				float d = -1.0f;
				for (ATgModifyPawnPropertiesVolume* x : exits) {
					const float dx = Dist2(c->Location, x->Location);
					if (d < 0.0f || dx < d) d = dx;
				}
				SuperAgent::Log("entrance candidate %p: distToExit=%.0f\n",
					(void*)c, sqrtf(d));
				if (!pick || d < pickDist) { pick = c; pickDist = d; }
			}
			entrances.clear();
			entrances.push_back(pick);
			SuperAgent::Log("entrance resolved: %p (%.0f from exit)\n",
				(void*)pick, sqrtf(pickDist));
		}
	}

	float outsideBest = 0.0f;
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
		// Outside-factory candidate: normal population factory only — never an
		// alarm/adds one (those are already spoken for by Unleash/FireGlobalAlarm)
		// and never a boss-room initial-adds one (table 46: pre-spawned adds
		// INSIDE the room). Distance is measured from the factory's LocationList
		// nav points — bots spawn THERE, the factory actor itself can sit
		// anywhere on the map — to the nearest entrance-candidate volume. A
		// factory with no usable LocationList entry can't spawn and is skipped.
		if (!entrances.empty() && !f->bSpawnOnAlarm &&
		    f->nSpawnTableId != kBossAddsTable &&
		    f->nSpawnTableId != kBossInitialAddsTable) {
			for (int i = 0; i < f->LocationList.Num(); i++) {
				ANavigationPoint* nav =
					f->LocationList.Data ? f->LocationList.Data[i] : nullptr;
				if (!nav) continue;
				for (ATgModifyPawnPropertiesVolume* v : entrances) {
					const float d = Dist2(nav->Location, v->Location);
					if (!s_OutsideFactory || d < outsideBest) {
						s_OutsideFactory = f;
						s_EntranceVolume = v;
						outsideBest = d;
					}
				}
			}
		}
	}
	// Boss-room patrol anchor: the adds factories' nav nearest the boss.
	if (boss) {
		float best = 0.0f;
		for (ATgBotFactory* af : s_AddFactories) {
			if (!af) continue;
			for (int i = 0; i < af->LocationList.Num(); i++) {
				ANavigationPoint* nav =
					af->LocationList.Data ? af->LocationList.Data[i] : nullptr;
				if (!nav) continue;
				const float d = Dist2(nav->Location, boss->Location);
				if (!s_BossRoomNav || d < best) { s_BossRoomNav = nav; best = d; }
			}
		}
	}

	SuperAgent::Log("factories: %zu boss-room (table %d), %zu alarm-gated (global), "
		"%zu entrance volume(s), outside=%p (table %d, nav %.0f from entrance), "
		"boss-room patrol nav=%p\n",
		s_AddFactories.size(), kBossAddsTable, s_AlarmFactories.size(),
		entrances.size(), (void*)s_OutsideFactory,
		s_OutsideFactory ? s_OutsideFactory->nSpawnTableId : 0,
		s_OutsideFactory ? sqrtf(outsideBest) : 0.0f,
		(void*)s_BossRoomNav);
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
		// Human-only capture. GetNearByPlayersTaskforce counts EVERY pawn in the
		// proxy, so an enemy AI bot (TF2) standing in the cylinder reads as a
		// "defender" — bDefenderFound stays true, so CalculateNearByPlayers never
		// reaches `currStatus = 8` and the bar pins at 100% forever (the escape
		// phase swarms B with bots). m_bIgnoreNonPlayers makes ShouldIgnoreActor
		// skip non-hacked, non-henchman bots, so only humans/henchmen count —
		// matching the mode's human-only capture intent.
		Obj->s_CollisionProxy->m_bIgnoreNonPlayers = 1;
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

void CapturePoint::oneOf(float pct, std::vector<Event> events) {
	// Wrap the weighted roll in a normal one-shot hook — reuses the fire-once
	// machinery, so "one per mission" is automatic. The roll runs when the hook
	// fires (crossing `pct`), not now.
	at(pct, [events = std::move(events)]() {
		float total = 0.0f;
		for (const Event& e : events) if (e.weight > 0.0f) total += e.weight;
		if (total <= 0.0f) { Log("oneOf: no positive-weight events — nothing fired\n"); return; }
		float r = RandRange(0.0f, total);
		for (size_t i = 0; i < events.size(); i++) {
			if (events[i].weight <= 0.0f) continue;
			r -= events[i].weight;
			if (r <= 0.0f) {
				Log("oneOf: rolled event %zu (weight %.1f / %.1f)\n", i, events[i].weight, total);
				if (events[i].action) events[i].action();
				return;
			}
		}
		// FP slop: the roll can graze past the last boundary — fire the last
		// positive-weight event so a valid roll never silently no-ops.
		for (size_t i = events.size(); i-- > 0; ) {
			if (events[i].weight > 0.0f) { if (events[i].action) events[i].action(); return; }
		}
	});
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

// Shared per-factory unleash policy, used by Unleash() and UnleashOutside().
// Spawns `tableId`'s roster from `f`, ADDING to whatever's alive. spawnDelay<=0
// dumps the FULL roster in one frame (instant, the default); spawnDelay>0 DRIPS
// it out one bot every `spawnDelay` seconds.
static void UnleashFactory(ATgBotFactory* f, int tableId, float spawnDelay = 0.0f) {
	if (tableId > 0) f->nSpawnTableId = tableId;
	// SpawnBotById drops every bot from a factory whose nPriority>0 and
	// != s_nCurrentPriority. The boss-room factories are nPriority=1 (boss
	// phase), so once the mission advances they spawn nothing. Set the "any
	// phase" sentinel -1 so they fire in every phase from here on.
	f->nPriority = -1;
	// Keep these adds (incl. the juggernaut) alive through a team wipe.
	// TgBotEncounterVolume.CheckTouching despawns a factory's bots (60s
	// EndEncounter -> Despawn -> KillBots) when no human is left in the volume,
	// but ONLY if the factory's bRespawn is set. Real bosses (Shrike) survive
	// because they aren't bRespawn factory bots; clear it here so our unleashed
	// bosses get the same treatment instead of vanishing on a wipe.
	f->bRespawn = 0;

	// Whole roster ADDS to whatever's alive — no KillBots, no count reset — and
	// the active cap is disabled (nActiveCount=0) so the full roster lands
	// regardless of what previous waves left alive.
	if (spawnDelay > 0.0f) {
		// DRIP: spawn one bot now, then one every `spawnDelay` seconds until the
		// roster is exhausted. Leave the factory in drip mode (bAutoSpawn=1,
		// bBulkSpawn=0, fSpawnDelay=spawnDelay, cap off) so SpawnNextBot's own
		// self-scheduled timer keeps popping the finite queue (bRespawn=0 → it
		// drains once and stops). NOT restored — the later timers need these; a
		// subsequent unleash re-tables + re-kicks the factory anyway (its
		// ResetQueue replaces any not-yet-spawned remainder of this wave, so keep
		// the delay short relative to the gap between waves on the same factory).
		f->bAutoSpawn   = 1;
		f->bBulkSpawn   = 0;
		f->fSpawnDelay  = spawnDelay;
		f->nActiveCount = 0;
		TgBotFactory__ResetQueue::Call(f, nullptr, tableId);
		TgBotFactory__SpawnNextBot::Call(f, nullptr);
		return;
	}

	// INSTANT (default): force bulk drain so the whole roster lands in one pass
	// (a single-spawn factory would otherwise drop all but the first bot once
	// bAutoSpawn is restored), then restore all three so the factory stays
	// pristine.
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
}

void Unleash(int tableId, float spawnDelay) {
	// ONE factory per call, picked at random each time. Maps vary wildly in how
	// many boss-room alarm spawn points they bake in (1 vs 3+); unleashing at all
	// of them scaled the boss/juggernaut count with map geometry instead of the
	// authored wave design.
	std::vector<ATgBotFactory*> valid;
	for (ATgBotFactory* f : s_AddFactories) {
		if (f) valid.push_back(f);
	}
	if (valid.empty()) { Log("Adds::Unleash(%d): no factories\n", tableId); return; }
	ATgBotFactory* f = valid[rand() % valid.size()];

	UnleashFactory(f, tableId, spawnDelay);

	Log("Adds::Unleash(%d, delay=%.2f): picked factory %p (%zu candidates)\n",
		tableId, spawnDelay, (void*)f, valid.size());
}

// Apply the optional per-spawn modifiers to one freshly spawned bot.
static void ApplyUnleashOpts(ATgPawn* pawn, const UnleashOpts& o) {
	if (o.scale > 0.0f) {
		// DrawScale is CPF_Net, so clients rescale the mesh (and its hit
		// zones) too. The collision cylinder does NOT follow DrawScale —
		// scale it explicitly via the native.
		pawn->SetDrawScale(pawn->DrawScale * o.scale);
		UCylinderComponent* cyl = pawn->CylinderComponent;
		if (cyl) {
			const float oldHalf   = cyl->CollisionHeight;
			const float oldRadius = cyl->CollisionRadius;
			pawn->SetCollisionSize(oldRadius * o.scale, oldHalf * o.scale);

			// SetCollisionSize alone does NOT hold: SpawnBotById's
			// ApplyBotCollisionData installs a whole companion set from the
			// asm row, and UnCrouch() restores the cylinder from
			// m_fStandingHeight/Radius — so the first crouch transition
			// silently reverts the resize to the UNSCALED size (symptom: the
			// drawn cylinder is short relative to the scaled mesh). Scale the
			// full set so every restore path agrees with the new size.
			pawn->m_fStandingHeight       = pawn->m_fStandingHeight * o.scale;
			pawn->m_fStandingRadius       = pawn->m_fStandingRadius * o.scale;
			pawn->CrouchHeight            = pawn->CrouchHeight * o.scale;
			pawn->CrouchRadius            = pawn->CrouchRadius * o.scale;
			// AI-aim "tight hit body" (separate TargetCylinder component) —
			// without this, shots aim at a small box inside a huge model.
			pawn->m_fTargetCylinderHeight = pawn->m_fTargetCylinderHeight * o.scale;
			pawn->m_fTargetCylinderRadius = pawn->m_fTargetCylinderRadius * o.scale;
			// Eye height drives the AI LOS trace origin; keep it above the
			// (now taller) cylinder top or the bot traces from inside itself.
			pawn->BaseEyeHeight           = oldHalf * o.scale + 1.0f;
			pawn->EyeHeight               = pawn->BaseEyeHeight;

			// Location is the cylinder CENTER and SpawnNextBot lifted this bot
			// by the UNSCALED halfHeight+5, so a grown cylinder's bottom would
			// sit below the floor. Lift by the half-height delta.
			const float lift = oldHalf * o.scale - oldHalf;
			if (lift > 0.0f) {
				FVector loc = pawn->Location;
				loc.Z += lift;
				pawn->SetLocation(loc);
			}
		}
	}
	if (o.speedMult > 0.0f) {
		pawn->GroundSpeed = pawn->GroundSpeed * o.speedMult;   // CPF_Net
	}
	if (o.damageMult > 0.0f) {
		// Canonical outgoing knob: TgEffectDamage.uc:120 AND TgEffectHeal.uc:71
		// both multiply by it, so this scales damage and healing alike.
		// NOTE: this OVERWRITES the difficulty/BBM baseline that
		// TgPawn__InitializeDefaultProps.cpp:214 seeds
		// (combinedMultiplier / 1.15f) — it is an absolute value, not a
		// multiplier on top of it.
		pawn->s_fDamageAdjustment = o.damageMult;
	}
	if (o.healthMult > 0.0f) {
		// Must go through the PROPERTY system, not the engine fields. Writing
		// Health/r_nHealthMaximum directly leaves props 51/304 holding the
		// original spawn HP — then the first effect that touches health
		// (a heal!) fans out from that stale m_fRaw and collapses the pawn to
		// its pre-multiplier HP with a multiplied bar. Playtested 2026-07-21:
		// healthMult=100 + a medic heal = instant 1%-HP Agenderp.
		//
		// MAX before CURRENT: TGPID_HEALTH (51)'s fanout clamps against
		// r_nHealthMaximum, which TGPID_HEALTH_MAX (304)'s fanout writes.
		// m_fMaximum is a hard clamp inside ApplyProperty, so leave 10x
		// headroom or later buffs get clipped — same trap as
		// TgPawn__InitializeDefaultProps.cpp:160-171.
		const float newMax = (float)pawn->r_nHealthMaximum * o.healthMult;
		pawn->AddProperty(GA_PROPERTY::TGPID_HEALTH_MAX, newMax, newMax, 0, newMax * 10.0f);
		pawn->AddProperty(GA_PROPERTY::TGPID_HEALTH,     newMax, newMax, 0, newMax * 10.0f);
		pawn->HealthMax = (int)newMax;   // base UE3 field, not written by the fanout
	}
	// ---- cheater knobs ----
	if (o.rotationRateYaw > 0) {
		// The aim path caps correction at +-14000 yaw units per adjustment
		// ONLY in the spread/lag aim branches; actual turn speed is this.
		pawn->RotationRate.Yaw = o.rotationRateYaw;
	}
	if (o.accuracy > 0.0f) {
		pawn->AddProperty(GA_PROPERTY::TGPID_ACCURACY, o.accuracy, o.accuracy, 0, 1);
	}
	if (o.setAccuracyLoss) {
		pawn->AddProperty(GA_PROPERTY::TGPID_ACCURACY_LOSS_ON_SHOOT,
			o.accuracyLossOnShoot, o.accuracyLossOnShoot, 0, 1);
		pawn->AddProperty(GA_PROPERTY::TGPID_ACCURACY_LOSS_ON_JUMP,
			o.accuracyLossOnJump, o.accuracyLossOnJump, 0, 1);
		pawn->AddProperty(GA_PROPERTY::TGPID_ACCURACY_LOSS_MAX,
			o.accuracyLossMax, o.accuracyLossMax, 0, 1);
	}
	if (o.jumpMult > 0.0f)       pawn->JumpZ      = pawn->JumpZ * o.jumpMult;          // CPF_Net
	if (o.airControlMult > 0.0f) pawn->AirControl = pawn->AirControl * o.airControlMult;
	if (o.frictionMult > 0.0f)   pawn->r_fFrictionMultiplier = o.frictionMult;         // CPF_Net

	// Retail dev-console cheat flags, both checked SERVER-side so setting them
	// on a bot just works — no RPC, so the DevCheatRpc console gate isn't
	// involved. Both resolve the pawn the same way our devices are wired:
	//   noCooldowns    -> TgDevice.StartCooldown reads TgPawn(Instigator) and
	//                     early-returns before registering the cooldown timer.
	//   infinitePower  -> TgPawn.ConsumePowerPool skips the SetProperty(243)
	//                     drain entirely.
	// Instigator is set on BOTH the DB loadout (GiveDevicesFromBotConfig) and
	// swapped devices (EquipInHandDevice), so this covers all of the bot's gear.
	if (o.noCooldowns)   pawn->m_bCheatNoRecharge   = 1;
	if (o.infinitePower) pawn->m_bCheatUseNoEnergy  = 1;

	// Cosmetics. CosmeticEquip::ApplyItemToBot self-guards on pawn class and
	// no-ops for bot-mesh classes, so this is safe to call unconditionally.
	for (size_t ci = 0; ci < o.cosmeticItemIds.size(); ci++) {
		CosmeticEquip::ApplyItemToBot(pawn, o.cosmeticItemIds[ci]);
	}

	if (o.name && o.name[0] && pawn->PlayerReplicationInfo) {
		// PRI PlayerName is the replicated display name. eventSetPlayerName's
		// UC body copies the string engine-side, so our GAllocator buffer is
		// ours to free afterward (FString hazard rule: never let the SDK
		// FString own heap it didn't allocate).
		FString s;
		const int len = (int)wcslen(o.name) + 1;
		s.Data = (wchar_t*)GAllocator::Malloc(len * sizeof(wchar_t));
		s.Count = s.Max = len;
		wcscpy(s.Data, o.name);
		pawn->PlayerReplicationInfo->eventSetPlayerName(s);
		GAllocator::Free(s.Data);
		s.Data = nullptr; s.Count = s.Max = 0;
	}
}

// Swap a spawned bot's AI: re-run InitBehavior with a DIFFERENT bot id's
// config. SpawnBotById does exactly this call at spawn time with the bot's own
// config (m_BotPointers[botId], the client-marshalled AmBot row), so feeding it
// a donor id gives this controller the donor's brain while the pawn keeps its
// own body and stats. `factory` is passed through because InitBehavior stamps
// m_pFactory from its factory arg — passing the real one keeps BotDied wiring.
static void SwapBehavior(ATgAIController* aic, int donorBotId, ATgBotFactory* factory) {
	auto it = CAmBot__LoadBotMarshal::m_BotPointers.find((uint32_t)donorBotId);
	if (it == CAmBot__LoadBotMarshal::m_BotPointers.end() || it->second == 0) {
		Log("behavior swap: bot %d has no marshalled config — skipped\n", donorBotId);
		return;
	}
	void* donorConfig = (void*)(intptr_t)it->second;
	TgAIController__InitBehavior::CallOriginal(aic, nullptr, donorConfig, factory);
	Log("behavior swap: controller %p now runs bot %d's AI\n", (void*)aic, donorBotId);
}

// Arm a spawned pawn's periodic VO barks from opts. No-op when unconfigured.
// Self-contained (writes only the global s_Barkers map) — safe to call at any
// point after the pawn exists, in any order relative to other staging.
static void RegisterBarker(ATgPawn* pawn, const UnleashOpts& opts) {
	if (opts.voMaxSecs <= 0.0f || opts.voCueIds.empty()) return;
	Barker b = { pawn, opts.voCueIds, opts.voMinSecs, opts.voMaxSecs,
	             s_Game->WorldInfo->TimeSeconds
	                 + RandRange(opts.voMinSecs, opts.voMaxSecs), 1,
	             opts.voPitch };
	s_Barkers[pawn->r_nPawnId] = b;
}

// Apply opts.devices to a spawned pawn (replaces gear per equip point). MUST be
// the LAST thing done to the pawn in its staging block — EquipInHandDevice ends
// in UpdateClientDevices (the SpawnBotById frame trap); uses the crash-safe
// single-device helper, NOT GiveDeviceById.
static void ApplyDeviceSwaps(ATgPawn* pawn, const UnleashOpts& opts) {
	for (size_t di = 0; di < opts.devices.size(); di++) {
		const DeviceSwap& sw = opts.devices[di];
		if (sw.deviceId == 0) continue;
		Log("device swap: equip point %d -> device %d (type %d)\n",
			sw.equipPoint, sw.deviceId, sw.deviceType);
		TgGame__SpawnBotById::EquipInHandDevice(
			pawn, (ATgRepInfo_Player*)pawn->PlayerReplicationInfo,
			sw.deviceId, sw.equipPoint, sw.quality, sw.deviceType);
	}
}

// Periodic GLOBAL escape alarm. Ambush every player from the alarm-gated factory
// nearest to them, capped at wave->maxFactories (0 = unlimited). `wave` picks the
// spawn table (0 = each factory's own default roster) and applies its per-pawn
// overrides to the fresh spawns; nullptr = legacy own-table, no overrides.
// Respects each factory's active cap (no count reset) so ambushers replenish as
// they die rather than piling up. Also fires the native siren/AlarmBots kismet.
static void FireGlobalAlarm(const Escape::AlarmWave* wave) {
	ATgRepInfo_TaskForce* tf1 = GTeamsData.Attackers;
	if (!tf1 || s_AlarmFactories.empty()) return;

	const int tableId = wave ? wave->tableId : 0;
	const int maxFac  = wave ? wave->maxFactories : 0;

	std::vector<ATgBotFactory*> seen;   // dedupe: consider each factory once
	int erupted = 0;                    // factories that actually spawned (maxFac target)
	for (int i = 0; i < tf1->m_TeamPlayers.Count; i++) {
		if (maxFac > 0 && erupted >= maxFac) break;
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
		for (ATgBotFactory* g : seen) if (g == nearest) { already = true; break; }
		if (already) continue;
		seen.push_back(nearest);

		// Skip if it's already at its active cap from a recent ambush.
		if (nearest->nActiveCount > 0 && nearest->nCurrentCount >= nearest->nActiveCount) continue;

		// Snapshot this factory's current bots so the wave's opts hit only the
		// bots THIS eruption spawns, not survivors from a prior alarm.
		std::vector<int> preexisting;
		if (wave && s_Game && s_Game->WorldInfo) {
			for (AController* C = s_Game->WorldInfo->ControllerList; C; C = C->NextController) {
				if (!ObjectClassCache::ClassNameContains(C, "TgAIController")) continue;
				ATgAIController* aic = (ATgAIController*)C;
				if (aic->m_pFactory == nearest && aic->Pawn)
					preexisting.push_back(((ATgPawn*)aic->Pawn)->r_nPawnId);
			}
		}

		nearest->nPriority = -1;   // phase-gate: eligible in the escape phase
		// Spawn the wave and let the factory DRIP it at its own pace — mirror the
		// standard-wave discipline, differing ONLY in the table. The old override
		// path forced bBulkSpawn=1 + nActiveCount=0, dumping the FULL roster in a
		// single frame → enemies appeared all at once (unfair) AND a replication
		// spike lag-hitched the spawn. Left at the factory's natural bBulkSpawn=0,
		// SpawnNextBot spawns one bot and self-schedules the rest at fSpawnDelay,
		// and the active cap stays IN FORCE (replenish-as-they-die, no pile-up).
		// The native ActivateAlarm below is spawn-suppressed, so nothing re-queues
		// the factory; the queue built here (composite-aware via RollSpawnPlan)
		// drains over time. tableId>0 = the wave's override table; 0 = factory
		// default. Restore the table after the override case so the factory stays
		// pristine (the queue is already built, so the drip keeps popping wave
		// bots regardless).
		const int           savedTable = nearest->nSpawnTableId;
		const unsigned long savedAuto  = nearest->bAutoSpawn;
		nearest->bAutoSpawn = 1;
		TgBotFactory__ResetQueue::Call(nearest, nullptr, tableId);
		TgBotFactory__SpawnNextBot::Call(nearest, nullptr);
		nearest->bAutoSpawn = savedAuto;
		if (tableId > 0) nearest->nSpawnTableId = savedTable;
		erupted++;

		// Apply the wave's per-pawn overrides to the bots spawned SO FAR. Escape
		// spawns fight in place — no march/leader/follow, just the stat/gear/
		// cosmetic/bark/behavior overrides. ApplyDeviceSwaps LAST (frame trap).
		// NOTE: the wave now DRIPS (see above), so only the bot(s) already spawned
		// at eruption time are covered here; the rest arrive on later fSpawnDelay
		// timers WITHOUT opts. The active pool uses no opts, so this is currently
		// a no-op — if an opts-bearing escape wave is ever configured, apply the
		// opts from a SpawnNextBot post-hook / per-tick sweep instead.
		if (wave && s_Game && s_Game->WorldInfo) {
			for (AController* C = s_Game->WorldInfo->ControllerList; C; C = C->NextController) {
				if (!ObjectClassCache::ClassNameContains(C, "TgAIController")) continue;
				ATgAIController* aic = (ATgAIController*)C;
				if (aic->m_pFactory != nearest || !aic->Pawn) continue;
				ATgPawn* p = (ATgPawn*)aic->Pawn;
				bool old = false;
				for (int id : preexisting) if (id == p->r_nPawnId) { old = true; break; }
				if (old) continue;
				if (wave->opts.behaviorFromBotId > 0)
					SwapBehavior(aic, wave->opts.behaviorFromBotId, nearest);
				ApplyUnleashOpts(p, wave->opts);
				RegisterBarker(p, wave->opts);
				ApplyDeviceSwaps(p, wave->opts);
			}
		}
	}

	// Siren / AlarmBots kismet for the alarm feel — WITHOUT the native's own bot
	// spawn. The native ActivateAlarm picks the single closest bSpawnOnAlarm
	// factory to the originator and runs ResetQueue+SpawnNextBot on it using THAT
	// factory's baked default table — a rogue "huge default wave" that ignores
	// which wave we rolled and contaminated EVERY alarm (elite squads drowned out
	// by the default, ticks doubled, "silent" waves buried, spawn never matching
	// the telegraph). It picks by closest-to-originator + priority, so which
	// factory (and whether it overlapped our per-player picks) varied run to run
	// — hence the "all over the place" feel. Clear bSpawnOnAlarm on every alarm
	// factory across the call so the native finds none eligible and spawns
	// nothing; the siren / AlarmBots kismet runs earlier in the native (driven by
	// s_SeqEventAlarmBots, independent of these factories) and is unaffected. Our
	// per-player loop above is now the ONLY spawn source, so the spawn always
	// matches the telegraphed wave.
	if (s_Game && s_AlarmOriginator) {
		std::vector<std::pair<ATgBotFactory*, unsigned long>> alarmFlagSaved;
		alarmFlagSaved.reserve(s_AlarmFactories.size());
		for (ATgBotFactory* f : s_AlarmFactories) {
			if (!f) continue;
			const unsigned long savedFlag = f->bSpawnOnAlarm;   // copy: can't bind a ref to a bitfield
			alarmFlagSaved.push_back({ f, savedFlag });
			f->bSpawnOnAlarm = 0;
		}
		TgGame__ActivateAlarm::Call(s_Game, nullptr, s_AlarmOriginator, s_AlarmId, 0, 0, 0);
		for (auto& r : alarmFlagSaved) if (r.first) r.first->bSpawnOnAlarm = r.second;
	}

	SuperAgent::Log("global alarm: %d eruption(s), table=%d maxFac=%d\n",
		erupted, tableId, maxFac);
}

// Match a spawned pawn to its group by bot id. SpawnBotById stamps the bot id
// onto the pawn as r_nProfileId, so no side map is needed. botId 0 = wildcard.
static const UnleashOpts& ResolveGroupOpts(ATgPawn* pawn,
                                           const std::vector<UnleashGroup>& groups) {
	static const UnleashOpts kNone;
	const UnleashOpts* wild = nullptr;
	for (size_t i = 0; i < groups.size(); i++) {
		if (groups[i].botId == pawn->r_nProfileId) return groups[i].opts;
		if (groups[i].botId == 0 && !wild) wild = &groups[i].opts;
	}
	return wild ? *wild : kNone;
}

// Stage ONE freshly-spawned outside bot: behavior swap, march/patrol trigger,
// run pace, per-bot opts, barks, aggro seed, device swaps, marcher/switcher
// tracking. Returns the resolved per-bot opts so the CALLER handles the
// cross-bot leader/escort wiring (which bot leads, who follows). Shared by the
// instant path (synchronous loop) and the drip path (ProcessPendingMarches, one
// bot at a time as they trickle in).
static const UnleashOpts& StageOutsideBot(ATgAIController* aic, ATgPawn* pawn,
		ATgBotFactory* f, const std::vector<UnleashGroup>& groups,
		AActor* leg1, ATgPawn* quarry) {
	const UnleashOpts& opts = ResolveGroupOpts(pawn, groups);
	// Behavior swap FIRST: InitBehavior re-initializes controller behavior state,
	// so every march field below has to be written after it.
	if (opts.behaviorFromBotId > 0) SwapBehavior(aic, opts.behaviorFromBotId, f);
	// Non-escorts march. Trigger fields are the ones SpawnNextBot auto-assigns
	// when a taskforce's active objective is a TgMissionObjective_Bot; A is a
	// proximity point with no objective pawn, so we set them by hand. Leg 1 aims
	// at the ENTRANCE volume, not A — pathing straight to A can route a bot into
	// the one-way exit. m_bInterrupt makes the behavior pick it up immediately.
	const bool escort = opts.followLeader;
	if (!escort) {
		aic->m_pTriggerTarget      = leg1;
		aic->m_pTriggerDestination = leg1;
		aic->m_vTriggerLocation    = leg1->Location;
		aic->m_bInterrupt          = 1;
	}
	// SpawnBotById copies the factory's SafetyLocation onto the controller; a
	// low-health marcher would panic-retreat OUT of the boss room to it. Clear it.
	aic->m_pSafetyLocation = nullptr;
	// Patrol drive: single-point route to the baked in-room nav. Patrol is the
	// phase-0 behavior every bot runs, so this moves the trigger-blind elites too.
	if (s_BossRoomNav && !escort) {
		aic->m_PatrolPath.Clear();
		aic->m_PatrolPath.Add(s_BossRoomNav);
		aic->m_bPatrolLoop = 0;
	}
	// Force RUN pace (356=run, 357=walk, 358=crouch); the patrol/walk anim
	// otherwise reads as a permanent crouch on the sneaky bots.
	aic->eventSetPace(356);
	ApplyUnleashOpts(pawn, opts);
	RegisterBarker(pawn, opts);
	if (quarry) {
		aic->SetTargetActor((AActor*)quarry);
		aic->AddThreat(quarry, kQuarryThreatSeed);
	}
	if (!escort) {
		if (s_EntranceVolume) {
			// Tracked march: target switching is armed at the doorway handoff.
			OutsideMarcher m = { pawn, quarry, quarry ? quarry->r_nPawnId : -1,
			                     opts.targetSwitchSecs,
			                     s_Game->WorldInfo->TimeSeconds + kPaceReassertSecs };
			s_OutsideMarchers[pawn->r_nPawnId] = m;
		} else if (opts.targetSwitchSecs > 0.0f) {
			// No entrance volume -> no march to protect, so arm it now.
			const float now = s_Game->WorldInfo->TimeSeconds;
			TargetSwitcher ts = { pawn, opts.targetSwitchSecs, now + opts.targetSwitchSecs };
			s_TargetSwitchers[pawn->r_nPawnId] = ts;
		}
	}
	// Equipment overrides LAST (EquipInHandDevice ends in UpdateClientDevices,
	// the SpawnBotById frame trap).
	ApplyDeviceSwaps(pawn, opts);
	return opts;
}

// Fire the call-level spawn heads-up once: the FIRST group with alertText wins.
static void FireUnleashAlert(const std::vector<UnleashGroup>& groups) {
	for (size_t gi = 0; gi < groups.size(); gi++) {
		const UnleashOpts& go = groups[gi].opts;
		if (!go.alertText || !go.alertText[0]) continue;
		SendAlert::BroadcastText(go.alertText, /*APT_Normal*/ 1, /*ATT_Detrimental*/ 2,
		                         go.alertSecs);
		return;
	}
}

void UnleashOutside(int tableId, const UnleashOpts& opts) {
	// Single-opts form = one wildcard group.
	std::vector<UnleashGroup> groups;
	UnleashGroup g = { 0, opts };
	groups.push_back(g);
	UnleashOutside(tableId, groups);
}

void UnleashOutside(int tableId, const std::vector<UnleashGroup>& groups) {
	ATgBotFactory* f = s_OutsideFactory;
	if (!f) { Log("Adds::UnleashOutside(%d): no outside factory cached\n", tableId); return; }
	if (!s_Game || !s_Game->WorldInfo) { Log("Adds::UnleashOutside(%d): no world\n", tableId); return; }

	// Wave-level drip rate: FIRST group whose opts.spawnDelay > 0 wins (same
	// "first group wins" rule as alertText). 0 = spawn the whole roster at once.
	float spawnDelay = 0.0f;
	for (size_t gi = 0; gi < groups.size(); gi++) {
		if (groups[gi].opts.spawnDelay > 0.0f) { spawnDelay = groups[gi].opts.spawnDelay; break; }
	}

	// Leg 1 aims at the ENTRANCE volume, not A: pathing straight to A can route a
	// bot into the one-way exit, which blocks inward movement and strands it.
	// OnCaptureTick flips each marcher to A at the doorway.
	AActor* leg1 = s_EntranceVolume ? (AActor*)s_EntranceVolume : (AActor*)A.obj;
	if (!leg1) { Log("Adds::UnleashOutside(%d): no entrance/A to march at\n", tableId); return; }

	// Snapshot the factory's pre-existing bots so only THIS call's spawns get
	// staged — a later unleash must not re-route survivors already inside.
	std::set<int> preexisting;
	for (AController* C = s_Game->WorldInfo->ControllerList; C; C = C->NextController) {
		if (!ObjectClassCache::ClassNameContains(C, "TgAIController")) continue;
		ATgAIController* aic = (ATgAIController*)C;
		if (aic->m_pFactory == f && aic->Pawn)
			preexisting.insert(((ATgPawn*)aic->Pawn)->r_nPawnId);
	}

	// Some behaviors (the elites) have no trigger-move action and ignore the
	// march trigger — the patrol drive in StageOutsideBot moves them instead.
	// Quarry aggro is off by default (kSeedQuarryAggro).
	ATgPawn* quarry = kSeedQuarryAggro ? PickQuarry() : nullptr;

	if (spawnDelay > 0.0f) {
		// DRIP: kick the factory's timed trickle and register a pending march.
		// The roster is still in the queue — nothing to stage now; each bot is
		// staged by ProcessPendingMarches as it appears (per capture tick), so a
		// huge wave walks in one-by-one instead of swarming the room at once.
		UnleashFactory(f, tableId, spawnDelay);
		PendingMarch pm;
		pm.f = f; pm.groups = groups; pm.leg1 = leg1; pm.quarry = quarry;
		pm.seen = preexisting; pm.leader = nullptr; pm.alerted = false;
		pm.tableId = tableId; pm.lastActivity = s_Game->WorldInfo->TimeSeconds;
		s_pendingMarches.push_back(pm);
		Log("Adds::UnleashOutside(%d): DRIP every %.2fs from factory %p (staged per tick)\n",
			tableId, spawnDelay, (void*)f);
		return;
	}

	// INSTANT (default): spawn the whole roster now and stage it synchronously.
	UnleashFactory(f, tableId);
	int pointed = 0;
	ATgPawn* leader = nullptr;                  // first isLeader pawn seen
	std::vector<ATgAIController*> escorts;      // wired to the leader after the loop
	for (AController* C = s_Game->WorldInfo->ControllerList; C; C = C->NextController) {
		if (!ObjectClassCache::ClassNameContains(C, "TgAIController")) continue;
		ATgAIController* aic = (ATgAIController*)C;
		if (aic->m_pFactory != f || !aic->Pawn) continue;
		ATgPawn* pawn = (ATgPawn*)aic->Pawn;
		if (preexisting.count(pawn->r_nPawnId)) continue;
		const UnleashOpts& opts = StageOutsideBot(aic, pawn, f, groups, leg1, quarry);
		if (opts.isLeader && !leader) leader = pawn;
		if (opts.followLeader) escorts.push_back(aic);
		pointed++;
	}
	// Escort wiring — deferred to here because ControllerList order is arbitrary,
	// so a follower can be seen before its leader. m_pOwner is what destination
	// code 310 (SetMovementTarget(m_pOwner)) / SetFormationLocation read.
	if (!escorts.empty()) {
		if (!leader) {
			Log("UnleashOutside(%d): %d escort(s) but NO group set isLeader — "
				"they will not follow anything\n", tableId, (int)escorts.size());
		} else {
			for (size_t ei = 0; ei < escorts.size(); ei++) escorts[ei]->m_pOwner = leader;
			Log("UnleashOutside(%d): %d escort(s) following pawn %d (bot %d)\n",
				tableId, (int)escorts.size(), leader->r_nPawnId, leader->r_nProfileId);
		}
	}
	if (pointed > 0) FireUnleashAlert(groups);

	Log("Adds::UnleashOutside(%d): factory %p, %d bot(s) marching at %s, quarry=%d\n",
		tableId, (void*)f, pointed, s_EntranceVolume ? "entrance (leg 1)" : "A",
		quarry ? quarry->r_nPawnId : -1);
}

// Stage the bots of any in-progress DRIPPED UnleashOutside as they trickle in.
// Called every capture tick. Leader/escort wiring is deferred ACROSS ticks: a
// follower that drips in before its leader waits in `waitingEscorts` and gets
// m_pOwner wired the tick the leader appears. A pending march expires once its
// factory queue is drained and no new bot has appeared for kMarchDripGrace.
static void ProcessPendingMarches(float now) {
	if (s_pendingMarches.empty() || !s_Game || !s_Game->WorldInfo) return;
	for (size_t i = 0; i < s_pendingMarches.size(); ) {
		PendingMarch& pm = s_pendingMarches[i];
		bool leaderJustAppeared = false;
		for (AController* C = s_Game->WorldInfo->ControllerList; C; C = C->NextController) {
			if (!ObjectClassCache::ClassNameContains(C, "TgAIController")) continue;
			ATgAIController* aic = (ATgAIController*)C;
			if (aic->m_pFactory != pm.f || !aic->Pawn) continue;
			ATgPawn* pawn = (ATgPawn*)aic->Pawn;
			const int pid = pawn->r_nPawnId;
			if (pm.seen.count(pid)) continue;
			pm.seen.insert(pid);
			pm.lastActivity = now;
			const UnleashOpts& opts =
				StageOutsideBot(aic, pawn, pm.f, pm.groups, pm.leg1, pm.quarry);
			if (opts.isLeader && !pm.leader) { pm.leader = pawn; leaderJustAppeared = true; }
			if (opts.followLeader) {
				if (pm.leader) aic->m_pOwner = pm.leader;
				else pm.waitingEscorts.push_back(pid);
			}
			if (!pm.alerted) { pm.alerted = true; FireUnleashAlert(pm.groups); }
		}
		// Leader just showed up -> wire the escorts that dripped in before it.
		if (leaderJustAppeared && !pm.waitingEscorts.empty()) {
			for (AController* C = s_Game->WorldInfo->ControllerList; C; C = C->NextController) {
				if (!ObjectClassCache::ClassNameContains(C, "TgAIController")) continue;
				ATgAIController* aic = (ATgAIController*)C;
				if (!aic->Pawn) continue;
				const int pid = ((ATgPawn*)aic->Pawn)->r_nPawnId;
				for (int wid : pm.waitingEscorts)
					if (wid == pid) { aic->m_pOwner = pm.leader; break; }
			}
			pm.waitingEscorts.clear();
		}
		// Expire once the factory queue is drained and the trickle has gone quiet.
		if (pm.f->m_SpawnQueue.Num() == 0 && (now - pm.lastActivity) > kMarchDripGrace) {
			if (!pm.waitingEscorts.empty())
				Log("UnleashOutside(%d): drip ended, %d escort(s) never wired (no leader)\n",
					pm.tableId, (int)pm.waitingEscorts.size());
			Log("Adds::UnleashOutside(%d): drip complete\n", pm.tableId);
			s_pendingMarches.erase(s_pendingMarches.begin() + i);
			continue;
		}
		++i;
	}
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
	s_escapeWaves.clear();
}

void PeriodicAlarm(float minSecs, float maxSecs, const std::vector<AlarmWave>& waves) {
	s_escapeAlarmMin = minSecs;
	s_escapeAlarmMax = maxSecs;
	s_escapeWaves = waves;
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
	s_escapeAlarmWarned = false;
	s_escapeWaves.clear();
	s_pendingWaveIdx = -1;
	A.hooks.clear(); A.obj = nullptr;
	B.hooks.clear(); B.obj = nullptr;
	AuthorMission();

	boss->nPriority = 1;

	// Cache geometry + adds factories BEFORE the boss-room factories get re-tabled.
	CacheGeometry(attackerStart, defenderStart);
	CacheAddFactories(boss);

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

	// Stage any in-progress DRIPPED UnleashOutside spawns as they trickle in
	// (spawnDelay>0). Runs after the hooks so a march a hook just registered
	// starts staging immediately.
	if (s_Game && s_Game->WorldInfo) {
		Adds::ProcessPendingMarches(s_Game->WorldInfo->TimeSeconds);
	}

	// A captured (status 8 = TGMOS_ATTACKER_CAPTURED) -> open the path back to B.
	if (!s_aCaptured && A.obj->r_eStatus == 8) {
		s_aCaptured = true;
		Log("A captured\n");
		OnACaptured();
	}

	// Outside-march leg 2: a marcher that reached the entrance doorway gets
	// retargeted from the entrance volume to A — from there the inward one-way
	// path is the natural route, so it walks in instead of hugging the exit.
	// Stale entries (died, despawned, slot recycled) are dropped by the
	// r_nPawnId re-validation.
	if (!s_OutsideMarchers.empty() && s_EntranceVolume && A.obj &&
	    s_Game && s_Game->WorldInfo) {
		const float now = s_Game->WorldInfo->TimeSeconds;
		ATgPawn* fresh = nullptr;         // lazy, at most one PickQuarry per tick
		bool freshPicked = false;
		for (auto it = s_OutsideMarchers.begin(); it != s_OutsideMarchers.end(); ) {
			ATgPawn* p = it->second.pawn;
			bool drop = true;
			if (p && p->r_nPawnId == it->first && p->Controller && p->Health > 0) {
				drop = false;
				// Patrol actions set their own pace, so the run gait forced at
				// staging gets overwritten — re-assert it while marching.
				if (now >= it->second.nextPaceReassert) {
					((ATgAIController*)p->Controller)->eventSetPace(356);
					it->second.nextPaceReassert = now + kPaceReassertSecs;
				}
				// (kSeedQuarryAggro only) Quarry died / left mid-march -> the
				// pursuit driver is gone; re-seed a fresh one.
				ATgPawn* q = it->second.quarry;
				if (kSeedQuarryAggro &&
				    (!q || q->r_nPawnId != it->second.quarryId ||
				     !IsHumanPawn(q) || q->Health <= 0)) {
					if (!freshPicked) { fresh = PickQuarry(); freshPicked = true; }
					if (fresh) {
						ATgAIController* aic = (ATgAIController*)p->Controller;
						aic->SetTargetActor((AActor*)fresh);
						aic->AddThreat(fresh, kQuarryThreatSeed);
						it->second.quarry   = fresh;
						it->second.quarryId = fresh->r_nPawnId;
						Log("outside march: pawn %d quarry re-seeded to %d\n",
							it->first, fresh->r_nPawnId);
					}
				}
				if (Dist2(p->Location, s_EntranceVolume->Location) <
				    kEntranceReachDist * kEntranceReachDist) {
					ATgAIController* aic = (ATgAIController*)p->Controller;
					aic->m_pTriggerTarget      = (AActor*)A.obj;
					aic->m_pTriggerDestination = (AActor*)A.obj;
					aic->m_vTriggerLocation    = A.obj->Location;
					aic->m_bInterrupt          = 1;
					// (kSeedQuarryAggro only) Revert the seeded aggro now that
					// it's through the door: subtract exactly our threat
					// contribution and clear the forced target, so normal
					// behavior picks its own. Re-read the entry — the quarry
					// may have been re-seeded.
					if (kSeedQuarryAggro) {
						ATgPawn* rq = it->second.quarry;
						if (rq && rq->r_nPawnId == it->second.quarryId) {
							aic->AddThreat(rq, -kQuarryThreatSeed);
						}
						aic->SetTargetActor(nullptr);
					}
					// Through the door — NOW arm erratic target switching. Doing
					// it any earlier would hand the behavior a combat target
					// mid-march and stall the patrol drive outside.
					if (it->second.pendingSwitchSecs > 0.0f) {
						TargetSwitcher ts = { p, it->second.pendingSwitchSecs,
						                      now + it->second.pendingSwitchSecs };
						s_TargetSwitchers[it->first] = ts;
						Log("outside march: pawn %d target-switching armed (%.1fs)\n",
							it->first, it->second.pendingSwitchSecs);
					}
					Log("outside march: pawn %d at entrance, retargeted to A\n",
						it->first);
					drop = true;
				}
			}
			it = drop ? s_OutsideMarchers.erase(it) : ++it;
		}
	}

	// Erratic target switching (UnleashOpts::targetSwitchSecs): re-roll a
	// RANDOM living player as the target on each bot's own interval. With a
	// high rotationRateYaw this reads as a cheater flicking at people across
	// the room for no reason.
	if (!s_TargetSwitchers.empty() && s_Game && s_Game->WorldInfo) {
		const float now = s_Game->WorldInfo->TimeSeconds;
		std::vector<ATgPawn*> pool;   // built lazily, at most once per tick
		bool poolBuilt = false;
		for (auto it = s_TargetSwitchers.begin(); it != s_TargetSwitchers.end(); ) {
			ATgPawn* p = it->second.pawn;
			if (!p || p->r_nPawnId != it->first || !p->Controller || p->Health <= 0) {
				it = s_TargetSwitchers.erase(it);
				continue;
			}
			if (now >= it->second.nextSwitch) {
				if (!poolBuilt) {
					poolBuilt = true;
					ATgRepInfo_TaskForce* tf1 = GTeamsData.Attackers;
					if (tf1) {
						for (int i = 0; i < tf1->m_TeamPlayers.Count; i++) {
							ATgRepInfo_Player* pri = tf1->m_TeamPlayers.Data[i].pPrep;
							if (!pri || pri->bBot) continue;
							ATgPawn* pw = pri->r_PawnOwner;
							if (IsHumanPawn(pw) && pw->Health > 0) pool.push_back(pw);
						}
					}
				}
				if (!pool.empty()) {
					ATgPawn* victim = pool[rand() % pool.size()];
					((ATgAIController*)p->Controller)->SetTargetActor((AActor*)victim);
				}
				it->second.nextSwitch = now + it->second.interval;
			}
			++it;
		}
	}

	// Periodic VO barks (UnleashOpts::voCueIds). Writing r_nBotSoundCueId is the
	// whole mechanism — it's a repnotify in the V2 rep list, and every client
	// that has the pawn relevant runs PlayBotSound(Abs(id)). Sign alternates so
	// a repeated cue still reads as a CHANGE (same trick retail's
	// ATgAIController::InstallChosenAction uses). Do NOT call the SDK
	// PlayBotSound/PlaySoundCue wrappers: they carry the FunctionFlags |= ~0x400
	// corruption bug, and PlayBotSound is the client-side leaf anyway.
	if (!s_Barkers.empty() && s_Game && s_Game->WorldInfo) {
		const float now = s_Game->WorldInfo->TimeSeconds;
		for (auto it = s_Barkers.begin(); it != s_Barkers.end(); ) {
			ATgPawn* p = it->second.pawn;
			if (!p || p->r_nPawnId != it->first || p->Health <= 0) {
				it = s_Barkers.erase(it);
				continue;
			}
			if (now >= it->second.nextBark) {
				const int cue = it->second.cues[rand() % it->second.cues.size()];
				// Pitched path first when asked for; it needs a server-resident
				// USoundCue*, so a failed resolve/load falls through to the
				// plain replicated bark instead of going silent.
				bool played = false;
				if (it->second.pitch > 0.0f) {
					played = BotVoice::PlayPitched(p, cue, it->second.pitch);
				}
				if (!played) {
					it->second.sign = -it->second.sign;
					p->r_nBotSoundCueId = cue * it->second.sign;
				}
				it->second.nextBark = now + RandRange(it->second.minSecs,
				                                      it->second.maxSecs);
			}
			++it;
		}
	}

	// Escape phase: fire a GLOBAL alarm at a random interval to keep the run-back
	// tense — players get ambushed from whichever alarm factory is nearest them.
	// Each alarm is telegraphed kEscapeAlarmWarnSecs ahead with a center-screen
	// toast (same style as the scanner "player detected" alert) so the huge
	// ambush wave doesn't read as a random unfair teamwipe.
	if (s_aCaptured && s_escapeAlarmMax > 0.0f && s_Game && s_Game->WorldInfo) {
		const float now = s_Game->WorldInfo->TimeSeconds;
		// The wave for the NEXT alarm was chosen when it was scheduled, so its
		// text/duration telegraph correctly here.
		const Escape::AlarmWave* wave =
			(s_pendingWaveIdx >= 0 && s_pendingWaveIdx < (int)s_escapeWaves.size())
				? &s_escapeWaves[s_pendingWaveIdx] : nullptr;
		if (!s_escapeAlarmWarned && now >= s_nextEscapeAlarm - kEscapeAlarmWarnSecs) {
			s_escapeAlarmWarned = true;
			// A POOLED wave with no alertText is SILENT (unannounced) — lets you
			// slip small surprise waves in. Only the legacy no-pool path
			// (wave == nullptr) keeps the default warning.
			const wchar_t* txt = wave
				? wave->alertText
				: L"WARNING! Recursive ambush wave detected!";
			if (txt) {
				const float dur = wave ? wave->alertSecs : kEscapeAlarmWarnSecs;
				SendAlert::BroadcastText(txt,
				                         /*priority=APT_Normal*/  1,
				                         /*type=ATT_Detrimental*/ 2,
				                         dur);
				Log("escape alarm warning broadcast (alarm in %.1fs, wave=%d)\n",
					s_nextEscapeAlarm - now, s_pendingWaveIdx);
			} else {
				Log("escape alarm SILENT (wave=%d, alarm in %.1fs)\n",
					s_pendingWaveIdx, s_nextEscapeAlarm - now);
			}
		}
		if (now >= s_nextEscapeAlarm) {
			Adds::FireGlobalAlarm(wave);
			ArmNextEscapeAlarm(now);
		}
	}
}

}  // namespace SuperAgent
