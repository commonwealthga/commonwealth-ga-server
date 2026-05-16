#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <cstdlib>
#include <ctime>
#include <unordered_map>
#include <vector>

namespace {

// map_object_id → column_name → resolved value (as TEXT). Parsing happens
// per Get* call; cheap, and lets one row serve int/float/string callers.
std::unordered_map<int, std::unordered_map<std::string, std::string>> g_overrides;

// Raw row as loaded from `map_object_config`. variantGroup/variantId empty
// strings denote NULL columns; weight defaults to 1.0 in SQL.
struct ConfigRow {
	int         mapObjectId;
	std::string columnName;
	std::string value;
	std::string variantGroup;  // "" if NULL = always applied
	std::string variantId;     // "" if NULL — paired with variantGroup ""
	float       weight;
};

// Variant bucket: rows sharing the same (mapObjectId, variantGroup, variantId).
// All rows in a bucket apply atomically if this variantId wins its group.
struct Bucket {
	float weight = 0.0f;  // first seen wins (rows of one variant share weight)
	std::vector<const ConfigRow*> rows;
};

// Group key: (mapObjectId, variantGroup). Buckets under this key compete;
// one wins.
struct GroupKey {
	int         mapObjectId;
	std::string variantGroup;
	bool operator==(const GroupKey& o) const {
		return mapObjectId == o.mapObjectId && variantGroup == o.variantGroup;
	}
};
struct GroupKeyHash {
	std::size_t operator()(const GroupKey& k) const {
		return std::hash<int>()(k.mapObjectId) ^ (std::hash<std::string>()(k.variantGroup) << 1);
	}
};

const char* SafeText(sqlite3_stmt* stmt, int col) {
	const unsigned char* t = sqlite3_column_text(stmt, col);
	return t ? reinterpret_cast<const char*>(t) : "";
}

}  // namespace

void MapObjectConfig::Init() {
	g_overrides.clear();

	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("config", "MapObjectConfig::Init — DB connection unavailable\n");
		return;
	}

	sqlite3_stmt* stmt = nullptr;
	const char* kSql =
		"SELECT map_object_id, column_name, value, "
		"       COALESCE(variant_group, ''), COALESCE(variant_id, ''), weight "
		"FROM map_object_config";
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("config", "MapObjectConfig::Init prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}

	std::vector<ConfigRow> allRows;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		ConfigRow r;
		r.mapObjectId  = sqlite3_column_int   (stmt, 0);
		r.columnName   = SafeText             (stmt, 1);
		r.value        = SafeText             (stmt, 2);
		r.variantGroup = SafeText             (stmt, 3);
		r.variantId    = SafeText             (stmt, 4);
		r.weight       = static_cast<float>(sqlite3_column_double(stmt, 5));
		allRows.push_back(std::move(r));
	}
	sqlite3_finalize(stmt);

	// Static rows go straight into the registry. Variant rows are bucketed by
	// (mapObjectId, variantGroup, variantId); after all rows are scanned we
	// pick one variantId per (mapObjectId, variantGroup) weighted-randomly.
	std::unordered_map<GroupKey,
		std::unordered_map<std::string, Bucket>,
		GroupKeyHash> groups;

	int staticCount = 0;
	for (const auto& r : allRows) {
		if (r.variantGroup.empty()) {
			g_overrides[r.mapObjectId][r.columnName] = r.value;
			staticCount++;
		} else {
			Bucket& b = groups[GroupKey{r.mapObjectId, r.variantGroup}][r.variantId];
			if (b.rows.empty()) b.weight = r.weight;  // first row sets the bucket weight
			b.rows.push_back(&r);
		}
	}

	// Seed once per Init() so calling Init() twice in the same second still
	// rolls differently (rand_r would be nicer but srand+rand is plenty here).
	static bool seeded = false;
	if (!seeded) { srand(static_cast<unsigned>(time(nullptr))); seeded = true; }

	int variantCount = 0;
	for (auto& kv : groups) {
		auto& variantMap = kv.second;
		if (variantMap.empty()) continue;

		float totalWeight = 0.0f;
		for (const auto& v : variantMap) totalWeight += v.second.weight;
		if (totalWeight <= 0.0f) continue;

		float pick = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * totalWeight;
		float accum = 0.0f;
		const Bucket* chosen = nullptr;
		for (const auto& v : variantMap) {
			accum += v.second.weight;
			if (pick <= accum) { chosen = &v.second; break; }
		}
		// Floating-point edge — fall back to first bucket if rounding skipped us.
		if (!chosen) chosen = &variantMap.begin()->second;

		for (const ConfigRow* r : chosen->rows) {
			g_overrides[r->mapObjectId][r->columnName] = r->value;
			variantCount++;
		}
	}

	Logger::Log("config",
		"MapObjectConfig::Init — loaded %d static + %d variant overrides across %zu objects\n",
		staticCount, variantCount, g_overrides.size());
}

bool MapObjectConfig::Has(int mapObjectId, const char* column) {
	auto it = g_overrides.find(mapObjectId);
	if (it == g_overrides.end()) return false;
	return it->second.find(column) != it->second.end();
}

int MapObjectConfig::GetInt(int mapObjectId, const char* column, int defaultValue) {
	auto it = g_overrides.find(mapObjectId);
	if (it == g_overrides.end()) return defaultValue;
	auto colIt = it->second.find(column);
	if (colIt == it->second.end()) return defaultValue;
	try {
		return std::stoi(colIt->second);
	} catch (...) {
		Logger::Log("config",
			"MapObjectConfig::GetInt — failed to parse '%s' for map_object_id=%d column=%s; using default %d\n",
			colIt->second.c_str(), mapObjectId, column, defaultValue);
		return defaultValue;
	}
}

float MapObjectConfig::GetFloat(int mapObjectId, const char* column, float defaultValue) {
	auto it = g_overrides.find(mapObjectId);
	if (it == g_overrides.end()) return defaultValue;
	auto colIt = it->second.find(column);
	if (colIt == it->second.end()) return defaultValue;
	try {
		return std::stof(colIt->second);
	} catch (...) {
		Logger::Log("config",
			"MapObjectConfig::GetFloat — failed to parse '%s' for map_object_id=%d column=%s; using default %g\n",
			colIt->second.c_str(), mapObjectId, column, (double)defaultValue);
		return defaultValue;
	}
}

std::string MapObjectConfig::GetString(int mapObjectId, const char* column, const std::string& defaultValue) {
	auto it = g_overrides.find(mapObjectId);
	if (it == g_overrides.end()) return defaultValue;
	auto colIt = it->second.find(column);
	if (colIt == it->second.end()) return defaultValue;
	return colIt->second;
}
