#include "src/GameServer/TgGame/TgPlayerController/ServerRepairInvItem/TgPlayerController__ServerRepairInvItem.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Agent Profile wrench -> Repair (client RepairInvItem ->
// this RPC; the client already checked that a same-quality Repair Kit is in
// the bag and asked to confirm above 10% durability). The original native
// asked Hi-Rez's backend; the control server owns the bag and durability, so
// the request goes there (GAME_EVENT repair_item, Durability::Repair: one kit
// used, item back to full). Same shape as combine_items.
void __fastcall TgPlayerController__ServerRepairInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId) {
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
	Logger::Log("vendor", "[Bag] repair_item char=%lld inv=%d\n", (long long)character_id, nInvId);
	if (Pawn && !session_guid.empty() && character_id > 0 && nInvId > 0) {
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "repair_item";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = session_guid;
		ev["character_id"] = character_id;
		ev["pawn_id"]      = (int)Pawn->r_nPawnId;
		ev["inventory_id"] = nInvId;
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
