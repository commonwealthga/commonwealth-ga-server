#include "src/ControlServer/CombatRating/CombatRating.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "sqlite3.h"

#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/MmrService/MmrService.hpp"
#include "src/Shared/EconRules2.hpp"

namespace CombatRating {

namespace {

const char* ClassName(int profile_id) {  // same names as MmrService's history rows
	switch (profile_id) {
		case 680: return "Assault";
		case 567: return "Medic";
		case 681: return "Recon";
		case 679: return "Robotic";
		default:  return nullptr;
	}
}

double PerStar(sqlite3* db) {
	double v = 100.0;
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'combat_rating_per_star'",
	        -1, &st, nullptr) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW) {
		const double x = sqlite3_column_double(st, 0);
		if (x > 0.0) v = x;
	}
	if (st) sqlite3_finalize(st);
	return v;
}

}  // namespace

float Stars(int64_t character_id, uint32_t level) {
	float stars = 0.0f;
	sqlite3* db = Database::GetConnection();
	if (db && character_id > 0) {
		int64_t user_id = 0;
		int profile_id = 0;
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
		        -1, &st, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(st, 1, character_id);
			if (sqlite3_step(st) == SQLITE_ROW) {
				user_id = sqlite3_column_int64(st, 0);
				profile_id = sqlite3_column_int(st, 1);
			}
		}
		if (st) sqlite3_finalize(st);
		const char* cls = ClassName(profile_id);
		if (user_id > 0 && cls) {
			const std::string table = MmrService::GetActiveEngine() == "perf" ? "ga_mmr_history" : "ga_wl_mmr_history";
			int games = 0;
			st = nullptr;
			if (sqlite3_prepare_v2(db, ("SELECT COUNT(*) FROM " + table + " WHERE user_id = ? AND class_name = ?").c_str(),
			        -1, &st, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(st, 1, user_id);
				sqlite3_bind_text(st, 2, cls, -1, SQLITE_STATIC);
				if (sqlite3_step(st) == SQLITE_ROW) games = sqlite3_column_int(st, 0);
			}
			if (st) sqlite3_finalize(st);
			if (games > 0) {
				const double rating = MmrService::GetCurrentRating(user_id, (uint32_t)profile_id);
				const double raw = 2.5 + (rating - 1000.0) / PerStar(db);
				stars = (float)(std::round(raw * 2.0) / 2.0);
			}
		}
	}
	if (level >= 30) stars = std::max(stars, 2.0f);
	stars = std::clamp(stars, 0.0f, 5.0f);
	EconRules2::RememberStars(db, character_id, stars);  // instance vendor discount reads it
	return stars;
}

}  // namespace CombatRating
