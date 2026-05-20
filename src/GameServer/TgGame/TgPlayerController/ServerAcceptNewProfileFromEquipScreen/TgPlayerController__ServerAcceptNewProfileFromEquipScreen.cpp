#include "src/GameServer/TgGame/TgPlayerController/ServerAcceptNewProfileFromEquipScreen/TgPlayerController__ServerAcceptNewProfileFromEquipScreen.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/Database/Database.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <cstdlib>

// Reliable-server RPC fired by the client's equip-screen "Apply" button.
//
// `DeviceArray.SlotIndices[i]` carries the inventory_id (= ga_players_inventory.id)
// the player chose for equip-point i+0. Index 0 is unused on the wire (engine
// equip points run 1..24); the client always sends index 0 as 0. Non-zero
// entries are real inventory ids; zero means "leave this slot empty".
//
// `nProfileId` is the LOADOUT profile (1..5 — different gear/skill builds
// per character). Only profile 1 is wired for now per scope; we log the
// received value so when multi-loadout work starts we can confirm the client
// payload matches expectations.
//
// `DeviceArray.MiscItems[i]` has been all-zero in every observed payload —
// likely consumable/dye related. Logged but otherwise ignored until we have
// a concrete use case.
//
// Forwards everything via IPC `equip_save` to the control server, which is
// the authority on ga_character_devices: it validates each (equip_point,
// inventory_id) pair against the user's inventory + current class profile +
// item allowed_slots, transactionally replaces the equipped set, then echoes
// SEND_INVENTORY back to refresh the client.
void __fastcall TgPlayerController__ServerAcceptNewProfileFromEquipScreen::Call(
	ATgPlayerController* PlayerController, void* edx,
	int nProfileId, FTGEQUIP_SLOTS_STRUCT DeviceArray)
{
	LogCallBegin();

	Logger::Log(GetLogChannel(), "nProfileId (loadout): %d\n", nProfileId);

// 	int SlotIndices[ 0x19 ];   // 0x0000 (0x0064) — inventory_id per equip point
// 	int MiscItems  [ 0x19 ];   // 0x0064 (0x0064) — observed all-zero

	nlohmann::json slotMap = nlohmann::json::object();
	int populated = 0;
	for (int i = 0; i < 0x19; i++) {
		Logger::Log(GetLogChannel(), "SlotIndices[%d]: %d\n", i, DeviceArray.SlotIndices[i]);
		Logger::Log(GetLogChannel(), "MiscItems[%d]: %d\n", i, DeviceArray.MiscItems[i]);
		if (DeviceArray.SlotIndices[i] > 0) {
			// JSON object key is the equip-point as a string ("1".."24");
			// the value is the inventory_id. The control-server handler
			// (TcpSession.cpp `equip_save` branch) parses it back with atoi.
			slotMap[std::to_string(i)] = DeviceArray.SlotIndices[i];
			++populated;
		}
	}

	// Resolve session + character. PlayerController->Pawn may be null if the
	// equip screen was opened from lobby with no live pawn — log and bail in
	// that case rather than sending a useless IPC.
	ATgPawn* Pawn = (ATgPawn*)PlayerController->Pawn;
	if (!Pawn) {
		Logger::Log(GetLogChannel(),
			"equip-save: PlayerController->Pawn=null — nothing to forward (populated=%d)\n",
			populated);
		LogCallEnd();
		return;
	}
	auto it = GPawnSessions.find(Pawn);
	if (it == GPawnSessions.end()) {
		Logger::Log(GetLogChannel(),
			"equip-save: no GPawnSessions entry for pawn=%p — dropped\n",
			(void*)Pawn);
		LogCallEnd();
		return;
	}

	PlayerInfo* info = PlayerRegistry::GetByGuidPtr(it->second);
	const int64_t character_id = info ? info->selected_character_id : 0;

	// Engine-side re-equip. The original `ServerAcceptNewProfileFromEquipScreen`
	// native at 0x10963040 is stripped (decompile is just `return;`), so without
	// this loop the pawn keeps its spawn-time loadout while the client believes
	// it's now wielding what was picked in the equip screen. Result: the client's
	// `CGameClient::SendInventory` recovery scan can't find backing device actors
	// for the new equipped invIds and emits a SEND_INVENTORY resubmit every
	// frame.
	sqlite3* db = Database::GetConnection();

	// Pre-pass: Unequip anything that's being SWAPPED for a different invId.
	// Without this, `Inventory::Equip` would create a second ATgDevice actor
	// at the same slot carrying a now-stale invId from the previous loadout,
	// and the device's effects (equip baseline + rolled-mod buffs) would stack
	// on top of the existing ones. Specifically caught by 2026-05-20: after
	// any re-equip, jetpack stopped pausing power regen because two ATgDevice
	// actors with the same r_nDeviceInstanceId existed, and server's
	// GetDeviceByEqPoint(5) returned the new idle one while client/state
	// machine operated on the leaked old one.
	//
	// Conservative gate: only unequip when the new invId is non-zero AND
	// different from current. Treat newInvId==0 as "client didn't ship this
	// slot" rather than "user removed this item" — the equip screen UI on
	// this build doesn't expose a clear-slot action, and the slot-14 rest
	// device is invisible to the screen so the client may legitimately omit
	// it. Skipping zero entries keeps that device safe.
	for (int slot = 1; slot < 0x19; ++slot) {
		const int newInvId = DeviceArray.SlotIndices[slot];
		if (newInvId <= 0) continue;
		ATgDevice* cur = Pawn->m_EquippedDevices[slot];
		if (cur == nullptr) continue;
		if (cur->r_nDeviceInstanceId == newInvId) continue;  // same item, leave alone (Equip will short-circuit)
		Inventory::Unequip(Pawn, slot);
	}


	int equipped_now = 0;
	for (int slot = 1; slot < 0x19; ++slot) {
		const int invId = DeviceArray.SlotIndices[slot];
		if (invId <= 0) continue;

		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT device_id, quality, mod_effect_group_ids "
			"FROM ga_players_inventory WHERE id = ?",
			-1, &stmt, nullptr);
		if (rc != SQLITE_OK || !stmt) continue;
		sqlite3_bind_int(stmt, 1, invId);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			const int deviceId = sqlite3_column_int(stmt, 0);
			const int quality  = sqlite3_column_int(stmt, 1);
			std::vector<int> mods;
			const char* csv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			if (csv && *csv) {
				const char* p = csv;
				while (*p) {
					while (*p == ',' || *p == ' ') ++p;
					if (!*p) break;
					char* end = nullptr;
					long v = std::strtol(p, &end, 10);
					if (end == p) break;
					mods.push_back((int)v);
					p = end;
				}
			}
			Inventory::Equip(Pawn, deviceId, slot, quality, invId, mods);
			++equipped_now;
		}
		sqlite3_finalize(stmt);
	}
	if (equipped_now > 0) {
		Inventory::Finalize(Pawn);
		Logger::Log(GetLogChannel(), "equip-save: engine-side equipped %d new device(s)\n", equipped_now);
	}

	// Reset r_ItemCount to the full pool size (see SpawnPlayerCharacter
	// comment for the IsValid gate). Inventory::Equip blindly ++'s it on
	// every call — without this fix-up, after a re-equip r_ItemCount drifts
	// above the actual count and IsValid keeps failing.
	{
		sqlite3_stmt* who = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
			-1, &who, nullptr);
		if (rc == SQLITE_OK && who) {
			sqlite3_bind_int64(who, 1, character_id);
			if (sqlite3_step(who) == SQLITE_ROW) {
				int64_t  user_id    = sqlite3_column_int64(who, 0);
				int      profile_id = sqlite3_column_int  (who, 1);
				sqlite3_stmt* cnt = nullptr;
				rc = sqlite3_prepare_v2(db,
					"SELECT COUNT(*) FROM ga_players_inventory "
					"WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?)",
					-1, &cnt, nullptr);
				if (rc == SQLITE_OK && cnt) {
					sqlite3_bind_int64(cnt, 1, user_id);
					sqlite3_bind_int  (cnt, 2, profile_id);
					if (sqlite3_step(cnt) == SQLITE_ROW) {
						const int total = sqlite3_column_int(cnt, 0);
						((ATgInventoryManager*)Pawn->InvManager)->r_ItemCount = total;
						Logger::Log(GetLogChannel(),
							"equip-save: InvManager->r_ItemCount=%d (pool total, IsValid gate)\n", total);
					}
					sqlite3_finalize(cnt);
				}
			}
			sqlite3_finalize(who);
		}
	}

	nlohmann::json ev;
	ev["type"]              = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]           = "equip_save";
	ev["instance_id"]       = IpcClient::GetInstanceId();
	ev["session_guid"]      = it->second;
	ev["pawn_id"]           = (int)Pawn->r_nPawnId;
	ev["character_id"]      = character_id;
	ev["loadout_profile"]   = nProfileId;
	ev["slot_to_inventory"] = std::move(slotMap);
	IpcClient::Send(ev.dump());

	Logger::Log(GetLogChannel(),
		"equip-save: forwarded loadout=%d populated=%d/24 pawn=%p guid=%s\n",
		nProfileId, populated, (void*)Pawn, it->second.c_str());

	LogCallEnd();
}
