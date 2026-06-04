#pragma once

#include <string>
#include <optional>
#include <map>
#include <array>
#include <vector>
#include <cstdint>
#include <mutex>
#include "src/ControlServer/QuestStore/QuestStore.hpp"

struct SessionInfo {
    std::string session_guid;
    std::string player_name;
    std::string ip_address;
    int64_t user_id = 0;
    int64_t selected_character_id = 0;
    uint32_t selected_profile_id = 0;
};

struct CharacterInfo {
    int64_t id = 0;
    int64_t user_id = 0;
    uint32_t profile_id = 0;
    uint32_t head_asm_id = 0;
    uint32_t gender_type_value_id = 0;
    std::vector<uint8_t> morph_data;
    // Appearance fields the head-menu collects but pre-v81 used to throw
    // away. Skin/eye material parameter ids are stored for completeness;
    // not yet emitted in GSC_CHARACTER_LIST (the lobby card protocol only
    // ships hair) — kept available for future spawn-time application.
    //
    // hair_asm_id default 1974 = "NewHair15" (asm_mesh_type_value_id=850),
    // the in-game pawn hair category — same asm SpawnBotPawn sets on bots.
    // 0 crashes (no such asm). 403 PC_CHARBUILD_Bald also crashes at
    // gameplay time (type 596, character-builder UI only, not loaded by
    // the in-game asm-attach pipeline).
    uint32_t hair_asm_id = 1974;
    uint32_t skin_mat_param_id = 0;
    uint32_t eye_mat_param_id = 0;
};

struct DeviceRow {
    int              device_id;
    int              equip_slot;       // engine equip point 1..24 (from ga_character_devices.equipped_slot)
    int              slot_value_id;    // canonical SVID for this slot — derived from equip_slot at read time
    int              quality;
    int              inventory_id;     // = ga_players_inventory.id (new source of truth)
    int              effect_group_id;  // legacy single-egid (kept zero — wire reader stays untouched)
    std::vector<int> mods;             // rolled-mod effect_group_ids (one entry → one [...] letter on client)
    bool             oc;               // pick an Overclocked-named blueprint when sending BLUEPRINT_ID
    // v76 cosmetics: non-zero when the equipped row points at a cosmetic
    // item (asm_data_set_items.id) rather than a device. device_id is 0 in
    // that case. SEND_INVENTORY marshaling dispatches on this.
    int              item_id = 0;
};

// Inventory record (account-scoped, profile-filtered). Lives in
// ga_players_inventory; same fields the SEND_INVENTORY pipe needs plus the
// allowed-slots whitelist for equip validation.
struct InventoryRow {
    int              inventory_id;     // = ga_players_inventory.id
    int              profile_id;       // 0 = shared across classes
    int              device_id;
    int              quality;
    std::vector<int> mods;             // mod_effect_group_ids CSV exploded
    bool             oc;
    std::vector<int> allowed_slots;    // slot_value_ids the item may occupy (1 for most, 3 for offhand)
    // v76 cosmetics: when this row is a cosmetic, item_id holds the
    // asm_data_set_items.id and device_id is 0. stock_n is which copy
    // (0 for everything except dyes; dyes have stock_n 0..4 so the same
    // color can be equipped to all 5 dye slots simultaneously).
    int              item_id = 0;
    int              stock_n = 0;
};

struct SkillRow {
    int skill_group_id;
    int skill_id;
    int points;
    int item_profile_id = 0;  // populated by GetAllSkillsForCharacter; 0 from
                              // the single-profile GetSkillsForCharacter (caller
                              // already knows the profile in that path).
};

class PlayerSessionStore {
public:
    static void Init();
    static void Register(const SessionInfo& info);
    static void Unregister(const std::string& guid);
    static std::optional<SessionInfo> GetByGuid(const std::string& guid);
    static std::optional<SessionInfo> GetByIp(const std::string& ip);
    static std::optional<SessionInfo> GetByPlayerName(const std::string& player_name);
    static SessionInfo* GetByGuidPtr(const std::string& guid);

    static int64_t UpsertUser(const std::string& username);
    static int64_t InsertCharacter(int64_t user_id, uint32_t profile_id,
                                   uint32_t head_asm_id, uint32_t gender_type_value_id,
                                   const std::vector<uint8_t>& morph_data,
                                   uint32_t hair_asm_id = 1974,  // NewHair15 / type-850 (0 and 403 both crash at gameplay time)
                                   uint32_t skin_mat_param_id = 0,
                                   uint32_t eye_mat_param_id = 0);
    static std::vector<CharacterInfo> GetCharactersByUserId(int64_t user_id);
    static std::optional<CharacterInfo> GetCharacterById(int64_t id);
    static void SetSelectedCharacter(const std::string& guid, int64_t char_id, uint32_t profile_id);

