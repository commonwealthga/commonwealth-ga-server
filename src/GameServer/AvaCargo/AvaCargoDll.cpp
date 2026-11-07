#include "src/GameServer/AvaCargo/AvaCargoDll.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/PossessPawn/PossessPawn.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

// See AvaCargoDll.hpp.
namespace AvaCargoDll {
namespace {

struct Spawned { int tf; int item_id; int deployable_id; ATgDeployable* dep; bool taken = false; };   // taken: bot picked it up
std::vector<Spawned> g_spawned;
bool g_reported = false;
int g_botTf = 0;                                  // side whose packs bots carry
std::map<ATgDeployable*, std::pair<int, float>> g_claims;   // pack -> (pawn id, claim time)

// asm pickup_device_id (what the pack turns into when picked up); the setup chain leaves it 0 for packs.
int PickupDevice(int deployableId) {
	static std::map<int, int> cache;
	auto it = cache.find(deployableId);
	if (it != cache.end()) return it->second;
	int v = 0;
	if (sqlite3* db = Database::GetConnection()) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT pickup_device_id FROM asm_data_set_deployables WHERE deployable_id = ? LIMIT 1", -1, &st, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(st, 1, deployableId);
			if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
			sqlite3_finalize(st);
		}
	}
	cache[deployableId] = v;
	return v;
}

ATgRepInfo_TaskForce* TaskForce(int tf) {
	if (GTeamsData.Attackers && GTeamsData.Attackers->r_nTaskForce == tf) return GTeamsData.Attackers;
	if (GTeamsData.Defenders && GTeamsData.Defenders->r_nTaskForce == tf) return GTeamsData.Defenders;
	return nullptr;
}

float Dist2(const FVector& a, const FVector& b) {
	const float x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
	return x * x + y * y + z * z;
}

// The side's spawn spots: TgHexItemFactory actors whose nearest TgTeamPlayerStart has this task
// force (spot owner = factory). Fallback: a ring around that side's first player start.
struct Spot { FVector loc; FRotator rot; AActor* owner; };
std::vector<Spot> SpotsFor(int tf) {
	std::vector<Spot> out;
	std::vector<ATgTeamPlayerStart*> starts;
	for (ATgTeamPlayerStart* s : ActorCache::PlayerStarts) if (s && (s->m_nTaskForce == 1 || s->m_nTaskForce == 2)) starts.push_back(s);
	std::vector<ATgHexItemFactory*> facs;
	for (UObject* o : ObjectCache::FindAllByClass("Class TgGame.TgHexItemFactory")) if (o) facs.push_back((ATgHexItemFactory*)o);
	std::sort(facs.begin(), facs.end(), [](ATgHexItemFactory* a, ATgHexItemFactory* b) { return a->m_nMapObjectId < b->m_nMapObjectId; });
	for (ATgHexItemFactory* f : facs) {
		ATgTeamPlayerStart* best = nullptr;
		for (ATgTeamPlayerStart* s : starts)
			if (!best || Dist2(s->Location, f->Location) < Dist2(best->Location, f->Location)) best = s;
		if (best && best->m_nTaskForce == tf) out.push_back({f->Location, f->Rotation, (AActor*)f});
	}
	if (out.empty()) {
		for (ATgTeamPlayerStart* s : starts) {
			if (s->m_nTaskForce != tf) continue;
			for (int k = 0; k < 12; ++k) {
				const float a = 6.2831853f * (float)k / 12.0f;
				FVector l = s->Location; l.X += 350.0f * std::cos(a); l.Y += 350.0f * std::sin(a);
				out.push_back({l, s->Rotation, (AActor*)s});
			}
			break;
		}
	}
	Logger::Log("ava", "[AvaCargo] tf %d: %zu spot(s) (%zu hex item factories, %zu team starts)\n",
		tf, out.size(), facs.size(), starts.size());
	return out;
}

