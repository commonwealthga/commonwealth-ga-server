#pragma once

#include <string>
#include <optional>
#include <map>

struct PlayerInfo {
	std::string session_guid; // 32-char lowercase hex
	std::string player_name;
	std::string ip_address;
	std::wstring player_name_w;
};

class PlayerRegistry {
public:
	static void Init();
	static void Register(const PlayerInfo& info);
	static void Unregister(const std::string& session_guid);
	static std::optional<PlayerInfo> GetByGuid(const std::string& guid);
	static std::optional<PlayerInfo> GetByIp(const std::string& ip);

private:
	static std::map<std::string, PlayerInfo> by_guid_;
	static std::map<std::string, PlayerInfo> by_ip_;
};
