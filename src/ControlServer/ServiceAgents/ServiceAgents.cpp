// Service agents (see ServiceAgents.hpp).
#include "src/ControlServer/ServiceAgents/ServiceAgents.hpp"
#include "src/ControlServer/AvaZones/AvaZones.hpp"
#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <set>
#include <vector>

namespace ServiceAgents {
namespace {

std::mutex g_mu;  // one admin op at a time

int64_t Scalar(sqlite3* db, const char* sql, int64_t a, const std::string* text = nullptr, int64_t def = 0) {
	int64_t v = def;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return def;
	if (text) sqlite3_bind_text(s, 1, text->c_str(), -1, SQLITE_TRANSIENT);
	else sqlite3_bind_int64(s, 1, a);
	if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL) v = sqlite3_column_int64(s, 0);
	sqlite3_finalize(s);
	return v;
}

bool ValidName(const std::string& n) {
	if (n.size() < 3 || n.size() > 24) return false;
	for (unsigned char c : n)
		if (!std::isalnum(c) && c != '-' && c != '_' && c != '.' && c != ' ') return false;
	return true;
}

int64_t UserByName(sqlite3* db, const std::string& name) {
	return Scalar(db, "SELECT id FROM ga_users WHERE username = ?1 COLLATE NOCASE", 0, &name);
}

int64_t CharacterOfUser(sqlite3* db, int64_t uid) {
	return Scalar(db, "SELECT id FROM ga_characters WHERE user_id = ?1 AND deleted_at = 0 ORDER BY id LIMIT 1", uid);
}

bool OpAgent(sqlite3* db, const nlohmann::json& p, nlohmann::json& out, std::string& msg) {
	const std::string name = p.value("name", std::string());
	const int profile = p.value("profile_id", 681);
	if (!ValidName(name)) { msg = "bad name"; return false; }
	int64_t uid = UserByName(db, name);
	bool created = false;
	if (uid > 0 && !IsServiceUser(uid)) { msg = "name belongs to a player account"; return false; }
	if (uid == 0) {
		sqlite3_stmt* s = nullptr;
		// randomblob verifier: no password can match it, and a NULL verifier would let
		// the first login set one.
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_users (username, password_verifier, registered_at, service_account) "
		        "VALUES (?1, randomblob(32), strftime('%s','now'), 1)", -1, &s, nullptr) != SQLITE_OK) {
			msg = sqlite3_errmsg(db); return false;
		}
		sqlite3_bind_text(s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
		const int rc = sqlite3_step(s);
		sqlite3_finalize(s);
		if (rc != SQLITE_DONE) { msg = "user insert failed"; return false; }
		uid = sqlite3_last_insert_rowid(db);
		created = true;
	}
	int64_t cid = CharacterOfUser(db, uid);
	if (cid == 0) {
		cid = PlayerSessionStore::InsertCharacter(uid, (uint32_t)profile, (uint32_t)p.value("head_asm_id", 1605),
			(uint32_t)p.value("gender_type_value_id", 853), {}, (uint32_t)p.value("hair_asm_id", 1974), 0, 0);
		if (cid <= 0) { msg = "character insert failed"; return false; }
	}
	if (p.contains("xp")) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "UPDATE ga_characters SET xp = ?1 WHERE id = ?2", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, std::max<int64_t>(0, p.value("xp", (int64_t)0)));
			sqlite3_bind_int64(s, 2, cid);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	}
	out = {{"user_id", uid}, {"character_id", cid}, {"created", created}};
	Logger::Log("admin", "[ServiceAgents] agent %s user=%lld char=%lld created=%d\n", name.c_str(),
		(long long)uid, (long long)cid, created ? 1 : 0);
	return true;
}

