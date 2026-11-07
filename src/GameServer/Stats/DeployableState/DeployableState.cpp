#include "src/GameServer/Stats/DeployableState/DeployableState.hpp"

#include "src/Database/Database.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Stats/InstanceRestore/InstanceRestore.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace DeployableState {

namespace {

struct Carried {
    int      deployable_id;
    int      device_id;    // device that placed it (must be equipped on the new host)
    int      mode;         // index into that device's fire modes
    FVector  loc;
    FRotator rot;
    int      health;       // 0 = not tracked (fresh value kept)
    float    deploy_time;  // r_fCurrentDeployTime
    bool     deployed;
    float    destroy_in;   // 'DestroyIt' timer left, < 0 = none
    float    fire_in;      // 'StartFire' timer left (timer bombs), < 0 = none
    float    life_span;    // Actor.LifeSpan left, 0 = none
};

struct PerUser {
    std::vector<Carried> items;
    DWORD since;
    DWORD alive_at = 0;   // owner first seen alive here
};

// A re-spawned deployable whose timers are set once it has finished
// deploying (Active.BeginState starts its own fresh timers).
struct Fixup {
    ATgDeployable* dep;
    int            obj_index;
    Carried        c;
    DWORD          spawned_at;
};

std::map<int64_t, PerUser> g_carried;
std::vector<Fixup> g_fixups;

// Owner never shows up / device never equipped -> dropped.
constexpr DWORD kCarryTtlMs = 180000;
constexpr DWORD kDeviceWaitMs = 15000;
// Timers are fixed when the deployable reports deployed, or after this.
constexpr DWORD kFixupWaitMs = 4000;
// Round start clears every deployable: re-spawn only once the carried match
// state is applied (timer running) and a moment has passed.
constexpr DWORD kSettleMs = 2000;
constexpr int   kMaxTotal = 64;
constexpr int   kMaxPerUser = 8;
constexpr int   kEquipSlots = 0x19;
constexpr float kMinLeftSecs = 0.5f;
constexpr float kMaxLifeSecs = 600.0f;
constexpr float kMaxFireSecs = 60.0f;

AWorldInfo* World() {
    if (!Globals::Get().GWorld) return nullptr;
    return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

FName DestroyItName() { static FName n("DestroyIt"); return n; }
FName StartFireName() { static FName n("StartFire"); return n; }

bool Alive(ATgDeployable* d, int idx) {
    TArray<UObject*>* objs = UObject::GObjObjects();
    return d && objs && idx >= 0 && idx < objs->Count && objs->Data[idx] == (UObject*)d && !d->bDeleteMe;
}

float Round1(float v) { return std::lround(v * 10.0f) / 10.0f; }

// Owning user: the placing pawn if it is still the user's pawn, else the
// pawn that now carries the same PRI (owner died and respawned).
int64_t OwnerUser(ATgDeployable* d) {
    ATgPawn* inst = (d->Instigator && !d->Instigator->bDeleteMe) ? (ATgPawn*)d->Instigator : nullptr;
    int64_t user = inst ? MatchStats::UserForPawn(inst) : 0;
    if (user || !d->r_DRI || !d->r_DRI->r_InstigatorInfo) return user;
    for (const auto& ps : GPawnSessions) {
        ATgPawn* P = ps.first;
        if (P && !P->bDeleteMe && P->PlayerReplicationInfo == (APlayerReplicationInfo*)d->r_DRI->r_InstigatorInfo)
            return MatchStats::UserForPawn(P);
    }
    return 0;
}

int ModeIndex(ATgDeployable* d) {
    ATgDevice* Dev = d->r_Owner;
    if (Dev && d->s_SpawnerDeviceMode)
        for (int i = 0; i < Dev->m_FireMode.Count; i++)
            if (Dev->m_FireMode.Data[i] == d->s_SpawnerDeviceMode) return i;
    return d->r_nOwnerFireMode;
}

ATgDevice* EquippedDevice(ATgPawn* Pawn, int deviceId) {
    for (int slot = 0; slot < kEquipSlots; slot++) {
        ATgDevice* Dev = Pawn->m_EquippedDevices[slot];
        if (Dev && !Dev->bDeleteMe && Dev->r_nDeviceId == deviceId) return Dev;
    }
    return nullptr;
}

void ApplyTimers(ATgDeployable* d, const Carried& c) {
    if (c.destroy_in > 0.0f) {
        d->ClearTimer(DestroyItName(), nullptr);
        Actor__SetTimer::SetTimer(d, c.destroy_in, false, DestroyItName(), nullptr);
    }
    if (c.fire_in > 0.0f) {
        d->ClearTimer(StartFireName(), nullptr);
        Actor__SetTimer::SetTimer(d, c.fire_in, false, StartFireName(), nullptr);
    }
    if (c.life_span > 0.0f) d->LifeSpan = c.life_span;
    Logger::Log("restore", "[Restore] deployable id=%d timers destroy=%.1f fire=%.1f life=%.1f deployed=%d\n",
        c.deployable_id, d->GetRemainingTimeForTimer(DestroyItName(), nullptr),
        d->GetRemainingTimeForTimer(StartFireName(), nullptr), d->LifeSpan, (int)d->m_bIsDeployed);
}

// True when some device mode places this deployable directly (stations,
// force fields, sensors); mines/bombs come from thrown projectiles instead.
bool HasDirectDeployMode(int deployableId) {
    static std::unordered_map<int, bool> s_cache;
    auto it = s_cache.find(deployableId);
    if (it != s_cache.end()) return it->second;
    bool found = false;
    if (sqlite3* db = Database::GetConnection()) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT 1 FROM asm_data_set_devices_data_set_device_modes WHERE deployable_id = ? LIMIT 1;",
                -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, deployableId);
            found = sqlite3_step(stmt) == SQLITE_ROW;
            sqlite3_finalize(stmt);
        }
    }
    s_cache[deployableId] = found;
    return found;
}

