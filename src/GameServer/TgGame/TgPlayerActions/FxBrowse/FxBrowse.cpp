#include "src/GameServer/TgGame/TgPlayerActions/FxBrowse/FxBrowse.hpp"

#include <map>
#include <string>

#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgGame/TgEffectManager/SetEffectRep/TgEffectManager__SetEffectRep.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/Markers/Markers.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::FxBrowseCmd {

namespace {

struct Candidate {
    int         egId;
    int         fxId;
    float       lifetime;
    const char* name;
};

// Generated from gaa.db: effect groups whose target_fx_id has zero rows in
// asm_data_set_special_fx_sounds (silent) and non-zero particles or materials.
// Longest lifetime first — a longer-lived effect needs fewer re-pushes, and
// each re-push replays the intro animation.
const Candidate kCandidates[] = {
    { 25353, 1543, 600.0f, "P_Sword_DamageBuff" },
    {  3007,  455, 120.0f, "NanoSwarmDebuff_OrangeA" },
    {  3385,  125,  60.0f, "DOTGasGreenA00" },
    {  3236,  254,  60.0f, "NanoSwarmDOTA" },
    {  2712,  328,  60.0f, "NanoSwarmHealA" },
    {  2005,  126,  60.0f, "MeleeStunA_Orange" },
    { 25830, 1616,  50.0f, "CubeBuffA" },
    {  7282,  643,  40.0f, "Player_Contagious_boundary" },
    {  3502,  320,  30.0f, "BioBodyLoop" },
    { 27646, 1770,  30.0f, "ChloeBuff" },
    {  1185,   81,  30.0f, "ElecHold_CWhite00" },
    {  1469,  131,  30.0f, "(materials only)" },
    {  2198,  116,  20.0f, "NanoSwarmHealA (short)" },
    {  8861,  860,  20.0f, "ElecDOT" },
    {  7063,  656,  18.0f, "NanoSwarmDebuff_OrangeA (b)" },
    {  3472,  331,  15.0f, "NanoSwarmSLOWA" },
    {  3897,  456,  15.0f, "NanoSwarmSLOWGOLD" },
    {  7756,  682,  15.0f, "GraphicHOTA" },
    {  1481,   79,  12.0f, "ElecHold" },
    {  1480,  128,  12.0f, "MeleeHoldA_Orange" },
    {  5853,  741,  12.0f, "OnFireA_torso" },
    { 10021,  448,  10.0f, "GraphicBuffMixA" },
    {  4525,  450,  10.0f, "GraphicBuffShieldA" },
    {  3814,  459,  10.0f, "GraphicHOTA (b)" },
    {  3996,  482,  10.0f, "GraphicRepairA00" },
    {  8769,  680,  10.0f, "NanoSwarmSLOWGOLD (b)" },
    {  5098,  918,  10.0f, "PurpleBolts" },
    { 16625, 1295,  10.0f, "Agony_DEBUFF" },
    {  7351, 1505,  10.0f, "Poisoned" },
    { 26009, 1628,  10.0f, "GraphicBuff_Green" },
    {  4188,  434,   8.0f, "GraphicRedHitA" },
    {  5864,  552,   8.0f, "TurretStun_01" },
    { 27755, 1823,   8.0f, "Bolonov_Stun" },
    { 16932, 1289,   7.0f, "EnergySapInit" },
    {  7379,  522,   6.0f, "CyberBodyLoopA" },
    {  4836,  489,   6.0f, "Crit" },
    { 22323, 1567,   5.0f, "Commander_MoralBoost_Buff" },
    { 13004, 1233,   5.0f, "Poisoned (b)" },
    {  7724,  828,   3.0f, "GraphicBuffPurpleA" },
    {  8698,  847,   3.0f, "MixColorFractal (team-coloured, materials only)" },
    { 13708, 1165,   0.0f, "(materials only) 1165" },
    { 13709, 1166,   0.0f, "(materials only) 1166" },
};
constexpr int kCandidateCount = (int)(sizeof(kCandidates) / sizeof(kCandidates[0]));

// Re-push a little before the entry's own lifetime lapses. Entries with
// lifetime 0 are one-shot, so fall back to a steady low rate.
constexpr DWORD kZeroLifetimeStepMs = 2000;
constexpr DWORD kMinStepMs          = 500;

enum class DeliveryMode { Pawn, Own };

// The first push into a freshly-spawned manager is always lost: the client's
// initial-replication handler does `c_nLastQueueIndex = r_nNextQueueIndex - 1`,
// marking the entry we just wrote as consumed. Re-push shortly after so the
// selected entry actually appears instead of waiting for the next step.
constexpr DWORD kRepushMs   = 1200;
constexpr int   kRepushCount = 2;

struct Session {
    APlayerController* pc      = nullptr;
    int                index   = 0;
    DeliveryMode       mode    = DeliveryMode::Pawn;
    ATgEffectManager*  ownMgr  = nullptr;   // Own mode only
    ATgPawn*           lastTarget = nullptr;
    DWORD              lastPushMs = 0;
    int                repushesLeft = 0;
};

std::map<std::string, Session> g_active;

UNetConnection* ConnectionForSession(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid && !kv.second.bClosed) {
            return (UNetConnection*)(uintptr_t)(uint32_t)kv.first;
        }
    }
    return nullptr;
}

