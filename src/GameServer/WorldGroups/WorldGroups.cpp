#include "src/GameServer/WorldGroups/WorldGroups.hpp"
#include "src/Database/Database.hpp"
#include <chrono>
#include "src/GameServer/WorldGroups/BossMove.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "src/GameServer/Constants/TgProperties.h"
#include "src/GameServer/Core/LoadObject/Core__LoadObject.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/SetEffectRep/TgEffectManager__SetEffectRep.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/SyncPawnHealth/SyncPawnHealth.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace WorldGroups {
namespace {

struct Member {
	ATgPawn* pawn;
	int bot_id;
	int pawn_id;
	float baseHp;   // max HP per 1.0 of hp_scale (spawn max / spawn hp_scale)
	float baseDmg;  // s_fDamageAdjustment per 1.0 of dmg_scale
	FVector home{};  // ground spot it was placed on (no drop height) - engage walks back here
	bool lead = false;  // spawned as the group's first requested bot (the boss) - the dome sits on it only
};

// Engage-driven members never run the native "TARGET - ..." bot action
// (the drive below sets ai->Enemy itself), and that action is what carried the
// original "target sighted" VO (asm_data_set_bot_actions.sound_cue_id, e.g. Dune
// Commander 1486 cue 55 Bot_TargetSighted, Desert Commander 1465 cue 74
// Commander_EnterCombat). Bark that bot's own TARGET cue when the drive picks a new
// foe, same replicated r_nBotSoundCueId write (sign toggle) InstallChosenAction does.
// Bots whose behavior has no voiced TARGET action stay silent, as in the original.
struct TargetVo { int cue = 0; float chance = 0.0f; };
TargetVo LookupTargetVo(int botId) {
	static std::map<int, TargetVo> s_cache;
	auto it = s_cache.find(botId);
	if (it != s_cache.end()) return it->second;
	TargetVo v;
	if (sqlite3* db = Database::GetConnection()) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT a.sound_cue_id, a.audio_chance FROM asm_data_set_bot_actions a "
			"JOIN asm_data_set_bots b ON a.behavior_id = b.behavior_id "
			"WHERE b.bot_id = ? AND a.sound_cue_id > 0 AND a.description LIKE 'TARGET - %' "
			"ORDER BY a.id LIMIT 1;", -1, &st, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(st, 1, botId);
			if (sqlite3_step(st) == SQLITE_ROW) { v.cue = sqlite3_column_int(st, 0); v.chance = (float)sqlite3_column_double(st, 1); }
			sqlite3_finalize(st);
		}
	}
	s_cache[botId] = v;
	Logger::Log("worldgroups", "[WorldGroups] p0773 target vo bot=%d cue=%d chance=%.0f\n", botId, v.cue, v.chance);
	return v;
}
std::map<ATgPawn*, float> s_targetVoAt;  // pawn -> game time of its last target bark
struct VoHold { int val; float until; };
std::map<ATgPawn*, VoHold> s_targetVoHold;
void HoldTargetVo(ATgPawn* p, float now) {
	auto it = s_targetVoHold.find(p);
	if (it == s_targetVoHold.end()) return;
	if (now > it->second.until) { s_targetVoHold.erase(it); return; }
	if (p->r_nBotSoundCueId != it->second.val) {
		static int s_ow = 0;
		if (s_ow < 20) { ++s_ow; Logger::Log("worldgroups", "[WorldGroups] p0773 hold: pawn=%d cue overwritten %d -> reassert %d\n",
			p->r_nPawnId, p->r_nBotSoundCueId, it->second.val); }
		p->r_nBotSoundCueId = it->second.val; p->bNetDirty = 1;
	}
}
void BarkTargetVo(ATgPawn* p, int botId, float now) {
	const TargetVo v = LookupTargetVo(botId);
	if (v.cue <= 0 || !p) return;
	auto it = s_targetVoAt.find(p);
	if (it != s_targetVoAt.end() && now - it->second < 8.0f) return;  // retarget storms: one line per 8 s
	if (v.chance < 100.0f && (float)std::rand() / RAND_MAX * 100.0f >= v.chance) return;
	s_targetVoAt[p] = now;
	// Replicated r_nBotSoundCueId write (sign toggle, as InstallChosenAction does); the client
	// plays the cue of the pawn's own audio group. Held for 1.5 s (HoldTargetVo) so a native
	// action re-pick in the same tick cannot replace it before it replicates.
	const int val = p->r_nBotSoundCueId == v.cue ? -v.cue : v.cue;
	p->r_nBotSoundCueId = val; p->bNetDirty = 1;
	s_targetVoHold[p] = {val, now + 1.5f};
	static int s_logs = 0;
	if (s_logs < 50) { ++s_logs; Logger::Log("worldgroups", "[WorldGroups] p0773 target vo pawn=%d bot=%d cue=%d\n",
		p->r_nPawnId, botId, val); }
}

struct Contribution {
	double dmg = 0.0;
	double heal = 0.0;
};

// tag -> members. Pointers are only trusted while still in WorldInfo.PawnList.
std::map<std::string, std::vector<Member>> s_groups;
// tag -> character id -> damage dealt to members / healing done on characters
// that damaged members. Kept after members die; cleared by despawn or a
// replacing spawn (unless keep_contrib).
std::map<std::string, std::map<int64_t, Contribution>> s_contrib;

// tag -> group-control state (passive return / regen / speed scale).
struct Ctl {
	bool passive = false;
	float regen = 0.0f, homeRadius = 600.0f, settleS = 10.0f, maxReturnS = 30.0f;
	float startedAt = 0.0f, homeAt = -1.0f, last = 0.0f;
	int interrupts = 0, settled = 0;
	AActor* attacker = nullptr;  // set by OnCombat, applied on the next Tick
	std::map<ATgPawn*, float> baseSpeed, hpCarry;
	// Not passive: regen_frac_s applies while HP < regenCap * max (0 = off).
	float regenCap = 0.0f;
	// Protection: shipped "Invulnerable" payload on every member (damage lands
	// as 0, clients show *Immune*); optional display-only effect group too.
	bool protect = false;
	int auraGroup = 0;
	std::map<ATgPawn*, float> protectHp;  // HP fraction when protect went on: heals can't lift a member above max(this, cap)
	// Protect off: removing the protection effect can reset a member's HP to full (stale
	// property raw value); keep the HP it had for a few seconds.
	std::map<ATgPawn*, int> keepHp;
	float keepUntil = 0.0f;
	float hpScale = 0.0f, dmgScale = 0.0f;  // last values set by control (0 = spawn values)
	// native_regen false: the AI's own out-of-combat regen (TgAIController.Regenerate: +5 % max HP
	// once m_fLastRegenTime is 10 s old) is held off by keeping that stamp current.
	bool nativeRegen = true;
	// With native_regen false a member's HP may only rise through our own writes (SetHp) or
	// heal effects reported by TrackStats (healAllow); anything else is reverted.
	std::map<ATgPawn*, int> hpBase;
	std::map<ATgPawn*, float> healAllow;
	float speedK = 1.0f; int speedClamps = 0;  // current speed_scale + clamp count
	int commitStops = 0;
	float healCap = 0.0f;                  // max healed HP per s as a fraction of max HP (0 = no cap)
	std::map<ATgPawn*, float> healCapAt;   // last time the cap budget was taken
	int riseLogs = 0, healLogs = 0, healCapped = 0;
	// Engage: members pick a hostile player (last attacker first, else the nearest
	// inside engageAggro), run at it until inside engageHold, then the behavior fights; with
	// nobody to fight they walk back to their spawn spot (homeRadius). Driven every 0.5 s.
	bool engage = false;
	float engageAggro = 3500.0f, engageHold = 1100.0f, driveLast = 0.0f;
	AActor* lastAttacker = nullptr;
	float lastAttackAt = -1000.0f;
	std::map<ATgPawn*, int> driveMode;  // 0 idle, 1 chase, 2 fight, 3 home
	std::map<ATgPawn*, std::pair<FVector, float>> homeProgress;  // walking home: last progress spot + time
	int driveLogs = 0;
	// Dome: while protect is on, a stock deployable (visual only: no collision, no
	// damage) stands over the first live member, DrawScale domeScale, following it.
	int domeId = 0;
	float domeScale = 1.0f, domeZ = 0.0f;
	ATgDeployable* dome = nullptr;
	int domeIdx = -1, domeSpawns = 0;
	// Fit the dome to the boss from the mesh bounds (per unit DrawScale): radius, height, pivot offset.
	bool domeFit = true;
	float domeRadius = 0.0f;  // uu, 0 = auto from the boss's collision cylinder
	float domeUsedScale = 0.0f, domeOffX = 0.0f, domeOffY = 0.0f, domeOffZ = 0.0f;
	int domeMoves = 0, domeMoveFails = 0;
	float domeBossR = 0.0f, domeBossH = 0.0f;  // boss mesh bounds used for the fit (log)
	int domeMeshSrc = 0;  // where domeBossR/H came from (log)
	// Attack drive: the Colony Juggernaut's native behavior (sub-behavior tree,
	// path_only) never targets or fires in the open world (weapon none, 0 sight checks), so
	// members whose bot id is listed fire their own equipped devices (equip points) at the
	// engaged foe: melee points close in, ranged points (rotated every atkSwapS) further out,
	// aoe points (off-hand, fired without a switch) on a cooldown. Real device fire = real
	// projectiles, damage, animations. Same switch-then-StartAction route as PvP bots.
	std::vector<int> atkBots, atkRanged, atkMelee, atkAoe;
	float atkRange = 2500.0f, atkMeleeRange = 500.0f, atkAoeRange = 650.0f, atkAoeCd = 9.0f, atkSwapS = 6.0f;
	// Own range per ranged point (device TGPID_RANGE: Juggernaut Autofire 1000, Plasma 500);
	// a point is only used when the foe is inside it. Range rule: a foe the member can
	// shoot (inside atkRange, clear shot) is fought from where it stands even if unreachable; with no foe
	// in range + sight for atkNoShotHomeS the member drops it and walks home (anti-kite).
	std::vector<float> atkRangedRange;
	float atkNoShotHomeS = 20.0f;
	float atkMinRange = 0.0f;
	// Hit roll. The native AI aim lands the hitscan bursts just beside the target (impacts
	// at the agent, 0 hits), so while a ranged point is really firing (device in DeviceFiring) with a
	// clear shot, each 0.5 s tick lands that point's own hit effect group (DB damage, normal resists +
	// the boss's damage scale) on the target with probability atkHitChance. 0 groups = off.
	std::vector<int> atkHitGroups;
	float atkHitChance = 0.5f;  // foe closer than this (2D) -> back off to engageHold (guns refuse point-blank)
	std::map<ATgPawn*, float> lastShotChance;
	std::map<ATgPawn*, APawn*> lastFoe;
	struct Atk { int pt = -1, idx = 0, sw = 0, swTries = 0, starts = 0, fails = 0, aoe = 0, logs = 0;
		int noSight = 0, noSightRun = 0, busy = 0, busyRun = 0, kicks = 0, lastHits = 0;
		int assists = 0, backoffs = 0, forced = 0, useDev = 0, refuse = 0, outRange = 0, shootUnreach = 0, noShotHome = 0; std::string why;
		float since = -100.0f, lastSw = -100.0f, lastAoe = -100.0f; bool firing = false; };
	std::map<ATgPawn*, Atk> atk;
};
std::map<std::string, Ctl> s_ctl;

// tag -> area radius (uu, 2D around the first live member) for status "near".
std::map<std::string, float> s_area;
// tag -> task force the group was spawned on (engage: hostile = any other task force).
std::map<std::string, int> s_groupTf;
// AI controller -> run-to point: OverrideMovement turns its action picks into
// "run to m_vTriggerLocation" (238/289) stamped with this point.
std::map<ATgAIController*, FVector> s_runTo;
// Engaged members' controllers -> fire diagnostics (TargetInLOS fallback, FireSight).
struct FireDiag { int calls = 0, origOk = 0, ours = 0, blocked = 0; };
std::map<ATgAIController*, FireDiag> s_fireAi;
// Member -> hits it landed on player characters (TrackStats -> OnMemberHit).
std::map<AActor*, int> s_memberHits;
// tag -> character id -> last fight time with the group (damaged a member, was
// hit by a member, or healed an engaged character). Status "fighters".
std::map<std::string, std::map<int64_t, float>> s_engaged;
constexpr float kEngagedS = 30.0f;        // heal credit window after the last fight action
constexpr float kFightersReportS = 60.0f; // fighters[] lists actions this recent
// Shipped "Invulnerable" effect group (see TgPawn__MakeInvulnerable): +100 on
// every protection prop -> 0 damage + *Immune* combat text. Type 264 = Hit.
constexpr int kProtectEffectGroupId = 8698;
constexpr int kProtectEffectGroupType = 264;
constexpr float kProtectLifeS = 3600.0f;

// Boss fight bar : tag -> linked tag prefix. Spawned with "bar": true; the
// first member is the boss. A player who fought any group whose tag starts with the
// prefix (damaged a member, was hit by one, healed someone in the fight) within
// kBarS and is inside the group's area gets a marker in the boss PRI's r_eBonusType
// (that viewer only, see BarValue); the client patch draws the top-centre bar from it.
// Everyone else gets 0xA0000000 | HP per mille (client patch: no overhead bar on a world boss).
std::map<std::string, std::string> s_barLink;
struct BarState { int marker; std::vector<int64_t> viewers; };
std::map<ATgRepInfo_Player*, BarState> s_bars;  // rebuilt by BarTick
float s_barLast = 0.0f;
constexpr float kBarS = 25.0f;

constexpr int kMaxMembers = 40;
// Boss move set (defined next to AttackTick).
nlohmann::json MovesStatus(const std::string& tag, ATgPawn* p);
void ParseMoves(const std::string& tag, const nlohmann::json& arr);
void ClearMoves(const std::string& tag);
constexpr float kPi = 3.14159265358979323846f;
constexpr float kSpawnFloorBufferUU = 5.0f;

bool IsLive(AWorldInfo* wi, ATgPawn* pawn);
float Now();
bool IsLive(AWorldInfo* wi, ATgPawn* pawn) {
	for (APawn* p = wi ? wi->PawnList : nullptr; p != nullptr; p = p->NextPawn) {
		if (p == (APawn*)pawn) return !p->bDeleteMe && p->Health > 0;
	}
	return false;
}

AWorldInfo* CurrentWorldInfo() {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	return Game ? Game->WorldInfo : nullptr;
}

float Now() {
	AWorldInfo* wi = CurrentWorldInfo();
	return wi ? wi->TimeSeconds : 0.0f;
}

bool AtHome(ATgPawn* pawn, float radius) {
	ATgAIController* c = (ATgAIController*)pawn->Controller;
	if (!c) return true;
	const float dx = pawn->Location.X - c->m_vSpawnLocation.X, dy = pawn->Location.Y - c->m_vSpawnLocation.Y;
	return dx * dx + dy * dy <= radius * radius;
}

void ClearTarget(ATgPawn* pawn) {
	ATgAIController* c = (ATgAIController*)pawn->Controller;
	if (!c || c->bDeleteMe) return;
	c->Enemy = nullptr;
	if (pawn->r_TargetActor != nullptr) c->SetTargetActor(nullptr);
}

// World-only floor trace straight down from (x, y, zTop) to zTop - depth.
bool FloorAt(float x, float y, float zTop, float depth, float* outZ) {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return false;
	FVector start; start.X = x; start.Y = y; start.Z = zTop;
	FVector end = start; end.Z = zTop - depth;
	FVector hitLoc, hitNorm;
	FTraceHitInfo hitInfo;
	std::memset(&hitInfo, 0, sizeof(hitInfo));
	if (!Game->Trace(end, start, 0, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo)) return false;
	*outZ = hitLoc.Z;
	return true;
}

// Flattest spot for a big pawn: rings of candidates around (cx, cy) every 150 uu up to
// maxR, 8 per ring; each candidate samples the floor under its centre and 8 points at footR. A
// candidate whose highest-lowest sample spread is <= tol wins (nearest ring first), else the
// smallest spread. Returns false if no candidate had all 9 samples.
bool FindFlatSpot(float cx, float cy, float zTop, float maxR, float footR, float tol,
                  float* outX, float* outY, float* outFloor, float* outSpread, int* outTried) {
	bool found = false;
	float best = 1e9f;
	int tried = 0;
	for (float r = 0.0f; r <= maxR + 1.0f; r += 150.0f) {
		const int n = r < 1.0f ? 1 : 8;
		for (int k = 0; k < n; ++k) {
			const float a = 2.0f * kPi * (float)k / (float)n + r * 0.001f;
			const float x = cx + std::cos(a) * r, y = cy + std::sin(a) * r;
			float zc;
			++tried;
			if (!FloorAt(x, y, zTop, 30000.0f, &zc)) continue;
			float lo = zc, hi = zc;
			bool ok = true;
			for (int q = 0; q < 8 && ok; ++q) {
				const float b = 2.0f * kPi * (float)q / 8.0f;
				float z;
				if (!FloorAt(x + std::cos(b) * footR, y + std::sin(b) * footR, zTop, 30000.0f, &z)) ok = false;
				else { lo = std::fmin(lo, z); hi = std::fmax(hi, z); }
			}
			if (!ok) continue;
			const float spread = hi - lo;
			if (spread < best) {
				best = spread; found = true;
				*outX = x; *outY = y; *outFloor = hi; *outSpread = spread;
			}
		}
		if (found && best <= tol) break;
	}
	*outTried = tried;
	return found;
}

bool HostileTo(APawn* p, int groupTf) {
	ATgRepInfo_Player* pri = p ? (ATgRepInfo_Player*)p->PlayerReplicationInfo : nullptr;
	const int tf = (pri && pri->r_TaskForce) ? pri->r_TaskForce->r_nTaskForce : 0;
	return tf > 0 && tf != groupTf;
}

bool IsPlayerPawn(APawn* p) {
	return p && p->Controller && !p->bDeleteMe && p->Health > 0
		&& ObjectClassCache::ClassNameContains((UObject*)p->Controller, "PlayerController");
}

bool DomeAlive(Ctl& c) {
	TArray<UObject*>* objs = UObject::GObjObjects();
	const bool ok = c.dome && objs && c.domeIdx >= 0 && c.domeIdx < objs->Count
		&& objs->Data[c.domeIdx] == (UObject*)c.dome && !c.dome->bDeleteMe;
	if (!ok) { c.dome = nullptr; c.domeIdx = -1; }
	return ok;
}

void DomeOff(Ctl& c, const std::string& tag) {
	if (DomeAlive(c)) {
		((AActor*)c.dome)->Destroy();
		Logger::Log("worldgroups", "[WorldGroups] dome off tag=%s\n", tag.c_str());
	}
	c.dome = nullptr; c.domeIdx = -1;
}

