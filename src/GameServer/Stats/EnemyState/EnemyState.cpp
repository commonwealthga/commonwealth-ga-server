#include "src/GameServer/Stats/EnemyState/EnemyState.hpp"

#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <map>
#include <set>
#include <vector>

namespace EnemyState {

namespace {

struct Carried {
    int      bot_id;
    FVector  loc;
    FRotator rot;
    int      health;
    bool     has_home;
    FVector  home;       // AI anchor (m_vSpawnLocation) bots return to
    FRotator home_rot;
    int64_t  target;     // user the bot was fighting, 0 = none
    bool     claimed;
};

// Placed bot whose carried target was not back in yet.
struct PendingTarget {
    ATgAIController* ctrl;
    int pawn_id;
    int64_t user;
};
std::vector<PendingTarget> g_targets;

struct CarriedFactory {
    std::vector<Carried> bots;
    bool spawned = false;   // factory had spawned in the snapshot
    bool respawn = false;   // bRespawn: refills on its own, never trimmed
    std::map<int, int> pending;   // bot id -> spawns started with it, not matched yet
    // Carried bots of this id a new spawn may still take.
    int Free(int botId) const {
        int n = 0;
        for (const auto& c : bots) if (!c.claimed && c.bot_id == botId) n++;
        auto it = pending.find(botId);
        return n - (it == pending.end() ? 0 : it->second);
    }
};

std::map<int, CarriedFactory> g_factories;
std::set<int> g_seen;       // r_nPawnId of bots already matched or trimmed
DWORD g_since = 0;
DWORD g_lastScanMs = 0;
int g_placed = 0, g_trimmed = 0;

// Long enough for wave/encounter spawns after the players are back in.
constexpr DWORD kCarryTtlMs = 180000;
constexpr DWORD kScanIntervalMs = 100;
// Keeps the feed frame well under the 64 KB IPC frame limit.
constexpr size_t kMaxEnemies = 300;

int Rot16(float v) { return ((int)std::lround(v)) & 0xffff; }

AWorldInfo* World() {
    if (!Globals::Get().GWorld) return nullptr;
    return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

// Live bot pawn of a bot factory, or nullptr.
ATgPawn* FactoryBot(AController* C, ATgBotFactory** outFactory) {
    if (!C || !C->Pawn || C->bDeleteMe) return nullptr;
    if (!ObjectClassCache::ClassNameContains((UObject*)C, "TgAIController")) return nullptr;
    ATgBotFactory* F = ((ATgAIController*)C)->m_pFactory;
    ATgPawn* P = (ATgPawn*)C->Pawn;
    if (!F || P->bDeleteMe || P->Health <= 0) return nullptr;
    *outFactory = F;
    return P;
}

float Dist2(const FVector& a, const FVector& b) {
    const float dx = a.X - b.X, dy = a.Y - b.Y, dz = a.Z - b.Z;
    return dx * dx + dy * dy + dz * dz;
}

// Unclaimed carried bot for this pawn: same bot id and nearest first, then
// the nearest bot of an id no started spawn is waiting for.
Carried* Pick(CarriedFactory& cf, ATgPawn* P) {
    Carried* best = nullptr;
    bool bestSameId = false;
    float bestD = 0.0f;
    for (auto& c : cf.bots) {
        if (c.claimed) continue;
        const bool sameId = c.bot_id == P->r_nProfileId;
        if (!sameId && cf.Free(c.bot_id) <= 0) continue;
        const float d = Dist2(c.loc, P->Location);
        if (!best || (sameId && !bestSameId) || (sameId == bestSameId && d < bestD)) {
            best = &c; bestSameId = sameId; bestD = d;
        }
    }
    return best;
}

void Apply(ATgPawn* P, const Carried& c) {
    FRotator rot = c.rot;
    rot.Roll = 0;
    if (!P->SetLocation(c.loc)) {
        FVector up = c.loc;
        up.Z += 48.0f;
        P->SetLocation(up);
    }
    P->SetRotation(rot);
    if (P->Controller) P->Controller->Rotation = rot;
    const int maxHp = P->r_nHealthMaximum > 0 ? P->r_nHealthMaximum : P->HealthMax;
    if (c.health > 0) P->Health = maxHp > 0 ? (std::min)(c.health, maxHp) : c.health;
    P->bNetDirty = 1;
    P->bForceNetUpdate = 1;
    // Home anchor: without it the bot walks back to where this host spawned it.
    if (ATgAIController* AI = (ATgAIController*)P->Controller) {
        AI->m_vSpawnLocation  = c.has_home ? c.home : c.loc;
        AI->m_rSpawnDirection = c.has_home ? c.home_rot : rot;
        if (c.target) g_targets.push_back({AI, P->r_nPawnId, c.target});
    }
}

// Re-acquire carried targets through the game's own sight event once the
// player's pawn is alive here.
void ApplyTargets() {
    for (size_t i = 0; i < g_targets.size();) {
        const PendingTarget& t = g_targets[i];
        ATgPawn* Bot = t.ctrl && !t.ctrl->bDeleteMe ? (ATgPawn*)t.ctrl->Pawn : nullptr;
        if (!Bot || Bot->bDeleteMe || Bot->Health <= 0 || Bot->r_nPawnId != t.pawn_id) {
            g_targets.erase(g_targets.begin() + i);
            continue;
        }
        ATgPawn* Enemy = MatchStats::LivePawnForUser(t.user);
        if (!Enemy) { i++; continue; }
        t.ctrl->eventSeePlayer(Enemy);
        Logger::Log("restore", "[Restore] enemy bot=%d target user=%lld re-acquired (enemy=%d)\n",
            Bot->r_nProfileId, (long long)t.user, t.ctrl->Enemy == Enemy ? 1 : 0);
        g_targets.erase(g_targets.begin() + i);
    }
}

}  // namespace

void Collect(nlohmann::json& out) {
    ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
    AWorldInfo* WI = World();
    if (!Game || !WI) return;
    // Map bot factories only (pet/deployable spawners are not in this list);
    // a factory that spawned and has nobody alive now is still listed so its
    // killed bots stay dead after a restore.
    std::map<int, ATgBotFactory*> factories;
    for (int i = 0; i < Game->s_ActorFactories.Count; i++) {
        ATgActorFactory* af = Game->s_ActorFactories.Data[i];
        if (!af || af->m_nMapObjectId <= 0) continue;
        if (!ObjectClassCache::ClassNameContains(af, "TgBotFactory")) continue;
        ATgBotFactory* F = (ATgBotFactory*)af;
        if (F->nTotalSpawns > 0) factories[F->m_nMapObjectId] = F;
    }
    nlohmann::json enemies = nlohmann::json::array();
    for (AController* C = WI->ControllerList; C != nullptr; C = C->NextController) {
        ATgBotFactory* F = nullptr;
        ATgPawn* P = FactoryBot(C, &F);
        if (!P || !factories.count(F->m_nMapObjectId)) continue;
        if (enemies.size() >= kMaxEnemies) break;
        const FRotator rot = P->Rotation;
        const int maxHp = P->r_nHealthMaximum > 0 ? P->r_nHealthMaximum : P->HealthMax;
        const ATgAIController* AI = (ATgAIController*)C;
        nlohmann::json e = {{"f", F->m_nMapObjectId}, {"b", P->r_nProfileId},
            {"x", std::lround(P->Location.X)}, {"y", std::lround(P->Location.Y)}, {"z", std::lround(P->Location.Z)},
            {"yaw", rot.Yaw & 0xffff}, {"hp", P->Health}, {"hp_max", maxHp},
            {"hx", std::lround(AI->m_vSpawnLocation.X)}, {"hy", std::lround(AI->m_vSpawnLocation.Y)},
            {"hz", std::lround(AI->m_vSpawnLocation.Z)}, {"hyaw", AI->m_rSpawnDirection.Yaw & 0xffff}};
        // Only player targets carry over (UserForPawn is 0 for bots).
        const int64_t target = C->Enemy ? MatchStats::UserForPawn((ATgPawn*)C->Enemy) : 0;
        if (target) e["t"] = target;
        enemies.push_back(e);
    }
    nlohmann::json flist = nlohmann::json::array();
    for (const auto& kv : factories)
        flist.push_back({{"f", kv.first}, {"spawned", kv.second->nTotalSpawns}, {"respawn", kv.second->bRespawn ? 1 : 0}});
    if (enemies.empty() && flist.empty()) return;
    out["enemies"] = enemies;
    out["factories"] = flist;
}

void SetCarried(const nlohmann::json& progress) {
    g_factories.clear();
    g_seen.clear();
    g_targets.clear();
    g_placed = g_trimmed = 0;
    const auto enemies = progress.value("enemies", nlohmann::json::array());
    const auto flist = progress.value("factories", nlohmann::json::array());
    if (!enemies.is_array() || !flist.is_array()) return;
    for (const auto& f : flist) {
        if (!f.is_object()) continue;
        CarriedFactory& cf = g_factories[f.value("f", 0)];
        cf.spawned = f.value("spawned", 0) > 0;
        cf.respawn = f.value("respawn", 0) != 0;
    }
    for (const auto& e : enemies) {
        if (!e.is_object()) continue;
        Carried c{};
        c.bot_id = e.value("b", 0);
        c.loc.X = e.value("x", 0.0f);
        c.loc.Y = e.value("y", 0.0f);
        c.loc.Z = e.value("z", 0.0f);
        c.rot.Yaw = Rot16(e.value("yaw", 0.0f));
        c.health = e.value("hp", 0);
        c.has_home = e.contains("hx");
        c.home.X = e.value("hx", 0.0f);
        c.home.Y = e.value("hy", 0.0f);
        c.home.Z = e.value("hz", 0.0f);
        c.home_rot.Yaw = Rot16(e.value("hyaw", 0.0f));
        c.target = e.value("t", (int64_t)0);
        if (c.health > 0) g_factories[e.value("f", 0)].bots.push_back(c);
    }
    g_since = GetTickCount();
    size_t n = 0;
    for (const auto& kv : g_factories) n += kv.second.bots.size();
    Logger::Log("restore", "[Restore] %zu enemy state(s) carried in, %zu factories\n", n, g_factories.size());
}

int ChooseBotId(int factoryId, int rolledId) {
    if (g_factories.empty() || GetTickCount() - g_since > kCarryTtlMs) return rolledId;
    auto it = g_factories.find(factoryId);
    if (it == g_factories.end()) return rolledId;
    CarriedFactory& cf = it->second;
    int id = rolledId;
    if (cf.Free(rolledId) <= 0) {
        id = 0;
        for (const auto& c : cf.bots)
            if (!c.claimed && cf.Free(c.bot_id) > 0) { id = c.bot_id; break; }
        if (!id) return rolledId;   // nothing left to carry: trim/fresh decides
        Logger::Log("restore", "[Restore] enemy factory=%d rolled bot=%d -> carried variant %d\n",
            factoryId, rolledId, id);
    }
    cf.pending[id]++;
    return id;
}

void Tick(ATgGame* Game) {
    if (!g_targets.empty() && Game && !Game->bGameEnded) ApplyTargets();
    if (g_factories.empty()) return;
    const DWORD now = GetTickCount();
    if (!Game || Game->bGameEnded || now - g_since > kCarryTtlMs) {
        Logger::Log("restore", "[Restore] enemies done: %d placed, %d removed\n", g_placed, g_trimmed);
        g_factories.clear();
        g_seen.clear();
        g_targets.clear();
        return;
    }
    if (now - g_lastScanMs < kScanIntervalMs) return;
    g_lastScanMs = now;
    AWorldInfo* WI = World();
    if (!WI) return;

    std::vector<ATgPawn*> trim;
    for (AController* C = WI->ControllerList; C != nullptr; C = C->NextController) {
        ATgBotFactory* F = nullptr;
        ATgPawn* P = FactoryBot(C, &F);
        if (!P || g_seen.count(P->r_nPawnId)) continue;
        auto it = g_factories.find(F->m_nMapObjectId);
        if (it == g_factories.end()) continue;   // not in the snapshot: fresh content
        g_seen.insert(P->r_nPawnId);
        auto pend = it->second.pending.find(P->r_nProfileId);
        if (pend != it->second.pending.end() && pend->second > 0) pend->second--;
        if (Carried* c = Pick(it->second, P)) {
            c->claimed = true;
            Apply(P, *c);
            g_placed++;
            Logger::Log("restore", "[Restore] enemy factory=%d bot=%d -> %.0f %.0f %.0f hp=%d\n",
                F->m_nMapObjectId, P->r_nProfileId, P->Location.X, P->Location.Y, P->Location.Z, P->Health);
        } else if (it->second.spawned && !it->second.respawn) {
            trim.push_back(P);   // killed before the hand-off: stays dead
        }
    }
    // Outside the controller walk: dying unlinks the controller.
    for (ATgPawn* P : trim) {
        Logger::Log("restore", "[Restore] enemy factory bot=%d at %.0f %.0f %.0f removed (dead in snapshot)\n",
            P->r_nProfileId, P->Location.X, P->Location.Y, P->Location.Z);
        P->eventSuicide();
        g_trimmed++;
    }
}

}  // namespace EnemyState
