#include "src/ControlServer/AuctionService/AuctionService.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <algorithm>

namespace AuctionService {

namespace {

struct Stmt {
    sqlite3_stmt* s = nullptr;
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("mail", "[AH] prepare failed: %s (%s)\n", sqlite3_errmsg(db), sql);
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    explicit operator bool() const { return s != nullptr; }
};

bool Exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("mail", "[AH] exec failed: %s (%s)\n", err ? err : "?", sql);
        sqlite3_free(err);
        return false;
    }
    return true;
}

std::string ColText(sqlite3_stmt* s, int i) {
    const unsigned char* t = sqlite3_column_text(s, i);
    return t ? reinterpret_cast<const char*>(t) : std::string();
}

const char* kCols =
    "id, seller_character_id, holding_mail_id, item_id, name_msg_id, quantity, start_price, buyout_price, "
    "bid, bidder_character_id, expire_at, name";

Listing ReadListing(sqlite3_stmt* s, std::string* name = nullptr) {
    Listing l;
    l.id                  = sqlite3_column_int64(s, 0);
    l.seller_character_id = sqlite3_column_int64(s, 1);
    l.holding_mail_id     = sqlite3_column_int64(s, 2);
    l.item_id             = sqlite3_column_int(s, 3);
    l.name_msg_id         = sqlite3_column_int(s, 4);
    l.quantity            = sqlite3_column_int64(s, 5);
    l.start_price         = sqlite3_column_int64(s, 6);
    l.buyout_price        = sqlite3_column_int64(s, 7);
    l.bid                 = sqlite3_column_int64(s, 8);
    l.bidder_character_id = sqlite3_column_int64(s, 9);
    l.expire_at           = sqlite3_column_int64(s, 10);
    if (name) *name = ColText(s, 11);
    return l;
}

std::vector<Listing> Query(const std::string& where, const std::vector<std::string>& binds_text,
                           const std::vector<int64_t>& binds_int) {
    sqlite3* db = Database::GetConnection();
    std::vector<Listing> out;
    const std::string sql = std::string("SELECT ") + kCols + " FROM ga_auctions WHERE " + where;
    Stmt st(db, sql.c_str());
    if (!st) return out;
    int k = 1;
    for (int64_t v : binds_int) sqlite3_bind_int64(st.s, k++, v);
    for (const auto& t : binds_text) sqlite3_bind_text(st.s, k++, t.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(st.s) == SQLITE_ROW) out.push_back(ReadListing(st.s));
    for (auto& l : out) l.items = MailService::HeldItems(l.holding_mail_id);
    return out;
}

// active listing (inside the caller's transaction)
bool LoadActive(sqlite3* db, int64_t id, Listing* l, std::string* name) {
    const std::string sql = std::string("SELECT ") + kCols +
        " FROM ga_auctions WHERE id = ? AND state = 0 AND expire_at > strftime('%s','now')";
    Stmt st(db, sql.c_str());
    if (!st) return false;
    sqlite3_bind_int64(st.s, 1, id);
    if (sqlite3_step(st.s) != SQLITE_ROW) return false;
    *l = ReadListing(st.s, name);
    return true;
}

bool Credits(sqlite3* db, int64_t character_id, int64_t delta) {
    Stmt st(db, delta < 0
        ? "UPDATE ga_characters SET credits = credits + ?1 WHERE id = ?2 AND deleted_at = 0 AND credits >= -?1"
        : "UPDATE ga_characters SET credits = credits + ?1 WHERE id = ?2");
    if (!st) return false;
    sqlite3_bind_int64(st.s, 1, delta);
    sqlite3_bind_int64(st.s, 2, character_id);
    return sqlite3_step(st.s) == SQLITE_DONE && sqlite3_changes(db) > 0;
}

bool SetState(sqlite3* db, int64_t id, int state) {
    Stmt st(db, "UPDATE ga_auctions SET state = ?, settled_at = strftime('%s','now') WHERE id = ? AND state = 0");
    if (!st) return false;
    sqlite3_bind_int(st.s, 1, state);
    sqlite3_bind_int64(st.s, 2, id);
    return sqlite3_step(st.s) == SQLITE_DONE && sqlite3_changes(db) > 0;
}

std::string Quoted(const std::string& name) { return "\"" + name + "\""; }

// Sold (buyout or won bid): item -> winner, price minus the cut -> seller.
// Market bot seller name (seller id -bot id), "" for players.
std::string BotName(sqlite3* db, int64_t seller) {
    if (seller >= 0) return "";
    Stmt st(db, "SELECT name FROM ga_market_bots WHERE id = ?");
    if (!st) return "";
    sqlite3_bind_int64(st.s, 1, -seller);
    return sqlite3_step(st.s) == SQLITE_ROW ? ColText(st.s, 0) : std::string();
}

