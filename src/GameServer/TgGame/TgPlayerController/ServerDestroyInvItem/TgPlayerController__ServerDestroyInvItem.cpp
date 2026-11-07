#include "src/GameServer/TgGame/TgPlayerController/ServerDestroyInvItem/TgPlayerController__ServerDestroyInvItem.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Database/Database.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Vendor 'Sell' and bag 'Destroy' both arrive here (bSellIt).
// The row, its sell value and the wallet live in the DB (Database::DestroyInvItem);
// the original native talked to Hi-Rez's backend, so it is not called.
void __fastcall TgPlayerController__ServerDestroyInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId, int nCount, unsigned long bSellIt) {
	LogCallBegin();
	ATgPawn_Character* Pawn = Controller ? (ATgPawn_Character*)Controller->Pawn : nullptr;
	std::string session_guid;
	int64_t user_id = 0, character_id = 0;
	for (auto& kv : GClientConnectionsData) {
		if (!Pawn || kv.second.Pawn != Pawn) continue;
		session_guid = kv.second.SessionGuid;
		if (kv.second.pPlayerInfo) {
			user_id      = kv.second.pPlayerInfo->user_id;
			character_id = kv.second.pPlayerInfo->selected_character_id;
		}
		break;
	}

	const auto r = Database::DestroyInvItem(user_id, character_id, nInvId, bSellIt != 0);
	Logger::Log("vendor",
		"[Vendor] %s char=%lld inv=%d n=%d -> %s item=%d value=%d cur=%d (%s) wallet cr=%d tok=%d\n",
		bSellIt ? "sell" : "destroy", (long long)character_id, nInvId, nCount, r.ok ? "OK" : "REJECTED",
		r.item_id, r.value, r.currency, r.reason.c_str(), r.wallet.credits, r.wallet.tokens);

	if (Pawn && character_id > 0) {
		Pawn->r_nCurrency = r.wallet.credits;
		Pawn->r_nToken    = r.wallet.tokens;
		Pawn->r_nHZPoints = r.wallet.ap;
	}
	if (Pawn && !session_guid.empty()) {
		// Removed -> drop the row from the client's bag. Rejected -> re-add it, in
		// case the client already took it out of its own list.
		nlohmann::json ev;
		ev["type"]          = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]       = "inventory_removed";
		ev["instance_id"]   = IpcClient::GetInstanceId();
		ev["session_guid"]  = session_guid;
		ev["character_id"]  = character_id;
		ev["pawn_id"]       = (int)Pawn->r_nPawnId;
		ev["ok"]            = r.ok;
		ev["inventory_ids"] = std::vector<int64_t>{ (int64_t)nInvId };
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
