#include "src/ControlServer/TeamService/TeamService.hpp"

#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Constants/GameTypes.h"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>

std::mutex TeamService::mutex_;
asio::io_context* TeamService::io_ctx_ = nullptr;
std::vector<TeamService::PendingInvite> TeamService::invites_;
std::vector<TeamService::Team> TeamService::teams_;
uint64_t TeamService::next_invite_id_ = 1;
uint64_t TeamService::next_team_id_ = 1;

namespace {

bool NamesEqualCi(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    return true;
}

// Class-name msg id per profile (mirrors TcpSession's FallbackClassMsgId).
uint32_t ClassMsgIdFor(uint32_t profile_id) {
    switch (profile_id) {
        case GA_G::PROFILE_ID_ROBOTIC: return 15447;
        case GA_G::PROFILE_ID_ASSAULT: return 15448;
        case GA_G::PROFILE_ID_RECON:   return 15449;
        case GA_G::PROFILE_ID_MEDIC:   return 15450;
        default:                       return 22976;
    }
}

uint32_t MapMsgIdForSession(const std::string& session_guid) {
    auto placed = InstanceRegistry::GetInstancePlayerTaskForce(session_guid);
    if (!placed) return 0;
    auto inst = InstanceRegistry::GetInstanceById(placed->first);
    if (!inst) return 0;
    auto rec = MapGameInfo::LookupByName(inst->map_name);
    return rec ? rec->friendly_name_msg_id : 0;
}

} // namespace

void TeamService::SetIoContext(asio::io_context* io) {
    io_ctx_ = io;
}

TeamService::Team* TeamService::FindTeamByMemberLocked(const std::string& session_guid) {
    for (auto& team : teams_)
        for (const auto& m : team.members)
            if (m.session_guid == session_guid) return &team;
    return nullptr;
}

bool TeamService::IsTeamed(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    return FindTeamByMemberLocked(session_guid) != nullptr;
}

bool TeamService::IsLeader(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    Team* team = FindTeamByMemberLocked(session_guid);
    return team && team->leader_guid == session_guid;
}

TeamRoster TeamService::BuildRosterLocked(const Team& team) {
    TeamRoster roster;
    roster.team_id = team.id;
    roster.team_mode = 1;
    for (const auto& m : team.members) {
        if (m.session_guid == team.leader_guid)
            roster.leader_character_id = m.character_id;

        TeamRosterRow row;
        row.character_id    = m.character_id;
        row.player_name     = m.player_name;
        row.profile_id      = m.profile_id;
        row.class_msg_id    = ClassMsgIdFor(m.profile_id);
        row.map_name_msg_id = MapMsgIdForSession(m.session_guid);
        row.offline         = m.offline;
        roster.rows.push_back(std::move(row));
    }
    return roster;
}

void TeamService::BroadcastRoster(uint64_t team_id) {
    TeamRoster roster;
    std::vector<std::string> recipients;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(teams_.begin(), teams_.end(),
            [&](const Team& t) { return t.id == team_id; });
        if (it == teams_.end()) return;
        roster = BuildRosterLocked(*it);
        for (const auto& m : it->members) recipients.push_back(m.session_guid);
    }
    for (const auto& guid : recipients)
        TcpSession::DeliverTeamUpdate(guid, roster);
}

