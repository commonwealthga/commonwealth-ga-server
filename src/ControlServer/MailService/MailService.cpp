#include "src/ControlServer/MailService/MailService.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <ctime>

namespace MailService {

namespace {

struct Stmt {
    sqlite3_stmt* s = nullptr;
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("mail", "[Mail] prepare failed: %s (%s)\n", sqlite3_errmsg(db), sql);
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    explicit operator bool() const { return s != nullptr; }
};

std::string ColText(sqlite3_stmt* s, int i) {
    const unsigned char* t = sqlite3_column_text(s, i);
    return t ? reinterpret_cast<const char*>(t) : std::string();
}

const char* kRowCols =
    "id, from_character_id, from_name, subject, body, credits, mail_type, source, state, sent_at, expire_at";

MailRow ReadRow(sqlite3_stmt* s) {
    MailRow r;
    r.id                = sqlite3_column_int64(s, 0);
    r.from_character_id = sqlite3_column_int64(s, 1);
    r.from_name         = ColText(s, 2);
    r.subject           = ColText(s, 3);
    r.body              = ColText(s, 4);
    r.credits           = sqlite3_column_int64(s, 5);
    r.mail_type         = (uint32_t)sqlite3_column_int(s, 6);
    r.source            = (uint32_t)sqlite3_column_int(s, 7);
    r.state             = (uint32_t)sqlite3_column_int(s, 8);
    r.sent_at           = sqlite3_column_int64(s, 9);
    r.expire_at         = sqlite3_column_int64(s, 10);
    return r;
}

bool Exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("mail", "[Mail] exec failed: %s (%s)\n", err ? err : "?", sql);
        sqlite3_free(err);
        return false;
    }
    return true;
}

std::vector<MailItem> LoadItems(sqlite3* db, int64_t mail_id);

bool LoadRow(sqlite3* db, int64_t character_id, int64_t mail_id, MailRow* out) {
    std::string sql = std::string("SELECT ") + kRowCols +
        " FROM ga_mail WHERE id = ? AND to_character_id = ? AND deleted_at = 0";
    Stmt st(db, sql.c_str());
    if (!st) return false;
    sqlite3_bind_int64(st.s, 1, mail_id);
    sqlite3_bind_int64(st.s, 2, character_id);
    if (sqlite3_step(st.s) != SQLITE_ROW) return false;
    if (out) { *out = ReadRow(st.s); out->items = LoadItems(db, mail_id); }
    return true;
}

int64_t InsertMail(sqlite3* db, int64_t to_char, int64_t from_char, const std::string& from_name,
                   const std::string& subject, const std::string& body, int64_t credits,
                   uint32_t type, uint32_t source) {
    Stmt st(db,
        "INSERT INTO ga_mail (to_character_id, from_character_id, from_name, subject, body, credits, "
        " mail_type, source, state, sent_at, expire_at) "
        "VALUES (?,?,?,?,?,?,?,?,?, strftime('%s','now'), strftime('%s','now') + ?)");
    if (!st) return 0;
    sqlite3_bind_int64(st.s, 1, to_char);
    sqlite3_bind_int64(st.s, 2, from_char);
    sqlite3_bind_text(st.s, 3, from_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 4, subject.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 5, body.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.s, 6, credits);
    sqlite3_bind_int(st.s, 7, (int)type);
    sqlite3_bind_int(st.s, 8, (int)source);
    sqlite3_bind_int(st.s, 9, (int)STATE_POSTED);
    sqlite3_bind_int64(st.s, 10, (int64_t)EXPIRE_DAYS * 86400);
    if (sqlite3_step(st.s) != SQLITE_DONE) return 0;
    return sqlite3_last_insert_rowid(db);
}

std::vector<MailItem> LoadItems(sqlite3* db, int64_t mail_id) {
    std::vector<MailItem> items;
    Stmt st(db,
        "SELECT m.id, m.inventory_id, m.item_id, m.quantity, COALESCE(p.device_id, 0), "
        "  CASE WHEN m.inventory_id > 0 THEN COALESCE(p.quality, 0) "
        "       ELSE COALESCE((SELECT quality_value_id FROM asm_data_set_items WHERE item_id = m.item_id), 0) END, "
        "  COALESCE(p.mod_effect_group_ids, ''), COALESCE(p.oc, 0) "
        "FROM ga_mail_items m LEFT JOIN ga_players_inventory p ON p.id = m.inventory_id AND p.user_id = 0 "
        "WHERE m.mail_id = ? AND m.taken = 0 ORDER BY m.id");
    if (!st) return items;
    sqlite3_bind_int64(st.s, 1, mail_id);
    while (sqlite3_step(st.s) == SQLITE_ROW) {
        MailItem it;
        it.id           = sqlite3_column_int64(st.s, 0);
        it.inventory_id = sqlite3_column_int64(st.s, 1);
        it.item_id      = sqlite3_column_int(st.s, 2);
        it.quantity     = sqlite3_column_int64(st.s, 3);
        it.device_id    = sqlite3_column_int(st.s, 4);
        it.quality      = sqlite3_column_int(st.s, 5);
        it.mods         = ColText(st.s, 6);
        it.oc           = sqlite3_column_int(st.s, 7);
        items.push_back(it);
    }
    return items;
}

