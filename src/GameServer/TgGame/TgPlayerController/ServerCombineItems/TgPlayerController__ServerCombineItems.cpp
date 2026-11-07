#include "src/GameServer/TgGame/TgPlayerController/ServerCombineItems/TgPlayerController__ServerCombineItems.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Apply a crafted mod (Item Modification screen / profile
// wrench -> CombineItems). The original native asked Hi-Rez's backend; the
// control server owns the bag, so the request goes there (GAME_EVENT
// combine_items, EconParity::CombineItems) and is checked against the game's
// blueprint mod tables. Same shape as salvage_item.
void __fastcall TgPlayerController__ServerCombineItems::Call(ATgPlayerController* Controller, void* edx, int nInvId, int nModKitInvId) {
	LogCallBegin();
	ATgPawn_Character* Pawn = Controller ? (ATgPawn_Character*)Controller->Pawn : nullptr;
	std::string session_guid;
	int64_t character_id = 0;
	for (auto& kv : GClientConnectionsData) {
		if (!Pawn || kv.second.Pawn != Pawn) continue;
		session_guid = kv.second.SessionGuid;
		if (kv.second.pPlayerInfo) character_id = kv.second.pPlayerInfo->selected_character_id;
		break;
	}
	Logger::Log("vendor", "[Bag] combine_items char=%lld inv=%d modkit=%d\n", (long long)character_id, nInvId, nModKitInvId);
	if (Pawn && !session_guid.empty() && character_id > 0 && nInvId > 0 && nModKitInvId > 0) {
		nlohmann::json ev;
		ev["type"]             = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]          = "combine_items";
		ev["instance_id"]      = IpcClient::GetInstanceId();
		ev["session_guid"]     = session_guid;
		ev["character_id"]     = character_id;
		ev["pawn_id"]          = (int)Pawn->r_nPawnId;
		ev["inventory_id"]     = nInvId;
		ev["mod_inventory_id"] = nModKitInvId;
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
