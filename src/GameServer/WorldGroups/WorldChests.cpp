#include "src/GameServer/WorldGroups/WorldGroups.hpp"
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include "src/GameServer/Core/FMallocWindows/GAllocator.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"

// "Placed chest" primitive (control-server admin actions
// place-chest / release-chest). Spawns a NEW TgChestActor at x,y,z (dropped
// onto the ground below) and reports every Use as GAME_EVENT chest_use
// {tag, character_id} instead of opening it. Release (or life_s) hides and
// destroys it.
//
// Default__TgChestActor is bNoDelete (level chests only), so the class
// default is cleared just around our Spawn; the spawned chest itself is a
// normal replicated actor. Clients need the launcher client patch to show
// it (exe: SpawnActor accepts the bNoDelete class, LoadMesh falls back to
// CHEST_Large01 when the replicated copy has no assembly name). A client
// only builds the mesh on a replicated r_eChestState change, so the state
// is pulsed 0 -> 3 -> 0 every few seconds (3 = no client case, invisible
// change): players who come into the zone later see the chest too.
namespace WorldGroups {
namespace {

using SpawnFn = void(__fastcall*)(ATgChestActor*, void*);
SpawnFn const kSpawnChest = reinterpret_cast<SpawnFn>(0x109b2de0);  // load mesh + ShowMesh

constexpr unsigned char kShown = 0;   // closed chest, Touch offers Use
constexpr unsigned char kGone  = 2;   // client HideMesh
constexpr unsigned char kPulse = 3;   // no client case: only re-arms the 0 change
constexpr int kUseMsgId = 52729;      // the map chests' Use prompt
constexpr float kPulseEvery = 2.0f;
constexpr float kPulseLen = 0.4f;
constexpr float kUseRange = 180.0f;   // proximity Use (beside the chest's own Touch)
constexpr float kDestroyAfterHide = 3.0f;

struct Spawned {
	ATgChestActor* chest;
	float expires;     // WorldInfo.TimeSeconds, 0 = never
	float nextPulse;
	bool pulsing;
	float destroyAt;   // > 0: hidden, destroy then
};

std::map<std::string, Spawned> s_chests;

AWorldInfo* World() {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	return Game ? Game->WorldInfo : nullptr;
}

bool Alive(ATgChestActor* c) { return c && !c->bDeleteMe; }

ATgChestActor* ClassDefault() {
	UClass* cls = ATgChestActor::StaticClass();
	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		UObject* obj = UObject::GObjObjects()->Data[i];
		if (obj && obj->Class == cls && strstr(obj->GetFullName(), "Default__")) return (ATgChestActor*)obj;
	}
	return nullptr;
}

// Height of a level chest's origin over the floor under it (placed chests
// stand right; 0 if this level has none).
float GroundOffset(AActor* tracer) {
	UClass* cls = ATgChestActor::StaticClass();
	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		UObject* obj = UObject::GObjObjects()->Data[i];
		if (!obj || obj->Class != cls || strstr(obj->GetFullName(), "Default__")) continue;
		ATgChestActor* c = (ATgChestActor*)obj;
		if (c->bDeleteMe || c->WorldInfo == nullptr || c->m_sAssemblyName.Count <= 1) continue;
		FVector start = c->Location, end = c->Location, hit, n;
		start.Z += 10.0f; end.Z -= 1000.0f;
		FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
		if (tracer->Trace(end, start, 0, FVector(0, 0, 0), 0, &hit, &n, &hi)) {
			const float off = c->Location.Z - hit.Z;
			if (off >= 0.0f && off < 200.0f) return off;
		}
	}
	return 0.0f;
}

void SetAssembly(ATgChestActor* c, const std::string& a) {
	std::wstring w(a.begin(), a.end());
	const int len = (int)w.size() + 1;
	c->m_sAssemblyName.Data = (wchar_t*)GAllocator::Malloc(len * sizeof(wchar_t));  // engine frees it
	c->m_sAssemblyName.Count = c->m_sAssemblyName.Max = len;
	wcscpy(c->m_sAssemblyName.Data, w.c_str());
}