// Hands the untaken attachments of one mail to another mail (returns).
bool MoveItems(sqlite3* db, int64_t from_mail, int64_t to_mail) {
    Stmt st(db, "UPDATE ga_mail_items SET mail_id = ? WHERE mail_id = ? AND taken = 0");
    if (!st) return false;
    sqlite3_bind_int64(st.s, 1, to_mail);
    sqlite3_bind_int64(st.s, 2, from_mail);
    return sqlite3_step(st.s) == SQLITE_DONE;
}

int64_t Scalar(sqlite3* db, const char* sql, int64_t a, int64_t b = 0) {
    Stmt st(db, sql);
    if (!st) return 0;
    sqlite3_bind_int64(st.s, 1, a);
    if (sqlite3_bind_parameter_count(st.s) >= 2) sqlite3_bind_int64(st.s, 2, b);
    return sqlite3_step(st.s) == SQLITE_ROW ? sqlite3_column_int64(st.s, 0) : 0;
}

std::string NameOf(sqlite3* db, int64_t character_id) {
    Stmt st(db, "SELECT u.username FROM ga_characters c JOIN ga_users u ON u.id = c.user_id WHERE c.id = ?");
    if (!st) return "";
    sqlite3_bind_int64(st.s, 1, character_id);
    return sqlite3_step(st.s) == SQLITE_ROW ? ColText(st.s, 0) : std::string();
}

bool Step(sqlite3* db, Stmt& st, bool need_change = true) {
    return st && sqlite3_step(st.s) == SQLITE_DONE && (!need_change || sqlite3_changes(db) > 0);
}

// Escrows one attachment of the sender (inside the caller's transaction).
// Refused: not theirs, equipped, starter kit, bound, other class, empty stack.
bool EscrowOne(sqlite3* db, int64_t user_id, int to_profile, int64_t mail_id, const Attachment& a,
               SendResult& r) {
    if (a.inventory_id >= (int64_t)COMPONENT_INV_BASE) {
        const int item_id = (int)(a.inventory_id - COMPONENT_INV_BASE);
        const int64_t qty = a.count;
        if (item_id <= 0 || qty <= 0) return false;
        const int64_t bind = Scalar(db, "SELECT COALESCE(item_bind_type_value_id,0) FROM asm_data_set_items WHERE item_id = ?", item_id);
        if (bind == BIND_ON_ACQUIRE || bind == BIND_TO_AGENCY) return false;
        {
            Stmt st(db, "UPDATE ga_user_components SET quantity = quantity - ?1, updated_at = strftime('%s','now') "
                        "WHERE user_id = ?2 AND item_id = ?3 AND quantity >= ?1");
            if (!st) return false;
            sqlite3_bind_int64(st.s, 1, qty); sqlite3_bind_int64(st.s, 2, user_id); sqlite3_bind_int(st.s, 3, item_id);
            if (!Step(db, st)) return false;
        }
        {
            Stmt st(db, "DELETE FROM ga_user_components WHERE user_id = ? AND item_id = ? AND quantity <= 0");
            if (st) {
                sqlite3_bind_int64(st.s, 1, user_id); sqlite3_bind_int(st.s, 2, item_id);
                if (Step(db, st)) r.depleted_components.push_back(item_id);
            }
        }
        Stmt ins(db, "INSERT INTO ga_mail_items (mail_id, inventory_id, item_id, quantity) VALUES (?, 0, ?, ?)");
        if (!ins) return false;
        sqlite3_bind_int64(ins.s, 1, mail_id); sqlite3_bind_int(ins.s, 2, item_id); sqlite3_bind_int64(ins.s, 3, qty);
        if (!Step(db, ins)) return false;
        r.components_changed = true;
        return true;
    }
    // gear / flair row
    int item_id = 0, profile = 0, equipped = 0;
    int64_t sell_value = 0, sell_cur = 0, bind = 0;
    {
        Stmt st(db,
            "SELECT p.item_id, p.profile_id, COALESCE(p.sell_value,0), COALESCE(p.sell_currency,0), "
            "  (SELECT COUNT(*) FROM ga_character_devices d JOIN ga_characters c ON c.id = d.character_id "
            "   WHERE d.inventory_id = p.id AND c.user_id = p.user_id), "
            "  COALESCE((SELECT MAX(item_bind_type_value_id) FROM asm_data_set_items i "
            "            WHERE (p.item_id > 0 AND i.item_id = p.item_id) "
            "               OR (p.item_id = 0 AND p.device_id > 0 AND i.ref_device_id = p.device_id)), 0) "
            "FROM ga_players_inventory p WHERE p.id = ? AND p.user_id = ?");
        if (!st) return false;
        sqlite3_bind_int64(st.s, 1, a.inventory_id); sqlite3_bind_int64(st.s, 2, user_id);
        if (sqlite3_step(st.s) != SQLITE_ROW) return false;
        item_id = sqlite3_column_int(st.s, 0); profile = sqlite3_column_int(st.s, 1);
        sell_value = sqlite3_column_int64(st.s, 2); sell_cur = sqlite3_column_int64(st.s, 3);
        equipped = sqlite3_column_int(st.s, 4); bind = sqlite3_column_int64(st.s, 5);
    }
    if (equipped > 0) return false;
    if (item_id == 0 && sell_value <= 0 && sell_cur == 0) return false;   // starter kit
    if (bind == BIND_ON_ACQUIRE || bind == BIND_TO_AGENCY) return false;
    if (to_profile >= 0 && profile != 0 && profile != to_profile) return false;  // class gear for another class (AH: -1 = any)
    Stmt ins(db, "INSERT INTO ga_mail_items (mail_id, inventory_id, item_id, quantity) VALUES (?, ?, ?, 1)");
    if (!ins) return false;
    sqlite3_bind_int64(ins.s, 1, mail_id); sqlite3_bind_int64(ins.s, 2, a.inventory_id); sqlite3_bind_int(ins.s, 3, item_id);
    if (!Step(db, ins)) return false;
    const int64_t mi = sqlite3_last_insert_rowid(db);
    Stmt park(db, "UPDATE ga_players_inventory SET user_id = 0, stock_n = ? WHERE id = ? AND user_id = ?");
    if (!park) return false;
    sqlite3_bind_int64(park.s, 1, -mi); sqlite3_bind_int64(park.s, 2, a.inventory_id); sqlite3_bind_int64(park.s, 3, user_id);
    if (!Step(db, park)) return false;
    r.removed_rows.push_back(a.inventory_id);
    return true;
}

