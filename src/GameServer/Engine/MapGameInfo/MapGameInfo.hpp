#pragma once
#include <cstdint>
#include <optional>
#include <string>

// Game-server-side accessor for the `map_game_info` table — the per-map
// override that lets two maps of the same TgGame class diverge on knobs the
// engine would otherwise treat as class-scoped (mission time, PvP flag).
//
// Shape: a single SELECT against the current map name on demand. LoadGameConfig
// and InitGameRepInfo each fire once per match, so we don't bother caching.
// Returns std::nullopt iff there's no row for the map — callers MUST fall back
// to their old hardcoded/class-based behavior in that case.
//
// Defaults inside the DB row (when present): mission_time_secs=900, is_pvp=0,
// overtime_secs=0, allow_overtime=false.
struct MapGameInfoRow {
	int  map_game_id;
	int  mission_time_secs;
	bool is_pvp;
	int  overtime_secs;
	bool allow_overtime;
	// The real differentiator between game modes that share a TgGame class --
	// e.g. TgGame_Mission covers both Breach (1544) and solo/PvE missions
	// (1553); TgGame_PointRotation covers both standard (1548) and small-map
	// (1569) Scramble. 0 = column absent on this DB (older schema -- this
	// repo's own dev copy predates it) or no row; callers must fall back to
	// class-name-only detection in that case. See MissionProgressFeed.cpp's
	// ResolveMissionMode for the full value -> mode mapping.
	int  gameplay_type_value_id = 0;
};

class MapGameInfo {
public:
	// Two rows can share a map_name across game classes (e.g. a stock CTR row
	// plus a custom PointRotation row). Prefer the row whose game_class equals
	// `gameMode` (the "TgGame.TgGame_X" string, no "Class " prefix); fall back to
	// any name match when gameMode is empty or unmatched.
	static std::optional<MapGameInfoRow> LookupByNameAndGameMode(
		const std::string& mapName, const std::string& gameMode);

	// Name-only convenience for callers without a game-class context. Equivalent
	// to LookupByNameAndGameMode(mapName, "") — first name match wins.
	static std::optional<MapGameInfoRow> LookupByName(const std::string& mapName);
};