// Pawn-independent deployable spawn (same order of operations as TgDeployableFactory__SpawnObject):
// a pack waits at its spot until an agent picks it up, so no LifeSpan and no instigator are needed.
ATgDeployable* SpawnPack(const Spot& sp, int deployableId, int tf, const FVector& offset) {
	if (!sp.owner || !DeployableClassify::IsKnownDeployableId(deployableId)) return nullptr;
	const char* clsName = TgProj_Deployable__SpawnDeployable::GetDeployableClassName(deployableId);
	UClass* cls = clsName ? ClassPreloader::GetClass(clsName) : nullptr;
	if (!cls) {
		Logger::Log("ava", "[AvaCargo] deployable %d: no class (%s)\n", deployableId, clsName ? clsName : "?");
		return nullptr;
	}
	float r = 0.f, hh = 0.f, lift = 0.f;
	TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(deployableId, &r, &hh);
	TgProj_Deployable__SpawnDeployable::GetDeployableSpawnZLift(deployableId, &lift);
	FVector loc = sp.loc;
	loc.X += offset.X; loc.Y += offset.Y; loc.Z += offset.Z + lift + 5.0f;
	ATgDeployable* dep = (ATgDeployable*)sp.owner->Spawn(cls, sp.owner, FName(), loc, sp.rot, nullptr, 1);
	if (!dep) return nullptr;
	dep->r_nDeployableId = deployableId;
	dep->AdjustMeshToGround();
	dep->Instigator = nullptr;
	dep->eventInitReplicationInfo();
	if (dep->r_DRI) {
		if (ATgRepInfo_TaskForce* t = TaskForce(tf)) {
			dep->r_DRI->r_bOwnedByTaskforce = 1;
			dep->r_DRI->r_TaskforceInfo = t;
		}
		dep->r_DRI->bNetDirty = 1;
		dep->r_DRI->bForceNetUpdate = 1;
	}
	dep->r_bInitialIsEnemy = 0;
	TgProj_Deployable__SpawnDeployable::RegisterDeployableInGRI(dep);
	const bool destructible = DeployableClassify::IsDestructible(deployableId);
	dep->m_bInDestroyedState = 0;
	dep->r_bTakeDamage = destructible ? 1 : 0;
	dep->s_bIsActivated = 1;
	dep->m_bIsDeployed = 0;
	dep->r_nPhysicalType = 861;
	dep->bAlwaysRelevant = 1;
	if (!dep->r_EffectManager && destructible) {
		if (UClass* emClass = ClassPreloader::GetClass("Class TgGame.TgEffectManager")) {
			FVector z = {0.0f, 0.0f, 0.0f};
			dep->r_EffectManager = (ATgEffectManager*)dep->Spawn(emClass, dep, FName(), z, dep->Rotation, nullptr, 1);
		}
		if (dep->r_EffectManager) {
			dep->r_EffectManager->r_Owner = (AActor*)dep;
			dep->r_EffectManager->SetOwner((AActor*)dep);
			dep->r_EffectManager->Base = (AActor*)dep;
			dep->r_EffectManager->Role = 3;
			dep->r_EffectManager->RemoteRole = 1;
		}
	}
	dep->InitializeDefaultProps();
	dep->ApplyDeployableSetup();
	if (r > 0.0f && hh > 0.0f) {
		dep->SetCollisionSize(r, hh);
		if (dep->m_TargetComponent) dep->m_TargetComponent->SetCylinderSize(r, hh);
	}
	if (dep->m_EquipEffect) dep->eventApplyEquipEffects();
	dep->eventStartDeploy();
	dep->LifeSpan = 0.0f;   // waits at the dropship for the whole match
	if (const int pd = PickupDevice(deployableId)) dep->m_nPickupDeviceId = pd;   // B -> this device (Pack -> deployable device)
	dep->m_bPickupOnlyOnce = 1;
	Logger::Log("ava", "[AvaCargo]   %s hp=%d persist=%.1f activation=%.1f physics=%d deployed=%d\n", clsName,
		dep->r_nHealth, dep->s_fPersistTime, dep->s_fActivationTime, (int)dep->Physics, (int)dep->m_bIsDeployed);
	return dep;
}


// ---- Siege vehicles ------------------------------------------------------------------------
struct Vehicle {
	int tf; int item_id; int bot_id; ATgPawn* pawn;
	bool crewed; bool gone; int hp; float lastLog;
	ATgPawn* driver;
	std::string devs;   // crewed: "id:canfire" per device, logged on change (offhand/boost cooldowns)
};
std::vector<Vehicle> g_vehicles;
// Crew reach beyond the vehicle's own radius = the bot's asm crew_control_radius (feet, x16 = uu, 10 ft for all 8).
float CrewReach(int botId) {
	static std::map<int, float> cache;
	auto it = cache.find(botId);
	if (it != cache.end()) return it->second;
	float v = 0.0f;
	if (sqlite3* db = Database::GetConnection()) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT crew_control_radius FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1", -1, &st, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(st, 1, botId);
			if (sqlite3_step(st) == SQLITE_ROW) v = (float)sqlite3_column_double(st, 0) * 16.0f;
			sqlite3_finalize(st);
		}
	}
	if (v <= 0.0f) v = 160.0f;
	cache[botId] = v;
	return v;
}

