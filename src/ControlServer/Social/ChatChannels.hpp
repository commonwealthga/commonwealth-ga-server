#pragma once
// Chat channel extras the client has but the server never answered.
// Client behaviour mined from GlobalAgenda.exe:
//  - /chatwho  -> CHAT_LIST_CHANNEL_MEMBERS 0x206 on the CHAT socket, one field CHAT_CH_TYPE = the
//                 channel currently selected in the chat combo box. The client's chat receive path reads
//                 every non-0x72 packet as CHAT_MESSAGE, so the answer is plain chat lines.
//  - /gl text  -> CHAT_MESSAGE on channel 0 ("Global Channel").
//  - /alert t  -> CHAT_MESSAGE on channel 0 + MSG_ID 23083 + DISPLAY_AS_ALERT/ALERT_PRIORITY/ALERT_TYPE/
//                 FLOAT_VALUE(seconds)/TEXT_VALUE = a centre-screen alert for everyone.
// Channel 0 is staff-only here (the -announce list): the client ships "You are not authorized to send
// on this channel." for exactly this case.
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>

namespace Social {
namespace ChatChannels {

constexpr uint16_t kListChannelMembers = 0x0206;   // GA_U::CHAT_LIST_CHANNEL_MEMBERS
constexpr uint32_t kGlobalChannelId    = 0;        // /gl, /alert

// frame = whole chat-socket frame [2B len][2B type][2B count][fields]
void OnListMembers(const std::string& session_guid, const uint8_t* frame, size_t length);

// Channel-0 CHAT_MESSAGE. Always handles it (returns true): staff -> relayed to every
// connected chat session, everyone else -> a "not authorized" system line.
bool OnGlobal(const std::string& session_guid, const std::string& player_name, bool staff,
              const uint8_t* frame, size_t length);

// Zone chat (patch 1.5): City (7) is the zone channel - /c, /1, /z. Recipients = every player
// on the sender's zone: all home-map (Dome) players, all instances of the same open zone map
// (gameplay_type 1554), inside a mission = that instance. Not in any instance = only the sender.
std::unordered_set<std::string> ZoneRecipients(const std::string& sender_guid);

}  // namespace ChatChannels
}  // namespace Social
