#include "src/GameServer/TgGame/TgPawn_Character/ServerPurchaseItem/TgPawn_Character__ServerPurchaseItem.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Database/Database.hpp"
#include "src/Shared/EconRules2.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Vendor buy. The client RPC carries the loot table + row it clicked;
// the price and the item come from the game tables (nAttemptedCostPerItem is only
// logged), the wallet lives in ga_characters. The original native talked to Hi-Rez's
// backend, so it is not called.
void __fastcall TgPawn_Character__ServerPurchaseItem::Call(ATgPawn_Character* Pawn, void* edx, int nLootTableId, int nLootTableItemId, int nItemCount, int nAttemptedCurrencyType, int nAttemptedCostPerItem) {
	LogCallBegin();
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

	const auto r = Database::VendorPurchase(user_id, character_id, nLootTableId, nLootTableItemId,
		nItemCount, nAttemptedCurrencyType);
	Logger::Log("vendor",
		"[Vendor] buy char=%lld table=%d row=%d n=%d cur=%d client_cost=%d -> %s item=%d price=%d (%s) wallet cr=%d tok=%d\n",
		(long long)character_id, nLootTableId, nLootTableItemId, nItemCount, nAttemptedCurrencyType,
		nAttemptedCostPerItem, r.ok ? "OK" : "REJECTED", r.item_id, r.price, r.reason.c_str(),
		r.wallet.credits, r.wallet.tokens);

	// The client's own line (msg 39146) when the combat rating discount applied.
	if (r.ok && r.discount > 0) for (auto& kv : GClientConnectionsData) if (kv.second.Pawn == Pawn) {
		Logger::Log("vendor", "[Vendor] combat rating discount %d%% char=%lld item=%d list=%d x%d\n", r.discount, (long long)character_id, r.item_id, r.price, nItemCount);
		SendAlert::SendTextAlert((UNetConnection*)(intptr_t)kv.first, EconRules2::DiscountText(Database::GetConnection(), r.discount).c_str(), 1, 1, 4.0f);
		break;
	}
	if (character_id > 0) {
		// Replicated wallet -> vendor window + Agent Info update live.
		Pawn->r_nCurrency = r.wallet.credits;
		Pawn->r_nToken    = r.wallet.tokens;
		Pawn->r_nHZPoints = r.wallet.ap;
		Pawn->r_nBoostTimeRemaining = Database::GetBoosterSecondsLeft(character_id);
	}
	if (r.ok && !session_guid.empty()) {
		// Control server re-sends the inventory so the new item shows in the bag.
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "vendor_purchase";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = session_guid;
		ev["character_id"] = character_id;
		ev["pawn_id"]      = (int)Pawn->r_nPawnId;
		ev["item_id"]      = r.item_id;
		ev["inventory_ids"] = r.inventory_ids;
		ev["component"]    = r.component;
		ev["count"]        = nItemCount;
		ev["price"]        = (int)((int64_t)r.price * (nItemCount > 1 ? nItemCount : 1) * (100 - r.discount) / 100);
		ev["currency"]     = nAttemptedCurrencyType;
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