    // Walk every profile's ClassLoadouts entry and INSERT any (user_id,
    // profile_id, device_id, mods_csv, quality, oc) tuple that doesn't
    // already exist in ga_players_inventory. Idempotent — existing rows are
    // never touched, so a player keeps their `inventory_id`s stable across
    // logins and we get incremental seeding for free (new ClassLoadouts
    // entries propagate to existing players on their next login).
    //
    // Skips slot_value_id == SVID_CLASS_DEVICE (502): the class device is
    // not player-equippable, it's auto-attached at spawn by SpawnPlayerCharacter
    // via asm_data_set_bots_data_set_bot_devices.
    //
    // Does not equip normal gameplay devices. Those stay in the bag until the
    // player equips them; hidden class state is pinned separately.
    static void SeedInventoryFromLoadouts(int64_t user_id);

    // v76 cosmetic seed. Idempotent — runs every login. Inserts one row per
    // owned cosmetic (helmet/flair/suit/trail) and five rows per dye (so the
    // same dye can be set across all 5 dye slots simultaneously).
    // Helmets/suits get profile_id = class id via asm_data_set_items.skill_id
    // mapping; flairs/dyes/trails get profile_id = 0 (shared).
    //
    // Safe to call multiple times per user — the partial unique index
    // idx_ga_players_inventory_cosmetic_uniq dedupes.
    static void SeedCosmetics(int64_t user_id);

    // v76 per-character "fill empty cosmetic slots" seed. INSERT OR IGNORE
    // against ga_character_devices using CosmeticLoadouts defaults for the
    // character's profile. Existing player choices win; only consulted for
    // slots the character hasn't yet customized. Safe to call per login.
    static void SeedCharacterCosmeticDefaults(int64_t character_id, uint32_t profile_id);

    // Armor seed. Inserts 35 armor inventory rows per user — 7 group-126
    // slots × 5 mod variants (see ArmorLoadouts.hpp). Idempotent via the
    // existing (user_id, item_id, stock_n) unique index. Armor pieces are
    // shared across all class profiles (profile_id = 0), so seeding doesn't
    // need to know which classes the user has rolled.
    static void SeedArmor(int64_t user_id);

    // Per-character armor equip default. INSERT OR IGNORE against
    // ga_character_devices for the 7 armor slots — picks the variant at
    // ArmorLoadouts::kDefaultVariantIndex ([rrrrrr] by default) for each
    // slot. Existing player choices win. Safe to call per login.
    static void SeedCharacterArmorDefaults(int64_t character_id);

    // Slot 14 (CLASS_DEVICE / HUMAN BASE ATTRIBUTES) is server-pinned across
    // all 5 loadout profiles. The client UI hides the slot and the player
    // can't equip or replace it, so the row MUST exist in
    // ga_character_devices before the first SEND_INVENTORY or the client
    // hits the resubmit loop on the REST device until the next session.
    // INSERT OR IGNORE per profile (1..5) so player-modified rows are never
    // touched. Safe to call per login.
    static void PinClassDeviceSlot14(int64_t character_id);

    // Remove every ga_players_inventory row owned by this user whose item_id
    // is in ga_item_blacklist, along with any ga_character_devices rows that
    // reference those inventory rows. Runs once per GSC_USER_LOGIN after
    // UpsertUser. Idempotent. Logs each purged (character_id, item_id, slot)
    // tuple on the "blacklist" channel.
    static void PurgeBlacklistedItems(int64_t user_id);

    // Returns everything in this user's account-scoped inventory pool that's
    // visible to a character on `profile_id`. Includes shared items
    // (profile_id=0) plus class-specific items. Used by SEND_INVENTORY to
    // render the unequipped pool in the equip screen.
    static std::vector<InventoryRow> GetInventoryForUser(int64_t user_id, uint32_t profile_id);

    // Returns the equipped devices for this character — JOIN of
    // ga_character_devices and ga_players_inventory. Has the same DeviceRow
    // shape SEND_INVENTORY consumes today; equip_slot is taken from the
    // character_devices row, the rest from the inventory row.
    // Profile-scoped: returns rows for (character_id, item_profile_id) only.
    // Passes the active loadout slot (1..5). For full per-profile slot data
    // see GetPerProfileSlotMap.
    static std::vector<DeviceRow> GetDevicesForCharacter(int64_t character_id,
                                                        int item_profile_id);

    // Per-profile slot map. Returns { inventory_id ->
    // [_, slot_in_p1, slot_in_p2, ..., slot_in_p5] } where slot_in_pN is 0
    // if the item isn't equipped in profile N. The stored value is the raw
    // equipped_slot (engine equip-point 1..24 for devices, group-129 SVID
    // 1130..1143 for armor) — caller resolves to slot_value_id via
    // EquipPointToSvid/IsArmorSvid. Used by send_inventory_response to ship
    // correct DATA_SET_CHARACTER_PROFILES rows per item.
    static std::map<int, std::array<int,6>>
        GetPerProfileSlotMap(int64_t character_id);

