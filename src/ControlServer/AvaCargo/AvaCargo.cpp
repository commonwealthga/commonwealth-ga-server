#include "src/ControlServer/AvaCargo/AvaCargo.hpp"
#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "lib/nlohmann/json.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <map>
#include <mutex>
#include <set>

// See AvaCargo.hpp . Subtype-1300 vehicles pack too (AVA_CARGO bot_id).
namespace AvaCargo {
namespace {

constexpr int64_t kSoloOwnerBase = 0x40000000;
constexpr uint32_t kBasicDropship = 5111;
constexpr int kPropCargoSpace = 393, kPropFootprint = 394;
constexpr int kMaxRows = 60;            // maximum cargo rows
constexpr int kMaxSpawnPerSide = 40;    // server safety cap (14 spots per side, packs share them)
constexpr int64_t kStaleEscrowSecs = 3 * 3600;
std::mutex g_mu;                        // escrow bookkeeping (IPC thread + TCP threads)

void EnsureSchema(sqlite3* db) {
	static bool done = false;
	if (done) return;
	done = true;
	sqlite3_exec(db,
		"CREATE TABLE IF NOT EXISTS ga_ava_cargo_held (owner_agency_id INTEGER NOT NULL, item_id INTEGER NOT NULL,"
		" count INTEGER NOT NULL, PRIMARY KEY (owner_agency_id, item_id));"
		"CREATE TABLE IF NOT EXISTS ga_ava_cargo_out (instance_id INTEGER NOT NULL, task_force INTEGER NOT NULL,"
		" owner_agency_id INTEGER NOT NULL, item_id INTEGER NOT NULL, count INTEGER NOT NULL, sent_at INTEGER NOT NULL,"
		" reported INTEGER NOT NULL DEFAULT 0, PRIMARY KEY (instance_id, task_force, item_id));",
		nullptr, nullptr, nullptr);
}

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
void Exec(sqlite3* db, const char* sql, int64_t a, int64_t b, int64_t c, int64_t d = 0, int64_t e = 0) {
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
		Logger::Log("ava", "[AvaCargo] SQL error %s: %s\n", sqlite3_errmsg(db), sql);
		return;
	}
	sqlite3_bind_int64(s, 1, a); sqlite3_bind_int64(s, 2, b); sqlite3_bind_int64(s, 3, c);
	sqlite3_bind_int64(s, 4, d); sqlite3_bind_int64(s, 5, e);
	sqlite3_step(s);
	sqlite3_finalize(s);
}
bool Enabled(sqlite3* db) {
	bool on = true;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'ava_cargo'", -1, &s, nullptr) == SQLITE_OK) {
		if (sqlite3_step(s) == SQLITE_ROW) { const char* c = (const char*)sqlite3_column_text(s, 0); on = !(c && std::string(c) == "off"); }
		sqlite3_finalize(s);
	}
	return on;
}
int64_t OwnerOfCharacter(sqlite3* db, int64_t cid) {
	return Scalar(db, "SELECT COALESCE(m.agency_id, ?2 + c.id) FROM ga_characters c "
		"LEFT JOIN ga_agency_members m ON m.user_id = c.user_id WHERE c.id = ?1", cid, kSoloOwnerBase);
}

