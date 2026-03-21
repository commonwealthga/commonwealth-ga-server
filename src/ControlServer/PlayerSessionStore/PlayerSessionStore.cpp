// Placeholder — will be replaced by Task 3
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Logger.hpp"

std::mutex PlayerSessionStore::mutex_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_guid_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_ip_;

void PlayerSessionStore::Init() {}
void PlayerSessionStore::Register(const SessionInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    by_guid_[info.session_guid] = info;
    by_ip_[info.ip_address] = info;
}
void PlayerSessionStore::Unregister(const std::string& guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_guid_.find(guid);
    if (it != by_guid_.end()) {
        by_ip_.erase(it->second.ip_address);
        by_guid_.erase(it);
    }
}
std::optional<SessionInfo> PlayerSessionStore::GetByGuid(const std::string& guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_guid_.find(guid);
    if (it != by_guid_.end()) return it->second;
    return std::nullopt;
}
std::optional<SessionInfo> PlayerSessionStore::GetByIp(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_ip_.find(ip);
    if (it != by_ip_.end()) return it->second;
    return std::nullopt;
}
SessionInfo* PlayerSessionStore::GetByGuidPtr(const std::string& guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_guid_.find(guid);
    if (it != by_guid_.end()) return &it->second;
    return nullptr;
}
int64_t PlayerSessionStore::UpsertUser(const std::string&) { return 0; }
int64_t PlayerSessionStore::InsertCharacter(int64_t, uint32_t, uint32_t, uint32_t, const std::vector<uint8_t>&) { return 0; }
std::vector<CharacterInfo> PlayerSessionStore::GetCharactersByUserId(int64_t) { return {}; }
std::optional<CharacterInfo> PlayerSessionStore::GetCharacterById(int64_t) { return std::nullopt; }
void PlayerSessionStore::SetSelectedCharacter(const std::string&, int64_t, uint32_t) {}