ATgGame* GameNow() { return (ATgGame*)Globals::Get().GGameInfo; }
float Now() { ATgGame* g = GameNow(); return (g && g->WorldInfo) ? g->WorldInfo->TimeSeconds : 0.0f; }

// Our vehicles that are still in the world's pawn list (a destroyed pawn leaves it in Destroyed()).
// Never dereference a vehicle pointer that is not in this list.
std::set<ATgPawn*> LivePawns() {
	std::set<ATgPawn*> out;
	ATgGame* g = GameNow();
	if (!g || !g->WorldInfo || g_vehicles.empty()) return out;
	std::set<ATgPawn*> mine;
	for (const auto& v : g_vehicles) if (v.pawn) mine.insert(v.pawn);
	int guard = 0;
	for (APawn* p = g->WorldInfo->PawnList; p && guard < 4096; p = p->NextPawn, ++guard)
		if (mine.count((ATgPawn*)p)) out.insert((ATgPawn*)p);
	return out;
}

bool VehicleAlive(const Vehicle& v, const std::set<ATgPawn*>& live) {
	return v.pawn && live.count(v.pawn) && !v.pawn->bDeleteMe && v.pawn->Health > 0;
}

void ApplyTaskForce(ATgPawn* bot, int tf) {
	ATgRepInfo_Player* rep = (ATgRepInfo_Player*)bot->PlayerReplicationInfo;
	ATgRepInfo_TaskForce* team = TaskForce(tf);
	if (!rep || !team) return;
	rep->r_TaskForce = team;
	rep->Team        = team;
	rep->SetTeam(team);
	bot->NotifyTeamChanged();
}

void SpawnUseProxy(ATgPawn* bot, float radius, float halfHeight, float reach) {
	if (bot->s_UseProxy != nullptr) return;
	UClass* cls = ClassPreloader::GetClass("Class TgGame.TgCollisionProxy");
	if (!cls) return;
	ATgCollisionProxy* p = (ATgCollisionProxy*)bot->Spawn(cls, (AActor*)bot, FName(), bot->Location, bot->Rotation, nullptr, 1);
	if (!p) { Logger::Log("ava", "[AvaCargo] vehicle use-proxy spawn FAILED\n"); return; }
	p->SetOwner((AActor*)bot);
	p->SetBase((AActor*)bot, FVector(0, 0, 0), nullptr, FName());
	if (UCylinderComponent* cyl = (UCylinderComponent*)p->CollisionComponent)
		cyl->SetCylinderSize(radius + reach, std::max(halfHeight, 80.0f) + 120.0f);
	bot->s_UseProxy = p;
}

// The vehicle waits at its spot under its own AI controller (same as the Acquisition robot) until an agent of
// its side crews it with B. Collision ignored at spawn so it always appears even on a crowded pad.
ATgPawn* SpawnVehicle(const Spot& sp, int botId, int tf, const FVector& offset) {
	ATgGame* g = GameNow();
	if (!g || botId <= 0) return nullptr;
	float r = 0.0f, hh = 0.0f;
	TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &r, &hh);
	FVector loc = sp.loc;
	loc.X += offset.X; loc.Y += offset.Y; loc.Z += offset.Z + (hh > 0.0f ? hh : 60.0f) + 5.0f;
	ATgPawn* bot = (ATgPawn*)g->SpawnBotById(botId, loc, sp.rot, /*bKillController=*/false, /*pFactory=*/nullptr,
		/*bIgnoreCollision=*/true, /*pOwnerPawn=*/nullptr, /*bIsDecoy=*/false, /*deviceFire=*/nullptr, 0.0f);
	if (!bot) return nullptr;
	ApplyTaskForce(bot, tf);
	bot->s_bIsCrewable = 1;
	SpawnUseProxy(bot, r, hh, CrewReach(botId));
	bot->bNetDirty = 1;
	bot->bForceNetUpdate = 1;
	return bot;
}

Vehicle* VehicleOf(ATgPawn* p) {
	for (auto& v : g_vehicles) if (v.pawn == p) return &v;
	return nullptr;
}
}  // namespace