bool OpAgency(sqlite3* db, const nlohmann::json& p, nlohmann::json& out, std::string& msg) {
	const std::string name = p.value("name", std::string());
	const std::string leader = p.value("leader", std::string());
	if (!ValidName(name)) { msg = "bad agency name"; return false; }
	const int64_t luid = UserByName(db, leader);
	if (luid <= 0 || !IsServiceUser(luid)) { msg = "leader must be a service agent"; return false; }
	int64_t aid = Scalar(db, "SELECT id FROM ga_agencies WHERE name = ?1", 0, &name);
	bool created = false;
	if (aid > 0 && Scalar(db, "SELECT leader_user_id FROM ga_agencies WHERE id = ?1", aid) != luid) {
		msg = "agency name taken"; return false;
	}
	if (aid == 0) {
		float c[3] = {0.5f, 0.5f, 0.5f};
		if (p.contains("color") && p["color"].is_array() && p["color"].size() == 3)
			for (int i = 0; i < 3; ++i) c[i] = p["color"][i].get<float>();
		aid = Database::CreateAgency(name, c[0], c[1], c[2], luid, CharacterOfUser(db, luid), leader);
		if (aid <= 0) { msg = "create agency failed"; return false; }
		created = true;
	}
	int added = 0;
	if (p.contains("members") && p["members"].is_array())
		for (const auto& m : p["members"]) {
			if (!m.is_string()) continue;
			const std::string mn = m.get<std::string>();
			const int64_t uid = UserByName(db, mn);
			if (uid <= 0 || !IsServiceUser(uid) || Database::GetAgencyIdForUser(uid) != 0) continue;
			if (Database::AddAgencyMember(aid, uid, CharacterOfUser(db, uid), mn, 2)) ++added;
		}
	out = {{"agency_id", aid}, {"created", created}, {"added", added}};
	Logger::Log("admin", "[ServiceAgents] agency %s id=%lld created=%d added=%d\n", name.c_str(),
		(long long)aid, created ? 1 : 0, added);
	return true;
}

bool OpBid(const nlohmann::json& p, nlohmann::json& out, std::string& msg) {
	const int64_t cid = p.value("character_id", (int64_t)0);
	const int64_t pos = p.value("position", (int64_t)-1);
	if (!IsServiceCharacter(cid) || pos < 0 || !AvaZones::Valid((uint32_t)pos)) { msg = "not a service agent / bad position"; return false; }
	const auto g = AvaParity::Check(cid, (uint32_t)pos, AvaParity::kBid);
	if (g.refuse) { out = {{"ok", false}, {"msg_id", g.msg_id}, {"text", g.text}}; return true; }
	const auto r = AvaParity::SealedBid(cid, (uint32_t)pos, (uint32_t)p.value("amount", 0),
		(uint32_t)p.value("attack_type", 0), p.value("cancel", false));
	out = {{"ok", r.handled && r.msg_id == 0 && r.text.empty()}, {"handled", r.handled},
	       {"msg_id", r.msg_id}, {"text", r.text}};
	return true;
}

