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

    // ---- Agencies (design 2026-07-18) --------------------------------------
    struct AgencyMemberRow {
        int64_t     user_id      = 0;
        int64_t     character_id = 0;
        std::string player_name;
        int         rank_id      = 0;
        std::string public_comment;
        std::string officer_comment;
        uint32_t    profile_id = 0;   // ga_characters.profile_id (class), joined in
    };
    // PERMISSION_FLAGS (0x3B0) bits, read off single-checkbox AGENCY_UPDATE_RANKS
    // submits from the Management tab's "Rank Abilities" panel. Bits 0-3 (0x0F)
    // ride along on every submit but map to no visible checkbox.
    enum AgencyPerm : uint32_t {
        AGENCY_PERM_PROMOTE            = 0x00000040,
        AGENCY_PERM_DEMOTE             = 0x00000080,
        AGENCY_PERM_INVITE             = 0x00000100,
        AGENCY_PERM_KICK               = 0x00000200,
        AGENCY_PERM_EDIT_DESCRIPTION   = 0x00000400,
        AGENCY_PERM_EDIT_PUBLIC_MSG    = 0x00000800,
        AGENCY_PERM_EDIT_OFFICER_MSG   = 0x00001000,
        AGENCY_PERM_VIEW_OFFICER_MSG   = 0x00002000,
        AGENCY_PERM_EDIT_MOTD          = 0x00008000,
        AGENCY_PERM_FACILITY_MGMT      = 0x00020000,
        AGENCY_PERM_INVENTORY_REMOVAL  = 0x00040000,
    };

    struct AgencyRankRow {
        int         rank_id     = 0;
        int         rank_level  = 0;
        int         permissions = 0;
        std::string rank_name;
    };
    struct AgencyInfo {
        int64_t     id = 0;
        std::string name;
        std::string motd;
        std::string information;
        float       color_r = 0, color_g = 0, color_b = 0;
        bool        recruiting = false;
        std::string recruiting_text;
        bool        sub_only = false;
        int64_t     leader_user_id = 0;
        std::vector<AgencyMemberRow> members;
        std::vector<AgencyRankRow>   ranks;
    };

    // Create an agency owned by leader_user_id. Seeds the 3 default ranks and
    // adds the leader as a member at the top rank, atomically. Returns the new
    // agency id, or 0 on failure (name already taken, or the account is already
    // in an agency).
    static int64_t CreateAgency(const std::string& name,
                                float r, float g, float b,
                                int64_t leader_user_id,
                                int64_t leader_character_id,
                                const std::string& leader_name);

    // Agency id the account currently belongs to, or 0 if none.
    static int64_t GetAgencyIdForUser(int64_t user_id);

    // Full agency record (meta + members + ranks), or nullopt if not found.
    static std::optional<AgencyInfo> GetAgencyInfo(int64_t agency_id);

    // Delete an agency and everything attached: its members, ranks, any alliance
    // it owns (+ that alliance's memberships), and its own alliance membership.
    static void DisbandAgency(int64_t agency_id);

    // Add an account to an agency (invite-accept). rank_id 2 = default Member.
    // Returns false if the account is already in an agency or the insert failed.
    static bool AddAgencyMember(int64_t agency_id, int64_t user_id,
                                int64_t character_id,
                                const std::string& player_name,
                                int rank_id);

    // Change a member's rank (promote / demote). Member keyed by character_id —
    // that's what the client sends back as PLAYER_ID. False if no such member.
    static bool SetAgencyMemberRank(int64_t agency_id, int64_t character_id,
                                    int rank_id);

    // Drop a member from an agency (kick / leave). Keyed by character_id.
    static bool RemoveAgencyMember(int64_t agency_id, int64_t character_id);

    // ---- Alliances (groups of agencies) ------------------------------------
    struct AllianceMemberRow {
        int64_t     agency_id       = 0;
        std::string agency_name;
        int         member_count    = 0;  // agents in that agency
        int         territory_count = 0;
    };
    struct AllianceInfo {
        int64_t     id = 0;
        std::string name;
        std::string motd;
        std::string information;
        int64_t     owner_agency_id = 0;
        std::string owner_agency_name;
        int64_t     created_at = 0;   // unix seconds
        std::vector<AllianceMemberRow> members;
    };

    // Alliance id the agency currently belongs to, or 0 if none.
    static int64_t GetAllianceIdForAgency(int64_t agency_id);

    // Create an alliance owned by owner_agency_id and add it as the first
    // member, atomically. Returns the new alliance id, or 0 on failure (name
    // taken, or the agency is already in an alliance).
    static int64_t CreateAlliance(const std::string& name, int64_t owner_agency_id);

    // Add an agency to an alliance (invite-accept / join). Returns false if the
    // agency is already in an alliance or the insert failed.
    static bool AddAgencyToAlliance(int64_t alliance_id, int64_t agency_id);

    // Full alliance record (meta + member agencies), or nullopt if not found.
    static std::optional<AllianceInfo> GetAllianceInfo(int64_t alliance_id);

    // Delete an alliance + all its agency memberships.
    static void DisbandAlliance(int64_t alliance_id);

    // Remove one agency from its alliance (member leave / owner kick).
    static void RemoveAgencyFromAlliance(int64_t agency_id);

    // Agency MOTD (is_motd) or description/information text.
    static bool SetAgencyText(int64_t agency_id, bool is_motd,
                              const std::string& text);

    // A member's public or officer note. Member keyed by character_id.
    static bool SetAgencyMemberComment(int64_t agency_id, int64_t character_id,
                                       bool officer, const std::string& text);

    // Replace an agency's whole rank table (AGENCY_UPDATE_RANKS sends every row,
    // so adds / renames / permission edits / deletes all arrive together).
    // Members sitting on a deleted rank fall to the lowest-authority rank left.
    static bool ReplaceAgencyRanks(int64_t agency_id,
                                   const std::vector<AgencyRankRow>& ranks);

    // Recruiting listing: text + the two flags (Recruiting Members / AvA Only).
    static bool SetAgencyRecruiting(int64_t agency_id, const std::string& text,
                                    bool recruiting, bool sub_only);

    // Hand the agency to another account (Transfer Leader). Rank rows are moved
    // by the caller; this only updates ga_agencies.leader_user_id.
    static bool SetAgencyLeader(int64_t agency_id, int64_t user_id);

    // character_id -> map name, for the agency's members currently in a running
    // instance. One query per roster poll; deliberately narrower than
    // InstanceRegistry::GetActiveSearchablePlayers (which joins users/players
    // for the name columns the roster already has).
    static std::map<int64_t, std::string> GetOnlineAgencyMemberMaps(int64_t agency_id);

    // Every account in every agency of an alliance. Used to push alliance-state
    // changes to the players affected, so their panels don't need a relog.
    static std::vector<int64_t> GetAllianceMemberUserIds(int64_t alliance_id);

    // Every account in one agency.
    static std::vector<int64_t> GetAgencyMemberUserIds(int64_t agency_id);

    // Agency led by the account whose leader player_name matches (rank_id 0 =
    // leader). Used to resolve ALLIANCE_INVITE (invite by leader name). 0 if none.
    static int64_t GetAgencyIdByLeaderName(const std::string& leader_name);
};