void Spawn(const std::string& json) {
	auto j = nlohmann::json::parse(json, nullptr, false);
	if (j.is_discarded() || !j.contains("sides")) return;
	int total = 0, failed = 0, vehicles = 0;
	g_vehicles.clear();
	g_botTf = 0; g_claims.clear();
	for (const auto& side : j["sides"]) {
		const int tf = side.value("task_force", 0);
		if (tf != 1 && tf != 2) continue;
		if (side.value("bot_carriers", 0)) { g_botTf = tf; Logger::Log("ava", "[AvaCargo] tf %d: bot carriers (service agency cargo)\n", tf); }
		const auto spots = SpotsFor(tf);
		if (spots.empty()) {
			Logger::Log("ava", "[AvaCargo] tf %d: no spot on this map - cargo not spawned (returned at match end)\n", tf);
			continue;
		}
		size_t k = 0, kv = 0;
		for (const auto& it : side["items"]) {
			const int item = it.value("item_id", 0), dep_id = it.value("deployable_id", 0), n = it.value("count", 0);
			if (const int bot_id = it.value("bot_id", 0)) {   // a siege vehicle - from the far end of the spot list
				for (int c = 0; c < n && c < 20; ++c, ++kv) {
					// every 3rd spot from the far end: vehicles are big, keep them apart (14 spots -> 5 per lap)
					const size_t lap = (spots.size() + 2) / 3, slot = kv % lap;
					const Spot& sp = spots[spots.size() - 1 - std::min(spots.size() - 1, slot * 3)];
					const float layer = (float)(kv / lap);
					FVector off = {-layer * 300.0f, layer * 300.0f, 0.0f};
					ATgPawn* v = SpawnVehicle(sp, bot_id, tf, off);
					if (v) { g_vehicles.push_back({tf, item, bot_id, v, false, false, v->Health, 0.0f, nullptr, {}}); ++vehicles; }
					else ++failed;
					Logger::Log("ava", "[AvaCargo] tf %d item %d vehicle bot %d -> %s at (%.0f,%.0f,%.0f) hp=%d class=%s crewable=%d\n", tf, item, bot_id,
						v ? "spawned" : "FAILED", sp.loc.X + off.X, sp.loc.Y + off.Y, sp.loc.Z, v ? v->Health : 0,
						v ? v->Class->GetName() : "-", v ? (int)v->s_bIsCrewable : 0);
					if (v) Logger::Log("ava", "[AvaCargo]   vehicle stats bot %d: hpmax %d ground %.1f air %.1f accel %.1f jumpz %.1f rot %d/%d/%d power %.0f/%.0f body_asm %d mesh %s\n",
						bot_id, v->r_nHealthMaximum, v->GroundSpeed, v->AirSpeed, v->AccelRate, v->JumpZ, v->RotationRate.Pitch, v->RotationRate.Yaw,
						v->RotationRate.Roll, v->r_fCurrentPowerPool, v->r_fMaxPowerPool, v->r_nBodyMeshAsmId,
						(v->Mesh && v->Mesh->SkeletalMesh) ? v->Mesh->SkeletalMesh->GetName() : "-");
				}
				continue;
			}
			for (int c = 0; c < n && c < 100; ++c, ++k) {
				const Spot& sp = spots[k % spots.size()];
				const float layer = (float)(k / spots.size());
				FVector off = {layer * 90.0f, layer * 90.0f, layer * 40.0f};   // later laps stand beside the first ones
				ATgDeployable* d = SpawnPack(sp, dep_id, tf, off);
				if (d) { g_spawned.push_back({tf, item, dep_id, d}); ++total; }
				else ++failed;
				Logger::Log("ava", "[AvaCargo] tf %d item %d deployable %d -> %s at (%.0f,%.0f,%.0f) pickup_device=%d\n", tf, item, dep_id,
					d ? "spawned" : "FAILED", sp.loc.X + off.X, sp.loc.Y + off.Y, sp.loc.Z, d ? d->m_nPickupDeviceId : 0);
			}
		}
	}
	g_reported = false;
	Logger::Log("ava", "[AvaCargo] AVA_CARGO: %d pack(s) spawned, %d failed\n", total, failed);
	if (vehicles) Logger::Log("ava", "[AvaCargo] AVA_CARGO: %d vehicle(s) spawned (crew with B)\n", vehicles);
}