// Snapshots come from the hosting node: the carried fire mode must be one
// that places this deployable (direct deploy: the mode's own deployable id;
// thrown: a mode that is not a direct deploy of something else).
bool ModePlaces(UTgDeviceFire* Mode, int deployableId) {
    if (!Mode || !Mode->m_pFireModeSetup.Dummy) return false;
    const int modeDeployable = *(int*)((char*)Mode->m_pFireModeSetup.Dummy + 0x28);
    if (modeDeployable == deployableId) return true;
    return modeDeployable == 0 && !HasDirectDeployMode(deployableId);
}

// Re-spawn one carried deployable for its owner; false = device not equipped yet.
bool Spawn(ATgPawn* Pawn, int64_t user_id, const Carried& c) {
    ATgDevice* Dev = EquippedDevice(Pawn, c.device_id);
    if (!Dev) return false;
    UTgDeviceFire* Mode = (c.mode >= 0 && c.mode < Dev->m_FireMode.Count) ? Dev->m_FireMode.Data[c.mode] : nullptr;
    if (!ModePlaces(Mode, c.deployable_id)) {
        Logger::Log("restore", "[Restore] deployable user=%lld id=%d device=%d mode=%d - mode does not place it, dropped\n",
            (long long)user_id, c.deployable_id, c.device_id, c.mode);
        return true;
    }
    FVector up{0.0f, 0.0f, 1.0f};
    ATgDeployable* d = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(
        Pawn, c.deployable_id, c.loc, up, Dev, Mode);
    if (!d) {
        Logger::Log("restore", "[Restore] deployable user=%lld id=%d spawn failed - dropped\n",
            (long long)user_id, c.deployable_id);
        return true;
    }
    // Exact spot (the deploy path lifts off the ground point it is given).
    const bool placed = d->SetLocation(c.loc);
    d->SetRotation(c.rot);
    d->r_nOwnerFireMode = c.mode;
    if (c.health > 0 && d->r_nHealth > 0) d->r_nHealth = (std::min)(c.health, d->r_nHealth);
    // Deploy progress: the Deploy state adds DeltaTime * rate each tick and
    // completes once it reaches r_fTimeToDeploySecs.
    if (d->r_fTimeToDeploySecs > 0.0f)
        d->r_fCurrentDeployTime = c.deployed ? d->r_fTimeToDeploySecs
                                             : (std::min)(c.deploy_time, d->r_fTimeToDeploySecs);
    d->bNetDirty = 1;
    d->bForceNetUpdate = 1;
    g_fixups.push_back({d, d->ObjectInternalInteger, c, GetTickCount()});
    Logger::Log("restore", "[Restore] deployable user=%lld id=%d device=%d mode=%d placed=%d at %.0f %.0f %.0f "
        "(carried %.0f %.0f %.0f) hp=%d deploy=%.1f/%.1f\n",
        (long long)user_id, c.deployable_id, c.device_id, c.mode, (int)placed, d->Location.X, d->Location.Y,
        d->Location.Z, c.loc.X, c.loc.Y, c.loc.Z, d->r_nHealth, d->r_fCurrentDeployTime, d->r_fTimeToDeploySecs);
    return true;
}

}  // namespace

