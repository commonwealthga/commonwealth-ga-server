#pragma once
// In-game mail. The original mail lived on Hi-Rez's
// backend; the client (UTgUIMailMenu) only sends GET/READ/DELETE/ACCEPT/SEND and
// renders DATA_SET 0x10C rows. Field layout decoded from GlobalAgenda.exe.
#include <cstdint>
#include <string>
#include <vector>

namespace MailService {

// asm valid values (groups 122 / 123 / 145)
constexpr uint32_t SOURCE_PERSON   = 1070;
constexpr uint32_t SOURCE_AUCTION  = 1071;
constexpr uint32_t SOURCE_SYSTEM   = 1072;
constexpr uint32_t SOURCE_RETURNED = 1346;
constexpr uint32_t STATE_POSTED    = 1073;
constexpr uint32_t STATE_READ      = 1074;
constexpr uint32_t STATE_ACCEPTED  = 1075;
constexpr uint32_t TYPE_NORMAL     = 1328;
constexpr uint32_t TYPE_COD        = 1329;
constexpr uint32_t TYPE_COD_RETURNED = 1334;
constexpr uint32_t STATE_RETURNED  = 1076;

// item binds (asm valid values group 141): these never leave the account
constexpr int BIND_ON_ACQUIRE = 1291;
constexpr int BIND_TO_AGENCY  = 1472;
// client inventory ids >= this are component stacks (TcpSession::kComponentInventoryIdBase)
constexpr uint32_t COMPONENT_INV_BASE  = 0x40000000u;
// component stacks shown inside a mail get their own synthetic ids so they
// never collide with the reader's own component entries
constexpr uint32_t MAIL_COMPONENT_BASE = 0x50000000u;
constexpr size_t   MAX_ATTACHMENTS     = 10;

// client message ids (asm_data_set_msg_translations)
constexpr uint32_t MSG_NO_RECIPIENT   = 29617; // You did not specify the recipient.
constexpr uint32_t MSG_SEND_FAILED    = 29619; // Was unable to send mail, possible lack of currency or problem item.
constexpr uint32_t MSG_ALREADY_DONE   = 29620; // The mail has already been accept, returned, or reported, etc.
constexpr uint32_t MSG_ACCEPT_FAILED  = 29621; // There was a problem with accepting the mail, possible attachment issues.
constexpr uint32_t MSG_TOO_MANY_ATT   = 37988; // Too many mail attachments.

constexpr int     MAX_LIST     = 50;          // the inbox shows 50 rows (client loop cap 0x32)
constexpr int     EXPIRE_DAYS  = 30;
constexpr size_t  MAX_SUBJECT  = 64;
constexpr size_t  MAX_BODY     = 1000;
constexpr int64_t MAX_CREDITS  = 100000000;

// One attached item. Gear rows stay in ga_players_inventory, parked on
// user_id 0 (stock_n = -mail item id) while in the mail; component stacks
// live only here until taken.
struct MailItem {
    int64_t     id = 0;
    int64_t     inventory_id = 0;  // escrowed ga_players_inventory row, 0 = component stack
    int         item_id = 0;
    int64_t     quantity = 1;
    int         device_id = 0;
    int         quality = 0;
    std::string mods;              // mod_effect_group_ids CSV
    int         oc = 0;
};

struct MailRow {
    int64_t     id = 0;
    int64_t     from_character_id = 0;
    std::string from_name;
    std::string subject;
    std::string body;
    int64_t     credits = 0;
    uint32_t    mail_type = TYPE_NORMAL;
    uint32_t    source = SOURCE_PERSON;
    uint32_t    state = STATE_POSTED;
    int64_t     sent_at = 0;
    int64_t     expire_at = 0;
    std::vector<MailItem> items;   // untaken attachments
};

void EnsureSchema();

// Inbox of a character (newest first, max MAX_LIST). Expired mail is handled
// first: credits go back to the sender as a Returned mail, the rest is deleted.
std::vector<MailRow> List(int64_t character_id);
uint32_t UnreadCount(int64_t character_id);

struct Attachment { int64_t inventory_id = 0; int64_t count = 1; };

struct SendResult {
    bool ok = false;
    std::vector<int64_t> removed_rows;   // gear rows that left the sender's bag
    bool components_changed = false;
    std::vector<int> depleted_components; // whole stacks mailed away
    uint32_t error_msg_id = 0;
    int64_t mail_id = 0;
    int64_t to_character_id = 0;
    int64_t to_user_id = 0;
};
// Person-to-person mail. Credits are debited from the sender in the same
// transaction (credits >= amount checked in SQL). Attachments are escrowed in
// the same transaction; COD = items + a price (credits) the reader pays on accept.
SendResult Send(int64_t from_character_id, const std::string& to_name,
                const std::string& subject, const std::string& body,
                int64_t credits, uint32_t mail_type, const std::vector<Attachment>& attachments);

// Marks Posted -> Read. Returns false if the mail is not the character's.
bool MarkRead(int64_t character_id, int64_t mail_id, MailRow* out);

struct AcceptResult {
    bool ok = false; uint32_t error_msg_id = 0;
    int64_t credits = 0;                 // credits gained (negative = COD paid)
    std::vector<int64_t> gained_rows;    // gear rows now in the reader's bag
    bool components_changed = false;
    int64_t cod_paid_to_character = 0;   // COD: the sender got a payment mail
    MailRow row;
};
// Takes the attached credits + items (once). COD: the price is debited first
// and mailed to the sender.
AcceptResult Accept(int64_t character_id, int64_t mail_id);

// Declines a COD mail: its items go back to the sender as a COD Returned mail.
// Returns the sender's character id (0 = refused).
int64_t DeclineCod(int64_t character_id, int64_t mail_id);

// ── Auction House escrow (AuctionService) ──
// All three run inside the CALLER's transaction (db = Database::GetConnection()).
// A listing's item sits in a hidden holding mail (to_character_id 0, source
// Auction) so the escrow + delivery reuse the attachment code above.
// Returns the holding mail id, 0 = refused (same rules as mailing it).
int64_t HoldForAuction(int64_t user_id, const Attachment& a, SendResult& r);
// Market bots: a holding mail with a NEW item nobody owned (component = stack
// row; anything else = a new bag row parked on user 0, same shape as a loot drop).
int64_t HoldNewForAuction(int item_id, int64_t quantity, bool component);
// Moves quantity units of a held component stack to a new holding mail (id, 0 = refused).
int64_t SplitHeld(int64_t holding_mail_id, int64_t quantity);
// Destroys the untaken items of a holding mail (a bot bought it / a bot listing expired).
bool DiscardHeld(int64_t holding_mail_id);
// Untaken items of a holding mail (for the AH rows).
std::vector<MailItem> HeldItems(int64_t holding_mail_id);
// System mail from "Auction House" to a character: credits and/or the held
// items of holding_mail_id (0 = credits only). Returns the new mail id.
int64_t DeliverFromAuction(int64_t holding_mail_id, int64_t to_character_id,
                           const std::string& subject, const std::string& body, int64_t credits);

// System mail with credits and/or NEW items (nobody owned them) to a character,
// one transaction. Items: item_id must exist in asm_data_set_items; components
// (item type 1171) arrive as a stack, anything else as one bag row per quantity.
// Returns the new mail id, 0 = refused (nothing written).
struct NewItem { int item_id = 0; int64_t quantity = 1; };
int64_t DeliverSystem(int64_t to_character_id, const std::string& from_name,
                      const std::string& subject, const std::string& body,
                      int64_t credits, const std::vector<NewItem>& items);

// Deletes a mail of the character. Mail that still carries credits or items is kept
// (the original refused too: take or return first).
bool Delete(int64_t character_id, int64_t mail_id);

} // namespace MailService