void ReportResult() {
	if ((g_spawned.empty() && g_vehicles.empty()) || g_reported) return;
	g_reported = true;
	// Which of our packs still exist untouched? Never dereference a pointer we did not find alive.
	// Direct GObjObjects walk: spawned actors can reuse array slots below ObjectCache's scan mark
	// so FindAllByClass* may miss them.
	std::set<ATgDeployable*> mine, alive;
	for (const auto& s : g_spawned) mine.insert(s.dep);
	if (TArray<UObject*>* objects = UObject::GObjObjects())
		for (int i = 0; i < objects->Count; i++) {
			UObject* o = objects->Data[i];
			if (o && mine.count((ATgDeployable*)o)) alive.insert((ATgDeployable*)o);
		}
	std::map<std::pair<int, int>, int> back;
	int used = 0;
	for (const auto& s : g_spawned) {
		ATgDeployable* d = s.dep;
		const bool untouched = !s.taken && alive.count(d) && !d->bDeleteMe && d->r_nDeployableId == s.deployable_id
			&& !d->s_bWasPickedUp && !d->m_bInDestroyedState;
		if (untouched) back[{s.tf, s.item_id}]++;
		else ++used;
		if (alive.count(d))
			Logger::Log("ava", "[AvaCargo]   item %d dep %d: alive deleteMe=%d pickedUp=%d destroyed=%d hp=%d at (%.0f,%.0f,%.0f) life=%.1f\n", s.item_id, s.deployable_id,
				(int)d->bDeleteMe, (int)d->s_bWasPickedUp, (int)d->m_bInDestroyedState, d->r_nHealth, d->Location.X, d->Location.Y, d->Location.Z, d->LifeSpan);
		else Logger::Log("ava", "[AvaCargo]   item %d dep %d: gone\n", s.item_id, s.deployable_id);
	}
	// Vehicles: alive at the end = back to the agency (crewed or not), destroyed = used.
	{
		const auto live = LivePawns();
		for (auto& v : g_vehicles) {
			const bool alive = VehicleAlive(v, live);
			if (!v.item_id) continue;   // dev-spawned (stat check), not cargo
			if (alive) back[{v.tf, v.item_id}]++;
			else ++used;
			Logger::Log("ava", "[AvaCargo]   item %d vehicle bot %d: %s%s\n", v.item_id, v.bot_id,
				alive ? "alive -> returned" : "destroyed -> used", v.crewed ? " (was crewed)" : "");
		}
		g_vehicles.clear();
	}
	nlohmann::json msg;
	msg["type"] = "AVA_CARGO_RESULT";
	msg["instance_id"] = IpcClient::GetInstanceId();
	msg["returns"] = nlohmann::json::array();
	int ret = 0;
	for (const auto& [k, n] : back) { msg["returns"].push_back({{"task_force", k.first}, {"item_id", k.second}, {"count", n}}); ret += n; }
	IpcClient::Send(msg.dump());
	Logger::Log("ava", "[AvaCargo] AVA_CARGO_RESULT: %d untouched (returned), %d picked up/used\n", ret, used);  // vehicles: alive = returned
	g_spawned.clear();
}


// --- pickup fallback -------------------------------------------------------------------
namespace {
std::vector<ATgDeployable*> LiveUntouched() {
	std::vector<ATgDeployable*> out;
	if (g_spawned.empty()) return out;
	std::map<ATgDeployable*, int> mine;
	for (const auto& s : g_spawned) if (!s.taken) mine[s.dep] = s.deployable_id;   // taken = used even if the address is reused
	if (TArray<UObject*>* objects = UObject::GObjObjects())
		for (int i = 0; i < objects->Count; i++) {
			auto it = mine.find((ATgDeployable*)objects->Data[i]);
			if (it == mine.end()) continue;
			ATgDeployable* d = it->first;
			if (!d->bDeleteMe && d->r_nDeployableId == it->second && !d->s_bWasPickedUp && !d->m_bInDestroyedState) out.push_back(d);
		}
	return out;
}
int TfOfPack(ATgDeployable* d) {
	for (const auto& s : g_spawned) if (s.dep == d) return s.tf;
	return 0;
}
}  // namespace

int UntouchedCount() { return g_spawned.empty() ? 0 : (int)LiveUntouched().size(); }

// ---- Bot carriers ----
int BotCarrierTf() { return g_spawned.empty() ? 0 : g_botTf; }
bool BotPackLive(ATgDeployable* d) {
	if (!d || !g_botTf) return false;
	for (ATgDeployable* x : LiveUntouched()) if (x == d) return true;
	return false;
}
ATgDeployable* BotClaimPack(int tf, const FVector& at, float maxDist, int pawnId, float now) {
	if (!g_botTf || tf != g_botTf) return nullptr;
	ATgDeployable* best = nullptr; float bd = maxDist * maxDist;
	for (ATgDeployable* d : LiveUntouched()) {
		if (TfOfPack(d) != tf) continue;
		auto c = g_claims.find(d);
		if (c != g_claims.end() && c->second.first != pawnId && now - c->second.second < 45.0f) continue;   // someone else's run
		const float d2 = Dist2(d->Location, at);
		if (d2 < bd) { bd = d2; best = d; }
	}
	if (best) g_claims[best] = {pawnId, now};
	return best;
}
int BotTakePack(ATgDeployable* d) {
	if (!BotPackLive(d)) return 0;
	int id = 0;
	for (auto& s : g_spawned) if (s.dep == d && !s.taken) { id = s.deployable_id; s.taken = true; break; }
	g_claims.erase(d);
	d->s_bWasPickedUp = 1;   // not untouched any more -> not returned at match end (used, like a player pickup)
	d->Destroy();
	return id;
}
void BotReleasePack(int pawnId) {
	for (auto it = g_claims.begin(); it != g_claims.end();) it = it->second.first == pawnId ? g_claims.erase(it) : std::next(it);
}