bool Settle(sqlite3* db, const Listing& l, const std::string& name, int64_t winner, int64_t price, Result& r) {
    if (!SetState(db, l.id, 1)) return false;
    const int64_t cut = price * CUT_PERCENT / 100;
    const std::string bot = BotName(db, l.seller_character_id);
    if (!MailService::DeliverFromAuction(l.holding_mail_id, winner, "Auction Won - " + name,
            "You won the auction for " + Quoted(name) + " (x" + std::to_string(l.quantity) + ") for " +
            std::to_string(price) + " credits." + (bot.empty() ? "" : " Seller: " + bot + "."), 0)) return false;
    r.mailed.push_back(winner);
    if (l.seller_character_id < 0) {   // market bot: the credits leave the economy
        // Ledger line "sold" (bot_id, buyer character, price) so the market service can see who bought
        Stmt lg(db, "INSERT INTO ga_market_bot_log (kind, bot_id, item_id, quantity, credits, character_id, auction_id, at) "
                    "VALUES ('sold', ?, ?, ?, ?, ?, ?, strftime('%s','now'))");
        if (lg) {
            sqlite3_bind_int64(lg.s, 1, -l.seller_character_id); sqlite3_bind_int64(lg.s, 2, l.item_id);
            sqlite3_bind_int64(lg.s, 3, l.quantity); sqlite3_bind_int64(lg.s, 4, price);
            sqlite3_bind_int64(lg.s, 5, winner); sqlite3_bind_int64(lg.s, 6, l.id);
            sqlite3_step(lg.s);
        }
        Logger::Log("mail", "[AH] bot auction %lld sold to char %lld for %lld\n",
            (long long)l.id, (long long)winner, (long long)price);
        return true;
    }
    if (!MailService::DeliverFromAuction(0, l.seller_character_id, "Auction Successful - " + Quoted(name),
            "Your auction of " + Quoted(name) + " (x" + std::to_string(l.quantity) + ") has sold for " +
            std::to_string(price) + " credits (house cut " + std::to_string(cut) + ").", price - cut)) return false;
    r.mailed.push_back(l.seller_character_id);
    Logger::Log("mail", "[AH] auction %lld sold to char %lld for %lld (cut %lld)\n",
        (long long)l.id, (long long)winner, (long long)price, (long long)cut);
    return true;
}

// Market bot ledger line (best effort, inside the caller's transaction)
void BotLog(sqlite3* db, const char* kind, const Listing& l, int64_t credits) {
    Stmt st(db, "INSERT INTO ga_market_bot_log (kind, bot_id, item_id, quantity, credits, character_id, auction_id, at) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, strftime('%s','now'))");
    if (!st) return;
    sqlite3_bind_text(st.s, 1, kind, -1, SQLITE_STATIC);
    sqlite3_bind_int64(st.s, 2, -l.bidder_character_id); sqlite3_bind_int64(st.s, 3, l.item_id);
    sqlite3_bind_int64(st.s, 4, l.quantity); sqlite3_bind_int64(st.s, 5, credits);
    sqlite3_bind_int64(st.s, 6, l.seller_character_id); sqlite3_bind_int64(st.s, 7, l.id);
    sqlite3_step(st.s);
}

bool RefundBidder(const Listing& l, const std::string& name, Result& r) {
    if (l.bidder_character_id == 0 || l.bid <= 0) return true;
    if (l.bidder_character_id < 0) { BotLog(Database::GetConnection(), "bid_refund", l, l.bid); return true; }   // bot money, no mail
    if (!MailService::DeliverFromAuction(0, l.bidder_character_id, "Auction Outbid - " + Quoted(name),
            "You were outbid on " + Quoted(name) + ". Your bid of " + std::to_string(l.bid) +
            " credits is returned.", l.bid)) return false;
    r.mailed.push_back(l.bidder_character_id);
    return true;
}

// A market bot won a player auction with its bid: seller paid, item leaves the game.
bool BotWon(sqlite3* db, const Listing& l, const std::string& name, Result& r) {
    if (!SetState(db, l.id, 1)) return false;
    const int64_t price = l.bid, cut = price * CUT_PERCENT / 100;
    const std::string bot = BotName(db, l.bidder_character_id);
    if (!MailService::DeliverFromAuction(0, l.seller_character_id, "Auction Successful - " + Quoted(name),
            "Your auction of " + Quoted(name) + " (x" + std::to_string(l.quantity) + ") has sold to " + bot +
            " for " + std::to_string(price) + " credits (house cut " + std::to_string(cut) + ").", price - cut)) return false;
    if (!MailService::DiscardHeld(l.holding_mail_id)) return false;
    r.mailed.push_back(l.seller_character_id);
    BotLog(db, "bid_won", l, price);
    return true;
}

Result Fail(sqlite3* db, Result r, uint32_t msg) {
    Exec(db, "ROLLBACK");
    r.ok = false; r.error_msg_id = msg;
    r.removed_rows.clear(); r.components_changed = false; r.depleted_components.clear(); r.mailed.clear();
    return r;
}

} // namespace

