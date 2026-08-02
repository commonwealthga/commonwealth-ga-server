#include "src/GameServer/TgGame/TgPlayerActions/Markers/Markers.hpp"

#include <map>
#include <set>
#include <vector>

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgEffectManager/SetEffectRep/TgEffectManager__SetEffectRep.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::MarkersCmd {

namespace {

// ── glow route ──────────────────────────────────────────────────────────────
// Re-push at ~80% of the 30s lifetime so the skin swap never lapses.
constexpr DWORD kGlowRefreshMs = 24000;

// The FIRST push into a freshly-spawned manager is always lost. Client-side,
// the initial replication fires ReplicatedEvent('r_bRelevancyNotify'), whose
// handler does `c_nLastQueueIndex = r_nNextQueueIndex - 1` — marking the entry
// we just wrote as already consumed. Only a LATER push, which bumps the index
// again and fires ReplicatedEvent('r_nNextQueueIndex'), renders.
//
// So a new manager gets a short warm-up cadence until it has landed a few
// pushes, then settles to kGlowRefreshMs. Without this the marker takes a full
// refresh interval to appear.
constexpr DWORD kWarmupRefreshMs = 1500;
constexpr int   kWarmupPushes    = 3;

// Destroying a manager does NOT remove an already-applied skin swap — the
// effect form lives on the target pawn, not on the manager. So teardown is two
// phase: push ClearEffectRep and leave the manager alive long enough for that
// to replicate, then destroy it on a later tick.
constexpr DWORD kClearLingerMs = 1000;

// How often to look for spectators who should have markers but don't. Cheap:
// a walk of GClientConnectionsData.
constexpr DWORD kAutoScanMs = 3000;

// ── foreman route ───────────────────────────────────────────────────────────
// FX 1163's own timer is 7s (TgPawn.uc:9919); complete a full round inside 6s.
constexpr DWORD kSweepWindowMs = 6000;
constexpr DWORD kMinStepMs     = 250;
constexpr DWORD kMaxStepMs     = 3000;

enum class Route { Glow, Foreman };
enum class TeamFilter { All, Attackers, Defenders, Friendly, Enemy };

struct Session {
    APlayerController* pc    = nullptr;
    Route              route = Route::Glow;
    TeamFilter         team  = TeamFilter::All;
    float              minDistanceUU = kDefaultMinDistanceUU;

    // glow route: one owned manager per marked pawn. pushes counts how many
    // SetEffectRep calls that manager has had, so we know when it is past the
    // swallowed-first-push warm-up.
    struct GlowEntry {
        ATgEffectManager* mgr    = nullptr;
        int               pushes = 0;
    };
    std::map<ATgPawn*, GlowEntry> glowMgrs;
    DWORD glowLastPushMs = 0;

    // foreman route
    ATgPawn_SupportForeman* foreman = nullptr;
    int                     cursor  = 0;
    DWORD                   lastStepMs = 0;
};

std::map<std::string, Session> g_active;

// Managers awaiting destruction after their ClearEffectRep has had time to
// replicate. Keyed by actor, valued by the tick at which it is safe to destroy.
std::map<ATgEffectManager*, DWORD> g_pendingDestroy;

// Sessions that explicitly turned markers OFF. Without this the auto-enable
// scan below would immediately turn them back on.
std::set<std::string> g_optedOut;

DWORD g_lastAutoScanMs = 0;

// Every live glow-route manager, across all sessions. Consulted by the
// replication hook — see IsMarkerEffectManager in the header for why.
std::set<const void*> g_markerMgrs;

// The GClientConnectionsData key IS the UNetConnection pointer (see
// UdpNetDriver__TickDispatch.cpp:204).
UNetConnection* ConnectionForSession(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid && !kv.second.bClosed) {
            return (UNetConnection*)(uintptr_t)(uint32_t)kv.first;
        }
    }
    return nullptr;
}

