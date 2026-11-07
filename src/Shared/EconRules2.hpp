#pragma once
// Two original economy rules (crafting skill gain, combat rating store discount) as
// small shared primitives (control server + instance). Switches/rates live in
// cs_settings so an admin can tune them without a build.
#include "sqlite3.h"
#include <algorithm>
#include <cstdint>
#include <string>

namespace EconRules2 {

inline int SettingInt(sqlite3* db, const char* key, int def) {
	int v = def;
	sqlite3_stmt* st = nullptr;
	if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?1", -1, &st, nullptr) == SQLITE_OK && st) {
		sqlite3_bind_text(st, 1, key, -1, SQLITE_STATIC);
		if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) v = sqlite3_column_int(st, 0);
	}
	if (st) sqlite3_finalize(st);
	return v;
}

// ---- Crafting skill gain (control server, Database::CraftItem) ----
// Original (wiki Crafting, 2010): "If the Mod was of sufficient difficulty, the
// Agent may also gain a point of crafting skill." No formula survives.
// Reconstruction: +craft_skill_gain (1) point per craft while the agent's skill
// is below the blueprint's required skill + craft_skill_window (100 = one
// quality tier). Uncommon blueprints (need 0-4) lift an agent to ~100 (the Rare
// gate), Rare (100) to 200 (the Epic gate), Epic (200) to the cap. Easier
// blueprints give nothing.
inline int CraftSkillGain(sqlite3* db, int skill_min, int points) {
	const int gain = std::max(0, SettingInt(db, "craft_skill_gain", 1));
	const int window = std::max(0, SettingInt(db, "craft_skill_window", 100));
	return points < skill_min + window ? gain : 0;
}

// ---- Combat rating store discount ----
// Original (wiki Combat_Rating, 2010): 3 stars 5%, 4 stars 20%, 4.5 stars 25%
// discount on store purchases; client text msg 39146 "You received a
// @@DISCOUNT@@% discount based on your combat rating!". 5 stars (not listed)
// keeps the 4.5-star 25%. Credit prices only (tokens/AP pay full).
// The control server computes the stars (CombatRating::Stars, PvP MMR) and
// remembers the last value per agent here; the instance reads it at purchase.
inline void RememberStars(sqlite3* db, int64_t character_id, float stars) {
	if (!db || character_id <= 0) return;
	static bool schema = false;
	if (!schema) {
		schema = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ga_character_combat_stars ("
			"character_id INTEGER PRIMARY KEY, stars REAL NOT NULL DEFAULT 0, updated_at INTEGER NOT NULL DEFAULT 0);",
			nullptr, nullptr, nullptr) == SQLITE_OK;
	}
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db,
			"INSERT INTO ga_character_combat_stars (character_id, stars, updated_at) VALUES (?1, ?2, strftime('%s','now')) "
			"ON CONFLICT(character_id) DO UPDATE SET stars = excluded.stars, updated_at = excluded.updated_at "
			"WHERE stars <> excluded.stars", -1, &st, nullptr) == SQLITE_OK && st) {
		sqlite3_bind_int64(st, 1, character_id);
		sqlite3_bind_double(st, 2, (double)stars);
		sqlite3_step(st);
	}
	if (st) sqlite3_finalize(st);
}

inline int DiscountForStars(double stars) {
	return stars >= 4.5 ? 25 : stars >= 4.0 ? 20 : stars >= 3.0 ? 5 : 0;
}

// Percent off a vendor purchase for this agent in this currency (1603 = credits).
inline int StoreDiscount(sqlite3* db, int64_t character_id, int currency_type) {
	if (!db || character_id <= 0 || currency_type != 1603) return 0;
	if (SettingInt(db, "combat_rating_discount_enabled", 1) == 0) return 0;
	double stars = 0.0;
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT stars FROM ga_character_combat_stars WHERE character_id = ?1",
			-1, &st, nullptr) == SQLITE_OK && st) {
		sqlite3_bind_int64(st, 1, character_id);
		if (sqlite3_step(st) == SQLITE_ROW) stars = sqlite3_column_double(st, 0);
	}
	if (st) sqlite3_finalize(st);
	return DiscountForStars(stars);
}

// The client's own line (msg 39146) with @@DISCOUNT@@ filled in.
inline std::wstring DiscountText(sqlite3* db, int percent) {
	std::string t = "You received a @@DISCOUNT@@% discount based on your combat rating!";
	sqlite3_stmt* st = nullptr;
	if (db && sqlite3_prepare_v2(db, "SELECT message FROM asm_data_set_msg_translations WHERE msg_id = 39146 LIMIT 1",
			-1, &st, nullptr) == SQLITE_OK && st) {
		if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_text(st, 0))
			t = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
	}
	if (st) sqlite3_finalize(st);
	const std::string tag = "@@DISCOUNT@@";
	const size_t p = t.find(tag);
	if (p != std::string::npos) t.replace(p, tag.size(), std::to_string(percent));
	return std::wstring(t.begin(), t.end());
}

}  // namespace EconRules2