void EnsureSchema() {
    Exec(Database::GetConnection(),
        "CREATE TABLE IF NOT EXISTS ga_auctions ("
        "  id                  INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  seller_character_id INTEGER NOT NULL,"
        "  holding_mail_id     INTEGER NOT NULL,"   // ga_mail row (to_character_id 0) holding the item
        "  item_id             INTEGER NOT NULL DEFAULT 0,"
        "  name_msg_id         INTEGER NOT NULL DEFAULT 0,"
        "  name                TEXT NOT NULL DEFAULT '',"
        "  item_type           INTEGER NOT NULL DEFAULT 0,"
        "  item_subtype        INTEGER NOT NULL DEFAULT 0,"
        "  quality             INTEGER NOT NULL DEFAULT 0,"
        "  quantity            INTEGER NOT NULL DEFAULT 1,"
        "  start_price         INTEGER NOT NULL,"
        "  buyout_price        INTEGER NOT NULL,"
        "  bid                 INTEGER NOT NULL DEFAULT 0,"
        "  bidder_character_id INTEGER NOT NULL DEFAULT 0,"
        "  fee                 INTEGER NOT NULL DEFAULT 0,"
        "  created_at          INTEGER NOT NULL,"
        "  expire_at           INTEGER NOT NULL,"
        "  state               INTEGER NOT NULL DEFAULT 0,"   // 0 active, 1 sold, 2 cancelled, 3 expired
        "  settled_at          INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS ga_auctions_active ON ga_auctions(state, expire_at);"
        "CREATE INDEX IF NOT EXISTS ga_auctions_seller ON ga_auctions(seller_character_id, state);");
}

int DurationHours(uint32_t v) {
    // UTgUIAuctionSellMenu duration combo = valid values group 121
    switch (v) {
    case 1067: return 12;
    case 1069: return 48;
    default:   return 24;   // 1068
    }
}

namespace {
void BuildWhere(const Filter& f, std::string& where, std::vector<int64_t>& ints, std::vector<std::string>& texts) {
    where = "state = 0 AND expire_at > strftime('%s','now')";
    if (!f.types.empty()) {
        where += " AND (";
        for (size_t i = 0; i < f.types.size(); ++i) {
            if (i) where += " OR ";
            // The client has no blueprint category and refuses a search without one; its only blueprint
            // filter (Special Auction, 686/1574) therefore shows every blueprint, or they could not be found.
            const int subtype = f.types[i].first == 686 ? 0 : f.types[i].second;
            where += subtype ? "(item_type = ? AND item_subtype = ?)" : "(item_type = ? AND 0 = ?)";
            ints.push_back(f.types[i].first);
            ints.push_back(subtype);
        }
        where += ")";
    }
    // a quality no item has (the combo's "any" entry) does not filter; a row
    // stored without quality (0, older bot rows) matches its item's quality
    if (f.quality) {
        where += " AND (quality = ? OR (quality = 0 AND EXISTS (SELECT 1 FROM asm_data_set_items x "
                 "WHERE x.item_id = ga_auctions.item_id AND x.quality_value_id = ?)) "
                 "OR NOT EXISTS (SELECT 1 FROM asm_data_set_items WHERE quality_value_id = ?))";
        ints.push_back(f.quality); ints.push_back(f.quality); ints.push_back(f.quality);
    }
    if (!f.name.empty()) { where += " AND name LIKE ? ESCAPE '\\'"; }
    if (!f.name.empty()) {
        std::string esc;
        for (char c : f.name) { if (c == '%' || c == '_' || c == '\\') esc += '\\'; esc += c; }
        texts.push_back("%" + esc + "%");
    }
}

// " LIMIT n OFFSET m" of the page the client asked for
std::string PageClause(const Filter& f, uint32_t total) {
    // the client keeps its page when the category changes: a page past the end is the first page
    const int start = f.start_row > 0 && (uint32_t)f.start_row < total ? f.start_row : 0;
    const int n = f.end_row > start ? std::min(f.end_row - start, MAX_ROWS) : MAX_ROWS;
    return " LIMIT " + std::to_string(n) + " OFFSET " + std::to_string(start);
}

void BindAll(sqlite3_stmt* s, const std::vector<int64_t>& ints, const std::vector<std::string>& texts) {
    int k = 1;
    for (int64_t v : ints) sqlite3_bind_int64(s, k++, v);
    for (const auto& t : texts) sqlite3_bind_text(s, k++, t.c_str(), -1, SQLITE_TRANSIENT);
}
} // namespace

