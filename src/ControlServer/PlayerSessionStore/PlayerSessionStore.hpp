#pragma once

#include <string>
#include <optional>
#include <map>
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
};

struct DeviceRow {
    int              device_id;
    int              equip_slot;
    int              slot_value_id;
    int              quality;
    int              inventory_id;
    int              effect_group_id;  // legacy single-egid (still written for back-compat); unused on read
    std::vector<int> mods;             // rolled-mod effect_group_ids (one entry → one [...] letter on client)
};

struct SkillRow {
    int skill_group_id;
    int skill_id;
    int points;
};

class PlayerSessionStore {
public:
    static void Init();
    static void Register(const SessionInfo& info);
    static void Unregister(const std::string& guid);
    static std::optional<SessionInfo> GetByGuid(const std::string& guid);
    static std::optional<SessionInfo> GetByIp(const std::string& ip);
    static SessionInfo* GetByGuidPtr(const std::string& guid);

    static int64_t UpsertUser(const std::string& username);
    static int64_t InsertCharacter(int64_t user_id, uint32_t profile_id,
                                   uint32_t head_asm_id, uint32_t gender_type_value_id,
                                   const std::vector<uint8_t>& morph_data);
    static std::vector<CharacterInfo> GetCharactersByUserId(int64_t user_id);
    static std::optional<CharacterInfo> GetCharacterById(int64_t id);
    static void SetSelectedCharacter(const std::string& guid, int64_t char_id, uint32_t profile_id);

    // Wipes ga_character_devices for this character and re-inserts rows from
    // Loadouts::GetLoadout(profile_id). MUST be called before the game DLL's
    // SpawnPlayerCharacter reads the table — i.e. at character-SELECT time,
    // not later — otherwise the in-engine pawn keys devices by stale
    // inventory_ids while the client gets the resynced ones.
    static void ResyncCharacterDevicesFromLoadout(int64_t character_id, uint32_t profile_id);

    // Returns whatever is currently in ga_character_devices for this character.
    // No side-effects; the resync is gated to character-SELECT (above).
    static std::vector<DeviceRow> GetDevicesForCharacter(int64_t character_id);

    // Returns every effect group that should be reported in DATA_SET_INVENTORY_STATE
    // for a given equipped device. Client (FUN_10a13820 @ 0x10a13820) accumulates
    // these into the inventory object's effect list, which drives TgDeviceFire::
    // GetEffectGroup runtime lookups. Queried from asm_data_set_device_effect_groups
    // + asm_data_set_device_mode_effect_groups; falls back to the legacy hardcoded
    // map when the asm tables are empty (pre-capture runs).
    static std::vector<int> GetEffectGroupIds(int device_id);

    // Returns the lowest blueprint_id from asm_data_set_blueprints whose
    // created_item_id matches the given device id, or 0 if none exists.
    // The client gates the [...] mod-letter suffix on m_pAmBlueprint != null
    // (FUN_10a12740 sets it from FInventoryData.nBlueprintId). Items that
    // ship rolled mods need a non-zero BLUEPRINT_ID for the suffix to render.
    // Result is cached per-device for the lifetime of the process.
    static int GetBlueprintIdForDevice(int device_id);

    // Skill tree allocations — ga_character_skills.
    static std::vector<SkillRow> GetSkillsForCharacter(int64_t character_id);
    // Replaces the full allocation in one transaction. `last_respec_at` on
    // ga_characters is bumped to now.
    static void SetSkillsForCharacter(int64_t character_id, const std::vector<SkillRow>& skills);
    // Clears every row for this character and bumps last_respec_at.
    static void ClearSkillsForCharacter(int64_t character_id);
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