// HARD GATE. Every -markers form is spectator-only.
//
// Without this a live player could run `-markers all` and give themselves a
// see-through-the-team highlight on every enemy — a wallhack, self-served from
// chat. PlayerInfo.is_spectator is set authoritatively by the control server
// from the ga_user_roles "spectator" role at PLAYER_REGISTER; it is never
// supplied by the connecting client.
bool IsSpectatorSession(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid && !kv.second.bClosed) {
            return kv.second.PlayerInfo.is_spectator;
        }
    }
    return false;
}

AWorldInfo* World() {
    AActor* game = (AActor*)Globals::Get().GGameInfo;
    return game ? game->WorldInfo : nullptr;
}

// Spectators have no pawn, so resolve the controller from the connection by
// walking WorldInfo.ControllerList rather than from a pawn lookup.
APlayerController* FindControllerForSession(const std::string& guid) {
    UNetConnection* conn = ConnectionForSession(guid);
    if (!conn) return nullptr;
    AWorldInfo* wi = World();
    if (!wi) return nullptr;
    for (AController* c = wi->ControllerList; c; c = c->NextController) {
        if (!ObjectClassCache::ClassNameContains(c, "PlayerController")) continue;
        APlayerController* pc = (APlayerController*)c;
        // Pointer identity only — UNetConnection derives from UPlayer at
        // offset 0, so compare as void* rather than assume the downcast.
        if ((void*)pc->Player == (void*)conn) return pc;
    }
    return nullptr;
}

// 1 = attackers, 2 = defenders, 0 = unknown. Same resolution as
// ChangeTeam.cpp: compare the PRI's r_TaskForce against the global team
// rep-info pointers.
int TaskForceNumber(ATgPawn* pawn) {
    if (!pawn || !pawn->PlayerReplicationInfo) return 0;
    ATgRepInfo_Player* repinfo = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
    ATgRepInfo_TaskForce* tf = repinfo->r_TaskForce;
    if (!tf) return 0;
    if (tf == GTeamsData.Attackers) return 1;
    if (tf == GTeamsData.Defenders) return 2;
    return 0;
}

// The spectator's own cosmetic side, as seeded by `-spectate <id> <team>`
// (UObject__ProcessEvent.cpp:1119-1128). 0 = teamless.
int SpectatorTaskForce(APlayerController* pc) {
    if (!pc || !pc->PlayerReplicationInfo) return 0;
    ATgRepInfo_Player* repinfo = (ATgRepInfo_Player*)pc->PlayerReplicationInfo;
    ATgRepInfo_TaskForce* tf = repinfo->r_TaskForce;
    if (!tf) return 0;
    if (tf == GTeamsData.Attackers) return 1;
    if (tf == GTeamsData.Defenders) return 2;
    return 0;
}

// Collapse the filter to the concrete task force it selects this sweep.
// Returns 0 for "no filter" — either TeamFilter::All, or a relative filter on
// a teamless spectator, which degrades to All rather than marking nobody.
int ResolveWantedTaskForce(const Session& s);

void Audit(const std::string& guid,
           const std::string& outcome, const std::string& details) {
    IpcClient::SendChatCommandAudit(guid, "-markers", outcome, details);
}

FVector ViewLocation(APlayerController* pc) {
    if (pc->ViewTarget && !pc->ViewTarget->bDeleteMe) return pc->ViewTarget->Location;
    return ((AActor*)pc)->Location;
}

bool WithinNearCull(const FVector& view, ATgPawn* pawn, float minDistUU) {
    if (minDistUU <= 0.0f) return false;
    const float dx = pawn->Location.X - view.X;
    const float dy = pawn->Location.Y - view.Y;
    const float dz = pawn->Location.Z - view.Z;
    return (dx * dx + dy * dy + dz * dz) < (minDistUU * minDistUU);
}

