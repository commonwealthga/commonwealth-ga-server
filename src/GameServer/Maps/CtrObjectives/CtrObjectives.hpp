#pragma once

#include <string>
#include <vector>

// Custom objective positions surveyed on the CTR_* maps (via the -coords
// command), for repurposing them under a custom game mode — point rotation,
// ticket, control point, whatever we land on. This is JUST the survey data: an
// ordered list of world-space points per map. What's done with them, and the
// gating on which mode is active (so the original CTR mode never picks them up),
// lives in the consuming mode logic — not here.
//
// To add/adjust points, edit the per-map lists below. No DB, no migration.
namespace CtrObjectives {

struct Vec3 { float x, y, z; };

// Ordered objective positions for a CTR map, or empty if none surveyed yet.
inline std::vector<Vec3> ForMap(const std::string& mapName) {
	if (mapName == "CTR_Recursive_P") {
		return {
			{ -576.0f,  527.0f, -3522.0f },  // central column
			{ -1310.0f, 533.0f, -2351.0f },  // crate room (one side)
			{ 1531.0f,  513.0f, -2511.0f },  // crate room (other side)
			{ -994.0f,  512.0f, -3019.0f },  // crate (mid)
		};
	}
	if (mapName == "CTR_DuelStrike3_P") {  // "Hart Station"
		return {
			{ 7155.0f,  665.0f, 6113.0f },
			{ 425.0f,   665.0f, 6113.0f },
			{ -1485.0f, 665.0f, 5907.0f },
			{ -1225.0f, 665.0f, 6371.0f },
		};
	}
	if (mapName == "CTR_DuelStrike2_P") {
		return {
			{ 7751.0f,  172.0f, 6113.0f },
			{ -1516.0f, 172.0f, 6113.0f },
			{ -43.0f,   172.0f, 6370.0f },
		};
	}
	if (mapName == "CTR_DuelStrike_P") {  // "Kimerial Point"
		return {
			{ -2199.0f, -400.0f, 6113.0f },
			{ -919.0f,  -400.0f, 6373.0f },
			{ 8251.0f,  -400.0f, 6113.0f },
		};
	}
	return {};
}

}  // namespace CtrObjectives
