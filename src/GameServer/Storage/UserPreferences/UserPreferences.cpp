#include "src/GameServer/Storage/UserPreferences/UserPreferences.hpp"

#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <unordered_map>

namespace {

struct UserPrefs {
	std::unordered_map<std::string, std::string> values;
};

// user_id → loaded prefs. Presence in the map means the DB was already read
// for that user (possibly zero rows) — subsequent Gets are pure memory.
std::unordered_map<int64_t, UserPrefs> g_cache;

bool g_tableEnsured = false;

// The control server creates the table at boot too; this covers a DLL
// instance run against a DB the control server hasn't touched yet.
void EnsureTable(sqlite3* db) {
	if (g_tableEnsured) return;
	g_tableEnsured = true;
	char* err = nullptr;
	if (sqlite3_exec(db,
	        "CREATE TABLE IF NOT EXISTS ga_user_preferences ("
	        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
	        "  user_id      INTEGER NOT NULL,"
	        "  config_key   TEXT    NOT NULL,"
	        "  config_value TEXT    NOT NULL,"
	        "  UNIQUE(user_id, config_key)"
	        ")",
	        nullptr, nullptr, &err) != SQLITE_OK) {
		Logger::Log("userprefs", "[UserPreferences] ensure table failed: %s\n",
			err ? err : "?");
		if (err) sqlite3_free(err);
	}
}

// Returns the cached prefs for the user, reading them from the DB on first
// call. nullptr only when the DB connection is unavailable AND the user was
// never loaded (so a later call retries instead of caching emptiness).
UserPrefs* LoadUser(int64_t user_id) {
	auto it = g_cache.find(user_id);
	if (it != g_cache.end()) return &it->second;

	sqlite3* db = Database::GetConnection();
	if (!db) return nullptr;
	EnsureTable(db);

	UserPrefs& prefs = g_cache[user_id];
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT config_key, config_value FROM ga_user_preferences WHERE user_id = ?",
	        -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(stmt, 1, user_id);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const char* k = (const char*)sqlite3_column_text(stmt, 0);
			const char* v = (const char*)sqlite3_column_text(stmt, 1);
			if (k && v) prefs.values[k] = v;
		}
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("userprefs", "[UserPreferences] load prepare failed: %s\n",
			sqlite3_errmsg(db));
	}
	Logger::Log("userprefs", "[UserPreferences] loaded %zu pref(s) for user %lld\n",
		prefs.values.size(), (long long)user_id);
	return &prefs;
}

} // namespace

void UserPreferences::Preload(int64_t user_id) {
	if (user_id > 0) LoadUser(user_id);
}

std::string UserPreferences::Get(int64_t user_id, const std::string& key,
                                 const std::string& default_value) {
	UserPrefs* prefs = LoadUser(user_id);
	if (!prefs) return default_value;
	auto it = prefs->values.find(key);
	return (it != prefs->values.end()) ? it->second : default_value;
}

bool UserPreferences::GetBool(int64_t user_id, const std::string& key,
                              bool default_value) {
	const std::string v = Get(user_id, key, "");
	if (v == "1" || v == "true")  return true;
	if (v == "0" || v == "false") return false;
	return default_value;
}

void UserPreferences::Set(int64_t user_id, const std::string& key,
                          const std::string& value) {
	// Cache first — authoritative for this session even if the DB write fails.
	UserPrefs* prefs = LoadUser(user_id);
	if (prefs) prefs->values[key] = value;

	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("userprefs",
			"[UserPreferences] Set user=%lld key=%s: no DB connection, cache-only\n",
			(long long)user_id, key.c_str());
		return;
	}
	EnsureTable(db);
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
	        "INSERT INTO ga_user_preferences (user_id, config_key, config_value) "
	        "VALUES (?, ?, ?) "
	        "ON CONFLICT(user_id, config_key) DO UPDATE SET config_value = excluded.config_value",
	        -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(stmt, 1, user_id);
		sqlite3_bind_text (stmt, 2, key.c_str(),   -1, SQLITE_TRANSIENT);
		sqlite3_bind_text (stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			Logger::Log("userprefs",
				"[UserPreferences] Set user=%lld key=%s FAILED: %s\n",
				(long long)user_id, key.c_str(), sqlite3_errmsg(db));
		}
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("userprefs", "[UserPreferences] Set prepare failed: %s\n",
			sqlite3_errmsg(db));
	}
	Logger::Log("userprefs", "[UserPreferences] user=%lld %s=%s\n",
		(long long)user_id, key.c_str(), value.c_str());
}

bool UserPreferences::ToggleBool(int64_t user_id, const std::string& key,
                                 bool default_value) {
	const bool newValue = !GetBool(user_id, key, default_value);
	Set(user_id, key, newValue ? "1" : "0");
	return newValue;
}
