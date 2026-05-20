#include "src/GameServer/TgGame/TgDroppedItem/ApplyItemSetup/TgDroppedItem__ApplyItemSetup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <sqlite3.h>

bool __fastcall TgDroppedItem__ApplyItemSetup::Call(ATgDroppedItem* Item, void* edx) {
	if (!Item) return false;

	// Native original handles m_fLifeSpan (time_to_live_secs) and c_Mesh
	// attachment from the item resource. If it returns false (item_id 0 or
	// missing asm row) we bail out — nothing useful to do.
	const bool baseOk = CallOriginal(Item, edx);
	if (!baseOk) {
		Logger::Log("debug",
			"[TgDroppedItem::ApplyItemSetup] base setup failed for r_nItemId=%d\n",
			Item->r_nItemId);
		return false;
	}

	const int itemId = Item->r_nItemId;
	if (itemId <= 0) return true;  // nothing to populate; not an error

	// If the list is already populated (e.g. server pre-seeded then client
	// fires ReplicatedEvent and ends up here again), skip to keep this
	// idempotent. UTgEffectGroup allocations are not cheap.
	if (Item->s_EffectGroupList.Count > 0) {
		return true;
	}

	// Pull (effect_group_id, effect_group_type_value_id) rows from the DB. UC's
	// GiveTo only iterates type=264 (aura) groups, but we populate ALL types so
	// future callers can find non-aura entries too (e.g. type=0 group 4129 on
	// item 1509 — currently unused by GiveTo but legitimate data).
	sqlite3* db = Database::GetConnection();
	if (!db) return true;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT effect_group_id, effect_group_type_value_id "
		"FROM asm_data_set_item_effect_groups WHERE item_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		Logger::Log("debug",
			"[TgDroppedItem::ApplyItemSetup] sqlite prepare failed rc=%d\n", rc);
		return true;
	}
	sqlite3_bind_int(stmt, 1, itemId);

	int added = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int egId   = sqlite3_column_int(stmt, 0);
		int egType = sqlite3_column_int(stmt, 1);

		UTgEffectGroup* eg = BuildEffectGroup(egId, egType);
		if (!eg) {
			Logger::Log("debug",
				"[TgDroppedItem::ApplyItemSetup] BuildEffectGroup(%d, %d) returned null\n",
				egId, egType);
			continue;
		}

		// Anchor the effect group to the picker pawn at apply time (UC's GiveTo
		// passes the pawn into ProcessEffect; m_Target is wired there). Leave
		// m_Instigator null — the pickup is a self-cast from the picker's POV.
		Item->s_EffectGroupList.Add(eg);
		++added;
	}
	sqlite3_finalize(stmt);

	Logger::Log("debug",
		"[TgDroppedItem::ApplyItemSetup] item_id=%d populated s_EffectGroupList with %d entries\n",
		itemId, added);
	return true;
}