std::vector<MarketRow> BrowseMarket(const Filter& f, uint32_t* total) {
    std::string where;
    std::vector<int64_t> ints;
    std::vector<std::string> texts;
    BuildWhere(f, where, ints, texts);
    const std::string groups =
        "WITH m AS (SELECT item_id, quality, name_msg_id, name, quantity, holding_mail_id, id, "
        "  buyout_price / MAX(quantity, 1) unit FROM ga_auctions WHERE " + where + ") "
        "SELECT item_id, quality, MIN(name_msg_id) nm, MAX(name) nt, SUM(quantity) c, unit, MIN(holding_mail_id) h FROM m " +
        (f.lowest ? std::string("WHERE unit = (SELECT MIN(x.unit) FROM m x WHERE x.item_id = m.item_id AND x.quality = m.quality) ") : std::string()) +
        "GROUP BY item_id, quality, unit";
    sqlite3* db = Database::GetConnection();
    std::vector<MarketRow> out;
    uint32_t count = 0;
    {
        Stmt st(db, ("SELECT COUNT(*) FROM (" + groups + ")").c_str());
        if (st) {
            BindAll(st.s, ints, texts);
            if (sqlite3_step(st.s) == SQLITE_ROW) count = (uint32_t)sqlite3_column_int(st.s, 0);
        }
    }
    if (total) *total = count;
    // lowest prices: cheapest first; every price: an item's prices side by side (the client rules off each item)
    Stmt st(db, (groups + (f.lowest ? " ORDER BY unit, nt, quality" : " ORDER BY nt, quality, unit") + PageClause(f, count)).c_str());
    if (st) {
        BindAll(st.s, ints, texts);
        while (sqlite3_step(st.s) == SQLITE_ROW) {
            MarketRow m;
            m.item_id = sqlite3_column_int(st.s, 0);
            m.quality = sqlite3_column_int(st.s, 1);
            m.name_msg_id = sqlite3_column_int(st.s, 2);
            m.count = sqlite3_column_int64(st.s, 4);
            m.unit = sqlite3_column_int64(st.s, 5);
            m.items = MailService::HeldItems(sqlite3_column_int64(st.s, 6));
            out.push_back(std::move(m));
        }
    }
    std::string ts;
    for (const auto& t : f.types) ts += (ts.empty() ? "" : ",") + std::to_string(t.first) + (t.second ? "/" + std::to_string(t.second) : "");
    Logger::Log("mail", "[AH] market browse types=%s quality=%u name='%s' lowest=%d rows %d-%d -> %zu rows (total %u)\n",
        ts.c_str(), (unsigned)f.quality, f.name.c_str(), f.lowest ? 1 : 0, f.start_row, f.end_row, out.size(), count);
    return out;
}

std::vector<Listing> Browse(const Filter& f, uint32_t* total) {
    std::string where;
    std::vector<int64_t> ints;
    std::vector<std::string> texts;
    BuildWhere(f, where, ints, texts);
    if (total) {
        sqlite3* db = Database::GetConnection();
        Stmt st(db, ("SELECT COUNT(*) FROM ga_auctions WHERE " + where).c_str());
        *total = 0;
        if (st) {
            int k = 1;
            for (int64_t v : ints) sqlite3_bind_int64(st.s, k++, v);
            for (const auto& t : texts) sqlite3_bind_text(st.s, k++, t.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(st.s) == SQLITE_ROW) *total = (uint32_t)sqlite3_column_int(st.s, 0);
        }
    }
    // On an equal price, player listings come before bot listings
    auto rows = Query(where + " ORDER BY buyout_price, seller_character_id < 0, id" + PageClause(f, total ? *total : 0), texts, ints);
    // one line per search: which categories the client asks for
    std::string ts;
    for (const auto& t : f.types) ts += (ts.empty() ? "" : ",") + std::to_string(t.first) + (t.second ? "/" + std::to_string(t.second) : "");
    Logger::Log("mail", "[AH] browse types=%s quality=%u name='%s' -> %zu rows (total %u)\n",
        ts.c_str(), (unsigned)f.quality, f.name.c_str(), rows.size(), total ? *total : 0u);
    return rows;
}

std::vector<Listing> MySells(int64_t character_id) {
    return Query("seller_character_id = ? AND state = 0 ORDER BY expire_at LIMIT " + std::to_string(MAX_ROWS),
                 {}, {character_id});
}

std::vector<Listing> MyBids(int64_t character_id) {
    return Query("bidder_character_id = ? AND state = 0 ORDER BY expire_at LIMIT " + std::to_string(MAX_ROWS),
                 {}, {character_id});
}

