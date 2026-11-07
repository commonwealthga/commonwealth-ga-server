#include "src/GameServer/TgGame/TgPlayerController/CanPlayerUseVolume/TgPlayerController__CanPlayerUseVolume.hpp"
#include "src/GameServer/TgGame/TgPawn/KillCredit/KillCredit.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstddef>
#include <cstdint>

// 0987: TgPawn.OmegaVolumePropertiesChanged (TgGame.u) makes the touching
// volume with the highest m_nOmegaPriority that CanPlayerUseVolume() accepts the
// pawn's r_CurrentOmegaVolume - the one whose prompt the client shows and whose
// Used() a press of U reaches. The upstream stub said "yes" to every volume, so
// a higher-priority NPC volume always hid an overlapping one. Dome City's north
// door (Map Transition 262, priority 0) sits inside the Northern Gate Guard
// (Quest Giver 259, priority 1): the guard always won, U always opened the
// guard's "civilian access to the North is restricted" talk, and nobody could
// ever leave through the north gate.
//
// Retail rule, from the volume's quest group (asm_data_set_quest_group_lists):
//  - rows flagged disregard_quest_flag = 1 are the volume's own conditions and
//    must ALL hold (guard 259: "A Call to Arms" (80) neither active nor complete
//    -> the guard steps aside once you carry the quest);
//  - a Map Transition's status rows are its travel conditions (door 262: quest 80
//    active or complete) - same check as the server-side travel gate in
//    TgOmegaVolume__Used.
// 0988 (house rule, not retail): the north door opens with the Sonoran door, so
// the Northern Gate Guard (259, overlaps the door) steps aside for an agent who
// has that clearance. Dome City Guards (284, same group, elsewhere in the city)
// are left alone: they still start "A Call to Arms".
// Hub + open zones only (Dome City, Sonoran Desert, North Sonora); missions and
// the tutorial keep the old always-yes answer.
namespace {
	// Same cached asm_data_set_ui_volumes row as TgOmegaVolume__Used (layout there).
	struct AmUiVolumeRowHead {
		unsigned char header[0x20];
		int volume_type_value_id;  // +0x20
		unsigned char pad[0x4c - 0x24];
		int quest_group_id;        // +0x4c
	};
	static_assert(offsetof(AmUiVolumeRowHead, volume_type_value_id) == 0x20, "type at +0x20");
	static_assert(offsetof(AmUiVolumeRowHead, quest_group_id) == 0x4c, "quest group at +0x4c");
	constexpr int kVolumeTypeMapTransition = 1255;
	constexpr int kNorthernGateGuardVolume = 259;

	int64_t CharacterIdOfController(ATgPlayerController* PC) {
		if (!PC || !PC->Pawn) return 0;
		for (auto& kv : GClientConnectionsData) {
			const ClientConnectionData& cd = kv.second;
			if (cd.bClosed || cd.Pawn != (ATgPawn_Character*)PC->Pawn) continue;
			return cd.pPlayerInfo ? cd.pPlayerInfo->selected_character_id
			                      : cd.PlayerInfo.selected_character_id;
		}
		return 0;
	}

	// All disregard_quest_flag rows of the group hold for the character.
	bool VolumeConditionsMet(int64_t characterId, int questGroupId) {
		sqlite3* db = Database::GetConnection();
		if (!db) return true;
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db,
				"SELECT COUNT(*) FROM asm_data_set_quest_group_lists l "
				"LEFT JOIN ga_character_quests q ON q.quest_id = l.quest_id AND q.character_id = ?1 "
				"WHERE l.quest_group_id = ?2 AND l.disregard_quest_flag = 1 AND ("
				" (l.quest_status_type_value_id = 1618 AND q.status IS NOT 'complete') OR"
				" (l.quest_status_type_value_id = 1619 AND q.status IS NOT 'active') OR"
				" (l.quest_status_type_value_id = 1620 AND q.status IS NULL) OR"
				" (l.quest_status_type_value_id = 1621 AND q.status IS 'complete') OR"
				" (l.quest_status_type_value_id = 1625 AND q.status IS 'active') OR"
				" (l.quest_status_type_value_id = 1626 AND q.status IS NOT NULL))",
				-1, &st, nullptr) != SQLITE_OK || !st) {
			return true;  // old DB without the column: keep the old answer
		}
		sqlite3_bind_int64(st, 1, characterId);
		sqlite3_bind_int(st, 2, questGroupId);
		int fails = 0;
		if (sqlite3_step(st) == SQLITE_ROW) fails = sqlite3_column_int(st, 0);
		sqlite3_finalize(st);
		return fails == 0;
	}
}

bool __fastcall TgPlayerController__CanPlayerUseVolume::Call(ATgPlayerController* PlayerController, void* edx, ATgOmegaVolume* Volume) {
	if (!PlayerController || !Volume || !KillCredit::InOpenZone()) return true;
	const AmUiVolumeRowHead* row = reinterpret_cast<const AmUiVolumeRowHead*>(
		static_cast<uintptr_t>(static_cast<uint32_t>(Volume->m_pAmVolume.Dummy)));
	if (!row || row->quest_group_id <= 0) return true;
	const int64_t characterId = CharacterIdOfController(PlayerController);
	if (characterId <= 0) return true;

	const bool ok = row->volume_type_value_id == kVolumeTypeMapTransition
		? Database::MapDoorOpen(characterId, Volume->m_nOmegaAlertId, row->quest_group_id)
		: VolumeConditionsMet(characterId, row->quest_group_id)
			&& !(Volume->m_nOmegaAlertId == kNorthernGateGuardVolume && Database::DomeExitCleared(characterId));
	Logger::Log("omegavolume", "[CanPlayerUseVolume] vol=%d type=%d group=%d char=%lld -> %d\n",
		Volume->m_nOmegaAlertId, row->volume_type_value_id, row->quest_group_id,
		(long long)characterId, (int)ok);
	return ok;
}
