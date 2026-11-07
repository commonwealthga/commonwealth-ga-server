#include "src/GameServer/TgGame/TgPlayerController/ServerCraftItem/TgPlayerController__ServerCraftItem.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Crafting (blueprint + components -> item). The original native
// talked to Hi-Rez's backend; the control server owns components + bag, so the
// request is forwarded there (GAME_EVENT craft_item) and checked against the
// game's own recipe tables (asm_data_set_blueprints / _blueprint_items).
void __fastcall TgPlayerController__ServerCraftItem::Call(ATgPlayerController* Controller, void* edx, int nBlueprintId) {
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
	Logger::Log("vendor", "[Craft] request char=%lld blueprint=%d\n", (long long)character_id, nBlueprintId);
	if (Pawn && !session_guid.empty() && character_id > 0) {
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "craft_item";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = session_guid;
		ev["character_id"] = character_id;
		ev["pawn_id"]      = (int)Pawn->r_nPawnId;
		ev["blueprint_id"] = nBlueprintId;
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