struct Item {
	uint32_t item_id = 0, subtype = 0, deployable = 0, bot = 0;   // bot: vehicle (subtype 1300 ref_bot_id)
	int count = 0, held = 0;
	double footprint = 0, space = 0;
	std::string name;
};
// The owner's ga_ava_items in row order (item id), with game data. Row k (0-based) = INVENTORY_ID k + 2.
std::vector<Item> Stock(sqlite3* db, int64_t owner) {
	EnsureSchema(db);
	std::vector<Item> v;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT i.item_id, i.count, COALESCE(d.item_subtype_value_id, 0), COALESCE(d.ref_deployable_id, 0),"
	        " COALESCE(h.count, 0),"
	        " COALESCE((SELECT value FROM asm_data_set_item_props p WHERE p.item_id = i.item_id AND p.prop_id = 394), 0),"
	        " COALESCE((SELECT value FROM asm_data_set_item_props p WHERE p.item_id = i.item_id AND p.prop_id = 393), 0),"
	        " COALESCE((SELECT message FROM asm_data_set_msg_translations t WHERE t.msg_id = d.name_msg_id), 'item ' || i.item_id),"
	        " COALESCE(d.ref_bot_id, 0)"
	        " FROM ga_ava_items i LEFT JOIN asm_data_set_items d ON d.item_id = i.item_id"
	        " LEFT JOIN ga_ava_cargo_held h ON h.owner_agency_id = i.owner_agency_id AND h.item_id = i.item_id"
	        " WHERE i.owner_agency_id = ?1 ORDER BY i.item_id LIMIT ?2", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		sqlite3_bind_int(s, 2, kMaxRows - 1);
		while (sqlite3_step(s) == SQLITE_ROW) {
			Item it;
			it.item_id = (uint32_t)sqlite3_column_int(s, 0);
			it.count = sqlite3_column_int(s, 1);
			it.subtype = (uint32_t)sqlite3_column_int(s, 2);
			it.deployable = (uint32_t)sqlite3_column_int(s, 3);
			it.held = std::clamp(sqlite3_column_int(s, 4), 0, std::max(it.count, 0));
			it.footprint = sqlite3_column_double(s, 5);
			it.space = sqlite3_column_double(s, 6);
			const char* n = (const char*)sqlite3_column_text(s, 7);
			it.name = n ? n : "item";
			it.bot = (uint32_t)sqlite3_column_int(s, 8);
			v.push_back(it);
		}
		sqlite3_finalize(s);
	}
	return v;
}
int Loaded(const Item& it) { return std::max(0, it.count - it.held); }
// Equipped dropship: the loaded dropship with the most cargo space (index into stock), -1 = Basic.
int EquippedDropship(const std::vector<Item>& st) {
	int best = -1;
	for (size_t k = 0; k < st.size(); ++k)
		if (st[k].subtype == 1302 && Loaded(st[k]) > 0 && (best < 0 || st[k].space > st[(size_t)best].space)) best = (int)k;
	return best;
}
double BasicSpace(sqlite3* db) {
	sqlite3_stmt* s = nullptr;
	double v = 25;
	if (sqlite3_prepare_v2(db, "SELECT value FROM asm_data_set_item_props WHERE item_id = 5111 AND prop_id = 393", -1, &s, nullptr) == SQLITE_OK) {
		if (sqlite3_step(s) == SQLITE_ROW) v = sqlite3_column_double(s, 0);
		sqlite3_finalize(s);
	}
	return v;
}
void SetHeld(sqlite3* db, int64_t owner, uint32_t item, int held) {
	if (held <= 0) Exec(db, "DELETE FROM ga_ava_cargo_held WHERE owner_agency_id = ?1 AND item_id = ?2", owner, item, 0);
	else Exec(db, "INSERT OR REPLACE INTO ga_ava_cargo_held (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3)", owner, item, held);
}
// Cargo used by the loaded devices (what would fly), for the chat summary.
double LoadedFootprint(const std::vector<Item>& st) {
	double f = 0;
	for (const auto& it : st) if (it.subtype == 1301 || it.subtype == 1300) f += it.footprint * Loaded(it);
	return f;
}
std::string Summary(sqlite3* db, const std::vector<Item>& st) {
	const int eq = EquippedDropship(st);
	const double cap = eq < 0 ? BasicSpace(db) : st[(size_t)eq].space;
	char buf[160];
	std::snprintf(buf, sizeof(buf), "Dropship: %s, cargo %.0f / %.0f", eq < 0 ? "Basic Dropship" : st[(size_t)eq].name.c_str(),
	              LoadedFootprint(st), cap);
	std::string s = buf;
	if (LoadedFootprint(st) > cap) s += " (over capacity: the rest stays home)";
	return s;
}

struct Side { int tf = 0; int64_t owner = 0; double cap = 0; std::string guid; bool bots = false; };  // bots: service agency, bots carry

}  // namespace