// Live player pawns, unfiltered by team or distance.
void CollectCandidates(std::vector<ATgPawn*>& out) {
    for (auto& kv : GClientConnectionsData) {
        ClientConnectionData& data = kv.second;
        if (data.bClosed) continue;

        ATgPawn_Character* pawn = data.Pawn;
        if (!pawn || pawn->bDeleteMe) continue;
        if (pawn->Health <= 0) continue;  // don't highlight corpses

        // Real players only. bIsPlayer is unreliable on this build (AI bots
        // default it true), so test the controller class instead.
        if (!pawn->Controller ||
            !ObjectClassCache::ClassNameContains(pawn->Controller, "PlayerController")) {
            continue;
        }
        out.push_back((ATgPawn*)pawn);
    }
}

int ResolveWantedTaskForce(const Session& s) {
    switch (s.team) {
        case TeamFilter::Attackers: return 1;
        case TeamFilter::Defenders: return 2;
        case TeamFilter::Friendly:  return SpectatorTaskForce(s.pc);
        case TeamFilter::Enemy: {
            const int own = SpectatorTaskForce(s.pc);
            // Teamless -> 0, which the caller treats as "no filter".
            return (own == 1) ? 2 : (own == 2 ? 1 : 0);
        }
        default: return 0;  // All
    }
}

// Per-session view: team filter always; near-cull only on the foreman route,
// which is the one with the close-range scaling problem.
void FilterForSession(const Session& s, const std::vector<ATgPawn*>& candidates,
                      std::vector<ATgPawn*>& out) {
    if (!s.pc) return;
    const int wantTf = ResolveWantedTaskForce(s);
    const FVector view = ViewLocation(s.pc);
    for (ATgPawn* p : candidates) {
        if (wantTf != 0 && TaskForceNumber(p) != wantTf) continue;
        if (s.route == Route::Foreman && WithinNearCull(view, p, s.minDistanceUU)) continue;
        out.push_back(p);
    }
}

DWORD StepIntervalMs(size_t target_count) {
    if (target_count == 0) return kMaxStepMs;
    DWORD step = (DWORD)(kSweepWindowMs / target_count);
    if (step < kMinStepMs) step = kMinStepMs;
    if (step > kMaxStepMs) step = kMaxStepMs;
    return step;
}

// ── glow route ──────────────────────────────────────────────────────────────

ATgEffectManager* SpawnOwnedManager(APlayerController* pc) {
    UClass* cls = ClassPreloader::GetClass("Class TgGame.TgEffectManager");
    if (!cls) {
        Logger::Log("markers", "[Markers] TgEffectManager class not resolved\n");
        return nullptr;
    }
    FRotator rot; rot.Pitch = 0; rot.Yaw = 0; rot.Roll = 0;
    ATgEffectManager* m = (ATgEffectManager*)((AActor*)pc)->Spawn(
        cls, (AActor*)pc, FName(), ((AActor*)pc)->Location, rot, nullptr, 1);
    if (!m) {
        Logger::Log("markers", "[Markers] effect manager Spawn returned null\n");
        return nullptr;
    }
    // Owner + bOnlyRelevantToOwner is what makes this per-viewer: the actor
    // channel opens for this one connection and no other.
    //
    // The client must NOT see this Owner value — it would resolve the effect
    // target as the PlayerController instead of falling through to r_Owner.
    // Registering here makes the replication hook send Owner as null. See
    // IsMarkerEffectManager in the header.
    ((AActor*)m)->Owner                = (AActor*)pc;
    ((AActor*)m)->bOnlyRelevantToOwner = 1;
    ((AActor*)m)->bAlwaysRelevant      = 0;
    ((AActor*)m)->bHidden              = 1;
    ((AActor*)m)->bCollideActors       = 0;
    ((AActor*)m)->bCollideWorld        = 0;
    g_markerMgrs.insert((const void*)m);
    return m;
}

