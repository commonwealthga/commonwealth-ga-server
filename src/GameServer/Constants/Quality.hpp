#pragma once

namespace GA {

// ---------------------------------------------------------------------------
// Quality — item quality value IDs
//
// Values from TgObject.uc:
//   ITEM_QUALITY_EPIC=1162, ITEM_QUALITY_RARE=1163,
//   ITEM_QUALITY_UNCOMMON=1164, ITEM_QUALITY_COMMON=1165
//
// These are quality_value_id values used in inventory/device records.
// ---------------------------------------------------------------------------
namespace Quality {

    constexpr int None     = 0;
    constexpr int Common   = 1165;
    constexpr int Uncommon = 1164;
    constexpr int Rare     = 1163;
    constexpr int Epic     = 1162;

} // namespace Quality

} // namespace GA