// Expired mail of one character: credits go home as a Returned mail (a
// returned mail that expires again is simply dropped), everything else is deleted.
void ExpireFor(sqlite3* db, int64_t character_id) {
    std::vector<MailRow> expired;
    {
        std::string sql = std::string("SELECT ") + kRowCols +
            " FROM ga_mail WHERE to_character_id = ? AND deleted_at = 0 AND expire_at < strftime('%s','now')";
        Stmt st(db, sql.c_str());
        if (!st) return;
        sqlite3_bind_int64(st.s, 1, character_id);
        while (sqlite3_step(st.s) == SQLITE_ROW) expired.push_back(ReadRow(st.s));
    }
    if (expired.empty()) return;
    if (!Exec(db, "BEGIN IMMEDIATE")) return;
    for (const auto& m : expired) {
        const bool has_items = Scalar(db, "SELECT COUNT(*) FROM ga_mail_items WHERE mail_id = ? AND taken = 0", m.id) > 0;
        const int64_t value_credits = m.mail_type == TYPE_COD ? 0 : m.credits;  // a COD price is not money in the mail
        const bool returned = m.source == SOURCE_RETURNED || m.mail_type == TYPE_COD_RETURNED;
        // returned mail that still holds credits/items never expires (nothing is ever lost)
        if (returned && (value_credits > 0 || has_items)) continue;
        Stmt del(db, "UPDATE ga_mail SET deleted_at = strftime('%s','now') WHERE id = ? AND deleted_at = 0");
        if (!del) continue;
        sqlite3_bind_int64(del.s, 1, m.id);
        if (sqlite3_step(del.s) != SQLITE_DONE || sqlite3_changes(db) == 0) continue;
        if ((value_credits > 0 || has_items) && m.from_character_id > 0) {
            const int64_t back = InsertMail(db, m.from_character_id, character_id, "", m.subject /* client adds Returned: */,
                       m.body, value_credits, has_items && m.mail_type == TYPE_COD ? TYPE_COD_RETURNED : TYPE_NORMAL,
                       SOURCE_RETURNED);
            if (back && has_items) MoveItems(db, m.id, back);
            Logger::Log("mail", "[Mail] expired mail %lld -> %lld credits%s returned to char %lld\n",
                (long long)m.id, (long long)value_credits, has_items ? " + items" : "", (long long)m.from_character_id);
        }
    }
    Exec(db, "COMMIT");
}

} // namespace

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    Exec(db,
        "CREATE TABLE IF NOT EXISTS ga_mail ("
        "  id                INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  to_character_id   INTEGER NOT NULL,"
        "  from_character_id INTEGER NOT NULL DEFAULT 0,"
        "  from_name         TEXT NOT NULL DEFAULT '',"
        "  subject           TEXT NOT NULL DEFAULT '',"
        "  body              TEXT NOT NULL DEFAULT '',"
        "  credits           INTEGER NOT NULL DEFAULT 0,"
        "  mail_type         INTEGER NOT NULL DEFAULT 1328,"
        "  source            INTEGER NOT NULL DEFAULT 1070,"
        "  state             INTEGER NOT NULL DEFAULT 1073,"
        "  sent_at           INTEGER NOT NULL DEFAULT 0,"
        "  expire_at         INTEGER NOT NULL DEFAULT 0,"
        "  deleted_at        INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS ga_mail_to ON ga_mail(to_character_id, deleted_at);"
        "CREATE TABLE IF NOT EXISTS ga_mail_items ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  mail_id      INTEGER NOT NULL,"
        "  inventory_id INTEGER NOT NULL DEFAULT 0,"   // escrowed ga_players_inventory row (user_id 0), 0 = component
        "  item_id      INTEGER NOT NULL DEFAULT 0,"
        "  quantity     INTEGER NOT NULL DEFAULT 1,"
        "  taken        INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS ga_mail_items_mail ON ga_mail_items(mail_id, taken);");
    // 0958: items taken from mail before the accept tagged the agent -> the mail's recipient agent (same account).
    Exec(db,
        "UPDATE ga_players_inventory SET character_id = (SELECT m.to_character_id FROM ga_mail_items mi "
        "  JOIN ga_mail m ON m.id = mi.mail_id JOIN ga_characters c ON c.id = m.to_character_id "
        "  WHERE mi.inventory_id = ga_players_inventory.id AND mi.taken = 1 AND c.user_id = ga_players_inventory.user_id "
        "  ORDER BY mi.id DESC LIMIT 1) "
        "WHERE character_id = 0 AND user_id > 0 AND id IN (SELECT mi.inventory_id FROM ga_mail_items mi "
        "  JOIN ga_mail m ON m.id = mi.mail_id JOIN ga_characters c ON c.id = m.to_character_id "
        "  WHERE mi.taken = 1 AND mi.inventory_id > 0 AND c.user_id = ga_players_inventory.user_id);");
}