FVector DomeSpot(ATgPawn* leader, const Ctl& c) {
	FVector at = leader->Location;
	const float half = leader->CylinderComponent ? leader->CylinderComponent->CollisionHeight : 0.0f;
	at.Z = at.Z - half + c.domeZ;
	// Centre the mesh (not its pivot) on the boss and put its bottom at the feet (fitted dome).
	const float sc = c.domeUsedScale > 0.0f ? c.domeUsedScale : 0.0f;
	at.X -= c.domeOffX * sc; at.Y -= c.domeOffY * sc; at.Z -= c.domeOffZ * sc;
	return at;
}

// The dome's boss = the lead member (first requested bot of the spawn), only while it lives.
ATgPawn* DomeBoss(AWorldInfo* wi, const std::vector<Member>& members) {
	for (const Member& m : members) if (m.lead) return IsLive(wi, m.pawn) ? m.pawn : nullptr;
	return nullptr;
}

// Spawns the dome deployable (visual only) over leader. Same pawn-independent setup as the
// deployable factory (TgDeployableFactory__SpawnObject), minus damage/fire/GRI registration.
void DomeOn(Ctl& c, const std::string& tag, ATgPawn* leader, int groupTf) {
	if (c.domeSpawns >= 20) return;  // something keeps killing it: stop trying
	const char* clsName = TgProj_Deployable__SpawnDeployable::GetDeployableClassName(c.domeId);
	UClass* cls = clsName ? ClassPreloader::GetClass(clsName) : nullptr;
	if (!cls) {
		Logger::Log("worldgroups", "[WorldGroups] dome tag=%s id=%d: no class (%s)\n", tag.c_str(), c.domeId, clsName ? clsName : "-");
		c.domeSpawns = 20;
		return;
	}
	FRotator rot; rot.Pitch = 0; rot.Yaw = 0; rot.Roll = 0;  // yaw 0 keeps the pivot offsets world-aligned
	c.domeUsedScale = 0.0f; c.domeOffX = c.domeOffY = c.domeOffZ = 0.0f;
	const FVector at = DomeSpot(leader, c);
	ATgDeployable* dep = (ATgDeployable*)((AActor*)leader)->Spawn(cls, (AActor*)leader, FName(), at, rot, nullptr, 1);
	c.domeSpawns++;
	if (!dep) {
		Logger::Log("worldgroups", "[WorldGroups] dome tag=%s id=%d: Spawn returned null\n", tag.c_str(), c.domeId);
		return;
	}
	dep->r_nDeployableId = c.domeId;
	dep->Instigator = (APawn*)leader;
	dep->eventInitReplicationInfo();
	ATgRepInfo_TaskForce* tf = groupTf == 1 ? GTeamsData.Attackers : GTeamsData.Defenders;
	if (dep->r_DRI) {
		if (tf) { dep->r_DRI->r_bOwnedByTaskforce = 1; dep->r_DRI->r_TaskforceInfo = tf; }
		if (leader->PlayerReplicationInfo) dep->r_DRI->r_InstigatorInfo = (ATgRepInfo_Player*)leader->PlayerReplicationInfo;
		dep->r_DRI->bNetDirty = 1;
		dep->r_DRI->bForceNetUpdate = 1;
	}
	dep->m_bInDestroyedState = 0;
	dep->r_bTakeDamage = 0;
	dep->s_bIsActivated = 1;
	dep->r_nPhysicalType = 861;
	dep->bAlwaysRelevant = 1;
	dep->InitializeDefaultProps();
	dep->ApplyDeployableSetup();
	dep->eventStartDeploy();
	dep->LifeSpan = 0.0f;
	((AActor*)dep)->SetCollision(0, 0, 1);
	// Size from the mesh's real bounds so the dome just encloses the boss.
	float useScale = c.domeScale;
	const char* fit = "raw";
	const float ds0 = dep->DrawScale > 0.01f ? dep->DrawScale : 1.0f;
	UPrimitiveComponent* mc = (UPrimitiveComponent*)dep->c_Mesh;
	float baseR = 0.0f, baseH = 0.0f;
	if (mc) { baseR = std::fmax(mc->Bounds.BoxExtent.X, mc->Bounds.BoxExtent.Y) / ds0; baseH = 2.0f * mc->Bounds.BoxExtent.Z / ds0; }
	if (c.domeFit && baseR > 5.0f && baseH > 5.0f) {
		const float cr = leader->CylinderComponent ? leader->CylinderComponent->CollisionRadius : 60.0f;
		const float ch = leader->CylinderComponent ? leader->CylinderComponent->CollisionHeight : 90.0f;
		// the boss's visible mesh is bigger than its collision (Juggernaut ~2x): enclose the mesh bounds too
		float meshR = 0.0f, meshH = 0.0f;
		int meshSrc = 0;  // 1 = live component bounds, 2 = skeletal mesh asset bounds (server never updates skel bounds), 3 = 1.25 x collision guess
		if (USkeletalMeshComponent* bm = ((APawn*)leader)->Mesh) {
			const FVector& e = bm->Bounds.BoxExtent;
			if (e.X > 1.0f && e.X < 3000.0f && e.Y < 3000.0f && e.Z > 1.0f && e.Z < 3000.0f) { meshR = std::fmax(e.X, e.Y); meshH = 2.0f * e.Z; meshSrc = 1; }
			else if (bm->SkeletalMesh) {
				const FVector& a = bm->SkeletalMesh->Bounds.BoxExtent;
				const float k = (bm->Scale > 0.01f ? bm->Scale : 1.0f)
					* std::fmax(std::fmax(bm->Scale3D.X, bm->Scale3D.Y), std::fmax(bm->Scale3D.Z, 0.01f))
					* (leader->DrawScale > 0.01f ? leader->DrawScale : 1.0f);
				if (a.X > 1.0f && a.Z > 1.0f && a.X * k < 3000.0f && a.Y * k < 3000.0f && a.Z * k < 3000.0f) { meshR = std::fmax(a.X, a.Y) * k; meshH = 2.0f * a.Z * k; meshSrc = 2; }
			}
		}
		if (meshSrc == 0) { meshR = 1.25f * 2.0f * cr; meshH = 1.25f * 2.0f * ch * 1.2f; meshSrc = 3; }
		c.domeMeshSrc = meshSrc;
		meshR = std::fmin(meshR, 5.0f * cr); meshH = std::fmin(meshH, 5.0f * 2.0f * ch);  // a bogus bound can't blow it up again
		c.domeBossR = meshR; c.domeBossH = meshH;
		const float needR = c.domeRadius > 0.0f ? c.domeRadius : std::fmax(std::fmax(140.0f, 2.0f * cr), 1.05f * meshR);
		const float needH = c.domeRadius > 0.0f ? 0.0f : std::fmax(2.0f * ch * 1.2f, 1.05f * meshH);
		useScale = std::fmax(0.25f, std::fmin(30.0f, std::fmax(needR / baseR, needH / baseH)));
		c.domeOffX = (mc->Bounds.Origin.X - dep->Location.X) / ds0;
		c.domeOffY = (mc->Bounds.Origin.Y - dep->Location.Y) / ds0;
		c.domeOffZ = (mc->Bounds.Origin.Z - mc->Bounds.BoxExtent.Z - dep->Location.Z) / ds0 + 8.0f / useScale;  // bottom 8 uu under the feet
		fit = c.domeRadius > 0.0f ? "radius" : "boss";
	} else if (c.domeFit) {
		fit = "no-bounds";
	}
	c.domeUsedScale = useScale;
	((AActor*)dep)->SetDrawScale(useScale);
	dep->bReplicateMovement = 1;  // it follows the boss - clients must see the moves
	// A simulated proxy only gets Location after the initial bunch if bUpdateSimulatedPosition is set and
	// property replication isn't skipped (otherwise the server moved it 2700 uu while clients kept it at the spawn spot).
	Logger::Log("worldgroups", "[WorldGroups] dome net tag=%s was updsim=%d skip=%d freq=%.1f\n", tag.c_str(),
		(int)dep->bUpdateSimulatedPosition, (int)dep->bSkipActorPropertyReplication, dep->NetUpdateFrequency);
	dep->bUpdateSimulatedPosition = 1;
	dep->bSkipActorPropertyReplication = 0;
	if (dep->NetUpdateFrequency < 20.0f) dep->NetUpdateFrequency = 20.0f;
	((AActor*)dep)->SetLocation(DomeSpot(leader, c));
	Logger::Log("worldgroups", "[WorldGroups] dome fit tag=%s mode=%s base_r=%.0f base_h=%.0f ds0=%.2f scale=%.2f -> r=%.0f h=%.0f boss cr=%.0f ch=%.0f mesh r=%.0f h=%.0f src=%d off=(%.0f,%.0f,%.0f) role=%d movable=%d\n",
		tag.c_str(), fit, baseR, baseH, ds0, useScale, baseR * useScale, baseH * useScale,
		leader->CylinderComponent ? leader->CylinderComponent->CollisionRadius : -1.0f,
		leader->CylinderComponent ? leader->CylinderComponent->CollisionHeight : -1.0f, c.domeBossR, c.domeBossH, c.domeMeshSrc,
		c.domeOffX * useScale, c.domeOffY * useScale, c.domeOffZ * useScale, (int)dep->RemoteRole, (int)dep->bMovable);
	dep->bNetDirty = 1;
	dep->bForceNetUpdate = 1;
	c.dome = dep;
	c.domeIdx = dep->ObjectInternalInteger;
	Logger::Log("worldgroups", "[WorldGroups] dome on tag=%s id=%d (%s) scale=%.2f boss=%d at (%.0f,%.0f,%.0f) spawns=%d\n",
		tag.c_str(), c.domeId, clsName, useScale, leader->r_nPawnId, dep->Location.X, dep->Location.Y, dep->Location.Z, c.domeSpawns);
}

void DestroyMembers(std::vector<Member>& members) {
	AWorldInfo* wi = CurrentWorldInfo();
	for (const Member& m : members) {
		if (!IsLive(wi, m.pawn)) continue;
		AController* c = m.pawn->Controller;
		if (c) s_runTo.erase((ATgAIController*)c);
		if (c) s_fireAi.erase((ATgAIController*)c);
		s_memberHits.erase((AActor*)m.pawn);
		BossMove::ForgetPawn(m.pawn);
		((AActor*)m.pawn)->Destroy();
		if (c && !c->bDeleteMe) ((AActor*)c)->Destroy();
	}
	members.clear();
}

// Our own HP writes (pawn -> {hp, time}): the native_regen guard takes them as the new baseline.
std::map<ATgPawn*, std::pair<int, float>> s_ourHp;

// HP write through the property system too (props 51/304), else the next
// effect touching health fans out from a stale m_fRaw (see SuperAgent).
void SetHp(ATgPawn* pawn, int hp, int maxHp) {
	maxHp = std::max(1, maxHp);
	hp = std::max(0, std::min(maxHp, hp));
	if (AWorldInfo* w = CurrentWorldInfo()) s_ourHp[pawn] = {hp, w->TimeSeconds};
	if (pawn->s_Properties.Data) {
		for (int i = 0; i < pawn->s_Properties.Num(); ++i) {
			UTgProperty* p = pawn->s_Properties.Data[i];
			if (!p) continue;
			if ((p->m_nPropertyId == GA_PROPERTY::TGPID_HEALTH || p->m_nPropertyId == GA_PROPERTY::TGPID_HEALTH_MAX)
				&& p->m_fMaximum < (float)maxHp * 2.0f) p->m_fMaximum = (float)maxHp * 2.0f;
		}
	}
	SyncPawnHealth::Apply(pawn, hp, maxHp);
}

// TgEffectManager::ClearEffectRep native (same as RemoveEffectGroup.cpp uses).
typedef void(__fastcall* ClearEffectRepFn)(ATgEffectManager*, void*, int /*nEffectId*/, int /*nManagedListIndex*/);
const ClearEffectRepFn ClearEffectRepNative = (ClearEffectRepFn)0x10a6f030;

bool HasGroup(ATgPawn* pawn, int egId) {
	ATgEffectManager* mgr = pawn->r_EffectManager;
	if (!mgr || !mgr->s_AppliedEffectGroups.Data) return false;
	for (int i = 0; i < mgr->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* ag = mgr->s_AppliedEffectGroups.Data[i];
		if (ag && ag->m_nEffectGroupId == egId) return true;
	}
	return false;
}

void RemoveGroup(ATgPawn* pawn, int egId) {
	ATgEffectManager* mgr = pawn->r_EffectManager;
	if (!mgr || !mgr->s_AppliedEffectGroups.Data) return;
	FImpactInfo impact{};
	for (int i = mgr->s_AppliedEffectGroups.Count - 1; i >= 0; i--) {
		UTgEffectGroup* ag = mgr->s_AppliedEffectGroups.Data[i];
		if (ag && ag->m_nEffectGroupId == egId)
			mgr->eventProcessEffect(ag, /*bRemove=*/1, nullptr, (AActor*)pawn, impact);
	}
}

void SetProtect(ATgPawn* pawn, bool on, int auraGroup) {
	ATgEffectManager* mgr = pawn->r_EffectManager;
	if (!mgr || pawn->Role != 3 /*ROLE_Authority*/) return;
	FImpactInfo impact{};
	if (on) {
		if (!HasGroup(pawn, kProtectEffectGroupId)) {
			if (UTgEffectGroup* g = BuildEffectGroup(kProtectEffectGroupId, kProtectEffectGroupType)) {
				g->m_fLifeTime = kProtectLifeS;
				mgr->eventProcessEffect(g, /*bRemove=*/0, nullptr, (AActor*)pawn, impact);
			}
		}
		// Visual: the aura group through the normal effect path (its client FX comes with the
		// replicated managed-effect entry). Category forced to 302 so a shield group's 862
		// "Invulnerable" bucket can't stop the member's own device fire (TgDevice StartFire).
		if (auraGroup > 0 && !HasGroup(pawn, auraGroup)) {
			if (UTgEffectGroup* g = BuildEffectGroup(auraGroup, kProtectEffectGroupType)) {
				g->m_fLifeTime = kProtectLifeS;
				g->m_nCategoryCode = 302;
				const int vis = (int)g->m_bHasVisual, nfx = g->m_Effects.Count;
				mgr->eventProcessEffect(g, /*bRemove=*/0, nullptr, (AActor*)pawn, impact);
				int slot = -1;
				for (int i = 0; i < 0x10; i++) if (mgr->r_ManagedEffectList[i].nEffectGroupID == auraGroup) { slot = i; break; }
				Logger::Log("worldgroups", "[WorldGroups] aura eg=%d visual=%d effects=%d applied=%d managed_slot=%d\n",
					auraGroup, vis, nfx, (int)HasGroup(pawn, auraGroup), slot);
			} else {
				Logger::Log("worldgroups", "[WorldGroups] aura eg=%d: BuildEffectGroup failed\n", auraGroup);
			}
		}
	} else {
		RemoveGroup(pawn, kProtectEffectGroupId);
		if (auraGroup > 0) {
			RemoveGroup(pawn, auraGroup);
			for (int i = 0; i < 0x10; i++) {
				if (mgr->r_ManagedEffectList[i].nEffectGroupID == auraGroup) ClearEffectRepNative(mgr, nullptr, auraGroup, i);
			}
		}
	}
	((AActor*)mgr)->bNetDirty = 1;
	((AActor*)mgr)->bForceNetUpdate = 1;
}

} // namespace

void ReportStatus(const std::string& tag) {
	AWorldInfo* wi = CurrentWorldInfo();
	nlohmann::json msg;
	msg["type"] = IpcProtocol::MSG_GROUP_STATUS;
	msg["tag"]  = tag;
	nlohmann::json members = nlohmann::json::array();
	int spawned = 0, alive = 0;
	auto it = s_groups.find(tag);
	if (it != s_groups.end()) {
		for (const Member& m : it->second) {
			++spawned;
			if (!IsLive(wi, m.pawn)) continue;
			++alive;
			members.push_back({{"bot_id", m.bot_id}, {"hp", m.pawn->Health},
				{"hp_max", m.pawn->r_nHealthMaximum}, {"x", m.pawn->Location.X},
				{"y", m.pawn->Location.Y}, {"z", m.pawn->Location.Z},
				{"phys", (int)m.pawn->Physics}, {"target", m.pawn->r_TargetActor != nullptr}});
			// Diagnostics: floor right under the member (world only), cylinder foot,
			// collision size, 2D speed, AI enemy + state.
			nlohmann::json& mj = members.back();
			const float ch = m.pawn->CylinderComponent ? m.pawn->CylinderComponent->CollisionHeight : 0.0f;
			const float cr = m.pawn->CylinderComponent ? m.pawn->CylinderComponent->CollisionRadius : 0.0f;
			float fz;
			if (FloorAt(m.pawn->Location.X, m.pawn->Location.Y, m.pawn->Location.Z, 5000.0f, &fz)) mj["floor"] = fz;
			mj["foot"] = m.pawn->Location.Z - ch;
			mj["ch"] = ch; mj["cr"] = cr;
			mj["spd"] = std::sqrt(m.pawn->Velocity.X * m.pawn->Velocity.X + m.pawn->Velocity.Y * m.pawn->Velocity.Y);
			ATgAIController* mai = m.pawn->Controller ? (ATgAIController*)m.pawn->Controller : nullptr;
			mj["enemy"] = mai && mai->Enemy != nullptr;
			if (mai) mj["state"] = std::string(mai->GetStateName().GetName());
			if (mai) {  // fire diagnostics
				auto fd = s_fireAi.find(mai);
				nlohmann::json f = {{"weapon", m.pawn->Weapon ? std::string(((UObject*)m.pawn->Weapon)->GetName()) : std::string("none")},
					{"last_fire_age", Now() - mai->m_fLastFireTime}, {"los_age", Now() - mai->m_fTargetLastClearLOSTime},
					{"hit_age", Now() - mai->m_fLastTargetHitTime}};
				if (fd != s_fireAi.end())
					f.update({{"los_calls", fd->second.calls}, {"los_native_ok", fd->second.origOk}, {"los_ours", fd->second.ours}, {"los_blocked", fd->second.blocked}});
				mj["fire"] = f;
			}
			{  // attack drive diagnostics
				auto hi = s_memberHits.find((AActor*)m.pawn);
				mj["hits"] = hi != s_memberHits.end() ? hi->second : 0;
				if (mai && mai->Enemy && !mai->Enemy->bDeleteMe) {  // the foe's HP (boss damage lands)
					mj["foe_hp"] = mai->Enemy->Health;
					mj["foe_player"] = IsPlayerPawn(mai->Enemy);
				}
				auto cl0 = s_ctl.find(tag);
				if (cl0 != s_ctl.end()) {
					auto ai0 = cl0->second.atk.find(m.pawn);
					if (ai0 != cl0->second.atk.end()) {
						const Ctl::Atk& a = ai0->second;
						mj["atk"] = {{"pt", a.pt}, {"sw", a.sw}, {"starts", a.starts}, {"fails", a.fails}, {"aoe", a.aoe},
							{"firing", a.firing}, {"nosight", a.noSight}, {"busy", a.busy}, {"kicks", a.kicks},
							{"forced", a.forced}, {"assists", a.assists}, {"backoffs", a.backoffs}, {"usedev", a.useDev}, {"refuse", a.refuse}, {"out_range", a.outRange},
							{"shoot_unreach", a.shootUnreach}, {"no_shot_home", a.noShotHome}, {"why", a.why},
							{"cur_dev", m.pawn->Weapon ? ((ATgDevice*)m.pawn->Weapon)->r_nDeviceId : -1},
							{"dev_state", m.pawn->Weapon ? std::string(m.pawn->Weapon->GetStateName().GetName()) : std::string("-")}};
						mj["moves"] = MovesStatus(tag, m.pawn);
					}
				}
			}
		}
	}
	msg["spawned"] = spawned;
	msg["alive"]   = alive;
	if (it != s_groups.end()) {  // movement diagnostics
		size_t k = 0;
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			if (k < members.size()) BossMove::Status(tag, m.pawn, members[k]);
			++k;
		}
	}
	BossMove::TagStatus(tag, msg);
	msg["members"] = members;
	auto cl = s_ctl.find(tag);
	msg["passive"]    = cl != s_ctl.end() && cl->second.passive;
	msg["interrupts"] = cl != s_ctl.end() ? cl->second.interrupts : 0;
	msg["settled"]    = cl != s_ctl.end() ? cl->second.settled : 0;
	nlohmann::json contrib = nlohmann::json::array();
	auto ct = s_contrib.find(tag);
	if (ct != s_contrib.end()) {
		for (const auto& [charId, c] : ct->second) {
			contrib.push_back({{"char_id", charId}, {"dmg", (int64_t)c.dmg}, {"heal", (int64_t)c.heal}});
		}
	}
	msg["contrib"] = contrib;
	msg["protected"] = cl != s_ctl.end() && cl->second.protect;
	// Players near: live player pawns within area_radius (2D) of the first live member.
	int nearN = 0;
	nlohmann::json nearIds = nlohmann::json::array();
	if (it != s_groups.end() && wi) {
		const Member* lead = nullptr;
		for (const Member& m : it->second) if (IsLive(wi, m.pawn)) { lead = &m; break; }
		auto ar = s_area.find(tag);
		const float area = ar != s_area.end() ? ar->second : 6000.0f;
		for (AController* c = lead ? wi->ControllerList : nullptr; c; c = c->NextController) {
			if (!ObjectClassCache::ClassNameContains(c, "PlayerController") || !c->Pawn) continue;
			if (c->Pawn->bDeleteMe || c->Pawn->Health <= 0) continue;
			const float dx = c->Pawn->Location.X - lead->pawn->Location.X, dy = c->Pawn->Location.Y - lead->pawn->Location.Y;
			if (dx * dx + dy * dy > area * area) continue;
			++nearN;
			nearIds.push_back((int64_t)((ATgPawn_Character*)c->Pawn)->s_nCharacterId);
		}
	}
	msg["near"] = nearN;
	msg["near_ids"] = nearIds;
	// Fighters: characters with a fight action on this group in the last kFightersReportS.
	nlohmann::json fighters = nlohmann::json::array();
	auto en = s_engaged.find(tag);
	const float tnow = Now();
	if (en != s_engaged.end()) {
		for (const auto& [charId, t] : en->second)
			if (tnow - t <= kFightersReportS) fighters.push_back({{"char_id", charId}, {"ago", (int)(tnow - t)}});
	}
	msg["fighters"] = fighters;
	IpcClient::Send(msg.dump());
	Logger::Log("worldgroups", "[WorldGroups] status tag=%s alive=%d/%d first_hp=%d/%d\n", tag.c_str(), alive, spawned,
		members.empty() ? 0 : members[0].value("hp", 0), members.empty() ? 0 : members[0].value("hp_max", 0));
}