Result Add(int64_t character_id, int64_t inventory_id, int64_t count,
           int64_t start_price, int64_t buyout_price, uint32_t duration_value_id) {
    Result r;
    r.error_msg_id = MSG_ADD_FAILED;
    if (character_id <= 0 || inventory_id <= 0) return r;
    if (count <= 0) { r.error_msg_id = MSG_BAD_QUANTITY; return r; }
    if (start_price <= 0) {   // market sell panel: Unit Price x Quantity
        if (buyout_price <= 0 || buyout_price > MAX_PRICE / count) { r.error_msg_id = MSG_BAD_BUYOUT; return r; }
        buyout_price *= count;
        start_price = buyout_price;
    }
    if (buyout_price <= 0 || buyout_price > MAX_PRICE || start_price > buyout_price) {
        r.error_msg_id = MSG_BAD_BUYOUT; return r;
    }
    sqlite3* db = Database::GetConnection();
    int64_t user_id = 0, active = 0;
    {
        Stmt st(db, "SELECT c.user_id, (SELECT COUNT(*) FROM ga_auctions a WHERE a.seller_character_id = c.id AND a.state = 0) "
                    "FROM ga_characters c WHERE c.id = ? AND c.deleted_at = 0");
        if (!st) return r;
        sqlite3_bind_int64(st.s, 1, character_id);
        if (sqlite3_step(st.s) != SQLITE_ROW) return r;
        user_id = sqlite3_column_int64(st.s, 0);
        active  = sqlite3_column_int64(st.s, 1);
    }
    if (active >= MAX_LISTINGS) return r;
    const int64_t fee = std::max<int64_t>(FEE_MIN, start_price * FEE_PERCENT / 100);

    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    if (!Credits(db, character_id, -fee)) return Fail(db, r, MSG_NO_FEE_CREDITS);
    MailService::SendResult sr;
    MailService::Attachment a;
    a.inventory_id = inventory_id;
    a.count = count;
    const int64_t hold = MailService::HoldForAuction(user_id, a, sr);
    if (hold == 0) return Fail(db, r, MSG_ADD_FAILED);
    const auto items = MailService::HeldItems(hold);
    if (items.size() != 1) return Fail(db, r, MSG_ADD_FAILED);
    const auto& it = items[0];
    {
        Stmt st(db,
            "INSERT INTO ga_auctions (seller_character_id, holding_mail_id, item_id, name_msg_id, name, item_type, "
            "  item_subtype, quality, quantity, start_price, buyout_price, fee, created_at, expire_at) "
            "SELECT ?1, ?2, COALESCE(i.item_id, ?3), COALESCE(i.name_msg_id, 0), COALESCE(NULLIF(i.name_msg_translated, ''), "
            "  (SELECT message FROM asm_data_set_msg_translations WHERE msg_id = i.name_msg_id), ''), "
            "  COALESCE(i.item_type_value_id, 0), COALESCE(i.item_subtype_value_id, 0), ?4, ?5, ?6, ?7, ?8, "
            "  strftime('%s','now'), strftime('%s','now') + ?9 "
            "FROM (SELECT 1) LEFT JOIN asm_data_set_items i ON i.id = (SELECT id FROM asm_data_set_items "
            "  WHERE (?3 > 0 AND item_id = ?3) OR (?3 = 0 AND ?10 > 0 AND ref_device_id = ?10) ORDER BY id LIMIT 1)");
        if (!st) return Fail(db, r, MSG_ADD_FAILED);
        sqlite3_bind_int64(st.s, 1, character_id);
        sqlite3_bind_int64(st.s, 2, hold);
        sqlite3_bind_int(st.s, 3, it.item_id);
        sqlite3_bind_int(st.s, 4, it.quality);
        sqlite3_bind_int64(st.s, 5, it.quantity);
        sqlite3_bind_int64(st.s, 6, start_price);
        sqlite3_bind_int64(st.s, 7, buyout_price);
        sqlite3_bind_int64(st.s, 8, fee);
        sqlite3_bind_int64(st.s, 9, (int64_t)DurationHours(duration_value_id) * 3600);
        sqlite3_bind_int(st.s, 10, it.device_id);
        if (sqlite3_step(st.s) != SQLITE_DONE) return Fail(db, r, MSG_ADD_FAILED);
        r.auction_id = sqlite3_last_insert_rowid(db);
    }
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_ADD_FAILED);
    r.ok = true;
    r.error_msg_id = 0;
    r.removed_rows = sr.removed_rows;
    r.components_changed = sr.components_changed;
    r.depleted_components = sr.depleted_components;
    Logger::Log("mail", "[AH] char %lld listed auction %lld (inv %lld x%lld) start %lld buyout %lld fee %lld\n",
        (long long)character_id, (long long)r.auction_id, (long long)inventory_id, (long long)count,
        (long long)start_price, (long long)buyout_price, (long long)fee);
    return r;
}

Result Buyout(int64_t character_id, int64_t auction_id) {
    Result r;
    r.error_msg_id = MSG_BUY_FAILED;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    Listing l; std::string name;
    if (!LoadActive(db, auction_id, &l, &name)) return Fail(db, r, MSG_GONE);
    if (l.seller_character_id == character_id) return Fail(db, r, MSG_BUY_FAILED);
    if (!Credits(db, character_id, -l.buyout_price)) return Fail(db, r, MSG_NO_CURRENCY);
    if (!RefundBidder(l, name, r)) return Fail(db, r, MSG_BUY_FAILED);
    Exec(db, ("UPDATE ga_auctions SET bid = 0, bidder_character_id = 0 WHERE id = " + std::to_string(l.id)).c_str());  // bought out, not won by the bid
    if (!Settle(db, l, name, character_id, l.buyout_price, r)) return Fail(db, r, MSG_BUY_FAILED);
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_BUY_FAILED);
    r.ok = true; r.error_msg_id = 0; r.auction_id = auction_id;
    return r;
}

