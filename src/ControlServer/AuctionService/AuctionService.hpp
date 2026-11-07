#pragma once
// Player-to-player Auction House. The original AH
// lived on Hi-Rez's backend; the client (UTgUIAuction* menus) only sends
// browse/add/bid/buyout/cancel and renders rows. Field layout decoded from
// GlobalAgenda.exe.
// Rules come from the client's own message table (5% bid step 32332, buyout
// required 60234, listing fee 55194); escrow + delivery go through MailService.
#include "src/ControlServer/MailService/MailService.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace AuctionService {

// client message ids (asm_data_set_msg_translations)
constexpr uint32_t MSG_BID_STEP        = 32332; // bid needs to be over 5% higher than the current bid
constexpr uint32_t MSG_BID_OVER_BUYOUT = 32333; // bid is higher than the buyout
constexpr uint32_t MSG_GONE            = 32334; // auction item no longer exists
constexpr uint32_t MSG_NO_CURRENCY     = 32826; // insufficient currency to purchase
constexpr uint32_t MSG_BUY_FAILED      = 32827;
constexpr uint32_t MSG_BID_FAILED      = 32828;
constexpr uint32_t MSG_BAD_BUYOUT      = 33492; // buyout can not be 0 or lower than the min price
constexpr uint32_t MSG_BAD_QUANTITY    = 41227;
constexpr uint32_t MSG_ADD_FAILED      = 52327;
constexpr uint32_t MSG_NO_FEE_CREDITS  = 55193; // not enough credits to place an item

// our numbers (the originals lived server-side and are not archived)
constexpr int     MAX_ROWS        = 50;    // client list cap
constexpr int     MAX_LISTINGS    = 20;    // active listings per agent
constexpr int     FEE_PERCENT     = 4;     // listing deposit of the start price (client const TG_AUCTION_DEPOSIT = "4"), not refunded
constexpr int64_t FEE_MIN         = 1;
constexpr int     CUT_PERCENT     = 5;     // house cut of the sale price
constexpr int64_t MAX_PRICE       = 100000000;

struct Listing {
    int64_t id = 0;
    int64_t seller_character_id = 0;
    int64_t holding_mail_id = 0;
    int     item_id = 0;
    int     name_msg_id = 0;
    int64_t quantity = 1;
    int64_t start_price = 0;
    int64_t buyout_price = 0;
    int64_t bid = 0;
    int64_t bidder_character_id = 0;
    int64_t expire_at = 0;
    std::vector<MailService::MailItem> items;  // the held item (one row)
};

void EnsureSchema();

struct Filter {
    std::vector<std::pair<uint32_t, uint32_t>> types;  // {item type, subtype (0 = any)}
    uint32_t    quality = 0;                          // 0 = any
    std::string name;                                 // substring, case-insensitive
    bool        lowest = false;                       // market view: only the lowest unit price of each item
    int         start_row = 0;                        // requested page (START/END_ROW_NUMBER), end 0 = first MAX_ROWS
    int         end_row = 0;
};
// Market view row (every category but Special Auction): one item + quality at one unit price.
struct MarketRow {
    int     item_id = 0;
    int     quality = 0;
    int     name_msg_id = 0;
    int64_t count = 0;      // units for sale at this price
    int64_t unit = 0;
    std::vector<MailService::MailItem> items;  // a held item of the group, for the bag record
};
std::vector<MarketRow> BrowseMarket(const Filter& f, uint32_t* total);
// Active listings matching the filter, cheapest buyout first (max MAX_ROWS);
// *total = all matches.
std::vector<Listing> Browse(const Filter& f, uint32_t* total);
std::vector<Listing> MySells(int64_t character_id);
std::vector<Listing> MyBids(int64_t character_id);

struct Result {
    bool ok = false;
    uint32_t error_msg_id = 0;
    std::vector<int64_t> removed_rows;    // gear rows that left the seller's bag
    bool components_changed = false;
    std::vector<int> depleted_components;
    std::vector<int64_t> mailed;          // characters that got mail
    int64_t auction_id = 0;
};

// Lists one bag row (or part of a component stack). Fee = FEE_PERCENT of the start price.
// start_price 0 = the client's market sell panel: buyout_price is the UNIT price.
Result Add(int64_t character_id, int64_t inventory_id, int64_t count,
           int64_t start_price, int64_t buyout_price, uint32_t duration_value_id);
// Bid >= start, > 5% over the current bid; bid == buyout -> buyout.
Result Bid(int64_t character_id, int64_t auction_id, int64_t amount);
Result Buyout(int64_t character_id, int64_t auction_id);
// Market view buy: count units of item + quality, never above the unit price the buyer saw, cheapest
// listings first (player listings first on an equal price). Part of a stack splits the listing.
Result BuyMarket(int64_t character_id, int item_id, int quality, int64_t count, int64_t unit);
// Seller only, no bids yet; the item comes back by mail.
Result Cancel(int64_t character_id, int64_t auction_id);
// Settles every expired listing (winner gets the item, seller the bid minus the
// cut; no bid -> item back to the seller). Returns the characters that got mail.
std::vector<int64_t> SettleExpired();

// Market bots (MarketBots): a bot listing of a NEW item (seller id = -bot_id, no fee);
// returns the auction id, 0 = failed. Bot listings that sell pay nobody; unsold ones vanish.
int64_t BotList(int64_t bot_id, int item_id, int64_t quantity, bool component,
                int64_t start_price, int64_t buyout_price, int hours);
// A bot buys a player listing at its buyout (no bids on it): seller gets price - cut, item destroyed.
Result BotBuyout(int64_t auction_id, const std::string& bot_name);
// Withdraw a bot listing (seller < 0, no bid); the held item is discarded like an expired bot row.
bool BotCancel(int64_t auction_id);
// A bot (bidder id = -bot_id) bids on a PLAYER listing with the player rules (>= start, +5 % step,
// < buyout); the outbid player is refunded by mail. Refused when a bot already holds the bid. The bid
// is logged as ga_market_bot_log kind=bid; a refund of a bot bid as kind=bid_refund; a bot that wins at
// expiry pays the seller (price - cut) and the item is discarded (kind=bid_won).
Result BotBid(int64_t auction_id, int64_t bot_id, int64_t amount);

// Duration value id (valid values group 121: 1067/1068/1069) -> hours; unknown -> 24.
int DurationHours(uint32_t duration_value_id);

} // namespace AuctionService