// Clear the applied effect, then queue the manager for destruction once the
// clear has had a chance to reach the client. Destroying outright leaves the
// skin swap stuck on the pawn until its own lifetime expires.
void ReleaseGlowMgr(ATgEffectManager* mgr) {
    if (!mgr || ((AActor*)mgr)->bDeleteMe) return;
    mgr->ClearEffectRep(kGlowEffectGroupId, -1);
    ((AActor*)mgr)->bNetDirty       = 1;
    ((AActor*)mgr)->bForceNetUpdate = 1;
    g_pendingDestroy[mgr] = GetTickCount() + kClearLingerMs;
}

void DestroyGlowMgrs(Session& s) {
    for (auto& kv : s.glowMgrs) {
        ReleaseGlowMgr(kv.second.mgr);
    }
    s.glowMgrs.clear();
}

// Second phase of teardown — run every tick.
void ReapPendingDestroys() {
    const DWORD now = GetTickCount();
    for (auto it = g_pendingDestroy.begin(); it != g_pendingDestroy.end(); ) {
        if ((int)(now - it->second) < 0) { ++it; continue; }
        ATgEffectManager* mgr = it->first;
        g_markerMgrs.erase((const void*)mgr);
        if (!((AActor*)mgr)->bDeleteMe) ((AActor*)mgr)->Destroy();
        it = g_pendingDestroy.erase(it);
    }
}

// True while any manager is still inside its warm-up, i.e. has not yet landed
// enough pushes to be sure one survived the channel-open swallow.
bool SessionIsWarmingUp(const Session& s) {
    for (const auto& kv : s.glowMgrs) {
        if (kv.second.mgr && kv.second.pushes < kWarmupPushes) return true;
    }
    return s.glowMgrs.empty();
}

// A target with no manager yet: someone joined, respawned, or moved into the
// team filter since the last sweep. Waiting out the 24s refresh before noticing
// would leave late joiners — and every respawn, which is constant in a real
// match — unmarked for most of a minute.
bool HasUnmarkedTarget(const Session& s, const std::vector<ATgPawn*>& targets) {
    for (ATgPawn* p : targets) {
        auto it = s.glowMgrs.find(p);
        if (it == s.glowMgrs.end() || !it->second.mgr) return true;
    }
    return false;
}

void GlowSweep(const std::string& guid, Session& s, const std::vector<ATgPawn*>& targets) {
    // Retire managers whose pawn is no longer a target (died, left, team
    // filter changed). The skin swap lapses on its own within kGlowLifetimeSec
    // once we stop pushing, so no explicit clear is needed.
    for (auto it = s.glowMgrs.begin(); it != s.glowMgrs.end(); ) {
        bool still = false;
        for (ATgPawn* p : targets) { if (p == it->first) { still = true; break; } }
        if (!still) {
            // Pawn died / left / fell out of the team filter — clear its glow
            // now rather than letting it linger for the effect's lifetime.
            ReleaseGlowMgr(it->second.mgr);
            it = s.glowMgrs.erase(it);
        } else {
            ++it;
        }
    }

    int pushed = 0;
    for (ATgPawn* p : targets) {
        Session::GlowEntry& e = s.glowMgrs[p];
        if (!e.mgr || ((AActor*)e.mgr)->bDeleteMe) {
            e.mgr    = SpawnOwnedManager(s.pc);
            e.pushes = 0;
        }
        ATgEffectManager* mgr = e.mgr;
        if (!mgr) continue;

        mgr->r_Owner = (AActor*)p;
        ((AActor*)mgr)->bNetDirty       = 1;
        ((AActor*)mgr)->bForceNetUpdate = 1;

        const int ret = TgEffectManager__SetEffectRep::Call(
            mgr, nullptr, kGlowEffectGroupId, kGlowLifetimeSec,
            /*bIsBuff=*/0, /*nHealthChange=*/0);
        ++e.pushes;
        ++pushed;

        if (Logger::IsChannelEnabled("markers")) {
            Logger::Log("markers", "[Markers] guid=%s glow eg=%d -> %s ret=%s(%d)\n",
                guid.c_str(), kGlowEffectGroupId, ((UObject*)p)->GetName(),
                (ret == -1) ? "A:queue" : "B:managed", ret);
        }
    }
    Logger::Log("markers", "[Markers] guid=%s glow sweep: %d pawn(s)\n",
        guid.c_str(), pushed);
}