void SendChestStatus(const std::string& tag, bool placed, const char* reason, ATgChestActor* c = nullptr) {
	nlohmann::json ev;
	ev["type"]        = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]     = "chest_status";
	ev["instance_id"] = IpcClient::GetInstanceId();
	ev["tag"]         = tag;
	ev["placed"]      = placed;
	ev["reason"]      = reason;
	if (c) { ev["x"] = c->Location.X; ev["y"] = c->Location.Y; ev["z"] = c->Location.Z; }
	IpcClient::Send(ev.dump());
}

// Hide on every client now, destroy a moment later (the hide must replicate
// first: a client copy that misses the destroy still stays hidden).
void Hide(const std::string& tag, Spawned& s, float now) {
	ATgChestActor* c = s.chest;
	if (Alive(c)) {
		c->r_eChestState = kGone;
		c->SetCollision(0, 0, c->bIgnoreEncroachers);
		if (c->m_CollisionProxy) ((AActor*)c->m_CollisionProxy)->SetCollision(0, 0, 0);
		c->bNetDirty = 1;
		for (auto& cc : GClientConnectionsData) {
			ATgPawn* p = (ATgPawn*)cc.second.Pawn;
			if (!cc.second.bClosed && p && p->m_CurrentUseActor == c) p->m_CurrentUseActor = nullptr;
		}
	}
	s.destroyAt = now + kDestroyAfterHide;
	Logger::Log("worldgroups", "[WorldChests] hide tag=%s\n", tag.c_str());
}

void DestroyNow(Spawned& s) {
	if (Alive(s.chest)) {
		if (s.chest->m_CollisionProxy) ((AActor*)s.chest->m_CollisionProxy)->Destroy();
		s.chest->Destroy();
	}
	s.chest = nullptr;
}

} // namespace

void PlaceChest(const nlohmann::json& j) {
	const std::string tag = j.value("tag", std::string());
	AWorldInfo* wi = World();
	if (tag.empty() || !wi || !j.contains("x") || !j.contains("y") || !j.contains("z")) {
		Logger::Log("worldgroups", "[WorldChests] place refused (tag/world/x,y,z missing)\n");
		return;
	}
	auto old = s_chests.find(tag);
	if (old != s_chests.end()) { DestroyNow(old->second); s_chests.erase(old); }

	FVector at{ j["x"].get<float>(), j["y"].get<float>(), j["z"].get<float>() };
	FRotator rot{ 0, (int)(j.value("yaw", 0.0f) * 65536.0f / 360.0f), 0 };
	// Drop onto the floor under the given spot (a pawn's x,y,z is its middle).
	FVector start = at, end = at, hit, n;
	start.Z += 100.0f; end.Z -= 2000.0f;
	FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
	const bool grounded = wi->Trace(end, start, 0, FVector(0, 0, 0), 0, &hit, &n, &hi) != nullptr;
	const float lift = GroundOffset(wi);
	if (grounded) at = FVector(hit.X, hit.Y, hit.Z + lift);

	ATgChestActor* def = ClassDefault();
	ATgChestActor* c = nullptr;
	if (def) {
		const unsigned long keep = def->bNoDelete;
		def->bNoDelete = 0;  // runtime Spawn refuses bNoDelete classes
		c = (ATgChestActor*)wi->Spawn(ATgChestActor::StaticClass(), nullptr, FName(), at, rot, nullptr, 1);
		def->bNoDelete = keep;
	}
	if (!c) {
		Logger::Log("worldgroups", "[WorldChests] tag=%s: spawn failed (default=%p)\n", tag.c_str(), def);
		SendChestStatus(tag, false, "no_chest");
		return;
	}
	SetAssembly(c, j.value("assembly", std::string("CHEST_Large01")));
	c->m_nUseMsgId = kUseMsgId;
	c->m_nLootTableId = 0;      // a use never rolls a map table
	c->m_MinRespawnTime = c->m_MaxRespawnTime = 0;
	c->m_bSpawnOnLevelLoad = 0;
	c->ClearTimer(FName("RespawnChest"), nullptr);
	c->bAlwaysRelevant = 1;
	c->AlwaysRelevantDistanceSquared = 1.0e12f;
	if (c->RemoteRole == 0) c->RemoteRole = 1;  // ROLE_SimulatedProxy
	c->r_eChestState = kShown;
	kSpawnChest(c, nullptr);    // server mesh + collision proxy (Touch -> Use)
	c->bNetDirty = 1;

	Spawned s{ c, 0.0f, wi->TimeSeconds + 1.0f, false, 0.0f };
	const float life = j.value("life_s", 300.0f);
	if (life > 0) s.expires = wi->TimeSeconds + (life > 3600 ? 3600 : life);
	s_chests[tag] = s;
	Logger::Log("worldgroups", "[WorldChests] spawn tag=%s chest=%s at (%.0f,%.0f,%.0f) asked (%.0f,%.0f,%.0f) ground=%d lift=%.0f life=%.0f proxy=%p\n",
		tag.c_str(), c->GetName(), c->Location.X, c->Location.Y, c->Location.Z,
		j["x"].get<float>(), j["y"].get<float>(), j["z"].get<float>(), (int)grounded, lift, life, c->m_CollisionProxy);
	SendChestStatus(tag, true, "placed", c);
}

