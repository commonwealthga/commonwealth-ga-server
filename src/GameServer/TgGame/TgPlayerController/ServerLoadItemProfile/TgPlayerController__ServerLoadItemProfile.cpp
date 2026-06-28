#include "src/GameServer/TgGame/TgPlayerController/ServerLoadItemProfile/TgPlayerController__ServerLoadItemProfile.hpp"
#include "src/GameServer/Armor/Armor.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.hpp"
#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/Database/Database.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/sqlite3/sqlite3.h"
#include <cstdlib>

static int LookupGameplayInvIdForSlot(sqlite3* db, int64_t character_id, int item_profile_id, int slot) {
    if (!db) return 0;
    sqlite3_stmt* st = nullptr;
    int invId = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT MIN(pi.id) "
        "FROM ga_character_devices cd "
        "JOIN ga_players_inventory pi ON pi.id = cd.inventory_id "
        "WHERE cd.character_id = ? AND cd.item_profile_id = ? "
        "  AND cd.equipped_slot = ? AND pi.device_id > 0",
        -1, &st, nullptr) == SQLITE_OK && st) {
        sqlite3_bind_int64(st, 1, character_id);
        sqlite3_bind_int  (st, 2, item_profile_id);
        sqlite3_bind_int  (st, 3, slot);
        if (sqlite3_step(st) == SQLITE_ROW) {
            invId = sqlite3_column_int(st, 0);
        }
        sqlite3_finalize(st);
    }
    return invId;
}