Result BuyMarket(int64_t character_id, int item_id, int quality, int64_t count, int64_t unit) {
    Result r;
    r.error_msg_id = MSG_BUY_FAILED;
    if (character_id <= 0 || item_id <= 0 || unit <= 0 || unit > MAX_PRICE) { r.error_msg_id = MSG_GONE; return r; }
    if (count <= 0) count = 1;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    std::vector<std::pair<Listing, std::string>> rows;
    {
        const std::string sql = std::string("SELECT ") + kCols +
            " FROM ga_auctions WHERE state = 0 AND expire_at > strftime('%s','now') AND item_id = ? AND quality = ? "
            "AND seller_character_id <> ? AND quantity > 0 AND buyout_price / quantity <= ? "
            "ORDER BY buyout_price / quantity, seller_character_id < 0, id";
        Stmt st(db, sql.c_str());
        if (!st) return Fail(db, r, MSG_BUY_FAILED);
        sqlite3_bind_int(st.s, 1, item_id); sqlite3_bind_int(st.s, 2, quality);
        sqlite3_bind_int64(st.s, 3, character_id); sqlite3_bind_int64(st.s, 4, unit);
        while (sqlite3_step(st.s) == SQLITE_ROW) {
            std::string name;
            Listing l = ReadListing(st.s, &name);
            rows.emplace_back(l, name);
        }
    }
    int64_t left = count, paid = 0;
    for (const auto& [l, name] : rows) {
        if (left <= 0) break;
        if (l.quantity <= left) {   // the whole listing
            const int64_t whole = l.buyout_price / l.quantity * l.quantity;   // what the buyer was shown: unit x count
            if (!Credits(db, character_id, -whole)) return Fail(db, r, MSG_NO_CURRENCY);
            if (!RefundBidder(l, name, r)) return Fail(db, r, MSG_BUY_FAILED);
            Exec(db, ("UPDATE ga_auctions SET bid = 0, bidder_character_id = 0 WHERE id = " + std::to_string(l.id)).c_str());
            if (!Settle(db, l, name, character_id, whole, r)) return Fail(db, r, MSG_BUY_FAILED);
            left -= l.quantity; paid += whole;
            continue;
        }
        if (l.bid > 0) continue;    // a bid holds the whole stack
        // part of a stack: the sold units become their own settled listing
        const int64_t u = l.buyout_price / l.quantity, price = u * left;
        const int64_t hold = MailService::SplitHeld(l.holding_mail_id, left);
        if (hold == 0) return Fail(db, r, MSG_BUY_FAILED);
        {
            Stmt st(db, "UPDATE ga_auctions SET quantity = quantity - ?1, buyout_price = buyout_price - ?2, "
                        "start_price = MAX(1, MIN(start_price, buyout_price - ?2)) WHERE id = ?3 AND state = 0");
            if (!st) return Fail(db, r, MSG_BUY_FAILED);
            sqlite3_bind_int64(st.s, 1, left); sqlite3_bind_int64(st.s, 2, price); sqlite3_bind_int64(st.s, 3, l.id);
            if (sqlite3_step(st.s) != SQLITE_DONE || sqlite3_changes(db) == 0) return Fail(db, r, MSG_BUY_FAILED);
        }
        Listing part = l;
        {
            Stmt st(db, "INSERT INTO ga_auctions (seller_character_id, holding_mail_id, item_id, name_msg_id, name, item_type, "
                        "  item_subtype, quality, quantity, start_price, buyout_price, fee, created_at, expire_at) "
                        "SELECT seller_character_id, ?1, item_id, name_msg_id, name, item_type, item_subtype, quality, ?2, ?3, ?3, 0, "
                        "  created_at, expire_at FROM ga_auctions WHERE id = ?4");
            if (!st) return Fail(db, r, MSG_BUY_FAILED);
            sqlite3_bind_int64(st.s, 1, hold); sqlite3_bind_int64(st.s, 2, left);
            sqlite3_bind_int64(st.s, 3, price); sqlite3_bind_int64(st.s, 4, l.id);
            if (sqlite3_step(st.s) != SQLITE_DONE) return Fail(db, r, MSG_BUY_FAILED);
            part.id = sqlite3_last_insert_rowid(db);
        }
        part.holding_mail_id = hold; part.quantity = left;
        part.start_price = part.buyout_price = price;
        if (!Credits(db, character_id, -price)) return Fail(db, r, MSG_NO_CURRENCY);
        if (!Settle(db, part, name, character_id, price, r)) return Fail(db, r, MSG_BUY_FAILED);
        paid += price; left = 0;
    }
    if (left > 0) return Fail(db, r, MSG_GONE);   // sold meanwhile or the price went up
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_BUY_FAILED);
    r.ok = true; r.error_msg_id = 0;
    Logger::Log("mail", "[AH] char %lld market buy item %d x%lld at <= %lld, paid %lld\n",
        (long long)character_id, item_id, (long long)count, (long long)unit, (long long)paid);
    return r;
}

Result Bid(int64_t character_id, int64_t auction_id, int64_t amount) {
    Result r;
    r.error_msg_id = MSG_BID_FAILED;
    if (amount <= 0 || amount > MAX_PRICE) return r;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    Listing l; std::string name;
    if (!LoadActive(db, auction_id, &l, &name)) return Fail(db, r, MSG_GONE);
    if (amount == l.buyout_price) { Exec(db, "ROLLBACK"); return Buyout(character_id, auction_id); }
    if (l.seller_character_id == character_id) return Fail(db, r, MSG_BID_FAILED);
    if (amount > l.buyout_price) return Fail(db, r, MSG_BID_OVER_BUYOUT);
    if (amount < l.start_price) return Fail(db, r, MSG_BID_STEP);
    if (l.bid > 0 && amount * 100 <= l.bid * 105) return Fail(db, r, MSG_BID_STEP);
    if (!Credits(db, character_id, -amount)) return Fail(db, r, MSG_NO_CURRENCY);
    {
        Stmt st(db, "UPDATE ga_auctions SET bid = ?, bidder_character_id = ? WHERE id = ? AND state = 0 AND bid = ?");
        if (!st) return Fail(db, r, MSG_BID_FAILED);
        sqlite3_bind_int64(st.s, 1, amount); sqlite3_bind_int64(st.s, 2, character_id);
        sqlite3_bind_int64(st.s, 3, auction_id); sqlite3_bind_int64(st.s, 4, l.bid);
        if (sqlite3_step(st.s) != SQLITE_DONE || sqlite3_changes(db) == 0) return Fail(db, r, MSG_BID_FAILED);
    }
    if (!RefundBidder(l, name, r)) return Fail(db, r, MSG_BID_FAILED);
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_BID_FAILED);
    r.ok = true; r.error_msg_id = 0; r.auction_id = auction_id;
    Logger::Log("mail", "[AH] char %lld bid %lld on auction %lld\n",
        (long long)character_id, (long long)amount, (long long)auction_id);
    return r;
}

