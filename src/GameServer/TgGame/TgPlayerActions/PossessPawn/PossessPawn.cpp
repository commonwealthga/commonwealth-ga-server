#include "src/GameServer/TgGame/TgPlayerActions/PossessPawn/PossessPawn.hpp"

#include <cmath>
#include <cstring>
#include <map>
#include <vector>

#include "lib/nlohmann/json.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::PossessCmd {

namespace {

struct State {
    ATgPlayerController* pc                = nullptr;
    ATgPawn*             original_pawn     = nullptr;
    ATgPawn*             possessed_bot     = nullptr;
    ATgAIController*     bot_ai_controller = nullptr;
    int                  player_pawn_id    = 0;
};

// Active possessions keyed by session_guid. We index on the guid (not the PC
// pointer) so reconnects / mid-session lifecycle changes can't strand the
// state. -unpossess looks the entry up by the same guid the player used.
std::map<std::string, State> g_states;

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid) {
            return kv.second.Pawn;
        }
    }
    return nullptr;
}

constexpr float kPi          = 3.14159265358979323846f;
constexpr float kRotToRad    = kPi / 32768.0f;   // UE3 rotator unit = 360/65536
constexpr float kTraceDistUU = 10000.0f;          // 100m forward — generous

// Inverse of the BotGetEquipPointBySlot table in TgGame__SpawnBotById.cpp:
// engine equip point (1..24) → slot_value_id (group 48). Returns 0 for an
// out-of-range/unknown slot — the client treats SVID 0 as "skip this profile
// row" so the device shows up in inventory but has no equip-bar binding.
int SlotValueIdFromEquipPoint(int equipPoint) {
    switch (equipPoint) {
        case 1:  return 221;  case 2:  return 198;  case 3:  return 200;
        case 4:  return 199;  case 5:  return 201;  case 6:  return 202;
        case 7:  return 203;  case 8:  return 204;  case 9:  return 385;
        case 10: return 386;  case 11: return 499;  case 12: return 500;
        case 13: return 501;  case 14: return 502;  case 15: return 823;
        case 16: return 996;  case 17: return 997;  case 18: return 998;
        case 19: return 999;  case 20: return 1000; case 21: return 1001;
        case 22: return 1002; case 23: return 1003; case 24: return 1004;
        default: return 0;
    }
}

// Walk the bot's 25 equip-point slots, flipping each ATgDevice between
// "server-only" (the default for bot loadouts, see GiveDevicesFromBotConfig)
// and "replicated to clients, owned by the controlling pawn". Needed during
// possession so the player's client gets device actors for the bot's weapons.
//
// Two pieces matter:
//   1. RemoteRole=1 / bSkipActorPropertyReplication=0 — so the device actor
//      replicates at all, and its CPF_Net fields (r_nDeviceInstanceId etc.)
//      actually go over the wire. Without these, the client's SEND_INVENTORY
//      parser can't link the inventory record to a live ATgDevice
//      (FUN_10a14a50 matches by r_nDeviceInstanceId) so m_EquippedDevices
//      stays empty and weapon switching / firing silently no-ops.
//   2. Device.Owner = pawn — anchors the device actor's NetDriver channel to
//      the pawn. NetDriver walks Owner up to find the owning Connection;
//      with Owner unset the channel is "any-relevant", which can be reclaimed
//      after the initial replication burst (matches the user's observation
//      that switching works briefly then breaks).
void SetBotDevicesReplication(ATgPawn* bot, bool replicate) {
    if (!bot) return;
    for (int slot = 0; slot < 25; ++slot) {
        ATgDevice* dev = (ATgDevice*)bot->m_EquippedDevices[slot];
        if (!dev) continue;
        if (replicate) {
            dev->SetOwner((AActor*)bot);
            dev->RemoteRole                    = 1;
            dev->bSkipActorPropertyReplication = 0;
            dev->bNetInitial                   = 1;
            dev->bNetDirty                     = 1;
            dev->bForceNetUpdate               = 1;
        } else {
            // Match the original bot-spawn settings — server-only, dropped
            // from NetDriver's per-tick consideration.
            dev->RemoteRole                    = 0;
            dev->bSkipActorPropertyReplication = 1;
            dev->bNetInitial                   = 0;
            dev->bNetDirty                     = 1;
        }
    }
}