// ── foreman route ───────────────────────────────────────────────────────────

ATgPawn_SupportForeman* SpawnForeman(APlayerController* pc) {
    UClass* cls = ClassPreloader::GetClass("Class TgGame.TgPawn_SupportForeman");
    if (!cls) {
        Logger::Log("markers", "[Markers] TgPawn_SupportForeman class not resolved\n");
        return nullptr;
    }
    FRotator rot; rot.Pitch = 0; rot.Yaw = 0; rot.Roll = 0;
    ATgPawn_SupportForeman* f = (ATgPawn_SupportForeman*)((AActor*)pc)->Spawn(
        cls, (AActor*)pc, FName(), ((AActor*)pc)->Location, rot, nullptr, 1);
    if (!f) {
        Logger::Log("markers", "[Markers] foreman Spawn returned null\n");
        return nullptr;
    }
    ((AActor*)f)->Owner                = (AActor*)pc;
    ((AActor*)f)->bOnlyRelevantToOwner = 1;
    ((AActor*)f)->bAlwaysRelevant      = 0;
    // IsNetRelevantFor short-circuits on IsOwnedBy before the bHidden reject,
    // so hiding it costs us nothing with the owner.
    ((AActor*)f)->bHidden              = 1;
    ((AActor*)f)->bCollideActors       = 0;
    ((AActor*)f)->bCollideWorld        = 0;
    ((AActor*)f)->bBlockActors         = 0;
    ((AActor*)f)->Physics              = 0;   // PHYS_None
    ((APawn*)f)->bSimulateGravity      = 0;
    return f;
}

void ForemanStep(const std::string& guid, Session& s, const std::vector<ATgPawn*>& targets) {
    if (!s.foreman || ((AActor*)s.foreman)->bDeleteMe) return;
    if (targets.empty()) return;

    if (s.cursor < 0 || (size_t)s.cursor >= targets.size()) s.cursor = 0;
    ATgPawn* next = targets[s.cursor];

    // repnotify only fires on CHANGE. With a single target the same pointer
    // would be written twice in a row and CheckBeingTargeted would never
    // re-run, so blank the slot on alternate steps.
    if (s.foreman->r_Target == next) {
        s.foreman->r_Target = nullptr;
    } else {
        s.foreman->r_Target = next;
        s.cursor = (s.cursor + 1) % (int)targets.size();
    }
    ((AActor*)s.foreman)->bNetDirty       = 1;
    ((AActor*)s.foreman)->bForceNetUpdate = 1;

    if (Logger::IsChannelEnabled("markers")) {
        Logger::Log("markers", "[Markers] guid=%s foreman mark=%s cursor=%d/%d\n",
            guid.c_str(),
            s.foreman->r_Target ? ((UObject*)s.foreman->r_Target)->GetName() : "<clear>",
            s.cursor, (int)targets.size());
    }
}

void DestroyForeman(Session& s) {
    if (s.foreman && !((AActor*)s.foreman)->bDeleteMe) ((AActor*)s.foreman)->Destroy();
    s.foreman = nullptr;
}

void TeardownRouteActors(Session& s) {
    DestroyGlowMgrs(s);
    DestroyForeman(s);
}

const char* TeamName(TeamFilter t) {
    switch (t) {
        case TeamFilter::Attackers: return "attackers";
        case TeamFilter::Defenders: return "defenders";
        case TeamFilter::Friendly:  return "friendly";
        case TeamFilter::Enemy:     return "enemy";
        default: return "all";
    }
}

} // namespace

bool IsMarkerEffectManager(const void* actor) {
    return actor && g_markerMgrs.count(actor) != 0;
}

void RegisterMarkerEffectManager(const void* actor) {
    if (actor) g_markerMgrs.insert(actor);
}