Result Cancel(int64_t character_id, int64_t auction_id) {
    Result r;
    r.error_msg_id = MSG_GONE;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    Listing l; std::string name;
    if (!LoadActive(db, auction_id, &l, &name)) return Fail(db, r, MSG_GONE);
    // a listing with a bid on it stays (the bidder's money is in it)
    if (l.seller_character_id != character_id || l.bid > 0) return Fail(db, r, MSG_GONE);
    if (!SetState(db, l.id, 2)) return Fail(db, r, MSG_GONE);
    if (!MailService::DeliverFromAuction(l.holding_mail_id, character_id, "Auction canceled - " + Quoted(name),
            "Auction canceled for item " + Quoted(name) + ".", 0)) return Fail(db, r, MSG_GONE);
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_GONE);
    r.ok = true; r.error_msg_id = 0; r.auction_id = auction_id;
    r.mailed.push_back(character_id);
    Logger::Log("mail", "[AH] char %lld cancelled auction %lld\n", (long long)character_id, (long long)auction_id);
    return r;
}

std::vector<int64_t> SettleExpired() {
    sqlite3* db = Database::GetConnection();
    std::vector<std::pair<Listing, std::string>> due;
    {
        const std::string sql = std::string("SELECT ") + kCols +
            " FROM ga_auctions WHERE state = 0 AND expire_at <= strftime('%s','now') LIMIT 200";
        Stmt st(db, sql.c_str());
        if (!st) return {};
        while (sqlite3_step(st.s) == SQLITE_ROW) {
            std::string name;
            Listing l = ReadListing(st.s, &name);
            due.emplace_back(l, name);
        }
    }
    std::vector<int64_t> mailed;
    for (const auto& [l, name] : due) {
        Result r;
        if (!Exec(db, "BEGIN IMMEDIATE")) break;
        bool ok;
        if (l.bidder_character_id > 0 && l.bid > 0) {
            ok = Settle(db, l, name, l.bidder_character_id, l.bid, r);
        } else if (l.bidder_character_id < 0 && l.bid > 0 && l.seller_character_id > 0) {
            ok = BotWon(db, l, name, r);
        } else if (l.seller_character_id < 0) {
            ok = SetState(db, l.id, 3) && MailService::DiscardHeld(l.holding_mail_id);
        } else {
            ok = SetState(db, l.id, 3) &&
                 MailService::DeliverFromAuction(l.holding_mail_id, l.seller_character_id,
                     "Auction Expired - " + Quoted(name), "Auction expired for item " + Quoted(name) + ".", 0);
            if (ok) r.mailed.push_back(l.seller_character_id);
        }
        if (!ok || !Exec(db, "COMMIT")) { Exec(db, "ROLLBACK"); continue; }
        mailed.insert(mailed.end(), r.mailed.begin(), r.mailed.end());
        Logger::Log("mail", "[AH] auction %lld ended (%s)\n", (long long)l.id, l.bid > 0 ? "sold" : "expired");
    }
    return mailed;
}

int64_t BotList(int64_t bot_id, int item_id, int64_t quantity, bool component,
                int64_t start_price, int64_t buyout_price, int hours) {
    if (bot_id <= 0 || buyout_price <= 0 || buyout_price > MAX_PRICE) return 0;
    if (start_price <= 0 || start_price > buyout_price) start_price = buyout_price;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return 0;
    const int64_t hold = MailService::HoldNewForAuction(item_id, quantity, component);
    if (hold == 0) { Exec(db, "ROLLBACK"); return 0; }
    Stmt st(db,
        "INSERT INTO ga_auctions (seller_character_id, holding_mail_id, item_id, name_msg_id, name, item_type, "
        "  item_subtype, quality, quantity, start_price, buyout_price, fee, created_at, expire_at) "
        "SELECT ?1, ?2, ?3, COALESCE(i.name_msg_id, 0), COALESCE(NULLIF(i.name_msg_translated, ''), "
        "  (SELECT message FROM asm_data_set_msg_translations WHERE msg_id = i.name_msg_id), ''), "
        "  COALESCE(i.item_type_value_id, 0), COALESCE(i.item_subtype_value_id, 0), COALESCE(i.quality_value_id, 0), "
        "  ?4, ?5, ?6, 0, "
        "  strftime('%s','now'), strftime('%s','now') + ?7 "
        "FROM asm_data_set_items i WHERE i.item_id = ?3 ORDER BY i.id LIMIT 1");
    if (!st) { Exec(db, "ROLLBACK"); return 0; }
    sqlite3_bind_int64(st.s, 1, -bot_id);
    sqlite3_bind_int64(st.s, 2, hold);
    sqlite3_bind_int(st.s, 3, item_id);
    sqlite3_bind_int64(st.s, 4, component ? quantity : 1);
    sqlite3_bind_int64(st.s, 5, start_price);
    sqlite3_bind_int64(st.s, 6, buyout_price);
    sqlite3_bind_int64(st.s, 7, (int64_t)hours * 3600);
    if (sqlite3_step(st.s) != SQLITE_DONE || sqlite3_changes(db) == 0) { Exec(db, "ROLLBACK"); return 0; }
    const int64_t id = sqlite3_last_insert_rowid(db);
    if (!Exec(db, "COMMIT")) { Exec(db, "ROLLBACK"); return 0; }
    return id;
}