bool PickupFallback(APlayerController* pc, int untouchedBefore) {
	if (g_spawned.empty() || g_reported || !pc || !pc->Pawn) return false;
	ATgPawn* pawn = (ATgPawn*)pc->Pawn;
	const auto live = LiveUntouched();
	if ((int)live.size() < untouchedBefore) {   // UC already picked one of ours up
		Logger::Log("ava", "[AvaCargo] pickup key: UC picked up a pack (%d -> %zu untouched)\n", untouchedBefore, live.size());
		return true;
	}
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
	const int tf = (pri && pri->r_TaskForce) ? pri->r_TaskForce->r_nTaskForce : 0;
	ATgDeployable* best = nullptr; float bestD = 1e30f;
	for (ATgDeployable* d : live) {
		if (TfOfPack(d) != tf) continue;
		const float dx = pawn->Location.X - d->Location.X, dy = pawn->Location.Y - d->Location.Y, dz = pawn->Location.Z - d->Location.Z;
		const float xy = dx * dx + dy * dy;
		// reach: on top of / next to a pack (platform packs are ~2 m boxes)
		if (xy > 160.f * 160.f || dz < -120.f || dz > 160.f) continue;
		if (xy < bestD) { bestD = xy; best = d; }
	}
	if (!best) {
		Logger::Log("ava", "[AvaCargo] pickup key: tf %d at (%.0f,%.0f,%.0f), no own pack within reach (%zu untouched)\n", tf,
			pawn->Location.X, pawn->Location.Y, pawn->Location.Z, live.size());
		return false;
	}
	const int dep = best->r_nDeployableId, dev = best->m_nPickupDeviceId;
	const bool ok = best->PickUpDeployable(pawn);
	Logger::Log("ava", "[AvaCargo] pickup key: tf %d picked up deployable %d (dist %.0f) -> device %d: %s\n", tf, dep, std::sqrt(bestD), dev, ok ? "OK" : "refused");
	return ok;
}


// --- Vehicle key, tick, dev ------------------------------------------------------------------
bool VehicleKeyBefore(APlayerController* pc) {
	if (g_vehicles.empty() || !pc || !pc->Pawn) return false;
	ATgPawn* p = (ATgPawn*)pc->Pawn;
	return p->r_bIsBot && p->r_bIsHacked;   // the controller drives a crewed bot: this B is "get out"
}

bool VehicleKeyAfter(APlayerController* pc, bool wasDriving) {
	if (g_vehicles.empty() || g_reported || !pc || !pc->Pawn) return false;
	ATgPawn* p = (ATgPawn*)pc->Pawn;
	if (wasDriving) {   // never pick up a pack/beacon with the same press that leaves a vehicle
		Logger::Log("ava", "[AvaCargo] vehicle key: left a vehicle -> agent at (%.0f,%.0f,%.0f)\n", p->Location.X, p->Location.Y, p->Location.Z);
		return true;
	}
	if (p->r_bIsBot && p->r_bIsHacked) {   // UC just crewed one (PickupNearestBot -> CrewBot)
		if (Vehicle* v = VehicleOf(p)) Logger::Log("ava", "[AvaCargo] vehicle key: crewed vehicle bot %d (UC)\n", v->bot_id);
		return true;
	}
	if (p->r_bIsCrewing || p->r_bIsBot) return false;
	if (Now() - ((ATgPlayerController*)pc)->m_fLastCrewBotTime < 1.0f) return false;   // UC crew delay (1 s) as in ServerPickupPutdownDeployable
	// Fallback: an own-side, uncrewed vehicle right beside the agent whose use volume never fired Touch.
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)p->PlayerReplicationInfo;
	const int tf = (pri && pri->r_TaskForce) ? pri->r_TaskForce->r_nTaskForce : 0;
	const auto live = LivePawns();
	Vehicle* best = nullptr; float bestD = 1e30f;
	for (auto& v : g_vehicles) {
		if (v.tf != tf || !VehicleAlive(v, live) || v.pawn->r_bIsHacked) continue;
		float r = 0.0f, hh = 0.0f;
		TgGame__SpawnBotById::GetBotCollisionCylinder(v.bot_id, &r, &hh);
		const float reach = r + CrewReach(v.bot_id);
		const float dx = p->Location.X - v.pawn->Location.X, dy = p->Location.Y - v.pawn->Location.Y, dz = p->Location.Z - v.pawn->Location.Z;
		const float xy = dx * dx + dy * dy;
		if (xy > reach * reach || dz < -250.0f || dz > 250.0f) continue;
		if (xy < bestD) { bestD = xy; best = &v; }
	}
	if (!best) return false;
	if (!best->pawn->CanBeCrewedBy(p)) {
		Logger::Log("ava", "[AvaCargo] vehicle key: bot %d beside tf %d agent refuses crew\n", best->bot_id, tf);
		return false;
	}
	p->CrewBot(best->pawn);
	((ATgPlayerController*)pc)->m_fLastCrewBotTime = Now();
	Logger::Log("ava", "[AvaCargo] vehicle key: tf %d crewed vehicle bot %d (fallback, dist %.0f) -> %s\n", tf, best->bot_id, std::sqrt(bestD),
		best->pawn->r_bIsHacked ? "OK" : "refused");
	return best->pawn->r_bIsHacked != 0;
}

