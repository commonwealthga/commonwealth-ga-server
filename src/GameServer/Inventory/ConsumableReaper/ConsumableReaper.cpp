#include "src/GameServer/Inventory/ConsumableReaper/ConsumableReaper.hpp"

#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <vector>

namespace ConsumableReaper {

namespace {

// Long enough for the fire to have applied its effect group, short enough that
// the client's cooldown sweep is barely visible before the slot empties.
constexpr float kDelaySecs = 0.25f;

struct Pending {
	int        pawnId;
	int        slot;
	int        invId;
	ATgDevice* device;     // identity only — never dereferenced unless still the slot occupant
	float      remaining;
};

std::vector<Pending> g_pending;

// Pawn pointers do not survive a death/respawn — resolve by r_nPawnId against
// the live connection table, same idiom as BeaconCarryReaper.
ATgPawn* ResolveLivePawn(int pawnId) {
	for (const auto& kv : GClientConnectionsData) {
		ATgPawn_Character* p = kv.second.Pawn;
		if (p && (int)p->r_nPawnId == pawnId) return (ATgPawn*)p;
	}
	return nullptr;
}

void RemoveFromSlot(ATgPawn* Pawn, int slot, int invId) {
	// No eventRemoveAllDeviceEffects: the buff the device just applied must outlive it.
	// r_ItemCount stays: the inventory row is kept, only the loadout slot empties.
	Inventory::Unequip(Pawn, slot);
	Pawn->UpdateClientDevices(0, 0);

	// Control server drops the slot from the saved loadout and resyncs the
	// client's inventory, so DB, server and client agree on "not equipped".
	auto sessIt = GPawnSessions.find(Pawn);
	if (sessIt == GPawnSessions.end()) return;
	nlohmann::json ev;
	ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]      = "consumable_unequip";
	ev["instance_id"]  = IpcClient::GetInstanceId();
	ev["session_guid"] = sessIt->second;
	ev["pawn_id"]      = (int)Pawn->r_nPawnId;
	ev["inventory_id"] = invId;
	IpcClient::Send(ev.dump());
}

}  // namespace

void Arm(ATgPawn* Pawn, ATgDevice* Device) {
	if (!Pawn || !Device) return;
	const int slot = (int)Device->r_eEquippedAt;
	if (slot < 1 || slot > 24 || Pawn->m_EquippedDevices[slot] != Device) return;

	for (const Pending& p : g_pending) {
		if (p.device == Device) return;   // continuous fire re-enters BeginState
	}
	g_pending.push_back(Pending{ (int)Pawn->r_nPawnId, slot, Device->r_nInventoryId, Device, kDelaySecs });
	Logger::Log("consumable", "ConsumableReaper: armed pawn=%d devId=%d slot=%d invId=%d\n",
		(int)Pawn->r_nPawnId, Device->r_nDeviceId, slot, Device->r_nInventoryId);
}

void Tick(float DeltaSeconds) {
	if (g_pending.empty()) return;

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

	for (const Pending& p : due) {
		ATgPawn* Pawn = ResolveLivePawn(p.pawnId);
		// Died in the meantime: the consumable stays equipped (it was not
		// unequipped yet), the buff ends with death anyway.
		if (!Pawn || Pawn->Health <= 0) continue;

		// Identity check: only the actor that fired, never a re-equipped one.
		ATgDevice* dev = Pawn->m_EquippedDevices[p.slot];
		if (dev != p.device || dev->r_nInventoryId != p.invId) continue;

		Logger::Log("consumable", "ConsumableReaper: removing pawn=%d slot=%d invId=%d\n",
			p.pawnId, p.slot, p.invId);
		RemoveFromSlot(Pawn, p.slot, p.invId);
	}
}

}  // namespace ConsumableReaper