void Spawn(const nlohmann::json& j) {
	const std::string tag = j.value("tag", std::string());
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (tag.empty() || !Game || !j.contains("bots") || !j["bots"].is_array()) {
		Logger::Log("worldgroups", "[WorldGroups] spawn refused (tag/game/bots missing)\n");
		return;
	}
	auto& members = s_groups[tag];
	DestroyMembers(members);
	{
		auto old = s_ctl.find(tag);
		if (old != s_ctl.end()) DomeOff(old->second, tag);
	}
	s_ctl.erase(tag);
	if (j.contains("native_regen") && j["native_regen"].is_boolean() && !j["native_regen"].get<bool>())
		s_ctl[tag].nativeRegen = false;  // see Ctl::nativeRegen
	if (j.contains("heal_cap_frac_s") && j["heal_cap_frac_s"].is_number()) {  // via spawn (CS forwards it)
		s_ctl[tag].healCap = std::fmax(0.0f, std::fmin(1.0f, j["heal_cap_frac_s"].get<float>()));
		Logger::Log("worldgroups", "[WorldGroups] heal cap tag=%s %.4f x max HP per s\n", tag.c_str(), s_ctl[tag].healCap);
	}
	// Engage / dome settings may come with the spawn (the control server forwards the
	// whole spawn payload, its group-control forward is a fixed key list).
	BossMove::Configure(tag, j);
	if (j.contains("engage") || j.contains("dome_id")) {
		Ctl& c0 = s_ctl[tag];
		auto cf = [&](const char* k, float lo, float hi, float def) {
			return j.contains(k) && j[k].is_number() ? std::fmax(lo, std::fmin(hi, j[k].get<float>())) : def;
		};
		c0.engage = j.value("engage", false);
		c0.engageAggro = cf("engage_aggro", 500.0f, 15000.0f, c0.engageAggro);
		c0.engageHold  = cf("engage_hold", 150.0f, 5000.0f, c0.engageHold);
		c0.homeRadius  = cf("home_radius", 100.0f, 5000.0f, c0.homeRadius);
		const int d = j.contains("dome_id") && j["dome_id"].is_number_integer() ? j["dome_id"].get<int>() : 0;
		c0.domeId = (d > 0 && d < 100000) ? d : 0;
		c0.domeScale = cf("dome_scale", 0.25f, 30.0f, c0.domeScale);
		c0.domeRadius = cf("dome_radius", 0.0f, 3000.0f, c0.domeRadius);
		if (j.contains("dome_fit") && j["dome_fit"].is_boolean()) c0.domeFit = j["dome_fit"].get<bool>();
		c0.domeZ = cf("dome_z", -2000.0f, 2000.0f, c0.domeZ);
	}
	if (j.contains("attack_bots") && j["attack_bots"].is_array()) {
		Ctl& c0 = s_ctl[tag];
		auto ints = [&](const char* k, std::vector<int>& out, int lo, int hi) {
			out.clear();
			if (!j.contains(k) || !j[k].is_array()) return;
			for (const auto& v : j[k]) if (v.is_number_integer() && v.get<int>() >= lo && v.get<int>() <= hi && out.size() < 8) out.push_back(v.get<int>());
		};
		auto cf = [&](const char* k, float lo, float hi, float def) {
			return j.contains(k) && j[k].is_number() ? std::fmax(lo, std::fmin(hi, j[k].get<float>())) : def;
		};
		ints("attack_bots", c0.atkBots, 1, 100000);
		ints("attack_ranged", c0.atkRanged, 0, 24);
		ints("attack_melee", c0.atkMelee, 0, 24);
		ints("attack_aoe", c0.atkAoe, 0, 24);
		c0.atkRange = cf("attack_range", 300.0f, 6000.0f, c0.atkRange);
		c0.atkMeleeRange = cf("attack_melee_range", 100.0f, 2000.0f, c0.atkMeleeRange);
		c0.atkAoeRange = cf("attack_aoe_range", 100.0f, 3000.0f, c0.atkAoeRange);
		c0.atkAoeCd = cf("attack_aoe_cd", 2.0f, 120.0f, c0.atkAoeCd);
		c0.atkSwapS = cf("attack_swap_s", 1.0f, 60.0f, c0.atkSwapS);
		c0.atkNoShotHomeS = cf("attack_no_shot_home_s", 5.0f, 300.0f, c0.atkNoShotHomeS);
		c0.atkMinRange = cf("attack_min_range", 0.0f, 2000.0f, c0.atkMinRange);
		ints("attack_hit_groups", c0.atkHitGroups, 0, 1000000);
		ClearMoves(tag);
		if (j.contains("attack_moves")) ParseMoves(tag, j["attack_moves"]);
		c0.atkHitChance = cf("attack_hit_chance", 0.0f, 1.0f, c0.atkHitChance);
		c0.atkRangedRange.clear();
		if (j.contains("attack_ranged_range") && j["attack_ranged_range"].is_array())
			for (const auto& v : j["attack_ranged_range"]) if (v.is_number() && c0.atkRangedRange.size() < 8)
				c0.atkRangedRange.push_back(std::fmax(100.0f, std::fmin(6000.0f, v.get<float>())));
		while (c0.atkRangedRange.size() < c0.atkRanged.size()) c0.atkRangedRange.push_back(c0.atkRange);
		Logger::Log("worldgroups", "[WorldGroups] attack drive tag=%s bots=%d ranged=%d melee=%d aoe=%d range=%.0f\n",
			tag.c_str(), (int)c0.atkBots.size(), (int)c0.atkRanged.size(), (int)c0.atkMelee.size(), (int)c0.atkAoe.size(), c0.atkRange);
	}
	if (!j.value("keep_contrib", false)) { s_contrib.erase(tag); s_engaged.erase(tag); }
	s_area[tag] = std::fmax(500.0f, std::fmin(20000.0f, j.value("area_radius", 6000.0f)));
	if (j.value("bar", false)) s_barLink[tag] = j.value("bar_link", tag).substr(0, 64);
	else s_barLink.erase(tag);

	const float cx = j.value("x", 0.0f), cy = j.value("y", 0.0f), cz = j.value("z", 0.0f);
	const int yaw = j.value("yaw", 0);
	// Optional floor snap: trace down from z + snap_up so callers may pass an approximate height.
	const bool snap = j.value("snap_ground", false);
	const float snapUp = std::fmax(0.0f, std::fmin(20000.0f, j.value("snap_up", 6000.0f)));
	int snapped = 0;
	// Optional sky drop: members appear drop_height above the floor in falling physics.
	const float dropHeight = std::fmax(0.0f, std::fmin(8000.0f, j.value("drop_height", 0.0f)));
	const float dropSpeed = std::fmax(0.0f, std::fmin(4000.0f, j.value("drop_speed", 600.0f)));
	const float radius = std::fmax(0.0f, std::fmin(3000.0f, j.value("radius", 300.0f)));
	const int tf = j.value("task_force", 1) == 2 ? 2 : 1;
	s_groupTf[tag] = tf;
	ATgRepInfo_TaskForce* team = tf == 1 ? GTeamsData.Attackers : GTeamsData.Defenders;
	const DifficultyScalar scalar(
		std::fmax(0.0f, std::fmin(50.0f, j.value("hp_scale", 0.0f))),
		std::fmax(0.0f, std::fmin(10.0f, j.value("dmg_scale", 0.0f))));

	// Flatten the bot list; the first entry stands on the point, the rest on a ring.
	std::vector<int> ids;
	for (const auto& b : j["bots"]) {
		const int bot_id = b.value("bot_id", 0);
		const int count = std::max(0, std::min(kMaxMembers, b.value("count", 1)));
		for (int i = 0; i < count && bot_id > 0 && (int)ids.size() < kMaxMembers; ++i) ids.push_back(bot_id);
	}
	const int ring = (int)ids.size() - 1;
	// flat_radius: the first member (and the ring around it) moves to the flattest
	// spot within flat_radius, so a big pawn does not rest its cylinder on a ridge or slope edge.
	float gx = cx, gy = cy;
	const float flatR = std::fmax(0.0f, std::fmin(3000.0f, j.value("flat_radius", 0.0f)));
	if (snap && flatR > 0.0f && !ids.empty()) {
		float r0 = 0.f, h0 = 0.f, fx = cx, fy = cy, ff = cz, spread = 0.f;
		int tried = 0;
		TgGame__SpawnBotById::GetBotCollisionCylinder(ids[0], &r0, &h0);
		const float tol = std::fmax(5.0f, std::fmin(500.0f, j.value("flat_tol", 30.0f)));
		if (FindFlatSpot(cx, cy, cz + snapUp, flatR, std::fmax(60.0f, r0 * 1.5f), tol, &fx, &fy, &ff, &spread, &tried)) {
			gx = fx; gy = fy;
			Logger::Log("worldgroups", "[WorldGroups] flat tag=%s spot (%.0f,%.0f) floor=%.0f spread=%.0f off=%.0f tried=%d\n",
				tag.c_str(), gx, gy, ff, spread, std::sqrt((gx - cx) * (gx - cx) + (gy - cy) * (gy - cy)), tried);
		} else {
			Logger::Log("worldgroups", "[WorldGroups] flat tag=%s: no spot found (tried %d)\n", tag.c_str(), tried);
		}
	}
	for (size_t i = 0; i < ids.size(); ++i) {
		float botRadius = 0.f, botHalfHeight = 0.f;
		TgGame__SpawnBotById::GetBotCollisionCylinder(ids[i], &botRadius, &botHalfHeight);
		FVector loc;
		loc.X = gx; loc.Y = gy;
		if (i > 0) {
			const float a = 2.0f * kPi * (float)(i - 1) / (float)ring;
			loc.X += std::cos(a) * radius;
			loc.Y += std::sin(a) * radius;
		}
		float floorZ = cz;
		if (snap) {
			FVector start = loc; start.Z = cz + snapUp;
			FVector end   = loc; end.Z   = cz - 20000.0f;
			FVector hitLoc, hitNorm;
			FTraceHitInfo hitInfo;
			std::memset(&hitInfo, 0, sizeof(hitInfo));
			if (Game->Trace(end, start, 1, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo)) {
				floorZ = hitLoc.Z;
				++snapped;
			}
		}
		loc.Z = floorZ + botHalfHeight + kSpawnFloorBufferUU + dropHeight;
		FRotator rot;
		rot.Pitch = 0; rot.Yaw = yaw; rot.Roll = 0;

		TgPawn__InitializeDefaultProps::nPendingDifficultyScalarOverride = scalar;
		TgPawn__InitializeDefaultProps::bPendingEnemyScaling = true;
		ATgPawn* Bot = TgGame__SpawnBotById::Call(Game, nullptr, ids[i], loc, rot,
			/*bKillController*/ false, /*pFactory*/ nullptr, /*bIgnoreCollision*/ true,
			/*pOwnerPawn*/ nullptr, /*bIsDecoy*/ false, /*deviceFire*/ nullptr, 0.0f);
		if (!Bot) {
			Logger::Log("worldgroups", "[WorldGroups] tag=%s bot_id=%d: SpawnBotById returned null\n",
				tag.c_str(), ids[i]);
			continue;
		}
		ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
		if (BotRep && team) {
			BotRep->r_TaskForce = team;
			BotRep->Team        = team;
			BotRep->SetTeam(team);
			Bot->NotifyTeamChanged();
		}
		// Zone players are task force 2; task force 1 = hostile to them.
		Bot->r_bInitialIsEnemy = tf == 1 ? 1 : 0;
		// SetPhysics is a stripped no-op here, so write the field (as the dying path does);
		// flying classes are switched back to PHYS_Flying by the PostPawnSetup hook.
		if (dropHeight > 0.0f) {
			Bot->Physics = 2 /*PHYS_Falling*/;
			Bot->bSimulateGravity = 1;
			Bot->Velocity.X = 0.0f; Bot->Velocity.Y = 0.0f; Bot->Velocity.Z = -dropSpeed;
			// No fall damage for dropped members (TakeFallingDamage returns early at modifier 0).
			Bot->SetProperty(GA_PROPERTY::TGPID_FALLING_DAMAGE_MODIFIER, 0.0f);
			Bot->bNetDirty = 1;
			Bot->bForceNetUpdate = 1;
		}
		const float kHp = scalar.HP > 0.0f ? scalar.HP : 1.0f, kDmg = scalar.Dmg > 0.0f ? scalar.Dmg : 1.0f;
		members.push_back({Bot, ids[i], Bot->r_nPawnId, (float)Bot->r_nHealthMaximum / kHp,
			Bot->s_fDamageAdjustment / kDmg});
		members.back().lead = (i == 0);  // the boss (dome target); an escort never inherits it
		members.back().home = loc;
		members.back().home.Z = floorZ + botHalfHeight + kSpawnFloorBufferUU;
	}
	Logger::Log("worldgroups", "[WorldGroups] spawn tag=%s n=%d/%d at (%.0f,%.0f,%.0f) r=%.0f tf=%d hp_scale=%.2f dmg_scale=%.2f snapped=%d drop=%.0f\n",
		tag.c_str(), (int)members.size(), (int)ids.size(), cx, cy, cz, radius, tf, scalar.HP, scalar.Dmg, snapped, dropHeight);
	ReportStatus(tag);
}

void Despawn(const std::string& tag) {
	BossMove::Forget(tag);
	{
		auto cl = s_ctl.find(tag);
		if (cl != s_ctl.end()) DomeOff(cl->second, tag);
	}
	s_groupTf.erase(tag);
	auto it = s_groups.find(tag);
	if (it != s_groups.end()) DestroyMembers(it->second);
	Logger::Log("worldgroups", "[WorldGroups] despawn tag=%s\n", tag.c_str());
	ReportStatus(tag);
	s_groups.erase(tag);
	s_contrib.erase(tag);
	s_ctl.erase(tag);
	ClearMoves(tag);
	s_engaged.erase(tag);
	s_area.erase(tag);
	s_barLink.erase(tag);
}

void OnCombat(int64_t charId, AActor* target, int64_t targetCharId, float magnitude, bool isHeal, AActor* attacker) {
	if (s_groups.empty() || charId <= 0 || target == nullptr || magnitude <= 0.0f) return;
	if (!isHeal) {
		for (const auto& [tag, members] : s_groups) {
			for (const Member& m : members) {
				if ((AActor*)m.pawn != target || m.pawn->r_nPawnId != m.pawn_id) continue;
				s_contrib[tag][charId].dmg += magnitude;
				s_engaged[tag][charId] = Now();
				auto cl = s_ctl.find(tag);
				if (cl != s_ctl.end() && attacker != nullptr) {
					cl->second.lastAttacker = attacker;  // fight back
					cl->second.lastAttackAt = Now();
				}
				if (cl != s_ctl.end() && cl->second.passive && attacker != nullptr) {
					// Hit while passive: back to the fight (HP kept), on the next Tick.
					cl->second.passive = false;
					cl->second.attacker = attacker;
					cl->second.interrupts++;
				}
				return;
			}
		}
		return;
	}
	// Heal: credited to every group the healed character is fighting (damaged a
	// member, or was hit by one, within kEngagedS); the healer counts as engaged.
	if (targetCharId <= 0 || targetCharId == charId) return;
	const float now = Now();
	for (auto& [tag, byChar] : s_engaged) {
		auto h = byChar.find(targetCharId);
		if (h == byChar.end() || now - h->second > kEngagedS) continue;
		s_contrib[tag][charId].heal += magnitude;
		byChar[charId] = now;
	}
}