void Tick() {
	if (g_vehicles.empty() || g_reported) return;
	const float t = Now();
	static float last = -10.0f;
	if (t - last < 1.0f && t >= last) return;
	last = t;
	const auto live = LivePawns();
	for (auto& v : g_vehicles) {
		if (v.gone) continue;
		if (!VehicleAlive(v, live)) {
			v.gone = true;
			Logger::Log("ava", "[AvaCargo] vehicle item %d bot %d tf %d DESTROYED (was crewed=%d) -> used up\n", v.item_id, v.bot_id, v.tf, (int)v.crewed);
			continue;
		}
		ATgPawn* b = v.pawn;
		const bool crewed = b->r_bIsHacked != 0;
		if (crewed != v.crewed) {
			v.crewed = crewed;
			v.driver = crewed ? b->r_ControlPawn : nullptr;
			Logger::Log("ava", "[AvaCargo] vehicle item %d bot %d %s agent %p hp=%d/%d at (%.0f,%.0f,%.0f)\n", v.item_id, v.bot_id,
				crewed ? "CREWED by" : "left by", (void*)v.driver, b->Health, b->r_nHealthMaximum,
				b->Location.X, b->Location.Y, b->Location.Z);
			if (crewed) {   // the driver fires the vehicle's own devices: list them, put a hand weapon up if none is
				ATgDevice* hand = nullptr;
				for (int i = 0; i < 0x19; ++i) {
					ATgDevice* d = b->m_EquippedDevices[i];
					if (!d) continue;
					Logger::Log("ava", "[AvaCargo]   vehicle device eq %d id %d hand=%d type=%d\n", i, d->r_nDeviceId, (int)d->m_bHandDevice, d->m_nDeviceType);
					if (!hand && d->m_bHandDevice) hand = d;
				}
				if (!b->Weapon && hand) {
					const bool ok = b->eventSetActiveWeapon((AWeapon*)hand, 0, 1);
					Logger::Log("ava", "[AvaCargo]   vehicle weapon -> device %d (SetActiveWeapon %d, now %p)\n", hand->r_nDeviceId, (int)ok, (void*)b->Weapon);
				}
			}
			// The driver's hotbar shows the vehicle's own devices while crewed (UC HackedBy/Unhacked call
			// SetInventoryDirty, a stub in this build): replicate the vehicle devices + push its loadout;
			// on leave the player's own refresh runs through the SetInventoryDirty hook.
			const int n = TgPlayerActions::PossessCmd::ShowVehicleLoadout(b, crewed ? v.driver : nullptr, crewed);
			Logger::Log("ava", "[AvaCargo]   vehicle hotbar %s (%d device(s))\n", crewed ? "-> vehicle devices" : "-> agent devices", n);
		}
		if (crewed || b->Health != v.hp) {
			unsigned fire = 0;   // shots: r_nFlashFireInfo changes with every replicated weapon flash
			for (int i = 0; i < 0x20; ++i) fire = fire * 31u + (unsigned)b->r_nFlashFireInfo[i];
			Logger::Log("ava", "[AvaCargo] vehicle bot %d pos (%.0f,%.0f,%.0f) yaw %d hp %d/%d crewed=%d vel %.0f flash %u fire %08x\n", v.bot_id,
				b->Location.X, b->Location.Y, b->Location.Z, b->Rotation.Yaw & 65535, b->Health, b->r_nHealthMaximum, (int)crewed,
				std::sqrt(b->Velocity.X * b->Velocity.X + b->Velocity.Y * b->Velocity.Y), (unsigned)b->FlashCount, fire);
			if (crewed) {
				std::string d;
				for (int i = 0; i < 0x19; ++i)
					if (ATgDevice* x = b->m_EquippedDevices[i]) d += std::to_string(x->r_nDeviceId) + ":" + (x->m_bDeviceCanFire ? "1 " : "0 ");
				if (d != v.devs) { v.devs = d; Logger::Log("ava", "[AvaCargo]   vehicle bot %d devices canfire %s\n", v.bot_id, d.c_str()); }
			}
			if (crewed) Logger::Log("ava", "[AvaCargo]   accel %.0f weapon %d ctrl %p ground %.1f power %.0f\n",
				std::sqrt(b->Acceleration.X * b->Acceleration.X + b->Acceleration.Y * b->Acceleration.Y),
				b->Weapon ? ((ATgDevice*)b->Weapon)->r_nDeviceId : 0, (void*)b->Controller, b->GroundSpeed, b->r_fCurrentPowerPool);
			v.hp = b->Health;
		}
	}
}

