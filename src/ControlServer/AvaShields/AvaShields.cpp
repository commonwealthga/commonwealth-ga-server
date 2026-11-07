#include "src/ControlServer/AvaShields/AvaShields.hpp"
#include "src/ControlServer/AvaZones/AvaZones.hpp"
#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <map>
#include <set>
#include <vector>

// See AvaShields.hpp.
namespace AvaShields {
namespace {

constexpr int kW = (int)AvaZones::kW, kH = (int)AvaZones::kH;  // board width x height
constexpr int64_t kSoloOwnerBase = 0x40000000;
constexpr uint32_t kMissileComplexBuilding = 15, kMissileComplexDeed = 5075, kDefenseBuilding = 35;

void EnsureSchema(sqlite3* db) {
	static bool done = false;
	if (done) return;
	done = true;
	sqlite3_exec(db,
		"CREATE TABLE IF NOT EXISTS ga_ava_shields (zone_id INTEGER NOT NULL, position INTEGER NOT NULL,"
		" owner_agency_id INTEGER NOT NULL, flags INTEGER NOT NULL, set_at INTEGER NOT NULL, PRIMARY KEY (zone_id, position));"
		"CREATE TABLE IF NOT EXISTS ga_ava_missile_picks (character_id INTEGER PRIMARY KEY, position INTEGER NOT NULL,"
		" item_id INTEGER NOT NULL, set_at INTEGER NOT NULL);"
		"CREATE TABLE IF NOT EXISTS ga_ava_shield_breaches (zone_id INTEGER NOT NULL, position INTEGER NOT NULL,"
		" owner_agency_id INTEGER NOT NULL, item_id INTEGER NOT NULL, until INTEGER NOT NULL,"
		" PRIMARY KEY (zone_id, position, owner_agency_id));",
		nullptr, nullptr, nullptr);
}

std::string Setting(sqlite3* db, const char* key, const char* def) {
	std::string v = def;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, key, -1, SQLITE_STATIC);
		if (sqlite3_step(s) == SQLITE_ROW) { const char* c = (const char*)sqlite3_column_text(s, 0); if (c && *c) v = c; }
		sqlite3_finalize(s);
	}
	return v;
}
double SettingNum(sqlite3* db, const char* key, double def) {
	const std::string v = Setting(db, key, "");
	if (v.empty()) return def;
	char* end = nullptr;
	const double d = strtod(v.c_str(), &end);
	return end && end != v.c_str() ? d : def;
}
bool Enabled(sqlite3* db) { return Setting(db, "ava_shields", "on") != "off"; }

int64_t Scalar(sqlite3* db, const char* sql, int64_t a, int64_t b = 0, int64_t def = 0) {
	int64_t v = def;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, a);
		sqlite3_bind_int64(s, 2, b);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL) v = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	return v;
}
int64_t OwnerOf(sqlite3* db, uint32_t pos) {
	return Scalar(db, "SELECT owner_agency_id FROM ga_ava_territories WHERE zone_id = 1 AND position = ?1", pos);
}
int64_t OwnerOfCharacter(sqlite3* db, int64_t cid) {
	return Scalar(db, "SELECT COALESCE(m.agency_id, ?2 + c.id) FROM ga_characters c "
		"LEFT JOIN ga_agency_members m ON m.user_id = c.user_id WHERE c.id = ?1", cid, kSoloOwnerBase);
}
// Leader or AGENCY_PERM_FACILITY_MGMT (same rule as give/abandon); solo owners always.
bool MayManage(sqlite3* db, int64_t cid, int64_t owner) {
	if (owner >= kSoloOwnerBase) return true;
	int level = 0, perms = -1;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT COALESCE(r.rank_level, m.rank_id), COALESCE(r.permissions, 0) FROM ga_agency_members m "
	        "JOIN ga_characters c ON c.user_id = m.user_id LEFT JOIN ga_agency_ranks r ON r.agency_id = m.agency_id "
	        "AND r.rank_id = m.rank_id WHERE c.id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, cid);
		if (sqlite3_step(s) == SQLITE_ROW) { level = sqlite3_column_int(s, 0); perms = sqlite3_column_int(s, 1); }
		sqlite3_finalize(s);
	}
	return level == 0 || (perms & 0x00020000);
}
// A defense facility = the owner's HQ (1.4: the only one) or a Defense Facility building (35), held by owner.
bool IsDefenseFacility(sqlite3* db, uint32_t pos, int64_t owner) {
	if (owner == 0 || OwnerOf(db, pos) != owner) return false;
	if (Scalar(db, "SELECT 1 FROM ga_ava_hq WHERE zone_id = 1 AND position = ?1 AND owner_agency_id = ?2", pos, owner)) return true;
	return AvaParityBuildingAt(pos) == kDefenseBuilding;
}
int Popcount(uint32_t f) { int n = 0; for (; f; f &= f - 1) ++n; return n; }

