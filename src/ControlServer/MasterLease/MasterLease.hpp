#pragma once

#include <cstdint>
#include <string>

// MasterLease.hpp -- one live master per database.
//
// Two masters against the same server.db is the worst failure mode available: both
// answer logins, both place instances, both hold a node link for the same node id.
// Nothing about the multi-node design prevents it, so it is prevented here: a
// single-row lease in the database that a starting master must acquire.
//
// Staleness, not liveness, is the signal -- a crashed master stops heartbeating
// and is replaced after kStaleSeconds. That also means a hard-killed master delays
// the next start by up to a minute, which is the correct trade: refusing to start
// is recoverable, two live masters is not.

class MasterLease {
public:
    // How long a heartbeat may go unrefreshed before the lease is considered
    // abandoned.
    static constexpr int kStaleSeconds = 60;

    // How often the master should call Heartbeat().
    static constexpr int kHeartbeatSeconds = 15;

    // Take the lease. False means another master holds it and is still alive --
    // the caller must refuse to start.
    static bool Acquire();

    // Refresh the lease. Logs loudly if the lease is no longer ours (another
    // master took over while we were stalled); the caller may keep running but
    // should treat the database as shared.
    static void Heartbeat();

    // Give the lease up on a clean shutdown.
    static void Release();

    // "hostname:pid" of this process, for log/messages.
    static std::string Holder();
};
