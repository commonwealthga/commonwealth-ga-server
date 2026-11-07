#include "src/GameServer/Tutorial/TutorialBotRoster.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TutorialBotRoster {

namespace {

constexpr const char* kCh = "tutbots";

struct Seen {
	int botId;
	int factory;
	float born;
	FVector first;
	FVector last;    // at the previous roster line
	FVector now;
	float maxDist;   // farthest from the spawn spot
};

std::unordered_map<int, Seen> g_bots;  // pawn id -> state
float g_acc = 0.0f;
float g_rosterAcc = 0.0f;

float Dist(const FVector& a, const FVector& b) {
	const float x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
	return sqrtf(x * x + y * y + z * z);
}

int BehaviorOf(int botId) {
	const auto it = CAmBot__LoadBotMarshal::m_BotBehaviorIds.find((uint32_t)botId);
	return it == CAmBot__LoadBotMarshal::m_BotBehaviorIds.end() ? -1 : (int)it->second;
}

AWorldInfo* World() {
	if (Globals::Get().GWorld == nullptr) return nullptr;
	return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}


// Touch events whose output chain (BFS, <= 10 deep) reaches one of these ops:
// alarm tripwires, the Skylark suit-up, chapter end. Object names lose their
// _N suffix in this SDK, so the trigger is reported by what it leads to.
void DumpTriggers() {
	static const char* kTargets[] = { "TgSeqAct_AlarmBots", "TgSeqAct_SetPlayerMesh", "TgSeqAct_RequestMission",
		"TgSeqAct_ActivateObjective", "TgSeqAct_SetPlayerLevel" };
	TArray<UObject*>* objs = UObject::GObjObjects();
	for (int i = 0; objs && i < objs->Count; i++) {
		UObject* o = objs->Data[i];
		if (o == nullptr || o->Class == nullptr) continue;
		if (ObjectClassCache::ClassNameContains(o, "TgMissionObjective")) {
			const std::string n = o->GetName();
			if (n.rfind("Default__", 0) == 0) continue;
			const AActor* a = (const AActor*)o;
			Logger::Log(kCh, "[TutBots] objective class=%s loc=(%.0f,%.0f,%.0f)\n", o->Class->GetName(),
				a->Location.X, a->Location.Y, a->Location.Z);
			continue;
		}
		if (!ObjectClassCache::ClassNameContains(o, "SeqEvent_Touch")) continue;
		USeqEvent_Touch* ev = (USeqEvent_Touch*)o;
		AActor* orig = ev->Originator;
		if (orig == nullptr) continue;
		std::vector<std::pair<USequenceOp*, int>> queue;
		std::unordered_set<USequenceOp*> seen;
		queue.push_back({ (USequenceOp*)ev, 0 });
		seen.insert((USequenceOp*)ev);
		for (size_t q = 0; q < queue.size() && q < 200; q++) {
			USequenceOp* op = queue[q].first;
			const int depth = queue[q].second;
			if (depth > 0) {
				for (const char* t : kTargets) {
					if (!ObjectClassCache::ClassNameContains((UObject*)op, t)) continue;
					FVector c = orig->Location;  // brush centre: a volume's origin may sit outside its brush
					if (ObjectClassCache::ClassNameContains((UObject*)orig, "Volume") && ((ABrush*)orig)->BrushComponent)
						c = ((ABrush*)orig)->BrushComponent->Bounds.Origin;
					Logger::Log(kCh, "[TutBots] touch reaches=%s depth=%d originator=%s enabled=%d trig=%d/%d loc=(%.0f,%.0f,%.0f) actor=(%.0f,%.0f,%.0f)\n",
						t, depth, orig->Class ? orig->Class->GetName() : "?", (int)ev->bEnabled, ev->TriggerCount, ev->MaxTriggerCount,
						c.X, c.Y, c.Z, orig->Location.X, orig->Location.Y, orig->Location.Z);
				}
			}
			if (depth >= 10) continue;
			for (int l = 0; l < op->OutputLinks.Count; l++) {
				const FSeqOpOutputLink& out = op->OutputLinks.Data[l];
				for (int k = 0; k < out.Links.Count; k++) {
					USequenceOp* nx = out.Links.Data[k].LinkedOp;
					if (nx == nullptr || seen.count(nx)) continue;
					seen.insert(nx);
					queue.push_back({ nx, depth + 1 });
				}
			}
		}
	}
}

}  // namespace