struct Origin { uint32_t pos; int64_t owner; uint32_t flags; int progress; };
int ProgressOf(sqlite3* db, int64_t set_at) {
	const double mins = std::max(0.0, SettingNum(db, "ava_shield_build_minutes", 60.0));
	if (mins <= 0.0) return 100;
	const double done = (double)((int64_t)std::time(nullptr) - set_at) / (mins * 60.0);
	return (int)std::clamp(done * 100.0, 0.0, 100.0);
}
// Valid shield rows: origin still a defense facility of its owner; only bits whose target the owner holds.
std::vector<Origin> Origins(sqlite3* db) {
	std::vector<Origin> out;
	if (!Enabled(db)) return out;
	EnsureSchema(db);
	struct Raw { uint32_t pos; int64_t owner; uint32_t flags; int64_t set_at; };
	std::vector<Raw> raw;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT position, owner_agency_id, flags, set_at FROM ga_ava_shields WHERE zone_id = 1",
	        -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW)
			raw.push_back({(uint32_t)sqlite3_column_int(s, 0), sqlite3_column_int64(s, 1),
				(uint32_t)sqlite3_column_int(s, 2), sqlite3_column_int64(s, 3)});
		sqlite3_finalize(s);
	}
	for (const auto& r : raw) {
		if (!IsDefenseFacility(db, r.pos, r.owner)) continue;
		uint32_t f = 0;
		for (int d = 0; d < 6; ++d) {
			if (!(r.flags & (1u << d))) continue;
			const int n = Neighbour(r.pos, d);
			if (n >= 0 && OwnerOf(db, (uint32_t)n) == r.owner && !IsDefenseFacility(db, (uint32_t)n, r.owner)) f |= 1u << d;
		}
		if (f) out.push_back({r.pos, r.owner, f, ProgressOf(db, r.set_at)});
	}
	return out;
}

const std::map<uint32_t, int> kMissileRange = {{5058, 2}, {5059, 3}, {5060, 4}};

// Hex distance from the closest Missile Complex the owner holds (-1 = none).
int ComplexDistance(sqlite3* db, int64_t owner, uint32_t target) {
	int best = -1;
	sqlite3_stmt* s = nullptr;
	std::vector<uint32_t> held;
	if (sqlite3_prepare_v2(db, "SELECT position FROM ga_ava_territories WHERE zone_id = 1 AND owner_agency_id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		while (sqlite3_step(s) == SQLITE_ROW) held.push_back((uint32_t)sqlite3_column_int(s, 0));
		sqlite3_finalize(s);
	}
	for (uint32_t p : held)
		if (AvaParityBuildingAt(p) == kMissileComplexBuilding) {
			const int d = Distance(p, target);
			if (best < 0 || d < best) best = d;
		}
	return best;
}
// The strike force inventory row -> item (same order as TcpSession::send_strike_force_inventory:
// 1 = Basic Dropship, then the owner's ga_ava_items by item id).
uint32_t ItemForInventoryId(sqlite3* db, int64_t cid, uint32_t inv) {
	if (inv < 2) return 0;
	return (uint32_t)Scalar(db, "SELECT i.item_id FROM ga_characters c LEFT JOIN ga_agency_members m ON m.user_id = c.user_id "
		"JOIN ga_ava_items i ON i.owner_agency_id = 1073741824 + c.id OR i.owner_agency_id = m.agency_id "
		"WHERE c.id = ?1 ORDER BY i.item_id LIMIT 1 OFFSET ?2", cid, (int64_t)inv - 2);
}
int64_t MissilesHeld(sqlite3* db, int64_t owner, uint32_t item) {
	return Scalar(db, "SELECT count FROM ga_ava_items WHERE owner_agency_id = ?1 AND item_id = ?2", owner, item);
}
// Picks the missile: the wanted one if held and in range, else the shortest-range held one that reaches.
uint32_t PickMissile(sqlite3* db, int64_t owner, int dist, uint32_t wanted) {
	auto ok = [&](uint32_t item) {
		const auto it = kMissileRange.find(item);
		return it != kMissileRange.end() && it->second >= dist && MissilesHeld(db, owner, item) > 0;
	};
	if (wanted && ok(wanted)) return wanted;
	for (const auto& [item, range] : kMissileRange) if (ok(item)) return item;  // map is ordered by range
	return 0;
}
std::string ItemName(sqlite3* db, uint32_t item) {
	std::string n = "missile";
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT m.message FROM asm_data_set_items i JOIN asm_data_set_msg_translations m "
	        "ON m.msg_id = i.name_msg_id WHERE i.item_id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)item);
		if (sqlite3_step(s) == SQLITE_ROW) { const char* c = (const char*)sqlite3_column_text(s, 0); if (c) n = c; }
		sqlite3_finalize(s);
	}
	return n;
}
bool HasBreach(sqlite3* db, uint32_t pos, int64_t owner) {
	EnsureSchema(db);
	return Scalar(db, "SELECT 1 FROM ga_ava_shield_breaches WHERE zone_id = 1 AND position = ?1 AND owner_agency_id = ?2 "
		"AND until > strftime('%s','now')", pos, owner) != 0;
}
std::string MissileHint(int dist) {
	if (dist < 0) return "AvA: that territory is shielded - you need a Missile Complex and a Tactical Missile to strike it.";
	return "AvA: that territory is shielded - you need a Tactical Missile with range " + std::to_string(dist) +
		" or more (distance from your closest Missile Complex; Tactical Missile I/II/III = range 2/3/4, built in a Factory).";
}

}  // namespace

