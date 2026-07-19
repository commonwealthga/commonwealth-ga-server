#pragma once

#include <cstdint>
#include <string>

// Per-user key/value preferences persisted in ga_user_preferences
// (id, user_id, config_key, config_value; UNIQUE(user_id, config_key)).
//
// All methods are game-thread only (game loop is single-threaded) — no locks.
// A user's rows are read from the DB ONCE (explicit Preload at PLAYER_REGISTER,
// or lazily on first access) and served from memory afterwards; Set writes
// through to both cache and DB. After Preload, Get/GetBool never touch the DB,
// so they are safe on hot per-connection paths (replication).
class UserPreferences {
public:
	// Load (or refresh nothing if already loaded) all prefs for a user.
	// Call at PLAYER_REGISTER so hot-path Gets never hit the DB.
	static void Preload(int64_t user_id);

	// Value for (user, key); default_value when unset. Cached.
	static std::string Get(int64_t user_id, const std::string& key,
	                       const std::string& default_value);

	// Bool view of Get: "1"/"true" → true, "0"/"false" → false, else default.
	static bool GetBool(int64_t user_id, const std::string& key, bool default_value);

	// Upsert (user, key) → value. Updates cache immediately; DB write-through.
	static void Set(int64_t user_id, const std::string& key, const std::string& value);

	// Flip a bool pref (current value read as GetBool with default_value).
	// Returns the NEW value.
	static bool ToggleBool(int64_t user_id, const std::string& key, bool default_value);
};