std::vector<MailRow> List(int64_t character_id) {
    std::vector<MailRow> rows;
    if (character_id <= 0) return rows;
    sqlite3* db = Database::GetConnection();
    ExpireFor(db, character_id);
    std::string sql = std::string("SELECT ") + kRowCols +
        " FROM ga_mail WHERE to_character_id = ? AND deleted_at = 0 ORDER BY id DESC LIMIT ?";
    Stmt st(db, sql.c_str());
    if (!st) return rows;
    sqlite3_bind_int64(st.s, 1, character_id);
    sqlite3_bind_int(st.s, 2, MAX_LIST);
    while (sqlite3_step(st.s) == SQLITE_ROW) rows.push_back(ReadRow(st.s));
    for (auto& m : rows) m.items = LoadItems(db, m.id);
    return rows;
}

uint32_t UnreadCount(int64_t character_id) {
    if (character_id <= 0) return 0;
    Stmt st(Database::GetConnection(),
        "SELECT COUNT(*) FROM ga_mail WHERE to_character_id = ? AND deleted_at = 0 AND state = 1073 "
        "AND expire_at >= strftime('%s','now')");
    if (!st) return 0;
    sqlite3_bind_int64(st.s, 1, character_id);
    return sqlite3_step(st.s) == SQLITE_ROW ? (uint32_t)sqlite3_column_int(st.s, 0) : 0;
}

SendResult Send(int64_t from_character_id, const std::string& to_name,
                const std::string& subject_in, const std::string& body_in,
                int64_t credits, uint32_t mail_type, const std::vector<Attachment>& attachments) {
    SendResult r;
    r.error_msg_id = MSG_SEND_FAILED;
    if (from_character_id <= 0) return r;
    if (to_name.empty()) { r.error_msg_id = MSG_NO_RECIPIENT; return r; }
    if (mail_type != TYPE_NORMAL && mail_type != TYPE_COD) return r;
    if (credits < 0 || credits > MAX_CREDITS) return r;
    if (attachments.size() > MAX_ATTACHMENTS) { r.error_msg_id = MSG_TOO_MANY_ATT; return r; }
    // COD = items + a price; nothing to pay for without items
    if (mail_type == TYPE_COD && (attachments.empty() || credits <= 0)) return r;
    for (size_t i = 0; i < attachments.size(); ++i)
        for (size_t j = i + 1; j < attachments.size(); ++j)
            if (attachments[i].inventory_id == attachments[j].inventory_id) return r;

    const std::string subject = subject_in.substr(0, MAX_SUBJECT);
    const std::string body    = body_in.substr(0, MAX_BODY);
    sqlite3* db = Database::GetConnection();

    // recipient = the account's live agent (agent name = account name)
    std::string from_name;
    int64_t from_user = 0;
    int to_profile = 0;
    {
        Stmt st(db,
            "SELECT c.id, u.id, c.profile_id FROM ga_users u JOIN ga_characters c ON c.user_id = u.id "
            "WHERE u.username = ? COLLATE NOCASE AND c.deleted_at = 0 ORDER BY c.id LIMIT 1");
        if (!st) return r;
        sqlite3_bind_text(st.s, 1, to_name.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(st.s) == SQLITE_ROW) {
            r.to_character_id = sqlite3_column_int64(st.s, 0);
            r.to_user_id      = sqlite3_column_int64(st.s, 1);
            to_profile        = sqlite3_column_int(st.s, 2);
        }
    }
    {
        Stmt st(db,
            "SELECT u.username, u.id FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
            "WHERE c.id = ? AND c.deleted_at = 0");
        if (!st) return r;
        sqlite3_bind_int64(st.s, 1, from_character_id);
        if (sqlite3_step(st.s) == SQLITE_ROW) { from_name = ColText(st.s, 0); from_user = sqlite3_column_int64(st.s, 1); }
    }
    if (r.to_character_id == 0) { r.error_msg_id = MSG_NO_RECIPIENT; return r; }
    if (from_name.empty() || r.to_character_id == from_character_id) return r;
    // an ignored sender's mail is refused (same rule as whispers)
    if (Database::IsIgnoring(to_name, from_name)) return r;

    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    // a COD price is paid by the reader, not taken from the sender now
    if (credits > 0 && mail_type == TYPE_NORMAL) {
        Stmt st(db, "UPDATE ga_characters SET credits = credits - ?1 "
                    "WHERE id = ?2 AND deleted_at = 0 AND credits >= ?1");
        if (!st) { Exec(db, "ROLLBACK"); return r; }
        sqlite3_bind_int64(st.s, 1, credits);
        sqlite3_bind_int64(st.s, 2, from_character_id);
        if (sqlite3_step(st.s) != SQLITE_DONE || sqlite3_changes(db) == 0) {
            Exec(db, "ROLLBACK");
            return r;  // not enough credits
        }
    }
    r.mail_id = InsertMail(db, r.to_character_id, from_character_id, from_name, subject, body,
                           credits, mail_type, SOURCE_PERSON);
    if (r.mail_id == 0) { Exec(db, "ROLLBACK"); return r; }
    for (const auto& a : attachments) {
        if (!EscrowOne(db, from_user, to_profile, r.mail_id, a, r)) {
            Exec(db, "ROLLBACK");
            Logger::Log("mail", "[Mail] char %lld send refused: attachment inv %lld x%lld not mailable\n",
                (long long)from_character_id, (long long)a.inventory_id, (long long)a.count);
            r.mail_id = 0; r.removed_rows.clear(); r.components_changed = false; r.depleted_components.clear();
            return r;
        }
    }
    if (!Exec(db, "COMMIT")) {
        Exec(db, "ROLLBACK"); r.mail_id = 0; r.removed_rows.clear(); r.components_changed = false; r.depleted_components.clear(); return r;
    }
    r.ok = true;
    r.error_msg_id = 0;
    Logger::Log("mail", "[Mail] char %lld -> %s (char %lld) mail %lld type %u credits %lld items %zu\n",
        (long long)from_character_id, to_name.c_str(), (long long)r.to_character_id,
        (long long)r.mail_id, mail_type, (long long)credits, attachments.size());
    return r;
}

