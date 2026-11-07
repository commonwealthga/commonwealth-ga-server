#include "src/ControlServer/MarketBots/MarketBots.hpp"
#include "src/ControlServer/AuctionService/AuctionService.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <climits>
#include <map>
#include <random>
#include <string>

namespace MarketBots {

namespace {

Settings g_settings;
std::mt19937 g_rng{std::random_device{}()};

struct Stmt {
    sqlite3_stmt* s = nullptr;
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("mail", "[Market] prepare failed: %s (%s)\n", sqlite3_errmsg(db), sql);
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    explicit operator bool() const { return s != nullptr; }
};

int64_t Scalar(sqlite3* db, const char* sql, std::initializer_list<int64_t> binds) {
    Stmt st(db, sql);
    if (!st) return 0;
    int k = 1;
    for (int64_t v : binds) sqlite3_bind_int64(st.s, k++, v);
    return sqlite3_step(st.s) == SQLITE_ROW ? sqlite3_column_int64(st.s, 0) : 0;
}

int Rand(int lo, int hi) { return std::uniform_int_distribution<int>(lo, std::max(lo, hi))(g_rng); }

// Agent-style handles (GA names are one word). Shown only in AH mail bodies.
const char* kBotNames[] = {
    "Kestrelle", "Brannoc", "Vexa", "Talloway", "Quorrin", "Mirelle",
    "Dashkov", "Ozmund", "Sablewyn", "Halvard", "Pyrra", "Corvane",
};
constexpr int kBotCount = sizeof(kBotNames) / sizeof(kBotNames[0]);

// Sellable catalog (every AH category the client browses). Only items
// with a vendor CREDIT price (asm_data_set_prices 1603) - so nothing AP-only -
// not bound on acquire / to agency (1291/1472), not seasonal/AvA award flair,
// no z-prefixed dev items. Groups and their share of the bot listings:
//   components (stack), weapon / armor / other blueprints, consumables, dyes,
//   weapon + armor mods, finished gear, flair, repair kits.
// Finished gear = the Binds-When-Equipped (1290) drop weapons and jetpacks
// (slots ranged 388, melee 389, jetpack 806, specialty 981), tradeable until
// equipped as in the original; anchor price = its own vendor price or the
// price of the blueprint that makes it, whichever is higher. Crafted armor is
// not listed: its stats are rolled by the craft, the item rows are only bases.
// Blueprints need a plain vendor/loot row: ones that only exist with auto_create_flag = 1
// ("Buy Melee - ...", "Specialty Drop - ...") hand out the finished item and are never held by a player.
// Repair kits have no credit price: half the credit price of their blueprint.
// AvA deployables (1298): own price or their blueprint's. Crafted pets (1535, unbound, none sold for AP) have no
// price at all: 2000, what the vendor pets cost in credits.
struct Item { int item_id; int type; int64_t price; int level; };
struct Group { const char* name; int weight; std::vector<Item> items; };
std::vector<Group> g_groups = {
    {"components", 18, {}}, {"weapon blueprints", 1, {}}, {"armor blueprints", 16, {}},
    {"other blueprints", 6, {}}, {"consumables", 5, {}}, {"dyes", 24, {}}, {"mods", 6, {}},
    {"gear", 8, {}}, {"flair", 1, {}}, {"repair kits", 2, {}}, {"ava", 6, {}}, {"pets", 2, {}},
};
std::map<int, size_t> g_group_of;   // item_id -> group index
bool g_loaded = false;

int GroupFor(int type, int subtype) {
    switch (type) {
    case 1171: return 0;
    case 686:  return subtype == 1417 ? 1 : subtype == 1345 ? 2 : 3;
    case 1412: return 4;
    case 1020: return 5;
    case 1434: case 1441: return 6;
    case 217:  return 7;
    case 950:  return 8;
    case 1369: return 9;
    case 1298: return 10;
    case 1535: return 11;
    default:   return -1;
    }
}

void LoadCatalog(sqlite3* db) {
    if (g_loaded) return;
    g_loaded = true;
    Stmt st(db,
        "SELECT item_id, t, st, CASE WHEN t IN (217, 1298) THEN MAX(own, bp) WHEN t = 1369 THEN bp / 2 WHEN t = 1535 THEN 2000 ELSE own END pr, lv FROM ("
        "  SELECT i.item_id, MIN(i.item_type_value_id) t, MAX(COALESCE(i.item_subtype_value_id, 0)) st, "
        "         MAX(MAX(COALESCE(i.required_level, 0)), MAX(COALESCE(i.skill_level_min, 0)) / 4, "
        "             COALESCE((SELECT MAX(COALESCE(ci.required_level, 0)) FROM asm_data_set_blueprints b "
        "                       JOIN asm_data_set_items ci ON ci.item_id = b.created_item_id "
        "                       WHERE b.blueprint_id = i.item_id), 0)) lv, "
        "         MAX(COALESCE(i.item_bind_type_value_id, 0)) bind, "
        "         COALESCE((SELECT MIN(p.price) FROM asm_data_set_prices p WHERE p.item_id = i.item_id "
        "                   AND p.currency_type_value_id = 1603 AND p.price > 0), 0) own, "
        "         COALESCE((SELECT MIN(bp.price) FROM asm_data_set_blueprints b JOIN asm_data_set_prices bp "
        "                   ON bp.item_id = b.blueprint_id AND bp.currency_type_value_id = 1603 AND bp.price > 0 "
        "                   WHERE b.created_item_id = i.item_id), 0) bp, "
        "         COALESCE((SELECT MIN(d.slot_used_value_id) FROM asm_data_set_devices d WHERE d.device_id = i.item_id), 0) slot, "
        "         COALESCE(MAX(m.message), '') name, "
        "         EXISTS (SELECT 1 FROM asm_data_set_prices p WHERE p.item_id = i.item_id "
        "                 AND p.currency_type_value_id <> 1603 AND p.price > 0) ap, "
        "         EXISTS (SELECT 1 FROM asm_data_set_loot_table_items l WHERE l.item_id = i.item_id AND l.auto_create_flag = 0) plain "
        "  FROM asm_data_set_items i "
        "  LEFT JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id "
        "  WHERE i.item_type_value_id IN (1171, 686, 950, 1412, 1020, 1434, 1441, 217, 1369, 1298, 1535) AND i.item_id > 0 "
        "  GROUP BY i.item_id) "
        "WHERE bind NOT IN (1291, 1472) AND name <> '' AND name NOT GLOB 'z*' "
        "  AND name NOT LIKE 'Seasonal%' AND name NOT LIKE '%Halloween%' AND name NOT LIKE '%AvA%' AND name NOT LIKE '%test%' "
        "  AND (t <> 217 OR (bind = 1290 AND slot IN (388, 389, 806, 981))) "
        "  AND (t <> 686 OR plain = 1) "
        "  AND (t <> 1535 OR ap = 0) "
        "  AND (CASE WHEN t IN (217, 1298) THEN MAX(own, bp) WHEN t = 1369 THEN bp / 2 WHEN t = 1535 THEN 2000 ELSE own END) >= CASE WHEN t = 1171 THEN 2 ELSE 10 END");
    if (!st) return;
    while (sqlite3_step(st.s) == SQLITE_ROW) {
        const int g = GroupFor(sqlite3_column_int(st.s, 1), sqlite3_column_int(st.s, 2));
        if (g < 0) continue;
        const Item it{sqlite3_column_int(st.s, 0), sqlite3_column_int(st.s, 1),
                      sqlite3_column_int64(st.s, 3), sqlite3_column_int(st.s, 4)};
        g_group_of[it.item_id] = (size_t)g;
        g_groups[g].items.push_back(it);
    }
    std::string sum;
    for (const auto& g : g_groups) sum += std::string(sum.empty() ? "" : ", ") + std::to_string(g.items.size()) + " " + g.name;
    Logger::Log("mail", "[Market] catalog: %s\n", sum.c_str());
}

// One item of a group, level band first (0-9, 10-19, ... 40+ - only bands the
// group has; a blueprint's level = what it creates, or its crafting skill / 4) so low and high level stock show up alike; items already at
// max_per_item active listings (have) are skipped.
const Item* PickFrom(const Group& g, const std::map<int, int>& have) {
    std::vector<const Item*> free;
    for (const auto& i : g.items) {
        const auto h = have.find(i.item_id);
        if (h == have.end() || h->second < g_settings.max_per_item) free.push_back(&i);
    }
    if (free.empty()) return nullptr;
    std::vector<int> bands;
    for (const Item* i : free) {
        const int b = std::min(i->level / 10, 4);
        if (std::find(bands.begin(), bands.end(), b) == bands.end()) bands.push_back(b);
    }
    const int band = bands[Rand(0, (int)bands.size() - 1)];
    std::vector<const Item*> in;
    for (const Item* i : free) if (std::min(i->level / 10, 4) == band) in.push_back(i);
    // breadth first: items with the fewest active listings, so a new item comes before a second copy
    int fewest = INT_MAX;
    for (const Item* i : in) { const auto h = have.find(i->item_id); fewest = std::min(fewest, h == have.end() ? 0 : h->second); }
    std::vector<const Item*> least;
    for (const Item* i : in) { const auto h = have.find(i->item_id); if ((h == have.end() ? 0 : h->second) == fewest) least.push_back(i); }
    return least[Rand(0, (int)least.size() - 1)];
}

// Stack size like a player would post: cheap mats in bigger stacks.
int64_t StackFor(const Item& it) {
    if (it.type != 1171) return 1;
    return it.price <= 50 ? Rand(5, 20) : it.price <= 250 ? Rand(3, 10) : Rand(1, 5);
}

void LogAction(sqlite3* db, const char* kind, int64_t bot, int item_id, int64_t qty, int64_t credits,
               int64_t character_id, int64_t auction_id) {
    Stmt st(db, "INSERT INTO ga_market_bot_log (kind, bot_id, item_id, quantity, credits, character_id, auction_id, at) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, strftime('%s','now'))");
    if (!st) return;
    sqlite3_bind_text(st.s, 1, kind, -1, SQLITE_STATIC);
    sqlite3_bind_int64(st.s, 2, bot); sqlite3_bind_int(st.s, 3, item_id); sqlite3_bind_int64(st.s, 4, qty);
    sqlite3_bind_int64(st.s, 5, credits); sqlite3_bind_int64(st.s, 6, character_id); sqlite3_bind_int64(st.s, 7, auction_id);
    sqlite3_step(st.s);
}

// A bot listing about to be posted (priced by the built-in rule or a service).
struct Listing { const Item* item; int64_t qty, player_min, unit, start; int hours; };

std::vector<Listing> PickListings(sqlite3* db) {
    const Settings& s = g_settings;
    std::vector<Listing> out;
    // active bot listings per item and per group
    std::map<int, int> have;
    std::vector<int> group_have(g_groups.size(), 0);
    int active = 0;
    {
        Stmt st(db, "SELECT item_id, COUNT(*) FROM ga_auctions WHERE state = 0 AND seller_character_id < 0 "
                    "AND expire_at > strftime('%s','now') GROUP BY item_id");
        if (!st) return out;
        while (sqlite3_step(st.s) == SQLITE_ROW) {
            const int id = sqlite3_column_int(st.s, 0), n = sqlite3_column_int(st.s, 1);
            have[id] = n;
            active += n;
            const auto g = g_group_of.find(id);
            if (g != g_group_of.end()) group_have[g->second] += n;
        }
    }
    const int need = std::min(s.restock_per_tick, s.target_listings - active);
    int weights = 0;
    for (const auto& g : g_groups) if (!g.items.empty()) weights += g.weight;
    if (need <= 0 || weights <= 0) return out;
    // fill the group furthest below its share of target_listings; a group with
    // nothing left to list drops out and its share goes to the others
    std::vector<bool> full(g_groups.size(), false);
    while ((int)out.size() < need) {
        int best = -1;
        double best_gap = -1e18;
        for (size_t i = 0; i < g_groups.size(); ++i) {
            if (full[i] || g_groups[i].items.empty()) continue;
            const double share = (double)s.target_listings * g_groups[i].weight / weights;
            const double gap = (share - group_have[i]) / share;
            if (gap > best_gap) { best_gap = gap; best = (int)i; }
        }
        if (best < 0) break;
        const Item* it = PickFrom(g_groups[best], have);
        if (!it) { full[best] = true; continue; }
        ++have[it->item_id];
        ++group_have[best];
        Listing l{it, StackFor(*it), 0, 0, 0, 24};
        l.player_min = Scalar(db,
            "SELECT COALESCE(MIN(buyout_price / quantity), 0) FROM ga_auctions WHERE state = 0 AND seller_character_id > 0 "
            "AND item_id = ? AND quantity > 0 AND expire_at > strftime('%s','now')", {it->item_id});
        // built-in price: vendor price * markup, not below the cheapest player listing
        l.unit  = std::max({(int64_t)1, it->price * s.markup / 100, l.player_min});
        l.start = std::max<int64_t>(1, l.unit * l.qty * 80 / 100);
        out.push_back(l);
    }
    return out;
}

struct Cand { int64_t id; int item_id; int64_t qty, buyout, seller, vendor, age; };

std::vector<Cand> BuyCandidates(sqlite3* db) {
    const Settings& s = g_settings;
    std::vector<Cand> cands;
    // player listings, no bid, old enough, unit buyout <= buy_percent of the vendor price
    Stmt st(db,
        "SELECT a.id, a.item_id, a.quantity, a.buyout_price, a.seller_character_id, v.pr, "
        "       strftime('%s','now') - a.created_at FROM ga_auctions a "
        "JOIN (SELECT item_id, MIN(price) pr FROM asm_data_set_prices WHERE currency_type_value_id = 1603 AND price > 0 "
        "      GROUP BY item_id) v ON v.item_id = a.item_id "
        "WHERE a.state = 0 AND a.seller_character_id > 0 AND a.bid = 0 AND a.item_id > 0 "
        "  AND a.expire_at > strftime('%s','now') AND a.created_at <= strftime('%s','now') - ?1 "
        "  AND a.buyout_price * 100 <= ?2 * a.quantity * v.pr "
        "ORDER BY random() LIMIT 30");
    if (!st) return cands;
    sqlite3_bind_int64(st.s, 1, (int64_t)s.buy_min_age_minutes * 60);
    sqlite3_bind_int64(st.s, 2, s.buy_percent);
    while (sqlite3_step(st.s) == SQLITE_ROW)
        cands.push_back({sqlite3_column_int64(st.s, 0), sqlite3_column_int(st.s, 1), sqlite3_column_int64(st.s, 2),
                         sqlite3_column_int64(st.s, 3), sqlite3_column_int64(st.s, 4), sqlite3_column_int64(st.s, 5),
                         sqlite3_column_int64(st.s, 6)});
    return cands;
}

// Player listings a bot may bid on: no bot bid on it yet (one bot bid per auction, ever), the
// lowest legal bid (start, or +5 % over the current player bid) is below the buyout and <= buy_percent.
struct BidCand { int64_t id; int item_id; int64_t qty, min_bid, buyout, seller, vendor, age, left; };

std::vector<BidCand> BidCandidates(sqlite3* db) {
    const Settings& s = g_settings;
    std::vector<BidCand> out;
    Stmt st(db,
        "SELECT id, item_id, quantity, nb, buyout_price, seller_character_id, pr, age, rem FROM ("
        " SELECT a.id, a.item_id, a.quantity, a.buyout_price, a.seller_character_id, v.pr,"
        "  CASE WHEN a.bid > 0 THEN a.bid * 105 / 100 + 1 ELSE a.start_price END nb,"
        "  strftime('%s','now') - a.created_at age, a.expire_at - strftime('%s','now') rem FROM ga_auctions a "
        " JOIN (SELECT item_id, MIN(price) pr FROM asm_data_set_prices WHERE currency_type_value_id = 1603 AND price > 0 "
        "       GROUP BY item_id) v ON v.item_id = a.item_id "
        " WHERE a.state = 0 AND a.seller_character_id > 0 AND a.bidder_character_id >= 0 AND a.item_id > 0 "
        "   AND a.expire_at > strftime('%s','now') AND a.created_at <= strftime('%s','now') - ?1 "
        "   AND NOT EXISTS (SELECT 1 FROM ga_market_bot_log g WHERE g.kind = 'bid' AND g.auction_id = a.id)) "
        "WHERE nb > 0 AND nb < buyout_price AND nb * 100 <= ?2 * quantity * pr ORDER BY random() LIMIT 30");
    if (!st) return out;
    sqlite3_bind_int64(st.s, 1, (int64_t)s.buy_min_age_minutes * 60);
    sqlite3_bind_int64(st.s, 2, s.buy_percent);
    while (sqlite3_step(st.s) == SQLITE_ROW)
        out.push_back({sqlite3_column_int64(st.s, 0), sqlite3_column_int(st.s, 1), sqlite3_column_int64(st.s, 2),
                       sqlite3_column_int64(st.s, 3), sqlite3_column_int64(st.s, 4), sqlite3_column_int64(st.s, 5),
                       sqlite3_column_int64(st.s, 6), sqlite3_column_int64(st.s, 7), sqlite3_column_int64(st.s, 8)});
    return out;
}

// Optional "market_tick" service: one call per round with the planned
// listings and buy candidates. Decision:
//   {"list":[{"n":i,"unit":U,"start":S,"hours":H} | {"n":i,"skip":true}], "buy":[auction ids]}
// Missing keys keep the built-in choice. Server limits: unit 50..300 % of the
// vendor price, start <= buyout, hours 12/24/48; "buy" can only narrow the candidates.
// Without a "buy" decision only candidates <= buy_percent_fallback of the vendor price stay.
void FallbackBuys(std::vector<Cand>& cands) {
    const int64_t p = g_settings.buy_percent_fallback;
    std::vector<Cand> kept;
    for (const auto& c : cands)
        if (c.buyout * 100 <= p * c.qty * c.vendor) kept.push_back(c);
    cands.swap(kept);
}

// Active bot listings without a bid (oldest first), sent as "mine"; the
// decision may name some of them in "cancel" (only these ids, cancel_per_tick max).
struct Mine { int64_t id; int item_id; int64_t qty, buyout, age; };

const Item* FindItem(int item_id) {
    const auto g = g_group_of.find(item_id);
    if (g == g_group_of.end()) return nullptr;
    for (const auto& i : g_groups[g->second].items) if (i.item_id == item_id) return &i;
    return nullptr;
}

std::vector<Mine> MyListings(sqlite3* db) {
    std::vector<Mine> out;
    Stmt st(db,
        "SELECT id, item_id, quantity, buyout_price, strftime('%s','now') - created_at FROM ga_auctions "
        "WHERE state = 0 AND seller_character_id < 0 AND bid = 0 AND expire_at > strftime('%s','now') "
        "ORDER BY created_at, id LIMIT 60");
    if (!st) return out;
    while (sqlite3_step(st.s) == SQLITE_ROW)
        out.push_back({sqlite3_column_int64(st.s, 0), sqlite3_column_int(st.s, 1), sqlite3_column_int64(st.s, 2),
                       sqlite3_column_int64(st.s, 3), sqlite3_column_int64(st.s, 4)});
    return out;
}

// Request "bids" (BidCand rows), decision "bid":[{id, amount}] -> only offered ids, bid_per_tick max,
// amount clamped to [min_bid, min(buyout - 1, buy_percent of the vendor price)].
void AskService(std::vector<Listing>& lists, std::vector<Cand>& cands, std::vector<int64_t>& cancel,
                std::vector<std::pair<BidCand, int64_t>>& bids, sqlite3* db) {
    if (!ServiceHooks::Enabled("market_tick")) { FallbackBuys(cands); return; }
    const auto bidc = (g_settings.buy_enabled && g_settings.bid_per_tick > 0) ? BidCandidates(db) : std::vector<BidCand>{};
    if (lists.empty() && cands.empty() && bidc.empty()) return;
    nlohmann::json jl = nlohmann::json::array(), jb = nlohmann::json::array(), jm = nlohmann::json::array();
    nlohmann::json jd = nlohmann::json::array();
    for (const auto& b : bidc) {
        const Item* it = FindItem(b.item_id);
        jd.push_back({{"id", b.id}, {"item_id", b.item_id}, {"type", it ? it->type : 0}, {"level", it ? it->level : 0},
                      {"vendor", b.vendor}, {"qty", b.qty}, {"min_bid", b.min_bid}, {"buyout", b.buyout},
                      {"age", b.age}, {"left", b.left}});
    }
    const auto mine = MyListings(db);
    for (const auto& m : mine) {
        const Item* it = FindItem(m.item_id);
        if (!it || m.qty <= 0) continue;
        jm.push_back({{"id", m.id}, {"item_id", m.item_id}, {"type", it->type}, {"level", it->level},
                      {"vendor", it->price}, {"qty", m.qty}, {"unit", m.buyout / m.qty}, {"age", m.age}});
    }
    for (size_t i = 0; i < lists.size(); ++i) {
        const auto& l = lists[i];
        jl.push_back({{"n", i}, {"item_id", l.item->item_id}, {"type", l.item->type}, {"level", l.item->level},
                      {"vendor", l.item->price}, {"qty", l.qty}, {"player_min", l.player_min}, {"unit", l.unit}});
    }
    for (const auto& c : cands)
        jb.push_back({{"id", c.id}, {"item_id", c.item_id}, {"qty", c.qty}, {"buyout", c.buyout},
                      {"vendor", c.vendor}, {"age", c.age}});
    const auto d = ServiceHooks::Call("market_tick", {{"list", jl}, {"buy", jb}, {"mine", jm}, {"bids", jd}});
    if (!d) { FallbackBuys(cands); return; }
    if (d->contains("bid") && (*d)["bid"].is_array()) {
        for (const auto& e : (*d)["bid"]) {
            if ((int)bids.size() >= g_settings.bid_per_tick) break;
            if (!e.is_object()) continue;
            const int64_t id = ServiceHooks::ClampInt(e, "id", 0, 0, INT64_MAX);
            for (const auto& b : bidc) {
                if (b.id != id) continue;
                bool dup = false;
                for (const auto& x : bids) dup = dup || x.first.id == id;
                const int64_t hi = std::min(b.buyout - 1, (int64_t)g_settings.buy_percent * b.qty * b.vendor / 100);
                if (!dup && hi >= b.min_bid) bids.push_back({b, ServiceHooks::ClampInt(e, "amount", b.min_bid, b.min_bid, hi)});
                break;
            }
        }
    }
    if (d->contains("cancel") && (*d)["cancel"].is_array()) {
        for (const auto& id : (*d)["cancel"]) {
            if ((int)cancel.size() >= g_settings.cancel_per_tick) break;
            if (!id.is_number_integer()) continue;
            const int64_t v = id.get<int64_t>();
            for (const auto& m : mine)
                if (m.id == v && std::find(cancel.begin(), cancel.end(), v) == cancel.end()) { cancel.push_back(v); break; }
        }
    }
    if (d->contains("list") && (*d)["list"].is_array()) {
        std::vector<bool> skip(lists.size(), false);
        for (const auto& e : (*d)["list"]) {
            if (!e.is_object()) continue;
            const int64_t n = ServiceHooks::ClampInt(e, "n", -1, -1, (int64_t)lists.size() - 1);
            if (n < 0) continue;
            auto& l = lists[n];
            if (e.value("skip", false)) { skip[n] = true; continue; }
            const int64_t v = l.item->price;
            l.unit = ServiceHooks::ClampInt(e, "unit", l.unit, std::max<int64_t>(1, v / 2), std::max<int64_t>(1, v * 3));
            const int64_t buyout = l.unit * l.qty;
            l.start = ServiceHooks::ClampInt(e, "start", buyout * 80 / 100, 1, buyout);
            const int64_t h = ServiceHooks::ClampInt(e, "hours", 24, 12, 48);
            l.hours = h < 18 ? 12 : h < 36 ? 24 : 48;
        }
        std::vector<Listing> kept;
        for (size_t i = 0; i < lists.size(); ++i) if (!skip[i]) kept.push_back(lists[i]);
        lists.swap(kept);
    }
    if (d->contains("buy") && (*d)["buy"].is_array()) {
        std::vector<Cand> kept;
        for (const auto& c : cands)
            for (const auto& id : (*d)["buy"])
                if (id.is_number_integer() && id.get<int64_t>() == c.id) { kept.push_back(c); break; }
        cands.swap(kept);
    } else {
        FallbackBuys(cands);
    }
}

int Restock(sqlite3* db, const std::vector<Listing>& lists) {
    int listed = 0;
    for (const auto& l : lists) {
        const int64_t bot = Rand(1, kBotCount);
        const int64_t buyout = l.unit * l.qty;
        const int64_t id = AuctionService::BotList(bot, l.item->item_id, l.qty, l.item->type == 1171,
                                                   std::min(l.start, buyout), buyout, l.hours);
        if (id == 0) continue;
        LogAction(db, "list", bot, l.item->item_id, l.qty, buyout, 0, id);
        ++listed;
    }
    return listed;
}

int PlaceBids(sqlite3* db, const std::vector<std::pair<BidCand, int64_t>>& bids, std::vector<int64_t>& mailed) {
    const Settings& s = g_settings;
    int placed = 0;
    for (const auto& [b, amount] : bids) {
        if (Scalar(db, "SELECT COUNT(*) FROM ga_market_bot_log WHERE kind IN ('buy', 'bid') AND item_id = ? "
                       "AND at > strftime('%s','now') - 86400", {b.item_id}) >= s.buy_daily_item_cap) continue;
        if (Scalar(db, "SELECT COALESCE(SUM(credits), 0) FROM ga_market_bot_log WHERE kind IN ('buy', 'bid') AND character_id = ? "
                       "AND at > strftime('%s','now') - 86400", {b.seller}) + amount > s.buy_daily_seller_credits) continue;
        const auto r = AuctionService::BotBid(b.id, Rand(1, kBotCount), amount);   // logs kind=bid itself
        if (!r.ok) continue;
        mailed.insert(mailed.end(), r.mailed.begin(), r.mailed.end());
        ++placed;
    }
    return placed;
}

int Buy(sqlite3* db, const std::vector<Cand>& cands, std::vector<int64_t>& mailed) {
    const Settings& s = g_settings;
    int bought = 0;
    for (const auto& c : cands) {
        if (bought >= s.buy_per_tick) break;
        if (Scalar(db, "SELECT COUNT(*) FROM ga_market_bot_log WHERE kind = 'buy' AND item_id = ? "
                       "AND at > strftime('%s','now') - 86400", {c.item_id}) >= s.buy_daily_item_cap) continue;
        if (Scalar(db, "SELECT COALESCE(SUM(credits), 0) FROM ga_market_bot_log WHERE kind = 'buy' AND character_id = ? "
                       "AND at > strftime('%s','now') - 86400", {c.seller}) + c.buyout > s.buy_daily_seller_credits) continue;
        const int64_t bot = Rand(1, kBotCount);
        const auto r = AuctionService::BotBuyout(c.id, kBotNames[bot - 1]);
        if (!r.ok) continue;
        LogAction(db, "buy", bot, c.item_id, c.qty, c.buyout, c.seller, c.id);
        mailed.insert(mailed.end(), r.mailed.begin(), r.mailed.end());
        ++bought;
    }
    return bought;
}

} // namespace

void Configure(const Settings& s) { g_settings = s; }
const Settings& Get() { return g_settings; }

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    char* err = nullptr;
    if (sqlite3_exec(db,
            "CREATE TABLE IF NOT EXISTS ga_market_bots (id INTEGER PRIMARY KEY, name TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS ga_market_bot_log ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT, kind TEXT NOT NULL, bot_id INTEGER NOT NULL DEFAULT 0,"
            "  item_id INTEGER NOT NULL DEFAULT 0, quantity INTEGER NOT NULL DEFAULT 0, credits INTEGER NOT NULL DEFAULT 0,"
            "  character_id INTEGER NOT NULL DEFAULT 0, auction_id INTEGER NOT NULL DEFAULT 0, at INTEGER NOT NULL);"
            "CREATE INDEX IF NOT EXISTS ga_market_bot_log_at ON ga_market_bot_log(kind, at);",
            nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("mail", "[Market] schema failed: %s\n", err ? err : "?");
        sqlite3_free(err);
        return;
    }
    Stmt st(db, "INSERT OR REPLACE INTO ga_market_bots (id, name) VALUES (?, ?)");
    if (!st) return;
    for (int i = 0; i < kBotCount; ++i) {
        sqlite3_reset(st.s);
        sqlite3_bind_int(st.s, 1, i + 1);
        sqlite3_bind_text(st.s, 2, kBotNames[i], -1, SQLITE_STATIC);
        sqlite3_step(st.s);
    }
}

TickResult Tick() {
    TickResult r;
    if (!g_settings.enabled) return r;
    sqlite3* db = Database::GetConnection();
    if (!g_loaded) {
        LoadCatalog(db);
        // listings from an older catalog (no bid on them) leave the market once
        struct Stale { int64_t id; int item; int64_t seller; };
        std::vector<Stale> stale;
        {
            Stmt st(db, "SELECT id, item_id, seller_character_id FROM ga_auctions WHERE state = 0 AND seller_character_id < 0 AND bid = 0");
            while (st && sqlite3_step(st.s) == SQLITE_ROW)
                if (!g_group_of.count(sqlite3_column_int(st.s, 1)))
                    stale.push_back({sqlite3_column_int64(st.s, 0), sqlite3_column_int(st.s, 1), sqlite3_column_int64(st.s, 2)});
        }
        for (const auto& x : stale)
            if (AuctionService::BotCancel(x.id)) LogAction(db, "cancel", -x.seller, x.item, 0, 0, 0, x.id);
        Logger::Log("mail", "[Market] withdrew %zu listings outside the catalog\n", stale.size());
    }
    r.mailed = AuctionService::SettleExpired();
    auto lists = PickListings(db);
    auto cands = g_settings.buy_enabled ? BuyCandidates(db) : std::vector<Cand>{};
    std::vector<int64_t> cancel;
    std::vector<std::pair<BidCand, int64_t>> bids;
    AskService(lists, cands, cancel, bids, db);
    for (int64_t id : cancel) {
        const int64_t item = Scalar(db, "SELECT item_id FROM ga_auctions WHERE id = ?", {id});
        const int64_t seller = Scalar(db, "SELECT seller_character_id FROM ga_auctions WHERE id = ?", {id});
        if (AuctionService::BotCancel(id)) { LogAction(db, "cancel", -seller, (int)item, 0, 0, 0, id); ++r.cancelled; }
    }
    r.listed = Restock(db, lists);
    r.bought = Buy(db, cands, r.mailed);
    r.bid = PlaceBids(db, bids, r.mailed);
    if (r.listed || r.bought || r.cancelled || r.bid)
        Logger::Log("mail", "[Market] tick: listed %d, bought %d, withdrew %d, bid %d\n", r.listed, r.bought, r.cancelled, r.bid);
    return r;
}

} // namespace MarketBots