void UnregisterMarkerEffectManager(const void* actor) {
    if (actor) g_markerMgrs.erase(actor);
}

void Execute(const std::string& session_guid, Mode mode, float distance_uu) {
    // Spectator-only, no exceptions — see IsSpectatorSession.
    if (!IsSpectatorSession(session_guid)) {
        Logger::Log("markers",
            "[Markers] guid=%s: rejected, session is not a spectator\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "not a spectator");
        return;
    }

    if (mode == Mode::Off) {
        // Remember the opt-out so the spectator auto-enable scan doesn't just
        // switch it straight back on.
        g_optedOut.insert(session_guid);
        auto it = g_active.find(session_guid);
        if (it == g_active.end()) {
            Audit(session_guid, "no-op", "already off");
            return;
        }
        TeardownRouteActors(it->second);
        g_active.erase(it);
        Logger::Log("markers", "[Markers] guid=%s: disabled\n", session_guid.c_str());
        Audit(session_guid, "restored", "markers off");
        return;
    }

    auto it = g_active.find(session_guid);
    const bool was_on = (it != g_active.end());

    // Bare -markers with markers already on is the off switch.
    if (mode == Mode::Toggle && was_on) {
        g_optedOut.insert(session_guid);
        TeardownRouteActors(it->second);
        g_active.erase(it);
        Logger::Log("markers", "[Markers] guid=%s: toggled off\n", session_guid.c_str());
        Audit(session_guid, "restored", "markers off");
        return;
    }

    APlayerController* pc = FindControllerForSession(session_guid);
    if (!pc) {
        Logger::Log("markers",
            "[Markers] guid=%s: no PlayerController for this connection; dropping\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "no player controller");
        return;
    }

    // Any explicit enable clears a previous opt-out.
    g_optedOut.erase(session_guid);

    Session& s = g_active[session_guid];
    s.pc = pc;

    const Route prevRoute = s.route;
    switch (mode) {
        case Mode::All:          s.team  = TeamFilter::All;       break;
        case Mode::Attackers:    s.team  = TeamFilter::Attackers; break;
        case Mode::Defenders:    s.team  = TeamFilter::Defenders; break;
        case Mode::Friendly:     s.team  = TeamFilter::Friendly;  break;
        case Mode::Enemy:        s.team  = TeamFilter::Enemy;     break;
        case Mode::RouteGlow:    s.route = Route::Glow;           break;
        case Mode::RouteForeman: s.route = Route::Foreman;        break;
        case Mode::SetDistance:
            if (distance_uu > 0.0f) s.minDistanceUU = distance_uu;
            break;
        default: break;  // Toggle on a fresh session — keep defaults
    }

    // Switching route tears down the other route's actors so we never have a
    // foreman and a manager set fighting over the same pawns.
    if (s.route != prevRoute) TeardownRouteActors(s);

    if (s.route == Route::Foreman && !s.foreman) {
        s.foreman = SpawnForeman(pc);
        if (!s.foreman) {
            g_active.erase(session_guid);
            Audit(session_guid, "ignored", "foreman spawn failed");
            return;
        }
    }

    std::vector<ATgPawn*> candidates, targets;
    CollectCandidates(candidates);
    FilterForSession(s, candidates, targets);

    const DWORD now = GetTickCount();
    if (s.route == Route::Glow) {
        s.glowLastPushMs = now;
        GlowSweep(session_guid, s, targets);
    } else {
        s.lastStepMs = now;
        ForemanStep(session_guid, s, targets);
    }

    // A relative filter on a teamless spectator resolves to 0 (= no filter),
    // so say so rather than let it look like the filter silently did nothing.
    const int ownTf  = SpectatorTaskForce(pc);
    const bool relative = (s.team == TeamFilter::Friendly || s.team == TeamFilter::Enemy);
    const bool degraded = relative && ownTf == 0;

    Logger::Log("markers",
        "[Markers] guid=%s: on route=%s team=%s ownTf=%d wantTf=%d%s targets=%d/%d nearcull=%.0fuu\n",
        session_guid.c_str(), s.route == Route::Glow ? "glow" : "foreman",
        TeamName(s.team), ownTf, ResolveWantedTaskForce(s),
        degraded ? " (teamless spectator -> falling back to all)" : "",
        (int)targets.size(), (int)candidates.size(), s.minDistanceUU);
    Audit(session_guid, "activated",
        std::string("route=") + (s.route == Route::Glow ? "glow" : "foreman")
        + " team=" + TeamName(s.team)
        + (degraded ? " (teamless->all)" : "")
        + " targets=" + std::to_string(targets.size()));
}

// Turn markers on automatically for any spectator that doesn't have them and
// hasn't explicitly opted out. This is what makes the feature need ZERO
// commands in normal use: joining as a spectator is the whole interaction.
//
// Runs on a scan rather than off the join event because it is self-healing —
// it also covers map changes, reconnects, and a spectator whose controller
// wasn't resolvable at PostLogin time.
static void AutoEnableForSpectators() {
    const DWORD now = GetTickCount();
    if (now - g_lastAutoScanMs < kAutoScanMs) return;
    g_lastAutoScanMs = now;

    for (auto& kv : GClientConnectionsData) {
        ClientConnectionData& data = kv.second;
        if (data.bClosed) continue;
        if (!data.PlayerInfo.is_spectator) continue;
        if (data.SessionGuid.empty()) continue;
        if (g_active.count(data.SessionGuid)) continue;
        if (g_optedOut.count(data.SessionGuid)) continue;

        Logger::Log("markers", "[Markers] guid=%s: auto-enabling for spectator\n",
            data.SessionGuid.c_str());
        Execute(data.SessionGuid, Mode::Enemy, 0.0f);
    }
}

void Tick() {
    ReapPendingDestroys();
    AutoEnableForSpectators();
    if (g_active.empty()) return;

    std::vector<ATgPawn*> candidates;
    CollectCandidates(candidates);
    const DWORD now = GetTickCount();

    for (auto it = g_active.begin(); it != g_active.end(); ) {
        Session& s = it->second;

        // Controller gone (map change, disconnect racing cleanup) — drop the
        // entry rather than keep driving dangling actors.
        if (!s.pc || ((AActor*)s.pc)->bDeleteMe) {
            TeardownRouteActors(s);
            it = g_active.erase(it);
            continue;
        }

        std::vector<ATgPawn*> targets;
        FilterForSession(s, candidates, targets);

        if (s.route == Route::Glow) {
            // Fast cadence until every manager has landed past the swallowed
            // first push, then settle to the lifetime-driven refresh. A newly
            // appeared target short-circuits the wait entirely.
            const DWORD interval = SessionIsWarmingUp(s) ? kWarmupRefreshMs : kGlowRefreshMs;
            if (HasUnmarkedTarget(s, targets) || now - s.glowLastPushMs >= interval) {
                s.glowLastPushMs = now;
                GlowSweep(it->first, s, targets);
            }
        } else {
            if (!s.foreman || ((AActor*)s.foreman)->bDeleteMe) {
                Logger::Log("markers",
                    "[Markers] guid=%s: foreman gone; disabling\n", it->first.c_str());
                s.foreman = nullptr;
                TeardownRouteActors(s);
                it = g_active.erase(it);
                continue;
            }
            if (now - s.lastStepMs >= StepIntervalMs(targets.size())) {
                s.lastStepMs = now;
                ForemanStep(it->first, s, targets);
            }
        }
        ++it;
    }
}

void ForgetSession(const std::string& session_guid) {
    g_optedOut.erase(session_guid);
    auto it = g_active.find(session_guid);
    if (it == g_active.end()) return;
    TeardownRouteActors(it->second);
    g_active.erase(it);
}

} // namespace TgPlayerActions::MarkersCmd
