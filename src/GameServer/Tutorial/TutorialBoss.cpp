#include "src/GameServer/Tutorial/TutorialBoss.hpp"

#include <string>
#include <unordered_set>
#include <vector>

#include "src/Config/Config.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"

namespace TutorialBoss {

namespace {

constexpr const char* kCh = "tutorial";
constexpr float kSafetyDelay = 20.0f;   // between safety-net checks
constexpr float kPostCine = 2.0f;       // first check: right after the cine ends
constexpr float kDefaultCine = 37.3f;   // Skylark InterpData_1 length (fallback)
constexpr float kNearArena = 1500.0f;   // boss counts as "in the arena" within this of Note_1

enum class Mapped { No, Yes, Skip };
Mapped s_state = Mapped::No;
float s_acc = 0.0f;

USeqEvent_Touch* s_identify = nullptr;   // TriggerVolume_28 touch (bot -> Instigator var)
USeqVar_Object* s_bossVar = nullptr;     // SeqVar_Object_115 (teleport Target)
USeqVar_Object* s_destVar = nullptr;     // teleport Destination (Note_1)
USeqEvent_Touch* s_cineTouch = nullptr;  // TriggerVolume_18 touch (player, boss cine)
USequenceOp* s_interp = nullptr;         // boss cine SeqAct_Interp (diagnostics)
USequenceOp* s_teleport = nullptr;       // the Kismet SeqAct_Teleport (diagnostics)
float s_cineLen = kDefaultCine;
float s_cineSeen = -1.0f;                // world time the cine touch was first seen fired
int s_netChecks = 0;                     // safety-net passes done (cine end + 2 s, then 3x 20 s apart)
float s_nextNet = 0.0f;
bool s_identLogged = false;

AWorldInfo* World() {
	if (Globals::Get().GWorld == nullptr) return nullptr;
	return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

float Dist(const FVector& a, const FVector& b) {
	const float x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
	return sqrtf(x * x + y * y + z * z);
}

bool LinkIs(const FSeqVarLink& l, const char* desc) {
	const wchar_t* w = l.LinkDesc.Data;
	if (w == nullptr) return false;
	for (; *desc; ++desc, ++w)
		if (*w != (wchar_t)*desc) return false;
	return *w == 0;
}

USeqVar_Object* FirstObjVar(USequenceOp* op, const char* desc) {
	for (int i = 0; i < op->VariableLinks.Count; i++) {
		const FSeqVarLink& l = op->VariableLinks.Data[i];
		if (!LinkIs(l, desc)) continue;
		for (int k = 0; k < l.LinkedVariables.Count; k++) {
			UObject* v = (UObject*)l.LinkedVariables.Data[k];
			if (v && ObjectClassCache::ClassNameContains(v, "SeqVar_Object")) return (USeqVar_Object*)v;
		}
	}
	return nullptr;
}

// BFS over output links (<= 12 deep); true if `target` is reached. Records the
// first SeqAct_Interp seen on the way (cine length).
bool Reaches(USequenceOp* from, USequenceOp* target, USequenceOp** interpOut) {
	std::vector<std::pair<USequenceOp*, int>> q(1, { from, 0 });
	std::unordered_set<USequenceOp*> seen{ from };
	for (size_t i = 0; i < q.size() && i < 300; i++) {
		USequenceOp* op = q[i].first;
		if (op == target) return true;
		if (interpOut && *interpOut == nullptr && ObjectClassCache::ClassNameContains((UObject*)op, "SeqAct_Interp"))
			*interpOut = op;
		if (q[i].second >= 12) continue;
		for (int l = 0; l < op->OutputLinks.Count; l++)
			for (int k = 0; k < op->OutputLinks.Data[l].Links.Count; k++) {
				USequenceOp* nx = op->OutputLinks.Data[l].Links.Data[k].LinkedOp;
				if (nx && !seen.count(nx)) { seen.insert(nx); q.push_back({ nx, q[i].second + 1 }); }
			}
	}
	return false;
}

void Map() {
	s_state = Mapped::Skip;
	TArray<UObject*>* objs = UObject::GObjObjects();
	std::vector<USeqEvent_Touch*> touches;
	std::vector<USequenceOp*> teleports;
	for (int i = 0; objs && i < objs->Count; i++) {
		UObject* o = objs->Data[i];
		if (o == nullptr || o->Class == nullptr) continue;
		const std::string n = o->GetName();
		if (n.rfind("Default__", 0) == 0) continue;
		if (ObjectClassCache::ClassNameContains(o, "SeqEvent_Touch")) touches.push_back((USeqEvent_Touch*)o);
		else if (ObjectClassCache::ClassNameContains(o, "SeqAct_Teleport")) teleports.push_back((USequenceOp*)o);
	}
	for (USequenceOp* tp : teleports) {
		USeqVar_Object* target = FirstObjVar(tp, "Target");
		USeqVar_Object* dest = FirstObjVar(tp, "Destination");
		if (target == nullptr || dest == nullptr) continue;
		for (USeqEvent_Touch* ev : touches) {
			if (ev->bHumanOnly || ev->bClientSideOnly || ev->Originator == nullptr) continue;
			if (FirstObjVar((USequenceOp*)ev, "Instigator") != target) continue;
			for (USeqEvent_Touch* cine : touches) {
				if (!cine->bHumanOnly || cine->bClientSideOnly) continue;
				USequenceOp* interp = nullptr;
				if (!Reaches((USequenceOp*)cine, tp, &interp)) continue;
				s_identify = ev; s_bossVar = target; s_destVar = dest; s_cineTouch = cine; s_interp = interp; s_teleport = tp;
				if (interp) {
					for (int v = 0; v < interp->VariableLinks.Count; v++) {
						const FSeqVarLink& l = interp->VariableLinks.Data[v];
						if (!LinkIs(l, "Data") || l.LinkedVariables.Count == 0 || l.LinkedVariables.Data[0] == nullptr) continue;
						const float len = ((UInterpData*)l.LinkedVariables.Data[0])->InterpLength;
						if (len > 1.0f && len < 300.0f) s_cineLen = len;
					}
				}
				s_state = Mapped::Yes;
				AActor* d = (AActor*)dest->ObjValue;
				Logger::Log(kCh, "[Tutorial] boss Kismet mapped: identify volume=(%.0f,%.0f,%.0f) dest=(%.0f,%.0f,%.0f) cine=%.1fs\n",
					ev->Originator->Location.X, ev->Originator->Location.Y, ev->Originator->Location.Z,
					d ? d->Location.X : 0.f, d ? d->Location.Y : 0.f, d ? d->Location.Z : 0.f, s_cineLen);
				return;
			}
		}
	}
	Logger::Log(kCh, "[Tutorial] boss Kismet: no identify/teleport chain on this map\n");
}

bool Inside(AActor* vol, const FVector& p, float margin) {
	FVector c = vol->Location, e = { 64.f, 64.f, 64.f };
	if (ObjectClassCache::ClassNameContains((UObject*)vol, "Volume") && ((ABrush*)vol)->BrushComponent) {
		c = ((ABrush*)vol)->BrushComponent->Bounds.Origin;
		e = ((ABrush*)vol)->BrushComponent->Bounds.BoxExtent;
	}
	return fabsf(p.X - c.X) <= e.X + margin && fabsf(p.Y - c.Y) <= e.Y + margin && fabsf(p.Z - c.Z) <= e.Z + margin;
}

APawn* AliveBoss() {
	UObject* v = s_bossVar ? s_bossVar->ObjValue : nullptr;
	if (v && ObjectClassCache::ClassNameContains(v, "Pawn")) {
		APawn* p = (APawn*)v;
		if (!p->bDeleteMe && p->Health > 0) return p;
	}
	return nullptr;
}

ATgMissionObjective_Bot* Objective() {
	TArray<UObject*>* objs = UObject::GObjObjects();
	for (int i = 0; objs && i < objs->Count; i++) {
		UObject* o = objs->Data[i];
		if (o == nullptr || o->Class == nullptr || !ObjectClassCache::ClassNameContains(o, "TgMissionObjective_Bot")) continue;
		if (std::string(o->GetName()).rfind("Default__", 0) == 0) continue;
		return (ATgMissionObjective_Bot*)o;
	}
	return nullptr;
}

void Report(const char* what, int botId, const FVector& at) {
	Logger::Log(kCh, "[Tutorial] boss safety net: %s bot=%d at=(%.0f,%.0f,%.0f)\n", what, botId, at.X, at.Y, at.Z);
	std::string guid;
	for (const auto& kv : GPawnSessions) { guid = kv.second; break; }
	nlohmann::json ev;
	ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]      = "tutorial_boss";
	ev["instance_id"]  = IpcClient::GetInstanceId();
	ev["session_guid"] = guid;
	ev["what"]         = what;
	ev["bot_id"]       = botId;
	IpcClient::Send(ev.dump());
}

void Identify(AWorldInfo* wi) {
	if (AliveBoss() != nullptr) return;
	AActor* vol = s_identify->Originator;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		APawn* p = c->Pawn;
		if (p == nullptr || p->bDeleteMe || p->Health <= 0) continue;
		if (!ObjectClassCache::ClassNameContains((UObject*)c, "TgAIController")) continue;
		if (!Inside(vol, p->Location, 150.0f)) continue;
		const int ok = s_identify->CheckTouchActivate(vol, (AActor*)p, 0) ? 1 : 0;
		if (s_bossVar->ObjValue != (UObject*)p) s_bossVar->ObjValue = (UObject*)p;  // filter refused a bot: set what the touch writes
		if (!s_identLogged) {
			s_identLogged = true;
			Logger::Log(kCh, "[Tutorial] boss identified (lost spawn touch re-run): bot=%d pawn=%d touch=%d loc=(%.0f,%.0f,%.0f)\n",
				((ATgPawn*)p)->r_nProfileId, ((ATgPawn*)p)->r_nPawnId, ok, p->Location.X, p->Location.Y, p->Location.Z);
		}
		return;
	}
}

void SafetyNet(float now) {
	AActor* dest = s_destVar ? (AActor*)s_destVar->ObjValue : nullptr;
	if (dest == nullptr) return;
	FVector to = dest->Location;
	APawn* boss = AliveBoss();
	ATgMissionObjective_Bot* obj = Objective();
	if (boss == nullptr && obj && obj->r_ObjectiveBot && !obj->r_ObjectiveBot->bDeleteMe && obj->r_ObjectiveBot->Health > 0)
		boss = obj->r_ObjectiveBot;
	if (s_netChecks == 1) {  // why the Kismet teleport did / did not run (seen live: cine played, no teleport)
		const USeqAct_Interp* ip = (const USeqAct_Interp*)s_interp;
		Logger::Log(kCh, "[Tutorial] boss post-cine: interp playing=%d pos=%.1f activations=%d teleport activations=%d boss=%s\n",
			ip ? (int)ip->bIsPlaying : -1, ip ? ip->Position : -1.f, s_interp ? s_interp->ActivateCount : -1,
			s_teleport ? s_teleport->ActivateCount : -1, boss ? "alive" : "none");
	}
	if (boss != nullptr) {
		if (Dist(boss->Location, to) <= kNearArena) {
			if (s_netChecks == 1) Logger::Log(kCh, "[Tutorial] boss safety net: boss in the arena (ok) bot=%d\n", ((ATgPawn*)boss)->r_nProfileId);
			return;
		}
		to.Z += 60.0f;
		const bool moved = boss->SetLocation(to) != 0;
		Report(moved ? "teleported (Kismet teleport missed)" : "teleport FAILED", ((ATgPawn*)boss)->r_nProfileId, boss->Location);
		s_bossVar->ObjValue = (UObject*)boss;
		return;
	}
	if (obj == nullptr || obj->r_eStatus == 8) return;  // objective done: the chapter ends through Kismet
	obj->r_ObjectiveBot = nullptr;
	obj->r_ObjectiveBotInfo = nullptr;
	TgMissionObjective_Bot__SpawnObjectiveBot::Call(obj, nullptr);
	APawn* nb = obj->r_ObjectiveBot;
	if (nb == nullptr) { Report("respawn FAILED", obj->s_nBotId, to); return; }
	to.Z += 60.0f;
	nb->SetLocation(to);
	s_bossVar->ObjValue = (UObject*)nb;
	Report("respawned from the objective's spawn table", ((ATgPawn*)nb)->r_nProfileId, nb->Location);
	(void)now;
}

}  // namespace

void Tick(float DeltaSeconds) {
	if (s_state == Mapped::Skip) return;
	s_acc += DeltaSeconds;
	if (s_acc < 1.0f) return;
	s_acc = 0.0f;
	if (std::string(Config::GetMapNameChar()) != "Skylark_P") { s_state = Mapped::Skip; return; }
	AWorldInfo* wi = World();
	if (wi == nullptr) return;
	const float now = wi->TimeSeconds;
	if (now < 3.0f) return;  // Kismet registers its events at level begin play
	if (s_state == Mapped::No) { Map(); if (s_state != Mapped::Yes) return; }
	if (s_cineSeen < 0.0f) {
		Identify(wi);
		if (s_cineTouch->TriggerCount > 0) {
			s_cineSeen = now;
			s_nextNet = now + s_cineLen + kPostCine;
			Logger::Log(kCh, "[Tutorial] boss cine started (t=%.0f) - safety net at t=%.0f, boss %s\n", now, s_nextNet,
				AliveBoss() ? "identified" : "NOT identified");
		}
		return;
	}
	if (s_netChecks < 4 && now >= s_nextNet) {
		s_netChecks++;
		s_nextNet = now + kSafetyDelay;
		SafetyNet(now);
	}
}

}  // namespace TutorialBoss