// HARD GATE — spectator-only, same reasoning as MarkersCmd::IsSpectatorSession.
// -fx paints arbitrary effects on other players' pawns; a live player must
// never be able to reach it.
bool IsSpectatorSession(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid && !kv.second.bClosed) {
            return kv.second.PlayerInfo.is_spectator;
        }
    }
    return false;
}

APlayerController* FindControllerForSession(const std::string& guid) {
    UNetConnection* conn = ConnectionForSession(guid);
    if (!conn) return nullptr;
    AActor* game = (AActor*)Globals::Get().GGameInfo;
    AWorldInfo* wi = game ? game->WorldInfo : nullptr;
    if (!wi) return nullptr;
    for (AController* c = wi->ControllerList; c; c = c->NextController) {
        if (!ObjectClassCache::ClassNameContains(c, "PlayerController")) continue;
        APlayerController* pc = (APlayerController*)c;
        if ((void*)pc->Player == (void*)conn) return pc;
    }
    return nullptr;
}

bool IsMarkablePawn(ATgPawn_Character* pawn) {
    if (!pawn || pawn->bDeleteMe) return false;
    if (pawn->Health <= 0) return false;
    // Real players only. bIsPlayer is unreliable on this build (AI bots
    // default it true), so test the controller class instead.
    return pawn->Controller &&
           ObjectClassCache::ClassNameContains(pawn->Controller, "PlayerController");
}

// Nearest live player pawn to `from`. Used when the spectator camera is in
// free-look rather than attached to a player.
ATgPawn* NearestPlayerPawn(const FVector& from) {
    ATgPawn* best = nullptr;
    float bestSq = 0.0f;
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.bClosed) continue;
        ATgPawn_Character* pawn = kv.second.Pawn;
        if (!IsMarkablePawn(pawn)) continue;
        const float dx = pawn->Location.X - from.X;
        const float dy = pawn->Location.Y - from.Y;
        const float dz = pawn->Location.Z - from.Z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (!best || d2 < bestSq) { best = (ATgPawn*)pawn; bestSq = d2; }
    }
    return best;
}

// The pawn to paint. Following a player as a spectator sets ViewTarget to that
// pawn, which is what we want — but a free-look spectator camera leaves
// ViewTarget pointing at the controller itself, which is not a pawn. Rather
// than refuse to do anything (which just looks like the command is broken),
// fall back to the nearest live player.
ATgPawn* CurrentTarget(APlayerController* pc) {
    AActor* vt = pc->ViewTarget;
    if (vt && !vt->bDeleteMe && ObjectClassCache::ClassNameContains(vt, "TgPawn")) {
        return (ATgPawn*)vt;
    }
    return NearestPlayerPawn(((AActor*)pc)->Location);
}

void Reply(const std::string& guid, const std::wstring& text) {
    UNetConnection* conn = ConnectionForSession(guid);
    if (!conn) return;
    // priority 1 Normal, type 0 Regular. SendText renders as a private chat
    // line rather than a toast, which is what we want for a readable label.
    SendAlert::SendText(conn, text.c_str(), 1, 0, 5.0f);
}

std::wstring Widen(const char* s) {
    std::wstring out;
    for (const char* p = s; p && *p; ++p) out.push_back((wchar_t)(unsigned char)*p);
    return out;
}

std::wstring DescribeEntry(const Session& s) {
    const Candidate& c = kCandidates[s.index];
    std::wstring w = L"[fx " + std::to_wstring(s.index + 1) + L"/"
                   + std::to_wstring(kCandidateCount) + L"] fx=" + std::to_wstring(c.fxId)
                   + L" eg=" + std::to_wstring(c.egId)
                   + L" life=" + std::to_wstring((int)c.lifetime) + L"s "
                   + Widen(c.name)
                   + (s.mode == DeliveryMode::Own ? L"  [own]" : L"  [pawn]");
    return w;
}

