#include "src/GameServer/Stats/PlayerState/PlayerState.hpp"

#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <map>
#include <vector>

namespace PlayerState {

namespace {

// One running device cooldown: equip slot, device id (must match on the new
// host), fire mode and seconds left.
struct Cooldown {
    int   slot;
    int   device_id;
    int   mode;
    float left;
};

// One running timed effect group (buff/debuff/heal-over-time): group id and
// type (BuildEffectGroup keys), seconds left, and the user who applied it
// (0 = the player itself or a non-player source).
struct Effect {
    int     eg_id;
    int     eg_type;
    float   left;
    int64_t src_user;
};

struct Carried {
    FVector  loc;
    FRotator rot;
    int      health;
    float    power;
    DWORD    since;
    bool     placed = false;   // pose/health applied; only cooldowns left
    DWORD    placed_at = 0;
    std::vector<Cooldown> cooldowns;
    std::vector<Effect>   effects;
    int      in_hand = -1;     // equip slot held in hand (-1 = nothing to set)
    int      in_hand_dev = 0;  // device id that must be in that slot
};

std::map<int64_t, Carried> g_carried;

// A carried state nobody claims (player did not come along) is dropped.
constexpr DWORD kCarryTtlMs = 180000;
constexpr float kSpawnProtectSecs = 1.5f;
// Devices are equipped a moment after the pawn spawns; cooldowns whose device
// never shows up in this window are dropped.
constexpr DWORD kCooldownWaitMs = 15000;
constexpr int   kEquipSlots = 0x19;
constexpr float kMinCooldownSecs = 0.2f;
constexpr float kMaxCooldownSecs = 300.0f;

// TgDevice.StartCooldown (UC): timer index = fire mode, manager =
// GetCooldownTimerManager(), fCooldownTimeOverride > 0 replaces the mode's
// cooldown time and is only taken when that mode's timer is not running.
constexpr int kMaxTimers = 0x20;

nlohmann::json DescribeCooldowns(ATgPawn* Pawn) {
    nlohmann::json out = nlohmann::json::array();
    for (int slot = 0; slot < kEquipSlots; slot++) {
        ATgDevice* Dev = Pawn->m_EquippedDevices[slot];
        if (!Dev || Dev->bDeleteMe) continue;
        ATgTimerManager* T = Dev->GetCooldownTimerManager();
        if (!T) continue;
        for (int mode = 0; mode < Dev->m_FireMode.Count && mode < kMaxTimers; mode++) {
            if (!T->IsSet((unsigned char)mode) || T->IsPaused((unsigned char)mode)) continue;
            const float left = T->GetTimeRemaining((unsigned char)mode);
            if (left < kMinCooldownSecs) continue;
            out.push_back({slot, Dev->r_nDeviceId, mode, std::lround(left * 10.0f) / 10.0});
        }
    }
    return out;
}

// Re-start each carried cooldown whose device is equipped now; returns true
// when none are left waiting.
bool ApplyCooldowns(ATgPawn* Pawn, int64_t user_id, Carried& c) {
    for (auto it = c.cooldowns.begin(); it != c.cooldowns.end();) {
        ATgDevice* Dev = (it->slot >= 0 && it->slot < kEquipSlots) ? Pawn->m_EquippedDevices[it->slot] : nullptr;
        if (!Dev || Dev->bDeleteMe || Dev->r_nDeviceId != it->device_id) { ++it; continue; }
        ATgTimerManager* T = Dev->GetCooldownTimerManager();
        if (!T || it->mode < 0 || it->mode >= kMaxTimers) { it = c.cooldowns.erase(it); continue; }
        // A cooldown the new host already runs (fired before we got here) keeps its own time.
        if (!T->IsSet((unsigned char)it->mode)) Dev->eventStartCooldown(it->mode, it->left);
        const float now_left = T->IsSet((unsigned char)it->mode) ? T->GetTimeRemaining((unsigned char)it->mode) : 0.0f;
        Logger::Log("restore", "[Restore] cooldown user=%lld slot=%d device=%d mode=%d carried=%.1f now=%.1f\n",
            (long long)user_id, it->slot, it->device_id, it->mode, it->left, now_left);
        it = c.cooldowns.erase(it);
    }
    return c.cooldowns.empty();
}

// Loadout as the clients see it: every equipped device (slot, device id,
// quality, inventory id), the slot + fire mode held in hand, and the visible
// assembly (suit/head/hair/helmet, flairs, jetpack trail, 5 dyes). The new
// host rebuilds the loadout from the same DB rows; this is the evidence that
// it matches, and the in-hand slot is the one value carried.
nlohmann::json DescribeEquipment(ATgPawn* Pawn) {
    nlohmann::json dev = nlohmann::json::array();
    for (int slot = 0; slot < kEquipSlots; slot++) {
        ATgDevice* Dev = Pawn->m_EquippedDevices[slot];
        if (!Dev || Dev->bDeleteMe) continue;
        dev.push_back({slot, Dev->r_nDeviceId, Dev->r_nQualityValueId, Dev->r_nInventoryId});
    }
    nlohmann::json out = {{"ih", (int)Pawn->r_eDesiredInHand}, {"im", Pawn->r_eEquippedInHandMode}, {"d", dev}};
    if (Pawn->IsA(ATgPawn_Character::StaticClass())) {
        const FCustomCharacterAssembly& a = ((ATgPawn_Character*)Pawn)->r_CustomCharacterAssembly;
        nlohmann::json look = {a.SuitMeshId, a.HeadMeshId, a.HairMeshId, a.HelmetMeshId, a.HeadFlairId,
                               a.SuitFlairId, a.JetpackTrailId};
        for (int i = 0; i < 5; i++) look.push_back(a.DyeList[i]);
        out["look"] = look;
    }
    return out;
}

// Put the carried hand slot back in hand once that device is equipped here;
// returns true when nothing is left to do.
bool ApplyInHand(ATgPawn* Pawn, int64_t user_id, Carried& c) {
    if (c.in_hand < 0) return true;
    ATgDevice* Dev = Pawn->m_EquippedDevices[c.in_hand];
    if (!Dev || Dev->bDeleteMe || Dev->r_nDeviceId != c.in_hand_dev) return false;
    const int before = Pawn->r_eDesiredInHand;
    if (before != c.in_hand) {
        Pawn->r_eDesiredInHand = (unsigned char)c.in_hand;
        Pawn->eventUpdateInHand(0);
        Pawn->bNetDirty = 1;
        Pawn->bForceNetUpdate = 1;
    }
    Logger::Log("restore", "[Restore] in-hand user=%lld slot %d -> %d device=%d mode=%d\n",
        (long long)user_id, before, (int)Pawn->r_eDesiredInHand, c.in_hand_dev, Pawn->r_eEquippedInHandMode);
    c.in_hand = -1;
    return true;
}

// Timed effect groups live in TgEffectManager.s_AppliedEffectGroups; their
// lifetime is a 'LifeDone' timer on the manager with the group as the timer
// object (TgEffectGroup.StartInterval). Permanent groups (lifetime 0), skill
// and item groups are re-applied by the new host's own loadout/skill path and
// are not carried; nor is spawn protection (re-applied by Apply) or anything
// in the Invulnerable category.
constexpr int   kSpawnProtectGroupId = 8698;
constexpr int   kCategoryInvulnerable = 862;
constexpr int   kMaxEffects = 16;
constexpr float kMinEffectSecs = 0.2f;
constexpr float kMaxEffectSecs = 300.0f;

FName LifeDoneName() {
    static FName n("LifeDone");
    return n;
}

bool Carryable(UTgEffectGroup* g) {
    return g && g->m_fLifeTime > 0.0f && !g->m_bSkillEffect && !g->m_bItemEffect && !g->m_bDeviceSpecificFlag &&
           g->m_nEffectGroupId != kSpawnProtectGroupId && g->m_nCategoryCode != kCategoryInvulnerable;
}

nlohmann::json DescribeEffects(ATgPawn* Pawn) {
    nlohmann::json out = nlohmann::json::array();
    ATgEffectManager* mgr = Pawn->r_EffectManager;
    if (!mgr || !mgr->s_AppliedEffectGroups.Data) return out;
    for (int i = 0; i < mgr->s_AppliedEffectGroups.Count && (int)out.size() < kMaxEffects; i++) {
        UTgEffectGroup* g = mgr->s_AppliedEffectGroups.Data[i];
        if (!Carryable(g)) continue;
        const float left = mgr->GetRemainingTimeForTimer(LifeDoneName(), g);
        if (left < kMinEffectSecs) continue;
        ATgPawn* src = (g->m_Instigator && g->m_Instigator != Pawn && g->m_Instigator->IsA(ATgPawn::StaticClass()))
                           ? (ATgPawn*)g->m_Instigator : nullptr;
        out.push_back({g->m_nEffectGroupId, g->m_nType, std::lround(left * 10.0f) / 10.0,
                       src ? MatchStats::UserForPawn(src) : 0});
    }
    return out;
}

// Re-apply each carried effect group through the normal ProcessEffect path,
// then set its life timer to exactly the time that was left.
void ApplyEffects(ATgPawn* Pawn, int64_t user_id, Carried& c) {
    ATgEffectManager* mgr = Pawn->r_EffectManager;
    int applied = 0, dropped = 0;
    for (const Effect& e : c.effects) {
        UTgEffectGroup* g = mgr ? BuildEffectGroup(e.eg_id, e.eg_type) : nullptr;
        // Snapshots come from the hosting node: only real timed groups, never
        // invulnerability, never longer than twice the group's own lifetime.
        if (!Carryable(g)) { dropped++; continue; }
        const float left = (std::min)(e.left, (std::min)(g->m_fLifeTime * 2.0f, kMaxEffectSecs));
        g->m_fLifeTime = left;
        ATgPawn* src = e.src_user ? MatchStats::LivePawnForUser(e.src_user) : nullptr;
        FImpactInfo impact{};
        mgr->eventProcessEffect(g, /*bRemove=*/0, /*Buffers=*/nullptr, (AActor*)(src ? src : Pawn), impact);
        // ProcessEffect may have cloned or refreshed the group: fix the timer
        // on the instance that is actually in the applied list.
        UTgEffectGroup* live = nullptr;
        for (int i = mgr->s_AppliedEffectGroups.Count - 1; i >= 0 && !live; i--) {
            UTgEffectGroup* a = mgr->s_AppliedEffectGroups.Data[i];
            if (a && a->m_nEffectGroupId == e.eg_id) live = a;
        }
        if (!live) { dropped++; continue; }
        live->m_fModLifeTime = left;
        mgr->SetTimer(left, 0, LifeDoneName(), live);
        Logger::Log("restore", "[Restore] effect user=%lld eg=%d type=%d src=%lld carried=%.1f now=%.1f\n",
            (long long)user_id, e.eg_id, e.eg_type, (long long)e.src_user, e.left,
            mgr->GetRemainingTimeForTimer(LifeDoneName(), live));
        applied++;
    }
    if (dropped)
        Logger::Log("restore", "[Restore] %d effect(s) of user=%lld dropped (unknown/not timed/not applied)\n",
            dropped, (long long)user_id);
    c.effects.clear();
}

int Rot16(float v) { return ((int)std::lround(v)) & 0xffff; }

bool Place(ATgPawn* Pawn, const Carried& c) {
    if (Pawn->SetLocation(c.loc)) return true;
    // Saved spot blocked (another pawn / mover there now): try just above it.
    FVector up = c.loc;
    up.Z += 48.0f;
    return Pawn->SetLocation(up);
}

void Apply(ATgPawn* Pawn, int64_t user_id, const Carried& c) {
    FRotator rot = c.rot;
    rot.Roll = 0;
    const bool placed = Place(Pawn, c);
    Pawn->SetRotation(rot);
    Pawn->SetViewRotation(rot);
    Pawn->ClientSetRotation(rot);
    if (AController* PC = Pawn->Controller) {
        if (placed) PC->ClientSetLocation(Pawn->Location, rot);
        PC->Rotation = rot;
        PC->ClientSetRotation(rot, 1);
    }

    const int maxHp = Pawn->r_nHealthMaximum > 0 ? Pawn->r_nHealthMaximum : Pawn->HealthMax;
    if (c.health > 0) Pawn->Health = maxHp > 0 ? (std::min)(c.health, maxHp) : c.health;
    if (c.power >= 0.0f)
        Pawn->r_fCurrentPowerPool = Pawn->r_fMaxPowerPool > 0.0f ? (std::min)(c.power, Pawn->r_fMaxPowerPool) : c.power;
    Pawn->MakeInvulnerable(kSpawnProtectSecs);
    Pawn->bNetDirty = 1;
    Pawn->bForceNetUpdate = 1;

    Logger::Log("restore", "[Restore] player user=%lld placed=%d at %.0f %.0f %.0f yaw=%d hp=%d/%d power=%.0f/%.0f\n",
        (long long)user_id, (int)placed, Pawn->Location.X, Pawn->Location.Y, Pawn->Location.Z, rot.Yaw,
        Pawn->Health, maxHp, Pawn->r_fCurrentPowerPool, Pawn->r_fMaxPowerPool);
}

}  // namespace

nlohmann::json Describe(ATgPawn* Pawn) {
    if (!Pawn || Pawn->Health <= 0 || Pawn->bDeleteMe) return nlohmann::json();
    FRotator rot = Pawn->Controller ? Pawn->Controller->Rotation : Pawn->Rotation;
    return {{"x", std::lround(Pawn->Location.X)}, {"y", std::lround(Pawn->Location.Y)},
            {"z", std::lround(Pawn->Location.Z)}, {"yaw", rot.Yaw & 0xffff}, {"pitch", rot.Pitch & 0xffff},
            {"hp", Pawn->Health}, {"hp_max", Pawn->r_nHealthMaximum},
            {"power", std::lround(Pawn->r_fCurrentPowerPool * 10.0f) / 10.0},
            {"power_max", std::lround(Pawn->r_fMaxPowerPool)}, {"cd", DescribeCooldowns(Pawn)},
            {"fx", DescribeEffects(Pawn)}, {"eq", DescribeEquipment(Pawn)}};
}

void SetCarried(const nlohmann::json& players) {
    g_carried.clear();
    if (!players.is_array()) return;
    const DWORD now = GetTickCount();
    for (const auto& p : players) {
        const int64_t user_id = p.value("user_id", (int64_t)0);
        if (user_id == 0 || !p.contains("pawn") || !p["pawn"].is_object()) continue;
        const auto& s = p["pawn"];
        Carried c{};
        c.loc.X = s.value("x", 0.0f);
        c.loc.Y = s.value("y", 0.0f);
        c.loc.Z = s.value("z", 0.0f);
        c.rot.Yaw   = Rot16(s.value("yaw", 0.0f));
        c.rot.Pitch = Rot16(s.value("pitch", 0.0f));
        c.health = s.value("hp", 0);
        c.power  = s.value("power", -1.0f);
        c.since  = now;
        if (s.contains("cd") && s["cd"].is_array()) {
            for (const auto& e : s["cd"]) {
                if (!e.is_array() || e.size() < 4 || !e[0].is_number() || !e[1].is_number() ||
                    !e[2].is_number() || !e[3].is_number()) continue;
                Cooldown cd{e[0].get<int>(), e[1].get<int>(), e[2].get<int>(), e[3].get<float>()};
                // Snapshots come from the hosting node: never longer than a real device cooldown.
                cd.left = (std::min)(cd.left, kMaxCooldownSecs);
                if (cd.left >= kMinCooldownSecs) c.cooldowns.push_back(cd);
            }
        }
        if (s.contains("fx") && s["fx"].is_array()) {
            for (const auto& e : s["fx"]) {
                if ((int)c.effects.size() >= kMaxEffects) break;
                if (!e.is_array() || e.size() < 4 || !e[0].is_number_integer() || !e[1].is_number_integer() ||
                    !e[2].is_number() || !e[3].is_number_integer()) continue;
                Effect fx{e[0].get<int>(), e[1].get<int>(), e[2].get<float>(), e[3].get<int64_t>()};
                if (fx.left >= kMinEffectSecs) c.effects.push_back(fx);
            }
        }
        if (s.contains("eq") && s["eq"].is_object() && s["eq"].contains("d") && s["eq"]["d"].is_array()) {
            // Only a hand slot whose device the snapshot lists is carried; the
            // new host must have that same device there (checked on apply).
            const int ih = s["eq"].value("ih", -1);
            if (ih == GA::EquipSlot::Melee || ih == GA::EquipSlot::Ranged || ih == GA::EquipSlot::Specialty) {
                for (const auto& e : s["eq"]["d"]) {
                    if (e.is_array() && e.size() >= 2 && e[0].is_number_integer() && e[1].is_number_integer() &&
                        e[0].get<int>() == ih) {
                        c.in_hand = ih;
                        c.in_hand_dev = e[1].get<int>();
                    }
                }
            }
        }
        if (c.health > 0) g_carried[user_id] = c;
    }
    Logger::Log("restore", "[Restore] %zu player state(s) carried in\n", g_carried.size());
}

void Tick(ATgGame* Game) {
    if (g_carried.empty()) return;
    if (!Game || Game->bGameEnded) { g_carried.clear(); return; }
    const DWORD now = GetTickCount();
    for (auto it = g_carried.begin(); it != g_carried.end();) {
        ATgPawn* Pawn = MatchStats::LivePawnForUser(it->first);
        Carried& c = it->second;
        if (Pawn && Pawn->Health > 0 && Pawn->Controller && !Pawn->bDeleteMe) {
            if (!c.placed) {
                Apply(Pawn, it->first, c);
                ApplyEffects(Pawn, it->first, c);
                c.placed = true;
                c.placed_at = now;
            }
            const bool hand_done = ApplyInHand(Pawn, it->first, c);
            if (ApplyCooldowns(Pawn, it->first, c) && hand_done) {
                it = g_carried.erase(it);
            } else if (now - c.placed_at > kCooldownWaitMs) {
                Logger::Log("restore", "[Restore] %zu cooldown(s)%s of user=%lld dropped (device not equipped)\n",
                    c.cooldowns.size(), c.in_hand >= 0 ? " + in-hand slot" : "", (long long)it->first);
                it = g_carried.erase(it);
            } else {
                ++it;
            }
        } else if (c.placed && now - c.placed_at > kCooldownWaitMs) {
            it = g_carried.erase(it);
        } else if (!c.placed && now - c.since > kCarryTtlMs) {
            it = g_carried.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace PlayerState
