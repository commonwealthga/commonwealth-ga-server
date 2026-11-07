// Logger channel: "vendor"
#include "src/GameServer/Inventory/ConsumableUse.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Database/Database.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// The Consume slot is engine equip point 13 = slot value 501 "ES13 (Consumable)"
// (asm valid values group 48); point 12 / 500 is ES12 (Helmet) - upstream's
// GA::EquipSlot::Consumable = 12 is mislabelled, so it is not used here.
static constexpr int kConsumePoint = 13;

void ConsumableUse::OnDeviceFired(ATgDevice* Device) {
	if (!Device || !Device->Instigator) return;
	if ((int)Device->r_eEquippedAt != kConsumePoint) return;
	const int invId = Device->r_nInventoryId;
	if (invId <= 0) return;
	ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
	if (Pawn->m_EquippedDevices[kConsumePoint] != Device) return;  // already spent

	std::string session_guid;
	int64_t user_id = 0, character_id = 0;
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.Pawn != Pawn) continue;
		session_guid = kv.second.SessionGuid;
		if (kv.second.pPlayerInfo) {
			user_id      = kv.second.pPlayerInfo->user_id;
			character_id = kv.second.pPlayerInfo->selected_character_id;
		}
		break;
	}
	if (user_id <= 0) return;  // bots / no session

	const bool ok = Database::ConsumeInvItem(user_id, invId);
	Logger::Log("vendor", "[Consumable] used char=%lld inv=%d device=%d -> %s\n",
		(long long)character_id, invId, Device->r_nDeviceId, ok ? "spent" : "NOT FOUND");
	if (!ok) return;
	if (Config::GetNodeMode())  // standalone mode: forward the event
		IpcClient::SendNodeClaim("consume", user_id, character_id, "inventory_id", invId);

	if (Pawn->InvManager)
		TgInventoryManager__NonPersistRemoveDevice::Call((ATgInventoryManager*)Pawn->InvManager, nullptr,
			kConsumePoint);
	if (!session_guid.empty()) {
		nlohmann::json ev;
		ev["type"]          = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]       = "inventory_removed";
		ev["instance_id"]   = IpcClient::GetInstanceId();
		ev["session_guid"]  = session_guid;
		ev["character_id"]  = character_id;
		ev["pawn_id"]       = (int)Pawn->r_nPawnId;
		ev["ok"]            = true;
		ev["inventory_ids"] = std::vector<int64_t>{ (int64_t)invId };
		IpcClient::Send(ev.dump());
	}
}