void Tick(float DeltaSeconds) {
	g_acc += DeltaSeconds;
	g_rosterAcc += DeltaSeconds;
	if (g_acc < 1.0f) return;
	g_acc = 0.0f;
	if (!Logger::IsChannelEnabled(kCh)) return;
	AWorldInfo* wi = World();
	if (wi == nullptr) return;
	const float now = wi->TimeSeconds;
	static bool s_volumesDumped = false;  // once: Kismet touch triggers + objectives (positions for dev tp)
	if (!s_volumesDumped && now > 5.0f) {
		s_volumesDumped = true;
		DumpTriggers();
	}
	const bool roster = g_rosterAcc >= 10.0f;
	if (roster) g_rosterAcc = 0.0f;

	std::unordered_set<int> alive;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		APawn* p = c->Pawn;
		if (p == nullptr || p->bDeleteMe) continue;
		if (ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) {
			if (roster)
				Logger::Log(kCh, "[TutBots] player hp=%d/%d loc=(%.0f,%.0f,%.0f)\n", p->Health,
					((ATgPawn*)p)->r_nHealthMaximum, p->Location.X, p->Location.Y, p->Location.Z);
			continue;
		}
		if (!ObjectClassCache::ClassNameContains((UObject*)c, "TgAIController")) continue;
		ATgAIController* ai = (ATgAIController*)c;
		ATgPawn* tp = (ATgPawn*)p;
		if (p->Health <= 0) continue;
		const int id = tp->r_nPawnId;
		alive.insert(id);
		const int factory = ai->m_pFactory ? ai->m_pFactory->m_nMapObjectId : 0;
		auto it = g_bots.find(id);
		if (it == g_bots.end()) {
			Seen s = { tp->r_nProfileId, factory, now, p->Location, p->Location, p->Location, 0.0f };
			g_bots[id] = s;
			Logger::Log(kCh, "[TutBots] spawn pawn=%d bot=%d beh=%d factory=%d alarm=%d hp=%d/%d loc=(%.0f,%.0f,%.0f) class=%s\n",
				id, tp->r_nProfileId, BehaviorOf(tp->r_nProfileId), factory, (int)ai->m_bAlarmBot, p->Health,
				tp->r_nHealthMaximum, p->Location.X, p->Location.Y, p->Location.Z, p->Class ? p->Class->GetName() : "?");
			continue;
		}
		Seen& s = it->second;
		s.now = p->Location;
		const float d = Dist(s.first, p->Location);
		if (d > s.maxDist) s.maxDist = d;
		if (roster) {
			const std::string st = ai->GetStateName().GetName();
			Logger::Log(kCh, "[TutBots] roster pawn=%d bot=%d factory=%d state=%s hp=%d/%d moved10s=%.0f fromSpawn=%.0f enemy=%d target=%d loc=(%.0f,%.0f,%.0f)\n",
				id, s.botId, factory, st.c_str(), p->Health, tp->r_nHealthMaximum, Dist(s.last, p->Location),
				d, ai->Enemy ? 1 : 0, tp->r_TargetActor ? 1 : 0, p->Location.X, p->Location.Y, p->Location.Z);
			s.last = p->Location;
		}
	}
	for (auto it = g_bots.begin(); it != g_bots.end();) {
		if (alive.count(it->first)) { ++it; continue; }
		const Seen& s = it->second;
		Logger::Log(kCh, "[TutBots] died pawn=%d bot=%d factory=%d lived=%.1fs maxFromSpawn=%.0f\n",
			it->first, s.botId, s.factory, now - s.born, s.maxDist);
		it = g_bots.erase(it);
	}
}