void OnMemberHit(AActor* member, int64_t victimCharId) {
	if (s_groups.empty() || member == nullptr || victimCharId <= 0) return;
	for (const auto& [tag, members] : s_groups) {
		for (const Member& m : members) {
			if ((AActor*)m.pawn != member) continue;
			s_engaged[tag][victimCharId] = Now();
			++s_memberHits[member];
			return;
		}
	}
}

void Control(const nlohmann::json& j) {
	const std::string tag = j.value("tag", std::string());
	auto it = s_groups.find(tag);
	if (it == s_groups.end()) {
		Logger::Log("worldgroups", "[WorldGroups] control tag=%s: no such group\n", tag.c_str());
		ReportStatus(tag);
		return;
	}
	AWorldInfo* wi = CurrentWorldInfo();
	Ctl& c = s_ctl[tag];
	auto clampf = [&](const char* k, float lo, float hi, float def) {
		return std::fmax(lo, std::fmin(hi, j.value(k, def)));
	};
	c.regen      = clampf("regen_frac_s", 0.0f, 0.2f, c.regen);
	c.homeRadius = clampf("home_radius", 100.0f, 5000.0f, c.homeRadius);
	c.settleS    = clampf("settle_s", 0.0f, 120.0f, c.settleS);
	c.maxReturnS = clampf("max_return_s", 5.0f, 600.0f, c.maxReturnS);
	if (j.contains("passive")) {
		c.passive = j.value("passive", false);
		c.attacker = nullptr;
		c.startedAt = Now();
		c.homeAt = -1.0f;
		c.hpCarry.clear();
		if (c.passive) for (const Member& m : it->second) if (IsLive(wi, m.pawn)) ClearTarget(m.pawn);
	}
	if (j.contains("speed_scale")) {
		const float k = clampf("speed_scale", 0.5f, 2.0f, 1.0f);
		c.speedK = k;
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			auto b = c.baseSpeed.find(m.pawn);
			if (b == c.baseSpeed.end()) b = c.baseSpeed.emplace(m.pawn, m.pawn->GroundSpeed).first;
			m.pawn->SetProperty(GA_PROPERTY::TGPID_GROUND_SPEED, b->second * k);
			m.pawn->GroundSpeed = b->second * k;
			m.pawn->bNetDirty = 1;
		}
	}
	if (j.value("heal_full", false)) {
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			SetHp(m.pawn, m.pawn->r_nHealthMaximum, m.pawn->r_nHealthMaximum);
		}
	}
	c.regenCap = clampf("regen_cap_frac", 0.0f, 1.0f, c.regenCap);
	c.healCap = clampf("heal_cap_frac_s", 0.0f, 1.0f, c.healCap);
	if (j.contains("native_regen") && j["native_regen"].is_boolean()) c.nativeRegen = j["native_regen"].get<bool>();
	// Live rescale: max HP = spawn base x hp_scale, current HP keeps its fraction.
	if (j.contains("hp_scale") && j["hp_scale"].is_number()) {
		const float k = clampf("hp_scale", 0.05f, 50.0f, 1.0f);
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn) || m.baseHp <= 0.0f) continue;
			const int oldMax = std::max(1, m.pawn->r_nHealthMaximum);
			const int newMax = std::max(1, (int)std::lround(m.baseHp * k));
			const double frac = (double)m.pawn->Health / (double)oldMax;
			SetHp(m.pawn, std::max(1, (int)std::lround(frac * newMax)), newMax);
		}
		c.hpScale = k;
	}
	if (j.contains("dmg_scale") && j["dmg_scale"].is_number()) {
		const float k = clampf("dmg_scale", 0.0f, 10.0f, 1.0f);
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn) || m.baseDmg <= 0.0f) continue;
			m.pawn->s_fDamageAdjustment = m.baseDmg * k;
		}
		c.dmgScale = k;
	}
	// Engage (fight players: chase, then the behavior fights; walk home when idle).
	if (j.contains("engage") && j["engage"].is_boolean()) {
		c.engage = j["engage"].get<bool>();
		if (!c.engage) for (const Member& m : it->second) if (m.pawn && IsLive(wi, m.pawn) && m.pawn->Controller)
			s_runTo.erase((ATgAIController*)m.pawn->Controller);
	}
	c.engageAggro = clampf("engage_aggro", 500.0f, 15000.0f, c.engageAggro);
	c.engageHold  = clampf("engage_hold", 150.0f, 5000.0f, c.engageHold);
	// Dome (visual-only deployable over the first member while protect is on).
	if (j.contains("dome_id") && j["dome_id"].is_number_integer()) {
		const int d = j["dome_id"].get<int>();
		if (d != c.domeId) { DomeOff(c, tag); c.domeSpawns = 0; }
		c.domeId = (d > 0 && d < 100000) ? d : 0;
	}
	if (j.contains("dome_scale") && j["dome_scale"].is_number()) {
		c.domeScale = clampf("dome_scale", 0.25f, 30.0f, 1.0f);
		// A fitted dome keeps its fit (dome_scale is the no-bounds fallback only)
		if (DomeAlive(c) && c.domeOffX == 0.0f && c.domeOffY == 0.0f && c.domeOffZ == 0.0f && !c.domeFit)
			((AActor*)c.dome)->SetDrawScale(c.domeScale), c.domeUsedScale = c.domeScale, c.dome->bNetDirty = 1, c.dome->bForceNetUpdate = 1;
	}
	if (j.contains("dome_radius") && j["dome_radius"].is_number()) {  // refit on the next spawn of the dome
		const float r = clampf("dome_radius", 0.0f, 3000.0f, 0.0f);
		if (r != c.domeRadius) { c.domeRadius = r; DomeOff(c, tag); }
	}
	if (j.contains("dome_fit") && j["dome_fit"].is_boolean() && j["dome_fit"].get<bool>() != c.domeFit) {
		c.domeFit = j["dome_fit"].get<bool>(); DomeOff(c, tag);
	}
	c.domeZ = clampf("dome_z", -2000.0f, 2000.0f, c.domeZ);
	if (j.contains("area_radius") && j["area_radius"].is_number())
		s_area[tag] = clampf("area_radius", 500.0f, 20000.0f, 6000.0f);
	if (j.contains("aura_group") && j["aura_group"].is_number_integer()) {
		const int a = j["aura_group"].get<int>();
		if (a != c.auraGroup && c.protect && c.auraGroup > 0)  // swap the visual: drop the old one
			for (const Member& m : it->second) if (IsLive(wi, m.pawn)) SetProtect(m.pawn, false, c.auraGroup);
		c.auraGroup = (a > 0 && a <= 1000000) ? a : 0;
		if (c.protect) for (const Member& m : it->second) if (IsLive(wi, m.pawn)) SetProtect(m.pawn, true, m.lead ? c.auraGroup : 0);  // aura on the boss only
	}
	if (j.contains("protect") && j["protect"].is_boolean()) {
		const bool was = c.protect;
		c.protect = j["protect"].get<bool>();
		c.protectHp.clear();
		if (was != c.protect) c.riseLogs = 0;
		if (was && !c.protect) { c.keepHp.clear(); c.keepUntil = Now() + 3.0f; }
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			const int hp0 = m.pawn->Health;
			SetProtect(m.pawn, c.protect, (c.protect && !m.lead) ? 0 : c.auraGroup);  // aura visual on the boss only (off: remove from all)
			if (was && !c.protect) {
				c.keepHp[m.pawn] = hp0;
				const int hp1 = m.pawn->Health;
				if (m.pawn->Health > hp0) SetHp(m.pawn, hp0, m.pawn->r_nHealthMaximum);
				Logger::Log("worldgroups", "[WorldGroups] protect off tag=%s hp before=%d after_remove=%d now=%d\n",
					tag.c_str(), hp0, hp1, m.pawn->Health);
			}
			if (c.protect) c.protectHp[m.pawn] = (float)m.pawn->Health / (float)std::max(1, m.pawn->r_nHealthMaximum);
		}
		c.hpCarry.clear();
	}
	// Dev-only (control server forwards it only with dev_controls on): take dev_damage_frac
	// of max HP off every live member, credited to dev_credit_char as player damage;
	// a member at 0 HP dies through its own Suicide path.
	const float devFrac = clampf("dev_damage_frac", 0.0f, 1.0f, 0.0f);
	const int64_t devChar = j.value("dev_credit_char", (int64_t)0);
	if (devFrac > 0.0f && devChar > 0 && !c.protect) {  // protected members are immune to it too
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			const int dmg = std::max(1, (int)std::ceil(devFrac * (float)m.pawn->r_nHealthMaximum));
			s_contrib[tag][devChar].dmg += std::min(dmg, m.pawn->Health);
			s_engaged[tag][devChar] = Now();
			if (m.pawn->Health - dmg <= 0) {
				SetHp(m.pawn, 1, m.pawn->r_nHealthMaximum);
				m.pawn->eventSuicide();
			} else {
				SetHp(m.pawn, m.pawn->Health - dmg, m.pawn->r_nHealthMaximum);
			}
		}
		Logger::Log("worldgroups", "[WorldGroups] dev damage tag=%s frac=%.3f char=%lld\n",
			tag.c_str(), devFrac, (long long)devChar);
	}
	Logger::Log("worldgroups", "[WorldGroups] control tag=%s passive=%d regen=%.3f cap=%.3f home_r=%.0f settle=%.0f max_return=%.0f speed=%s heal_full=%d protect=%d aura=%d hp_scale=%.2f dmg_scale=%.2f native_regen=%d\n",
		tag.c_str(), (int)c.passive, c.regen, c.regenCap, c.homeRadius, c.settleS, c.maxReturnS,
		j.contains("speed_scale") ? j["speed_scale"].dump().c_str() : "-", (int)j.value("heal_full", false),
		(int)c.protect, c.auraGroup, c.hpScale, c.dmgScale, (int)c.nativeRegen);
	Logger::Log("worldgroups", "[WorldGroups] control tag=%s engage=%d aggro=%.0f hold=%.0f dome=%d scale=%.2f z=%.0f\n",
		tag.c_str(), (int)c.engage, c.engageAggro, c.engageHold, c.domeId, c.domeScale, c.domeZ);
	ReportStatus(tag);
}

void OnMemberHealed(AActor* a, float amount, ATgPawn* healer) {
	if (!a || amount <= 0.0f) return;
	// Only a live healer with a live owner counts: the heal sphere a Repair Drone leaves behind
	// (TgPawn_HoverShieldSphere, r_Owner = the drone) keeps healing after its drone and the shield
	// wave are dead and must not refill the boss. No allowance while protected (cap clamp rules).
	// That sphere often has no r_Owner at all, so "live" also needs the healer to be
	// a live world-group minion, or (a non-member heal source such as the sphere) a sibling group
	// of this boss (same tag prefix, e.g. wb-<map>-w50 / -esc) with a live member.
	auto alive = [](ATgPawn* p) { return p && !p->bDeleteMe && p->Health > 0; };
	AWorldInfo* wi = CurrentWorldInfo();
	const bool liveSelf = alive(healer) && (!healer->r_Owner || alive(healer->r_Owner));
	bool healerMember = false;
	if (liveSelf && wi)
		for (auto& [gt, mem] : s_groups) {
			for (const Member& m : mem) if (m.pawn == healer && IsLive(wi, m.pawn)) { healerMember = true; break; }
			if (healerMember) break;
		}
	for (auto& [tag, c] : s_ctl) {
		if (c.nativeRegen) continue;
		auto it = s_groups.find(tag);
		if (it == s_groups.end()) continue;
		bool siblingLive = false;
		const size_t dash = tag.rfind('-');
		if (liveSelf && !healerMember && wi && dash != std::string::npos) {
			const std::string pre = tag.substr(0, dash + 1);
			for (auto& [gt, mem] : s_groups) {
				if (gt == tag || gt.compare(0, pre.size(), pre) != 0) continue;
				for (const Member& m : mem) if (IsLive(wi, m.pawn)) { siblingLive = true; break; }
				if (siblingLive) break;
			}
		}
		const bool live = liveSelf && (healerMember || siblingLive);
		for (const Member& m : it->second) {
			if ((AActor*)m.pawn != a) continue;
			if (live && !c.protect) c.healAllow[m.pawn] += amount;
			if (c.healLogs < 8 || (!live && c.healLogs < 16)) {
				++c.healLogs;
				Logger::Log("worldgroups", "[WorldGroups] heal on member tag=%s amount=%.0f healer=%s owner=%d/%d live=%d protect=%d hp=%d\n",
					tag.c_str(), amount, healer ? ObjectClassCache::GetClassName((UObject*)healer).c_str() : "-",
					(int)(healer && healer->r_Owner), (int)(healer && alive(healer->r_Owner)), (int)live, (int)c.protect, m.pawn->Health);
			}
		}
	}
}

bool IsMember(AActor* a) {
	if (a == nullptr || s_groups.empty()) return false;
	for (const auto& [tag, members] : s_groups)
		for (const auto& m : members) if ((AActor*)m.pawn == a) return true;
	return false;
}

namespace {
// Boss bar: ~4 Hz, per bar boss: marker = 0xB0000000 | shielded<<27 |
// min(255, live members of the other linked tags)<<16 | HP per mille; viewers =
// characters engaged with a linked tag within kBarS and inside the boss tag's area.
void BarTick(AWorldInfo* wi) {
	if (s_barLink.empty()) { s_bars.clear(); return; }
	const float now = wi->TimeSeconds;
	if (now - s_barLast < 0.25f && now >= s_barLast) return;
	s_barLast = now;
	s_bars.clear();
	for (const auto& [tag, link] : s_barLink) {
		auto g = s_groups.find(tag);
		if (g == s_groups.end() || g->second.empty()) continue;
		ATgPawn* boss = g->second[0].pawn;
		if (!IsLive(wi, boss)) continue;
		ATgRepInfo_Player* pri = (ATgRepInfo_Player*)boss->PlayerReplicationInfo;
		if (!pri) continue;
		int minions = 0;
		std::map<int64_t, float> seen;
		for (const auto& [t, members] : s_groups) {
			if (t.compare(0, link.size(), link) != 0) continue;
			if (t != tag) for (const Member& m : members) if (IsLive(wi, m.pawn)) ++minions;
			auto en = s_engaged.find(t);
			if (en == s_engaged.end()) continue;
			for (const auto& [cid, ts] : en->second) if (ts > seen[cid]) seen[cid] = ts;
		}
		auto cl = s_ctl.find(tag);
		const bool shielded = cl != s_ctl.end() && cl->second.protect;
		const int hpMax = std::max(1, boss->r_nHealthMaximum);
		const int pm = std::max(1, std::min(1000, (int)std::lround(1000.0 * boss->Health / hpMax)));
		BarState st{(int)(0xB0000000u | (shielded ? 1u << 27 : 0u) | ((unsigned)std::min(255, minions) << 16) | (unsigned)pm), {}};
		auto ar = s_area.find(tag);
		const float area = ar != s_area.end() ? ar->second : 6000.0f;
		for (AController* c = wi->ControllerList; c; c = c->NextController) {
			if (!c->Pawn || c->Pawn->bDeleteMe || !ObjectClassCache::ClassNameContains(c, "PlayerController")) continue;
			const int64_t cid = (int64_t)((ATgPawn_Character*)c->Pawn)->s_nCharacterId;
			auto f = seen.find(cid);
			if (f == seen.end() || now - f->second > kBarS) continue;
			const float dx = c->Pawn->Location.X - boss->Location.X, dy = c->Pawn->Location.Y - boss->Location.Y;
			if (dx * dx + dy * dy > area * area) continue;
			st.viewers.push_back(cid);
		}
		// PRIs replicate at ~1 Hz by default: a little faster while a bar is up.
		if (pri->NetUpdateFrequency < 4.0f) pri->NetUpdateFrequency = 4.0f;
		pri->bNetDirty = 1;
		s_bars[pri] = std::move(st);
	}
}
} // namespace

namespace {
// Clear world trace from the member's eye to the foe's head or centre.
bool ClearShot(APawn* bot, APawn* foe) {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game || !bot || !foe) return false;
	const float ch = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 60.0f;
	FVector eye = bot->Location; eye.Z += std::fmax(bot->BaseEyeHeight, 0.7f * ch);
	auto clear = [&](FVector to) {
		FVector hitLoc, hitNorm;
		FTraceHitInfo hitInfo;
		std::memset(&hitInfo, 0, sizeof(hitInfo));
		FVector start = eye;
		return !Game->Trace(to, start, 0, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo);
	};
	FVector head = foe->Location; head.Z += foe->BaseEyeHeight;
	if (clear(head) || clear(foe->Location)) return true;
	eye = bot->Location; eye.Z += ch + 40.0f;  // big pawns: the guns sit high (Juggernaut half height 131)
	return clear(head) || clear(foe->Location);
}

// ---------------------------------------------------------------------------
// Boss move set: data-driven ability rotation for attack-drive members.
// Each move = one of the boss's own equipped devices (equip point) + how its hit is
// resolved here. StartAction plays the device's real animation/visual (the telegraph:
// stomp/cleave buildup, plasma projectile); after the move's windup (device buildup /
// hit time from the data, + projectile travel) the move's own DB hit effect groups
// (damage, knockback, stun, slow - normal resists + the boss's damage scale) land on
// every hostile player the geometry catches at that moment (AoE ring around the boss,
// melee cone in front of it, blast at the aimed spot), so players can step out.
// The minigun is real aimed fire: hitscan traces from the gun muzzle to the target's
// body with the device's spread; a trace that hits a player is a hit, cover blocks it.
// Priority order + conditions follow the original Colony_Juggernaut_Combat_AI table
// (asm bot behavior 762): stomp when enemies are in melee range, cleave/bash close,
// EMP below 60 % HP at range, pulse cannon after 8 s in combat, minigun otherwise.
// ---------------------------------------------------------------------------
double WallNow() { return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); }
enum MoveKind { kMvGun = 0, kMvAoe = 1, kMvCone = 2, kMvBlast = 3 };
struct BossMoveDef {
	std::string name;
	int pt = -1, kind = kMvGun;
	float minD = 0.0f, maxD = 1000.0f;   // 2D centre distance to the foe for picking it
	float radius = 300.0f;               // aoe / blast radius, cone reach (from the boss's edge)
	float cone = 120.0f;                 // cone width, degrees
	float cd = 5.0f, windup = 0.5f, busy = 1.0f;
	float chance = 1.0f, chance1 = -1.0f; // chance per pick; chance1: when fewer than `need` foes are inside
	int need = 1;
	float hpBelow = 2.0f, combatS = 0.0f, dmg = 1.0f, projSpeed = 0.0f, push = 0.0f, pushZ = 0.0f;
	bool hand = true, apply = true, los = true;
	int shots = 3; float spread = 3.0f, hold = 3.0f;  // gun: traces per 0.5 s tick, cone deg, burst length
	std::vector<int> hit;
	std::vector<int> cc; float ccLock = 0.0f;  // crowd-control hit ids + per-victim lock after they land
};
// Victim -> game time until which boss crowd control (stun/slow groups listed in a move's "cc") is skipped.
std::map<APawn*, float> s_ccUntil;
int s_ccApplied = 0, s_ccSkipped = 0;
struct BossMoveStat { int uses = 0, hits = 0, victims = 0, shots = 0, dmg = 0, fails = 0; };
struct BossMoveState {
	std::map<int, float> lastUse;
	int cur = -1, want = -1, wantTicks = 0;
	float busyUntil = -1.0f, combatSince = -1.0f;
	int pend = -1; float resolveAt = 0.0f; FVector pendAt{}; int pendYaw = 0;  // pend = last started (status)
	struct Pending { int idx; float at; FVector spot; int yaw; };
	std::vector<Pending> pends;  // hits in flight (an EMP still flying when the next move starts)
	std::map<int, BossMoveStat> st;
	std::string last;
	int logs = 0, muzzleSock = 0, muzzleFallback = 0, nopick = 0;
	std::string why;
};
std::map<std::string, std::vector<BossMoveDef>> s_moves;           // tag -> moves (priority order)
std::map<std::string, std::map<ATgPawn*, BossMoveState>> s_moveSt;   // tag -> member -> state