void Collect(nlohmann::json& out) {
    AWorldInfo* WI = World();
    ATgRepInfo_Game* GRI = WI ? (ATgRepInfo_Game*)WI->GRI : nullptr;
    if (!GRI || !GRI->m_Deployables.Data) return;
    nlohmann::json arr = nlohmann::json::array();
    int live = 0, noOwner = 0;
    for (int i = 0; i < GRI->m_Deployables.Count && (int)arr.size() < kMaxTotal; i++) {
        ATgDeployable* d = GRI->m_Deployables.Data[i];
        if (!d || d->bDeleteMe || d->m_bInDestroyedState || d->s_DeployFactory) continue;
        live++;
        if (d->LifeSpan > 0.0f && d->LifeSpan < kMinLeftSecs) continue;   // going away now
        if (TgProj_Deployable__SpawnDeployable::IsBeaconDeployable(d)) continue;
        const int64_t user = d->r_Owner ? OwnerUser(d) : 0;
        if (!user) { noOwner++; continue; }
        const float destroyIn = d->GetRemainingTimeForTimer(DestroyItName(), nullptr);
        const float fireIn = d->GetRemainingTimeForTimer(StartFireName(), nullptr);
        arr.push_back({{"u", user}, {"d", d->r_nDeployableId}, {"dev", d->r_Owner->r_nDeviceId},
                       {"m", ModeIndex(d)}, {"x", std::lround(d->Location.X)}, {"y", std::lround(d->Location.Y)},
                       {"z", std::lround(d->Location.Z)}, {"yaw", d->Rotation.Yaw & 0xffff},
                       {"pitch", d->Rotation.Pitch & 0xffff}, {"roll", d->Rotation.Roll & 0xffff},
                       {"hp", d->r_nHealth}, {"dt", Round1(d->r_fCurrentDeployTime)},
                       {"dep", d->m_bIsDeployed ? 1 : 0},
                       {"di", destroyIn > 0.0f ? Round1(destroyIn) : -1.0f},
                       {"fi", fireIn > 0.0f ? Round1(fireIn) : -1.0f},
                       {"ls", d->LifeSpan > 0.0f ? Round1(d->LifeSpan) : 0.0f}});
    }
    // Log only when the picture changes (the feed runs every second).
    static int s_last = -1, s_lastLive = -1, s_lastNoOwner = -1;
    if ((int)arr.size() != s_last || live != s_lastLive || noOwner != s_lastNoOwner) {
        Logger::Log("restore", "[Restore] feed: %zu deployable(s) listed, %d player-placed live, %d without owner\n",
            arr.size(), live, noOwner);
        s_last = (int)arr.size(); s_lastLive = live; s_lastNoOwner = noOwner;
    }
    if (!arr.empty()) out["deployables"] = arr;
}

