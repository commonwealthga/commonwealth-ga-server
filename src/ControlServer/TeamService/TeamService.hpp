#pragma once
// TeamService — server-side team state: pending invites + team membership.
// Wire protocol RE in team-tcp-protocol.md:
//   TEAM_INVITE      0x004A  C→S request; S→C message display (MSG_ID 0x36E + tokens)
//   TEAM_INVITATION  0x004B  S→invitee popup (MSG_ID 20418 + LEADER_NAME)
//   TEAM_EXPIRED_INVITE 0x004C  S→invitee popup dismiss (no fields)
//   TEAM_ACCEPT      0x004D  C→S popup "accept" (no fields — correlate by session)
//   TEAM_LEAVE       0x004F  C→S popup "decline" OR leave-team (no fields)
//   TEAM_UPDATE      0x0051  S→C roster (CTeamClient FUN_1092a5a0)

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <asio.hpp>

#include "src/ControlServer/MatchmakingService/Domain.hpp"  // QueuedParty

// One TEAM_UPDATE member row (CTeamClient member node, FUN_1092a040).
struct TeamRosterRow {
    int64_t     character_id = 0;
    std::string player_name;
    uint32_t    profile_id = 0;
    uint32_t    class_msg_id = 0;     // sent as digit string (0xC2 is wire-string)
    uint32_t    status_msg_id = 0;    // 0 = blank
    uint32_t    map_name_msg_id = 0;
    uint32_t    level = 50;
    float       skill_rating = 0.0f;
    bool        offline = false;      // OFFLINE_FLAG — "disconnected" icon
    std::string agency_name;
    std::string alliance_name;
};

struct TeamRoster {
    uint64_t team_id = 0;
    uint32_t team_mode = 1;           // TODO verify enum; mode 3 = different UI event set
    int64_t  leader_character_id = 0;
    std::vector<TeamRosterRow> rows;  // empty => client clears "in a team" state
};

class TeamService {
public:
    // Localized message templates (asm_data_set_msg_translations).
    static constexpr uint32_t MSG_ALREADY_IN_YOUR_TEAM   = 20413; // "@@player_name@@ is already a member of your team."
    static constexpr uint32_t MSG_IN_ANOTHER_TEAM        = 20414; // "@@player_name@@ is a member of another team."
    static constexpr uint32_t MSG_INVITE_SENT            = 20415; // "Team invite was sent to @@player_name@@."
    static constexpr uint32_t MSG_HAS_PENDING_INVITE     = 20416; // "@@player_name@@ has a pending team invite."
    static constexpr uint32_t MSG_INVITATION_POPUP       = 20418; // "@@leader_name@@ has invited you to join a team."
    static constexpr uint32_t MSG_INVITE_DECLINED        = 20419; // "@@player_name@@ has declined your team invite."
    static constexpr uint32_t MSG_TOO_MANY_PENDING       = 20456; // "Too many pending invites to send another."
    static constexpr uint32_t MSG_INVITE_EXPIRED         = 20458; // "The team invite expired."
    static constexpr uint32_t MSG_CANNOT_INVITE_SELF     = 20548; // "You cannot invite yourself to a team."
    static constexpr uint32_t MSG_RESPOND_TO_YOURS_FIRST = 21976; // "You have a team invite you must respond to first."
    static constexpr uint32_t MSG_RANK_CANNOT_INVITE     = 14258; // "Your rank does not allow you to invite another player."
    static constexpr uint32_t MSG_PLAYER_NOT_ONLINE      = 17787; // "Player @@PLAYER_NAME@@ not found online. ..."
    static constexpr uint32_t MSG_NEW_LEADER             = 21795; // "@@player_name@@ is now the team leader."
    static constexpr uint32_t MSG_JOINED_TEAM            = 21796; // "@@player_name@@ has joined the team."
    static constexpr uint32_t MSG_LEFT_TEAM              = 21797; // "@@player_name@@ has left the team."
    static constexpr uint32_t MSG_YOU_DECLINED           = 28463; // "You declined a team invite from @@player_name@@."
    static constexpr uint32_t MSG_NOT_IN_A_TEAM          = 23044; // "You are not in a team."
    static constexpr uint32_t MSG_JOINED_QUEUE           = 18306; // "You have joined a match queue."
    static constexpr uint32_t MSG_LEFT_QUEUE             = 18308; // "You have left the match queue"
    static constexpr uint32_t MSG_TEAM_ENTERED_QUEUE     = 21794; // "@@player_name@@ entered the team into a match queue."
    static constexpr uint32_t MSG_QUEUE_LEADER_ONLY      = 18305; // "You are not the team leader, only leader can manage the match queue."
    static constexpr uint32_t MSG_TEAM_TOO_LARGE         = 22972; // "Your team is too large for the selected match queue."