void ParseMoves(const std::string& tag, const nlohmann::json& arr) {
	std::vector<BossMoveDef>& v = s_moves[tag];
	v.clear();
	if (!arr.is_array()) return;
	auto f = [](const nlohmann::json& o, const char* k, float lo, float hi, float d) {
		return o.contains(k) && o[k].is_number() ? std::fmax(lo, std::fmin(hi, o[k].get<float>())) : d; };
	for (const auto& o : arr) {
		if (!o.is_object() || v.size() >= 12) continue;
		BossMoveDef m;
		m.name = o.value("name", std::string("move"));
		m.pt = o.contains("pt") && o["pt"].is_number_integer() ? o["pt"].get<int>() : -1;
		if (m.pt < 0 || m.pt > 24) continue;
		const std::string k = o.value("kind", std::string("gun"));
		m.kind = k == "aoe" ? kMvAoe : k == "cone" ? kMvCone : k == "blast" ? kMvBlast : kMvGun;
		m.minD = f(o, "min", 0, 20000, m.minD);   m.maxD = f(o, "max", 50, 20000, m.maxD);
		m.radius = f(o, "radius", 0, 3000, m.radius); m.cone = f(o, "cone", 10, 360, m.cone);
		m.cd = f(o, "cd", 0, 300, m.cd); m.windup = f(o, "windup", 0, 10, m.windup); m.busy = f(o, "busy", 0, 15, m.busy);
		m.chance = f(o, "chance", 0, 1, m.chance); m.chance1 = f(o, "chance1", -1, 1, m.chance1);
		m.need = (int)f(o, "need", 1, 20, (float)m.need);
		m.hpBelow = f(o, "hp_below", 0, 2, m.hpBelow); m.combatS = f(o, "combat_s", 0, 600, m.combatS);
		m.dmg = f(o, "dmg", 0, 20, m.dmg); m.projSpeed = f(o, "proj_speed", 0, 50000, m.projSpeed);
		m.push = f(o, "push", 0, 5000, m.push); m.pushZ = f(o, "push_z", 0, 3000, m.pushZ);
		m.hand = o.value("hand", m.kind != kMvAoe); m.apply = o.value("apply", true); m.los = o.value("los", true);
		m.shots = (int)f(o, "shots", 1, 10, (float)m.shots); m.spread = f(o, "spread", 0, 45, m.spread); m.hold = f(o, "hold", 0.5f, 30, m.hold);
		if (o.contains("hit") && o["hit"].is_array())
			for (const auto& h : o["hit"]) if (h.is_number_integer() && h.get<int>() > 0 && m.hit.size() < 8) m.hit.push_back(h.get<int>());
		if (o.contains("cc") && o["cc"].is_array())
			for (const auto& h : o["cc"]) if (h.is_number_integer() && m.cc.size() < 8) m.cc.push_back(h.get<int>());
		m.ccLock = f(o, "cc_lock", 0, 60, m.ccLock);
		v.push_back(m);
	}
	Logger::Log("worldgroups", "[WorldGroups] moves tag=%s: %d moves\n", tag.c_str(), (int)v.size());
}

bool MoveFoe(APawn* p, int gtf, bool bots) {
	if (!p || p->bDeleteMe || p->Health <= 0 || !HostileTo(p, gtf)) return false;
	return IsPlayerPawn(p) || (bots && p->Controller != nullptr);
}

// World-only LOS between two points (no actors).
bool WorldClear(FVector from, FVector to) {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return false;
	FVector hl, hn; FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
	return !Game->Trace(to, from, 0, FVector(0, 0, 0), 0, &hl, &hn, &hi);
}

float Ch(APawn* p) { return p && p->CylinderComponent ? p->CylinderComponent->CollisionHeight : 60.0f; }
float Cr(APawn* p) { return p && p->CylinderComponent ? p->CylinderComponent->CollisionRadius : 30.0f; }

// Land one move's hit groups on a victim. Damage effects (prop 51) are scaled by dmg.
int LandMoveHit(const BossMoveDef& mv, ATgPawn* bot, APawn* victim, FVector from) {
	ATgPawn* tp = ObjectClassCache::ClassNameContains((UObject*)victim, "TgPawn") ? (ATgPawn*)victim : nullptr;
	if (!tp || !tp->r_EffectManager || tp->Health <= 0) return 0;
	const int hp0 = tp->Health;
	const float ccNow = Now();
	const bool ccFree = mv.cc.empty() || ccNow >= s_ccUntil[victim];
	bool ccLanded = false;
	for (int eg : mv.hit) {
		const bool isCc = std::find(mv.cc.begin(), mv.cc.end(), eg) != mv.cc.end();
		if (isCc && !ccFree) { s_ccSkipped++; continue; }  // still inside the lock of an earlier boss stun/slow
		if (isCc) ccLanded = true;
		UTgEffectGroup* g = BuildEffectGroup(eg, 264);
		if (!g) continue;
		if (mv.dmg != 1.0f && g->m_Effects.Data)
			for (int i = 0; i < g->m_Effects.Count; i++) {
				UTgEffect* e = g->m_Effects.Data[i];
				if (e && e->m_nPropertyId == 51) e->m_fBase *= mv.dmg;
			}
		FImpactInfo impact{};
		impact.HitActor = (AActor*)victim;
		impact.HitLocation = victim->Location;
		FVector dir(victim->Location.X - from.X, victim->Location.Y - from.Y, victim->Location.Z - from.Z);
		const float l = std::fmax(1.0f, std::sqrt(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z));
		impact.HitNormal = FVector(-dir.X / l, -dir.Y / l, -dir.Z / l);
		impact.RayDir = FVector(dir.X / l, dir.Y / l, dir.Z / l);
		impact.StartTrace = from;
		tp->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
		if (tp->Health <= 0) break;
	}
	if (ccLanded) {
		s_ccUntil[victim] = ccNow + mv.ccLock;
		s_ccApplied++;
		Logger::Log("worldgroups", "[WorldGroups] cc %s on pawn %d: locked %.1f s (applied %d skipped %d)\n",
			mv.name.c_str(), tp->r_nPawnId, mv.ccLock, s_ccApplied, s_ccSkipped);
	} else if (!ccFree) {
		Logger::Log("worldgroups", "[WorldGroups] cc %s on pawn %d: skipped (locked %.1f s more)\n",
			mv.name.c_str(), tp->r_nPawnId, s_ccUntil[victim] - ccNow);
	}
	if (mv.push > 0.0f && tp->Health > 0) {  // shove away from the hit origin (stomp ring / cleave arc)
		FVector d(victim->Location.X - from.X, victim->Location.Y - from.Y, 0.0f);
		const float l = std::fmax(1.0f, std::sqrt(d.X * d.X + d.Y * d.Y));
		FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
		victim->AddVelocity(FVector(d.X / l * mv.push, d.Y / l * mv.push, mv.pushZ), victim->Location, nullptr, hi);
	}
	return std::max(0, hp0 - tp->Health);
}

// Gun muzzle: the device's fire socket on the boss mesh, else the gun-arm spot (right, chest high).
FVector Muzzle(ATgPawn* bot, ATgDevice* dev, BossMoveState& s) {
	const float ch = Ch(bot), cr = Cr(bot);
	const float yaw = bot->Rotation.Yaw * 3.14159265f / 32768.0f;
	FVector fb = bot->Location;
	fb.X += std::cos(yaw) * cr * 0.9f - std::sin(yaw) * cr * 0.55f;
	fb.Y += std::sin(yaw) * cr * 0.9f + std::cos(yaw) * cr * 0.55f;
	fb.Z += ch * 0.35f;
	if (dev && bot->Mesh && bot->Mesh->SkeletalMesh) {
		FName sock = dev->GetFireSocketName();
		if (sock.Index != 0) {
			FVector loc(0, 0, 0); FRotator rot{};
			bot->Mesh->GetSocketWorldLocationAndRotation(sock, &loc, &rot);
			const float dx = loc.X - bot->Location.X, dy = loc.Y - bot->Location.Y, dz = loc.Z - bot->Location.Z;
			if ((loc.X != 0 || loc.Y != 0) && dx * dx + dy * dy + dz * dz < 4.0f * (ch + cr) * (ch + cr)) { s.muzzleSock++; return loc; }
		}
	}
	s.muzzleFallback++;
	return fb;
}

// Start a device for its animation/visual (same proven route as AttackTick).
bool StartDevice(ATgPawn* bot, ATgAIController* ai, ATgDevice* dev, APawn* foe, int pt) {
	auto stName = [](ATgDevice* d) -> std::string { const char* n = d ? d->GetStateName().GetName() : nullptr; return n ? n : "?"; };
	auto busySt = [](const std::string& n) { return n == "DeviceBuildup" || n == "DeviceFiring"; };
	bot->StopAction(dev, 0);
	if (dev->m_FireMode.Count > 0 && (dev->CurrentFireMode < 0 || dev->CurrentFireMode >= dev->m_FireMode.Count)) dev->CurrentFireMode = 0;
	FVector eye = bot->Location; eye.Z += std::fmax(bot->BaseEyeHeight, 0.7f * Ch(bot));
	ai->Location = eye;
	if (foe) {
		const float dx = foe->Location.X - bot->Location.X, dy = foe->Location.Y - bot->Location.Y;
		const float dz = foe->Location.Z - (bot->Location.Z + 0.7f * Ch(bot));
		FRotator r;
		r.Yaw = (int)(std::atan2(dy, dx) * 32768.0 / 3.14159265);
		r.Pitch = (int)(std::atan2(dz, std::sqrt(dx * dx + dy * dy)) * 32768.0 / 3.14159265);
		r.Roll = 0;
		ai->Rotation = r;
	}
	bot->StartAction(dev, 0);
	if (busySt(stName(dev))) return true;
	dev->ServerStartFire(foe && ObjectClassCache::ClassNameContains((UObject*)foe, "TgPawn") ? (ATgPawn*)foe : nullptr, 0);
	if (busySt(stName(dev))) return true;
	ai->UseDevice((unsigned char)pt, 0, 0, 0);
	return busySt(stName(dev));
}

void ResolveMove(const std::string& tag, const BossMoveDef& mv, BossMoveState& s, ATgPawn* bot, int gtf, bool bots, AWorldInfo* wi) {
	BossMoveStat& st = s.st[s.pend];
	FVector centre = mv.kind == kMvBlast ? s.pendAt : bot->Location;
	const float br = Cr(bot);
	const float yaw = (mv.kind == kMvCone ? bot->Rotation.Yaw : s.pendYaw) * 3.14159265f / 32768.0f;
	int victims = 0, dmg = 0;
	for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn) {
		if (!MoveFoe(p, gtf, bots)) continue;
		const float dx = p->Location.X - centre.X, dy = p->Location.Y - centre.Y, dz = p->Location.Z - centre.Z;
		const float d2 = std::sqrt(dx * dx + dy * dy);
		float reach = mv.radius + Cr(p);
		if (mv.kind != kMvBlast) reach += br;
		if (d2 > reach) continue;
		if (std::fabs(dz) > std::fmax(mv.radius, Ch(bot) + Ch(p) + 60.0f)) continue;
		if (mv.kind == kMvCone && d2 > 1.0f) {
			float a = std::atan2(dy, dx) - yaw;
			while (a > 3.14159265f) a -= 6.2831853f;
			while (a < -3.14159265f) a += 6.2831853f;
			if (std::fabs(a) * 180.0f / 3.14159265f > mv.cone * 0.5f) continue;
		}
		if (mv.los) {
			FVector from = centre; from.Z += mv.kind == kMvBlast ? 40.0f : 0.5f * Ch(bot);
			FVector to = p->Location; to.Z += 0.3f * Ch(p);
			if (!WorldClear(from, to)) continue;
		}
		victims++;
		dmg += mv.apply ? LandMoveHit(mv, bot, p, centre) : 0;
	}
	st.victims += victims; st.dmg += dmg; if (victims) st.hits++;
	if (s.logs < 400) { ++s.logs; Logger::Log("worldgroups",
		"[WorldGroups] move tag=%s %s resolve wall=%.2f at (%.0f,%.0f,%.0f) r=%.0f victims=%d dmg=%d\n",
		tag.c_str(), mv.name.c_str(), WallNow(), centre.X, centre.Y, centre.Z, mv.radius, victims, dmg); }
}

// One 0.5 s tick of the minigun: `shots` real traces from the muzzle at the foe's body.
void GunVolley(const std::string& tag, const BossMoveDef& mv, BossMoveState& s, ATgPawn* bot, ATgDevice* dev, APawn* foe, int gtf, bool bots) {
	BossMoveStat& st = s.st[s.cur];
	const FVector mz = Muzzle(bot, dev, s);
	FVector aim = foe->Location; aim.Z += 0.25f * Ch(foe);
	const float dx = aim.X - mz.X, dy = aim.Y - mz.Y, dz = aim.Z - mz.Z;
	const float len = std::fmax(1.0f, std::sqrt(dx * dx + dy * dy + dz * dz));
	FVector f(dx / len, dy / len, dz / len);
	// basis perpendicular to f
	FVector up(0, 0, 1);
	FVector r(f.Y * up.Z - f.Z * up.Y, f.Z * up.X - f.X * up.Z, f.X * up.Y - f.Y * up.X);
	float rl = std::fmax(1e-4f, std::sqrt(r.X * r.X + r.Y * r.Y + r.Z * r.Z)); r.X /= rl; r.Y /= rl; r.Z /= rl;
	FVector u(r.Y * f.Z - r.Z * f.Y, r.Z * f.X - r.X * f.Z, r.X * f.Y - r.Y * f.X);
	const float range = std::fmax(len + 200.0f, mv.maxD + 300.0f);
	const float sp = std::tan(mv.spread * 3.14159265f / 180.0f);
	for (int k = 0; k < mv.shots; ++k) {
		const float a = (float)std::rand() / RAND_MAX * 6.2831853f, rr = std::sqrt((float)std::rand() / RAND_MAX) * sp;
		FVector d(f.X + (r.X * std::cos(a) + u.X * std::sin(a)) * rr, f.Y + (r.Y * std::cos(a) + u.Y * std::sin(a)) * rr,
			f.Z + (r.Z * std::cos(a) + u.Z * std::sin(a)) * rr);
		const float dl = std::sqrt(d.X * d.X + d.Y * d.Y + d.Z * d.Z);
		FVector end(mz.X + d.X / dl * range, mz.Y + d.Y / dl * range, mz.Z + d.Z / dl * range);
		FVector hl, hn; FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
		AActor* hit = ((AActor*)bot)->Trace(end, mz, 1, FVector(0, 0, 0), 0, &hl, &hn, &hi);
		st.shots++;
		if (!hit || !ObjectClassCache::ClassNameContains((UObject*)hit, "Pawn")) continue;
		APawn* hp = (APawn*)hit;
		if (!MoveFoe(hp, gtf, bots)) continue;
		st.hits++; st.victims++;
		st.dmg += mv.apply ? LandMoveHit(mv, bot, hp, mz) : 0;
	}
}

