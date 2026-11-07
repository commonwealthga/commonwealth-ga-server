#pragma once
// Server-side skill-tree rules, shared by the control server
// (skill_save -> DB) and the game DLL (in-instance apply). Data = the game's
// own asm skill tables; budget = SUM(asm_data_set_xp_levels.skill_points) up
// to the agent's level, i.e. what the client UI shows as "0/N pts Allocated".
// Checks: class primary trees only, ranks within rank count / level / not
// auto-allocated, prerequisite skill points, points-in-tree tier gate.

#include "sqlite3.h"
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace SkillRules {

struct SkillPick { int skill_group_id; int skill_id; int points; };

inline bool ValidateSkillAllocation(sqlite3* db, int64_t character_id,
                                    const std::vector<SkillPick>& skills,
                                    std::string& why) {
	if (skills.empty()) return true;
	if (!db) { why = "no db"; return false; }

	auto scalar = [&](const char* sql, std::initializer_list<int64_t> args, int64_t dflt) {
		int64_t v = dflt;
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return dflt;
		int i = 1;
		for (int64_t a : args) sqlite3_bind_int64(s, i++, a);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL)
			v = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
		return v;
	};

	int64_t lv = scalar(
		"SELECT COALESCE((SELECT MAX(l.level) FROM asm_data_set_xp_levels l "
		"WHERE l.xp_value <= c.xp), 1) FROM ga_characters c WHERE c.id = ?",
		{character_id}, 1);
	const uint32_t level = lv < 1 ? 1u : (uint32_t)lv;
	const int64_t budget = scalar(
		"SELECT COALESCE(SUM(skill_points),0) FROM asm_data_set_xp_levels WHERE level <= ?",
		{(int64_t)level}, 0);
	const int64_t profile = scalar(
		"SELECT profile_id FROM ga_characters WHERE id = ?", {character_id}, 0);

	std::map<std::pair<int,int>, int> pts;
	std::map<int, int> group_pts;
	int64_t total = 0;
	for (const auto& r : skills) {
		if (r.points <= 0) continue;
		pts[{r.skill_group_id, r.skill_id}] += r.points;
		group_pts[r.skill_group_id] += r.points;
		total += r.points;
	}
	char buf[160];
	if (total > budget) {
		snprintf(buf, sizeof buf, "total %lld > budget %lld at level %u",
			(long long)total, (long long)budget, level);
		why = buf; return false;
	}
	for (const auto& [key, p] : pts) {
		const int g = key.first, sk = key.second;
		// Tree must be one of this class's player-allocated (primary) trees.
		if (!scalar("SELECT 1 FROM asm_data_set_skill_group_set_groups sgs "
		            "JOIN asm_data_set_skill_group_sets st ON st.skill_group_set_id = sgs.skill_group_set_id "
		            "WHERE st.bot_id = ? AND sgs.skill_group_id = ? AND sgs.primary_flag = 1",
		            {profile, g}, 0)) {
			snprintf(buf, sizeof buf, "group %d not a primary tree of profile %lld", g, (long long)profile);
			why = buf; return false;
		}
		sqlite3_stmt* s = nullptr;
		int pre_group = 0, pre_skill = 0, pre_skill_pts = 0; bool found = false;
		if (sqlite3_prepare_v2(db,
		        "SELECT prereq_group_points, prereq_skill_id, prereq_skill_points "
		        "FROM asm_data_set_skill_group_skills WHERE skill_group_id = ? AND skill_id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, g); sqlite3_bind_int(s, 2, sk);
			if (sqlite3_step(s) == SQLITE_ROW) {
				found = true;
				pre_group = sqlite3_column_int(s, 0);
				pre_skill = sqlite3_column_int(s, 1);
				pre_skill_pts = sqlite3_column_int(s, 2);
			}
			sqlite3_finalize(s);
		}
		if (!found) { snprintf(buf, sizeof buf, "unknown skill %d/%d", g, sk); why = buf; return false; }
		// Trainable ranks 1..p must exist, not be auto-allocated, and be within level.
		const int64_t ok_ranks = scalar(
			"SELECT COUNT(*) FROM asm_data_set_skill_group_ranks r "
			"LEFT JOIN asm_data_set_xp_levels l ON l.xp_level_id = r.required_xp_level_id "
			"WHERE r.skill_group_id = ? AND r.skill_id = ? AND r.rank BETWEEN 1 AND ? "
			"AND COALESCE(r.auto_allocate_flag,0) = 0 AND COALESCE(l.level,1) <= ?",
			{g, sk, p, (int64_t)level}, 0);
		if (ok_ranks < p) {
			snprintf(buf, sizeof buf, "skill %d/%d: %d points, only %lld ranks trainable at level %u",
				g, sk, p, (long long)ok_ranks, level);
			why = buf; return false;
		}
		// Tier gate: points spent on OTHER skills in the same tree.
		if (group_pts[g] - p < pre_group) {
			snprintf(buf, sizeof buf, "skill %d/%d needs %d points in tree, has %d",
				g, sk, pre_group, group_pts[g] - p);
			why = buf; return false;
		}
		if (pre_skill && pre_skill_pts > 0) {
			auto it = pts.find({g, pre_skill});
			const int have = it == pts.end() ? 0 : it->second;
			if (have < pre_skill_pts) {
				snprintf(buf, sizeof buf, "skill %d/%d needs %d points in skill %d",
					g, sk, pre_skill_pts, pre_skill);
				why = buf; return false;
			}
		}
	}
	return true;
}

} // namespace SkillRules