std::vector<Row> InventoryRows(int64_t cid) {
	std::vector<Row> rows;
	sqlite3* db = Database::GetConnection();
	if (!db) return {{1, kBasicDropship, 1, 1}};
	const auto st = Stock(db, OwnerOfCharacter(db, cid));
	const int eq = EquippedDropship(st);
	rows.push_back({1, kBasicDropship, 1, eq < 0 ? 1u : 0u});
	for (size_t k = 0; k < st.size() && rows.size() < (size_t)kMaxRows; ++k) {
		const auto& it = st[k];
		const uint32_t inv = (uint32_t)k + 2;
		if (it.subtype == 1302) { rows.push_back({inv, it.item_id, (uint32_t)std::max(it.count, 0), (int)k == eq ? 1u : 0u}); continue; }
		const int l = Loaded(it);
		if (l > 0) rows.push_back({inv, it.item_id, (uint32_t)l, 1});
		if (it.held > 0 && rows.size() < (size_t)kMaxRows) rows.push_back({inv, it.item_id, (uint32_t)it.held, 0});
	}
	return rows;
}

std::string Load(int64_t cid, uint32_t inv, uint32_t item, uint32_t count) {
	sqlite3* db = Database::GetConnection();
	if (!db) return "";
	const int64_t owner = OwnerOfCharacter(db, cid);
	auto st = Stock(db, owner);
	if (inv == 1 || item == kBasicDropship) {   // equip the Basic Dropship = unload every other dropship
		for (const auto& it : st) if (it.subtype == 1302) SetHeld(db, owner, it.item_id, it.count);
		Logger::Log("ava", "[AvaCargo] char %lld owner %lld: equip Basic Dropship\n", (long long)cid, (long long)owner);
		return Summary(db, Stock(db, owner));
	}
	if (inv < 2 || inv - 2 >= st.size() || (item && st[inv - 2].item_id != item)) return "That item is not in your agency's AvA stock.";
	const auto& it = st[inv - 2];
	if (it.subtype == 1302) {                   // a dropship: equip this one only
		for (const auto& o : st) if (o.subtype == 1302) SetHeld(db, owner, o.item_id, o.item_id == it.item_id ? 0 : o.count);
	} else {
		SetHeld(db, owner, it.item_id, it.held - (int)std::max<uint32_t>(count, 1));
	}
	Logger::Log("ava", "[AvaCargo] char %lld owner %lld: load item %u x%u\n", (long long)cid, (long long)owner, it.item_id, count);
	return Summary(db, Stock(db, owner));
}

std::string Unload(int64_t cid, uint32_t inv, uint32_t count) {
	sqlite3* db = Database::GetConnection();
	if (!db) return "";
	const int64_t owner = OwnerOfCharacter(db, cid);
	auto st = Stock(db, owner);
	if (inv == 1) return "The Basic Dropship is always available.";
	if (inv < 2 || inv - 2 >= st.size()) return "";
	const auto& it = st[inv - 2];
	SetHeld(db, owner, it.item_id, it.subtype == 1302 ? it.count : std::min(it.count, it.held + (int)std::max<uint32_t>(count, 1)));
	Logger::Log("ava", "[AvaCargo] char %lld owner %lld: unload item %u x%u\n", (long long)cid, (long long)owner, it.item_id, count);
	return Summary(db, Stock(db, owner));
}

std::string Command(int64_t cid, const std::string& raw) {
	std::string args = raw;
	while (!args.empty() && (args.front() == ' ' || args.front() == '-')) args.erase(args.begin());
	while (!args.empty() && (args.back() == ' ' || args.back() == '\r' || args.back() == '\n')) args.pop_back();
	for (auto& ch : args) ch = (char)std::tolower((unsigned char)ch);
	sqlite3* db = Database::GetConnection();
	if (!db) return "";
	const int64_t owner = OwnerOfCharacter(db, cid);
	if (args == "load" || args == "unload") {
		for (const auto& it : Stock(db, owner))
			if (it.subtype != 1302) SetHeld(db, owner, it.item_id, args == "load" ? 0 : it.count);
	}
	const auto st = Stock(db, owner);
	std::string s = Summary(db, st) + ".";
	int shown = 0;
	for (const auto& it : st) {
		if (it.subtype != 1300 && it.subtype != 1301) continue;
		if (shown++ >= 12) { s += " ..."; break; }
		s += " " + it.name + " " + std::to_string(Loaded(it)) + "/" + std::to_string(it.count) + (it.subtype == 1300 ? " (vehicle)" : "") + ";";
	}
	if (!shown) s += " No AvA tech in stock (build it in a Factory).";
	s += " [-cargo load | -cargo unload | T > Cargo]";
	return s;
}

