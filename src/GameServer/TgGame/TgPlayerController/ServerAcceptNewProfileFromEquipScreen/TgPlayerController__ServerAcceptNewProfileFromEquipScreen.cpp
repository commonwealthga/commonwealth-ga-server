#include "src/GameServer/TgGame/TgPlayerController/ServerAcceptNewProfileFromEquipScreen/TgPlayerController__ServerAcceptNewProfileFromEquipScreen.hpp"
#include "src/GameServer/Armor/Armor.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/Database/Database.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <cstdlib>
#include <set>

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

	Logger::Log(GetLogChannel(), "equip-save: nProfileId(loadout)=%d\n", nProfileId);

// 	int SlotIndices[ 0x19 ];   // 0x0000 (0x0064) — inventory_id per equip point
// 	int MiscItems  [ 0x19 ];   // 0x0064 (0x0064) — armor / Misc tab inventory ids
//                                                  (indexing scheme TBD via this log)

	// Compact dump: one line per non-zero entry from either array, plus a
	// final "(all zero entries omitted)" footer if either was sparse. Lets
	// us correlate `MiscItems[N] = <armor invId>` to which armor slot the
	// client dragged when the user does an Armor-tab Apply — that's the
	// data we need to wire up swap-variant handling.
	nlohmann::json slotMap  = nlohmann::json::object();
	nlohmann::json miscMap  = nlohmann::json::object();
	int slotPopulated = 0;
	int miscPopulated = 0;
	for (int i = 0; i < 0x19; i++) {
		const int s = DeviceArray.SlotIndices[i];
		const int m = DeviceArray.MiscItems[i];
		if (s != 0 || m != 0) {
			Logger::Log(GetLogChannel(),
				"equip-save: [%2d] SlotIndices=%d MiscItems=%d\n", i, s, m);
		}
		if (s > 0) {
			// JSON object key is the equip-point as a string ("1".."24");
			// the value is the inventory_id. The control-server handler
			// (TcpSession.cpp `equip_save` branch) parses it back with atoi.
			slotMap[std::to_string(i)] = s;
			++slotPopulated;
		}
		if (m > 0) {
			miscMap[std::to_string(i)] = m;
			++miscPopulated;
		}
	}
	Logger::Log(GetLogChannel(),
		"equip-save: SlotIndices populated=%d, MiscItems populated=%d\n",
		slotPopulated, miscPopulated);

	// Resolve session + character. PlayerController->Pawn may be null if the
	// equip screen was opened from lobby with no live pawn — log and bail in
	// that case rather than sending a useless IPC.
	ATgPawn* Pawn = (ATgPawn*)PlayerController->Pawn;
	if (!Pawn) {
		Logger::Log(GetLogChannel(),
			"equip-save: PlayerController->Pawn=null — nothing to forward (slots=%d misc=%d)\n",
			slotPopulated, miscPopulated);
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
	const int64_t user_id      = info ? info->user_id : 0;

	// Engine-side re-equip. The original `ServerAcceptNewProfileFromEquipScreen`
	// native at 0x10963040 is stripped (decompile is just `return;`), so without
	// this loop the pawn keeps its spawn-time loadout while the client believes
	// it's now wielding what was picked in the equip screen. Result: the client's
	// `CGameClient::SendInventory` recovery scan can't find backing device actors
	// for the new equipped invIds and emits a SEND_INVENTORY resubmit every
	// frame.
	sqlite3* db = Database::GetConnection();

	// Resolve every non-zero slot's inventory row up front. The pre-pass
	// needs to know whether the new invId is a gameplay device (deviceId > 0)
	// or a cosmetic (itemId > 0, deviceId == 0) to decide whether to unequip
	// the current engine-side device at that slot.
	//
	// Per-slot facts cached for the equip loop below.
	struct SlotResolution {
		int  newInvId   = 0;
		int  deviceId   = 0;
		int  quality    = 0;
		int  itemId     = 0;
		std::vector<int> mods;
		bool isCosmetic = false;  // deviceId == 0 && itemId > 0
	};
	SlotResolution resolved[0x19];
	{
		sqlite3_stmt* stmt = nullptr;
		// v76: also pull item_id and gate by user_id so the player can't
		// equip another account's inventory by guessing inv_ids.
		const int rc = sqlite3_prepare_v2(db,
			"SELECT device_id, quality, mod_effect_group_ids, item_id "
			"FROM ga_players_inventory WHERE id = ? AND user_id = ?",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK && stmt) {
			for (int slot = 1; slot < 0x19; ++slot) {
				const int invId = DeviceArray.SlotIndices[slot];
				if (invId <= 0) continue;
				sqlite3_reset(stmt);
				sqlite3_clear_bindings(stmt);
				sqlite3_bind_int  (stmt, 1, invId);
				sqlite3_bind_int64(stmt, 2, user_id);
				if (sqlite3_step(stmt) != SQLITE_ROW) continue;

				SlotResolution& r = resolved[slot];
				r.newInvId = invId;
				r.deviceId = sqlite3_column_int(stmt, 0);
				r.quality  = sqlite3_column_int(stmt, 1);
				r.itemId   = sqlite3_column_int(stmt, 3);
				r.isCosmetic = (r.deviceId == 0 && r.itemId > 0);

				const char* csv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
				if (csv && *csv) {
					const char* p = csv;
					while (*p) {
						while (*p == ',' || *p == ' ') ++p;
						if (!*p) break;
						char* end = nullptr;
						long v = std::strtol(p, &end, 10);
						if (end == p) break;
						r.mods.push_back((int)v);
						p = end;
					}
				}
			}
			sqlite3_finalize(stmt);
		}
	}

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
	//
	// Cosmetic items (deviceId == 0): they don't occupy m_EquippedDevices,
	// the engine-side write is just a r_CustomCharacterAssembly field.
	// Unequipping at slot 6/12 when the client sent a cosmetic at that slot
	// would tear out the gameplay suit/helmet — content-aware gate skips that.
	// Dyes (16-20) and jetpack trail (21) are also always cosmetic and
	// previously fell through this skip — the resolved.isCosmetic gate
	// subsumes the old slot-number whitelist.
	for (int slot = 1; slot < 0x19; ++slot) {
		const SlotResolution& r = resolved[slot];
		if (r.newInvId <= 0) continue;
		if (r.isCosmetic)    continue;  // no engine device to unequip for this slot
		ATgDevice* cur = Pawn->m_EquippedDevices[slot];
		if (cur == nullptr) continue;
		if (cur->r_nDeviceInstanceId == r.newInvId) continue;  // same item, leave alone (Equip will short-circuit)
		Inventory::Unequip(Pawn, slot);
	}


	int equipped_now = 0;
	std::set<int> equippedCosmeticEngineSlots;
	for (int slot = 1; slot < 0x19; ++slot) {
		const SlotResolution& r = resolved[slot];
		if (r.newInvId <= 0) continue;

		if (r.deviceId > 0) {
			Inventory::Equip(Pawn, r.deviceId, slot, r.quality, r.newInvId, r.mods);
			++equipped_now;
		} else if (r.itemId > 0) {
			// Cosmetic path. ApplyToPawn writes the right
			// r_CustomCharacterAssembly field + persists at the remapped DB
			// slot (cosmetic suit→22, cosmetic helmet→23) so it doesn't
			// collide with the gameplay device row at slot 6/12.
			CosmeticEquip::ApplyToPawn(Pawn, character_id, slot, r.newInvId, r.itemId);
			equippedCosmeticEngineSlots.insert(slot);
		}
	}
	CosmeticEquip::ClearUnsetSlots(Pawn, equippedCosmeticEngineSlots);
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

	// Armor runtime refresh. The client's Armor-tab equip selections arrive in
	// FTGEQUIP_SLOTS_STRUCT.MiscItems[] (index → slot_value_id = idx + 1128,
	// group-129 SVID space; see ArmorSlot in EquipSlot.hpp). The control-server
	// will persist these via SaveEquippedDevices's misc_items pass once we IPC,
	// but that doesn't refresh the pawn's runtime buffs — Armor.cpp only
	// queries ga_character_devices during ReapplyCharacterSkillTree (i.e. at
	// next spawn). To make the new variant's bonuses take effect immediately,
	// we write the same DELETE+INSERT here (local sqlite handle sees the same
	// server.db file as the control-server) and then call Armor::Revert+Apply
	// on this pawn. The control-server's later write is idempotent.
	//
	// Empty misc_items → no armor changes (player only swapped weapons or
	// cosmetics), so we leave the existing armor untouched. Matches the
	// gating in SaveEquippedDevices.
	if (character_id != 0) {
		bool anyArmorMisc = false;
		for (int i = 0; i < 0x19; ++i) {
			if (DeviceArray.MiscItems[i] > 0) { anyArmorMisc = true; break; }
		}

		if (anyArmorMisc) {
			// Profile-scoped: this RPC names its target loadout via nProfileId.
			// Only that profile's armor rows are replaced — other profiles
			// keep their armor untouched.
			sqlite3_stmt* delA = nullptr;
			if (sqlite3_prepare_v2(db,
			    "DELETE FROM ga_character_devices "
			    "WHERE character_id = ? AND item_profile_id = ? "
			    "  AND equipped_slot IN (1130, 1132, 1133, 1136, 1139, 1142, 1143)",
			    -1, &delA, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(delA, 1, character_id);
				sqlite3_bind_int  (delA, 2, nProfileId);
				sqlite3_step(delA);
				sqlite3_finalize(delA);
			}

			sqlite3_stmt* insA = nullptr;
			if (sqlite3_prepare_v2(db,
			    "INSERT INTO ga_character_devices "
			    "(character_id, item_profile_id, inventory_id, equipped_slot) "
			    "VALUES (?, ?, ?, ?)",
			    -1, &insA, nullptr) == SQLITE_OK) {
				int wroteArmor = 0;
				for (int i = 0; i < 0x19; ++i) {
					const int invId = DeviceArray.MiscItems[i];
					if (invId <= 0) continue;
					const int armorSvid = i + 1128;
					// Only the 7 visible armor slots; Core/Implant/Title indices
					// are ignored (they're not exposed by the shipped UI).
					if (armorSvid != 1130 && armorSvid != 1132 && armorSvid != 1133 &&
					    armorSvid != 1136 && armorSvid != 1139 && armorSvid != 1142 &&
					    armorSvid != 1143) {
						continue;
					}
					sqlite3_reset(insA);
					sqlite3_clear_bindings(insA);
					sqlite3_bind_int64(insA, 1, character_id);
					sqlite3_bind_int  (insA, 2, nProfileId);
					sqlite3_bind_int  (insA, 3, invId);
					sqlite3_bind_int  (insA, 4, armorSvid);
					if (sqlite3_step(insA) == SQLITE_DONE) ++wroteArmor;
				}
				sqlite3_finalize(insA);
				Logger::Log(GetLogChannel(),
					"equip-save: armor local-DB updated itemProf=%d, %d slot(s); refreshing buffs\n",
					nProfileId, wroteArmor);
			}

			// Reverse the previous armor's buff entries (recorded per-pawn in
			// Armor.cpp's Records() map) then re-query the DB and apply the
			// new ones. This is exactly what RCST does at spawn — same API,
			// just fired at runtime instead of waiting for next spawn.
			Armor::RevertDefaultArmor(Pawn);
			Armor::ApplyDefaultArmor(Pawn);
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
	ev["misc_items"]        = std::move(miscMap);  // armor / unknown-Misc-tab data
	IpcClient::Send(ev.dump());

	Logger::Log(GetLogChannel(),
		"equip-save: forwarded loadout=%d slots=%d/24 misc=%d/25 pawn=%p guid=%s\n",
		nProfileId, slotPopulated, miscPopulated, (void*)Pawn, it->second.c_str());

	LogCallEnd();
}
