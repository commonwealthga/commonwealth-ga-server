#pragma once

#include <string>
#include <optional>
#include <map>
#include <vector>
#include <cstdint>
#include <windows.h>

struct SkillAllocation {
	int skill_group_id = 0;
	int skill_id = 0;
	int points = 0;
};

struct PlayerInfo {
	std::string session_guid; // 32-char lowercase hex
	std::string player_name;
	std::string ip_address;
	std::wstring player_name_w;
	int64_t user_id = 0;
	int64_t selected_character_id = 0;
	uint32_t selected_profile_id = 0;
	int task_force = 1;
	// Active loadout slot (1..5). Mirrors
	// ga_characters.current_item_profile_id. Updated by the switch-profile
	// hook (TgPlayerController::ServerLoadItemProfile). Drives the SpawnPlayerCharacter
	// ga_character_devices query and the RCST armor + skill apply paths.
	int current_item_profile_id = 1;
	// Sent by the control server in PLAYER_REGISTER; consumed by
	// ReapplyCharacterSkillTree to build s_SkillBasedEffectGroups.
	// Scoped to current_item_profile_id (the active loadout's skills).
	std::vector<SkillAllocation> skills;
	int64_t last_respec_at = 0;
};

struct CharacterInfo {
	int64_t id = 0;
	int64_t user_id = 0;
	uint32_t profile_id = 0;
	uint32_t head_asm_id = 0;
	uint32_t gender_type_value_id = 0;
	std::vector<uint8_t> morph_data;
	// Persisted appearance fields the head menu collects (added in DB v81-v83).
	// Consumed by SpawnPlayerCharacter to fill the spawned pawn's
	// r_CustomCharacterAssembly struct — without that, HairMeshId defaults to
	// 0 in UC and the client's pawn-asm-attach pipeline (FUN_109d1860) crashes
	// at 0x109d1f5b. 1974 = "NewHair15" (asm_mesh_type_value_id=850), the
	// in-game pawn hair category — matches what SpawnBotPawn sets on bots and
	// is confirmed to load at gameplay time.
	uint32_t hair_asm_id = 1974;
	uint32_t skin_mat_param_id = 0;
	uint32_t eye_mat_param_id = 0;
};

class PlayerRegistry {
public:
	static void Init();
	static void Register(const PlayerInfo& info);
	static void Unregister(const std::string& session_guid);
	static std::optional<PlayerInfo> GetByGuid(const std::string& guid);
	static std::optional<PlayerInfo> GetByIp(const std::string& ip);
	// Returns a stable pointer into the registry map (valid until Unregister is called).
	static PlayerInfo* GetByGuidPtr(const std::string& guid);

	// Returns the ga_users.id for the given username, inserting if not present.
	static int64_t UpsertUser(const std::string& username);

	// Inserts a new character row and returns its id.
	static int64_t InsertCharacter(int64_t user_id, uint32_t profile_id,
	                               uint32_t head_asm_id, uint32_t gender_type_value_id,
	                               const std::vector<uint8_t>& morph_data);

	// Returns all characters for a user.
	static std::vector<CharacterInfo> GetCharactersByUserId(int64_t user_id);

	// Returns a single character by its DB id.
	static std::optional<CharacterInfo> GetCharacterById(int64_t id);

	// Updates selected_character_id / selected_profile_id in the in-memory registry.
	static void SetSelectedCharacter(const std::string& session_guid, int64_t char_id, uint32_t profile_id);

	// Re-reads ga_character_skills for (selected_character_id, item_profile_id)
	// into PlayerInfo->skills, replacing the existing vector. Used by the
	// loadout-profile switch hook so RCST's PHASE 2 iterates the right
	// allocation when the player changes profiles at runtime. Returns the
	// number of rows loaded (0 if no PlayerInfo or no rows).
	static int RefreshSkillsForProfile(const std::string& session_guid,
	                                   int item_profile_id);

private:
	static std::map<std::string, PlayerInfo> by_guid_;
	static std::map<std::string, PlayerInfo> by_ip_;
	static CRITICAL_SECTION cs_;
	static bool cs_initialized_;
};
