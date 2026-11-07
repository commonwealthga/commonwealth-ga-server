#include "src/GameServer/TgGame/TgPlayerController/ServerSalvageInvItem/TgPlayerController__ServerSalvageInvItem.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Bag 'Salvage'. The original native asked Hi-Rez's backend;
// the control server owns the bag, so the request goes there (GAME_EVENT
// salvage_item, Database::SalvageItem) and is checked against the game's tables.
void __fastcall TgPlayerController__ServerSalvageInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId, int nCount) {
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
	Logger::Log("vendor", "[Bag] salvage_item char=%lld inv=%d\n", (long long)character_id, nInvId);
	if (Pawn && !session_guid.empty() && character_id > 0 && nInvId > 0) {
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "salvage_item";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = session_guid;
		ev["character_id"] = character_id;
		ev["pawn_id"]      = (int)Pawn->r_nPawnId;
		ev["inventory_id"] = nInvId;
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