// Client GetNeighborIndexes (exe 0x10acbd60): rows are half rows, so a hex touches N (row - 2), NE, SE, S (row + 2), SW, NW
// and nothing on its own row; odd rows sit half a column to the right. dir = bit of the client's shield flags
// (exe 0x113f8210, measured on the shield diagram in a real client: bits 0..5 = NW, N, NE, SE, S, SW).
// A blank tile or the edge of the grid is no neighbour (-1).
int Neighbour(uint32_t pos, int dir) {
	if (!AvaZones::Valid(pos) || dir < 0 || dir > 5) return -1;
	const int base = (int)AvaZones::Base(AvaZones::ZoneOf(pos));
	const int r = (int)AvaZones::Local(pos) / kW, c = (int)AvaZones::Local(pos) % kW, odd = r & 1;
	static const int dr[6] = {-1, -2, -1, 1, 2, 1};
	const int dc[6] = {odd - 1, 0, odd, odd, 0, odd - 1};
	const int nr = r + dr[dir], nc = c + dc[dir];
	if (nr < 0 || nr >= kH || nc < 0 || nc >= kW) return -1;
	const uint32_t n = (uint32_t)(base + nr * kW + nc);
	return AvaZones::Valid(n) ? (int)n : -1;
}
// Steps between two hexes with x = 2 * col + (row odd), y = row: every step changes x by 1 and y by 1, or y by 2. Counted on
// the grid, so a shot crosses a chasm of blank tiles.
int Distance(uint32_t a, uint32_t b) {
	if (AvaZones::ZoneOf(a) != AvaZones::ZoneOf(b)) return 1000;  // no shot reaches another zone
	const auto xy = [](uint32_t g, int& x, int& y) { const int p = (int)AvaZones::Local(g); y = p / kW; x = 2 * (p % kW) + (y & 1); };
	int ax, ay, bx, by;
	xy(a, ax, ay);
	xy(b, bx, by);
	const int dx = std::abs(ax - bx), dy = std::abs(ay - by);
	return dx + std::max(0, (dy - dx) / 2);
}

