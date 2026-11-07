#pragma once
// Friends presence, /afk, invite flags, player report, GM gag.
// Original behaviour mined from the client:
//  - friend login/logout = CHAT_MESSAGE with CHAT_ANNOUNCEMENT_FLAG=1, MSG_ID 42225/42226
//    ("@@player_name@@ has come online." / "... has gone offline.") + PLAYER_NAME; the client
//    localises it and drops it when PLAYER_NAME is its own name.
//  - SET/GET_INVITE_FLAGS 0x1A4/0x1A5 carry INVITE_FLAGS (uint32 bit set, Settings > Social).
// All state lives here; ChatSession / TcpSession / PlayerCommands only call in (1-3 line hooks).
#include <asio.hpp>
#include <cstdint>
#include <string>
#include <vector>

class PacketView;

namespace Social {
namespace Friends {

// ---- presence (ChatSession join / teardown) ----
void OnOnline(const std::string& player_name, const asio::any_io_executor& ex);
void OnOffline(const std::string& player_name, const asio::any_io_executor& ex);

// ---- /afk ----
void ToggleAfk(const std::string& session_guid);          // PlayerCommands "afk"
// Client /afk -> game server ServerToggleAFK -> DLL AfkRelay -> IPC GAME_EVENT afk_state.
void SetAfk(const std::string& session_guid, bool afk);
void Activity(const std::string& player_name);            // any chat line clears AFK
bool IsAfk(const std::string& player_name);
// Whisper to an AFK player: auto-reply line for the sender ("" = not AFK).
std::string AfkAutoReply(const std::string& target_name);

// ---- invite flags (Settings) ----
// bit meaning (client 38937..38940): bit0 agency, bit1 alliance, bit2 team-anyone, bit3 strike-force-anyone
constexpr uint32_t kInviteDefault = 0xFFFFFFFFu;            // no row = everything allowed (old behaviour)
void SetInviteFlags(int64_t user_id, uint32_t flags);
uint32_t GetInviteFlags(int64_t user_id);
void HandleSetInviteFlags(int64_t user_id, const PacketView& pkt);
// The shipped client has no working Settings path for invite flags (SET_INVITE_FLAGS sender is dead code),
// so players set them with  -invites [team|agency|alliance|strikeforce|all] [on|off]  (PlayerCommands).
void InvitesCommand(const std::string& session_guid, const std::string& args);
std::vector<uint8_t> BuildInviteFlagsReply(int64_t user_id);   // GET_INVITE_FLAGS answer (no chunk header)
// false = invitee has turned this kind of invite off and the inviter is not a friend / same agency.
enum class InviteKind { Team, Agency, Alliance, StrikeForce };
bool InviteAllowed(const std::string& invitee_name, const std::string& inviter_name, InviteKind kind);

// ---- report / gag ----
void HandlePlayerReported(const std::string& session_guid, const std::string& reporter_name, const PacketView& pkt);
void HandleGag(int64_t gm_user_id, const std::string& gm_name, const PacketView& pkt);
bool Gag(const std::string& target_name, int minutes, const std::string& by);   // false = no such player
// true + reply line when this player's chat is gagged right now.
bool IsGagged(const std::string& player_name, std::string* reply);
bool IsGagged(const std::string& player_name, std::string* reply);
// Chat "-gag name [minutes]" / "-ungag name" (gm/dev role only; GAG_PLAYER has no client sender).
void GagCommand(const std::string& session_guid, const std::string& args, bool ungag);

// one-shot log helper for FRIEND_GET_LIST / FRIEND_UPDATE dispatch
void LogFriendPacket(const char* what, int64_t user_id, const std::string& player_name);

} // namespace Friends
} // namespace Social
