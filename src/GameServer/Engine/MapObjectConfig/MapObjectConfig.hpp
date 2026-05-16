#pragma once
#include <string>

// Runtime registry for the `map_object_config` table (v34). Layered on top of
// the raw `map_*` dump from MapDataDumper.
//
// `Init()` rebuilds the in-memory map from DB:
//   - Static rows (variant_group IS NULL) are registered directly.
//   - Variant rows are grouped by (map_object_id, variant_group); for each
//     group, one variant_id is weighted-randomly picked and all of its rows
//     applied. Re-rolls every Init() call — so calling Init() on each map
//     load gives session-to-session variety.
//
// Caller pattern (typically inside LoadObjectConfig hooks):
//   int tf = MapObjectConfig::GetInt(mapObjectId, "s_n_task_force", defaultTf);
//   actor->s_nTaskForce = tf;
//
// All values stored as TEXT internally; Get* parses on each call. If the
// column has no override, or the value fails to parse, defaultValue is
// returned — caller never needs to null-check.
class MapObjectConfig {
public:
	// Re-roll all variant groups and rebuild the registry. Call once on map
	// load (e.g. from World::BeginPlay before original CallOriginal).
	static void Init();

	// True iff this map_object_id has any registered value for `column`.
	static bool Has(int mapObjectId, const char* column);

	// Parse the registered value as the requested type. If no override exists
	// or the value can't be parsed, returns `defaultValue`.
	static int         GetInt   (int mapObjectId, const char* column, int defaultValue);
	static float       GetFloat (int mapObjectId, const char* column, float defaultValue);
	static std::string GetString(int mapObjectId, const char* column, const std::string& defaultValue);
};
