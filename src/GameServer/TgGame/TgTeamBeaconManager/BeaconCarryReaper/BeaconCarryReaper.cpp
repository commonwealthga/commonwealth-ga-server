#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconCarryReaper/BeaconCarryReaper.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <utility>
#include <vector>

namespace BeaconCarryReaper {

namespace {

struct Pending {
	int   pawnId;
	int   invId;
	float remaining;
};

std::vector<Pending> g_pending;

// (pawnId, invId) fire cycles whose deploy was refused by the placement gate.
std::vector<std::pair<int, int>> g_refused;

// Pawn pointers do not survive a death/respawn, so records are keyed by
// r_nPawnId and resolved against the live connection table (bounded by player
// count) rather than by storing a raw ATgPawn*. Same idiom as
// MatchStats::FlushAll / TgBotFactory__SpawnNextBot.
ATgPawn* ResolveLivePawn(int pawnId) {
	for (const auto& kv : GClientConnectionsData) {
		ATgPawn_Character* p = kv.second.Pawn;
		if (p && (int)p->r_nPawnId == pawnId) return (ATgPawn*)p;
	}
	return nullptr;
}

}  // namespace

void ArmConsume(ATgPawn* Pawn, int invId, float delaySecs) {
	if (!Pawn || invId <= 0) return;
	const int pawnId = (int)Pawn->r_nPawnId;

	if (delaySecs < 2.0f)  delaySecs = 2.0f;
	if (delaySecs > 10.0f) delaySecs = 10.0f;

	// Re-arming the same (pawn, invId) just refreshes the deadline.
	for (Pending& p : g_pending) {
		if (p.pawnId == pawnId && p.invId == invId) {
			p.remaining = delaySecs;
			return;
		}
	}
	g_pending.push_back(Pending{ pawnId, invId, delaySecs });
	Logger::Log("beacon",
		"BeaconCarryReaper: armed pawn=%d invId=%d in %.2fs\n", pawnId, invId, delaySecs);
}

void MarkDeployRefused(int pawnId, int invId) {
	if (invId <= 0) return;
	for (const auto& r : g_refused) {
		if (r.first == pawnId && r.second == invId) return;
	}
	g_refused.push_back({ pawnId, invId });
}

bool ConsumeDeployRefused(int pawnId, int invId) {
	for (size_t i = 0; i < g_refused.size(); ++i) {
		if (g_refused[i].first == pawnId && g_refused[i].second == invId) {
			g_refused.erase(g_refused.begin() + i);
			return true;
		}
	}
	return false;
}

void Disarm(int pawnId, int invId) {
	for (size_t i = 0; i < g_pending.size(); ++i) {
		if (g_pending[i].pawnId == pawnId && g_pending[i].invId == invId) {
			g_pending.erase(g_pending.begin() + i);
			return;
		}
	}
}

void Tick(float DeltaSeconds) {
	if (g_pending.empty()) return;

	// Collect the due records first — the sweep calls NonPersistRemoveDevice,
	// which re-enters CheckBeacon and could in principle arm/disarm again.
	std::vector<Pending> due;
	for (size_t i = 0; i < g_pending.size(); ) {
		g_pending[i].remaining -= DeltaSeconds;
		if (g_pending[i].remaining <= 0.0f) {
			due.push_back(g_pending[i]);
			g_pending.erase(g_pending.begin() + i);
		} else {
			++i;
		}
	}
	if (due.empty()) return;

	for (const Pending& p : due) {
		ATgPawn* Pawn = ResolveLivePawn(p.pawnId);
		if (!Pawn) continue;   // died / disconnected — DropCarriedBeacon owns that path

		ATgDevice* dev = Pawn->m_EquippedDevices[11];
		if (!dev) continue;                              // already cleaned — normal case
		if (dev->r_nInventoryId != p.invId) continue;    // a DIFFERENT beacon: leave it alone
		// m_bIsBeaconPlacing = bit 0x10000 of the dword at TgDevice+0x22C.
		if ((*(uint32_t*)((char*)dev + 0x22C) & 0x10000u) == 0) continue;

		ATgInventoryManager* invMgr = (ATgInventoryManager*)Pawn->InvManager;
		if (!invMgr) continue;

		Logger::Log("beacon",
			"BeaconCarryReaper: slot 11 STILL holds invId=%d on pawn=%d after deploy — "
			"EndState cleanup was missed, removing now\n",
			p.invId, p.pawnId);

		// Does the slot clear, the equip-effect reversal, the SEND_INVENTORY
		// delete, and (post-fix) the trailing CheckBeacon.
		TgInventoryManager__NonPersistRemoveDevice::Call(invMgr, nullptr, 11);
	}
}

}  // namespace BeaconCarryReaper
