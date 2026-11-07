#pragma once
// Agency bank (items + credits), agency tax, bank log, and the
// agency chat lines (login MOTD, member online/offline). Wire layouts decoded from
// GlobalAgenda.exe.
#include <asio.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include "src/ControlServer/Database/Database.hpp"

namespace AgencyBank {

// asm valid value group 140 (agency resource events)
constexpr uint32_t EV_TAX_INCOME     = 1288;
constexpr uint32_t EV_CURRENCY       = 1370;
constexpr uint32_t EV_ITEM           = 1371;

// client message ids (asm_data_set_msg_translations)
constexpr uint32_t MSG_DISBAND_HAS_INV  = 25558;
constexpr uint32_t MSG_TAX_EARNED       = 36255;  // You have earned @@currency_value@@ credits for your agency.
constexpr uint32_t MSG_NO_TAX_PERM      = 36268;
constexpr uint32_t MSG_NO_WITHDRAW_PERM = 55428;
constexpr uint32_t MSG_INV_CHANGED      = 37962;
constexpr uint32_t MSG_MOTD             = 42168;  // Message of the Day: @@agency_motd@@
constexpr uint32_t MSG_CAME_ONLINE      = 42225;  // @@player_name@@ has come online.
constexpr uint32_t MSG_WENT_OFFLINE     = 42226;

constexpr float    MAX_TAX_PERCENT  = 50.0f;      // patch 1.2: tax capped at 50 %
constexpr int64_t  MAX_CREDITS      = 2000000000;
// client inventory ids of component stacks in the bank (personal ones are 0x40000000+item)
constexpr uint32_t BANK_COMPONENT_BASE = 0x60000000u;

struct Item {
    int64_t     bank_id = 0;
    int64_t     inventory_id = 0;   // parked ga_players_inventory row, 0 = component stack
    int         item_id = 0;
    int64_t     quantity = 1;
    int         device_id = 0;
    int         quality = 0;
    std::string mods;
    int         oc = 0;
    int         profile_id = 0;
    uint32_t ClientId() const {
        return inventory_id > 0 ? (uint32_t)inventory_id : BANK_COMPONENT_BASE + (uint32_t)bank_id;
    }
};

struct Bank {
    int64_t credits = 0;
    float   tax_percent = 0;
    std::vector<Item> items;
};

struct Result {
    bool ok = false;
    uint32_t error_msg_id = 0;
    std::vector<int64_t> removed_rows;    // gear rows that left the player's bag
    std::vector<int64_t> gained_rows;     // gear rows now in the player's bag
    bool components_changed = false;
    std::vector<int> depleted_components;
    int64_t credits = 0;
    int     item_id = 0;   // component withdraw: which stack
    int64_t count = 0;
};

struct LogRow {
    int64_t     at = 0;
    uint32_t    change_type = 0;
    std::string player_name;
    int         item_id = 0;
    int64_t     currency = 0;
    int64_t     count = 0;
};

void EnsureSchema();
Bank Load(int64_t agency_id);
bool IsEmpty(int64_t agency_id);
// bumped on every bank change; the roster carries the bank block only when it changed
uint64_t Version(int64_t agency_id);

Result DepositItem(int64_t agency_id, int64_t character_id, int64_t inventory_id, int64_t count);
Result WithdrawItem(int64_t agency_id, int64_t character_id, int64_t client_inventory_id, int64_t count);
Result DepositCredits(int64_t agency_id, int64_t character_id, int64_t amount);
Result WithdrawCredits(int64_t agency_id, int64_t character_id, int64_t amount);
bool   SetTax(int64_t agency_id, int64_t character_id, float percent);
// Moves the agency's tax share of a credit reward that was just paid to the
// character into the bank. Returns the amount moved (0 = no agency / no tax).
int64_t TaxEarnings(int64_t character_id, int64_t credits);
// BOUND_FLAG of a bag row: "F" unless the item binds on acquire / to agency
// (client shows Deposit Item / mail attach only for unbound rows). item_id 0 = starter kit = "T".
const char* BagBoundFlag(int item_id);
// Save Ranks: the client sends a new rank with an already used RANK_ID -> PK clash.
// Gives each duplicate a fresh id (first occurrence keeps it) and logs the rows.
void NormalizeRanks(std::vector<Database::AgencyRankRow>& ranks);
std::vector<LogRow> GetLog(int64_t agency_id, uint32_t change_type, const std::string& name,
                           int64_t from_unix, int64_t to_unix, int limit);

} // namespace AgencyBank

namespace AgencyChat {
// chat socket hooks (ChatSession join / teardown)
void OnChatJoin(const std::string& session_guid);
void OnChatLeave(const std::string& session_guid, const asio::any_io_executor& ex);
// sends due offline notices (called from the roster poll)
void Tick();
// one localized line (MSG_ID + tokens) on the agency channel to these accounts
void SendMsgToUsers(const std::vector<int64_t>& user_ids, uint32_t msg_id,
                    const std::string& player_name, const std::string& motd, int64_t currency,
                    bool announcement);
} // namespace AgencyChat
