#pragma once
// Rotating vendor stock, shared by the control server (vendor
// list sent to the client) and the game DLL (vendor purchase check).
// In the game data some Dome City vendors point at a "LIVE" loot table that
// has no rows (508 "Burning Fashion LIVE", 498 "*Vendor - Rare Goods LIVE"):
// Hi-Rez's backend filled it from one of ten sets A..J that rotated. We serve
// one set per week, switching Monday 00:00 UTC, deterministic from the date so
// every player and every restart sees the same stock.
// Pin a set with env GA_VENDOR_ROTATION_PIN=0..9 (A..J) on the control server (the
// instances it spawns inherit it).

#include <cstdlib>
#include <ctime>

namespace VendorRotation {

struct Group { int live_table; int sets[10]; };

// Sets in letter order A..J (table ids are not in letter order in the data).
constexpr Group kGroups[] = {
	{ 508, { 519, 510, 511, 512, 513, 514, 515, 516, 517, 518 } },  // Burning Fashion A..J
	{ 498, { 520, 525, 521, 526, 522, 527, 523, 528, 524, 529 } },  // Rare Goods A..J
};

// 0..9 = A..J for the week containing t (weeks start Monday 00:00 UTC;
// 1970-01-01 was a Thursday, hence +3 days).
inline int SetIndex(std::time_t t) {
	if (const char* pin = std::getenv("GA_VENDOR_ROTATION_PIN")) {
		if (pin[0] >= '0' && pin[0] <= '9' && pin[1] == 0) return pin[0] - '0';
	}
	long long days = (long long)t / 86400;
	long long week = (days + 3) / 7;
	return (int)(week % 10);
}

// The table whose rows a vendor really sells right now: a LIVE table maps to
// this week's set, anything else is itself.
inline int Resolve(int loot_table_id, std::time_t t = std::time(nullptr)) {
	for (const auto& g : kGroups)
		if (g.live_table == loot_table_id) return g.sets[SetIndex(t)];
	return loot_table_id;
}

// True for a set table (A..J) of a rotating group. Purchases must name the
// LIVE table, so a request quoting a set directly (e.g. last week's) is refused.
inline bool IsSetTable(int loot_table_id) {
	for (const auto& g : kGroups)
		for (int s : g.sets) if (s == loot_table_id) return true;
	return false;
}

} // namespace VendorRotation