void SetCarried(const nlohmann::json& progress) {
    g_carried.clear();
    g_fixups.clear();
    const auto arr = progress.value("deployables", nlohmann::json::array());
    if (!arr.is_array()) return;
    const DWORD now = GetTickCount();
    int total = 0;
    for (const auto& e : arr) {
        if (total >= kMaxTotal) break;
        if (!e.is_object()) continue;
        const int64_t user = e.value("u", (int64_t)0);
        const int id = e.value("d", 0);
        // Snapshots come from the hosting node: only real deployable ids, a
        // few per player, timers no longer than a real deployable lives.
        if (!user || !DeployableClassify::IsKnownDeployableId(id)) continue;
        PerUser& pu = g_carried[user];
        if ((int)pu.items.size() >= kMaxPerUser) continue;
        Carried c{};
        c.deployable_id = id;
        c.device_id   = e.value("dev", 0);
        c.mode        = e.value("m", 0);
        c.loc.X       = e.value("x", 0.0f);
        c.loc.Y       = e.value("y", 0.0f);
        c.loc.Z       = e.value("z", 0.0f);
        c.rot.Yaw     = e.value("yaw", 0) & 0xffff;
        c.rot.Pitch   = e.value("pitch", 0) & 0xffff;
        c.rot.Roll    = e.value("roll", 0) & 0xffff;
        c.health      = e.value("hp", 0);
        c.deploy_time = (std::max)(0.0f, e.value("dt", 0.0f));
        c.deployed    = e.value("dep", 0) != 0;
        c.destroy_in  = (std::min)(e.value("di", -1.0f), kMaxLifeSecs);
        c.fire_in     = (std::min)(e.value("fi", -1.0f), kMaxFireSecs);
        c.life_span   = (std::min)(e.value("ls", 0.0f), kMaxLifeSecs);
        if (c.device_id <= 0) continue;
        pu.items.push_back(c);
        pu.since = now;
        total++;
    }
    for (auto it = g_carried.begin(); it != g_carried.end();)
        it = it->second.items.empty() ? g_carried.erase(it) : std::next(it);
    Logger::Log("restore", "[Restore] %d deployable(s) of %zu player(s) carried in\n", total, g_carried.size());
}

void Tick(ATgGame* Game) {
    if (g_carried.empty() && g_fixups.empty()) return;
    if (!Game || Game->bGameEnded) { g_carried.clear(); g_fixups.clear(); return; }
    const DWORD now = GetTickCount();

    const DWORD applied = InstanceRestore::AppliedAt();
    const bool ready = applied && now - applied >= kSettleMs;
    for (auto it = g_carried.begin(); it != g_carried.end();) {
        ATgPawn* Pawn = ready ? MatchStats::LivePawnForUser(it->first) : nullptr;
        PerUser& pu = it->second;
        if (Pawn && Pawn->Health > 0 && Pawn->Controller && !Pawn->bDeleteMe) {
            if (!pu.alive_at) pu.alive_at = now;
            for (auto c = pu.items.begin(); c != pu.items.end();)
                c = Spawn(Pawn, it->first, *c) ? pu.items.erase(c) : std::next(c);
        }
        if (pu.items.empty()) {
            it = g_carried.erase(it);
        } else if ((pu.alive_at && now - pu.alive_at > kDeviceWaitMs) || now - pu.since > kCarryTtlMs) {
            std::string have;
            if (Pawn)
                for (int slot = 0; slot < kEquipSlots; slot++)
                    if (ATgDevice* Dev = Pawn->m_EquippedDevices[slot])
                        have += " " + std::to_string(Dev->r_nDeviceId);
            Logger::Log("restore", "[Restore] %zu deployable(s) of user=%lld dropped (owner/device not here; equipped:%s)\n",
                pu.items.size(), (long long)it->first, have.c_str());
            it = g_carried.erase(it);
        } else {
            ++it;
        }
    }

    for (auto f = g_fixups.begin(); f != g_fixups.end();) {
        if (!Alive(f->dep, f->obj_index)) { f = g_fixups.erase(f); continue; }
        if (!f->dep->m_bIsDeployed && now - f->spawned_at < kFixupWaitMs) { ++f; continue; }
        ApplyTimers(f->dep, f->c);
        f = g_fixups.erase(f);
    }
}

}  // namespace DeployableState