// Called from AttackTick when the group has moves. Returns after at most one start per tick.
void MovesTick(AWorldInfo* wi, const std::string& tag, Ctl& c, const Member& m, ATgAIController* ai, APawn* foe, float dist, float now) {
	std::vector<BossMoveDef>& mvs = s_moves[tag];
	BossMoveState& s = s_moveSt[tag][m.pawn];
	ATgPawn* bot = m.pawn;
	c.atk[bot];  // status block (atk.moves) exists from the first tick
	const int gtf = s_groupTf.count(tag) ? s_groupTf[tag] : 1;
	const bool bots = BossMove::TargetsBots(tag);
	auto devAt = [&](int pt) -> ATgDevice* { return (pt >= 0 && pt < 25) ? bot->m_EquippedDevices[pt] : nullptr; };
	auto stopGun = [&]() {
		if (s.cur >= 0 && s.cur < (int)mvs.size() && mvs[s.cur].kind == kMvGun) {
			if (ATgDevice* d = devAt(mvs[s.cur].pt)) bot->StopAction(d, 0);
			ai->StopDevices();
		}
	};
	// A pending hit resolves even if the foe died or left (the stomp still lands on whoever is there).
	for (size_t k = 0; k < s.pends.size();) {
		const BossMoveState::Pending pd = s.pends[k];
		if (now < pd.at || pd.idx < 0 || pd.idx >= (int)mvs.size()) { ++k; continue; }
		s.pends.erase(s.pends.begin() + k);
		const int keepIdx = s.pend; const FVector keepAt = s.pendAt; const int keepYaw = s.pendYaw;
		s.pend = pd.idx; s.pendAt = pd.spot; s.pendYaw = pd.yaw;
		ResolveMove(tag, mvs[pd.idx], s, bot, gtf, bots, wi);
		s.pend = keepIdx == pd.idx ? -1 : keepIdx; s.pendAt = keepAt; s.pendYaw = keepYaw;
	}
	if (s.cur >= 0 && s.cur < (int)mvs.size() && mvs[s.cur].kind != kMvGun && now >= s.busyUntil) {
		if (ATgDevice* d = devAt(mvs[s.cur].pt)) bot->StopAction(d, 0);  // swing over -> firing self-buffs end
		s.cur = -1;
	}
	const bool ok = foe && foe->Health > 0 && !foe->bDeleteMe;
	if (!ok) { stopGun(); s.cur = -1; s.combatSince = -1.0f; return; }
	if (s.combatSince < 0) s.combatSince = now;
	ai->Focus = (AActor*)foe;
	if (ai->Enemy != foe) { ai->SetTargetActor((AActor*)foe); ai->Enemy = foe; }
	const bool sight = ClearShot(bot, foe);
	if (sight) c.lastShotChance[bot] = now;
	// Gun burst in progress: keep shooting (real traces) while the foe is in range + sight.
	if (s.cur >= 0 && s.cur < (int)mvs.size() && now < s.busyUntil) {
		const BossMoveDef& mv = mvs[s.cur];
		if (mv.kind != kMvGun) return;  // committed to a stomp/cleave/blast animation
		ATgDevice* d = devAt(mv.pt);
		if (d && sight && dist <= mv.maxD + 100.0f) {
			const char* n = d->GetStateName().GetName();
			const std::string sn = n ? n : "?";
			if (sn != "DeviceBuildup" && sn != "DeviceFiring") StartDevice(bot, ai, d, foe, mv.pt);  // keep the firing anim going
			GunVolley(tag, mv, s, bot, d, foe, gtf, bots);
			return;
		}
		stopGun(); s.busyUntil = now;
	}
	// Pick the first move (priority order) whose conditions hold.
	const float hpFrac = bot->r_nHealthMaximum > 0 ? (float)bot->Health / (float)bot->r_nHealthMaximum : 1.0f;
	int pick = -1;
	std::string why;
	for (int i = 0; i < (int)mvs.size() && pick < 0; ++i) {
		const BossMoveDef& mv = mvs[i];
		ATgDevice* d = devAt(mv.pt);
		if (!d) { why += mv.name + ":nodev "; continue; }
		auto lu = s.lastUse.find(i);
		if (lu != s.lastUse.end() && now - lu->second < mv.cd) { why += mv.name + ":cd "; continue; }
		if (dist < mv.minD || dist > mv.maxD) { why += mv.name + ":dist "; continue; }
		if (hpFrac >= mv.hpBelow) { why += mv.name + ":hp "; continue; }
		if (now - s.combatSince < mv.combatS) { why += mv.name + ":combat "; continue; }
		if (mv.los && !sight) { why += mv.name + ":los "; continue; }
		float chance = mv.chance;
		if (mv.kind == kMvAoe) {
			int n = 0;
			for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn) {
				if (!MoveFoe(p, gtf, bots)) continue;
				const float dx = p->Location.X - bot->Location.X, dy = p->Location.Y - bot->Location.Y;
				if (std::sqrt(dx * dx + dy * dy) <= mv.radius + Cr(bot) + Cr(p)) n++;
			}
			if (n == 0) { why += mv.name + ":n0 "; continue; }
			if (n < mv.need) { if (mv.chance1 < 0) { why += mv.name + ":need "; continue; } chance = mv.chance1; }
		}
		if (i == s.want && s.wantTicks > 0) chance = 1.0f;  // already chosen, waiting on the weapon switch
		if (chance < 1.0f && (float)std::rand() / RAND_MAX >= chance) { why += mv.name + ":roll "; continue; }
		pick = i;
	}
	s.why = why;
	if (pick < 0) { s.nopick++; stopGun(); s.cur = -1; return; }
	const BossMoveDef& mv = mvs[pick];
	ATgDevice* dev = devAt(mv.pt);
	Ctl::Atk& a = c.atk[bot];
	// In-hand devices need the switch first (takes a tick).
	if (mv.hand && (ATgDevice*)bot->Weapon != dev) {
		if (s.want != pick) { s.want = pick; s.wantTicks = 0; }
		if (++s.wantTicks > 4) { s.st[pick].fails++; s.lastUse[pick] = now; s.want = -1; return; }
		stopGun();
		bot->eventSetActiveWeapon((AWeapon*)dev, 0, 1);
		if ((ATgDevice*)bot->Weapon != dev) {
			bot->Weapon = (AWeapon*)dev;
			bot->m_CurrentInHandDevice = dev;
			bot->r_eDesiredInHand = (unsigned char)mv.pt;
			dev->Instigator = (APawn*)bot;
			bot->bNetDirty = 1;
			a.forced++;
		}
		a.sw++;
		return;
	}
	s.want = -1; s.wantTicks = 0;
	if (s.cur != pick) stopGun();
	const bool started = StartDevice(bot, ai, dev, foe, mv.pt);
	BossMoveStat& st = s.st[pick];
	if (!started && mv.kind != kMvGun) {  // no animation = no hit; retry in ~1 s
		st.fails++;
		s.lastUse[pick] = now - mv.cd + 1.0f;
		s.cur = -1; s.busyUntil = now;
		if (s.logs < 400) { ++s.logs; Logger::Log("worldgroups",
			"[WorldGroups] move tag=%s pawn=%d start %s wall=%.2f pt=%d dev=%d started=0 dist=%.0f: no anim, no hit, retry 1 s\n",
			tag.c_str(), m.pawn_id, mv.name.c_str(), WallNow(), mv.pt, dev->r_nDeviceId, dist); }
		return;
	}
	s.lastUse[pick] = now;
	s.cur = pick;
	s.last = mv.name;
	if (!started) st.fails++;
	st.uses++;
	a.starts++; a.pt = mv.pt;
	if (mv.kind == kMvGun) {
		s.busyUntil = now + mv.hold;
		GunVolley(tag, mv, s, bot, dev, foe, gtf, bots);
	} else {
		s.busyUntil = now + mv.busy;
		s_runTo.erase(ai); ai->m_bInterrupt = 1;  // plant the feet for the swing
		bot->Velocity.X = 0.0f; bot->Velocity.Y = 0.0f;
		ai->StopLatentExecution(); bot->Acceleration.X = 0.0f; bot->Acceleration.Y = 0.0f;  // end the running MoveTo now
		s.pend = pick;
		s.pendYaw = bot->Rotation.Yaw;
		s.pendAt = foe->Location;
		float t = mv.windup;
		if (mv.kind == kMvBlast && mv.projSpeed > 0.0f) {
			const float dx = foe->Location.X - bot->Location.X, dy = foe->Location.Y - bot->Location.Y, dz = foe->Location.Z - bot->Location.Z;
			t += std::sqrt(dx * dx + dy * dy + dz * dz) / mv.projSpeed;
		}
		s.resolveAt = now + t;
		if (s.pends.size() < 8) s.pends.push_back({pick, s.resolveAt, s.pendAt, s.pendYaw});
		if (mv.kind == kMvAoe) a.aoe++;
	}
	if (s.logs < 400) { ++s.logs; Logger::Log("worldgroups",
		"[WorldGroups] move tag=%s pawn=%d start %s wall=%.2f pt=%d dev=%d started=%d dist=%.0f hp=%.2f foe_hp=%d\n",
		tag.c_str(), m.pawn_id, mv.name.c_str(), WallNow(), mv.pt, dev->r_nDeviceId, (int)started, dist, hpFrac, foe->Health); }
}

nlohmann::json MovesStatus(const std::string& tag, ATgPawn* p) {
	nlohmann::json j = nlohmann::json::object();
	auto t = s_moveSt.find(tag);
	if (t == s_moveSt.end()) return j;
	auto it = t->second.find(p);
	if (it == t->second.end()) return j;
	const BossMoveState& s = it->second;
	const std::vector<BossMoveDef>& mvs = s_moves[tag];
	for (const auto& kv : s.st) {
		if (kv.first < 0 || kv.first >= (int)mvs.size()) continue;
		const BossMoveStat& x = kv.second;
		j[mvs[kv.first].name] = {{"uses", x.uses}, {"hits", x.hits}, {"victims", x.victims}, {"shots", x.shots}, {"dmg", x.dmg}, {"fails", x.fails}};
	}
	j["_last"] = s.last;
	j["_pend"] = s.pend >= 0 && s.pend < (int)mvs.size() ? mvs[s.pend].name : std::string("-");
	if (AWorldInfo* wi = CurrentWorldInfo()) {  // agents near the boss (HP of everyone a move can hit)
		nlohmann::json f = nlohmann::json::array();
		for (APawn* q = wi->PawnList; q != nullptr && f.size() < 8; q = q->NextPawn) {
			if (!IsPlayerPawn(q) || q->bDeleteMe) continue;
			const float dx = q->Location.X - p->Location.X, dy = q->Location.Y - p->Location.Y;
			const float d = std::sqrt(dx * dx + dy * dy);
			if (d > 4000.0f) continue;
			f.push_back({{"hp", q->Health}, {"d", (int)d}, {"x", (int)q->Location.X}, {"y", (int)q->Location.Y}, {"z", (int)q->Location.Z}});
		}
		j["_foes"] = f;
	}
	j["_yaw"] = p->Rotation.Yaw & 65535;
	j["_why"] = s.why; j["_nopick"] = s.nopick;
	j["_cc"] = {{"applied", s_ccApplied}, {"skipped", s_ccSkipped}};
	j["_muzzle"] = {{"sock", s.muzzleSock}, {"fallback", s.muzzleFallback}};
	return j;
}

void ClearMoves(const std::string& tag) { s_moves.erase(tag); s_moveSt.erase(tag); }

// Attack drive (see Ctl::atkBots). Called every 0.5 s per live member.
void AttackTick(const std::string& tag, Ctl& c, const Member& m, ATgAIController* ai, APawn* foe, float dist, float now) {
	if (c.atkBots.empty() || std::find(c.atkBots.begin(), c.atkBots.end(), m.bot_id) == c.atkBots.end()) return;
	{	// data-driven move set
		auto mv = s_moves.find(tag);
		if (mv != s_moves.end() && !mv->second.empty()) { if (AWorldInfo* wi = CurrentWorldInfo()) MovesTick(wi, tag, c, m, ai, foe, dist, now); return; }
	}
	ATgPawn* bot = m.pawn;
	Ctl::Atk& a = c.atk[bot];
	auto devAt = [&](int pt) -> ATgDevice* { return (pt >= 0 && pt < 25) ? bot->m_EquippedDevices[pt] : nullptr; };
	auto stop = [&]() {
		ATgDevice* cur = (ATgDevice*)bot->Weapon;
		if (cur) bot->StopAction(cur, 0);
		ai->StopDevices();
		a.firing = false;
	};
	const bool inRange = foe && foe->Health > 0 && !foe->bDeleteMe && dist <= c.atkRange;
	const bool sight = inRange && ClearShot(bot, foe);
	if (inRange && !sight) { a.noSight++; a.noSightRun++; }
	if (!sight) { if (a.firing) stop(); return; }
	a.noSightRun = 0;
	ai->Focus = (AActor*)foe;
	if (ai->Enemy != foe) { ai->SetTargetActor((AActor*)foe); ai->Enemy = foe; }
	// Off-hand AoE (stomp): no switch needed, on a cooldown, only close in.
	if (dist <= c.atkAoeRange && now - a.lastAoe >= c.atkAoeCd) {
		for (int pt : c.atkAoe) {
			ATgDevice* d = devAt(pt);
			if (!d) continue;
			if (d->m_FireMode.Count > 0 && (d->CurrentFireMode < 0 || d->CurrentFireMode >= d->m_FireMode.Count)) d->CurrentFireMode = 0;
			const bool ok = bot->StartAction(d, 0);  // mode 0 only
			a.lastAoe = now;
			if (ok) a.aoe++;
			if (a.logs < 60) { ++a.logs; Logger::Log("worldgroups", "[WorldGroups] attack tag=%s pawn=%d aoe pt=%d dev=%d ok=%d dist=%.0f\n",
				tag.c_str(), m.pawn_id, pt, d->r_nDeviceId, (int)ok, dist); }
			break;
		}
	}
	// Hand weapon: melee close, else the ranged rotation.
	int want = -1;
	if (dist <= c.atkMeleeRange) for (int pt : c.atkMelee) if (devAt(pt)) { want = pt; break; }
	if (want < 0 && !c.atkRanged.empty()) {
		const bool curRanged = std::find(c.atkRanged.begin(), c.atkRanged.end(), a.pt) != c.atkRanged.end();
		if (curRanged && now - a.since >= c.atkSwapS) a.idx = (a.idx + 1) % (int)c.atkRanged.size();
		for (size_t k = 0; k < c.atkRanged.size() && want < 0; ++k) {
			const size_t i = (a.idx + k) % c.atkRanged.size();
			const int pt = c.atkRanged[i];
			const float r = i < c.atkRangedRange.size() ? c.atkRangedRange[i] : c.atkRange;
			if (devAt(pt) && dist <= r) { want = pt; a.idx = (int)i; }
		}
		if (want < 0) a.outRange++;
	}
	ATgDevice* dev = devAt(want);
	if (!dev) return;
	if (want != a.pt) { a.pt = want; a.since = now; a.swTries = 0; }
	if ((ATgDevice*)bot->Weapon != dev && a.swTries < 2) {
		if (now - a.lastSw < 0.9f) return;  // the switch takes a tick
		stop();
		bool sw = bot->eventSetActiveWeapon((AWeapon*)dev, 0, 1);
		if ((ATgDevice*)bot->Weapon != dev) {
			// The pawn refuses (Weapon never changed) - put the device in hand directly,
			// same wiring as PossessPawn / SpawnBotById (in-hand device + desired in-hand point).
			bot->Weapon = (AWeapon*)dev;
			bot->m_CurrentInHandDevice = dev;
			bot->r_eDesiredInHand = (unsigned char)want;
			dev->Instigator = (APawn*)bot;
			bot->bNetDirty = 1;
			a.forced++;
			sw = (ATgDevice*)bot->Weapon == dev;
		}
		a.sw++; a.swTries++; a.lastSw = now;
		if (a.logs < 60) { ++a.logs; Logger::Log("worldgroups", "[WorldGroups] attack tag=%s pawn=%d switch pt=%d dev=%d sw=%d now=%s dist=%.0f\n",
			tag.c_str(), m.pawn_id, want, dev->r_nDeviceId, (int)sw, bot->Weapon ? bot->Weapon->GetName() : "none", dist); }
		return;
	}
	// SDK bool wrappers (IsFiring, StartAction, CanDeviceFireNow) read garbage bits (see
	// UObject__ProcessEvent ServerStartFire notes) - judge by the device's UC state name instead.
	auto stName = [](ATgDevice* d) -> std::string { const char* n = d ? d->GetStateName().GetName() : nullptr; return n ? n : "?"; };
	auto busySt = [](const std::string& n) { return n == "DeviceBuildup" || n == "DeviceFiring"; };
	const int hitsNow = s_memberHits.count((AActor*)bot) ? s_memberHits[(AActor*)bot] : 0;
	const std::string st0 = stName(dev);
	if (busySt(st0)) {  // firing: keep going while hits land; 4 s without a hit -> stop + restart
		a.busy++; a.firing = true;
		// Hit roll (see Ctl::atkHitGroups)
		size_t ri = 0;
		while (ri < c.atkRanged.size() && c.atkRanged[ri] != want) ++ri;
		const int hg = ri < c.atkHitGroups.size() ? c.atkHitGroups[ri] : 0;
		ATgPawn* tp = ObjectClassCache::ClassNameContains((UObject*)foe, "TgPawn") ? (ATgPawn*)foe : nullptr;
		if (hg > 0 && tp && tp->r_EffectManager && tp->Health > 0 && (float)std::rand() / RAND_MAX < c.atkHitChance) {
			if (UTgEffectGroup* g = BuildEffectGroup(hg, 264)) {
				FImpactInfo impact{};
				impact.HitLocation = foe->Location;
				const int hp0 = tp->Health;
				tp->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
				a.assists++;
				if (a.logs < 60 && (a.assists % 5) == 1) { ++a.logs; Logger::Log("worldgroups",
					"[WorldGroups] attack tag=%s pawn=%d hit pt=%d eg=%d foe_hp %d -> %d assists=%d dist=%.0f\n",
					tag.c_str(), m.pawn_id, want, hg, hp0, tp->Health, a.assists, dist); }
				a.lastHits = -1;  // a hit landed: not a stuck burst
			}
		}
		if (hitsNow != a.lastHits) { a.busyRun = 0; a.lastHits = hitsNow; return; }
		if (++a.busyRun < 8) return;
		a.busyRun = 0; a.kicks++; stop();
		return;
	}
	a.firing = false;
	// Every device fired exactly once, then Active->Active forever: the burst ends
	// on its own but the pending-fire flag stays set until a StopFire. Clear it before each start.
	bot->StopAction(dev, 0);
	// A mode-1 start attempt left CurrentFireMode=1 on a one-mode device,
	// so every later CanDeviceFireNow failed. Only mode 0, and reset it.
	if (dev->m_FireMode.Count > 0 && (dev->CurrentFireMode < 0 || dev->CurrentFireMode >= dev->m_FireMode.Count)) dev->CurrentFireMode = 0;
	// GetPlayerViewPoint = controller Location, which stayed at the sky-drop spawn point (z 250):
	// shots were traced from the sky. Put the controller at the pawn's eye before firing.
	{
		FVector eye = bot->Location;
		eye.Z += std::fmax(bot->BaseEyeHeight, 0.7f * (bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 60.0f));
		ai->Location = eye;
	}
	{	// Aim the controller (pitch too) at the foe's chest
		const float ch = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 60.0f;
		const float dx = foe->Location.X - bot->Location.X, dy = foe->Location.Y - bot->Location.Y;
		const float dz = foe->Location.Z - (bot->Location.Z + 0.7f * ch);
		FRotator r;
		r.Yaw = (int)(std::atan2(dy, dx) * 32768.0 / 3.14159265);
		r.Pitch = (int)(std::atan2(dz, std::sqrt(dx * dx + dy * dy)) * 32768.0 / 3.14159265);
		r.Roll = 0;
		ai->Rotation = r;
	}
	bot->StartAction(dev, 0);
	std::string st1 = stName(dev);
	int path = 1;
	if (!busySt(st1)) { dev->ServerStartFire(ObjectClassCache::ClassNameContains((UObject*)foe, "TgPawn") ? (ATgPawn*)foe : nullptr, 0); st1 = stName(dev); path = 3; }
	if (!busySt(st1)) { ai->UseDevice((unsigned char)want, 0, 0, 0); st1 = stName(dev); path = 4; }
	const bool ok = busySt(st1);
	if (ok) { a.starts++; a.firing = true; if (path >= 3) a.useDev++; } else {
		a.fails++;
		if ((a.fails % 10) == 1) {
			char buf[200];
			std::snprintf(buf, sizeof(buf), "dev=%d inhand=%d st=%s->%s disableAll=%d disableAct=%d offCd=%d phys=%d pwr=%.0f",
				dev->r_nDeviceId, bot->Weapon ? ((ATgDevice*)bot->Weapon)->r_nDeviceId : -1, st0.c_str(), st1.c_str(),
				(int)bot->r_bDisableAllDevices, (int)bot->c_bDisableAction, (int)bot->r_bInGlobalOffhandCooldown,
				(int)bot->Physics, bot->r_fCurrentPowerPool);
			a.why = buf;
			a.refuse++;
			if (a.logs < 60) { ++a.logs; Logger::Log("worldgroups", "[WorldGroups] attack tag=%s pawn=%d refused %s dist=%.0f\n", tag.c_str(), m.pawn_id, buf, dist); }
		}
	}
	if (a.logs < 60 && ok && (a.starts % 5) == 1) { ++a.logs; Logger::Log("worldgroups",
		"[WorldGroups] attack tag=%s pawn=%d fire pt=%d dev=%d path=%d st=%s->%s starts=%d fails=%d hits=%d dist=%.0f foe_hp=%d\n",
		tag.c_str(), m.pawn_id, want, dev->r_nDeviceId, path, st0.c_str(), st1.c_str(), a.starts, a.fails, hitsNow, dist, foe->Health); }
}