// Spawn the per-session effect manager used by DeliveryMode::Own.
ATgEffectManager* SpawnOwnManager(APlayerController* pc) {
    UClass* cls = ClassPreloader::GetClass("Class TgGame.TgEffectManager");
    if (!cls) {
        Logger::Log("markers", "[FxBrowse] TgEffectManager class not resolved\n");
        return nullptr;
    }
    FRotator rot; rot.Pitch = 0; rot.Yaw = 0; rot.Roll = 0;
    ATgEffectManager* m = (ATgEffectManager*)((AActor*)pc)->Spawn(
        cls, (AActor*)pc, FName(), ((AActor*)pc)->Location, rot, nullptr, 1);
    if (!m) {
        Logger::Log("markers", "[FxBrowse] effect manager Spawn returned null\n");
        return nullptr;
    }
    ((AActor*)m)->Owner                = (AActor*)pc;
    ((AActor*)m)->bOnlyRelevantToOwner = 1;
    ((AActor*)m)->bAlwaysRelevant      = 0;
    ((AActor*)m)->bHidden              = 1;
    ((AActor*)m)->bCollideActors       = 0;
    ((AActor*)m)->bCollideWorld        = 0;
    // Suppress Owner on the wire so the client falls through to r_Owner —
    // see MarkersCmd::IsMarkerEffectManager for the full reasoning.
    MarkersCmd::RegisterMarkerEffectManager((const void*)m);
    return m;
}

void Push(const std::string& guid, Session& s, ATgPawn* target) {
    const Candidate& c = kCandidates[s.index];

    ATgEffectManager* mgr = nullptr;
    if (s.mode == DeliveryMode::Own) {
        if (!s.ownMgr || ((AActor*)s.ownMgr)->bDeleteMe) s.ownMgr = SpawnOwnManager(s.pc);
        if (!s.ownMgr) return;
        // Point our manager at the pawn we want painted. This is the whole
        // experiment: if the client's UpdateEffectForms honours r_Owner, the
        // FX lands on that pawn and only this session ever sees it.
        s.ownMgr->r_Owner = (AActor*)target;
        ((AActor*)s.ownMgr)->bNetDirty       = 1;
        ((AActor*)s.ownMgr)->bForceNetUpdate = 1;
        mgr = s.ownMgr;
    } else {
        mgr = target->r_EffectManager;
    }
    if (!mgr) return;

    const int ret = TgEffectManager__SetEffectRep::Call(
        mgr, nullptr, c.egId, c.lifetime, /*bIsBuff=*/0, /*nHealthChange=*/0);

    Logger::Log("markers",
        "[FxBrowse] guid=%s mode=%s idx=%d eg=%d fx=%d life=%.1f target=%s -> %s(%d)\n",
        guid.c_str(), s.mode == DeliveryMode::Own ? "own" : "pawn",
        s.index, c.egId, c.fxId, c.lifetime,
        ((UObject*)target)->GetName(), (ret == -1) ? "A:queue" : "B:managed", ret);
}

// Clear the currently-selected entry from whichever manager is delivering it,
// so switching entries (or turning the browser off) takes effect immediately
// rather than waiting out the effect's lifetime.
void ClearCurrent(Session& s, ATgPawn* target) {
    const int egId = kCandidates[s.index].egId;
    ATgEffectManager* mgr = nullptr;
    if (s.mode == DeliveryMode::Own) {
        mgr = s.ownMgr;
    } else if (target) {
        mgr = target->r_EffectManager;
    }
    if (!mgr || ((AActor*)mgr)->bDeleteMe) return;
    mgr->ClearEffectRep(egId, -1);
    ((AActor*)mgr)->bNetDirty       = 1;
    ((AActor*)mgr)->bForceNetUpdate = 1;
}

DWORD StepFor(const Session& s) {
    const float life = kCandidates[s.index].lifetime;
    if (life <= 0.0f) return kZeroLifetimeStepMs;
    // Re-push at ~80% of lifetime so the look never lapses mid-inspection.
    DWORD step = (DWORD)(life * 800.0f);
    if (step < kMinStepMs) step = kMinStepMs;
    return step;
}

void DestroyOwnMgr(Session& s) {
    if (!s.ownMgr) return;
    // Clear the applied effect before tearing the manager down — destroying it
    // alone leaves the effect stuck on the pawn until its lifetime expires.
    if (!((AActor*)s.ownMgr)->bDeleteMe) {
        s.ownMgr->ClearEffectRep(kCandidates[s.index].egId, -1);
        ((AActor*)s.ownMgr)->bNetDirty       = 1;
        ((AActor*)s.ownMgr)->bForceNetUpdate = 1;
    }
    MarkersCmd::UnregisterMarkerEffectManager((const void*)s.ownMgr);
    if (!((AActor*)s.ownMgr)->bDeleteMe) ((AActor*)s.ownMgr)->Destroy();
    s.ownMgr = nullptr;
}

} // namespace