void TeamService::RequestInvite(const std::string& inviter_guid,
                                const std::string& inviter_name,
                                const std::string& target_player_name) {
    // Self-invite. The client blocks this locally (msg 20548) but guard anyway.
    if (NamesEqualCi(inviter_name, target_player_name)) {
        TcpSession::DeliverTeamSystemMessage(inviter_guid, MSG_CANNOT_INVITE_SELF, "");
        return;
    }

    auto target = PlayerSessionStore::GetByPlayerName(target_player_name);
    if (!target) {
        TcpSession::DeliverTeamSystemMessage(inviter_guid, MSG_PLAYER_NOT_ONLINE,
                                             target_player_name);
        return;
    }

    // TODO(matchmaking): retail also rejects when either side sits in a match
    // queue (msgs 22369 / 23065); MatchmakingService has no membership query yet.

    // Validate + insert under the lock; deliver after releasing it so we
    // never hold mutex_ while Deliver* takes sessions_mutex_.
    uint64_t invite_id = 0;
    uint32_t reject_msg = 0;
    bool     reject_with_name = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        Team* inviter_team = FindTeamByMemberLocked(inviter_guid);
        Team* target_team  = FindTeamByMemberLocked(target->session_guid);
        if (inviter_team && inviter_team->leader_guid != inviter_guid) {
            // An unteamed player or a team leader can invite; members can't.
            reject_msg = MSG_RANK_CANNOT_INVITE;
        } else if (target_team && target_team == inviter_team) {
            reject_msg = MSG_ALREADY_IN_YOUR_TEAM;
            reject_with_name = true;
        } else if (target_team) {
            reject_msg = MSG_IN_ANOTHER_TEAM;
            reject_with_name = true;
        }

        int outgoing = 0;
        for (const auto& inv : invites_) {
            if (reject_msg) break;
            if (inv.invitee_guid == inviter_guid) {
                // Inviter has an unanswered incoming invite of their own.
                reject_msg = MSG_RESPOND_TO_YOURS_FIRST;
                break;
            }
            if (inv.invitee_guid == target->session_guid) {
                reject_msg = MSG_HAS_PENDING_INVITE;
                reject_with_name = true;
                break;
            }
            if (inv.inviter_guid == inviter_guid) ++outgoing;
        }
        if (!reject_msg && outgoing >= kMaxOutgoingInvites)
            reject_msg = MSG_TOO_MANY_PENDING;

        if (!reject_msg) {
            PendingInvite inv;
            inv.id           = next_invite_id_++;
            inv.inviter_guid = inviter_guid;
            inv.inviter_name = inviter_name;
            inv.invitee_guid = target->session_guid;
            inv.invitee_name = target->player_name;
            if (io_ctx_) {
                inv.timer = std::make_shared<asio::steady_timer>(*io_ctx_);
                inv.timer->expires_after(std::chrono::seconds(kInviteLifetimeSec));
                const uint64_t id = inv.id;
                inv.timer->async_wait([id](std::error_code ec) {
                    if (!ec) ExpireInvite(id);
                });
            }
            invite_id = inv.id;
            invites_.push_back(std::move(inv));
        }
    }

    if (reject_msg) {
        TcpSession::DeliverTeamSystemMessage(inviter_guid, reject_msg,
            reject_with_name ? target->player_name : std::string());
        return;
    }

    Logger::Log("team", "[team] invite #%llu %s -> %s (expires in %ds)\n",
        (unsigned long long)invite_id, inviter_name.c_str(),
        target->player_name.c_str(), kInviteLifetimeSec);

    TcpSession::DeliverTeamSystemMessage(inviter_guid, MSG_INVITE_SENT, target->player_name);
    TcpSession::DeliverTeamInvitation(target->session_guid, inviter_name);
}

void TeamService::AcceptInvite(const std::string& invitee_guid) {
    // Pull the invite.
    PendingInvite inv;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(invites_.begin(), invites_.end(),
            [&](const PendingInvite& i) { return i.invitee_guid == invitee_guid; });
        if (it != invites_.end()) {
            inv = *it;
            if (inv.timer) inv.timer->cancel();
            invites_.erase(it);
            found = true;
        }
    }
    if (!found) {
        // Raced with expiry/cancel — the popup answered a dead invite.
        TcpSession::DeliverTeamSystemMessage(invitee_guid, MSG_INVITE_EXPIRED, "");
        return;
    }

    // Clear the client's queued invitation node (the dialog is answered, but
    // the pending list at CGameClient+0x18c is reaped via 0x4C).
    TcpSession::DeliverTeamInviteExpired(invitee_guid);

    // Composition change: dequeue both sides' current queue entries first
    // (the inviter's team-or-solo, and the invitee's solo). Leader re-queues.
    DequeueForCompositionChange(inv.inviter_guid);
    DequeueForCompositionChange(invitee_guid);

    auto inviter = PlayerSessionStore::GetByGuid(inv.inviter_guid);
    auto invitee = PlayerSessionStore::GetByGuid(invitee_guid);
    if (!inviter || !invitee) {
        TcpSession::DeliverTeamSystemMessage(invitee_guid, MSG_INVITE_EXPIRED, "");
        return;
    }

    uint64_t team_id = 0;
    bool stale = false;
    std::vector<std::string> announce_to;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        Team* invitee_team = FindTeamByMemberLocked(invitee_guid);
        Team* team = FindTeamByMemberLocked(inv.inviter_guid);
        if (invitee_team) {
            // Joined some other team between popup and accept; nothing sane to do.
            Logger::Log("team", "[team] accept from already-teamed %s ignored\n",
                inv.invitee_name.c_str());
            return;
        }
        if (team && team->leader_guid != inv.inviter_guid) {
            // Inviter became a non-leader member elsewhere meanwhile.
            stale = true;
        } else {
            if (!team) {
                Team fresh;
                fresh.id = next_team_id_++;
                fresh.leader_guid = inv.inviter_guid;
                TeamMember leader;
                leader.session_guid = inv.inviter_guid;
                leader.character_id = inviter->selected_character_id;
                leader.player_name  = inviter->player_name;
                leader.profile_id   = inviter->selected_profile_id;
                fresh.members.push_back(std::move(leader));
                teams_.push_back(std::move(fresh));
                team = &teams_.back();
            }

            TeamMember member;
            member.session_guid = invitee_guid;
            member.character_id = invitee->selected_character_id;
            member.player_name  = invitee->player_name;
            member.profile_id   = invitee->selected_profile_id;
            team->members.push_back(std::move(member));
            team_id = team->id;

            for (const auto& m : team->members)
                if (m.session_guid != invitee_guid)
                    announce_to.push_back(m.session_guid);
        }
    }

    if (stale) {
        TcpSession::DeliverTeamSystemMessage(invitee_guid, MSG_INVITE_EXPIRED, "");
        return;
    }

    Logger::Log("team", "[team] %s joined team #%llu (leader %s)\n",
        invitee->player_name.c_str(), (unsigned long long)team_id,
        inv.inviter_name.c_str());

    for (const auto& guid : announce_to)
        TcpSession::DeliverTeamSystemMessage(guid, MSG_JOINED_TEAM, invitee->player_name);
    BroadcastRoster(team_id);

    // Join-in-progress: if the team is already inside a PARTY_LOCKED mission,
    // give the new member a "mission is ready" popup into it.
    MaybeRouteInviteeIntoTeamMatch(inv.inviter_guid, invitee_guid);
}

