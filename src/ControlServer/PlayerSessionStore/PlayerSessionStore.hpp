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
    bool             oc;               // pick an Overclocked-named blueprint when sending BLUEPRINT_ID
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