    static void SetIoContext(asio::io_context* io);

    // TEAM_INVITE: validate, then invitation popup to target + confirmation
    // to inviter, or a rejection message to the inviter.
    static void RequestInvite(const std::string& inviter_guid,
                              const std::string& inviter_name,
                              const std::string& target_player_name);

    // TEAM_ACCEPT: join the inviter's team (creating it if needed) and
    // broadcast the roster.
    static void AcceptInvite(const std::string& invitee_guid);

    // TEAM_LEAVE: decline a pending incoming invite if one exists, otherwise
    // leave the current team (promote/disband as needed).
    static void HandleLeave(const std::string& session_guid);

    static bool IsTeamed(const std::string& session_guid);
    static bool IsLeader(const std::string& session_guid);

    // Session guids of all ONLINE members of the team containing `session_guid`
    // (leader + members), or just { session_guid } if the player isn't in a
    // team. Offline-marked members are skipped (they can't be routed into a
    // match). Used by the challenge system to pull whole teams into a private
    // challenge match rather than just the two individuals.
    static std::vector<std::string> GetTeamMemberGuids(const std::string& session_guid);

    // --- Team queueing -----------------------------------------------------

    // Build a matchmaking party from the team led by `leader_guid` (live
    // profile_id + user_id resolved from PlayerSessionStore). nullopt if the
    // guid isn't a team leader. The party_id is the team id.
    static std::optional<QueuedParty> BuildParty(const std::string& leader_guid);

    // After a leader queues their team: HUD "IN QUEUE" + a chat line to every
    // member (leader: "you joined", others: "<leader> entered the team into a
    // queue"). Sets each member's session current_match_queue_id_.
    static void NotifyTeamQueued(uint64_t team_id, uint32_t queue_id, uint32_t name_msg_id);

    // Any composition change dequeues the player's current queue entry (their
    // solo party OR the team they're in) first, messaging everyone affected
    // with "you left the match queue" + clearing their HUD. No-op if not
    // queued. Leader must manually re-queue afterward.
    static void DequeueForCompositionChange(const std::string& session_guid);

    // Disconnect cleanup: drops pending invites involving this session and
    // marks their team membership offline (membership itself survives —
    // reconnect rebinding is a future slice).
    static void HandleDisconnect(const std::string& session_guid);

    // Voluntary exit (Disconnect button / Quit / relogin): drops pending
    // invites AND removes the player from their team — unlike a socket drop,
    // which keeps offline-marked membership.
    static void HandleVoluntaryExit(const std::string& session_guid);

private:
    static constexpr int kInviteLifetimeSec   = 30;
    static constexpr int kMaxOutgoingInvites  = 5;

    struct PendingInvite {
        uint64_t    id = 0;
        std::string inviter_guid;
        std::string inviter_name;
        std::string invitee_guid;
        std::string invitee_name;
        std::shared_ptr<asio::steady_timer> timer;
    };

    struct TeamMember {
        std::string session_guid;
        int64_t     character_id = 0;
        std::string player_name;
        uint32_t    profile_id = 0;
        bool        offline = false;
    };

    struct Team {
        uint64_t id = 0;
        std::string leader_guid;
        std::vector<TeamMember> members;
    };

    static void ExpireInvite(uint64_t invite_id);
    static void CancelInvitesFor(const std::string& session_guid);

    // Join-in-progress: if `inviter_guid` is currently inside a live
    // PARTY_LOCKED mission (a team-created match), send the freshly-joined
    // invitee a "mission is ready" popup routing them into that instance.
    // SEALED matches (Double Agent) never admit invitees.
    static void MaybeRouteInviteeIntoTeamMatch(const std::string& inviter_guid,
                                               const std::string& invitee_guid);

    // Removes the player from their team (promote/disband + notifications).
    // Returns false if they weren't in one.
    static bool LeaveCurrentTeam(const std::string& session_guid);

    // Locked-context helpers (callers hold mutex_).
    static Team* FindTeamByMemberLocked(const std::string& session_guid);
    static TeamRoster BuildRosterLocked(const Team& team);

    // Sends the roster to every (online) member. Takes its own snapshot.
    static void BroadcastRoster(uint64_t team_id);

    static std::mutex mutex_;
    static asio::io_context* io_ctx_;
    static std::vector<PendingInvite> invites_;
    static std::vector<Team> teams_;
    static uint64_t next_invite_id_;
    static uint64_t next_team_id_;
};