Result Assign(int64_t cid, uint32_t pos, uint32_t flags) {
	Result r;
	sqlite3* db = Database::GetConnection();
	if (!db || !AvaZones::Valid(pos)) { r.refuse = true; return r; }
	EnsureSchema(db);
	if (!Enabled(db)) { r.refuse = true; r.text = "AvA: territory shields are switched off on this server."; return r; }
	const int64_t me = OwnerOfCharacter(db, cid);
	if (me == 0 || OwnerOf(db, pos) != me) { r.refuse = true; r.msg_id = 17494; return r; }  // not controlled by your agency
	if (!MayManage(db, cid, me)) { r.refuse = true; r.msg_id = 43914; return r; }           // no permission
	if (!IsDefenseFacility(db, pos, me)) {
		r.refuse = true;
		r.text = "AvA: shields are projected by a defense facility - your Headquarters is your defense facility (set it first).";
		return r;
	}
	flags &= 0x3F;
	bool bad = Popcount(flags) > 2;
	for (int d = 0; d < 6 && !bad; ++d) {
		if (!(flags & (1u << d))) continue;
		const int n = Neighbour(pos, d);
		bad = n < 0 || OwnerOf(db, (uint32_t)n) != me || IsDefenseFacility(db, (uint32_t)n, me);
	}
	if (bad) { r.refuse = true; r.msg_id = 43940; return r; }  // "You have chosen invalid shield directions."
	const int64_t old = Scalar(db, "SELECT flags FROM ga_ava_shields WHERE zone_id = 1 AND position = ?1 AND owner_agency_id = ?2",
		pos, me, -1);
	sqlite3_stmt* s = nullptr;
	if (flags == 0) {
		if (sqlite3_prepare_v2(db, "DELETE FROM ga_ava_shields WHERE zone_id = 1 AND position = ?1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)pos);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		r.text = "AvA: shields of territory " + AvaZones::Label(pos) + " removed.";
	} else if ((uint32_t)old != flags) {
		if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_shields (zone_id, position, owner_agency_id, flags, set_at) "
		        "VALUES (1, ?1, ?2, ?3, strftime('%s','now'))", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)pos);
			sqlite3_bind_int64(s, 2, me);
			sqlite3_bind_int(s, 3, (int)flags);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		std::string targets;
		for (int d = 0; d < 6; ++d)
			if (flags & (1u << d)) targets += (targets.empty() ? "" : " and ") + std::to_string(AvaZones::Local((uint32_t)Neighbour(pos, d)));
		r.text = "AvA: shields from territory " + AvaZones::Label(pos) + " are being built around territory " + targets +
			" (ready in " + std::to_string((int)SettingNum(db, "ava_shield_build_minutes", 60.0)) + " min).";
	} else {
		r.text = "AvA: those shields are already set.";
	}
	r.zone_changed = true;
	Logger::Log("tcp", "[AvaShields] assign pos %u flags 0x%x owner %lld (old 0x%llx)\n", pos, flags, (long long)me, (long long)old);
	return r;
}

Row RowFor(uint32_t pos) {
	Row row;
	sqlite3* db = Database::GetConnection();
	if (!db) return row;
	for (const auto& o : Origins(db)) {
		if (o.pos == pos) { row.position_flags = o.flags; row.progress = std::max(row.progress, o.progress); }
		for (int d = 0; d < 6; ++d)
			if ((o.flags & (1u << d)) && Neighbour(o.pos, d) == (int)pos) {
				row.origin_flags |= 1u << ((d + 3) % 6);  // direction from the shielded hex back to its origin
				row.progress = std::max(row.progress, o.progress);
			}
	}
	return row;
}

bool IsShielded(uint32_t pos) {
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	for (const auto& o : Origins(db))
		if (o.progress >= 100)
			for (int d = 0; d < 6; ++d)
				if ((o.flags & (1u << d)) && Neighbour(o.pos, d) == (int)pos) return true;
	return false;
}

Result CheckBid(int64_t cid, uint32_t pos, uint32_t inventory_id) {
	Result r;
	sqlite3* db = Database::GetConnection();
	if (!db || !AvaZones::Valid(pos) || !IsShielded(pos)) return r;
	const int64_t me = OwnerOfCharacter(db, cid);
	if (me == 0 || OwnerOf(db, pos) == me || HasBreach(db, pos, me)) return r;
	const int dist = ComplexDistance(db, me, pos);
	const uint32_t item = dist < 0 ? 0 : PickMissile(db, me, dist, ItemForInventoryId(db, cid, inventory_id));
	if (!item) { r.refuse = true; r.msg_id = 39075; r.text = MissileHint(dist); return r; }
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_missile_picks (character_id, position, item_id, set_at) "
	        "VALUES (?1, ?2, ?3, strftime('%s','now'))", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, cid);
		sqlite3_bind_int(s, 2, (int)pos);
		sqlite3_bind_int(s, 3, (int)item);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	r.text = "AvA: territory " + AvaZones::Label(pos) + " is shielded - your " + ItemName(db, item) +
		" will be launched when the attack starts.";
	return r;
}

