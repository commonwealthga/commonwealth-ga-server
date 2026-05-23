#include "src/ControlServer/MatchmakingService/TicketInfoEncoder.hpp"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"
#include <cstring>

namespace {

// Local TLV writers — duplicate the inline helpers in TcpSession.hpp without
// pulling TcpSession into the encoder's dependency graph. Wire format is
// little-endian throughout.

inline void Tag(std::vector<uint8_t>& b, uint16_t t) {
    b.push_back((uint8_t)(t & 0xFF));
    b.push_back((uint8_t)(t >> 8));
}

inline void W1B(std::vector<uint8_t>& b, uint16_t t, uint8_t v) {
    Tag(b, t); b.push_back(v);
}

inline void W4B(std::vector<uint8_t>& b, uint16_t t, uint32_t v) {
    Tag(b, t);
    b.push_back((uint8_t)(v       & 0xFF));
    b.push_back((uint8_t)((v >>  8) & 0xFF));
    b.push_back((uint8_t)((v >> 16) & 0xFF));
    b.push_back((uint8_t)((v >> 24) & 0xFF));
}

inline void W8B(std::vector<uint8_t>& b, uint16_t t, uint64_t v) {
    Tag(b, t);
    for (int i = 0; i < 8; ++i) b.push_back((uint8_t)((v >> (i * 8)) & 0xFF));
}

inline void WFloat(std::vector<uint8_t>& b, uint16_t t, float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    W4B(b, t, bits);
}

// 3-byte standard flag form `{tag, value, 0x00, 0x79}` matching the legacy
// WriteNBytes pattern in send_get_ticket_info_response. Used for the *_FLAG
// fields where the wire reader treats the second/third bytes as type tags.
inline void WFlag3(std::vector<uint8_t>& b, uint16_t t, bool v) {
    Tag(b, t);
    b.push_back(v ? 0x01 : 0x00);
    b.push_back(0x00);
    b.push_back(0x79);
}

// BONUS_QUEUE_FLAG is sent with a longer payload (11 bytes) in the legacy
// code: `{0x00, 0x08, 0x00, value-bit, 0x00 * 7}`. The bonus bit goes in the
// 4th byte (0x80 = on). Preserving the exact wire shape because the client
// reader's get_flag path is opinionated about payload length here.
inline void WBonusQueueFlag(std::vector<uint8_t>& b, uint16_t t, bool on) {
    Tag(b, t);
    b.push_back(0x00); b.push_back(0x08); b.push_back(0x00);
    b.push_back(on ? 0x80 : 0x00);
    for (int i = 0; i < 7; ++i) b.push_back(0x00);
}

}  // namespace