bool BotCancel(int64_t auction_id) {
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return false;
    Listing l; std::string name;
    Result r;
    if (!LoadActive(db, auction_id, &l, &name) || l.seller_character_id >= 0 || l.bid > 0 ||
        !SetState(db, l.id, 2) || !MailService::DiscardHeld(l.holding_mail_id) || !Exec(db, "COMMIT")) {
        Fail(db, r, MSG_GONE);
        return false;
    }
    Logger::Log("mail", "[AH] bot listing %lld withdrawn\n", (long long)auction_id);
    return true;
}

Result BotBid(int64_t auction_id, int64_t bot_id, int64_t amount) {
    Result r;
    r.error_msg_id = MSG_BID_FAILED;
    if (bot_id <= 0 || amount <= 0 || amount > MAX_PRICE) return r;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    Listing l; std::string name;
    if (!LoadActive(db, auction_id, &l, &name)) return Fail(db, r, MSG_GONE);
    if (l.seller_character_id <= 0 || l.bidder_character_id < 0) return Fail(db, r, MSG_BID_FAILED);  // players' rows, no bot-vs-bot
    if (amount >= l.buyout_price || amount < l.start_price) return Fail(db, r, MSG_BID_STEP);
    if (l.bid > 0 && amount * 100 <= l.bid * 105) return Fail(db, r, MSG_BID_STEP);
    {
        Stmt st(db, "UPDATE ga_auctions SET bid = ?, bidder_character_id = ? WHERE id = ? AND state = 0 AND bid = ?");
        if (!st) return Fail(db, r, MSG_BID_FAILED);
        sqlite3_bind_int64(st.s, 1, amount); sqlite3_bind_int64(st.s, 2, -bot_id);
        sqlite3_bind_int64(st.s, 3, auction_id); sqlite3_bind_int64(st.s, 4, l.bid);
        if (sqlite3_step(st.s) != SQLITE_DONE || sqlite3_changes(db) == 0) return Fail(db, r, MSG_BID_FAILED);
    }
    if (!RefundBidder(l, name, r)) return Fail(db, r, MSG_BID_FAILED);
    Listing mine = l; mine.bidder_character_id = -bot_id;
    BotLog(db, "bid", mine, amount);
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_BID_FAILED);
    r.ok = true; r.error_msg_id = 0; r.auction_id = auction_id;
    Logger::Log("mail", "[AH] bot %lld bid %lld on auction %lld\n", (long long)bot_id, (long long)amount, (long long)auction_id);
    return r;
}

Result BotBuyout(int64_t auction_id, const std::string& bot_name) {
    Result r;
    r.error_msg_id = MSG_BUY_FAILED;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    Listing l; std::string name;
    if (!LoadActive(db, auction_id, &l, &name)) return Fail(db, r, MSG_GONE);
    if (l.seller_character_id <= 0 || l.bid > 0) return Fail(db, r, MSG_BUY_FAILED);   // players only, no bid war
    if (!SetState(db, l.id, 1)) return Fail(db, r, MSG_BUY_FAILED);
    const int64_t price = l.buyout_price, cut = price * CUT_PERCENT / 100;
    if (!MailService::DeliverFromAuction(0, l.seller_character_id, "Auction Successful - " + Quoted(name),
            "Your auction of " + Quoted(name) + " (x" + std::to_string(l.quantity) + ") has sold to " + bot_name +
            " for " + std::to_string(price) + " credits (house cut " + std::to_string(cut) + ").", price - cut))
        return Fail(db, r, MSG_BUY_FAILED);
    if (!MailService::DiscardHeld(l.holding_mail_id)) return Fail(db, r, MSG_BUY_FAILED);
    if (!Exec(db, "COMMIT")) return Fail(db, r, MSG_BUY_FAILED);
    r.ok = true; r.error_msg_id = 0; r.auction_id = auction_id;
    r.mailed.push_back(l.seller_character_id);
    Logger::Log("mail", "[AH] bot %s bought auction %lld from char %lld for %lld\n",
        bot_name.c_str(), (long long)auction_id, (long long)l.seller_character_id, (long long)price);
    return r;
}

} // namespace AuctionService
