#pragma once
// Botted Auction House.
// A few market bots keep the AH alive with few players: they LIST items of
// every AH category (components, blueprints, consumables, dyes, mods, tradeable
// gear, flair, repair kits) from the game's own item table, a few hundred at a
// time, and BUY player listings that are priced sanely. Every price is anchored to the
// item's vendor credit price (asm_data_set_prices, currency 1603), so:
//   - bots sell at markup % of the vendor price (not below the cheapest player
//     listing of the same item), start bid 80 %, 24 h;
//   - bots buy at <= buy_percent % of the vendor price;
//   - caps per item and per seller per day, slow restock per tick.
// An optional "market_tick" service (ServiceHooks) may set listing prices and
// pick which candidates to buy, within the server limits (see MarketBots.cpp).
// Bot listings have a negative seller id (-bot id, names in ga_market_bots);
// credits a bot earns vanish (sink), items a bot buys are destroyed (sink).
#include <cstdint>
#include <vector>

namespace MarketBots {

struct Settings {
    bool enabled             = true;
    int  tick_seconds        = 300;   // one restock + buy round
    int  target_listings     = 700;   // bot listings kept on the AH
    int  restock_per_tick    = 50;
    int  max_per_item        = 3;     // active bot listings of one item
    int  markup              = 125;   // % of the vendor price
    bool buy_enabled         = true;
    int  buy_percent         = 75;    // max buyout (per unit) as % of the vendor price
    int  buy_percent_fallback = 50;   // same, when no market_tick service decides the buys
    int  buy_min_age_minutes = 30;    // a player listing must sit this long first
    int  buy_per_tick        = 3;
    int  bid_per_tick        = 3;     // max bot bids a market_tick decision may place per round (no service = no bids)
    int  cancel_per_tick     = 10;    // max bot listings a market_tick decision may withdraw per round
    int  buy_daily_item_cap  = 5;     // bot buys of one item per 24 h (all sellers)
    int64_t buy_daily_seller_credits = 20000;  // credits one seller can get from bots per 24 h
};

void Configure(const Settings& s);
const Settings& Get();
void EnsureSchema();

struct TickResult {
    int listed = 0;
    int bought = 0;
    int cancelled = 0;
    int bid = 0;
    std::vector<int64_t> mailed;   // characters that got AH mail (notify if online)
};
// One round: settle expired listings, restock bot listings, buy player listings.
TickResult Tick();

} // namespace MarketBots
