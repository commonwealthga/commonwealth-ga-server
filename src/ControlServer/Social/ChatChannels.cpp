// See ChatChannels.hpp.
#include "src/ControlServer/Social/ChatChannels.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/TcpSession/PacketView.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include <algorithm>
#include <vector>

namespace Social {
namespace ChatChannels {

namespace {

constexpr size_t kMaxNamesShown = 40;

const char* ChannelName(uint32_t ch) {
    switch (ch) {
        case 0:  return "Global";
        case 1:  return "Instance";
        case 2:  return "Agency";
        case 3:  return "Alliance";
        case 4:  return "Local";
        case 5:  return "Team";
        case 6:  return "Raid";
        case 7:  return "City";
        case 8:  return "Personal";
        case 12: return "Trade";
        case 13: return "LFG";
        default: return "this";
    }
}

// [2B len][2B type][2B count][CHAT_CH_TYPE ch][MESSAGE text] - same layout as ChatSession's BuildChatFrame.
std::vector<uint8_t> ChatLine(uint32_t channel, const std::string& text) {
    std::vector<uint8_t> b;
    auto u16 = [&](uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); };
    u16(GA_U::CHAT_MESSAGE); u16(2);
    u16(GA_T::CHAT_CH_TYPE);
    for (int i = 0; i < 4; i++) b.push_back((channel >> (8 * i)) & 0xFF);
    const std::string t = text.size() > 1000 ? text.substr(0, 1000) : text;
    u16(GA_T::MESSAGE); u16(static_cast<uint16_t>(t.size()));
    b.insert(b.end(), t.begin(), t.end());
    std::vector<uint8_t> out;
    out.push_back(b.size() & 0xFF); out.push_back((b.size() >> 8) & 0xFF);
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

size_t ToEveryone(const std::vector<uint8_t>& frame) {
    size_t n = 0;
    for (const auto& name : ChatSession::OnlinePlayerNames())
        if (ChatSession::DeliverToPlayer(name, frame)) n++;
    return n;
}

}  // namespace

void OnListMembers(const std::string& session_guid, const uint8_t* frame, size_t length) {
    if (session_guid.empty() || length < 6) return;
    PacketView pkt(frame + 6, length - 6);
    const uint32_t ch = pkt.Read4B(GA_T::CHAT_CH_TYPE).value_or(7);

    std::vector<std::string> names;
    bool usable = true;
    if (ch == kGlobalChannelId || ch == 8) {
        // Global is a staff broadcast and Personal is 1:1 - neither has a member list.
        usable = false;
    } else {
        const auto scope = ChatSession::ChannelScope(ch, session_guid);
        if (!scope) {
            names = ChatSession::OnlinePlayerNames();            // City / Trade / LFG: everyone
        } else if (scope->empty()) {
            usable = false;                                      // no team / agency / instance
        } else {
            for (const auto& g : *scope)
                if (auto info = PlayerSessionStore::GetByGuid(g))
                    if (!info->player_name.empty()) names.push_back(info->player_name);
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
        }
    }

    std::string line;
    if (!usable) {
        line = std::string("*** No member list for the ") + ChannelName(ch) + " channel ***";
    } else {
        line = std::string("*** ") + ChannelName(ch) + " channel: " + std::to_string(names.size())
             + (names.size() == 1 ? " player: " : " players: ");
        for (size_t i = 0; i < names.size() && i < kMaxNamesShown; i++) {
            if (i) line += ", ";
            line += names[i];
        }
        if (names.size() > kMaxNamesShown)
            line += " ... and " + std::to_string(names.size() - kMaxNamesShown) + " more";
        line += " ***";
    }
    Logger::Log("chat", "[ChatWho] guid=%s ch=%u members=%zu usable=%d\n",
        session_guid.c_str(), ch, names.size(), usable ? 1 : 0);
    ChatSession::SystemMessageToGuid(session_guid, line);
}

bool OnGlobal(const std::string& session_guid, const std::string& player_name, bool staff,
              const uint8_t* frame, size_t length) {
    if (length < 6) return true;
    PacketView pkt(frame + 6, length - 6);
    const bool alert = pkt.Exists(GA_T::DISPLAY_AS_ALERT) || pkt.Exists(GA_T::TEXT_VALUE);
    const std::string text = alert ? pkt.ReadString(GA_T::TEXT_VALUE).value_or("")
                                   : pkt.ReadString(GA_T::MESSAGE).value_or("");
    if (!staff) {
        Logger::Log("chat", "[Global] denied player='%s' kind=%s\n", player_name.c_str(), alert ? "alert" : "gl");
        ChatSession::SystemMessageToGuid(session_guid, "You are not authorized to send on this channel.");
        return true;
    }
    if (text.empty()) return true;
    size_t n = 0;
    if (alert) {
        // Relay the client's own alert frame unchanged: it already carries everything the
        // receiving client needs to show the centre-screen alert.
        n = ToEveryone(std::vector<uint8_t>(frame, frame + length));
    } else {
        n = ToEveryone(ChatLine(kGlobalChannelId, player_name + ": " + text));
    }
    Logger::Log("chat", "[Global] player='%s' kind=%s text='%s' delivered=%zu\n",
        player_name.c_str(), alert ? "alert" : "gl", text.c_str(), n);
    return true;
}

std::unordered_set<std::string> ZoneRecipients(const std::string& sender_guid) {
    std::unordered_set<std::string> out;
    out.insert(sender_guid);
    const auto rows = InstanceRegistry::GetActiveSearchablePlayers();
    const InstanceRegistry::SearchablePlayerRow* me = nullptr;
    for (const auto& r : rows) if (r.session_guid == sender_guid) { me = &r; break; }
    if (!me) {
        Logger::Log("chat", "[Zone] sender %s in no instance -> only self\n", sender_guid.c_str());
        return out;
    }
    const bool home = !me->in_mission;
    std::optional<MapGameRecord> mgi;
    if (!home) mgi = MapGameInfo::LookupByName(me->map_name);
    const bool open_zone = mgi && mgi->gameplay_type_value_id == MapGameInfo::kGameplayTypeOpenZone;
    if (home || open_zone) {
        for (const auto& r : rows) {
            if (r.session_guid.empty()) continue;
            if (home ? !r.in_mission : (r.in_mission && r.map_name == me->map_name)) out.insert(r.session_guid);
        }
    } else if (auto where = InstanceRegistry::GetInstancePlayerTaskForce(sender_guid)) {   // mission: that instance
        for (const auto& r : InstanceRegistry::GetActivePlayersForInstance(where->first))
            if (!r.guid.empty()) out.insert(r.guid);
    }
    Logger::Log("chat", "[Zone] ch=7 map=%s kind=%s recipients=%zu\n", me->map_name.c_str(),
        home ? "home" : (open_zone ? "open_zone" : "mission"), out.size());
    return out;
}

}  // namespace ChatChannels
}  // namespace Social