// Read the bot's currently-equipped devices off r_EquipDeviceInfo (24 slots).
// Each non-zero entry becomes one SEND_INVENTORY record on the client.
nlohmann::json BuildBotLoadoutJson(ATgPawn* bot) {
    nlohmann::json arr = nlohmann::json::array();
    if (!bot) return arr;
    // r_EquipDeviceInfo[0..24], indexed directly by engine equip point (1..24).
    // Slot 0 is unused (SpawnBotById writes the device at r_EquipDeviceInfo[
    // equipPoint] where equipPoint is the 1-based engine index).
    for (int slot = 1; slot < 25; ++slot) {
        const FEquipDeviceInfo& info = bot->r_EquipDeviceInfo[slot];
        if (info.nDeviceId == 0) continue;
        int svid = SlotValueIdFromEquipPoint(slot);
        if (svid == 0) continue;
        nlohmann::json rec;
        rec["device_id"]     = info.nDeviceId;
        rec["inventory_id"]  = info.nDeviceInstanceId;
        rec["quality"]       = info.nQualityValueId ? info.nQualityValueId : 1162;
        rec["slot_value_id"] = svid;
        arr.push_back(rec);
    }
    return arr;
}

void SendShowLoadoutEvent(const std::string& guid, int pawn_id, const nlohmann::json& devices) {
    nlohmann::json ev;
    ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
    ev["subtype"]      = "possess_show_loadout";
    ev["instance_id"]  = IpcClient::GetInstanceId();
    ev["session_guid"] = guid;
    ev["pawn_id"]      = pawn_id;
    ev["devices"]      = devices;
    IpcClient::Send(ev.dump());
}

void SendRestoreInventoryEvent(const std::string& guid, int pawn_id, int64_t character_id) {
    nlohmann::json ev;
    ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
    ev["subtype"]      = "possess_restore_inventory";
    ev["instance_id"]  = IpcClient::GetInstanceId();
    ev["session_guid"] = guid;
    ev["pawn_id"]      = pawn_id;
    ev["character_id"] = character_id;
    IpcClient::Send(ev.dump());
}

} // namespace