int64_t HoldForAuction(int64_t user_id, const Attachment& a, SendResult& r) {
    sqlite3* db = Database::GetConnection();
    Stmt st(db,
        "INSERT INTO ga_mail (to_character_id, from_name, subject, mail_type, source, state, sent_at, expire_at) "
        "VALUES (0, 'Auction House', 'auction escrow', ?, ?, ?, strftime('%s','now'), 4102444800)");
    if (!st) return 0;
    sqlite3_bind_int(st.s, 1, (int)TYPE_NORMAL);
    sqlite3_bind_int(st.s, 2, (int)SOURCE_AUCTION);
    sqlite3_bind_int(st.s, 3, (int)STATE_POSTED);
    if (!Step(db, st)) return 0;
    const int64_t hold = sqlite3_last_insert_rowid(db);
    return EscrowOne(db, user_id, -1, hold, a, r) ? hold : 0;
}

int64_t HoldNewForAuction(int item_id, int64_t quantity, bool component) {
    sqlite3* db = Database::GetConnection();
    if (item_id <= 0 || quantity <= 0) return 0;
    {
        Stmt st(db,
            "INSERT INTO ga_mail (to_character_id, from_name, subject, mail_type, source, state, sent_at, expire_at) "
            "VALUES (0, 'Auction House', 'auction escrow', ?, ?, ?, strftime('%s','now'), 4102444800)");
        if (!st) return 0;
        sqlite3_bind_int(st.s, 1, (int)TYPE_NORMAL);
        sqlite3_bind_int(st.s, 2, (int)SOURCE_AUCTION);
        sqlite3_bind_int(st.s, 3, (int)STATE_POSTED);
        if (!Step(db, st)) return 0;
    }
    const int64_t hold = sqlite3_last_insert_rowid(db);
    {
        Stmt ins(db, "INSERT INTO ga_mail_items (mail_id, inventory_id, item_id, quantity) VALUES (?, 0, ?, ?)");
        if (!ins) return 0;
        sqlite3_bind_int64(ins.s, 1, hold); sqlite3_bind_int(ins.s, 2, item_id);
        sqlite3_bind_int64(ins.s, 3, component ? quantity : 1);
        if (!Step(db, ins)) return 0;
    }
    if (component) return hold;
    const int64_t mi = sqlite3_last_insert_rowid(db);
    {
        // row shape = Database::GrantLootItems (unpriced = unsellable), parked like an escrowed row;
        // Weapons/jetpacks (item id = device id) get the device row + equip slots + quality
        Stmt st(db,
            "INSERT INTO ga_players_inventory "
            "(user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n, "
            " sell_value, sell_currency) "
            "VALUES (0, 0, CASE WHEN COALESCE((SELECT ref_device_id FROM asm_data_set_items WHERE item_id = ?1), 0) > 0 "
            "   THEN (SELECT ref_device_id FROM asm_data_set_items WHERE item_id = ?1) "
            "   WHEN (SELECT slot_used_value_id FROM asm_data_set_devices WHERE device_id = ?1 LIMIT 1) IN (388,389,390,476,806,981) "
            "   THEN ?1 ELSE 0 END, "
            " CASE WHEN (SELECT slot_used_value_id FROM asm_data_set_devices WHERE device_id = ?1 LIMIT 1) IN (388,389,390,476,806,981) "
            "   THEN COALESCE((SELECT quality_value_id FROM asm_data_set_items WHERE item_id = ?1), 0) ELSE 0 END, '', 0, "
            " CASE (SELECT item_type_value_id FROM asm_data_set_items WHERE item_id = ?1) WHEN 1412 THEN '501' "
            "   WHEN 1020 THEN '996,997,998,999,1000' "
            "   ELSE CASE (SELECT slot_used_value_id FROM asm_data_set_devices WHERE device_id = ?1 LIMIT 1) "
            "     WHEN 389 THEN '221' WHEN 388 THEN '198' WHEN 981 THEN '200' WHEN 806 THEN '201' "
            "     WHEN 476 THEN '386' WHEN 390 THEN '203,204,385' ELSE '' END END, ?1, ?2, "
            " COALESCE((SELECT price / 2 FROM asm_data_set_prices WHERE item_id = ?1 AND currency_type_value_id = 1603 ORDER BY id LIMIT 1), 0), "
            " CASE WHEN EXISTS (SELECT 1 FROM asm_data_set_prices WHERE item_id = ?1 AND currency_type_value_id = 1603) "
            "      THEN 1603 ELSE 0 END)");
        if (!st) return 0;
        sqlite3_bind_int(st.s, 1, item_id); sqlite3_bind_int64(st.s, 2, -mi);
        if (!Step(db, st)) return 0;
    }
    const int64_t inv = sqlite3_last_insert_rowid(db);
    Stmt up(db, "UPDATE ga_mail_items SET inventory_id = ? WHERE id = ?");
    if (!up) return 0;
    sqlite3_bind_int64(up.s, 1, inv); sqlite3_bind_int64(up.s, 2, mi);
    return Step(db, up) ? hold : 0;
}

