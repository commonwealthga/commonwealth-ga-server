#pragma once

#include <asio.hpp>

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <cstdint>

#include "src/ControlServer/MatchmakingService/Domain.hpp"
#include "src/ControlServer/MatchmakingService/MatchRule.hpp"

// ---------------------------------------------------------------------------
// PendingMatch — a spawned instance whose players haven't yet reported in.
// (Orchestration state, not part of the pure domain.)
// ---------------------------------------------------------------------------

struct PendingMatch {
    int64_t instance_id = 0;
    uint32_t queue_id = 0;
    std::string game_mode;
    std::vector<std::string> session_guids;
    std::unordered_map<std::string, int> task_force_assignments;  // guid -> task_force
    std::unordered_map<std::string, uint32_t> profile_ids;        // guid -> profile_id
    std::unordered_map<std::string, double> mmrs;                 // guid -> stamped rating
    uint32_t cap = 0;  // 0 = unlimited; hard ceiling on session_guids.size()

    // Access semantics carried from the rule. Coalesce only appends to OPEN
    // pendings; PARTY_LOCKED/SEALED never accept stranger coalescing.
    AccessMode access_mode = AccessMode::Open;
    std::vector<uint64_t> owner_party_ids;
};

// ---------------------------------------------------------------------------
// MatchmakingService -- collects parties into queues, evaluates rules.
// ---------------------------------------------------------------------------

class MatchmakingService {
public:
    using MatchPopCallback = std::function<void(uint32_t queue_id, MatchResult result)>;
    using InstanceProvider = std::function<std::vector<RunningInstance>(uint32_t queue_id)>;

    static void Init();
    static void ReloadQueues();

    // --- Party / player queue management -----------------------------------

    // Queue a whole party (solo of 1 or a team of N). The party is atomic from
    // here on: it pops, coalesces, and dequeues as a unit.
    static void AddParty(uint32_t queue_id, const QueuedParty& party);

    // Convenience: queue a single player as a solo party. Synthesises a stable
    // solo party_id from the session guid.
    static void AddPlayer(uint32_t queue_id, const QueuedPlayer& player);

    // Remove a party (by id) from whatever queue holds it. Returns true if it
    // was queued. Used by TeamService on any composition change. Does NOT touch
    // pending/ready state (a popped party is past the dequeue point).
    static bool RemoveParty(uint64_t party_id);

    // Remove the party CONTAINING this guid from its queue (parties are atomic)
    // AND scrub the guid from pending matches + ready reservations. Used by
    // generic per-session cleanup (disconnect, relogin, match cancel, leave).
    static void RemovePlayer(const std::string& session_guid);

    // Stable synthetic party id for a solo session (top bit set so it never
    // collides with TeamService team ids, which are small positive integers).
    static uint64_t SoloPartyId(const std::string& session_guid);

    // --- Callbacks / context ----------------------------------------------

    static void SetMatchPopCallback(MatchPopCallback cb);
    static void SetInstanceProvider(InstanceProvider provider);
    static void SetIoContext(asio::io_context* io);

    // --- Pending / ready reservation bookkeeping (unchanged surface) -------

    static void AddPendingMatch(int64_t instance_id, PendingMatch match);
    static std::optional<PendingMatch> ConsumePendingMatch(int64_t instance_id);
    static void TrackReadyMatchReservations(
        int64_t instance_id,
        uint32_t queue_id,
        const std::string& game_mode,
        const std::vector<std::string>& session_guids,
        const std::unordered_map<std::string, int>& task_force_assignments,
        const std::unordered_map<std::string, uint32_t>& profile_ids,
        const std::unordered_map<std::string, double>& mmrs,
        uint32_t cap);
    static std::vector<RunningInstance> GetReservedReadyInstances(uint32_t queue_id);
    static void RemoveReadyMatchReservation(
        int64_t instance_id, const std::string& session_guid, const char* reason);
    static void RemoveReadyMatchReservation(
        const std::string& session_guid, const char* reason);
    static void DropReadyMatchReservations(int64_t instance_id);

    static void SetPreAssignedTeam(int64_t instance_id, const std::string& guid, int tf);
    static std::optional<int>
        ConsumePreAssignedTeam(int64_t instance_id, const std::string& guid);
    static void DropPreAssignedTeams(int64_t instance_id);

    static void DiscardPendingMatchForDeadInstance(int64_t instance_id, const char* reason);

    static bool GetContinueInQueue(uint32_t queue_id);
    static std::vector<QueueConfig> GetEnabledQueueConfigs();
    static std::optional<QueueConfig> GetQueueConfig(uint32_t queue_id);

    // Cap of a single instance for this queue (delegates to mm::QueueInstanceCap;
    // kept as a member for the existing call sites in main.cpp / encoder).
    static uint32_t GetQueueInstanceCap(
        const QueueConfig& cfg, uint32_t instance_max_players = 0);

    struct QueuedProfileCounts { uint32_t assault, medic, recon, robotics; };
    static QueuedProfileCounts GetQueuedProfileCounts(uint32_t queue_id);

    static std::optional<uint32_t>
        GetDelayedPopRemainingSeconds(uint32_t queue_id);

    static std::optional<MapModeSpec>
        PickRandomMapPoolEntryForCount(uint32_t queue_id, int count);

private:
    struct DelayedPop {
        float next_duration = 0.0f;
        std::shared_ptr<asio::steady_timer> timer;
        std::chrono::steady_clock::time_point fires_at;
    };

    struct Queue {
        QueueConfig config;
        std::unique_ptr<MatchRule> rule;
        std::vector<QueuedParty> parties;          // the unit of queueing
        std::optional<DelayedPop> delayed_pop;
    };

    static std::unordered_map<uint32_t, Queue> queues_;
    static MatchPopCallback on_match_pop_;
    static InstanceProvider instance_provider_;
    static std::unordered_map<int64_t, PendingMatch> pending_matches_;
    static std::unordered_map<int64_t, PendingMatch> ready_match_reservations_;
    static std::unordered_map<int64_t,
                              std::unordered_map<std::string, int>>
        pre_assigned_teams_;
    static asio::io_context* io_ctx_;

    static void TryPop(uint32_t queue_id, bool delay_elapsed = false);
    static void OnQueueChanged(uint32_t queue_id, const char* trigger,
                               const std::string& who);
    static void MaybeResetDelayedPop(Queue& q, const char* trigger,
                                     const std::string& guid);
    static Queue BuildQueue(QueueConfig cfg);

    // Total players currently in a queue's parties (for min-to-pop, delay).
    static size_t QueuedPlayerCount(const Queue& q);
};