bool FireTouch(const std::string& op, int nth) {
	const std::string target = op.rfind("SeqAct_", 0) == 0 ? op : "TgSeqAct_" + op;  // full name (SeqAct_Toggle) allowed
	AWorldInfo* wi = World();
	APawn* player = nullptr;
	for (AController* c = wi ? wi->ControllerList : nullptr; c != nullptr; c = c->NextController)
		if (c->Pawn && ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) { player = c->Pawn; break; }
	if (player == nullptr) return false;
	TArray<UObject*>* objs = UObject::GObjObjects();
	int seenN = 0;
	for (int i = 0; objs && i < objs->Count; i++) {
		UObject* o = objs->Data[i];
		if (o == nullptr || o->Class == nullptr || !ObjectClassCache::ClassNameContains(o, "SeqEvent_Touch")) continue;
		USeqEvent_Touch* ev = (USeqEvent_Touch*)o;
		if (ev->Originator == nullptr) continue;
		std::vector<USequenceOp*> queue(1, (USequenceOp*)ev);
		std::unordered_set<USequenceOp*> seen(queue.begin(), queue.end());
		bool hit = false;
		for (size_t q = 0; q < queue.size() && q < 200 && !hit; q++) {
			USequenceOp* op2 = queue[q];
			if (q > 0 && ObjectClassCache::ClassNameContains((UObject*)op2, target.c_str())) { hit = true; break; }
			for (int l = 0; l < op2->OutputLinks.Count; l++)
				for (int k = 0; k < op2->OutputLinks.Data[l].Links.Count; k++) {
					USequenceOp* nx = op2->OutputLinks.Data[l].Links.Data[k].LinkedOp;
					if (nx && !seen.count(nx)) { seen.insert(nx); queue.push_back(nx); }
				}
		}
		if (!hit || ++seenN != nth) continue;
		bool ok = ev->CheckTouchActivate(ev->Originator, (AActor*)player, 0) != 0;
		const char* cls = (ev->ClassProximityTypes.Count > 0 && ev->ClassProximityTypes.Data[0]) ? ev->ClassProximityTypes.Data[0]->GetName() : "-";
		Logger::Log(kCh, "[TutBots] dev touch %s #%d -> CheckTouchActivate=%d (proximity classes=%d first=%s player=%s)\n", target.c_str(), nth,
			(int)ok, ev->ClassProximityTypes.Count, cls, player->Class ? player->Class->GetName() : "?");
		if (!ok) {  // the touch filter refused: activate the event itself
			TArray<int> none;
			ok = ev->CheckActivate(ev->Originator, (AActor*)player, 0, 0, none) != 0;
			Logger::Log(kCh, "[TutBots] dev touch %s #%d -> CheckActivate=%d\n", target.c_str(), nth, (int)ok);
		}
		return ok;
	}
	Logger::Log(kCh, "[TutBots] dev touch %s #%d: no such trigger\n", target.c_str(), nth);
	return false;
}

int KillBots(int botId, int factoryId) {
	AWorldInfo* wi = World();
	if (wi == nullptr) return 0;
	AController* killer = nullptr;
	std::vector<APawn*> victims;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		APawn* p = c->Pawn;
		if (p == nullptr || p->bDeleteMe || p->Health <= 0) continue;
		if (ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) {
			if (killer == nullptr) killer = c;
			continue;
		}
		if (!ObjectClassCache::ClassNameContains((UObject*)c, "TgAIController")) continue;
		ATgAIController* ai = (ATgAIController*)c;
		if (botId > 0 && ((ATgPawn*)p)->r_nProfileId != botId) continue;
		if (factoryId > 0 && (ai->m_pFactory == nullptr || ai->m_pFactory->m_nMapObjectId != factoryId)) continue;
		victims.push_back(p);
	}
	int n = 0;
	for (APawn* p : victims) {
		if (p->bDeleteMe || p->Health <= 0) continue;
		const int id = ((ATgPawn*)p)->r_nPawnId, bot = ((ATgPawn*)p)->r_nProfileId;
		p->Health = 0;
		p->Died(killer, nullptr, p->Location);
		Logger::Log(kCh, "[TutBots] dev kill pawn=%d bot=%d killer=%p\n", id, bot, (void*)killer);
		n++;
	}
	Logger::Log("revive", "[DEV] DEV_KILL bots bot=%d factory=%d -> %d killed\n", botId, factoryId, n);
	return n;
}

}  // namespace TutorialBotRoster