// Dome follow + engage drive (every 0.5 s).
void DriveTick(AWorldInfo* wi, const std::string& tag, Ctl& c, std::vector<Member>& members, float now) {
	if (c.domeId <= 0 && !c.engage && c.dome == nullptr) return;
	const int gtf = s_groupTf.count(tag) ? s_groupTf[tag] : 1;
	// Dome: up while protected and the BOSS (lead member) lives - never on an escort; follows
	// the boss every engine tick it moves (> 4 uu). Boss dead / shield down / despawn -> off.
	ATgPawn* domeBoss = c.domeId > 0 ? DomeBoss(wi, members) : nullptr;
	if (c.domeId > 0 && c.protect && domeBoss) {
		if (!DomeAlive(c)) DomeOn(c, tag, domeBoss, gtf);
		else {
			const FVector at = DomeSpot(domeBoss, c);
			const FVector& d = c.dome->Location;
			if ((d.X - at.X) * (d.X - at.X) + (d.Y - at.Y) * (d.Y - at.Y) + (d.Z - at.Z) * (d.Z - at.Z) > 4.0f * 4.0f) {
				((AActor*)c.dome)->SetLocation(at);
				c.dome->bNetDirty = 1;
				c.dome->bForceNetUpdate = 1;  // push the move this tick
				const FVector& n = c.dome->Location;
				const bool moved = (n.X - at.X) * (n.X - at.X) + (n.Y - at.Y) * (n.Y - at.Y) < 2.0f * 2.0f;
				if (moved) { if (++c.domeMoves % 200 == 1) Logger::Log("worldgroups", "[WorldGroups] dome follow tag=%s moves=%d at (%.0f,%.0f,%.0f)\n", tag.c_str(), c.domeMoves, n.X, n.Y, n.Z); }
				else if (c.domeMoveFails++ < 5) Logger::Log("worldgroups", "[WorldGroups] dome follow tag=%s SetLocation refused (want %.0f,%.0f have %.0f,%.0f)\n", tag.c_str(), at.X, at.Y, n.X, n.Y);
			}
		}
	} else if (c.dome != nullptr) {
		DomeOff(c, tag);
	}
	if (!c.engage || now - c.driveLast < 0.5f) return;
	c.driveLast = now;
	// BossMove: every step is checked for walkable ground (cliffs, map edges,
	// kill-Z, walls, steep rock) and walks around obstacles; no progress -> other side, then the
	// target counts as unreachable (chase) or the member is put home (walk home). Targets it
	// cannot reach are skipped; with "evade", members hit only by such targets take no damage
	// and walk home. The chase never leaves the limit circle (it fights from the edge).
	const auto costT0 = std::chrono::steady_clock::now();
	// Last attacker still a live hostile pawn?
	APawn* attacker = nullptr;
	if (c.lastAttacker && now - c.lastAttackAt < 20.0f) {
		for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn)
			if ((AActor*)p == c.lastAttacker) { if (!p->bDeleteMe && p->Health > 0 && HostileTo(p, gtf)) attacker = p; break; }
		if (!attacker) c.lastAttacker = nullptr;
	}
	const bool bots = BossMove::TargetsBots(tag);
	// Only players are fought (friendly NPCs/turrets that shoot it - vendors, quest givers,
	// town guards - are ignored, so a boss is never pulled into killing them).
	if (attacker && !bots && !IsPlayerPawn(attacker)) attacker = nullptr;
	const bool attackerUnreach = attacker && BossMove::IsUnreachable(tag, attacker, now);
	// Still shooting from where it can't be reached: keep it marked.
	if (attackerUnreach && now - c.lastAttackAt < 2.0f) BossMove::MarkUnreachable(tag, attacker, now, 30.0f);
	auto reachable = [&](APawn* p) { return !BossMove::IsUnreachable(tag, p, now); };
	// Range rule: attack-drive members shoot a foe inside their weapon range with a clear shot,
	// reachable or not (rock, ledge): stand, face, fire - no "unreachable -> home", no evade.
	auto shootable = [&](const Member& mm, APawn* p) {
		if (!p || c.atkBots.empty() || std::find(c.atkBots.begin(), c.atkBots.end(), mm.bot_id) == c.atkBots.end()) return false;
		const float dx = p->Location.X - mm.pawn->Location.X, dy = p->Location.Y - mm.pawn->Location.Y, dz = p->Location.Z - mm.pawn->Location.Z;
		return dx * dx + dy * dy + dz * dz <= c.atkRange * c.atkRange && ClearShot(mm.pawn, p);
	};
	bool anyFoe = false;
	static const char* names[] = {"idle", "chase", "fight", "home"};
	for (const Member& m : members) {
		if (!IsLive(wi, m.pawn) || !m.pawn->Controller) continue;
		ATgAIController* ai = (ATgAIController*)m.pawn->Controller;
		HoldTargetVo(m.pawn, now);
		s_fireAi[ai];  // FireSight fallback + diagnostics for engaged members
		int mode = 0;
		APawn* foe = nullptr;
		float dist = 0.0f;
		if (!c.passive) {
			auto d2 = [&](APawn* p) {
				const float dx = p->Location.X - m.pawn->Location.X, dy = p->Location.Y - m.pawn->Location.Y;
				return dx * dx + dy * dy;
			};
			const float aggro2 = c.engageAggro * c.engageAggro;
			if (attacker && !attackerUnreach && d2(attacker) < 4.0f * aggro2) foe = attacker;
			if (!foe && attacker && attackerUnreach && shootable(m, attacker)) foe = attacker;
			// Keep the current enemy while it is a live, reachable hostile inside 1.5 x aggro.
			if (!foe && ai->Enemy && !ai->Enemy->bDeleteMe && ai->Enemy->Health > 0 && HostileTo(ai->Enemy, gtf)
				&& (bots || IsPlayerPawn(ai->Enemy))
				&& d2(ai->Enemy) < 2.25f * aggro2 && (reachable(ai->Enemy) || shootable(m, ai->Enemy))) foe = ai->Enemy;
			if (!foe) {
				float best = aggro2;
				for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn) {
					const bool target = IsPlayerPawn(p) || (bots && p->Controller && !p->bDeleteMe && p->Health > 0);
					if (!target || !HostileTo(p, gtf)) continue;
					if (!reachable(p) && !shootable(m, p)) continue;  // unreachable but in range + sight = shoot it
					if (std::fabs(p->Location.Z - m.pawn->Location.Z) > 2500.0f) continue;
					const float q = d2(p);
					if (q < best) { best = q; foe = p; }
				}
			}
			if (foe) dist = std::sqrt(d2(foe));
		}
		if (foe) {
			if (ai->Enemy != foe) {
				ai->eventSeePlayer(foe);
				ai->SetTargetActor((AActor*)foe);
				if (ai->Enemy != foe) ai->Enemy = foe;
				ai->m_bInterrupt = 1;
				BarkTargetVo(m.pawn, m.bot_id, now);
			}
			{  // hysteresis band so a foe near the hold distance does not toggle run/stand every tick
				const float band = std::fmax(80.0f, 0.15f * c.engageHold);
				const int prev = c.driveMode[m.pawn];
				mode = dist > c.engageHold + band ? 1 : dist <= c.engageHold ? 2 : (prev == 1 ? 1 : 2);
			}
			// Shoot an unreachable foe from here when it is in range + sight.
			const bool canShoot = shootable(m, foe);
			if (c.lastFoe[m.pawn] != foe) { c.lastFoe[m.pawn] = foe; c.lastShotChance[m.pawn] = now; }
			if (canShoot) c.lastShotChance[m.pawn] = now;
			if (canShoot && BossMove::IsUnreachable(tag, foe, now)) { mode = 2; c.atk[m.pawn].shootUnreach++; }
			// No shot at this foe for atkNoShotHomeS (kited, hiding without LOS): drop it, walk home.
			if (!c.atkBots.empty() && now - c.lastShotChance[m.pawn] > c.atkNoShotHomeS
				&& std::find(c.atkBots.begin(), c.atkBots.end(), m.bot_id) != c.atkBots.end()) {
				Logger::Log("worldgroups", "[WorldGroups] engage tag=%s pawn=%d no shot at target for %.0f s (dist %.0f): drop it, walk home\n",
					tag.c_str(), m.pawn_id, now - c.lastShotChance[m.pawn], dist);
				c.atk[m.pawn].noShotHome++;
				BossMove::MarkUnreachable(tag, foe, now, 30.0f);
				ClearTarget(m.pawn);
				foe = nullptr;
				c.lastFoe[m.pawn] = nullptr;
				const float hx = m.pawn->Location.X - m.home.X, hy = m.pawn->Location.Y - m.home.Y;
				mode = hx * hx + hy * hy <= c.homeRadius * c.homeRadius ? 0 : 3;
			}
		}
		if (foe) {
			const bool canShoot = shootable(m, foe);
			// Attack-drive member without a clear shot for 3 s closes in instead of waiting.
			if (mode == 2 && dist > 350.0f) {
				auto ak = c.atk.find(m.pawn);
				if (ak != c.atk.end() && ak->second.noSightRun >= 6) mode = 1;
			}
			// Close in 2D but far above (rock top, ledge, roof it can't climb): unreachable.
			if (mode == 2 && foe->Location.Z - m.pawn->Location.Z > 900.0f && !canShoot) {
				BossMove::MarkUnreachable(tag, foe, now, 30.0f);
				Logger::Log("worldgroups", "[WorldGroups] engage tag=%s pawn=%d target %.0f uu above at 2D %.0f: unreachable 30 s\n",
					tag.c_str(), m.pawn_id, foe->Location.Z - m.pawn->Location.Z, dist);
				ClearTarget(m.pawn);
				foe = nullptr;
				const float hx = m.pawn->Location.X - m.home.X, hy = m.pawn->Location.Y - m.home.Y;
				mode = hx * hx + hy * hy <= c.homeRadius * c.homeRadius ? 0 : 3;
			}
		} else {
			if (ai->Enemy) ClearTarget(m.pawn);
			const float hx = m.pawn->Location.X - m.home.X, hy = m.pawn->Location.Y - m.home.Y;
			mode = hx * hx + hy * hy <= c.homeRadius * c.homeRadius ? 0 : 3;
		}
		if (foe) anyFoe = true;
		// Committed to a telegraphed move (windup/swing): stand still, keep the facing it started with.
		bool committed = false;
		if (foe) {
			auto ts = s_moveSt.find(tag);
			if (ts != s_moveSt.end()) {
				auto ps = ts->second.find(m.pawn);
				const std::vector<BossMoveDef>& mvs = s_moves[tag];
				if (ps != ts->second.end() && ps->second.cur >= 0 && ps->second.cur < (int)mvs.size()
					&& mvs[ps->second.cur].kind != kMvGun && now < ps->second.busyUntil) committed = true;
			}
			if (committed && mode == 1) mode = 2;
		}
		if (committed && m.pawn->Velocity.X * m.pawn->Velocity.X + m.pawn->Velocity.Y * m.pawn->Velocity.Y > 40.0f * 40.0f) {
			// Still running into a windup (latent MoveTo outlives m_bInterrupt) - stop it here
			s_runTo.erase(ai); ai->StopLatentExecution();
			m.pawn->Velocity.X = 0.0f; m.pawn->Velocity.Y = 0.0f; m.pawn->Acceleration.X = 0.0f; m.pawn->Acceleration.Y = 0.0f;
			if (c.commitStops++ < 20) Logger::Log("worldgroups", "[WorldGroups] commit stop tag=%s pawn=%d state=%s\n",
				tag.c_str(), m.pawn_id, std::string(ai->GetStateName().GetName()).c_str());
		}
		int& was = c.driveMode[m.pawn];
		bool running = false;
		{  // no leftover device speed buff: clamp to base x speed_scale outside a committed move
			auto bsp = c.baseSpeed.find(m.pawn);
			if (bsp == c.baseSpeed.end() && m.pawn->GroundSpeed > 1.0f && !committed) bsp = c.baseSpeed.emplace(m.pawn, m.pawn->GroundSpeed).first;
			if (bsp != c.baseSpeed.end() && !committed) {
				const float want = bsp->second * c.speedK;
				if (m.pawn->GroundSpeed > want * 1.1f) {
					if (c.speedClamps++ < 20) Logger::Log("worldgroups", "[WorldGroups] speed clamp tag=%s pawn=%d %.0f -> %.0f\n",
						tag.c_str(), m.pawn_id, m.pawn->GroundSpeed, want);
					m.pawn->SetProperty(GA_PROPERTY::TGPID_GROUND_SPEED, want);
					m.pawn->GroundSpeed = want;
					m.pawn->bNetDirty = 1;
				}
			}
		}
		if (mode == 1 || mode == 3) {
			FVector to;
			const BossMove::Step s = BossMove::Steer(tag, m.pawn, mode == 1 ? foe->Location : m.home, mode == 1, now, &to);
			if (s == BossMove::Step::Blocked && shootable(m, foe)) {
				mode = 2;  // cannot walk there but can shoot it: stand and fire
			} else if (s == BossMove::Step::Blocked) {
				BossMove::MarkUnreachable(tag, foe, now, 30.0f);
				Logger::Log("worldgroups", "[WorldGroups] engage tag=%s pawn=%d cannot reach target at (%.0f,%.0f,%.0f) from (%.0f,%.0f,%.0f): skipped 30 s\n",
					tag.c_str(), m.pawn_id, foe->Location.X, foe->Location.Y, foe->Location.Z,
					m.pawn->Location.X, m.pawn->Location.Y, m.pawn->Location.Z);
				ClearTarget(m.pawn);
				mode = 0;
			} else if (s == BossMove::Step::AtLimit) {
				mode = 2;  // at the edge of its area: stand and fight from here
			} else if (s == BossMove::Step::Teleport) {
				FVector home = m.home; home.Z += 30.0f;
				const bool ok = m.pawn->SetLocation(home);
				Logger::Log("worldgroups", "[WorldGroups] engage tag=%s pawn=%d stuck walking home at (%.0f,%.0f,%.0f): moved home ok=%d\n",
					tag.c_str(), m.pawn_id, m.pawn->Location.X, m.pawn->Location.Y, m.pawn->Location.Z, (int)ok);
				mode = 0;
			} else if (s != BossMove::Step::Hold) {
				s_runTo[ai] = to;
				if (m.pawn->bIsWalking) m.pawn->bIsWalking = 0;
				ai->eventSetWhatToDoNext(238, 289);
				running = true;
			}
		}
		bool backing = false;
		if (mode == 2 && foe && c.atkMinRange > 0.0f && dist < c.atkMinRange
			&& std::find(c.atkBots.begin(), c.atkBots.end(), m.bot_id) != c.atkBots.end()) {
			// Too close for its guns (every start refused at ~155 uu) - step back.
			const float bx = m.pawn->Location.X - foe->Location.X, by = m.pawn->Location.Y - foe->Location.Y;
			const float bl = std::fmax(1.0f, std::sqrt(bx * bx + by * by));
			FVector away = m.pawn->Location;
			away.X += bx / bl * (c.engageHold - dist); away.Y += by / bl * (c.engageHold - dist);
			FVector to;
			const BossMove::Step s = BossMove::Steer(tag, m.pawn, away, true, now, &to);
			if (s == BossMove::Step::Go || s == BossMove::Step::Detour) {
				s_runTo[ai] = to;
				if (m.pawn->bIsWalking) m.pawn->bIsWalking = 0;
				ai->eventSetWhatToDoNext(238, 289);
				running = backing = true;
				c.atk[m.pawn].backoffs++;
			}
		}
		if (mode == 2 && foe && !backing && !committed) BossMove::Face(m.pawn, foe);
		AttackTick(tag, c, m, ai, foe, dist, now);
		if (!running && s_runTo.count(ai)) {
			s_runTo.erase(ai);
			ai->m_bInterrupt = 1;
		}
		if (mode != was && c.driveLogs < 400) {
			++c.driveLogs;
			Logger::Log("worldgroups", "[WorldGroups] engage tag=%s pawn=%d %s -> %s dist=%.0f at (%.0f,%.0f,%.0f) spd=%.0f gs=%.0f state=%s\n",
				tag.c_str(), m.pawn_id, names[was & 3], names[mode & 3], dist, m.pawn->Location.X, m.pawn->Location.Y,
				m.pawn->Location.Z, std::sqrt(m.pawn->Velocity.X * m.pawn->Velocity.X + m.pawn->Velocity.Y * m.pawn->Velocity.Y),
				m.pawn->GroundSpeed, std::string(ai->GetStateName().GetName()).c_str());
		}
		was = mode;
	}
	// Evade: hit only by targets it cannot reach (rock, ledge, outside its area) -> no damage
	// (shipped Invulnerable payload, *Immune*), walks home; ends once nobody unreachable hit
	// it for 8 s or a reachable target is engaged. The stage shield (protect) owns the
	// payload while it is up.
	if (BossMove::EvadeEnabled(tag)) {
		const bool want = attackerUnreach && !anyFoe && now - c.lastAttackAt < 8.0f;
		const bool was = BossMove::Evading(tag);
		if (want != was) BossMove::SetEvading(tag, want, now);
		if (!c.protect) {
			for (const Member& m : members) {
				if (!IsLive(wi, m.pawn)) continue;
				const bool has = HasGroup(m.pawn, kProtectEffectGroupId);
				if (want && !has) SetProtect(m.pawn, true, 0);
				else if (!want && was && has) {
					c.keepHp[m.pawn] = m.pawn->Health;  // removing the payload may refill HP
					c.keepUntil = now + 5.0f;
					SetProtect(m.pawn, false, 0);
				}
			}
		}
	}
	BossMove::AddCost(tag, std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - costT0).count());
}
} // namespace

bool FireSight(ATgAIController* aic, bool origOk) {
	if (s_fireAi.empty() || !aic) return false;
	auto it = s_fireAi.find(aic);
	if (it == s_fireAi.end()) return false;
	FireDiag& d = it->second;
	d.calls++;
	if (origOk) { d.origOk++; return false; }
	APawn* bot = aic->Pawn;
	APawn* foe = aic->Enemy;
	if (!bot || !foe || bot->bDeleteMe || foe->bDeleteMe || bot->Health <= 0 || foe->Health <= 0) return false;
	const float dx = bot->Location.X - foe->Location.X, dy = bot->Location.Y - foe->Location.Y, dz = bot->Location.Z - foe->Location.Z;
	if (dx * dx + dy * dy + dz * dz > 5000.0f * 5000.0f) return false;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return false;
	const float ch = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 60.0f;
	FVector eye = bot->Location; eye.Z += std::fmax(bot->BaseEyeHeight, 0.7f * ch);
	auto clear = [&](FVector to) {
		FVector hitLoc, hitNorm;
		FTraceHitInfo hitInfo;
		std::memset(&hitInfo, 0, sizeof(hitInfo));
		FVector start = eye;
		return !Game->Trace(to, start, 0, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo);
	};
	FVector head = foe->Location; head.Z += foe->BaseEyeHeight;
	const bool ok = clear(head) || clear(foe->Location);
	if (ok) d.ours++; else d.blocked++;
	if (ok && (d.ours % 40) == 1)
		Logger::Log("worldgroups", "[WorldGroups] fire sight ok (native said no) dist=%.0f dz=%.0f n=%d native_ok=%d\n",
			std::sqrt(dx * dx + dy * dy + dz * dz), dz, d.ours, d.origOk);
	return ok;
}

