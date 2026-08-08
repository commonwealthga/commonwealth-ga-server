#include "src/GameServer/TgGame/TgOmegaVolume/Used/TgOmegaVolume__Used.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstddef>
#include <map>
#include <string>

namespace {
	// m_pAmVolume points at the cached asm_data_set_ui_volumes row. Layout
	// lifted from the row's unmarshal at 0x109468d0, which names every column
	// (CMarshal__get_int32_t(row, <COLUMN_ID>, &field)). Offsets below 0x1c
	// are the marshal object's own header — not row data.
	struct AmUiVolumeRow {
		unsigned char header[0x1c];
		int      ui_volume_id;             // +0x1c  col 0x0523
		int      volume_type_value_id;     // +0x20  col 0x054A
		wchar_t* name;                     // +0x24  col 0x0371 (translated)
		wchar_t* summary;                  // +0x28  col 0x04D5 (translated)
		wchar_t* desc;                     // +0x2c  col 0x01F9 (translated)
		int      parent_help_ui_volume_id; // +0x30  col 0x03A4
		int      use_msg_id;               // +0x34  col 0x0530
		int      ui_scene_res_a;           // +0x38  col 0x0522, resolved to a
		int      ui_scene_res_b;           // +0x3c  2-word handle by FUN_10941d00
		int      map_game_id;              // +0x40  col 0x0322  ← travel target
		int      loot_table_id;            // +0x44  col 0x0319
		int      queue_selection_list_id;  // +0x48  col 0x0407
		int      quest_group_id;           // +0x4c  col 0x03FF
		void*    requirement_set;          // +0x50  null = ungated
	};
	// 32-bit layout — the offsets only line up with 4-byte pointers.
	static_assert(offsetof(AmUiVolumeRow, volume_type_value_id) == 0x20,
	              "AmUiVolumeRow.volume_type_value_id must sit at +0x20");
	static_assert(offsetof(AmUiVolumeRow, map_game_id) == 0x40,
	              "AmUiVolumeRow.map_game_id must sit at +0x40");

	// volume_type_value_id 1255 = Map Transition (asm_data_set_valid_values
	// group 136). Its map_game_id is the travel destination.
	constexpr int kVolumeTypeMapTransition = 1255;

	// Use-spam guard. The control server dedupes too (one instance per map,
	// one travel poll per session), but a held Use key would otherwise push an
	// IPC message every frame for the whole ~30s the zone takes to spawn.
	constexpr uint64_t kTravelCooldownMs = 10000;
	std::map<std::string, uint64_t> g_lastTravelRequestMs;

	const wchar_t* SafeW(const wchar_t* s) {
		return s ? s : L"<null>";
	}

	// Reverse-resolve the using player's session guid from their pawn. Fewer
	// than a dozen connections in practice, so the walk is cheap.
	std::string SessionGuidForController(ATgPlayerController* PC) {
		if (!PC || !PC->Pawn) return std::string();
		for (const auto& kv : GClientConnectionsData) {
			const ClientConnectionData& cd = kv.second;
			if (cd.Pawn == (ATgPawn_Character*)PC->Pawn && !cd.bClosed) {
				return cd.SessionGuid;
			}
		}
		return std::string();
	}

	// Ask the control server to route this player to the volume's destination.
	// Everything past "is this a Map Transition volume" is decided server-side:
	// eligibility (map_game_info.gameplay_type_value_id must be 1554), whether
	// an instance exists, and whether to spawn one.
	void RequestTravel(const AmUiVolumeRow* row, ATgPlayerController* UsingPlayer) {
		if (!row || row->volume_type_value_id != kVolumeTypeMapTransition) return;
		if (row->map_game_id == 0) {
			Logger::Log("travel",
				"[OmegaVolume] Map Transition volume ui_volume_id=%d has map_game_id=0 — no destination\n",
				row->ui_volume_id);
			return;
		}

		const std::string guid = SessionGuidForController(UsingPlayer);
		if (guid.empty()) {
			Logger::Log("travel",
				"[OmegaVolume] Map Transition ui_volume_id=%d: no session guid for the using player\n",
				row->ui_volume_id);
			return;
		}

		const uint64_t now = GetTickCount64();
		auto it = g_lastTravelRequestMs.find(guid);
		if (it != g_lastTravelRequestMs.end() && now - it->second < kTravelCooldownMs) {
			return;  // silent — this is the held-key case
		}
		g_lastTravelRequestMs[guid] = now;

		Logger::Log("travel",
			"[OmegaVolume] Travel request: guid=%s ui_volume_id=%d → map_game_id=%d ('%ls')\n",
			guid.c_str(), row->ui_volume_id, row->map_game_id, SafeW(row->name));
		IpcClient::SendRequestTravel(guid, (uint32_t)row->map_game_id);
	}