void Execute(const std::string& session_guid, Action action, int index) {
    // Spectator-only, no exceptions — see IsSpectatorSession.
    if (!IsSpectatorSession(session_guid)) {
        Logger::Log("markers",
            "[FxBrowse] guid=%s: rejected, session is not a spectator\n",
            session_guid.c_str());
        IpcClient::SendChatCommandAudit(session_guid, "-fx", "ignored", "not a spectator");
        return;
    }

    if (action == Action::Off) {
        auto it = g_active.find(session_guid);
        if (it == g_active.end()) return;
        // Clear on the pawn route too — DestroyOwnMgr only covers "own".
        if (it->second.mode == DeliveryMode::Pawn && it->second.pc) {
            ClearCurrent(it->second, CurrentTarget(it->second.pc));
        }
        DestroyOwnMgr(it->second);
        g_active.erase(it);
        Reply(session_guid, L"[fx] browser off");
        IpcClient::SendChatCommandAudit(session_guid, "-fx", "restored", "browser off");
        return;
    }

    APlayerController* pc = FindControllerForSession(session_guid);
    if (!pc) {
        Logger::Log("markers", "[FxBrowse] guid=%s: no PlayerController\n", session_guid.c_str());
        IpcClient::SendChatCommandAudit(session_guid, "-fx", "ignored", "no player controller");
        return;
    }

    Session& s = g_active[session_guid];
    s.pc = pc;

    switch (action) {
        // Clear the outgoing entry before switching, so stepping doesn't stack
        // the previous look on top of the next one.
        case Action::Next:
            ClearCurrent(s, CurrentTarget(pc));
            s.index = (s.index + 1) % kCandidateCount;
            break;
        case Action::Prev:
            ClearCurrent(s, CurrentTarget(pc));
            s.index = (s.index + kCandidateCount - 1) % kCandidateCount;
            break;
        case Action::Jump:
            if (index < 1 || index > kCandidateCount) {
                Reply(session_guid, L"[fx] index out of range 1-"
                                    + std::to_wstring(kCandidateCount));
                return;
            }
            s.index = index - 1;
            break;
        case Action::ModePawn:
            s.mode = DeliveryMode::Pawn;
            DestroyOwnMgr(s);
            break;
        case Action::ModeOwn:
            s.mode = DeliveryMode::Own;
            break;
        default: break;  // Show
    }

    ATgPawn* target = CurrentTarget(pc);
    if (!target) {
        Reply(session_guid, L"[fx] no live player pawn in this instance");
        return;
    }

    s.lastTarget = target;
    s.lastPushMs = GetTickCount();
    s.repushesLeft = kRepushCount;
    Push(session_guid, s, target);
    // Name the pawn in the reply — with the free-look fallback you can't
    // otherwise tell which player is being painted.
    Reply(session_guid, DescribeEntry(s) + L" -> " + Widen(((UObject*)target)->GetName()));
    IpcClient::SendChatCommandAudit(session_guid, "-fx", "activated",
        "idx=" + std::to_string(s.index + 1));
}

void Tick() {
    if (g_active.empty()) return;
    const DWORD now = GetTickCount();

    for (auto it = g_active.begin(); it != g_active.end(); ) {
        Session& s = it->second;
        if (!s.pc || ((AActor*)s.pc)->bDeleteMe) {
            DestroyOwnMgr(s);
            it = g_active.erase(it);
            continue;
        }

        ATgPawn* target = CurrentTarget(s.pc);
        if (target) {
            // Re-push immediately when the spectator switches who they follow,
            // so the look moves with the camera instead of waiting a step.
            const bool switched = (target != s.lastTarget);
            const DWORD interval = (s.repushesLeft > 0) ? kRepushMs : StepFor(s);
            if (switched || now - s.lastPushMs >= interval) {
                if (switched) s.repushesLeft = kRepushCount;
                else if (s.repushesLeft > 0) --s.repushesLeft;
                s.lastTarget = target;
                s.lastPushMs = now;
                Push(it->first, s, target);
            }
        }
        ++it;
    }
}

void ForgetSession(const std::string& session_guid) {
    auto it = g_active.find(session_guid);
    if (it == g_active.end()) return;
    DestroyOwnMgr(it->second);
    g_active.erase(it);
}

} // namespace TgPlayerActions::FxBrowseCmd