void TeamService::MaybeRouteInviteeIntoTeamMatch(const std::string& inviter_guid,
                                                 const std::string& invitee_guid) {
    auto placed = InstanceRegistry::GetInstancePlayerTaskForce(inviter_guid);
    if (!placed) return;  // inviter isn't in a mission
    const int64_t instance_id = placed->first;
    const int     task_force  = placed->second;

    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    if (!inst) return;
    // Only live, PARTY_LOCKED team matches admit invitees. SEALED (Double
    // Agent) and OPEN (drop-in queues route via normal matchmaking) are skipped.
    if (inst->access_mode != "PARTY_LOCKED") return;
    if (inst->is_home_map || inst->end_mission_at != 0) return;
    if (inst->state != "READY" && inst->state != "DRAFTING") return;

    Logger::Log("team",
        "[team] routing invitee %s into team PARTY_LOCKED match instance=%lld tf=%d\n",
        invitee_guid.c_str(), (long long)instance_id, task_force);
    MatchmakingService::SetPreAssignedTeam(instance_id, invitee_guid, task_force);
    TcpSession::DeliverMatchInvitation(invitee_guid, instance_id, inst->game_mode, task_force);
}

void TeamService::HandleLeave(const std::string& session_guid) {
    // Decline path: an unanswered incoming invite takes precedence — the
    // client sends the same bare TEAM_LEAVE for the popup's decline button.
    PendingInvite declined;
    bool was_decline = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(invites_.begin(), invites_.end(),
            [&](const PendingInvite& i) { return i.invitee_guid == session_guid; });
        if (it != invites_.end()) {
            declined = *it;
            if (declined.timer) declined.timer->cancel();
            invites_.erase(it);
            was_decline = true;
        }
    }
    if (was_decline) {
        Logger::Log("team", "[team] invite #%llu declined by %s\n",
            (unsigned long long)declined.id, declined.invitee_name.c_str());
        // 0x4C clears the answered popup's node from CGameClient+0x18c.
        TcpSession::DeliverTeamInviteExpired(declined.invitee_guid);
        TcpSession::DeliverTeamSystemMessage(declined.inviter_guid, MSG_INVITE_DECLINED,
                                             declined.invitee_name);
        TcpSession::DeliverTeamSystemMessage(declined.invitee_guid, MSG_YOU_DECLINED,
                                             declined.inviter_name);
        return;
    }

    // Leave path.
    if (!LeaveCurrentTeam(session_guid))
        TcpSession::DeliverTeamSystemMessage(session_guid, MSG_NOT_IN_A_TEAM, "");
}