int64_t SplitHeld(int64_t holding_mail_id, int64_t quantity) {
    sqlite3* db = Database::GetConnection();
    Stmt st(db, "SELECT id, item_id FROM ga_mail_items WHERE mail_id = ? AND taken = 0 AND inventory_id = 0 AND quantity > ? LIMIT 1");
    if (!st || quantity <= 0) return 0;
    sqlite3_bind_int64(st.s, 1, holding_mail_id); sqlite3_bind_int64(st.s, 2, quantity);
    if (sqlite3_step(st.s) != SQLITE_ROW) return 0;
    const int64_t row = sqlite3_column_int64(st.s, 0);
    const int item_id = sqlite3_column_int(st.s, 1);
    const int64_t hold = HoldNewForAuction(item_id, quantity, true);
    if (hold == 0) return 0;
    Stmt up(db, "UPDATE ga_mail_items SET quantity = quantity - ? WHERE id = ?");
    if (!up) return 0;
    sqlite3_bind_int64(up.s, 1, quantity); sqlite3_bind_int64(up.s, 2, row);
    return Step(db, up) ? hold : 0;
}

bool DiscardHeld(int64_t holding_mail_id) {
    sqlite3* db = Database::GetConnection();
    const char* sqls[] = {
        "DELETE FROM ga_players_inventory WHERE user_id = 0 AND id IN "
        "  (SELECT inventory_id FROM ga_mail_items WHERE mail_id = ?1 AND taken = 0 AND inventory_id > 0)",
        "UPDATE ga_mail_items SET taken = 1 WHERE mail_id = ?1 AND taken = 0",
        "UPDATE ga_mail SET deleted_at = strftime('%s','now') WHERE id = ?1 AND to_character_id = 0",
    };
    for (const char* sql : sqls) {
        Stmt st(db, sql);
        if (!st) return false;
        sqlite3_bind_int64(st.s, 1, holding_mail_id);
        if (!Step(db, st, false)) return false;
    }
    return true;
}

std::vector<MailItem> HeldItems(int64_t holding_mail_id) {
    return LoadItems(Database::GetConnection(), holding_mail_id);
}

int64_t DeliverFromAuction(int64_t holding_mail_id, int64_t to_character_id,
                           const std::string& subject, const std::string& body, int64_t credits) {
    sqlite3* db = Database::GetConnection();
    const int64_t id = InsertMail(db, to_character_id, 0, "Auction House", subject.substr(0, MAX_SUBJECT),
                                  body.substr(0, MAX_BODY), credits, TYPE_NORMAL, SOURCE_SYSTEM);
    if (id == 0) return 0;
    if (holding_mail_id > 0) {
        if (!MoveItems(db, holding_mail_id, id)) return 0;
        Stmt del(db, "UPDATE ga_mail SET deleted_at = strftime('%s','now') WHERE id = ? AND to_character_id = 0");
        if (del) { sqlite3_bind_int64(del.s, 1, holding_mail_id); sqlite3_step(del.s); }
    }
    return id;
}