void OnMatchReady(int64_t inst, uint32_t queue_id, const std::unordered_map<std::string, int>& tfs) {
	sqlite3* db = Database::GetConnection();
	if (!db || queue_id < 40 || queue_id > 45) return;
	std::lock_guard<std::mutex> l(g_mu);
	EnsureSchema(db);
	// Stale escrow (instance died without MISSION_ENDED): give it back.
	{
		sqlite3_stmt* s = nullptr;
		std::vector<int64_t> stale;
		if (sqlite3_prepare_v2(db, "SELECT DISTINCT instance_id FROM ga_ava_cargo_out WHERE sent_at < ?1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, (int64_t)std::time(nullptr) - kStaleEscrowSecs);
			while (sqlite3_step(s) == SQLITE_ROW) stale.push_back(sqlite3_column_int64(s, 0));
			sqlite3_finalize(s);
		}
		for (int64_t i : stale) {
			Exec(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) SELECT owner_agency_id, item_id, count FROM ga_ava_cargo_out o "
			         "WHERE o.instance_id = ?1 AND o.count > 0 ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + excluded.count", i, 0, 0);
			Exec(db, "DELETE FROM ga_ava_cargo_out WHERE instance_id = ?1", i, 0, 0);
			Logger::Log("ava", "[AvaCargo] stale escrow of instance %lld returned\n", (long long)i);
		}
	}
	if (!Enabled(db)) return;
	// Attacker: a tf-1 player with a pending ga_ava_attacks row -> target position + owner.
	Side att{1}, def{2};
	int64_t pos = -1;
	for (const auto& [guid, tf] : tfs) {
		const auto info = PlayerSessionStore::GetByGuid(guid);
		if (!info || !info->selected_character_id) continue;
		const int64_t cid = info->selected_character_id;
		if (tf == 1 && pos < 0) {
			const int64_t p = Scalar(db, "SELECT position FROM ga_ava_attacks WHERE character_id = ?1", cid, 0, -1);
			if (p >= 0) {
				pos = p; att.owner = OwnerOfCharacter(db, cid); att.guid = guid;
				// An attack launched on behalf of the bid owner (an opened won bid) is that agency's
				// attack - its dropship and stock go in, not the launching player's own.
				const int64_t behalf = Scalar(db, "SELECT COALESCE(on_behalf_of, 0) FROM ga_ava_attacks WHERE character_id = ?1", cid, 0, 0);
				if (behalf > 0) {
					att.owner = behalf;
					Logger::Log("ava", "[AvaCargo] instance %lld: char %lld attacks on behalf of agency %lld - its cargo\n",
					            (long long)inst, (long long)cid, (long long)behalf);
				}
			}
		}
	}
	if (pos < 0 || !att.owner) { Logger::Log("ava", "[AvaCargo] instance %lld queue %u: no attacker with a target - no cargo\n", (long long)inst, queue_id); return; }
	const int64_t terr_owner = Scalar(db, "SELECT owner_agency_id FROM ga_ava_territories WHERE zone_id = 1 AND position = ?1", pos);
	for (const auto& [guid, tf] : tfs) {
		if (tf != 2 || !terr_owner) continue;
		const auto info = PlayerSessionStore::GetByGuid(guid);
		if (info && info->selected_character_id && OwnerOfCharacter(db, info->selected_character_id) == terr_owner) { def.owner = terr_owner; def.guid = guid; break; }
	}
	// No defending member online and the hex belongs to a service agency (every member service_account = 1):
	// its defender cargo still flies in - the server-run defenders carry the packs (no vehicles: bots do not drive).
	if (!def.owner && terr_owner && Scalar(db, "SELECT COUNT(*) FROM ga_agency_members m JOIN ga_users u ON u.id = m.user_id "
	        "WHERE m.agency_id = ?1 AND COALESCE(u.service_account, 0) = 1", terr_owner) > 0 &&
	    Scalar(db, "SELECT COUNT(*) FROM ga_agency_members m JOIN ga_users u ON u.id = m.user_id "
	        "WHERE m.agency_id = ?1 AND COALESCE(u.service_account, 0) = 0", terr_owner) == 0) { def.owner = terr_owner; def.bots = true; }
	def.cap = (double)Scalar(db, "SELECT defensive_cargo_space FROM asm_data_set_hex_buildings WHERE hex_building_id = ?1",
	                         (int64_t)AvaParityBuildingAt((uint32_t)pos));

	nlohmann::json msg;
	msg["type"] = "AVA_CARGO";
	msg["instance_id"] = inst;
	msg["position"] = pos;
	msg["sides"] = nlohmann::json::array();
	int total = 0;
	for (Side* sd : {&att, &def}) {
		if (!sd->owner || (sd->owner == att.owner && sd->tf == 2)) continue;
		auto st = Stock(db, sd->owner);
		if (sd->tf == 1) { const int eq = EquippedDropship(st); sd->cap = eq < 0 ? BasicSpace(db) : st[(size_t)eq].space; }
		double used = 0;
		int spawned = 0, home = 0, vehicles = 0;   // vehicles: siege vehicles packed
		nlohmann::json items = nlohmann::json::array();
		for (const auto& it : st) {
			const int l = Loaded(it);
			if (l <= 0) continue;
			const bool vehicle = it.subtype == 1300 && it.bot && it.footprint > 0;   // drivable siege vehicle (bot pawn)
			if (!vehicle && (it.subtype != 1301 || !it.deployable || it.footprint <= 0)) continue;
			if (vehicle && sd->bots) continue;   // bot carriers leave vehicles at home
			int n = 0;
			for (int k = 0; k < l; ++k) {
				if (used + it.footprint > sd->cap + 1e-6 || spawned >= kMaxSpawnPerSide) { ++home; continue; }  // HIS_DoNotSpawn
				used += it.footprint; ++n; ++spawned;
			}
			if (!n) continue;
			if (vehicle) vehicles += n;
			Exec(db, "UPDATE ga_ava_items SET count = count - ?3 WHERE owner_agency_id = ?1 AND item_id = ?2", sd->owner, it.item_id, n);
			Exec(db, "INSERT INTO ga_ava_cargo_out (instance_id, task_force, owner_agency_id, item_id, count, sent_at) VALUES (?1, ?2, ?3, ?4, ?5, strftime('%s','now')) "
			         "ON CONFLICT(instance_id, task_force, item_id) DO UPDATE SET count = count + excluded.count", inst, sd->tf, sd->owner, it.item_id, n);
			if (vehicle) items.push_back({{"item_id", it.item_id}, {"bot_id", it.bot}, {"count", n}});
			else items.push_back({{"item_id", it.item_id}, {"deployable_id", it.deployable}, {"count", n}});
		}
		Exec(db, "DELETE FROM ga_ava_items WHERE owner_agency_id = ?1 AND count <= 0", sd->owner, 0, 0);
		// held counts keep their meaning against the smaller stock
		Exec(db, "UPDATE ga_ava_cargo_held SET count = MIN(count, COALESCE((SELECT i.count FROM ga_ava_items i WHERE i.owner_agency_id = ga_ava_cargo_held.owner_agency_id "
		         "AND i.item_id = ga_ava_cargo_held.item_id), 0)) WHERE owner_agency_id = ?1", sd->owner, 0, 0);
		Logger::Log("ava", "[AvaCargo] instance %lld tf %d owner %lld: %d item(s) spawn, cargo %.0f/%.0f, %d over capacity stay home, %d vehicle(s) packed\n",
		            (long long)inst, sd->tf, (long long)sd->owner, spawned, used, sd->cap, home, vehicles);
		if (!sd->guid.empty()) {
			char buf[200];
			std::snprintf(buf, sizeof(buf), "AvA cargo: %d tech item(s) wait at your %s (cargo %.0f/%.0f). Pick up / deploy / enter with the beacon key (B).%s%s",
			              spawned, sd->tf == 1 ? "dropship" : "base", used, sd->cap,
			              home ? " Over capacity, stays home: " : "", home ? std::to_string(home).c_str() : "");
			std::string line = buf;
			if (vehicles) line += " " + std::to_string(vehicles) + " vehicle(s): stand beside one and press B to drive it, B again to get out.";
			if (spawned || home || vehicles) ChatSession::SystemMessageToGuid(sd->guid, line);
		}
		if (spawned) { msg["sides"].push_back({{"task_force", sd->tf}, {"items", items}, {"bot_carriers", sd->bots ? 1 : 0}}); total += spawned; }
	}
	if (total) {
		const bool ok = IpcServer::SendToInstance(inst, msg.dump());
		Logger::Log("ava", "[AvaCargo] instance %lld: AVA_CARGO sent (%d items) ok=%d\n", (long long)inst, total, ok ? 1 : 0);
	}
}

