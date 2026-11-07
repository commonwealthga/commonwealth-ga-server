#include "src/GameServer/Stats/PetState/PetState.hpp"

#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Stats/InstanceRestore/InstanceRestore.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace PetState {

namespace {

struct Carried {
    int      bot_id;
    int      device_id;    // device that spawned it (must be equipped on the new host)
    int      mode;         // index into that device's fire modes
    FVector  loc;
    FRotator rot;
    int      health;       // 0 = fresh value kept
    bool     turret;
    float    deploy_time;  // turret r_fCurrentDeployTime
    bool     deployed;
    float    suicide_in;   // 'Suicide' (life span) timer left, < 0 = none
    int64_t  target;       // player target, 0 = none
};

struct PerUser {
    std::vector<Carried> items;
    DWORD since;
    DWORD alive_at = 0;   // owner first seen alive here
};

// A re-spawned pet whose health/timer/target are set once a turret has
// finished deploying (the deploy ramps health) or right away for others.
struct Fixup {
    ATgPawn* pet;
    int      obj_index;
    Carried  c;
    DWORD    spawned_at;
    DWORD    deployed_at;   // turret first seen deployed here
    int      applied = 0;
};

std::map<int64_t, PerUser> g_carried;
std::vector<Fixup> g_fixups;

constexpr DWORD kCarryTtlMs = 180000;
constexpr DWORD kDeviceWaitMs = 15000;
constexpr DWORD kFixupWaitMs = 8000;
// The turret deploy ramps health up to full as it completes: set the carried
// health a moment after it reports deployed.
constexpr DWORD kAfterDeployMs = 1000;
// ...and once more after the deploy animation (DeployComplete heals to full).
constexpr DWORD kSecondApplyMs = 6000;
// Same settle as deployables: the round start clears owned bots.
constexpr DWORD kSettleMs = 2000;
constexpr int   kMaxTotal = 64;
constexpr int   kMaxPerUser = 6;
constexpr int   kEquipSlots = 0x19;
constexpr float kMaxLifeSecs = 600.0f;

AWorldInfo* World() {
    if (!Globals::Get().GWorld) return nullptr;
    return World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
}

FName SuicideName() { static FName n("Suicide"); return n; }

bool Alive(ATgPawn* p, int idx) {
    TArray<UObject*>* objs = UObject::GObjObjects();
    return p && objs && idx >= 0 && idx < objs->Count && objs->Data[idx] == (UObject*)p && !p->bDeleteMe;
}

float Round1(float v) { return std::lround(v * 10.0f) / 10.0f; }

bool IsTurret(ATgPawn* p) { return ObjectClassCache::ClassNameContains(p, "TgPawn_Turret"); }

// Owner = a live player pawn of this instance (r_Owner is only compared, never
// dereferenced unless it is one of them).
ATgPawn* PlayerOwner(ATgPawn* pet) {
    for (const auto& ps : GPawnSessions)
        if (ps.first && ps.first == pet->r_Owner && !ps.first->bDeleteMe) return ps.first;
    return nullptr;
}

ATgDevice* EquippedDevice(ATgPawn* Pawn, int deviceId) {
    for (int slot = 0; slot < kEquipSlots; slot++) {
        ATgDevice* Dev = Pawn->m_EquippedDevices[slot];
        if (Dev && !Dev->bDeleteMe && Dev->r_nDeviceId == deviceId) return Dev;
    }
    return nullptr;
}

// Device + fire mode index on the owner that spawned this pet.
ATgDevice* SpawnerDevice(ATgPawn* Owner, int modeId, int* modeIndex) {
    if (!modeId) return nullptr;
    for (int slot = 0; slot < kEquipSlots; slot++) {
        ATgDevice* Dev = Owner->m_EquippedDevices[slot];
        if (!Dev || Dev->bDeleteMe) continue;
        for (int i = 0; i < Dev->m_FireMode.Count; i++)
            if (Dev->m_FireMode.Data[i] && Dev->m_FireMode.Data[i]->m_nId == modeId) {
                *modeIndex = i;
                return Dev;
            }
    }
    return nullptr;
}

int ModeBotId(UTgDeviceFire* Mode) {
    return (Mode && Mode->m_pFireModeSetup.Dummy) ? *(int*)((char*)Mode->m_pFireModeSetup.Dummy + 0x2C) : 0;
}

void ApplyFixup(ATgPawn* pet, const Carried& c) {
    const int maxHp = pet->r_nHealthMaximum > 0 ? pet->r_nHealthMaximum : pet->HealthMax;
    if (c.health > 0 && maxHp > 0) {
        pet->Health = (std::min)(c.health, maxHp);
        // The health property is the source the pawn recomputes Health from.
        for (int i = 0; pet->s_Properties.Data && i < pet->s_Properties.Num(); ++i) {
            UTgProperty* prop = pet->s_Properties.Data[i];
            if (prop && prop->m_nPropertyId == GA_PROPERTY::TGPID_HEALTH) prop->m_fRaw = (float)pet->Health;
        }
    }
    if (c.suicide_in > 0.0f) {
        pet->ClearTimer(SuicideName(), nullptr);
        Actor__SetTimer::SetTimer(pet, c.suicide_in, false, SuicideName(), nullptr);
    }
    int targetOk = 0;
    if (c.target && pet->Controller) {
        if (ATgPawn* Enemy = MatchStats::LivePawnForUser(c.target)) {
            pet->Controller->eventSeePlayer(Enemy);
            targetOk = pet->Controller->Enemy == Enemy ? 1 : 0;
        }
    }
    pet->bNetDirty = 1;
    pet->bForceNetUpdate = 1;
    Logger::Log("restore", "[Restore] pet bot=%d hp=%d/%d suicide=%.1f target=%lld ok=%d\n",
        c.bot_id, pet->Health, maxHp, pet->GetRemainingTimeForTimer(SuicideName(), nullptr),
        (long long)c.target, targetOk);
}

// Re-spawn one carried pet for its owner; false = device not equipped yet.
bool Spawn(ATgPawn* Pawn, int64_t user_id, const Carried& c) {
    ATgDevice* Dev = EquippedDevice(Pawn, c.device_id);
    if (!Dev) return false;
    UTgDeviceFire* Mode = (c.mode >= 0 && c.mode < Dev->m_FireMode.Count) ? Dev->m_FireMode.Data[c.mode] : nullptr;
    // Snapshots come from the hosting node: the carried mode must spawn this bot.
    if (!Mode || ModeBotId(Mode) != c.bot_id) {
        Logger::Log("restore", "[Restore] pet user=%lld bot=%d device=%d mode=%d - mode does not spawn it, dropped\n",
            (long long)user_id, c.bot_id, c.device_id, c.mode);
        return true;
    }
    ATgPawn* pet = TgDeviceFire__SpawnPet::SpawnAt(Mode, Pawn, c.loc, c.rot);
    if (!pet) {
        Logger::Log("restore", "[Restore] pet user=%lld bot=%d spawn failed - dropped\n", (long long)user_id, c.bot_id);
        return true;
    }
    const bool placed = pet->SetLocation(c.loc);
    pet->SetRotation(c.rot);
    if (c.turret && IsTurret(pet)) {
        ATgPawn_Turret* T = (ATgPawn_Turret*)pet;
        T->r_fCurrentDeployTime = c.deployed ? T->r_fTimeToDeploySecs
                                             : (std::min)(c.deploy_time, T->r_fTimeToDeploySecs);
    }
    g_fixups.push_back({pet, pet->ObjectInternalInteger, c, GetTickCount(), 0});
    Logger::Log("restore", "[Restore] pet user=%lld bot=%d device=%d mode=%d placed=%d at %.0f %.0f %.0f "
        "(carried %.0f %.0f %.0f) turret=%d\n",
        (long long)user_id, c.bot_id, c.device_id, c.mode, (int)placed, pet->Location.X, pet->Location.Y,
        pet->Location.Z, c.loc.X, c.loc.Y, c.loc.Z, (int)c.turret);
    return true;
}

}  // namespace