int64_t DeliverSystem(int64_t to_character_id, const std::string& from_name,
                      const std::string& subject, const std::string& body,
                      int64_t credits, const std::vector<NewItem>& items) {
    sqlite3* db = Database::GetConnection();
    if (to_character_id <= 0 || credits < 0 || credits > MAX_CREDITS || items.size() > MAX_ATTACHMENTS) return 0;
    if (Scalar(db, "SELECT COUNT(*) FROM ga_characters WHERE id = ? AND deleted_at = 0", to_character_id) != 1) return 0;
    if (!Exec(db, "BEGIN IMMEDIATE")) return 0;
    auto fail = [&](const char* why) -> int64_t {
        Logger::Log("mail", "[Mail] system mail to char %lld refused: %s (%s)\n", (long long)to_character_id, why, sqlite3_errmsg(db));
        Exec(db, "ROLLBACK"); return 0;
    };
    const std::string from = from_name.empty() ? std::string("System") : from_name.substr(0, 32);
    const int64_t id = InsertMail(db, to_character_id, 0, from, subject.substr(0, MAX_SUBJECT),
                                  body.substr(0, MAX_BODY), credits, TYPE_NORMAL, SOURCE_SYSTEM);
    if (id == 0) return fail("insert");
    size_t rows = 0;
    for (const NewItem& it : items) {
        const int64_t type = Scalar(db, "SELECT COALESCE((SELECT item_type_value_id FROM asm_data_set_items "
                                        "WHERE item_id = ?), -1)", it.item_id);
        if (type < 0 || it.quantity <= 0 || it.quantity > 1000) return fail("unknown item");
        const bool component = type == 1171;
        const int64_t copies = component ? 1 : it.quantity;
        if ((rows += copies) > MAX_ATTACHMENTS) return fail("too many items");
        for (int64_t n = 0; n < copies; ++n) {
            const int64_t hold = HoldNewForAuction(it.item_id, component ? it.quantity : 1, component);
            if (hold == 0 || !MoveItems(db, hold, id)) return fail("new item");
            Stmt del(db, "DELETE FROM ga_mail WHERE id = ? AND to_character_id = 0");
            if (!del) return fail("prepare");
            sqlite3_bind_int64(del.s, 1, hold);
            if (!Step(db, del)) return fail("drop holder");
        }
    }
    if (!Exec(db, "COMMIT")) return fail("commit");
    return id;
}

bool MarkRead(int64_t character_id, int64_t mail_id, MailRow* out) {
    sqlite3* db = Database::GetConnection();
    {
        Stmt st(db, "UPDATE ga_mail SET state = 1074 WHERE id = ? AND to_character_id = ? "
                    "AND deleted_at = 0 AND state = 1073");
        if (!st) return false;
        sqlite3_bind_int64(st.s, 1, mail_id);
        sqlite3_bind_int64(st.s, 2, character_id);
        sqlite3_step(st.s);
    }
    return LoadRow(db, character_id, mail_id, out);
}

AcceptResult Accept(int64_t character_id, int64_t mail_id) {
    AcceptResult r;
    r.error_msg_id = MSG_ACCEPT_FAILED;
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    auto fail = [&](uint32_t msg) { Exec(db, "ROLLBACK"); r.error_msg_id = msg; r.gained_rows.clear(); return r; };
    MailRow m;
    if (!LoadRow(db, character_id, mail_id, &m)) return fail(MSG_ACCEPT_FAILED);
    const bool cod = m.mail_type == TYPE_COD;
    if (m.items.empty() && (cod || m.credits <= 0)) return fail(MSG_ALREADY_DONE);
    const int64_t user_id = Scalar(db, "SELECT user_id FROM ga_characters WHERE id = ? AND deleted_at = 0", character_id);
    if (user_id <= 0) return fail(MSG_ACCEPT_FAILED);

    if (cod && m.credits > 0) {
        // the reader pays the price; it goes to the sender as a mail
        Stmt pay(db, "UPDATE ga_characters SET credits = credits - ?1 WHERE id = ?2 AND deleted_at = 0 AND credits >= ?1");
        if (!pay) return fail(MSG_ACCEPT_FAILED);
        sqlite3_bind_int64(pay.s, 1, m.credits); sqlite3_bind_int64(pay.s, 2, character_id);
        if (!Step(db, pay)) return fail(MSG_ACCEPT_FAILED);   // not enough credits
        const std::string reader = NameOf(db, character_id);
        if (m.from_character_id <= 0 ||
            !InsertMail(db, m.from_character_id, character_id, reader, "COD paid: " + m.subject,
                        reader + " paid for your COD mail.", m.credits, TYPE_NORMAL, SOURCE_PERSON))
            return fail(MSG_ACCEPT_FAILED);
        r.credits = -m.credits;
        r.cod_paid_to_character = m.from_character_id;
    } else if (!cod && m.credits > 0) {
        Stmt st(db, "UPDATE ga_characters SET credits = credits + ? WHERE id = ? AND deleted_at = 0");
        if (!st) return fail(MSG_ACCEPT_FAILED);
        sqlite3_bind_int64(st.s, 1, m.credits); sqlite3_bind_int64(st.s, 2, character_id);
        if (!Step(db, st)) return fail(MSG_ACCEPT_FAILED);
        r.credits = m.credits;
    }
    for (const auto& it : m.items) {
        if (it.inventory_id > 0) {
            Stmt st(db,
                "UPDATE ga_players_inventory SET user_id = ?1, character_id = ?3, stock_n = "  /* 0958: tag the taking agent */
                " (SELECT COALESCE(MAX(p2.stock_n), -1) + 1 FROM ga_players_inventory p2 "
                "  WHERE p2.user_id = ?1 AND p2.item_id = ga_players_inventory.item_id) "
                "WHERE id = ?2 AND user_id = 0");
            if (!st) return fail(MSG_ACCEPT_FAILED);
            sqlite3_bind_int64(st.s, 1, user_id); sqlite3_bind_int64(st.s, 2, it.inventory_id);
            sqlite3_bind_int64(st.s, 3, character_id);
            if (!Step(db, st)) return fail(MSG_ACCEPT_FAILED);
            r.gained_rows.push_back(it.inventory_id);
        } else {
            Stmt st(db,
                "INSERT INTO ga_user_components (user_id, item_id, quantity, updated_at) "
                "VALUES (?, ?, ?, strftime('%s','now')) "
                "ON CONFLICT(user_id, item_id) DO UPDATE SET "
                "  quantity = quantity + excluded.quantity, updated_at = excluded.updated_at");
            if (!st) return fail(MSG_ACCEPT_FAILED);
            sqlite3_bind_int64(st.s, 1, user_id); sqlite3_bind_int(st.s, 2, it.item_id);
            sqlite3_bind_int64(st.s, 3, it.quantity);
            if (!Step(db, st)) return fail(MSG_ACCEPT_FAILED);
            r.components_changed = true;
        }
        Stmt tk(db, "UPDATE ga_mail_items SET taken = 1 WHERE id = ? AND taken = 0");
        if (!tk) return fail(MSG_ACCEPT_FAILED);
        sqlite3_bind_int64(tk.s, 1, it.id);
        if (!Step(db, tk)) return fail(MSG_ALREADY_DONE);
    }
    {
        Stmt st(db, "UPDATE ga_mail SET credits = 0, state = 1075 WHERE id = ? AND credits = ?");
        if (!st) return fail(MSG_ACCEPT_FAILED);
        sqlite3_bind_int64(st.s, 1, mail_id); sqlite3_bind_int64(st.s, 2, m.credits);
        if (!Step(db, st)) return fail(MSG_ALREADY_DONE);
    }
    if (!Exec(db, "COMMIT")) return fail(MSG_ACCEPT_FAILED);
    r.ok = true;
    r.error_msg_id = 0;
    LoadRow(db, character_id, mail_id, &r.row);
    Logger::Log("mail", "[Mail] char %lld accepted mail %lld: credits %lld, %zu item(s)%s\n",
        (long long)character_id, (long long)mail_id, (long long)r.credits, m.items.size(), cod ? " (COD)" : "");
    return r;
}