void DevVehicleHealth(int pct, int taskForce, int botId) {
	if (pct < 0 && botId > 0 && !g_vehicles.empty() && !g_reported) {   // dev: one more vehicle beside the first (stat checks; never in the result)
		const auto live0 = LivePawns();
		const Vehicle& f = g_vehicles.front();
		if (!VehicleAlive(f, live0)) return;
		float cr = 0.0f, ch = 0.0f; f.pawn->GetBoundingCylinder(&cr, &ch);
		Spot sp{f.pawn->Location, f.pawn->Rotation, nullptr}; sp.loc.Z -= ch;
		const int tf = taskForce ? taskForce : f.tf;
		ATgPawn* v = SpawnVehicle(sp, botId, tf, FVector(-pct * 1.0f, 0.0f, 0.0f));
		if (v) g_vehicles.push_back({tf, 0, botId, v, false, false, v->Health, 0.0f, nullptr, {}});
		Logger::Log("ava", "[AvaCargo] DEV vehicle bot %d -> %s at (%.0f,%.0f,%.0f) hp=%d class=%s\n", botId, v ? "spawned" : "FAILED",
			v ? v->Location.X : 0.0f, v ? v->Location.Y : 0.0f, v ? v->Location.Z : 0.0f, v ? v->Health : 0, v ? v->Class->GetName() : "-");
		if (v) Logger::Log("ava", "[AvaCargo]   vehicle stats bot %d: hpmax %d ground %.1f air %.1f accel %.1f jumpz %.1f rot %d/%d/%d power %.0f/%.0f body_asm %d mesh %s\n",
			botId, v->r_nHealthMaximum, v->GroundSpeed, v->AirSpeed, v->AccelRate, v->JumpZ, v->RotationRate.Pitch, v->RotationRate.Yaw,
			v->RotationRate.Roll, v->r_fCurrentPowerPool, v->r_fMaxPowerPool, v->r_nBodyMeshAsmId,
			(v->Mesh && v->Mesh->SkeletalMesh) ? v->Mesh->SkeletalMesh->GetName() : "-");
		return;
	}
	const auto live = LivePawns();
	for (auto& v : g_vehicles) {
		if ((taskForce && v.tf != taskForce) || (botId && v.bot_id != botId) || !VehicleAlive(v, live)) continue;
		ATgPawn* b = v.pawn;
		if (pct > 0) {
			b->Health = std::max(1, b->r_nHealthMaximum * pct / 100);
			Logger::Log("ava", "[AvaCargo] DEV vehicle bot %d hp -> %d\n", v.bot_id, b->Health);
		} else {
			// The game's own death entry: virtual TgPawn.Died (UnCrewed(true) -> driver out + killed, then
			// Pawn.Died). APawn::Died() would call Engine.Pawn.Died directly, skipping UnCrewed, and
			// TgGame.PreventDeath refuses any pawn a TgPlayerController still drives (HP clamps to 1).
			static UFunction* fnDied = nullptr;
			if (!fnDied) fnDied = (UFunction*)ObjectCache::Find("Function TgGame.TgPawn.Died");
			b->Health = 0;
			APawn_execDied_Parms prm{};
			prm.HitLocation = b->Location;
			if (fnDied) b->ProcessEvent(fnDied, &prm, nullptr);
			Logger::Log("ava", "[AvaCargo] DEV vehicle bot %d destroy via TgPawn.Died fn=%p -> ret=%d hp=%d hacked=%d\n", v.bot_id, (void*)fnDied,
				(int)prm.ReturnValue, b->Health, (int)b->r_bIsHacked);
		}
	}
}

}  // namespace AvaCargoDll
