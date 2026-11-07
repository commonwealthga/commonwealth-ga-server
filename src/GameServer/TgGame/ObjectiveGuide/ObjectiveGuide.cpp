#include "src/GameServer/TgGame/ObjectiveGuide/ObjectiveGuide.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Engine/ActorChannel/ReplicateActor/ActorChannel__ReplicateActor.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace {
	const float kRepeatSec = 30.0f;
	const float kFirstDelaySec = 5.0f;
	const float kNearSq = 600.0f * 600.0f;
	// Route graph: a nav point's own reach specs are the
	// walkable links (ProscribedReachSpec skipped); any two nav points can also
	// be a jump/drop/jetpack hop, costed x kGapMul plus kUpMul per unit climbed
	// plus kFarMul per unit beyond kGap (desert maps have 2-9k gaps with no
	// nodes at all - a hard 1200 u cut-off left them unroutable and fell
	// back to the straight-line pick = the old "turn around").
	const float kGap = 1200.0f;
	const float kGapMul = 2.0f;
	const float kUpMul = 2.0f;
	const float kFarMul = 4.0f;
	const float kAirMaxSec = 10.0f;   // airborne longer than this -> push anyway
	const int kMaxProbes = 8;
	const float kDetourMul = 1.25f;   // client route may be this much longer than the route (+kDetourAdd)
	const float kDetourAdd = 400.0f;
	const float kLevelZ = 300.0f;     // re-push when the player changed level by this much (drop on the same net)

	int s_enabled = -1; // -1 unknown, 0 off, 1 on
	AWorldInfo* s_world = nullptr;
	std::vector<ANavigationPoint*> s_nav;
	std::unordered_map<ANavigationPoint*, int> s_idx;
	std::vector<std::vector<std::pair<int, float>>> s_out; // reach-spec links per nav point
	std::unordered_map<AController*, float> s_next;
	std::unordered_map<AController*, float> s_airSince;
	std::unordered_map<AController*, int> s_pushedNet;
	std::unordered_map<AController*, FVector> s_pushedAt;
	AActor* s_lastTarget = nullptr;
	float s_lastPoll = -1.0f;
	// Comma redirect: Bot objective whose route target the viewer's client cannot
	// reach, and per viewer controller the location to show instead.
	ATgMissionObjective_Bot* s_redirObj = nullptr;
	std::unordered_map<AController*, FVector> s_redir;
	bool s_loggedFlags = false;
	std::unordered_map<AController*, std::vector<AActor*>> s_kismetHist;
	std::unordered_map<AController*, std::vector<int>> s_kismetNets;
	// Per route target node: next hop of every nav point toward it (-1 = none).
	std::unordered_map<int, std::vector<int>> s_routes;
	// And its route cost to that node (reworked-graph units).
	std::unordered_map<int, std::vector<float>> s_costs;
	// Probe: a hidden server-side TgNavRouteIndicator - the very actor the
	// client spawns for ClientShowPathTo - whose native SetRoute tells whether the
	// client's own route will work (1), fail = "Nav Error" msg 31313 (0) or say
	// "already there" (2). -1 = probe unavailable -> reach-spec graph check.
	AActor* s_probe = nullptr;
	UFunction* s_fnSetRoute = nullptr;
	int s_probeState = 0; // 0 untried, 1 ok, -1 broken
	int s_probeOk = 0, s_probeCalls = 0;

	float DistSq(const FVector& a, const FVector& b) {
		float x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
		return x * x + y * y + z * z;
	}

	bool Enabled() {
		if (s_enabled < 0) {
			ParsedOptions options = CommandLineParser::ParseCommandLine();
			std::wstring off = options.switches[L"noobjectiveguide"];
			bool disabled = off == L"1" || off == L"true";
			std::string map = Config::GetMapNameChar();
			bool mission = map.size() > 3 && map[0] == '1' && (map[1] == 'P' || map[1] == 'p') && map[2] == '_';
			s_enabled = (!disabled && mission) ? 1 : 0;
			Logger::Log("objguide", "[ObjectiveGuide] map=%s enabled=%d (0390+0391)\n", Config::GetMapNameChar().c_str(), s_enabled);
		}
		return s_enabled == 1;
	}

	// Nearest nav point; vertical distance counts double so a player on an upper
	// floor anchors to that floor, not to the node right under the floor.
	int NearestIdx(const FVector& at, int network) {
		int best = -1;
		float bestD = 0.0f;
		for (size_t i = 0; i < s_nav.size(); i++) {
			ANavigationPoint* n = s_nav[i];
			if (network >= 0 && n->NetworkID != network) continue;
			float x = at.X - n->Location.X, y = at.Y - n->Location.Y, z = (at.Z - n->Location.Z) * 2.0f;
			float d = x * x + y * y + z * z;
			if (best < 0 || d < bestD) { best = (int)i; bestD = d; }
		}
		return best;
	}

	AActor* ActiveTarget(ATgRepInfo_Game* GRI, ATgMissionObjective** outObj) {
		for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
			ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
			if (Obj == nullptr || !Obj->r_bIsActive) continue;
			AActor* a = Obj->eventGetLocationActor();
			*outObj = Obj;
			return a ? a : (AActor*)Obj;
		}
		return nullptr;
	}

	float HopCost(int u, int v) {
		const FVector& a = s_nav[u]->Location;
		const FVector& b = s_nav[v]->Location;
		for (const auto& e : s_out[u]) if (e.first == v) return e.second;
		float d = sqrtf(DistSq(a, b));
		float up = b.Z - a.Z;
		return d * kGapMul + (up > 0.0f ? up * kUpMul : 0.0f) + (d > kGap ? (d - kGap) * kFarMul : 0.0f);
	}

	// Reverse Dijkstra from the target node over all nav points (O(n^2), once
	// per target node): next hop of each node on its cheapest route there.
	const std::vector<int>& RouteTo(int goal) {
		auto found = s_routes.find(goal);
		if (found != s_routes.end()) return found->second;
		if (s_routes.size() >= 64) { s_routes.clear(); s_costs.clear(); } // boss pawn wandering over many nodes
		size_t n = s_nav.size();
		std::vector<float> dist(n, -1.0f);
		std::vector<int> next(n, -1);
		std::vector<char> done(n, 0);
		dist[goal] = 0.0f;
		for (;;) {
			int v = -1;
			for (size_t i = 0; i < n; i++)
				if (!done[i] && dist[i] >= 0.0f && (v < 0 || dist[i] < dist[v])) v = (int)i;
			if (v < 0) break;
			done[v] = 1;
			for (size_t u = 0; u < n; u++) {
				if (done[u]) continue;
				float c = HopCost((int)u, v);
				if (dist[u] < 0.0f || dist[v] + c < dist[u]) { dist[u] = dist[v] + c; next[u] = v; }
			}
		}
		s_costs[goal] = std::move(dist);
		return s_routes.emplace(goal, std::move(next)).first->second;
	}

	// Nav points reachable from `from` over reach specs only (what the client's
	// router can walk) - used when the probe is unavailable.
	std::vector<char> SpecReach(int from) {
		std::vector<char> seen(s_nav.size(), 0);
		std::vector<int> stack{from};
		seen[from] = 1;
		while (!stack.empty()) {
			int u = stack.back(); stack.pop_back();
			for (const auto& e : s_out[u]) if (!seen[e.first]) { seen[e.first] = 1; stack.push_back(e.first); }
		}
		return seen;
	}

	// Walking distance from `from` over reach specs only (-1 = not walkable).
	std::vector<float> WalkDist(int from) {
		size_t n = s_nav.size();
		std::vector<float> d(n, -1.0f);
		std::vector<char> done(n, 0);
		d[from] = 0.0f;
		for (;;) {
			int v = -1;
			for (size_t i = 0; i < n; i++)
				if (!done[i] && d[i] >= 0.0f && (v < 0 || d[i] < d[v])) v = (int)i;
			if (v < 0) break;
			done[v] = 1;
			for (const auto& e : s_out[v])
				if (!done[e.first] && (d[e.first] < 0.0f || d[v] + e.second < d[e.first])) d[e.first] = d[v] + e.second;
		}
		return d;
	}

	// 1 = the client's route from `from` to `dest` works, 0 = Nav Error,
	// 2 = already there, -1 = no probe (use the graph).
	int Probe(AController* C, const FVector& from, const FVector& dest, float* routeLen) {
		*routeLen = -1.0f;
		if (s_probeState < 0) return -1;
		if (s_probe == nullptr) {
			UClass* cls = (UClass*)ObjectCache::Find("Class TgGame.TgNavRouteIndicator");
			s_fnSetRoute = (UFunction*)ObjectCache::Find("Function TgGame.TgNavRouteIndicator.SetRoute");
			ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
			if (cls && s_fnSetRoute && Game)
				s_probe = Game->Spawn(cls, C, FName(), from, FRotator(), nullptr, 1);
			if (s_probe == nullptr) {
				s_probeState = -1;
				Logger::Log("objguide", "[ObjectiveGuide] probe unavailable (cls=%p fn=%p) - reach-spec graph only\n", cls, s_fnSetRoute);
				return -1;
			}
			s_probe->RemoteRole = 0; // ROLE_None: never replicated to anyone
			s_probe->bHidden = 1;
			Logger::Log("objguide", "[ObjectiveGuide] probe spawned %p\n", s_probe);
		}
		s_probe->Owner = C;
		s_probe->Location = from;
		ATgNavRouteIndicator_execSetRoute_Parms p;
		p.vDest = dest;
		p.ReturnValue = 0;
		s_probe->ProcessEvent(s_fnSetRoute, &p, nullptr);
		s_probeCalls++;
		if (p.ReturnValue == 1) {
			s_probeOk++; s_probeState = 1;
			// length of the client's arrow route = pawn -> waypoints
			TArray<FVector>& wp = ((ATgNavRouteIndicator*)s_probe)->WayPoints;
			float len = 0.0f;
			FVector prev = from;
			for (int i = 0; i < wp.Count; i++) { len += sqrtf(DistSq(prev, wp.Data[i])); prev = wp.Data[i]; }
			*routeLen = wp.Count > 0 ? len : -1.0f;
		}
		return p.ReturnValue;
	}

	void EnsureNav(AWorldInfo* WI) {
		if (WI == s_world) return;
		s_world = WI;
		s_nav.clear();
		s_idx.clear();
		s_out.clear();
		s_next.clear();
		s_airSince.clear();
		s_pushedNet.clear();
		s_pushedAt.clear();
		s_lastTarget = nullptr;
		s_redirObj = nullptr;
		s_redir.clear();
		s_kismetHist.clear();
		s_kismetNets.clear();
		s_routes.clear();
		s_costs.clear();
		s_probe = nullptr;
		s_probeState = 0;
		for (ANavigationPoint* n = WI->NavigationPointList; n; n = n->nextNavigationPoint) {
			s_idx[n] = (int)s_nav.size();
			s_nav.push_back(n);
		}
		s_out.resize(s_nav.size());
		int links = 0;
		for (size_t i = 0; i < s_nav.size(); i++) {
			ANavigationPoint* n = s_nav[i];
			for (int k = 0; k < n->PathList.Count; k++) {
				UReachSpec* r = n->PathList.Data[k];
				if (r == nullptr || r->End.Actor == nullptr) continue;
				if (ObjectClassCache::ClassNameContains((UObject*)r, "Proscribed")) continue;
				auto e = s_idx.find((ANavigationPoint*)r->End.Actor);
				if (e == s_idx.end() || e->second == (int)i) continue;
				s_out[i].push_back({e->second, sqrtf(DistSq(n->Location, r->End.Actor->Location))});
				links++;
			}
		}
		Logger::Log("objguide", "[ObjectiveGuide] %d nav points, %d reach links\n", (int)s_nav.size(), links);
	}

	bool Grounded(APawn* P) {
		// PHYS_Walking 1, PHYS_Spider 8, PHYS_Ladder 9; jetpack/fall = Falling/Flying
		return P->Physics == 1 || P->Physics == 8 || P->Physics == 9;
	}

	struct Pick {
		AActor* dest = nullptr;   // what to send (nullptr = send nothing)
		ANavigationPoint* exit = nullptr; // When nothing is sent, where the comma key points (the drop/jump edge)
		const char* kind = "none";
		int probe = -9;           // last probe result for dest
		int first = -9;           // probe result for the objective itself
		int tries = 0;
		float clen = -1.0f, want = 0.0f; // last good probe: client route length vs route length
		int mineNet = -1, targetNet = -1;
	};

	// The destination to send this controller so its client's route works and
	// leads toward `target`. Candidates lie on the cheapest route over the
	// reworked graph (reach specs + jump/drop/jetpack hops). A candidate is good
	// when the client can route to it (probe) AND that client route is not a big
	// detour (<= kDetourMul x route length + kDetourAdd): on 1P_SDDweller02 the
	// ledge east of the boss arena is a 450 u drop with no reach spec, so the
	// client's walk-only route there went 9-10k u back around ("turn around").
	// The objective itself when good, else the furthest good route node, else
	// nothing (< 600 u away = the player is at the drop/jump).
	Pick Choose(AController* C, AActor* target) {
		Pick pk;
		const FVector& pawnAt = C->Pawn->Location;
		int mine = NearestIdx(pawnAt, -1);
		int goal = NearestIdx(target->Location, -1);
		if (mine < 0 || goal < 0) return pk;
		pk.mineNet = s_nav[mine]->NetworkID;
		pk.targetNet = s_nav[goal]->NetworkID;
		const std::vector<int>& next = RouteTo(goal);
		std::vector<int> path;
		std::vector<float> plen;
		FVector prev = pawnAt;
		float acc = 0.0f;
		for (int at = mine; at >= 0 && path.size() <= s_nav.size(); at = next[at]) {
			acc += sqrtf(DistSq(prev, s_nav[at]->Location)); prev = s_nav[at]->Location;
			path.push_back(at); plen.push_back(acc);
			if (at == goal) break;
		}
		std::vector<char> reach;
		// 1 good, 0 not routable or detour, 2 already there
		auto good = [&](int node, const FVector& loc, float want) -> int {
			float clen;
			int r = Probe(C, pawnAt, loc, &clen);
			pk.tries++;
			if (r < 0) {
				if (reach.empty()) reach = SpecReach(mine);
				return reach[node] ? 1 : 0;
			}
			if (r != 1) return r;
			pk.clen = clen; pk.want = want;
			if (clen >= 0.0f && clen > want * kDetourMul + kDetourAdd) return 0;
			return 1;
		};
		float toTarget = plen.back() + sqrtf(DistSq(s_nav[goal]->Location, target->Location));
		int r = good(goal, target->Location, toTarget);
		pk.first = r;
		if (r == 2) { pk.kind = "already there"; pk.probe = 2; return pk; }
		if (r == 1) {
			pk.probe = 1;
			// A dynamic target (boss pawn) may not be net-relevant to a far viewer:
			// the RPC would arrive as None and the client draws nothing. Send its
			// nav point instead when that routes too.
			if (!target->bStatic && !target->bNoDelete && good(goal, s_nav[goal]->Location, plen.back()) == 1) {
				pk.dest = s_nav[goal]; pk.kind = "objective node";
			} else {
				pk.dest = target; pk.kind = "objective";
			}
			return pk;
		}
		// Furthest good node along the route (binary search).
		int lo = 1, hi = (int)path.size() - 2, best = -1;
		while (lo <= hi && pk.tries < kMaxProbes) {
			int mid = (lo + hi + 1) / 2;
			if (good(path[mid], s_nav[path[mid]]->Location, plen[mid]) == 1) { best = mid; lo = mid + 1; }
			else hi = mid - 1;
		}
		if (best <= 0) {
			// The route leaves the walkable area right here (start islands:
			// 1P_SDColony05). Walk to the reach-spec node from which the rest of the
			// trip is cheapest, if that whole trip stays within the detour rule.
			const std::vector<float>& cost = s_costs[goal];
			std::vector<float> wd = WalkDist(mine);
			std::vector<int> cand;
			for (size_t i = 0; i < s_nav.size(); i++)
				if (wd[i] > 0.0f && cost[i] >= 0.0f && cost[i] < cost[mine] && DistSq(pawnAt, s_nav[i]->Location) >= kNearSq
					&& wd[i] + cost[i] <= cost[mine] * kDetourMul + kDetourAdd) cand.push_back((int)i);
			std::sort(cand.begin(), cand.end(), [&](int a, int b) { return wd[a] + cost[a] < wd[b] + cost[b]; });
			for (size_t k = 0; k < cand.size() && k < 4; k++) {
				if (good(cand[k], s_nav[cand[k]]->Location, wd[cand[k]]) == 1) {
					pk.dest = s_nav[cand[k]]; pk.kind = "walk exit"; pk.probe = 1;
					return pk;
				}
			}
			pk.kind = "no client route"; pk.probe = 0; pk.exit = s_nav[mine];
			return pk;
		}
		ANavigationPoint* n = s_nav[path[best]];
		if (DistSq(pawnAt, n->Location) < kNearSq) { pk.kind = "at route exit"; pk.probe = 1; pk.exit = n; return pk; }
		pk.dest = n; pk.kind = "route waypoint"; pk.probe = 1;
		return pk;
	}

	void LogPick(const char* who, AController* C, const Pick& pk) {
		const FVector& at = C->Pawn->Location;
		if (pk.dest)
			Logger::Log("objguide", "[ObjectiveGuide] %s pc=%p at %.0f,%.0f,%.0f net %d -> %s %.0f,%.0f,%.0f (target net %d, objective probe=%d, probe=%d route %.0f/%.0f tries=%d probeOk=%d/%d)\n",
				who, C, at.X, at.Y, at.Z, pk.mineNet, pk.kind, pk.dest->Location.X, pk.dest->Location.Y, pk.dest->Location.Z,
				pk.targetNet, pk.first, pk.probe, pk.clen, pk.want, pk.tries, s_probeOk, s_probeCalls);
		else
			Logger::Log("objguide", "[ObjectiveGuide] %s pc=%p at %.0f,%.0f,%.0f net %d -> nothing (%s, target net %d, objective probe=%d, tries=%d probeOk=%d/%d, comma exit %.0f,%.0f,%.0f)\n",
				who, C, at.X, at.Y, at.Z, pk.mineNet, pk.kind, pk.targetNet, pk.first, pk.tries, s_probeOk, s_probeCalls,
				pk.exit ? pk.exit->Location.X : 0.0f, pk.exit ? pk.exit->Location.Y : 0.0f, pk.exit ? pk.exit->Location.Z : 0.0f);
	}

	void Guide(ATgPlayerController* PC, AActor* target, bool isBotObj) {
		Pick pk = Choose(PC, target);
		LogPick("push", PC, pk);
		s_pushedNet[PC] = pk.mineNet;
		s_pushedAt[PC] = PC->Pawn->Location;
		if (isBotObj) {
			// comma key: show the same waypoint while the boss itself is not routable
			// When nothing is sent, point the comma key at the drop/jump edge
			// (client: short trail or "already there") - never the real boss, whose
			// walk-only client route is the long way back = "turn around".
			if (pk.dest && pk.dest != target && strcmp(pk.kind, "objective node") != 0) s_redir[PC] = pk.dest->Location;
			else if (pk.dest == nullptr && pk.exit) s_redir[PC] = pk.exit->Location;
			else s_redir.erase(PC);
		}
		if (pk.dest) PC->ClientShowPathTo(pk.dest);
	}
}