void TeamService::PromoteLeader(const std::string& requester_guid, int64_t target_character_id) {
    if (target_character_id == 0) return;

    uint64_t team_id = 0;
    std::string new_leader_name;
    std::vector<std::string> member_guids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Team* team = FindTeamByMemberLocked(requester_guid);
        if (!team || team->leader_guid != requester_guid) return;

        auto it = std::find_if(team->members.begin(), team->members.end(),
            [&](const TeamMember& m) { return m.character_id == target_character_id; });
        if (it == team->members.end()) return;
        if (it->session_guid == requester_guid) return;  // already leader

        team->leader_guid = it->session_guid;
        new_leader_name   = it->player_name;
        team_id = team->id;
        for (const auto& m : team->members) member_guids.push_back(m.session_guid);
    }

    Logger::Log("team", "[team] leader promoted to %s in team #%llu\n",
        new_leader_name.c_str(), (unsigned long long)team_id);

    for (const auto& guid : member_guids)
        TcpSession::DeliverTeamSystemMessage(guid, MSG_NEW_LEADER, new_leader_name);
    BroadcastRoster(team_id);
}

void TeamService::KickMember(const std::string& requester_guid, int64_t target_character_id) {
    if (target_character_id == 0) return;

    std::string target_guid;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Team* team = FindTeamByMemberLocked(requester_guid);
        if (!team || team->leader_guid != requester_guid) return;

        auto it = std::find_if(team->members.begin(), team->members.end(),
            [&](const TeamMember& m) { return m.character_id == target_character_id; });
        if (it == team->members.end()) return;
        if (it->session_guid == requester_guid) return;  // can't kick yourself

        target_guid = it->session_guid;
    }

    LeaveCurrentTeam(target_guid);
}

bool TeamService::LeaveCurrentTeam(const std::string& session_guid) {
    // Composition change: dequeue the whole team (if queued) before mutating.
    DequeueForCompositionChange(session_guid);

    uint64_t team_id = 0;
    bool disbanded = false;
    std::string leaver_name;
    std::string new_leader_name;
    std::vector<std::string> remaining_guids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Team* team = FindTeamByMemberLocked(session_guid);
        if (!team) return false;
        team_id = team->id;

        auto mit = std::find_if(team->members.begin(), team->members.end(),
            [&](const TeamMember& m) { return m.session_guid == session_guid; });
        leaver_name = mit->player_name;
        const bool was_leader = team->leader_guid == session_guid;
        team->members.erase(mit);

        if (team->members.size() <= 1) {
            // Last pairing gone — the remaining player becomes unteamed too.
            for (const auto& m : team->members) remaining_guids.push_back(m.session_guid);
            teams_.erase(std::remove_if(teams_.begin(), teams_.end(),
                [&](const Team& t) { return t.id == team_id; }), teams_.end());
            disbanded = true;
        } else {
            if (was_leader) {
                team->leader_guid = team->members.front().session_guid;
                new_leader_name   = team->members.front().player_name;
            }
            for (const auto& m : team->members) remaining_guids.push_back(m.session_guid);
        }
    }

    Logger::Log("team", "[team] %s left team #%llu%s\n", leaver_name.c_str(),
        (unsigned long long)team_id, disbanded ? " (disbanded)" : "");

    // Leaver's client clears its team state via an empty roster.
    TeamRoster empty;
    empty.team_id = team_id;
    TcpSession::DeliverTeamUpdate(session_guid, empty);

    for (const auto& guid : remaining_guids)
        TcpSession::DeliverTeamSystemMessage(guid, MSG_LEFT_TEAM, leaver_name);

    if (disbanded) {
        for (const auto& guid : remaining_guids)
            TcpSession::DeliverTeamUpdate(guid, empty);
    } else {
        if (!new_leader_name.empty())
            for (const auto& guid : remaining_guids)
                TcpSession::DeliverTeamSystemMessage(guid, MSG_NEW_LEADER, new_leader_name);
        BroadcastRoster(team_id);
    }
    return true;
}

void TeamService::ExpireInvite(uint64_t invite_id) {
    PendingInvite inv;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(invites_.begin(), invites_.end(),
            [&](const PendingInvite& i) { return i.id == invite_id; });
        if (it == invites_.end()) return;
        inv = *it;
        invites_.erase(it);
    }

    Logger::Log("team", "[team] invite #%llu %s -> %s expired\n",
        (unsigned long long)inv.id, inv.inviter_name.c_str(), inv.invitee_name.c_str());

    // Dismiss the invitee's popup, then tell both parties why.
    TcpSession::DeliverTeamInviteExpired(inv.invitee_guid);
    TcpSession::DeliverTeamSystemMessage(inv.invitee_guid, MSG_INVITE_EXPIRED, "");
    TcpSession::DeliverTeamSystemMessage(inv.inviter_guid, MSG_INVITE_EXPIRED, "");
}

