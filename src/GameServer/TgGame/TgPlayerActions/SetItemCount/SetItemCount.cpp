#include "src/GameServer/TgGame/TgPlayerActions/SetItemCount/SetItemCount.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::SetItemCountCmd {

void Execute(const std::string& session_guid, int total) {
	if (total <= 0) return;

	ATgPawn_Character* Pawn = nullptr;
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == session_guid) { Pawn = kv.second.Pawn; break; }
	}
	if (Pawn == nullptr || Pawn->InvManager == nullptr) {
		Logger::Log("loot", "[Loot] set_item_count guid=%s total=%d: no pawn/InvManager\n",
			session_guid.c_str(), total);
		return;
	}

	ATgInventoryManager* Mgr = (ATgInventoryManager*)Pawn->InvManager;
	const int before = Mgr->r_ItemCount;
	Mgr->r_ItemCount = total;

	// ‼️ Without these the new value never leaves the server. r_ItemCount rides
	// the actor channel's INITIAL bunch fine (fresh channel, NEQ against the
	// CDO), but a later write that doesn't mark the manager dirty means the
	// InvManager is never reconsidered for replication — the client keeps the
	// count it received at spawn while its m_InventoryMap moves on, and
	// IsValid() fails on the first grant or removal. Same idiom as
	// CosmeticEquip / IpcClient use after any server-side replicated write.
	Mgr->bNetDirty       = 1;
	Mgr->bForceNetUpdate = 1;

	Logger::Log("loot", "[Loot] set_item_count guid=%s r_ItemCount %d -> %d (net dirty)\n",
		session_guid.c_str(), before, total);
}

} // namespace TgPlayerActions::SetItemCountCmd
