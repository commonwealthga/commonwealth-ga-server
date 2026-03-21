#pragma once

#include <string>
#include <optional>
#include <map>
#include <vector>
#include <cstdint>
#include <mutex>

struct PlayerInfo {
	std::string session_guid; // 32-char lowercase hex
	std::string player_name;
	std::string ip_address;
	std::wstring player_name_w;
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

private:
	static std::map<std::string, PlayerInfo> by_guid_;
	static std::map<std::string, PlayerInfo> by_ip_;
	static std::mutex mutex_;
};