void OverrideMovement(ATgAIController* aic, int* nMovementCode, int* nMoveDestination) {
	if (s_runTo.empty() || !aic || !nMovementCode || !nMoveDestination) return;
	auto it = s_runTo.find(aic);
	if (it == s_runTo.end()) return;
	ATgPawn* p = (ATgPawn*)aic->Pawn;
	if (!p || p->bDeleteMe || p->Health <= 0) { s_runTo.erase(it); return; }
	aic->m_vTriggerLocation = it->second;
	*nMovementCode = 238;
	*nMoveDestination = 289;
}

bool BarValue(ATgRepInfo_Player* pri, ATgPawn* viewer, int* value) {
	if (s_bars.empty() || pri == nullptr) return false;
	auto b = s_bars.find(pri);
	if (b == s_bars.end()) return false;
	int v = (int)(0xA0000000u | (unsigned)(b->second.marker & 0xffff));  // a world boss, this viewer not fighting it
	if (viewer && ObjectClassCache::ClassNameContains((UObject*)viewer, "TgPawn_Character")) {
		const int64_t cid = (int64_t)((ATgPawn_Character*)viewer)->s_nCharacterId;
		for (int64_t x : b->second.viewers) if (x == cid) { v = b->second.marker; break; }
	}
	*value = v;
	return true;
}

void Tick() {
	TickChests();
	if (AWorldInfo* bw = CurrentWorldInfo()) BarTick(bw);
	if (s_ctl.empty()) return;
	AWorldInfo* wi = CurrentWorldInfo();
	if (!wi) return;
	const float now = wi->TimeSeconds;
	for (auto& [tag, c] : s_ctl) {
		auto it = s_groups.find(tag);
		if (it == s_groups.end()) continue;
		if (c.attacker != nullptr) {
			// Interrupted: every live member targets the attacker (if it is still a live pawn).
			AActor* a = c.attacker;
			c.attacker = nullptr;
			bool aLive = false;
			for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn)
				if ((AActor*)p == a) { aLive = !p->bDeleteMe && p->Health > 0; break; }
			if (aLive) {
				for (const Member& m : it->second) {
					if (!IsLive(wi, m.pawn) || !m.pawn->Controller) continue;
					ATgAIController* ai = (ATgAIController*)m.pawn->Controller;
					ai->Enemy = (APawn*)a;
					ai->SetTargetActor(a);
				}
			}
			Logger::Log("worldgroups", "[WorldGroups] tag=%s passive return interrupted (#%d) attacker_live=%d\n",
				tag.c_str(), c.interrupts, (int)aLive);
			ReportStatus(tag);
		}
		DriveTick(wi, tag, c, it->second, now);
		if (!c.nativeRegen) {
			for (const Member& m : it->second) {
				if (!IsLive(wi, m.pawn)) continue;
				ATgAIController* ai = m.pawn->Controller ? (ATgAIController*)m.pawn->Controller : nullptr;
				if (ai) ai->m_fLastRegenTime = now;
				const int h = m.pawn->Health;
				auto ow = s_ourHp.find(m.pawn);
				if (ow != s_ourHp.end()) {
					if (now - ow->second.second < 1.0f) c.hpBase[m.pawn] = ow->second.first;
					s_ourHp.erase(ow);
				}
				auto bs = c.hpBase.find(m.pawn);
				if (bs == c.hpBase.end()) { c.hpBase[m.pawn] = h; continue; }
				if (h > bs->second) {
					float& allow = c.healAllow[m.pawn];
					if (c.healCap > 0.0f) {  // heals may add at most healCap x max HP per s
						auto ht = c.healCapAt.find(m.pawn);
						const float dt = ht == c.healCapAt.end() ? 0.25f : std::fmin(1.0f, std::fmax(0.0f, now - ht->second));
						c.healCapAt[m.pawn] = now;
						const float budget = c.healCap * (float)m.pawn->r_nHealthMaximum * dt;
						if (allow > budget) { allow = budget; c.healCapped++; }
					}
					const int ok = bs->second + (int)allow;
					if (h > ok) {
						static int lastFrom = -1, lastTo = -1;
						if (c.riseLogs < 25 && (bs->second != lastFrom || h != lastTo)) {
							++c.riseLogs; lastFrom = bs->second; lastTo = h;
							Logger::Log("worldgroups", "[WorldGroups] hp rise reverted tag=%s %d -> %d (max %d, heal allow %.0f) in_combat=%d protect=%d t=%.1f\n",
								tag.c_str(), bs->second, h, m.pawn->r_nHealthMaximum, allow, (int)m.pawn->r_bInCombatFlag, (int)c.protect, now);
						}
						SetHp(m.pawn, ok, m.pawn->r_nHealthMaximum);
						s_ourHp.erase(m.pawn);
						allow = 0.0f;
						bs->second = ok;
					} else {
						allow = std::fmax(0.0f, allow - (float)(h - bs->second));
						bs->second = h;
					}
				} else {
					bs->second = h;
				}
			}
		}
		// Every frame (a heal between 4 Hz ticks must not show): while protected, outside heals
		// (e.g. a healer minion) can't lift a member above max(HP when protect went on, regen
		// cap); just after protect went off, no member climbs above the HP it had then.
		if (c.protect || now < c.keepUntil) {
			for (const Member& m : it->second) {
				if (!IsLive(wi, m.pawn)) continue;
				int lim = -1;
				if (c.protect) {
					auto ph = c.protectHp.find(m.pawn);
					if (ph != c.protectHp.end())
						lim = std::max(1, (int)std::floor(std::max(ph->second, c.regenCap) * (float)m.pawn->r_nHealthMaximum));
				} else {
					auto kh = c.keepHp.find(m.pawn);
					if (kh != c.keepHp.end()) lim = kh->second;
				}
				if (lim > 0 && m.pawn->Health > lim) SetHp(m.pawn, lim, m.pawn->r_nHealthMaximum);
			}
		}
		if (now - c.last < 0.25f) continue;
		const float dt = c.last > 0.0f ? std::fmin(1.0f, now - c.last) : 0.25f;
		c.last = now;
		if (!c.passive) {
			// Fighting regen (e.g. while protected): regen_frac_s of max HP per s up to regen_cap_frac.
			if (c.regen <= 0.0f || c.regenCap <= 0.0f) continue;
			for (const Member& m : it->second) {
				if (!IsLive(wi, m.pawn)) continue;
				const int cap = (int)std::floor(c.regenCap * (float)m.pawn->r_nHealthMaximum);
				if (m.pawn->Health >= cap) continue;
				float& carry = c.hpCarry[m.pawn];
				carry += c.regen * (float)m.pawn->r_nHealthMaximum * dt;
				const int add = (int)carry;
				if (add > 0) {
					carry -= (float)add;
					SetHp(m.pawn, std::min(cap, m.pawn->Health + add), m.pawn->r_nHealthMaximum);
				}
			}
			continue;
		}
		bool allHome = true, any = false;
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			any = true;
			ClearTarget(m.pawn);
			if (c.regen > 0.0f && m.pawn->Health < m.pawn->r_nHealthMaximum) {
				float& carry = c.hpCarry[m.pawn];
				carry += c.regen * (float)m.pawn->r_nHealthMaximum * dt;
				const int add = (int)carry;
				if (add > 0) {
					carry -= (float)add;
					SetHp(m.pawn, std::min(m.pawn->r_nHealthMaximum, m.pawn->Health + add), m.pawn->r_nHealthMaximum);
				}
			}
			if (!AtHome(m.pawn, c.homeRadius)) allHome = false;
		}
		if (!any) { c.passive = false; continue; }
		if (!allHome && now - c.startedAt > c.maxReturnS) {
			// Stuck / no path home: put the members on their spawn spots.
			int moved = 0;
			for (const Member& m : it->second) {
				if (!IsLive(wi, m.pawn) || AtHome(m.pawn, c.homeRadius)) continue;
				ATgAIController* ai = (ATgAIController*)m.pawn->Controller;
				// The ground spot it was placed on (m_vSpawnLocation is the drop point, up in the air).
				FVector to = (m.home.X != 0.0f || m.home.Y != 0.0f || m.home.Z != 0.0f) ? m.home : ai->m_vSpawnLocation;
				to.Z += 50.0f;
				if (m.pawn->SetLocation(to)) ++moved;
			}
			Logger::Log("worldgroups", "[WorldGroups] tag=%s not home after %.0f s: moved %d members to their spawn spots\n",
				tag.c_str(), c.maxReturnS, moved);
			c.startedAt = now;  // one move per max_return_s
			continue;
		}
		if (!allHome) { c.homeAt = -1.0f; continue; }
		if (c.homeAt < 0.0f) c.homeAt = now;
		if (now - c.homeAt < c.settleS) continue;
		for (const Member& m : it->second) {
			if (!IsLive(wi, m.pawn)) continue;
			SetHp(m.pawn, m.pawn->r_nHealthMaximum, m.pawn->r_nHealthMaximum);
		}
		c.passive = false;
		c.settled++;
		Logger::Log("worldgroups", "[WorldGroups] tag=%s home and settled: full HP\n", tag.c_str());
		ReportStatus(tag);
	}
}

namespace {

// Asset paths come from the admin payload: package.group.name, ASCII only.
bool SafeAssetPath(const std::string& s) {
	if (s.empty() || s.size() > 128) return false;
	for (char ch : s) {
		if (!(std::isalnum((unsigned char)ch) || ch == '_' || ch == '.')) return false;
	}
	return true;
}

UObject* LoadAsset(const char* className, const std::string& path) {
	static std::map<std::string, UObject*> cache;   // nullptr = tried, failed
	auto it = cache.find(path);
	if (it != cache.end()) return it->second;
	// Already resident (e.g. a weapon/pawn effect the map uses) -> no load.
	const std::string shortClass = std::string(className).substr(std::string(className).rfind('.') + 1);
	UObject* obj = ObjectCache::Find((shortClass + " " + path).c_str());
	UClass* cls = obj ? nullptr : ClassPreloader::GetClass(className);
	if (!obj && cls) {
		std::wstring w(path.begin(), path.end());
		obj = (UObject*)Core__LoadObject::CallOriginal(cls, nullptr, const_cast<wchar_t*>(w.c_str()),
			nullptr, 0, nullptr, 0);
	}
	Logger::Log("worldgroups", "[WorldGroups] fx load %s '%s' -> %p (class %p)\n", className, path.c_str(), (void*)obj, (void*)cls);
	cache[path] = obj;
	return obj;
}

float Dist2D(const FVector& a, const FVector& b) {
	const float dx = a.X - b.X, dy = a.Y - b.Y;
	return std::sqrt(dx * dx + dy * dy);
}

} // namespace

void PlayFx(const nlohmann::json& j) {
	AWorldInfo* wi = CurrentWorldInfo();
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!wi || !Game) return;
	FVector loc;
	loc.X = j.value("x", 0.0f); loc.Y = j.value("y", 0.0f); loc.Z = j.value("z", 0.0f);
	const float radius = std::fmax(500.0f, std::fmin(20000.0f, j.value("radius", 6000.0f)));

	// Sound source: first live member of source_tag (already relevant on the
	// clients, so the cue plays positioned at it); else the listener's pawn.
	AActor* source = nullptr;
	auto g = s_groups.find(j.value("source_tag", std::string()));
	if (g != s_groups.end()) {
		for (const Member& m : g->second) {
			if (IsLive(wi, m.pawn)) { source = (AActor*)m.pawn; break; }
		}
	}

	// Effect groups: one-shot entries in the source member's own effect queue.
	// Each client resolves the group's FX (particles/sounds) from its own data
	// and plays it on that pawn, so the server needs no particle assets.
	int queued = 0;
	ATgEffectManager* fxMgr = source ? ((ATgPawn*)source)->r_EffectManager : nullptr;
	if (fxMgr && j.contains("effect_groups") && j["effect_groups"].is_array()) {
		for (const auto& v : j["effect_groups"]) {
			if (queued >= 3 || !v.is_number_integer()) continue;
			const int eg = v.get<int>();
			if (eg <= 0 || eg > 1000000) continue;
			TgEffectManager__SetEffectRep::Call(fxMgr, nullptr, eg, 0.0f, 0, 0);
			queued++;
		}
		((AActor*)fxMgr)->bNetDirty = 1;
		((AActor*)fxMgr)->bForceNetUpdate = 1;
	}

	// Device fire FX: one entry in the source member's flash queue (TgPawn
	// r_nFlashEvent ring, already replicated), type 0x20000 = fire. Each client
	// runs the equipped device form's Fire event at the spot (its own anim, impact
	// particles and sounds); nothing is fired on the server, so no damage.
	// Event = seq << 24 | 0x20000 | mode << 8 | equip slot; FireInfo = socket << 16
	// | equip slot << 8 | 1 (successful hit -> impact FX).
	int fired = 0;
	if (source && j.contains("fire_fx") && j["fire_fx"].is_object()) {
		ATgPawn* P = (ATgPawn*)source;
		const int devId = j["fire_fx"].value("device_id", 0);
		const int mode = std::max(0, std::min(7, j["fire_fx"].value("mode", 0)));
		int slot = -1;
		for (int i = 0; i < 0x19 && devId > 0; i++) {
			if (P->r_EquipDeviceInfo[i].nDeviceId == devId) { slot = i; break; }
		}
		if (slot >= 0) {
			const int q = (P->r_nFlashQueIndex >= 0 && P->r_nFlashQueIndex < 0x20) ? P->r_nFlashQueIndex : 0;
			unsigned char seq = P->m_byFlashNextUpValue;
			if (seq == 0 || seq > 0x7f) seq = 1;
			P->r_vFlashLocation[q] = loc;
			P->r_vFlashRayDir[q] = FVector{0.0f, 0.0f, -1.0f};
			P->r_vFlashRefireTime[q] = 0.0f;
			P->r_vFlashSituationalAttack[q] = 0;
			P->r_nFlashFireInfo[q] = (slot << 8) | 1;
			P->r_nFlashEvent[q] = ((int)seq << 24) | 0x20000 | (mode << 8) | slot;
			P->r_nFlashQueIndex = (q + 1) % 0x20;
			P->m_byFlashNextUpValue = (unsigned char)(seq >= 0x7f ? 1 : seq + 1);
			P->bNetDirty = 1;
			P->bForceNetUpdate = 1;
			fired = 1;
		} else {
			Logger::Log("worldgroups", "[WorldGroups] fx fire_fx device %d not equipped on source\n", devId);
		}
	}

	// Emitter: replicated EmitterSpawnable at the spot, removed after lifespan.
	int emitted = 0;
	const std::string emitter = j.value("emitter", std::string());
	if (SafeAssetPath(emitter)) {
		UObject* ps = LoadAsset("Class Engine.ParticleSystem", emitter);
		UClass* ec = ClassPreloader::GetClass("Class Engine.EmitterSpawnable");
		if (ps && !ec) Logger::Log("worldgroups", "[WorldGroups] fx EmitterSpawnable class not resident\n");
		if (ps && ec) {
			FRotator rot = {0, 0, 0};
			AEmitterSpawnable* e = (AEmitterSpawnable*)((AActor*)Game)->Spawn(ec, nullptr, FName(), loc, rot, nullptr, 1);
			if (e) {
				e->ParticleTemplate = (UParticleSystem*)ps;
				e->bCurrentlyActive = 1;
				e->LifeSpan = std::fmax(1.0f, std::fmin(30.0f, j.value("emitter_life", 6.0f)));
				emitted = 1;
			}
		}
	}

	std::vector<UObject*> cues;
	if (j.contains("sounds") && j["sounds"].is_array()) {
		for (const auto& sv : j["sounds"]) {
			if (cues.size() >= 3 || !sv.is_string() || !SafeAssetPath(sv.get<std::string>())) continue;
			if (UObject* c = LoadAsset("Class Engine.SoundCue", sv.get<std::string>())) cues.push_back(c);
		}
	}
	static UFunction* fnSound = (UFunction*)ObjectCache::Find("Function Engine.PlayerController.Kismet_ClientPlaySound");

	// Optional camera shake rings: shake_rings = up to 4 ascending 2D distances.
	// A player inside ring i gets reliable client call i (ClientAthena, ClientElectra,
	// ClientApollo, ClientHades) with bOn=false. Stock clients: a no-op (those calls
	// only set a client-side cheat flag back to its default false). Clients with the
	// launcher patch run TgPlayerController.TestCameraShake there instead
	// (strongest shake for ring 0). Absent/empty = no shake.
	static const char* kRingFns[4] = {
		"Function TgGame.TgPlayerController.ClientAthena", "Function TgGame.TgPlayerController.ClientElectra",
		"Function TgGame.TgPlayerController.ClientApollo", "Function TgGame.TgPlayerController.ClientHades" };
	float rings[4]; int nRings = 0;
	if (j.contains("shake_rings") && j["shake_rings"].is_array()) {
		for (const auto& rv : j["shake_rings"]) {
			if (nRings >= 4 || !rv.is_number()) break;
			const float r = std::fmax(0.0f, std::fmin(20000.0f, rv.get<float>()));
			if (nRings > 0 && r < rings[nRings - 1]) break;
			rings[nRings++] = r;
		}
	}
	int shaken = 0;

	int heard = 0;
	for (AController* c = wi->ControllerList; c; c = c->NextController) {
		if (!ObjectClassCache::ClassNameContains(c, "PlayerController") || !c->Pawn) continue;
		const float d = Dist2D(c->Pawn->Location, loc);
		for (int r = 0; r < nRings; r++) {
			if (d > rings[r]) continue;
			if (UFunction* fn = (UFunction*)ObjectCache::Find(kRingFns[r])) {
				uint32_t ringParms[2] = { 0, 0 };   // bOn = false (+ the TgPawn local)
				c->ProcessEvent(fn, ringParms, nullptr);
				shaken++;
			}
			break;
		}
		if (d > radius) continue;
		if (fnSound && !cues.empty()) {
			// Same hand-rolled layout as BotVoice::PlayPitched (one dword per bool).
			struct {
				void* ASound; void* SourceActor; float Volume; float Pitch; float FadeIn;
				uint32_t bSuppressSubtitles; uint32_t bSuppressSpatialization; void* AC;
			} parms = { nullptr, source ? (void*)source : (void*)c->Pawn, 1.0f, 1.0f, 0.0f, 1, source ? 0u : 1u, nullptr };
			for (UObject* cue : cues) { parms.ASound = cue; parms.AC = nullptr; c->ProcessEvent(fnSound, &parms, nullptr); }
			heard++;
		}
	}
	Logger::Log("worldgroups", "[WorldGroups] fx at (%.0f,%.0f,%.0f) r=%.0f emitter=%d groups=%d fire=%d cues=%d heard=%d shaken=%d\n",
		loc.X, loc.Y, loc.Z, radius, emitted, queued, fired, (int)cues.size(), heard, shaken);
}

} // namespace WorldGroups