bool OpMatch(sqlite3* db, const nlohmann::json& p, nlohmann::json& out, std::string& msg) {
	const int64_t pos = p.value("position", (int64_t)-1);
	const int winner = p.value("winning_task_force", 0);
	if (pos < 0 || winner < 0 || winner > 2) { msg = "bad position / winning_task_force"; return false; }
	std::vector<std::pair<int64_t, int>> seats;
	std::set<int64_t> seen;
	for (int tf = 1; tf <= 2; ++tf) {
		const char* key = tf == 1 ? "attackers" : "defenders";
		if (!p.contains(key) || !p[key].is_array()) continue;
		for (const auto& v : p[key]) {
			const int64_t cid = v.is_number_integer() ? v.get<int64_t>() : 0;
			if (!IsServiceCharacter(cid)) { msg = "only service agents can be seated"; return false; }
			if (seen.insert(cid).second) seats.push_back({cid, tf});
		}
	}
	bool any_attacker = false;
	for (auto& s : seats) any_attacker |= s.second == 1;
	if (!any_attacker) { msg = "no attackers"; return false; }
	// Same gate as a client's attack (bid window, lockdown, shields, region).
	for (auto& s : seats)
		if (s.second == 1) {
			const auto g = AvaParity::Check(s.first, (uint32_t)pos, AvaParity::kAttack);
			if (g.refuse) { out = {{"ok", false}, {"msg_id", g.msg_id}, {"text", g.text}}; return true; }
			break;
		}
	const int64_t now = (int64_t)std::time(nullptr);
	const std::string map = p.value("map_name", std::string("resolved"));
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "INSERT INTO ga_instances (map_name, game_mode, state, pid, udp_port, player_count, "
	        "started_at, queue_id, access_mode) VALUES (?1, 'resolved', 'STOPPED', 0, 0, 0, ?2, 40, 'CLOSED')",
	        -1, &s, nullptr) != SQLITE_OK) { msg = sqlite3_errmsg(db); return false; }
	sqlite3_bind_text(s, 1, map.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(s, 2, now);
	sqlite3_step(s);
	sqlite3_finalize(s);
	const int64_t inst = sqlite3_last_insert_rowid(db);
	char sql[160];
	std::snprintf(sql, sizeof sql, "UPDATE ga_instances SET instance_id = id WHERE id = %lld", (long long)inst);
	sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
	for (auto& st : seats) {
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_instance_players (instance_id, session_guid, character_id, "
		        "task_force_number, joined_at) VALUES (?1, ?2, ?3, ?4, ?5)", -1, &s, nullptr) == SQLITE_OK) {
			const std::string guid = "svc-" + std::to_string(inst) + "-" + std::to_string(st.first);
			sqlite3_bind_int64(s, 1, inst);
			sqlite3_bind_text(s, 2, guid.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int64(s, 3, st.first);
			sqlite3_bind_int(s, 4, st.second);
			sqlite3_bind_int64(s, 5, now);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		if (st.second == 1 && sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_attacks (character_id, zone_id, "
		        "position, queued_at) VALUES (?1, 1, ?2, ?3)", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, st.first);
			sqlite3_bind_int64(s, 2, pos);
			sqlite3_bind_int64(s, 3, now);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	}
	const auto paid = PlayerSessionStore::ResolveAvaMatch(inst, winner);
	if (sqlite3_prepare_v2(db, "UPDATE ga_instances SET outcome = ?2, winning_task_force = ?3, end_mission_at = ?4 "
	        "WHERE id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, inst);
		sqlite3_bind_text(s, 2, winner == 0 ? "draw" : "resolved", -1, SQLITE_STATIC);
		sqlite3_bind_int(s, 3, winner);
		sqlite3_bind_int64(s, 4, now);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	std::snprintf(sql, sizeof sql, "UPDATE ga_instance_players SET left_at = %lld WHERE instance_id = %lld",
		(long long)now, (long long)inst);
	sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
	int64_t owner = 0;
	std::string owner_name;
	if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, owner_name FROM ga_ava_territories WHERE zone_id = 1 "
	        "AND position = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, pos);
		if (sqlite3_step(s) == SQLITE_ROW) {
			owner = sqlite3_column_int64(s, 0);
			owner_name = reinterpret_cast<const char*>(sqlite3_column_text(s, 1));
		}
		sqlite3_finalize(s);
	}
	out = {{"ok", true}, {"instance_id", inst}, {"owner_agency_id", owner}, {"owner_name", owner_name},
	       {"paid", (int64_t)paid.size()}};
	Logger::Log("admin", "[ServiceAgents] match pos=%lld seats=%zu winner=%d inst=%lld owner=%lld\n",
		(long long)pos, seats.size(), winner, (long long)inst, (long long)owner);
	return true;
}

// "assign" {character_id, position, item} / "facility" {character_id, position, id} - the same
// rules and costs as a client's HEX_ASSIGN_RESOURCE_ITEM / HEX_ADD_FACILITY, for the agent's agency.
bool OpTerritory(sqlite3* db, const nlohmann::json& p, nlohmann::json& out, std::string& msg, bool assign) {
	const int64_t cid = p.value("character_id", (int64_t)0);
	const int64_t pos = p.value("position", (int64_t)-1);
	if (!IsServiceCharacter(cid) || pos < 0 || !AvaZones::Valid((uint32_t)pos)) { msg = "not a service agent / bad position"; return false; }
	const int64_t agency = Scalar(db, "SELECT m.agency_id FROM ga_agency_members m JOIN ga_characters c "
		"ON c.user_id = m.user_id WHERE c.id = ?1", cid);
	if (agency == 0) { msg = "service agent has no agency"; return false; }
	const uint32_t id = (uint32_t)p.value(assign ? "item" : "id", (int64_t)0);
	// Optional deed_price: the agent pays this from its wallet for the item a new facility costs (5106), like a vendor deed.
	const int64_t deed_price = assign ? 0 : p.value("deed_price", (int64_t)0);
	if (deed_price > 0) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "UPDATE ga_characters SET credits = credits - ?2 WHERE id = ?1 AND credits >= ?2", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, cid);
			sqlite3_bind_int64(s, 2, deed_price);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		if (sqlite3_changes(db) != 1) { msg = "not enough credits for the deed"; return false; }
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_resources (owner_agency_id, item_id, amount) VALUES (?1, 5106, 1) "
		        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET amount = amount + 1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, agency);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	}
	out = {{"agency_id", agency}, {"text", assign ? AvaServiceAssignResource(agency, (uint32_t)pos, id)
	                                              : AvaServiceAddFacility(agency, (uint32_t)pos, id)}};
	return true;
}

