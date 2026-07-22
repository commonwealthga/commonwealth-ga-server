#pragma once

#include "sqlite3.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

class Database {
private:
    static sqlite3* connection;
    static std::string db_path_;
public:
    static int Callback(void* data, int argc, char** argv, char** azColName);
    static sqlite3* GetConnection();
    static void CloseConnection();
    static void Init();
    static void SetDbPath(const std::string& path);

    // Returns "" (not found), "active", or "complete"
    static std::string GetQuestStatus(int64_t character_id, int quest_id);
    static void AcceptQuest(int64_t character_id, int quest_id);
    static void CompleteQuest(int64_t character_id, int quest_id);
    static void AbandonQuest(int64_t character_id, int quest_id);

    // ---- User moderation: session history ----------------------------------
    // outcome: "ok" (live or completed), "rejected" (validation failure),
    // "banned" (ban triggered). Returns the row id, or 0 on failure.
    static int64_t InsertSession(int64_t user_id_or_zero,
                                 const std::string& username,
                                 const std::string& ip,
                                 const std::string& outcome);
    // Sets logout_at = strftime('%s','now') on the given row.
    static void FinalizeSession(int64_t session_row_id);

    // ---- User moderation: bans ---------------------------------------------
    struct ActiveBan {
        int64_t     id        = 0;
        std::string reason;
        int64_t     banned_at = 0;
    };
    static std::optional<ActiveBan> FindActiveBanForUser(int64_t user_id);
    static std::optional<ActiveBan> FindActiveBanForIp  (const std::string& ip);

    static void InsertOrReplaceUserBan(int64_t user_id,       const std::string& reason);
    static void InsertOrReplaceIpBan  (const std::string& ip, const std::string& reason);
    static void LiftUserBan(int64_t user_id);
    static void LiftIpBan  (const std::string& ip);

    // ---- User moderation: dashboard read APIs ------------------------------
    struct SessionRow {
        int64_t                id       = 0;
        int64_t                user_id  = 0;  // 0 = NULL in DB
        std::string            username;
        std::string            ip;
        int64_t                login_at = 0;
        std::optional<int64_t> logout_at;
        std::string            outcome;
    };
    static std::vector<SessionRow> GetRecentSessionsDistinctByUser(int limit);
    static std::vector<SessionRow> GetSessionsForUser(int64_t user_id, int limit);
    static std::vector<SessionRow> GetSessionsForIp  (const std::string& ip, int limit);

    struct ActiveBanRow {
        int64_t     id              = 0;
        int64_t     user_id_or_zero = 0;
        std::string ip_or_empty;
        std::string reason;
        int64_t     banned_at       = 0;
    };
    static std::vector<ActiveBanRow> GetActiveUserBans();
    static std::vector<ActiveBanRow> GetActiveIpBans();

    // Username → user_id lookup (used by admin "ban" action when the
    // dashboard sends a username rather than a numeric id). Returns 0 if
    // the user does not exist.
    static int64_t FindUserIdByUsername(const std::string& username);

    // Set the operator "verified for PvP" flag on an account. Used by the
    // dashboard "pvp-toggle" admin action. Returns false if the update failed.
    static bool SetUserPvpVerification(int64_t user_id, bool verified);

    // Read an account's "verified for PvP" flag. Returns false for unknown /
    // invalid user_id. Used by the matchmaker to withhold unverified players
    // from queues flagged requires_pvp_verification.
    static bool IsUserVerifiedForPvp(int64_t user_id);

    // Clear an account's password verifier (set password_verifier = NULL) so
    // the player's next login re-registers their password (trust-on-first-use).
    // Used by the dashboard "reset-password" admin action for players locked
    // out after typing a throwaway password on their first verified login.
    // registered_at is left untouched. Returns false if the update failed.
    static bool ClearUserVerifier(int64_t user_id);

    // ---- User roles (spectator mode, design 2026-07-18) --------------------
    // Generic role grants on ga_users, e.g. "spectator". Control-server-owned
    // and authoritative — the game-server DLL never queries this directly,
    // it only trusts the pre-vetted flag threaded through the per-connection
    // control message (see PlayerInfo.is_spectator).
    static bool UserHasRole(int64_t user_id, const std::string& role);
    static void GrantRole(int64_t user_id, const std::string& role);
    static void RevokeRole(int64_t user_id, const std::string& role);

    // ---- Match stats (design 2026-06-12) -----------------------------------
    struct MatchEventRow {
        int64_t instance_id = 0;
        double  game_time   = 0;
        std::string event_type;
        // 0 = NULL for all identity/optional fields below.
        int64_t actor_user_id = 0,  actor_character_id = 0;
        int     actor_bot_id = 0,   actor_task_force = 0;
        int64_t target_user_id = 0, target_character_id = 0;
        int     target_bot_id = 0,  target_task_force = 0;
        int64_t owner_user_id = 0,  owner_character_id = 0;
        int     device_id = 0;
        int64_t detail = 0;
        int     flags = 0;
    };
    // Inserts one event row (ts stamped now). Returns rowid, 0 on failure.
    static int64_t InsertMatchEvent(const MatchEventRow& row);

    struct MatchPlayerStatsRow {
        int64_t instance_id = 0, user_id = 0, character_id = 0;
        int     task_force = 0;
        int     scores[11] = {};   // r_Scores order (STYPE_*)
        double  capture_seconds = 0, contest_seconds = 0;
        int     objective_captures = 0;
        int     beacon_spawns_provided = 0, beacon_spawns_used = 0;
        int     beacons_destroyed = 0;
        double  time_played_seconds = 0;
    };
    // Absolute totals; upsert on (instance_id, character_id, task_force).
    static void UpsertMatchPlayerStats(const MatchPlayerStatsRow& row);

    // Write-once outcome stamp (WHERE outcome IS NULL AND is_home_map=0).
    // winning_task_force 0 = NULL.
    static void SetInstanceOutcomeIfNull(int64_t instance_id,
                                         const std::string& outcome,
                                         int winning_task_force);

    // Final per-taskforce team-death totals from MSG_MISSION_ENDED
    // (challenge bonus source data).
    static void SetInstanceDeathCounts(int64_t instance_id,
                                       int count_deaths_attackers,
                                       int count_deaths_defenders);
};