void OnResult(int64_t inst, const std::string& json) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	std::lock_guard<std::mutex> l(g_mu);
	EnsureSchema(db);
	auto j = nlohmann::json::parse(json, nullptr, false);
	if (j.is_discarded()) return;
	int returned = 0;
	if (j.contains("returns") && j["returns"].is_array())
		for (const auto& r : j["returns"]) {
			const int tf = r.value("task_force", 0), n = r.value("count", 0);
			const int64_t item = r.value("item_id", (int64_t)0);
			if (n <= 0 || !item) continue;
			sqlite3_stmt* s = nullptr;
			int64_t row_owner = 0, row_count = 0;
			if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, count FROM ga_ava_cargo_out WHERE instance_id = ?1 AND task_force = ?2 AND item_id = ?3",
			                       -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, inst); sqlite3_bind_int(s, 2, tf); sqlite3_bind_int64(s, 3, item);
				if (sqlite3_step(s) == SQLITE_ROW) { row_owner = sqlite3_column_int64(s, 0); row_count = sqlite3_column_int64(s, 1); }
				sqlite3_finalize(s);
			}
			const int64_t back = std::min<int64_t>(n, row_count);
			if (back <= 0) continue;
			Exec(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3) "
			         "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + excluded.count", row_owner, item, back);
			Exec(db, "UPDATE ga_ava_cargo_out SET count = count - ?4 WHERE instance_id = ?1 AND task_force = ?2 AND item_id = ?3", inst, tf, item, back);
			returned += (int)back;
		}
	Exec(db, "UPDATE ga_ava_cargo_out SET reported = 1 WHERE instance_id = ?1", inst, 0, 0);
	Logger::Log("ava", "[AvaCargo] instance %lld: result - %d item(s) returned, the rest was used\n", (long long)inst, returned);
}

void OnMatchEnded(int64_t inst) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	std::lock_guard<std::mutex> l(g_mu);
	EnsureSchema(db);
	if (!Scalar(db, "SELECT COUNT(*) FROM ga_ava_cargo_out WHERE instance_id = ?1", inst)) return;
	const bool reported = Scalar(db, "SELECT MAX(reported) FROM ga_ava_cargo_out WHERE instance_id = ?1", inst) > 0;
	if (!reported) {  // no word from the instance: nothing is known to be used -> return everything
		Exec(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) SELECT owner_agency_id, item_id, count FROM ga_ava_cargo_out o "
		         "WHERE o.instance_id = ?1 AND o.count > 0 ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + excluded.count", inst, 0, 0);
	}
	Exec(db, "DELETE FROM ga_ava_cargo_out WHERE instance_id = ?1", inst, 0, 0);
	Logger::Log("ava", "[AvaCargo] instance %lld: escrow closed (%s)\n", (long long)inst, reported ? "result applied" : "no result - all returned");
}

}  // namespace AvaCargo