void ExecutePossess(const std::string& guid) {
    if (g_states.count(guid)) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: already possessing — run -unpossess first\n",
            guid.c_str());
        return;
    }

    ATgPawn_Character* PlayerPawn = FindPawnBySessionGuid(guid);
    if (!PlayerPawn || !PlayerPawn->Controller) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: no player pawn/controller\n", guid.c_str());
        return;
    }

    // The session's Controller is always a TgPlayerController in this build —
    // bots get TgAIController. Defensive class check anyway.
    if (!PlayerPawn->Controller->Class ||
        !std::strstr(PlayerPawn->Controller->Class->GetFullName(), "PlayerController")) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: session controller isn't a PlayerController\n",
            guid.c_str());
        return;
    }
    ATgPlayerController* PC = (ATgPlayerController*)PlayerPawn->Controller;

    // Build aim trace from pawn-eye outward along the PC's view rotation. The
    // PC view rotation is what the player is aiming (matches crosshair) — the
    // pawn's body rotation lags during movement.
    FVector start = PlayerPawn->Location;
    start.Z += PlayerPawn->EyeHeight;

    FRotator rot = PC->Rotation;
    float yawRad   = static_cast<float>(rot.Yaw)   * kRotToRad;
    float pitchRad = static_cast<float>(rot.Pitch) * kRotToRad;
    float cp = std::cos(pitchRad);

    FVector end;
    end.X = start.X + std::cos(yawRad) * cp * kTraceDistUU;
    end.Y = start.Y + std::sin(yawRad) * cp * kTraceDistUU;
    end.Z = start.Z + std::sin(pitchRad)    * kTraceDistUU;

    FVector hitLoc, hitNormal;
    FVector extent = {0.f, 0.f, 0.f};
    FTraceHitInfo hitInfo = {};
    // bTraceActors=1, ExtraTraceFlags=0 → engine defaults (block-static + pawn).
    AActor* hit = PlayerPawn->Trace(end, start, 1, extent, 0, &hitLoc, &hitNormal, &hitInfo);
    if (!hit) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: trace hit nothing\n", guid.c_str());
        return;
    }

    const char* hitClassName = hit->Class ? hit->Class->GetFullName() : "<no-class>";

    // Hit must be an ATgPawn (or descendant). Anything else (worldgeom, projectile,
    // device, etc.) is rejected silently with a log line.
    if (!std::strstr(hitClassName, "TgPawn")) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: trace hit non-pawn '%s'\n",
            guid.c_str(), hitClassName);
        return;
    }
    ATgPawn* target = (ATgPawn*)hit;

    if (target == (ATgPawn*)PlayerPawn) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: trace hit self\n", guid.c_str());
        return;
    }

    if (!target->Controller || !target->Controller->Class) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: target '%s' has no controller\n",
            guid.c_str(), hitClassName);
        return;
    }

    const char* targetCtrlName = target->Controller->Class->GetFullName();

    // AI-only gate. Refuse to seize another player's pawn.
    if (std::strstr(targetCtrlName, "PlayerController")) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: target is a player ('%s'); only AI allowed\n",
            guid.c_str(), targetCtrlName);
        return;
    }
    // Must be a TgAIController so re-attaching at -unpossess is sound.
    if (!std::strstr(targetCtrlName, "TgAIController")) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /possess guid=%s: target controller '%s' isn't a TgAIController\n",
            guid.c_str(), targetCtrlName);
        return;
    }
    ATgAIController* AI = (ATgAIController*)target->Controller;

    // Pawn IDs are stable identities — we never mutate them here. The client's
    // SEND_INVENTORY dispatcher (FUN_10913760) routes by PAWN_ID matching the
    // local PC.Pawn's r_nPawnId (or its r_ControlPawn fallback). After the
    // controller swap the client's PC.Pawn is the bot, so sending PAWN_ID =
    // bot->r_nPawnId routes the loadout into the bot's InvManager directly.
    // (UC TgPawn.PostBeginPlay assigns each pawn a unique per-instance id via
    // TgGame.GetNextPawnId(), so players and bots never collide within a map.)
    const int player_pawn_id = (int)PlayerPawn->r_nPawnId;
    const int bot_pawn_id    = (int)target->r_nPawnId;

    State st;
    st.pc                = PC;
    st.original_pawn     = (ATgPawn*)PlayerPawn;
    st.possessed_bot     = target;
    st.bot_ai_controller = AI;
    st.player_pawn_id    = player_pawn_id;
    g_states[guid]       = st;

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /possess guid=%s: PC=%p body=%p (pawn_id=%d) -> bot=%p "
        "(class=%s pawn_id=%d) AI=%p — invoking CrewBot (vehicle/crew model)\n",
        guid.c_str(), (void*)PC, (void*)PlayerPawn, player_pawn_id,
        (void*)target, hitClassName, bot_pawn_id, (void*)AI);

    // CREW path (vs HACK path).
    //
    // The previous attempt used TgPawn::HackedBy — the hacker-pet mechanic. That
    // path does a PRI swap + Controller swap on top of vehicle-mode Possess
    // calls, and is meant for the deploy-hacker-bot-and-temporarily-take-it-over
    // gameplay. In this build several invariants it relies on are stripped
    // (notably SetInventoryDirty is a no-op stub), and even with our SDK shims
    // the PRI swap leaves persistent corruption in player state after Unhacked.
    //
    // CrewBot (native @ 0x109d02c0, exposed via SDK) is the engine's
    // vehicle/crew mechanic — meant for "driver enters vehicle". It does:
    //   - player.r_bIsCrewing = true   (CPF_Net, repnotify → ClientOnCrewingBot
    //     on the client which hides the player mesh + disables collision)
    //   - bot.r_ControlPawn = player   (back-ref)
    //   - bot.r_nNumberTimesCrewed++
    //   - Disables player collision server-side
    //   - Fires three UC events on the player (probably setup hooks)
    //   - Optional HexItem notify
    // What it does NOT do:
    //   - No PRI swap        ← no PRI corruption to unwind
    //   - No Controller swap ← PC.Pawn stays bound to player_pawn at the engine
    //                          level; engine-level input routing to the bot
    //                          happens through native paths that read the
    //                          back-refs we just set.
    //
    // UNKNOWN: whether the engine's native input dispatch actually routes
    // player fire/move to the bot when r_bIsCrewing=true. This is the empirical
    // test. If yes, this is the clean path. If no, we'll see the player firing
    // their (invisible) own weapons and we'll need to add explicit input
    // forwarding hooks.
    SetBotDevicesReplication(target, true);

    // Set s_bIsCrewable=true on the bot so UC's CanBeCrewedBy gates would pass
    // (we call CrewBot directly so the gate isn't applied, but other engine
    // code that observes a crewed bot may check this flag).
    target->s_bIsCrewable = 1;

    // Prime bot.Weapon — same rationale as before. Even if the engine forwards
    // input to the bot via crewing semantics, the StartFire path still needs
    // Pawn.Weapon set.
    if (target->m_CurrentInHandDevice) {
        target->Weapon = (AWeapon*)target->m_CurrentInHandDevice;
        target->m_CurrentInHandDevice->Instigator = (APawn*)target;
    }

    PlayerPawn->CrewBot(target);

    // Force ClientWeaponSet so the client's bot.Weapon binds. Cheap and safe
    // regardless of whether crewing routes input correctly.
    if (target->m_CurrentInHandDevice && target->InvManager) {
        target->eventSetActiveWeapon((AWeapon*)target->m_CurrentInHandDevice, 0, 1);
    }
}