int64_t DeclineCod(int64_t character_id, int64_t mail_id) {
    sqlite3* db = Database::GetConnection();
    if (!Exec(db, "BEGIN IMMEDIATE")) return 0;
    MailRow m;
    // The client's Return button sends this for ANY mail with attachments: COD -> items back (price is
    // not money); a normal player mail -> its credits + untaken items back. Returned mail cannot bounce again.
    const bool loaded = LoadRow(db, character_id, mail_id, &m);
    const bool cod = loaded && m.mail_type == TYPE_COD;
    const bool returned = loaded && (m.source == SOURCE_RETURNED || m.mail_type == TYPE_COD_RETURNED);
    if (!loaded || m.from_character_id <= 0 || returned || (cod ? m.items.empty() : (m.credits <= 0 && m.items.empty()))) {
        Exec(db, "ROLLBACK");
        return 0;
    }
    const int64_t back = InsertMail(db, m.from_character_id, character_id, "", m.subject /* client adds Returned: */,
                                    m.body, cod ? 0 : m.credits, cod ? TYPE_COD_RETURNED : TYPE_NORMAL, SOURCE_RETURNED);
    bool ok = back && (m.items.empty() || MoveItems(db, mail_id, back));
    if (ok) {
        Stmt st(db, "UPDATE ga_mail SET credits = 0, state = 1076, deleted_at = strftime('%s','now') WHERE id = ?");
        ok = st && (sqlite3_bind_int64(st.s, 1, mail_id), Step(db, st));
    }
    if (!ok || !Exec(db, "COMMIT")) { Exec(db, "ROLLBACK"); return 0; }
    Logger::Log("mail", "[Mail] char %lld returned mail %lld -> back to char %lld (mail %lld)\n",
        (long long)character_id, (long long)mail_id, (long long)m.from_character_id, (long long)back);
    return m.from_character_id;
}

bool Delete(int64_t character_id, int64_t mail_id) {
    // mail that still holds credits or items is kept: take or decline/return first
    Stmt st(Database::GetConnection(),
        "UPDATE ga_mail SET deleted_at = strftime('%s','now') "
        "WHERE id = ? AND to_character_id = ? AND deleted_at = 0 AND credits = 0 "
        "AND NOT EXISTS (SELECT 1 FROM ga_mail_items WHERE mail_id = ga_mail.id AND taken = 0)");
    if (!st) return false;
    sqlite3_bind_int64(st.s, 1, mail_id);
    sqlite3_bind_int64(st.s, 2, character_id);
    return sqlite3_step(st.s) == SQLITE_DONE && sqlite3_changes(Database::GetConnection()) > 0;
}

} // namespace MailService