	const char* VolumeTypeName(int typeValueId) {
		switch (typeValueId) {
			case 1252: return "Help";
			case 1253: return "Vendor";
			case 1254: return "Open Scene";
			case 1255: return "Map Transition";
			case 1256: return "Queue";
			case 1430: return "Quest Giver";
			case 1504: return "Interaction";
			case 1638: return "Beacon Net Node";
			default:   return "<unknown>";
		}
	}
}

void __fastcall TgOmegaVolume_Used::Call(ATgOmegaVolume* Volume, void* edx,
                                         ATgPlayerController* UsingPlayer) {
	const AmUiVolumeRow* row = Volume
		? reinterpret_cast<const AmUiVolumeRow*>(
			static_cast<uintptr_t>(static_cast<uint32_t>(Volume->m_pAmVolume.Dummy)))
		: nullptr;

	if (Volume && Logger::IsChannelEnabled("omegavolume")) {
		const char* rawName = Volume->GetFullName();
		const std::string volName(rawName ? rawName : "<null>");

		const int typeValueId = row ? row->volume_type_value_id : 0;

		const wchar_t* playerName = L"<none>";
		if (UsingPlayer && UsingPlayer->PlayerReplicationInfo &&
		    UsingPlayer->PlayerReplicationInfo->PlayerName.Data) {
			playerName = UsingPlayer->PlayerReplicationInfo->PlayerName.Data;
		}

		Logger::Log("omegavolume",
			"[%s] Used: map=%s volume=%s player='%ls' PC=%p HUD=%p Pawn=%p\n",
			Logger::GetTime(), Config::GetMapNameChar().c_str(), volName.c_str(),
			playerName, (void*)UsingPlayer,
			UsingPlayer ? (void*)UsingPlayer->myHUD : nullptr,
			UsingPlayer ? (void*)UsingPlayer->Pawn : nullptr);

		// Actor-side (map-baked) data — m_nOmegaAlertId is the join key into
		// asm_data_set_ui_volumes.ui_volume_id (verified against
		// map_tg_omega_volume rows for Dome3_VR_Arena_P).
		Logger::Log("omegavolume",
			"[%s]   actor: mapObjectId=%d omegaAlertId(ui_volume_id)=%d "
			"subzoneMsg=%d subzoneSecondaryMsg=%d priority=%d bilboardKey=%d "
			"equip=%d skills=%d crafting=%d autoKick=%d visualCue=%d "
			"QueueTeleporter=%p StartPoint=%p OmegaNPC=%p\n",
			Logger::GetTime(),
			Volume->m_nMapObjectId, Volume->m_nOmegaAlertId,
			Volume->m_nSubzoneNameMsgId, Volume->m_nSubzoneSecondaryNameMsgId,
			Volume->m_nOmegaPriority, Volume->m_nBilboardKey,
			(int)Volume->m_bEnableEquip, (int)Volume->m_bEnableSkills,
			(int)Volume->m_bEnableCrafting, (int)Volume->m_bAutoKickIfIdle,
			(int)Volume->m_eVisualCue,
			(void*)Volume->m_QueueTeleporter, (void*)Volume->m_StartPoint,
			(void*)Volume->c_OmegaNPC);

		// AM row (asm_data_set_ui_volumes). For type 1255 the travel target is
		// map_game_id → map_game_info.map_game_id → map_name.
		if (!row) {
			Logger::Log("omegavolume", "[%s]   amRow=<null>\n", Logger::GetTime());
		} else {
			Logger::Log("omegavolume",
				"[%s]   amRow=%p uiVolumeId=%d type=%d (%s) mapGameId=%d "
				"questGroupId=%d queueSelectionListId=%d lootTableId=%d\n",
				Logger::GetTime(), (const void*)row,
				row->ui_volume_id, row->volume_type_value_id,
				VolumeTypeName(row->volume_type_value_id),
				row->map_game_id, row->quest_group_id,
				row->queue_selection_list_id, row->loot_table_id);

			Logger::Log("omegavolume",
				"[%s]   amRow: useMsgId=%d parentHelpUiVolumeId=%d "
				"uiSceneRes=0x%08x/0x%08x reqSet=%p\n",
				Logger::GetTime(), row->use_msg_id, row->parent_help_ui_volume_id,
				(unsigned)row->ui_scene_res_a, (unsigned)row->ui_scene_res_b,
				row->requirement_set);

			Logger::Log("omegavolume",
				"[%s]   amRow: name='%ls' summary='%ls' desc='%ls'\n",
				Logger::GetTime(), SafeW(row->name), SafeW(row->summary),
				SafeW(row->desc));
		}
	}

	// Server-side reimplementation of the Map Transition case. Retail routed it
	// through the HUD (client-only, and the base ATgHUD slot is a stub), so the
	// dedicated server has always dropped it on the floor.
	RequestTravel(row, UsingPlayer);

	// Retail body is intact (not a stub) — chain it. Server-side it bails at
	// its own Cast_TgHUD(myHUD) null check.
	CallOriginal(Volume, edx, UsingPlayer);
}