void ObjectiveGuide::Tick(float DeltaSeconds) {
	if (Globals::Get().GWorld == nullptr || !Enabled()) return;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (Game == nullptr) return;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (GRI == nullptr || WI == nullptr) return;

	float now = WI->TimeSeconds;
	if (s_lastPoll >= 0.0f && now - s_lastPoll < 1.0f && WI == s_world) return;
	s_lastPoll = now;

	EnsureNav(WI);
	if (s_nav.empty()) return;

	ATgMissionObjective* obj = nullptr;
	AActor* target = ActiveTarget(GRI, &obj);
	if (target == nullptr) { s_redir.clear(); s_redirObj = nullptr; return; }
	bool changed = target != s_lastTarget;
	s_lastTarget = target;
	if (changed) { s_redir.clear(); s_redirObj = nullptr; }
	bool isBotObj = ObjectClassCache::ClassNameContains((UObject*)obj, "TgMissionObjective_Bot");
	if (isBotObj) s_redirObj = (ATgMissionObjective_Bot*)obj;
	if (s_redirObj) {
		ATgMissionObjective_Bot* Obj = s_redirObj;
		// TgMeshAssembly is bOnlyDirtyReplication: without a dirty flag the
		// engine never calls ReplicateActor for it, so the per-viewer values
		// below would never be sent. 1 Hz, one actor.
		Obj->bNetDirty = 1;
		if (!s_loggedFlags) {
			s_loggedFlags = true;
			Logger::Log("objguide", "[ObjectiveGuide] comma redirect on obj %d: RemoteRole=%d repMove=%d updSim=%d onlyDirty=%d\n",
				Obj->nObjectiveId, (int)Obj->RemoteRole, (int)Obj->bReplicateMovement, (int)Obj->bUpdateSimulatedPosition,
				(int)Obj->bOnlyDirtyReplication);
		}
	}

	for (AController* C = WI->ControllerList; C; C = C->NextController) {
		if (!ObjectClassCache::ClassNameContains((UObject*)C, "TgPlayerController")) continue;
		auto it = s_next.find(C);
		if (it == s_next.end()) { s_next[C] = now + kFirstDelaySec; continue; }
		if (C->Pawn == nullptr || C->Pawn->Health <= 0) { s_airSince.erase(C); continue; }
		// Also push at once when the player reached another path network
		// (dropped/jetpacked to the next level), not 30 s later.
		bool netChanged = false;
		auto pn = s_pushedNet.find(C);
		if (pn != s_pushedNet.end()) {
			int m = NearestIdx(C->Pawn->Location, -1);
			netChanged = m >= 0 && s_nav[m]->NetworkID != pn->second;
			auto pa = s_pushedAt.find(C);
			if (pa != s_pushedAt.end() && fabsf(C->Pawn->Location.Z - pa->second.Z) > kLevelZ) netChanged = true;
		}
		if (!changed && !netChanged && now < it->second) continue;
		// Never while jetpacking/falling - the client's router has no
		// anchor in the air and answers "Nav Error". Wait for the landing (max kAirMaxSec).
		if (!Grounded(C->Pawn)) {
			auto as = s_airSince.find(C);
			if (as == s_airSince.end()) { s_airSince[C] = now; continue; }
			if (now - as->second < kAirMaxSec) continue;
		}
		s_airSince.erase(C);
		it->second = now + kRepeatSec;
		Guide((ATgPlayerController*)C, target, isBotObj);
	}
}

