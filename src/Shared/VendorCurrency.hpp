#pragma once
// Which currency a vendor row can be paid in,
// shared by the control server (vendor list) and the game DLL (purchase check).
// The client's vendor scene draws ONE price column next to Agenda Points, chosen
// by the vendor's loot table type: 1439 "Vendor - Mercenary Token" / 1438
// "Vendor - Conquest Token" -> Tokens, anything else (656 "Vendor") -> Credits.
// A row with no price in that column currency is drawn "--" + "You cannot afford
// this item" and cannot be bought (Burning/Chloe's Fashion 7 of 11 rows a week,
// Rare Goods, Skyward Jetpacks' Flight Helmet: former cash-shop cosmetics with
// only a credit + Agenda Points price). Those rows are sold for their ORIGINAL
// AP price (AP is earned in play only - F2P rule, docs/F2P-REWORK.md); the
// unusable other in-game price is not offered. Rows that have a price in the
// column currency keep the usual rule (credits/tokens; AP only for AP-only items).
// "Own" price = lowest price id per item + currency (stray 2nd set ignored).

namespace VendorCurrency {

// ?1 = loot table (LIVE or stock - rotation sets share the LIVE table's type).
// One row: the column currency, 1604 tokens or 1603 credits.
inline const char* kColumnSql =
	"SELECT CASE WHEN COALESCE((SELECT loot_table_type_value_id FROM asm_data_set_loot_tables "
	"  WHERE loot_table_id = ?1 LIMIT 1), 0) IN (1438, 1439) THEN 1604 ELSE 1603 END";

// ?1 = item id, ?2 = currency. One row if the item has its own price > 0 in it.
inline const char* kHasPriceSql =
	"SELECT 1 FROM asm_data_set_prices p WHERE p.item_id = ?1 AND p.currency_type_value_id = ?2 AND p.price > 0 "
	"  AND p.id = (SELECT MIN(p2.id) FROM asm_data_set_prices p2 WHERE p2.item_id = p.item_id AND p2.currency_type_value_id = ?2) LIMIT 1";

} // namespace VendorCurrency
