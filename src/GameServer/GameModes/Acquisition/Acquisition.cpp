#include "src/GameServer/GameModes/Acquisition/Acquisition.hpp"

#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <cmath>
#include <vector>

namespace {
constexpr const char* CH = "acq";

// Original rule set (Demolition -> Acquisition, patch 1.4): first to 3
// robot captures wins; more captures at time-out wins.
constexpr int   kCapsToWin       = 3;
// Robot respawn safety net (the UC OnFlagConsumed path re-requests the robot
// itself; this only catches a robot that vanished without that path).
constexpr float kRespawnNetDelay = 12.0f;
// Crew use-proxy (PickupNearestBot scans TgCollisionProxy owned by a crewable
// bot): radius around the robot where key B crews it.
constexpr float kUseRadius       = 260.0f;
constexpr float kUseHalfHeight   = 200.0f;
constexpr int   kDefaultRobotBot = 1127;   // asm objective 230 "02_StandardCTR" bot_id

// Announcer lines (asm msg + sound): 23525 robot available, 23526 your team
// has your robot, 23527 enemy picked up theirs, 23528 your team captured,
// 23529 enemy captured, 24095 robot destroyed.
constexpr int MSG_AVAILABLE      = 23525;
constexpr int MSG_YOU_HAVE       = 23526;
constexpr int MSG_ENEMY_HAS      = 23527;
constexpr int MSG_YOU_CAPTURED   = 23528;
constexpr int MSG_ENEMY_CAPTURED = 23529;
constexpr int MSG_DESTROYED      = 24095;
constexpr unsigned char APT_HIGH = 2;
constexpr unsigned char ATT_REGULAR = 0, ATT_BENEFICIAL = 1, ATT_DETRIMENTAL = 2, ATT_IMPORTANT = 3;

// TgBaseObjective_CTFBot vtable slots (exec thunks: SetObjectiveActive
// [+0x37c], SpawnObjectiveBot [+0x3e8], RemoveFlagFromPlayField [+0x3f0],
// SendFlagPickedUp/Captured/Spawned/ConsumedMessage [+0x3f4/+0x3f8/+0x3fc/+0x400])
// and the binary's values in vtable 0x1170d220 (checked before patching).
struct Slot { unsigned off; uintptr_t expect; };
constexpr Slot kSetActive = {0x37c, 0x10a797e0};   // stub `ret 4`
constexpr Slot kSpawn     = {0x3e8, 0x10a7a620};   // real: TF + CTR flag on an EXISTING bot
constexpr Slot kRemove    = {0x3f0, 0x10a7a560};   // real: kill + clear r_ObjectiveBot
constexpr Slot kPicked    = {0x3f4, 0x10a79810};   // stub `ret 4`
constexpr Slot kCaptured  = {0x3f8, 0x10a79820};   // stub `ret 4`
constexpr Slot kSpawned   = {0x3fc, 0x10a797f0};   // stub `ret`
constexpr Slot kConsumed  = {0x400, 0x10a79800};   // stub `ret`

typedef void(__fastcall* ObjVoid_t)(ATgBaseObjective_CTFBot*, void*);
ObjVoid_t s_binSpawnSetup = nullptr;
ObjVoid_t s_binRemove     = nullptr;
void**    s_patchedVt     = nullptr;
bool      s_patchFailed   = false;

struct ObjState { ATgBaseObjective_CTFBot* obj; float goneSince; ATgCollisionProxy* proxy; ATgPawn* bot; };
std::vector<ObjState> s_objs;
void*  s_world        = nullptr;   // WorldInfo the state above belongs to
bool   s_volumesDone  = false;
float  s_lastTick     = -1.0f;
float  s_lastSpawnMsg = -100.0f;

ATgGame* GameNow() { return (ATgGame*)Globals::Get().GGameInfo; }

bool IsDualCTF(UObject* Game) {
	return Game != nullptr && ObjectClassCache::ClassNameContains(Game, "TgGame_DualCTF");
}

float Now() {
	ATgGame* G = GameNow();
	return (G && G->WorldInfo) ? G->WorldInfo->TimeSeconds : 0.0f;
}

ATgRepInfo_TaskForce* TfPtr(int tf) {
	return tf == 1 ? GTeamsData.Attackers : (tf == 2 ? GTeamsData.Defenders : nullptr);
}

void AnnounceTeams(int tf, int msgMine, int msgOther) {
	ATgRepInfo_TaskForce* mine  = TfPtr(tf);
	ATgRepInfo_TaskForce* other = TfPtr(tf == 1 ? 2 : 1);
	if (mine)  SendAlert::BroadcastToTaskforce(mine,  msgMine,  APT_HIGH, ATT_BENEFICIAL,  3.0f);
	if (other) SendAlert::BroadcastToTaskforce(other, msgOther, APT_HIGH, ATT_DETRIMENTAL, 3.0f);
}

bool PatchSlot(void** vt, const Slot& s, void* fn, void** outOrig) {
	void** p = (void**)((char*)vt + s.off);
	if ((uintptr_t)*p != s.expect && *p != fn) {
		Logger::Log(CH, "[Acq] vtable %p slot +0x%x = %p, expected %08x - NOT patching (binary differs)\n",
			vt, s.off, *p, (unsigned)s.expect);
		return false;
	}
	if (outOrig && *p != fn) *outOrig = *p;
	DWORD old = 0;
	if (!VirtualProtect(p, sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
	*p = fn;
	VirtualProtect(p, sizeof(void*), old, &old);
	return true;
}

void ApplyTaskForce(ATgPawn* Bot, int tf) {
	ATgRepInfo_Player* rep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	ATgRepInfo_TaskForce* team = TfPtr(tf);
	if (!rep || !team) return;
	rep->r_TaskForce = team;
	rep->Team        = team;
	rep->SetTeam(team);
	Bot->NotifyTeamChanged();
}

void SpawnUseProxy(ATgPawn* Bot) {
	if (Bot->s_UseProxy != nullptr) return;
	UClass* cls = ClassPreloader::GetClass("Class TgGame.TgCollisionProxy");
	if (!cls) return;
	ATgCollisionProxy* P = (ATgCollisionProxy*)Bot->Spawn(cls, (AActor*)Bot, FName(),
		Bot->Location, Bot->Rotation, nullptr, 1);
	if (!P) { Logger::Log(CH, "[Acq] use-proxy spawn FAILED\n"); return; }
	P->SetOwner((AActor*)Bot);
	P->SetBase((AActor*)Bot, FVector(0, 0, 0), nullptr, FName());
	if (UCylinderComponent* cyl = (UCylinderComponent*)P->CollisionComponent)
		cyl->SetCylinderSize(kUseRadius, kUseHalfHeight);
	Bot->s_UseProxy = P;
}

bool BotAlive(ATgPawn* B) {
	return B != nullptr && !B->bDeleteMe && B->Health > 0;
}

// ---- patched natives ----------------------------------------------------

void __fastcall AcqSpawnObjectiveBot(ATgBaseObjective_CTFBot* Obj, void* edx) {
	if (Obj == nullptr) return;
	if (!BotAlive((ATgPawn*)Obj->r_ObjectiveBot)) {
		ATgGame* Game = GameNow();
		if (Game == nullptr) return;
		const int botId = Obj->s_nBotId > 0 ? Obj->s_nBotId : kDefaultRobotBot;
		FVector loc = Obj->Location;
		float radius = 0.0f, halfHeight = 0.0f;
		TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
		if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
		ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(botId, loc, Obj->Rotation,
			/*bKillController=*/false, /*pFactory=*/nullptr, /*bIgnoreCollision=*/true,
			/*pOwnerPawn=*/nullptr, /*bIsDecoy=*/false, /*deviceFire=*/nullptr, 0.0f);
		if (Bot == nullptr) {
			Logger::Log(CH, "[Acq] SpawnBotById(%d) returned null for %s\n", botId, Obj->GetName());
			return;
		}
		Obj->Role               = 3;
		Obj->r_ObjectiveBot     = Bot;
		Obj->r_ObjectiveBotInfo = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
		Obj->bNetDirty          = 1;
		Obj->bForceNetUpdate    = 1;
		const int tf = Obj->m_bSpawnUnalignedBot ? 0 : Obj->nDefaultOwnerTaskForce;
		if (tf) ApplyTaskForce(Bot, tf);
		Logger::Log(CH, "[Acq] robot spawned obj=%s bot=%d tf=%d at (%.0f,%.0f,%.0f) pawn=%p\n",
			Obj->GetName(), botId, tf, loc.X, loc.Y, loc.Z, Bot);
	}
	// Binary half: SetTaskForceNumber(unaligned ? 0 : nDefaultOwnerTaskForce)
	// + marks a TgPawn_CTR robot.
	if (s_binSpawnSetup) s_binSpawnSetup(Obj, edx);
	if (ATgPawn* Bot = (ATgPawn*)Obj->r_ObjectiveBot) {
		Bot->s_bIsCrewable = 1;
		SpawnUseProxy(Bot);
		for (ObjState& s : s_objs) if (s.obj == Obj) { s.bot = Bot; s.proxy = Bot->s_UseProxy; }
	}
}

void __fastcall AcqSetObjectiveActive(ATgBaseObjective_CTFBot* Obj, void* edx, unsigned long bActive) {
	if (Obj == nullptr) return;
	const bool changed = Obj->r_bIsActive != (bActive ? 1u : 0u);
	Obj->r_bIsActive     = bActive ? 1 : 0;
	Obj->bEnabled        = bActive ? 1 : 0;
	Obj->bNetDirty       = 1;
	Obj->bForceNetUpdate = 1;
	if (changed) Obj->OnActiveStateChanged();
	Logger::Log(CH, "[Acq] SetObjectiveActive %s active=%d bot=%p\n", Obj->GetName(), (int)bActive, Obj->r_ObjectiveBot);
	if (bActive) {
		if (!BotAlive((ATgPawn*)Obj->r_ObjectiveBot)) Obj->eventRequestFlagSpawn();
	} else if (Obj->r_ObjectiveBot != nullptr && s_binRemove) {
		s_binRemove(Obj, nullptr);
	}
}

void __fastcall AcqFlagPickedUp(ATgBaseObjective_CTFBot* Obj, void* edx, int tf) {
	Logger::Log(CH, "[Acq] robot picked up obj=%s by tf=%d\n", Obj ? Obj->GetName() : "?", tf);
	AnnounceTeams(tf, MSG_YOU_HAVE, MSG_ENEMY_HAS);
}

void __fastcall AcqFlagCaptured(ATgBaseObjective_CTFBot* Obj, void* edx, int tf) {
	ATgGame* Game = GameNow();
	ATgRepInfo_TaskForce* team = TfPtr(tf);
	if (team) {
		team->r_nCurrentPointCount += 1;
		team->Score = (float)team->r_nCurrentPointCount;
		team->bNetDirty = 1;
		team->bForceNetUpdate = 1;
	}
	ATgRepInfo_Game* GRI = Game ? (ATgRepInfo_Game*)Game->GameReplicationInfo : nullptr;
	Logger::Log(CH, "[Acq] robot captured obj=%s by tf=%d -> score att=%d def=%d (to win %d)\n",
		Obj ? Obj->GetName() : "?", tf,
		GTeamsData.Attackers ? GTeamsData.Attackers->r_nCurrentPointCount : -1,
		GTeamsData.Defenders ? GTeamsData.Defenders->r_nCurrentPointCount : -1,
		GRI ? GRI->r_nPointsToWin : -1);
	AnnounceTeams(tf, MSG_YOU_CAPTURED, MSG_ENEMY_CAPTURED);
	if (Game && IsDualCTF((UObject*)Game))
		TgGame_Ticket__UpdateGameWinState::Call((ATgGame_Ticket*)Game, nullptr);
}

void __fastcall AcqFlagSpawned(ATgBaseObjective_CTFBot* Obj, void* edx) {
	Logger::Log(CH, "[Acq] robot available obj=%s\n", Obj ? Obj->GetName() : "?");
	const float t = Now();
	if (t - s_lastSpawnMsg > 5.0f) {   // both robots appear together at round start
		s_lastSpawnMsg = t;
		SendAlert::Broadcast(MSG_AVAILABLE, APT_HIGH, ATT_IMPORTANT, 3.0f);
	}
}

void __fastcall AcqFlagConsumed(ATgBaseObjective_CTFBot* Obj, void* edx) {
	Logger::Log(CH, "[Acq] robot destroyed obj=%s\n", Obj ? Obj->GetName() : "?");
	SendAlert::Broadcast(MSG_DESTROYED, APT_HIGH, ATT_REGULAR, 3.0f);
}

void PatchCtfBotVtable(ATgMissionObjective* Obj) {
	void** vt = *(void***)Obj;
	if (vt == s_patchedVt || s_patchFailed) return;
	void* binSpawn = nullptr; void* binRemove = (void*)kRemove.expect;
	if ((uintptr_t)*(void**)((char*)vt + kRemove.off) != kRemove.expect) {
		Logger::Log(CH, "[Acq] CTFBot vtable %p unexpected RemoveFlag slot - mode disabled\n", vt);
		s_patchFailed = true; return;
	}
	bool ok = PatchSlot(vt, kSpawn, (void*)&AcqSpawnObjectiveBot, &binSpawn)
	       && PatchSlot(vt, kSetActive, (void*)&AcqSetObjectiveActive, nullptr)
	       && PatchSlot(vt, kPicked, (void*)&AcqFlagPickedUp, nullptr)
	       && PatchSlot(vt, kCaptured, (void*)&AcqFlagCaptured, nullptr)
	       && PatchSlot(vt, kSpawned, (void*)&AcqFlagSpawned, nullptr)
	       && PatchSlot(vt, kConsumed, (void*)&AcqFlagConsumed, nullptr);
	if (!ok) { s_patchFailed = true; return; }
	if (binSpawn) s_binSpawnSetup = (ObjVoid_t)binSpawn;
	s_binRemove = (ObjVoid_t)binRemove;
	s_patchedVt = vt;
	Logger::Log(CH, "[Acq] CTFBot vtable %p patched (spawn/active/flag messages)\n", vt);
}

void ResetIfNewWorld() {
	ATgGame* G = GameNow();
	void* w = G ? (void*)G->WorldInfo : nullptr;
	if (w == s_world) return;
	s_world = w; s_objs.clear(); s_volumesDone = false; s_lastTick = -1.0f; s_lastSpawnMsg = -100.0f;
}

// Capture-volume team: map_object_config s_n_task_force wins; else the team
// whose robot is NOT parked next to the platform (1.4: "pick up their robot
// from the opposing team's base and bring it back to theirs").
void SetupCaptureVolumes() {
	if (s_objs.empty()) return;
	for (UObject* o : ObjectCache::FindAllByClass("Class TgGame.TgFlagCaptureVolume")) {
		ATgFlagCaptureVolume* V = (ATgFlagCaptureVolume*)o;
		if (V == nullptr || V->bDeleteMe) continue;
		int nearTf = 0; float best = 1e30f;
		for (const ObjState& s : s_objs) {
			const FVector& a = s.obj->Location; const FVector& b = V->Location;
			const float d = (a.X - b.X) * (a.X - b.X) + (a.Y - b.Y) * (a.Y - b.Y) + (a.Z - b.Z) * (a.Z - b.Z);
			if (d < best) { best = d; nearTf = s.obj->nDefaultOwnerTaskForce; }
		}
		const int derived = nearTf == 1 ? 2 : (nearTf == 2 ? 1 : 0);
		const int tf = MapObjectConfig::GetInt(V->m_nMapObjectId, "s_n_task_force", derived);
		V->r_nTaskForce = (unsigned char)tf;
		V->r_eCoalition = 0;
		V->bNetDirty = 1; V->bForceNetUpdate = 1;
		Logger::Log(CH, "[Acq] capture volume mid=%d at (%.0f,%.0f,%.0f) tf=%d (derived %d, nearest robot %.0f uu)\n",
			V->m_nMapObjectId, V->Location.X, V->Location.Y, V->Location.Z, tf, derived, std::sqrt(best));
	}
	s_volumesDone = true;
}

}  // namespace

namespace Acquisition {

void OnRegisterObjective(ATgMissionObjective* Obj) {
	if (Obj == nullptr || !ObjectClassCache::ClassNameContains((UObject*)Obj, "TgBaseObjective_CTFBot")) return;
	ResetIfNewWorld();
	if (!IsDualCTF((UObject*)GameNow())) return;   // CTR rotation keeps its own handling
	PatchCtfBotVtable(Obj);
	for (const ObjState& s : s_objs) if (s.obj == (ATgBaseObjective_CTFBot*)Obj) return;
	s_objs.push_back({(ATgBaseObjective_CTFBot*)Obj, -1.0f, nullptr, nullptr});
	Logger::Log(CH, "[Acq] CTFBot registered %s owner tf=%d prio=%d botId=%d at (%.0f,%.0f,%.0f)\n",
		Obj->GetName(), Obj->nDefaultOwnerTaskForce, Obj->nPriority, ((ATgBaseObjective_CTFBot*)Obj)->s_nBotId,
		Obj->Location.X, Obj->Location.Y, Obj->Location.Z);
}

void OnLoadGameConfig(ATgGame* Game) {
	if (!IsDualCTF((UObject*)Game)) return;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI) GRI->r_nPointsToWin = kCapsToWin;
	Logger::Log(CH, "[Acq] DualCTF config: caps to win %d, mission %.0f s\n", kCapsToWin, Game->m_fGameMissionTime);
}

bool OnTickTickets(ATgGame_Ticket* Game) {
	if (!IsDualCTF((UObject*)Game)) return false;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI && GRI->r_nPointsToWin != kCapsToWin) GRI->r_nPointsToWin = kCapsToWin;
	TgGame_Ticket__UpdateGameWinState::Call(Game, nullptr);
	return true;
}

void Tick() {
	ATgGame* Game = GameNow();
	if (!IsDualCTF((UObject*)Game) || s_patchedVt == nullptr) return;
	ResetIfNewWorld();
	const float t = Now();
	if (s_lastTick >= 0.0f && t - s_lastTick < 1.0f) return;
	s_lastTick = t;
	if (!s_volumesDone) SetupCaptureVolumes();
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr || Game->bGameEnded) return;
	if (GRI->r_nPointsToWin != kCapsToWin) GRI->r_nPointsToWin = kCapsToWin;
	// Robots go live when the match timer runs (after setup), unless the
	// priority unlock chain already activated them.
	if (GRI->r_nMissionTimerState != 1) return;
	for (ObjState& s : s_objs) {
		ATgBaseObjective_CTFBot* O = s.obj;
		if (O == nullptr || O->bDeleteMe) continue;
		if (!O->r_bIsActive) { AcqSetObjectiveActive(O, nullptr, 1); continue; }
		if (BotAlive((ATgPawn*)O->r_ObjectiveBot)) { s.goneSince = -1.0f; continue; }
		// Robot gone: drop its use-proxy and the dangling objective pointer.
		if (s.proxy && !s.proxy->bDeleteMe) s.proxy->Destroy();
		s.proxy = nullptr; s.bot = nullptr;
		if (O->r_ObjectiveBot && ((AActor*)O->r_ObjectiveBot)->bDeleteMe) {
			O->r_ObjectiveBot = nullptr; O->r_ObjectiveBotInfo = nullptr; O->bNetDirty = 1;
		}
		if (s.goneSince < 0.0f) { s.goneSince = t; continue; }
		if (t - s.goneSince >= kRespawnNetDelay) {
			Logger::Log(CH, "[Acq] respawn net: %s robot gone %.0f s -> RequestFlagSpawn\n", O->GetName(), t - s.goneSince);
			s.goneSince = -1.0f;
			O->r_ObjectiveBot = nullptr;
			O->eventRequestFlagSpawn();
		}
	}
}

