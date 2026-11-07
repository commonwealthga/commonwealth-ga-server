#pragma once
// Cyber Cuts (hair) + Genolab Facial Reconstruction (face),
// Dome City shops from patch 1.3, tokens since 1.35 (3,000 tokens each).
// The client's own screens (TgUIUpdateHairMenu / TgUIUpdateFaceMenu, opened by
// the shop's UI volume 140 / 177 -> ui scene 6316 / 6474) send TCP 0x01D3
// UPDATE_CHAR_VISUAL_SETTINGS {CURRENCY_TYPE_VALUE_ID, PRICE, LOOT_TABLE_ID,
// LOOT_TABLE_ITEM_ID, ITEM_ID (6019 Cyber Cut / 6128 Genolab virtual item),
// HAIR_ASM_ID, DWORDS = (morph node index, weight) pairs}. The control server
// checks the shop row + the game's own price, takes the tokens and stores the
// new hair / morphs on the agent (shown from the next map load).
#include <cstdint>
#include <string>
#include <vector>

namespace Appearance {

struct Request {
    uint32_t currency = 0, price = 0, loot_table_id = 0, loot_table_item_id = 0, item_id = 0;
    uint32_t hair_asm_id = 0, head_asm_id = 0;
    std::vector<uint8_t> morph;   // raw (index, weight) uint32 pairs, length prefix stripped
};
struct Result { bool ok = false; int price = 0; int currency = 0; std::string reason; };

Result Apply(int64_t character_id, const Request& req);

}  // namespace Appearance