void ReleaseChest(const std::string& tag) {
	auto it = s_chests.find(tag);
	if (it == s_chests.end() || it->second.destroyAt > 0) return;
	AWorldInfo* wi = World();
	Hide(tag, it->second, wi ? wi->TimeSeconds : 0.0f);
	SendChestStatus(tag, false, "released");
}

bool OnChestUse(ATgChestActor* chest, ATgPawn* pawn) {
	for (auto& kv : s_chests) {
		if (kv.second.chest != chest) continue;
		if (kv.second.destroyAt > 0) return true;
		for (auto& cc : GClientConnectionsData) {
			if (cc.second.bClosed || (ATgPawn*)cc.second.Pawn != pawn) continue;
			const int64_t characterId = cc.second.pPlayerInfo
				? cc.second.pPlayerInfo->selected_character_id
				: cc.second.PlayerInfo.selected_character_id;
			if (characterId == 0 || cc.second.SessionGuid.empty()) break;
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = "chest_use";
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = cc.second.SessionGuid;
			ev["character_id"] = characterId;
			ev["tag"]          = kv.first;
			IpcClient::Send(ev.dump());
			Logger::Log("worldgroups", "[WorldChests] use tag=%s char=%lld\n", kv.first.c_str(), (long long)characterId);
			break;
		}
		return true;  // spawned chests stay closed for everyone (loot is per player)
	}
	return false;
}

void TickChests() {
	if (s_chests.empty()) return;
	AWorldInfo* wi = World();
	if (!wi) return;
	const float now = wi->TimeSeconds;
	for (auto it = s_chests.begin(); it != s_chests.end();) {
		Spawned& s = it->second;
		if (s.destroyAt > 0) {
			if (now >= s.destroyAt) { DestroyNow(s); it = s_chests.erase(it); continue; }
			++it; continue;
		}
		if (!Alive(s.chest)) {
			Logger::Log("worldgroups", "[WorldChests] tag=%s chest gone\n", it->first.c_str());
			SendChestStatus(it->first, false, "expired");
			it = s_chests.erase(it); continue;
		}
		if (s.expires > 0 && now >= s.expires) {
			Hide(it->first, s, now);
			SendChestStatus(it->first, false, "expired");
			++it; continue;
		}
		ATgChestActor* c = s.chest;
		if (now >= s.nextPulse) {
			c->r_eChestState = s.pulsing ? kShown : kPulse;
			c->bNetDirty = 1;
			s.nextPulse = now + (s.pulsing ? kPulseEvery : kPulseLen);
			s.pulsing = !s.pulsing;
		}
		// Use by standing next to it (the Touch of the proxy also sets this).
		for (auto& cc : GClientConnectionsData) {
			ATgPawn* p = (ATgPawn*)cc.second.Pawn;
			if (cc.second.bClosed || !p || p->bDeleteMe) continue;
			const float dx = p->Location.X - c->Location.X, dy = p->Location.Y - c->Location.Y;
			const float dz = p->Location.Z - c->Location.Z;
			const bool inRange = dx * dx + dy * dy < kUseRange * kUseRange && std::fabs(dz) < 200.0f;
			if (inRange && p->m_CurrentUseActor == nullptr) p->m_CurrentUseActor = c;
			else if (!inRange && p->m_CurrentUseActor == c) p->m_CurrentUseActor = nullptr;
		}
		++it;
	}
}

} // namespace WorldGroups