// Reliable-server RPC fired when the player clicks a loadout-slot button
// (1..5) anywhere in the UI. Atomic switch:
//   1. unequip current profile's devices (RCST's PHASE 1 still sees the
//      old applied state and reverses it on the next call)
//   2. flip Pawn->r_nItemProfileId + refresh info->skills from DB
//   3. equip target profile's gameplay devices + reload profile cosmetics
//   4. RCST PHASE 1 reverts old armor/skills, PHASE 2 applies new armor
//      (FetchEquippedArmor scoped by new r_nItemProfileId) + new skills
//      (from refreshed info->skills)
//   5. IPC the control-server so it persists current_item_profile_id and
//      re-ships SEND_INVENTORY + SEND_PLAYER_SKILLS to the client
//   6. CallOriginal runs the engine native (which may update
//      r_nItemProfileNbr / fire UC follow-ups we don't model)
//
// Designed for anywhere-anytime invocation including mid-combat. Hostile
// DoT effects (r_EffectManager-managed) ride through unchanged since the
// revert pass operates only on loadout-sourced state (Armor Records map +
// s_AppliedEffectGroups skill clones).
void __fastcall TgPlayerController__ServerLoadItemProfile::Call(
    ATgPlayerController* Controller, void* edx, int nId)
{
    LogCallBegin();

    if (!Controller || !Controller->Pawn) {
        Logger::Log("loadout", "[ServerLoadItemProfile] no controller/pawn; nId=%d — passthrough\n", nId);
        CallOriginal(Controller, edx, nId);
        LogCallEnd();
        return;
    }
    if (nId < 1 || nId > 5) {
        Logger::Log("loadout", "[ServerLoadItemProfile] out-of-range nId=%d — ignored\n", nId);
        LogCallEnd();
        return;
    }

    ATgPawn_Character* Pawn = (ATgPawn_Character*)Controller->Pawn;
    const int oldProfile = Pawn->r_nItemProfileId;
    if (nId == oldProfile) {
        Logger::Log("loadout", "[ServerLoadItemProfile] no-op self-click profile=%d\n", nId);
        CallOriginal(Controller, edx, nId);
        LogCallEnd();
        return;
    }

    auto sessIt = GPawnSessions.find((ATgPawn*)Pawn);
    if (sessIt == GPawnSessions.end()) {
        Logger::Log("loadout", "[ServerLoadItemProfile] no GPawnSessions entry — bail\n");
        LogCallEnd();
        return;
    }
    const std::string guid = sessIt->second;
    PlayerInfo* info = PlayerRegistry::GetByGuidPtr(guid);
    if (!info) {
        Logger::Log("loadout", "[ServerLoadItemProfile] no PlayerInfo for guid=%s — bail\n",
            guid.c_str());
        LogCallEnd();
        return;
    }
    const int64_t character_id = info->selected_character_id;

    Logger::Log("loadout",
        "[ServerLoadItemProfile] switch %d -> %d char=%lld guid=%s\n",
        oldProfile, nId, (long long)character_id, guid.c_str());

    sqlite3* db = Database::GetConnection();

    // Destroy all personal deployables on profile switch — turrets, drones,
    // stations etc. placed under the old loadout should not persist.
    TgPawn__KillDeployables::KillAllOwned((ATgPawn*)Pawn);

    // ── Step 1: Tear down current-profile device equipment ────────────────
    // Inventory::Unequip is idempotent on already-cleared slots; we walk all
    // engine equip points (1..24) to cover every weapon/utility/cosmetic.
    // Cosmetic visual fields (r_CustomCharacterAssembly) are reset in step 5
    // after the new profile's cosmetics apply.
    //
    // SKIP SLOT 14 (CLASS_DEVICE / HUMAN BASE ATTRIBUTES). It's server-pinned:
    // SaveEquippedDevices forces a slot-14 row into every profile keyed to the
    // class device's inventory_id (client UI hides slot 14, so the client always
    // sends SlotIndices[14]=0 and the equip-screen save path never re-creates
    // the actor). The class device is also per-CHARACTER (same across all 5
    // loadout profiles), so destroying + not re-spawning it on every switch
    // breaks SEND_INVENTORY: the post-equip-save ship contains the slot-14
    // invId with DEVICE_ID=class_inv_id, but no live actor matches →
    // CGameClient::SendInventory's recovery scan resubmits every frame
    // ("Inventory Error Recovery: Resubmitting…") until the client reconnects.
    // Leaving slot 14 alone keeps the original spawn-time class device alive
    // through every profile switch.
    // SKIP non-persist devices (beacons) along with slot 14 (class device).
    // A picked-up beacon is bound to the PAWN not the LOADOUT — it's equipped
    // in slot 11 across all 5 profiles (see send_beacon_pickup_response which
    // ships EQUIPPED_SLOT_VALUE_ID=11 for every profile row), so the device
    // bar shows it regardless of the active profile. Tearing it down on
    // switch destroys the carrier device actor without removing its applied
    // equip effects (the carrier-ring visual stays on the pawn) AND leaves
    // the client's m_InventoryMap with a stale invId → "Inventory Error
    // Recovery: Resubmitting" loop + r_ItemCount mismatch. Same idiom as
    // slot 14: leave it alone, the consume path (NonPersistRemoveDevice on
    // place / death) and beacon respawn handle the actual teardown.
    int teardown_count = 0;
    for (int slot = 1; slot < 0x19; ++slot) {
        if (slot == 14) continue;
        ATgDevice* slotDev = Pawn->m_EquippedDevices[slot];
        if (slotDev == nullptr) continue;

        using IsBeaconFn = int(__fastcall*)(ATgDevice*, void*);
        const bool bIsBeacon = ((IsBeaconFn)0x10a19a40)(slotDev, nullptr) != 0;
        if (bIsBeacon) continue;  // beacon survives profile switch

        const int targetInvId = LookupGameplayInvIdForSlot(db, character_id, nId, slot);
        if (targetInvId > 0 && slotDev->r_nDeviceInstanceId == targetInvId) {
            continue;
        }

        Inventory::Unequip((ATgPawn*)Pawn, slot);
        ++teardown_count;
    }
    Logger::Log("loadout",
        "[ServerLoadItemProfile] unequipped %d device(s) from old profile %d\n",
        teardown_count, oldProfile);

    // ── Step 2: Flip active profile + refresh in-memory skill cache ──────
    Pawn->r_nItemProfileId = nId;
    info->current_item_profile_id = nId;
    PlayerRegistry::RefreshSkillsForProfile(guid, nId);

    // ── Step 3: Equip new profile's gameplay devices ─────────────────────
    int gameplay_equipped = 0;
    int gameplay_reused = 0;
    if (db) {
        sqlite3_stmt* st = nullptr;
        // GROUP BY equipped_slot defends against duplicate ga_character_devices
        // rows for the same (character_id, item_profile_id, equipped_slot). The
        // UNIQUE constraint *should* prevent this, but a historical bug (a
        // cosmetic-apply path that lacked INSERT OR REPLACE) seeded thousands
        // of dupes on legacy DBs — one slot processed 50000+ times before this
        // guard, and the downstream crash signature in repro was
        // FString::operator*() with this=null, almost certainly from
        // replication/serialization choking on the over-dirtied
        // r_CustomCharacterAssembly. MIN(cd.inventory_id) just picks one
        // deterministically; PurgeDuplicateCharacterDevices in Database.cpp
        // collapses the table to one row per slot at next server start.
        const char* sql =
            "SELECT cd.equipped_slot, pi.device_id, pi.quality, "
            "       pi.id, pi.mod_effect_group_ids, pi.item_id "
            "FROM ga_character_devices cd "
            "JOIN ga_players_inventory pi ON pi.id = cd.inventory_id "
            "WHERE cd.character_id = ? AND cd.item_profile_id = ? "
            "GROUP BY cd.equipped_slot";
        if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(st, 1, character_id);
            sqlite3_bind_int  (st, 2, nId);
            while (sqlite3_step(st) == SQLITE_ROW) {
                const int slot    = sqlite3_column_int(st, 0);
                const int devId   = sqlite3_column_int(st, 1);
                const int quality = sqlite3_column_int(st, 2);
                const int invId   = sqlite3_column_int(st, 3);
                const int itemId  = sqlite3_column_int(st, 5);

                // Skip armor — RCST's ApplyDefaultArmor handles those rows
                // via Armor::FetchEquippedArmor (now profile-scoped).
                if (slot >= 1130 && slot <= 1143) continue;

                // Skip slot 14 — class device is server-pinned and persists
                // across profile switches (see teardown comment above). If the
                // DB has a slot-14 row for this profile it'll match the live
                // class device's invId and Inventory::Equip's same-invId
                // short-circuit would no-op anyway; explicit skip documents
                // the invariant.
                if (slot == 14) continue;

                std::vector<int> mods;
                const char* csv = (const char*)sqlite3_column_text(st, 4);
                if (csv && *csv) {
                    const char* p = csv;
                    while (*p) {
                        while (*p == ',' || *p == ' ') ++p;
                        if (!*p) break;
                        char* end = nullptr;
                        long v = std::strtol(p, &end, 10);
                        if (end == p) break;
                        mods.push_back((int)v);
                        p = end;
                    }
                }

                if (devId > 0 && slot >= 1 && slot < 0x19) {
                    ATgDevice* current = Pawn->m_EquippedDevices[slot];
                    if (current != nullptr && current->r_nDeviceInstanceId == invId) {
                        ++gameplay_reused;
                        Logger::Log("loadout",
                            "[ServerLoadItemProfile] equip-loop reuse: slot=%d devId=%d invId=%d\n",
                            slot, devId, invId);
                        continue;
                    }
                    Logger::Log("loadout",
                        "[ServerLoadItemProfile] equip-loop pre:  slot=%d devId=%d invId=%d quality=%d mods=%zu pawn=%p invMgr=%p\n",
                        slot, devId, invId, quality, mods.size(), Pawn, Pawn->InvManager);
                    Inventory::Equip((ATgPawn*)Pawn, devId, slot, quality, invId, mods);
                    Logger::Log("loadout",
                        "[ServerLoadItemProfile] equip-loop post: slot=%d devId=%d (Equip returned)\n",
                        slot, devId);
                    ++gameplay_equipped;
                }
            }
            sqlite3_finalize(st);
        }
    }

    CosmeticEquip::LoadFromDB((ATgPawn*)Pawn, character_id, nId);

    Logger::Log("loadout",
        "[ServerLoadItemProfile] pre-Finalize: pawn=%p invMgr=%p changed=%d reused=%d\n",
        Pawn, Pawn->InvManager, teardown_count + gameplay_equipped, gameplay_reused);
    // Use native UpdateClientDevices but skip the post-native dirty tail.
    // Bare Finalize(Pawn) previously faulted before refresh_profile_ui could repaint.
    Inventory::Finalize((ATgPawn*)Pawn, false);
    Logger::Log("loadout",
        "[ServerLoadItemProfile] post-Finalize done; changed=%d reused=%d\n",
        teardown_count + gameplay_equipped, gameplay_reused);
    Logger::Log("loadout",
        "[ServerLoadItemProfile] equipped new profile %d: %d gameplay device(s), reused=%d; cosmetics reloaded\n",
        nId, gameplay_equipped, gameplay_reused);

    // ── Step 3b: Reset InvManager.r_ItemCount to the DB pool size ────────
    // Inventory::Equip blindly ++'s r_ItemCount on every call (gameplay devices
    // only). Across a few switches into populated profiles it drifts above the
    // real pool count → the client's InvManager.IsValid (r_ItemCount vs
    // m_InventoryMap.Num) gate flips false → FixupWidgets bails → equip-screen
    // slots blank AND the device-bar binding stops repainting. Same fix as
    // ServerAcceptNewProfileFromEquipScreen — reseat to the actual ga_players_inventory
    // count for this user+class so the client's IsValid passes after every switch.
    if (Pawn->InvManager && db) {
        sqlite3_stmt* who = nullptr;
        if (sqlite3_prepare_v2(db,
            "SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
            -1, &who, nullptr) == SQLITE_OK && who) {
            sqlite3_bind_int64(who, 1, character_id);
            if (sqlite3_step(who) == SQLITE_ROW) {
                const int64_t user_id    = sqlite3_column_int64(who, 0);
                const int     profile_id = sqlite3_column_int  (who, 1);
                sqlite3_stmt* cnt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "SELECT COUNT(*) FROM ga_players_inventory "
                    "WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?)",
                    -1, &cnt, nullptr) == SQLITE_OK && cnt) {
                    sqlite3_bind_int64(cnt, 1, user_id);
                    sqlite3_bind_int  (cnt, 2, profile_id);
                    if (sqlite3_step(cnt) == SQLITE_ROW) {
                        const int total = sqlite3_column_int(cnt, 0);
                        ((ATgInventoryManager*)Pawn->InvManager)->r_ItemCount = total;
                        Logger::Log("loadout",
                            "[ServerLoadItemProfile] InvManager->r_ItemCount=%d (pool total, IsValid gate)\n",
                            total);
                    }
                    sqlite3_finalize(cnt);
                }
            }
            sqlite3_finalize(who);
        }
    }

    // ── Step 4: RCST does the armor + skill revert+apply atomically ──────
    TgPawn_Character__ReapplyCharacterSkillTree::Call(Pawn, nullptr);

    // ── Step 5: Run native before the control-server UI refresh packet.
    // Native owns client profile button state; refreshing earlier races it.
    Logger::Log("loadout",
        "[ServerLoadItemProfile] pre-CallOriginal\n");
    CallOriginal(Controller, edx, nId);
    Logger::Log("loadout",
        "[ServerLoadItemProfile] post-CallOriginal\n");

    // The control server sends ClientResetEquipScreen after its TCP refresh.
    // Firing it here can rebuild the open skill UI before new rows arrive.
    // ── Step 6: IPC the control-server to persist + push refreshed packets
    {
        nlohmann::json ev;
        ev["type"]            = IpcProtocol::MSG_GAME_EVENT;
        ev["subtype"]         = "profile_switch";
        ev["instance_id"]     = IpcClient::GetInstanceId();
        ev["session_guid"]    = guid;
        ev["character_id"]    = character_id;
        ev["pawn_id"]         = (int)Pawn->r_nPawnId;
        ev["item_profile_id"] = nId;
        IpcClient::Send(ev.dump());
    }

    Logger::Log("loadout",
        "[ServerLoadItemProfile] switch complete %d -> %d\n", oldProfile, nId);

    LogCallEnd();
}