void TeamService::CancelInvitesFor(const std::string& session_guid) {
    std::vector<PendingInvite> dropped;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = invites_.begin(); it != invites_.end();) {
            if (it->inviter_guid == session_guid || it->invitee_guid == session_guid) {
                if (it->timer) it->timer->cancel();
                dropped.push_back(*it);
                it = invites_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (const auto& inv : dropped) {
        // Dismiss the popup on the side that's still online.
        if (inv.invitee_guid != session_guid)
            TcpSession::DeliverTeamInviteExpired(inv.invitee_guid);
        else
            TcpSession::DeliverTeamSystemMessage(inv.inviter_guid, MSG_INVITE_EXPIRED, "");
    }
}

void TeamService::HandleDisconnect(const std::string& session_guid) {
    CancelInvitesFor(session_guid);

    // A disconnect is a composition change — dequeue the whole team first.
    DequeueForCompositionChange(session_guid);

    // Per spec an involuntary disconnect keeps the player in the team with a
    // "disconnected" marker. Reconnect rebinding (new session guid → same
    // character) is a future slice.
    uint64_t team_id = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Team* team = FindTeamByMemberLocked(session_guid);
        if (!team) return;
        for (auto& m : team->members)
            if (m.session_guid == session_guid) m.offline = true;
        team_id = team->id;
    }
    BroadcastRoster(team_id);
}

void TeamService::HandleVoluntaryExit(const std::string& session_guid) {
    CancelInvitesFor(session_guid);
    LeaveCurrentTeam(session_guid);
}

// ---------------------------------------------------------------------------
// Team queueing
// ---------------------------------------------------------------------------

std::optional<QueuedParty> TeamService::BuildParty(const std::string& leader_guid) {
    std::vector<std::string> guids;
    uint64_t team_id = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Team* t = FindTeamByMemberLocked(leader_guid);
        if (!t || t->leader_guid != leader_guid) return std::nullopt;
        team_id = t->id;
        for (const auto& m : t->members) guids.push_back(m.session_guid);
    }
    if (guids.empty()) return std::nullopt;

    QueuedParty party;
    party.party_id    = team_id;
    party.is_team     = true;
    party.leader_guid = leader_guid;
    party.joined_at   = std::chrono::steady_clock::now();
    for (const auto& g : guids) {
        QueuedPlayer qp;
        qp.session_guid = g;
        qp.joined_at    = party.joined_at;
        // Live class/account from the session store (member may have re-specced
        // since joining the team).
        auto sess = PlayerSessionStore::GetByGuid(g);
        if (sess) { qp.profile_id = sess->selected_profile_id; qp.user_id = sess->user_id; }
        party.members.push_back(std::move(qp));
    }
    return party;
}

void TeamService::NotifyTeamQueued(uint64_t team_id, uint32_t queue_id, uint32_t name_msg_id) {
    std::string leader_guid, leader_name;
    std::vector<std::string> members;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(teams_.begin(), teams_.end(),
            [&](const Team& t) { return t.id == team_id; });
        if (it == teams_.end()) return;
        leader_guid = it->leader_guid;
        for (const auto& m : it->members) {
            members.push_back(m.session_guid);
            if (m.session_guid == leader_guid) leader_name = m.player_name;
        }
    }
    for (const auto& g : members) {
        TcpSession::DeliverMatchQueueStatus(g, queue_id, name_msg_id);
        if (g == leader_guid)
            TcpSession::DeliverTeamSystemMessage(g, MSG_JOINED_QUEUE, "");
        else
            TcpSession::DeliverTeamSystemMessage(g, MSG_TEAM_ENTERED_QUEUE, leader_name);
    }
}

void TeamService::DequeueForCompositionChange(const std::string& session_guid) {
    uint64_t party_id = 0;
    std::vector<std::string> notify;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Team* t = FindTeamByMemberLocked(session_guid);
        if (t) {
            party_id = t->id;
            for (const auto& m : t->members) notify.push_back(m.session_guid);
        } else {
            party_id = MatchmakingService::SoloPartyId(session_guid);
            notify.push_back(session_guid);
        }
    }
    if (!MatchmakingService::RemoveParty(party_id)) return;  // wasn't queued
    Logger::Log("team", "[team] dequeued party %llu on composition change (%zu member(s))\n",
        (unsigned long long)party_id, notify.size());
    for (const auto& g : notify) {
        TcpSession::DeliverMatchQueueStatus(g, 0, 0);
        TcpSession::DeliverTeamSystemMessage(g, MSG_LEFT_QUEUE, "");
    }
}