// "produce" {character_id, position, item, count 1-100, remove false} - the same rules and costs as a
// client's HEX_ADD_FACTORY_ITEM / HEX_REMOVE_FACTORY_ITEM, for the agent's agency. Reply {agency_id, built, text}.
bool OpProduce(sqlite3* db, const nlohmann::json& p, nlohmann::json& out, std::string& msg) {
	const int64_t cid = p.value("character_id", (int64_t)0);
	const int64_t pos = p.value("position", (int64_t)-1);
	if (!IsServiceCharacter(cid) || pos < 0 || !AvaZones::Valid((uint32_t)pos)) { msg = "not a service agent / bad position"; return false; }
	const int64_t agency = Scalar(db, "SELECT m.agency_id FROM ga_agency_members m JOIN ga_characters c "
		"ON c.user_id = m.user_id WHERE c.id = ?1", cid);
	if (agency == 0) { msg = "service agent has no agency"; return false; }
	bool changed = false;
	const std::string text = AvaServiceFactoryItem(agency, (uint32_t)pos, (uint32_t)p.value("item", (int64_t)0),
		(uint32_t)p.value("count", (int64_t)1), p.value("remove", false), &changed);
	out = {{"agency_id", agency}, {"built", changed}, {"text", text}};
	return true;
}

// Open the agent's agency's won bid on a territory for secs (0 = close).
bool OpOpen(sqlite3* db, const nlohmann::json& p, nlohmann::json& out, std::string& msg) {
	const int64_t cid = p.value("character_id", (int64_t)0);
	const int64_t pos = p.value("position", (int64_t)-1);
	if (!IsServiceCharacter(cid) || pos < 0 || !AvaZones::Valid((uint32_t)pos)) { msg = "not a service agent / bad position"; return false; }
	const int64_t agency = Scalar(db, "SELECT m.agency_id FROM ga_agency_members m JOIN ga_characters c "
		"ON c.user_id = m.user_id WHERE c.id = ?1", cid);
	if (agency == 0) { msg = "service agent has no agency"; return false; }
	out = {{"agency_id", agency}, {"opened", AvaParity::OpenBid(agency, (uint32_t)pos, p.value("secs", (int64_t)0))}};
	return true;
}

bool OpList(sqlite3* db, nlohmann::json& out) {
	out = {{"agents", nlohmann::json::array()}};
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT u.id, u.username, c.id, c.profile_id, c.xp, c.credits, COALESCE(m.agency_id, 0) "
	        "FROM ga_users u LEFT JOIN ga_characters c ON c.user_id = u.id AND c.deleted_at = 0 "
	        "LEFT JOIN ga_agency_members m ON m.user_id = u.id WHERE u.service_account = 1 ORDER BY u.id",
	        -1, &s, nullptr) != SQLITE_OK) return true;
	while (sqlite3_step(s) == SQLITE_ROW)
		out["agents"].push_back({{"user_id", sqlite3_column_int64(s, 0)},
			{"name", reinterpret_cast<const char*>(sqlite3_column_text(s, 1))},
			{"character_id", sqlite3_column_int64(s, 2)}, {"profile_id", sqlite3_column_int64(s, 3)},
			{"xp", sqlite3_column_int64(s, 4)}, {"credits", sqlite3_column_int64(s, 5)},
			{"agency_id", sqlite3_column_int64(s, 6)}});
	sqlite3_finalize(s);
	return true;
}

}  // namespace

void EnsureSchema() {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	// fails harmlessly when the column exists
	sqlite3_exec(db, "ALTER TABLE ga_users ADD COLUMN service_account INTEGER NOT NULL DEFAULT 0",
		nullptr, nullptr, nullptr);
}

bool IsServiceUser(int64_t user_id) {
	sqlite3* db = Database::GetConnection();
	if (!db || user_id <= 0) return false;
	return Scalar(db, "SELECT service_account FROM ga_users WHERE id = ?1", user_id) == 1;
}

bool IsServiceCharacter(int64_t character_id) {
	sqlite3* db = Database::GetConnection();
	if (!db || character_id <= 0) return false;
	return Scalar(db, "SELECT u.service_account FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
		"WHERE c.id = ?1 AND c.deleted_at = 0", character_id) == 1;
}

bool Admin(const nlohmann::json& payload, nlohmann::json& out, std::string& message) {
	std::lock_guard<std::mutex> lk(g_mu);
	sqlite3* db = Database::GetConnection();
	if (!db) { message = "no db"; return false; }
	EnsureSchema();
	const std::string op = payload.value("op", std::string());
	if (op == "agent") return OpAgent(db, payload, out, message);
	if (op == "agency") return OpAgency(db, payload, out, message);
	if (op == "bid") return OpBid(payload, out, message);
	if (op == "match") return OpMatch(db, payload, out, message);
	if (op == "list") return OpList(db, out);
	if (op == "assign") return OpTerritory(db, payload, out, message, true);
	if (op == "facility") return OpTerritory(db, payload, out, message, false);
	if (op == "produce") return OpProduce(db, payload, out, message);
	if (op == "open") return OpOpen(db, payload, out, message);
	message = "unknown op";
	return false;
}

}  // namespace ServiceAgents