    // Active loadout slot (1..5) for a character. DEFAULT 1 if missing or
    // out of range. Persisted in ga_characters.current_item_profile_id.
    static int  GetCurrentItemProfile(int64_t character_id);
    static void SetCurrentItemProfile(int64_t character_id, int item_profile_id);

    // Validates and persists an equip-screen save. `slot_to_inventory` is the
    // client's SlotIndices[] decoded into (equip_point → inventory_id) for the
    // non-zero entries only. Returns true and replaces ga_character_devices
    // rows in one transaction when EVERY mapping passes:
    //   - inventory_id exists in ga_players_inventory AND user_id matches
    //   - inventory.profile_id is 0 OR equals current profile_id
    //   - the equip_point's canonical slot_value_id is in inventory.allowed_slots
    // On any failure, the whole save is rejected (no partial writes) and the
    // reason is logged. Callers should then re-send SEND_INVENTORY to resync
    // the client's view to what's actually persisted.
    // `slot_to_inventory` = client's FTGEQUIP_SLOTS_STRUCT.SlotIndices[]
    //                       (engine equip-points 1..24 for weapons + cosmetics).
    // `misc_items`         = client's FTGEQUIP_SLOTS_STRUCT.MiscItems[]
    //                       (armor slots, indexed such that slot_value_id =
    //                       index + 1128 → group-129 SVID). Empty if the
    //                       client didn't change armor. See ArmorSlot
    //                       constants in src/GameServer/Constants/EquipSlot.hpp
    //                       for the index↔SVID decode.
    // `item_profile_id` is the loadout slot (1..5) being saved into; ONLY
    // rows in that slot are replaced. Other profiles' equipped state is
    // untouched.
    static bool SaveEquippedDevices(int64_t character_id, int64_t user_id,
                                    uint32_t profile_id,
                                    int item_profile_id,
                                    const std::map<int, int>& slot_to_inventory,
                                    const std::map<int, int>& misc_items);

    // UNUSED — kept for reference. Returns the union of every static effect
    // group attached to a device in asm.dat (equip + per-fire-mode). We used
    // to send these via DATA_SET_INVENTORY_STATE alongside rolled mods, but
    // that leaked built-in props into the [...] letter suffix on the client
    // (e.g. a built-in +25% Mech Damage equip-effect rendered as a stray 'm').
    // The pipe is now rolled-mods-only; built-in effects are applied
    // server-side via Inventory::ApplyDeviceEquipEffects and read on the
    // client from its own asm.dat (Device.m_EquipEffect / m_pFireModeSetup).
    // See TcpSession::send_inventory_response for the current shape.
    static std::vector<int> GetEffectGroupIds(int device_id);

    // Returns the lowest blueprint_id from asm_data_set_blueprints whose
    // created_item_id matches the given device id, or 0 if none exists.
    // The client gates the [...] mod-letter suffix on m_pAmBlueprint != null
    // (FUN_10a12740 sets it from FInventoryData.nBlueprintId). Items that
    // ship rolled mods need a non-zero BLUEPRINT_ID for the suffix to render.
    //
    // When `oc` is true, prefers a blueprint with override_name_msg_id != 0
    // (Overclocked-named variant — e.g. 6424 = "Focused Repair Arm OC" for
    // device 2918). Falls back to the standard MIN(blueprint_id) if no OC
    // variant exists for the device. Result is cached per (device_id, oc).
    static int GetBlueprintIdForDevice(int device_id, bool oc = false);

    // Skill tree allocations — ga_character_skills. Profile-scoped.
    static std::vector<SkillRow> GetSkillsForCharacter(int64_t character_id,
                                                     int item_profile_id);
    // Returns every profile's skill rows in one shot, each tagged with its
    // item_profile_id. Used by SEND_PLAYER_SKILLS to ship a full picture so
    // the client's skill-tree widget can re-filter on r_nItemProfileId change
    // without needing a fresh server push (r_nItemProfileId has no UC
    // repnotify, so no client-side refresh callback fires on switch — the
    // widget blanks until close+reopen if it can only see active-profile
    // rows).
    static std::vector<SkillRow> GetAllSkillsForCharacter(int64_t character_id);
    // Replaces ONE profile's allocation in a transaction. `last_respec_at`
    // on ga_characters is bumped (per-character, not per-profile).
    static void SetSkillsForCharacter(int64_t character_id, int item_profile_id,
                                     const std::vector<SkillRow>& skills);
    // Clears one profile's rows and bumps last_respec_at.
    static void ClearSkillsForCharacter(int64_t character_id, int item_profile_id);
    // Epoch seconds of the last save/respec, 0 if never set.
    static int64_t GetLastRespecAt(int64_t character_id);

    static QuestStore& Quests() {
        static QuestStore instance;
        return instance;
    }

private:
    static std::mutex mutex_;
    static std::map<std::string, SessionInfo> by_guid_;
    static std::map<std::string, SessionInfo> by_ip_;

    static int GetEffectGroupId(int device_id);
};
