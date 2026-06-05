#include "src/GameServer/TgGame/TgInventoryManager/SetInventoryDirty/TgInventoryManager__SetInventoryDirty.hpp"

#include "lib/nlohmann/json.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/PossessPawn/PossessPawn.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"

// The original native at 0x10a12320 is a stub on this binary (Ghidra symbol:
// `TgInventoryManager__SetInventoryDirty_notimplemented`). UC code expects this
// to be the universal "your inventory state changed, push fresh records to the
// client" signal — called by HackedBy/Unhacked, weapon switches, equip/unequip,
// and friends — but since the native does nothing in this build the entire
// engine-driven inventory-sync path is dead. The DLL has been picking up
// pieces of this manually (possess_show_loadout / possess_restore_inventory)
// but the stripped UC sites that ALSO call SetInventoryDirty (HackedBy line
// 7053, Unhacked line 7115) never get serviced — which is why state stayed
// stale across the possess/unpossess transition: HUD entries kept pointing at
// the bot's ATgDevices, cooldown timers ran on those, and the player slot UI
// showed the bot's device state instead of the player's.
//
// Implementation: when fired, identify which player session this InvManager
// belongs to via AActor::Owner (AInventoryManager.SetupFor(P) sets Owner=P),
// then bounce an IPC to the control server so it re-emits the canonical
// SEND_INVENTORY for that pawn's character. Bots have no session; skipped.

void __fastcall TgInventoryManager__SetInventoryDirty::Call(ATgInventoryManager* Manager, void* edx) {
	LogCallBegin();
	CallOriginal(Manager, edx);
	LogCallEnd();

	if (!Manager) return;

	// Owner is engine AActor* (polymorphic). Gate the cast so a stale/non-pawn
	// Owner doesn't get its non-pawn fields read as Pawn->PlayerReplicationInfo.
	AActor* owner = Manager->Owner;
	if (!ObjectClassCache::ClassNameContains(owner, "TgPawn")) return;
	ATgPawn* Pawn = (ATgPawn*)owner;

	// Match this Pawn against any active session. Iterating is cheap — fewer
	// than a dozen entries in practice — and we don't need a separate index.
	std::string session_guid;
	int64_t     character_id = 0;
	int         pawn_id      = 0;
	for (const auto& kv : GClientConnectionsData) {
		const ClientConnectionData& cd = kv.second;
		if (cd.Pawn == (ATgPawn_Character*)Pawn && cd.pPlayerInfo) {
			session_guid = cd.SessionGuid;
			character_id = cd.pPlayerInfo->selected_character_id;
			pawn_id      = (int)Pawn->r_nPawnId;
			break;
		}
	}
	if (!session_guid.empty() && character_id != 0 && pawn_id != 0) {
		// Player path: refresh via DB-backed inventory.
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "possess_restore_inventory";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = session_guid;
		ev["pawn_id"]      = pawn_id;
		ev["character_id"] = character_id;
		IpcClient::Send(ev.dump());

		Logger::Log("inventory",
			"SetInventoryDirty: player refresh pawn=%p pawn_id=%d char_id=%lld guid=%s\n",
			(void*)Pawn, pawn_id, (long long)character_id, session_guid.c_str());
		return;
	}

	// Bot path: SetInventoryDirty fires on the bot's InvManager during
	// HackedBy (UC line 7053). If a player session is currently possessing
	// this bot, push the bot's r_EquipDeviceInfo as a loadout. Otherwise (a
	// regular NPC bot under AI control with no observing client) skip.
	TgPlayerActions::PossessCmd::TrySendBotLoadoutRefresh(Pawn);
}
