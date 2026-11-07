#include "src/ControlServer/AvaZones/AvaZones.hpp"
#include "src/ControlServer/AvaZones/AvaBoards.hpp"
#include "src/ControlServer/Database/Database.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <map>
#include <mutex>
#include <sqlite3.h>

namespace AvaZones {
namespace {
// Client texts: 59847 NA Sovereign (the zone Steam news 4416 names), 59849 NA Totalize, 59839 EU Echelon, 59841 EU Rampart,
// 59843 PAC Dauntless, 59845 PAC Warlord; each message's next id is its "150 Hexes" / "75 Hexes" description.
// Zone 1 is the old "War Zone".
constexpr Zone kZones[] = {
	{1, 59847, 59848, 150, "NA Sovereign"},
	{2, 59849, 59850, 150, "NA Totalize"},
	{3, 59839, 59840, 150, "EU Echelon"},
	{4, 59841, 59842, 150, "EU Rampart"},
	{5, 59843, 59844, 75, "PAC Dauntless"},
	{6, 59845, 59846, 75, "PAC Warlord"},
};
constexpr int kMax = (int)(sizeof kZones / sizeof kZones[0]);
}  // namespace

int Count() {
	static int cached = kMax;
	static int64_t read_at = 0;
	const int64_t now = (int64_t)std::time(nullptr);
	if (now - read_at < 60) return cached;
	read_at = now;
	cached = kMax;
	if (sqlite3* db = Database::GetConnection()) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'ava_zones'", -1, &s, nullptr) == SQLITE_OK) {
			if (sqlite3_step(s) == SQLITE_ROW) {
				const char* v = (const char*)sqlite3_column_text(s, 0);
				if (v) cached = std::min(kMax, std::max(1, std::atoi(v)));
			}
			sqlite3_finalize(s);
		}
	}
	return cached;
}

const Zone* Find(uint32_t zone_id) {
	return zone_id >= 1 && (int)zone_id <= Count() ? &kZones[zone_id - 1] : nullptr;
}

int Region(uint32_t zone_id, uint32_t local) {
	if (zone_id < 1 || zone_id > 6 || local >= kW * kH) return 0;
	const char c = kBoards[zone_id - 1].rows[local / kW][local % kW];
	return c == '.' ? 0 : c - '0';
}

uint32_t Background(uint32_t zone_id) {
	return zone_id >= 1 && zone_id <= 6 ? kBoards[zone_id - 1].background_res : 0;
}

bool Valid(uint32_t g) {
	return g != kBad && Find(ZoneOf(g)) && Region(ZoneOf(g), Local(g)) != 0;
}

uint32_t ToGlobal(uint32_t zone_id, uint32_t local) {
	return Find(zone_id) && Region(zone_id, local) != 0 ? Base(zone_id) + local : kBad;
}

std::string Label(uint32_t g) {
	const Zone* z = g == kBad ? nullptr : Find(ZoneOf(g));
	if (!z || z->id == 1) return std::to_string(z ? Local(g) : g);
	return std::to_string(Local(g)) + " in " + z->name;
}
static std::mutex g_view_mu;
static std::map<std::string, uint32_t> g_view;

void SetView(const std::string& guid, uint32_t zone) {
	std::lock_guard<std::mutex> l(g_view_mu);
	g_view[guid] = zone;
}

uint32_t ViewOf(const std::string& guid) {
	std::lock_guard<std::mutex> l(g_view_mu);
	const auto it = g_view.find(guid);
	return it != g_view.end() && Find(it->second) ? it->second : 1;
}
static std::map<std::string, uint32_t> g_battle;
static std::mutex g_battle_mu;
void SetBattle(const std::string& guid, uint32_t g) { std::lock_guard<std::mutex> l(g_battle_mu); g_battle[guid] = g; }
uint32_t BattleOf(const std::string& guid) {
	std::lock_guard<std::mutex> l(g_battle_mu);
	const auto it = g_battle.find(guid);
	return it != g_battle.end() ? it->second : kBad;
}

}  // namespace AvaZones