Result CheckAttack(int64_t cid, uint32_t pos, bool consume) {
	Result r;
	sqlite3* db = Database::GetConnection();
	if (!db || !AvaZones::Valid(pos) || !IsShielded(pos)) return r;
	const int64_t me = OwnerOfCharacter(db, cid);
	if (me == 0 || OwnerOf(db, pos) == me || HasBreach(db, pos, me)) return r;
	const int dist = ComplexDistance(db, me, pos);
	const uint32_t wanted = (uint32_t)Scalar(db, "SELECT item_id FROM ga_ava_missile_picks WHERE character_id = ?1 AND position = ?2",
		cid, pos);
	const uint32_t item = dist < 0 ? 0 : PickMissile(db, me, dist, wanted);
	if (!item) { r.refuse = true; r.msg_id = 39075; r.text = MissileHint(dist); return r; }
	if (!consume) return r;
	const int minutes = (int)std::max(1.0, SettingNum(db, "ava_missile_breach_minutes", 30.0));
	sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "UPDATE ga_ava_items SET count = count - 1 WHERE owner_agency_id = ?1 AND item_id = ?2 AND count > 0",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, me);
		sqlite3_bind_int(s, 2, (int)item);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "DELETE FROM ga_ava_items WHERE count <= 0", nullptr, nullptr, nullptr);
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_shield_breaches (zone_id, position, owner_agency_id, item_id, until) "
	        "VALUES (1, ?1, ?2, ?3, strftime('%s','now') + ?4)", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)pos);
		sqlite3_bind_int64(s, 2, me);
		sqlite3_bind_int(s, 3, (int)item);
		sqlite3_bind_int(s, 4, minutes * 60);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	if (sqlite3_prepare_v2(db, "DELETE FROM ga_ava_missile_picks WHERE character_id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, cid);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	r.text = "AvA: " + ItemName(db, item) + " launched - the shield of territory " + AvaZones::Label(pos) +
		" is down for your side for " + std::to_string(minutes) + " min.";
	Logger::Log("tcp", "[AvaShields] missile %u at pos %u by owner %lld (distance %d)\n", item, pos, (long long)me, dist);
	return r;
}

// Deed item -> {facility it places, facility it must replace (0 = an empty "No Facility" hex)}.
// Tech II/III deeds (5112-5117, Factory 200/800 production + blueprint components) upgrade the
// matching facility one tier. Tech I deeds (Facility Vendor, 10000 credits in the data) place a Tech I
// on an empty hex; the Power Core route stays for the bots. 5763 Tech I Defense Facility: 1.4 made the HQ the only defense facility.
struct Deed { uint32_t deed, building, replaces; };
constexpr Deed kDeeds[] = {
	{kMissileComplexDeed, kMissileComplexBuilding, 18},
	{4807, 28, 18}, {4808, 25, 18}, {4797, 29, 18},   // Tech I Mine / Lab / Factory
	{5112, 13, 28}, {5113, 26, 13},   // Tech II / III Mine
	{5114, 17, 25}, {5115, 24, 17},   // Tech II / III Lab
	{5116, 14, 29}, {5117, 27, 14},   // Tech II / III Factory
};

// Change Facility opens the Personal Inventory filtered to Facilities (exe 0x113fa592): INVENTORY_ID is a bag row.
static uint32_t BagDeed(sqlite3* db, int64_t cid, int64_t row) {
	uint32_t item = 0;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT p.item_id FROM ga_players_inventory p JOIN ga_characters c ON c.user_id = p.user_id "
	        "JOIN asm_data_set_items i ON i.item_id = p.item_id WHERE p.id = ?1 AND c.id = ?2 "
	        "AND i.item_type_value_id = 1312 AND i.item_subtype_value_id = 1338", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, row);
		sqlite3_bind_int64(s, 2, cid);
		if (sqlite3_step(s) == SQLITE_ROW) item = (uint32_t)sqlite3_column_int(s, 0);
		sqlite3_finalize(s);
	}
	return item;
}