void Collect(nlohmann::json& out) {
    AWorldInfo* WI = World();
    if (!WI) return;
    nlohmann::json arr = nlohmann::json::array();
    int owned = 0;
    for (AController* C = WI->ControllerList; C != nullptr && (int)arr.size() < kMaxTotal; C = C->NextController) {
        ATgPawn* P = (ATgPawn*)C->Pawn;
        if (!P || P->bDeleteMe || P->Health <= 0 || !P->r_Owner) continue;
        if (!ObjectClassCache::ClassNameContains(P, "TgPawn")) continue;
        if (ObjectClassCache::ClassNameContains(P, "VanityPet")) continue;
        ATgPawn* Owner = PlayerOwner(P);
        const int64_t user = Owner ? MatchStats::UserForPawn(Owner) : 0;
        if (!user) continue;
        owned++;
        int mode = -1;
        ATgDevice* Dev = SpawnerDevice(Owner, P->s_nSpawnerDeviceModeId, &mode);
        if (!Dev) continue;
        const float suicideIn = P->GetRemainingTimeForTimer(SuicideName(), nullptr);
        nlohmann::json e = {{"u", user}, {"b", P->r_nProfileId}, {"dev", Dev->r_nDeviceId}, {"m", mode},
            {"x", std::lround(P->Location.X)}, {"y", std::lround(P->Location.Y)}, {"z", std::lround(P->Location.Z)},
            {"yaw", P->Rotation.Yaw & 0xffff}, {"hp", P->Health},
            {"su", suicideIn > 0.0f ? Round1(suicideIn) : -1.0f}};
        if (IsTurret(P)) {
            ATgPawn_Turret* T = (ATgPawn_Turret*)P;
            e["tur"] = 1;
            e["dt"] = Round1(T->r_fCurrentDeployTime);
            e["dep"] = T->r_bIsDeployed ? 1 : 0;
        }
        const int64_t target = C->Enemy ? MatchStats::UserForPawn((ATgPawn*)C->Enemy) : 0;
        if (target) e["t"] = target;
        arr.push_back(e);
    }
    static int s_last = -1, s_lastOwned = -1;
    if ((int)arr.size() != s_last || owned != s_lastOwned) {
        Logger::Log("restore", "[Restore] feed: %zu pet(s) listed, %d player-owned live\n", arr.size(), owned);
        s_last = (int)arr.size(); s_lastOwned = owned;
    }
    if (!arr.empty()) out["pets"] = arr;
}