bool ObjectiveGuide::ReplicateObjective(AActor* Actor, ATgPawn* Viewer, void* Channel, void* edx) {
	if (s_enabled != 1 || s_redirObj == nullptr || Actor != (AActor*)s_redirObj) return false;
	ATgMissionObjective_Bot* Obj = s_redirObj;
	// Location only replicates for simulated/dumb proxies; never change the role itself.
	if (Obj->RemoteRole != 1 && Obj->RemoteRole != 2) return false;

	FVector realLoc = Obj->Location;
	ATgPawn* realBot = Obj->r_ObjectiveBot;
	unsigned long repMove = Obj->bReplicateMovement, updSim = Obj->bUpdateSimulatedPosition;
	auto it = Viewer && Viewer->Controller ? s_redir.find(Viewer->Controller) : s_redir.end();
	bool redirect = it != s_redir.end();
	// Always force movement replication for this objective so a viewer that got
	// the edge location gets the real one back once it reaches the boss's network.
	Obj->bReplicateMovement = 1;
	Obj->bUpdateSimulatedPosition = 1;
	Obj->bNetDirty = 1;
	if (redirect) {
		Obj->Location = it->second;
		Obj->r_ObjectiveBot = nullptr;
	}
	ActorChannel__ReplicateActor::CallOriginal(Channel, edx);
	Obj->Location = realLoc;
	Obj->r_ObjectiveBot = realBot;
	Obj->bReplicateMovement = repMove;
	Obj->bUpdateSimulatedPosition = updSim;
	return true;
}