void ExecuteUnpossess(const std::string& guid) {
    auto it = g_states.find(guid);
    if (it == g_states.end()) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /unpossess guid=%s: not currently possessing\n",
            guid.c_str());
        return;
    }
    State st = it->second;
    g_states.erase(it);

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /unpossess guid=%s: PC=%p back to body=%p, AI=%p re-takes bot=%p\n",
        guid.c_str(), (void*)st.pc, (void*)st.original_pawn,
        (void*)st.bot_ai_controller, (void*)st.possessed_bot);

    if (!st.pc) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /unpossess guid=%s: stored PC is null — dropping\n", guid.c_str());
        return;
    }

    ATgPawn* bot = st.possessed_bot;

    // Mirror image of CrewBot: UnCrewed (native @ 0x109c8550) clears
    // r_bIsCrewing on the player, re-enables player collision, and fires the
    // shared UC events to undo the crew setup. No PRI/Controller restoration
    // needed because crewing didn't swap them in the first place.
    if (bot && !bot->bDeleteMe) {
        bot->UnCrewed(/*bKillControlPawn*/0);
        SetBotDevicesReplication(bot, false);
    } else {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /unpossess guid=%s: bot gone (bot=%p bDeleteMe=%d) — skipping UnCrewed\n",
            guid.c_str(), (void*)bot, bot ? (int)bot->bDeleteMe : -1);
    }

    // Re-bind the player's weapon on the client for safety. With the crew
    // model PC.Pawn never changed, so this should be a no-op refresh — but
    // cheap insurance against any client state drift.
    if (st.original_pawn && !st.original_pawn->bDeleteMe &&
        st.original_pawn->m_CurrentInHandDevice && st.original_pawn->InvManager) {
        st.original_pawn->eventSetActiveWeapon(
            (AWeapon*)st.original_pawn->m_CurrentInHandDevice, 0, 1);
    }

    // No explicit SendRestoreInventoryEvent here — Unhacked fires
    // SetInventoryDirty on C.Pawn.InvManager (now the player's InvManager),
    // and our hook handles player-side refresh through the DB-backed path.
    // Doing it manually here on top of the hook double-sends and was racing
    // against the natural engine flow.
}

// ---------------------------------------------------------------------------
// TrySendBotLoadoutRefresh — bot-side hook for SetInventoryDirty
// ---------------------------------------------------------------------------
// Bots have no DB-backed inventory, so the standard player refresh path
// (DB lookup → send_inventory_response) doesn't fit. When SetInventoryDirty
// fires on a bot's InvManager AND that bot is currently being possessed by
// some session, push the bot's r_EquipDeviceInfo as a loadout instead. Same
// wire shape as send_loadout_inventory_response.
bool TrySendBotLoadoutRefresh(ATgPawn* bot) {
    if (!bot) return false;

    std::string guid;
    for (const auto& kv : g_states) {
        if (kv.second.possessed_bot == bot) { guid = kv.first; break; }
    }
    if (guid.empty()) return false;

    nlohmann::json loadout = BuildBotLoadoutJson(bot);
    if (loadout.empty()) return false;

    SendShowLoadoutEvent(guid, (int)bot->r_nPawnId, loadout);
    Logger::Log("inventory",
        "TrySendBotLoadoutRefresh: bot=%p pawn_id=%d %d devices guid=%s\n",
        (void*)bot, (int)bot->r_nPawnId, (int)loadout.size(), guid.c_str());
    return true;
}

} // namespace TgPlayerActions::PossessCmd