void TicketInfoEncoder::EncodeRecord(
    std::vector<uint8_t>& out,
    const QueueConfig& cfg,
    const QueueRuntimeStats& stats)
{
    // Field count emitted on the wire. Required fields = 27; optional adds
    // for status_msg_id, access_flags, remaining_seconds when non-default.
    //
    // IMPORTANT: emit actual_count + 1 — the original send_get_ticket_info_
    // response code shipped 0x1c (28) for a 27-field record and the client's
    // record-walking loop depends on that off-by-one. Emitting the honest
    // count makes record N+1 misalign and the client drops every record past
    // the first; only the first card appears in the UI.
    uint16_t actual_count = 27;
    const bool has_status_msg = cfg.status_msg_id != 0;
    const bool has_access     = cfg.access_flags != 0;
    const bool has_remaining  = cfg.remaining_seconds.has_value()
                                && *cfg.remaining_seconds != 0;
    if (has_status_msg) actual_count++;
    if (has_access)     actual_count++;
    if (has_remaining)  actual_count++;
    const uint16_t field_count = actual_count + 1;

    out.push_back((uint8_t)(field_count & 0xFF));
    out.push_back((uint8_t)(field_count >> 8));

    W4B(out, GA_T::QUEUE_TYPE_VALUE_ID,     cfg.queue_type_value_id);
    W4B(out, GA_T::MATCH_QUEUE_ID,          cfg.queue_id);
    if (has_status_msg) W4B(out, GA_T::STATUS_MSG_ID, cfg.status_msg_id);
    W4B(out, GA_T::NAME_MSG_ID,             cfg.name_msg_id);
    W4B(out, GA_T::DESC_MSG_ID,             cfg.desc_msg_id);
    W4B(out, GA_T::ICON_ID,                 cfg.icon_id);
    W4B(out, GA_T::PLAYER_COUNT,            stats.player_count);
    W4B(out, GA_T::MAX_PLAYERS_PER_SIDE,    cfg.max_players_per_side);
    W4B(out, GA_T::MIN_PLAYERS_PER_TEAM,    cfg.min_players_per_team);
    W4B(out, GA_T::MAX_PLAYERS_PER_TEAM,    cfg.max_players_per_team);
    W4B(out, GA_T::INSTANCE_COUNT,          stats.instance_count);
    W4B(out, GA_T::LEVEL_MIN,               cfg.level_min);
    W4B(out, GA_T::LEVEL_MAX,               cfg.level_max);
    W4B(out, GA_T::TAB,                     cfg.tab);
    WFloat(out, GA_T::MAP_X,                cfg.map_x);
    WFloat(out, GA_T::MAP_Y,                cfg.map_y);
    WFlag3(out, GA_T::MAP_ACTIVE_FLAG,      cfg.map_active_flag);
    W4B(out, GA_T::MAP_ICON_TEXTURE_RES_ID, cfg.map_icon_texture_res_id);
    W4B(out, GA_T::VIDEO_RES_ID,            cfg.video_res_id);
    W4B(out, GA_T::LOCATION_VALUE_ID,       cfg.location_value_id);
    WFlag3(out, GA_T::DOUBLE_AGENT_FLAG,    cfg.double_agent_flag);
    W4B(out, GA_T::SYS_SITE_ID,             cfg.sys_site_id);
    W4B(out, GA_T::SORT_ORDER,              cfg.sort_order);
    WBonusQueueFlag(out, GA_T::BONUS_QUEUE_FLAG, cfg.bonus_queue_flag);
    if (has_access)    W8B(out, GA_T::ACCESS_FLAGS, cfg.access_flags);
    W4B(out, GA_T::DIFFICULTY_VALUE_ID,     cfg.difficulty_value_id);
    WFlag3(out, GA_T::ACTIVE_FLAG,          cfg.active_flag);
    if (has_remaining) W4B(out, GA_T::REMAINING_SECONDS, *cfg.remaining_seconds);

    // DATA_SET_PROFILE_COUNTS — per-class breakdown (queued + in-mission).
    // Each non-zero class contributes one inner record {PLAYER_PROFILE_ID,
    // PLAYER_PROFILE_COUNT}. The client reader's switch only knows the four
    // dashboard classes; emitting zeros for them is harmless but wasteful,
    // so we suppress empty rows.
    struct Row { uint32_t profile_id; uint32_t count; };
    const Row rows[] = {
        { 680, stats.assault  },
        { 567, stats.medic    },
        { 681, stats.recon    },
        { 679, stats.robotics },
    };
    uint16_t inner_count = 0;
    for (const auto& r : rows) if (r.count != 0) inner_count++;

    Tag(out, GA_T::DATA_SET_PROFILE_COUNTS);
    out.push_back((uint8_t)(inner_count & 0xFF));
    out.push_back((uint8_t)(inner_count >> 8));
    for (const auto& r : rows) {
        if (r.count == 0) continue;
        // 2 fields per inner record. PLAYER_PROFILE_ID and _COUNT are both
        // TYPE_TCP_UINT32 per TcpTypes.h; the binary's "get_byte" symbol is
        // a misnomer (it dispatches on the registered type and reads 4 bytes
        // — needed because profile IDs like 0x2A8 don't fit in one byte).
        out.push_back(0x02); out.push_back(0x00);
        W4B(out, GA_T::PLAYER_PROFILE_ID,    r.profile_id);
        W4B(out, GA_T::PLAYER_PROFILE_COUNT, r.count);
    }

    W1B(out, GA_T::LOCKED_FLAG, cfg.locked_flag ? 0x01 : 0x00);
}