void SetCarried(const nlohmann::json& progress) {
    g_carried.clear();
    g_fixups.clear();
    const auto arr = progress.value("pets", nlohmann::json::array());
    if (!arr.is_array()) return;
    const DWORD now = GetTickCount();
    int total = 0;
    for (const auto& e : arr) {
        if (total >= kMaxTotal) break;
        if (!e.is_object()) continue;
        const int64_t user = e.value("u", (int64_t)0);
        Carried c{};
        c.bot_id      = e.value("b", 0);
        c.device_id   = e.value("dev", 0);
        c.mode        = e.value("m", -1);
        if (!user || c.bot_id <= 0 || c.device_id <= 0 || c.mode < 0) continue;
        PerUser& pu = g_carried[user];
        if ((int)pu.items.size() >= kMaxPerUser) continue;
        c.loc.X       = e.value("x", 0.0f);
        c.loc.Y       = e.value("y", 0.0f);
        c.loc.Z       = e.value("z", 0.0f);
        c.rot.Yaw     = e.value("yaw", 0) & 0xffff;
        c.health      = e.value("hp", 0);
        c.turret      = e.value("tur", 0) != 0;
        c.deploy_time = (std::max)(0.0f, e.value("dt", 0.0f));
        c.deployed    = e.value("dep", 0) != 0;
        c.suicide_in  = (std::min)(e.value("su", -1.0f), kMaxLifeSecs);
        c.target      = e.value("t", (int64_t)0);
        pu.items.push_back(c);
        pu.since = now;
        total++;
    }
    for (auto it = g_carried.begin(); it != g_carried.end();)
        it = it->second.items.empty() ? g_carried.erase(it) : std::next(it);
    Logger::Log("restore", "[Restore] %d pet(s) of %zu player(s) carried in\n", total, g_carried.size());
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
            Logger::Log("restore", "[Restore] %zu pet(s) of user=%lld dropped (owner/device not here; equipped:%s)\n",
                pu.items.size(), (long long)it->first, have.c_str());
            it = g_carried.erase(it);
        } else {
            ++it;
        }
    }

    for (auto f = g_fixups.begin(); f != g_fixups.end();) {
        if (!Alive(f->pet, f->obj_index)) { f = g_fixups.erase(f); continue; }
        if (!f->applied && f->c.turret && IsTurret(f->pet) && now - f->spawned_at < kFixupWaitMs) {
            if (!((ATgPawn_Turret*)f->pet)->r_bIsDeployed) { ++f; continue; }
            if (!f->deployed_at) f->deployed_at = now;
            if (now - f->deployed_at < kAfterDeployMs) { ++f; continue; }
        }
        if (f->applied == 0) {
            ApplyFixup(f->pet, f->c);
            f->applied = 1;
            if (!f->deployed_at) f->deployed_at = now;
            if (!(f->c.turret && f->c.health > 0)) { f = g_fixups.erase(f); continue; }
        }
        if (now - f->deployed_at < kSecondApplyMs) { ++f; continue; }
        const int maxHp = f->pet->r_nHealthMaximum > 0 ? f->pet->r_nHealthMaximum : f->pet->HealthMax;
        Logger::Log("restore", "[Restore] pet bot=%d hp before 2nd set=%d\n", f->c.bot_id, f->pet->Health);
        if (f->pet->Health > f->c.health && maxHp > 0) {
            f->c.suicide_in = -1.0f;   // timer already set
            f->c.target = 0;
            ApplyFixup(f->pet, f->c);
        }
        f = g_fixups.erase(f);
    }
}

}  // namespace PetState