// Kismet arrows (intro Inception_ALL): the map has two path networks, 13 = the
// start room and 12 = everything after the vault/jump, and "Round Start Room"
// (PathNode_98, net 13) refires every 30 s from a Delay loop. Once the agent
// is on net 12 every refire is unroutable -> "Nav Error" every 30 s. Same
// network -> unchanged. Other network: drop a refire of a destination that a
// later indicator already replaced (stale loop) or one on a network the agent
// has already left (it is behind them), else point at the node of
// the agent's network nearest the destination (drop when that node is close:
// the agent is at the gap and has to jump/jetpack).
AActor* ObjectiveGuide::KismetNavTarget(AController* C, AActor* dest) {
	if (dest == nullptr || C == nullptr || C->Pawn == nullptr || Globals::Get().GWorld == nullptr) return dest;
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WI == nullptr) return dest;
	EnsureNav(WI);
	if (s_nav.empty()) return dest;

	std::vector<AActor*>& hist = s_kismetHist[C];
	bool seen = std::find(hist.begin(), hist.end(), dest) != hist.end();
	bool stale = seen && hist.back() != dest;
	if (!seen) hist.push_back(dest);

	int mineI = NearestIdx(C->Pawn->Location, -1);
	int theirsI = NearestIdx(dest->Location, -1);
	if (mineI < 0 || theirsI < 0) return dest;
	ANavigationPoint* mine = s_nav[mineI];
	ANavigationPoint* theirs = s_nav[theirsI];
	std::vector<int>& nets = s_kismetNets[C];
	if (std::find(nets.begin(), nets.end(), mine->NetworkID) == nets.end()) nets.push_back(mine->NetworkID);
	bool behind = mine->NetworkID != theirs->NetworkID && std::find(nets.begin(), nets.end(), theirs->NetworkID) != nets.end();
	if ((stale && mine->NetworkID != theirs->NetworkID) || behind) {
		Logger::Log("objguide", "[KismetNav] drop %s: player net %d, destination net %d\n",
			stale ? "stale refire" : "destination behind", mine->NetworkID, theirs->NetworkID);
		return nullptr;
	}
	// Same check the arrows use - pass the destination through when the
	// client can route to it, else the furthest routable waypoint, else drop.
	Pick pk = Choose(C, dest);
	LogPick("kismet", C, pk);
	if (pk.dest == nullptr) return nullptr;
	if (strcmp(pk.kind, "objective node") == 0) return dest; // Kismet targets are map actors
	return pk.dest;
}