bool DeedsToBag(int64_t cid) {
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	const int64_t me = OwnerOfCharacter(db, cid);
	if (me == 0 || !MayManage(db, cid, me)) return false;
	std::map<int, int> items;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT a.item_id, MIN(a.count, 10) FROM ga_ava_items a JOIN asm_data_set_items i ON i.item_id = a.item_id "
	        "WHERE a.owner_agency_id = ?1 AND a.count > 0 AND ((i.item_type_value_id = 1312 AND i.item_subtype_value_id = 1338) "
	        "OR (i.item_type_value_id = 686 AND i.item_subtype_value_id = 1319)) "
	        "AND (SELECT COUNT(*) FROM ga_players_inventory p JOIN ga_characters c ON c.user_id = p.user_id WHERE c.id = ?2 AND p.item_id = a.item_id) < 10",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, me);
		sqlite3_bind_int64(s, 2, cid);
		while (sqlite3_step(s) == SQLITE_ROW) items[sqlite3_column_int(s, 0)] = sqlite3_column_int(s, 1);
		sqlite3_finalize(s);
	}
	if (items.empty()) return false;
	int64_t user = 0;
	if (sqlite3_prepare_v2(db, "SELECT user_id FROM ga_characters WHERE id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, cid);
		if (sqlite3_step(s) == SQLITE_ROW) user = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	if (Database::GrantLootItems(user, items).empty()) return false;
	for (const auto& [item, n] : items)
		if (sqlite3_prepare_v2(db, "UPDATE ga_ava_items SET count = count - ?3 WHERE owner_agency_id = ?1 AND item_id = ?2", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, me);
			sqlite3_bind_int(s, 2, item);
			sqlite3_bind_int(s, 3, n);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	sqlite3_exec(db, "DELETE FROM ga_ava_items WHERE count <= 0", nullptr, nullptr, nullptr);
	return true;
}

// Factory Add Task opens the Personal Inventory filtered to blueprints (exe 0x1093da42): INVENTORY_ID is a bag row.
uint32_t BagBlueprint(int64_t cid, int64_t row) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	return (uint32_t)Scalar(db, "SELECT p.item_id FROM ga_players_inventory p JOIN ga_characters c ON c.user_id = p.user_id "
		"JOIN asm_data_set_items i ON i.item_id = p.item_id WHERE p.id = ?1 AND c.id = ?2 "
		"AND i.item_type_value_id = 686 AND i.item_subtype_value_id = 1319", row, cid);
}

Result AddFacility(int64_t cid, uint32_t pos, uint32_t id) {
	Result r;
	int64_t bag_row = 0;
	if (sqlite3* bdb = Database::GetConnection())
		if (const uint32_t bag_item = BagDeed(bdb, cid, id)) { bag_row = id; id = bag_item; }
	if (id == kMissileComplexBuilding) id = kMissileComplexDeed;
	const Deed* deed = nullptr;
	for (const auto& d : kDeeds) if (d.deed == id) deed = &d;
	if (id == 5763) {
		r.handled = true;
		r.text = "AvA: since patch 1.4 your Headquarters is the only defense facility - set an HQ to project shields.";
		return r;
	}
	if (!deed) return r;
	r.handled = true;
	sqlite3* db = Database::GetConnection();
	if (!db || !AvaZones::Valid(pos)) return r;
	const int64_t me = OwnerOfCharacter(db, cid);
	if (me == 0 || OwnerOf(db, pos) != me) { r.msg_id = 17494; return r; }
	if (!MayManage(db, cid, me)) { r.msg_id = 43914; return r; }
	const std::string what = ItemName(db, deed->deed);
	if (AvaParityBuildingAt(pos) != deed->replaces) {
		r.text = deed->replaces == 18 ? "AvA: a " + what + " can only be placed on a territory with no facility."
			: "AvA: a " + what + " deed upgrades the facility one tech level below it on this territory.";
		return r;
	}
	if (!bag_row && MissilesHeld(db, me, deed->deed) < 1) {
		r.text = "AvA: you need a " + what + " deed - buy it from the Facility Vendor or build it in a Factory.";
		return r;
	}
	sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
	sqlite3_stmt* s = nullptr;
	if (bag_row) {
		if (sqlite3_prepare_v2(db, "DELETE FROM ga_players_inventory WHERE id = ?1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, bag_row);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	} else if (sqlite3_prepare_v2(db, "UPDATE ga_ava_items SET count = count - 1 WHERE owner_agency_id = ?1 AND item_id = ?2",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, me);
		sqlite3_bind_int(s, 2, (int)deed->deed);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "DELETE FROM ga_ava_items WHERE count <= 0", nullptr, nullptr, nullptr);
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_buildings (zone_id, position, owner_agency_id, hex_building_id, built_at) "
	        "VALUES (1, ?1, ?2, ?3, strftime('%s','now'))", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)pos);
		sqlite3_bind_int64(s, 2, me);
		sqlite3_bind_int(s, 3, (int)deed->building);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	r.text = "AvA: " + what + " placed - territory " + AvaZones::Label(pos) + " now has it.";
	Logger::Log("tcp", "[AvaShields] deed %u -> building %u at pos %u owner %lld\n", deed->deed, deed->building, pos, (long long)me);
	r.zone_changed = true;
	return r;
}

}  // namespace AvaShields
