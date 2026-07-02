#include "src/GameServer/Cosmetics/SuitRebuildKick.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <vector>

namespace {

constexpr float kKickDelaySeconds = 2.0f;  // async preloads from the switch settle well within this

struct Pending {
	ATgPawn* pawn;
	int      pawnId;      // second factor for the stale-pointer guard
	float    remaining;   // countdown to the kick
};

std::vector<Pending> g_pending;

}  // namespace

void SuitRebuildKick::Schedule(ATgPawn* Pawn) {
	if (Pawn == nullptr) return;
	for (Pending& p : g_pending) {
		if (p.pawn == Pawn && p.pawnId == Pawn->r_nPawnId) {
			p.remaining = kKickDelaySeconds;
			return;
		}
	}
	g_pending.push_back(Pending{Pawn, Pawn->r_nPawnId, kKickDelaySeconds});
}

void SuitRebuildKick::Tick(float DeltaSeconds) {
	if (g_pending.empty()) return;

	for (size_t i = 0; i < g_pending.size();) {
		Pending& p = g_pending[i];
		auto it = GPawnSessions.find(p.pawn);
		if (it == GPawnSessions.end() || p.pawn->r_nPawnId != p.pawnId) {
			Logger::Log("loadout", "[SuitRebuildKick] pawn=%d gone — dropped\n", p.pawnId);
			g_pending.erase(g_pending.begin() + i);
			continue;
		}
		p.remaining -= DeltaSeconds;
		if (p.remaining > 0.0f) { ++i; continue; }

		// Persistent alternating sentinel, NOT a blip: replication 'recent'
		// state is per actor channel, so a one-tick flip+restore coalesces to
		// no delta on any connection whose channel skipped that frame — the
		// owner (every-frame channel) healed while observers missed the kick
		// entirely (out/logs/278: server assembly correct, owner correct,
		// observer stuck on class-default suit). A permanent -1↔-2 toggle
		// yields exactly one delta per connection whenever its channel next
		// updates. Harmless: player pawns hold 0 here anyway (invalid id →
		// FireSockets null-model fallback is already their steady state) and
		// custom characters ignore the value visually (GetBodyMeshId returns
		// the suit id, TgPawn_Character.uc:369).
		const int sentinel = (p.pawn->r_nBodyMeshAsmId == -1) ? -2 : -1;
		p.pawn->r_nBodyMeshAsmId = sentinel;
		p.pawn->bNetDirty        = 1;
		p.pawn->bForceNetUpdate  = 1;
		Logger::Log("loadout",
			"[SuitRebuildKick] r_nBodyMeshAsmId -> %d pawn=%d — rebuild kicked on all clients\n",
			sentinel, p.pawnId);
		g_pending.erase(g_pending.begin() + i);
	}
}