bool SkipLockout(ATgGame* Game) {
	if (!IsDualCTF((UObject*)Game)) return false;
	Logger::Log(CH, "[Acq] LockoutObjectives skipped (DualCTF)\n");
	return true;
}

bool DevTeleportCrewed(ATgPawn* Player, const FVector& Loc) {
	if (Player == nullptr || !Player->r_bIsCrewing) return false;
	for (const ObjState& s : s_objs) {
		ATgPawn* R = s.obj ? (ATgPawn*)s.obj->r_ObjectiveBot : nullptr;
		if (!BotAlive(R) || R->r_ControlPawn != Player) continue;
		// Land 120 uu short (+X) and sweep the rest with Move: a SetLocation
		// teleport alone fires no Touch, so the capture volume would never see it.
		FVector pre = Loc; pre.X -= 120.0f;
		const bool ok = R->SetLocation(pre);
		const bool mv = ok && R->Move(FVector(120.0f, 0.0f, 0.0f));
		Logger::Log(CH, "[Acq] dev teleport crewed robot %p ok=%d move=%d now (%.0f,%.0f,%.0f)\n", R, (int)ok, (int)mv, R->Location.X, R->Location.Y, R->Location.Z);
		return ok;
	}
	return false;
}

}  // namespace Acquisition
