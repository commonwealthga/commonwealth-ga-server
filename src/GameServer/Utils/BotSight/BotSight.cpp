#include "src/GameServer/Utils/BotSight/BotSight.hpp"

#include <unordered_map>
#include <vector>

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace BotSight {

namespace {

constexpr float kInterval = 0.5f;
constexpr float kWatchDistSq = 12000.0f * 12000.0f;  // a player this close may see the bot

float g_acc = 0.0f;
float g_logAcc = 0.0f;
unsigned g_nScans = 0, g_nAi = 0, g_nBots = 0, g_nIdle = 0, g_nRange = 0, g_nHostile = 0, g_nSeen = 0;

struct Bot {
	ATgAIController* ctrl;
	APawn* pawn;
	float radiusSq;
	int tf;
};

// opted-in pawn/controller -> factory task force, rebuilt every scan
std::unordered_map<const AActor*, int> g_side;
// opted-in controller -> its factory bot_sight_radius squared, same rebuild
std::unordered_map<const ATgAIController*, float> g_radiusSq;
unsigned g_nForcedLos = 0;

float DistSq(const FVector& a, const FVector& b) {
	const float x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
	return x * x + y * y + z * z;
}

}  // namespace

bool IsHostilePair(AActor* a, AActor* b) {
	if (a == nullptr || b == nullptr || g_side.empty()) return false;
	const auto ia = g_side.find(a);
	if (ia == g_side.end()) return false;
	const auto ib = g_side.find(b);
	return ib != g_side.end() && ia->second != ib->second;
}

bool TargetInRadius(ATgAIController* ctrl) {
	if (ctrl == nullptr || ctrl->Pawn == nullptr || g_radiusSq.empty()) return false;
	const auto ir = g_radiusSq.find(ctrl);
	if (ir == g_radiusSq.end()) return false;
	AActor* target = ((ATgPawn*)ctrl->Pawn)->r_TargetActor;
	if (target == nullptr || !IsHostilePair((AActor*)ctrl->Pawn, target)) return false;
	if (DistSq(target->Location, ctrl->Pawn->Location) >= ir->second) return false;
	++g_nForcedLos;
	return true;
}

void Tick(float DeltaSeconds) {
	g_acc += DeltaSeconds;
	g_logAcc += DeltaSeconds;
	if (g_logAcc >= 10.0f) {
		if (g_nScans > 0)
			Logger::Log("tgbotfactory", "[BotSight] 10s scans=%u ai=%u bots=%u idle=%u in_range=%u hostile=%u seen=%u forced_los=%u\n",
				g_nScans, g_nAi, g_nBots, g_nIdle, g_nRange, g_nHostile, g_nSeen, g_nForcedLos);
		g_logAcc = 0.0f; g_nScans = g_nAi = g_nBots = g_nIdle = g_nRange = g_nHostile = g_nSeen = g_nForcedLos = 0;
	}
	if (g_acc < kInterval) return;
	g_acc = 0.0f;
	g_side.clear();
	g_radiusSq.clear();
	if (Globals::Get().GWorld == nullptr) return;
	AWorldInfo* wi = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (wi == nullptr || wi->Game == nullptr) return;

	++g_nScans;
	std::vector<Bot> bots;
	std::vector<ATgPawn*> players;
	for (AController* c = wi->ControllerList; c != nullptr; c = c->NextController) {
		APawn* p = c->Pawn;
		if (p == nullptr || p->bDeleteMe || p->Health <= 0) continue;
		if (ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) { players.push_back((ATgPawn*)p); continue; }
		if (!ObjectClassCache::ClassNameContains((UObject*)c, "TgAIController")) continue;
		ATgAIController* ai = (ATgAIController*)c;
		++g_nAi;
		if (ai->m_pFactory == nullptr || ai->m_pFactory->m_nMapObjectId <= 0) continue;
		const int r = MapObjectConfig::GetInt(ai->m_pFactory->m_nMapObjectId, "bot_sight_radius", 0);
		const int tf = (int)ai->m_pFactory->s_nTaskForce;
		if (r <= 0 || tf <= 0) continue;
		bots.push_back({ai, p, (float)r * (float)r, tf});
		g_side[(const AActor*)p] = tf;
		g_side[(const AActor*)ai] = tf;
		g_radiusSq[ai] = (float)r * (float)r;
	}
	g_nBots += (unsigned)bots.size();
	if (bots.size() < 2 || players.empty()) return;

	for (const Bot& b : bots) {
		// A hostile agent in sight and closer than the bot this one is fighting
		// comes first, so opted-in bots never ignore a player next to them.
		AActor* cur = ((ATgPawn*)b.pawn)->r_TargetActor;
		bool onPlayer = false;
		for (ATgPawn* pl : players) if ((AActor*)pl == cur) { onPlayer = true; break; }
		if (onPlayer) continue;
		float curSq = (cur != nullptr) ? DistSq(cur->Location, b.pawn->Location) : b.radiusSq;
		if (curSq > b.radiusSq) curSq = b.radiusSq;
		ATgPawn* nearPlayer = nullptr;
		bool watched = false;
		for (ATgPawn* pl : players) {
			const float d = DistSq(pl->Location, b.pawn->Location);
			if (d < kWatchDistSq) watched = true;
			if (d >= curSq || pl->r_bIsStealthed || !b.ctrl->IsEnemy((AActor*)pl)) continue;
			if (!b.ctrl->LineOfSightTo((AActor*)pl, FVector(0, 0, 0), 0)) continue;
			nearPlayer = pl;
			curSq = d;
		}
		if (nearPlayer != nullptr) {
			b.ctrl->eventSeePlayer(nearPlayer);
			b.ctrl->SetTargetActor((AActor*)nearPlayer);
			b.ctrl->Enemy = nearPlayer;
			Logger::Log("tgbotfactory", "[BotSight] factory %d bot targets player at %.0f uu\n", b.ctrl->m_pFactory->m_nMapObjectId, sqrtf(curSq));
			continue;
		}
		// The AI's current target is TgPawn.r_TargetActor (native GetTargetPawn
		// reads it); Enemy alone is never used by the target pick.
		if (b.ctrl->Enemy != nullptr || cur != nullptr) continue;
		if (!watched) continue;
		++g_nIdle;
		const Bot* best = nullptr;
		float bestSq = b.radiusSq;
		for (const Bot& o : bots) {
			if (&o == &b) continue;
			const float d = DistSq(o.pawn->Location, b.pawn->Location);
			if (d >= bestSq) continue;
			++g_nRange;
			if (b.tf == o.tf) continue;
			++g_nHostile;
			// No sight line test: LineOfSightTo and FastTrace both fail between two
			// AI pawns here. The AI's own fire path (TargetInLOS) still checks
			// line of sight before it shoots, so the radius alone picks the target.
			best = &o;
			bestSq = d;
		}
		if (best == nullptr) continue;
		++g_nSeen;
		b.ctrl->eventSeePlayer(best->pawn);
		b.ctrl->SetTargetActor((AActor*)best->pawn);
		if (b.ctrl->Enemy == nullptr) b.ctrl->Enemy = best->pawn;
		Logger::Log("tgbotfactory", "[BotSight] factory %d bot sees factory %d bot at %.0f uu (enemy=%d target=%d isenemy=%d)\n",
			b.ctrl->m_pFactory->m_nMapObjectId, best->ctrl->m_pFactory->m_nMapObjectId, sqrtf(bestSq),
			b.ctrl->Enemy == best->pawn ? 1 : 0,
			((ATgPawn*)b.pawn)->r_TargetActor == (AActor*)best->pawn ? 1 : 0, b.ctrl->IsEnemy((AActor*)best->pawn) ? 1 : 0);
	}
}

}  // namespace BotSight
