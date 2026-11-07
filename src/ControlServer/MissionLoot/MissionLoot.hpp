#pragma once
#include <string>
#include <utility>
#include <vector>
// Mission-end awarded loot. The final build gives
// gear as single-use "Range/Melee Drop" blueprints from the "*PVE SOLO/GROUP ..."
// tables (type 1167 Awarded Loot - Boss -> "*Master Armor and Weapons" sub-tables);
// Hi-Rez's backend rolled them at mission end, nothing in the files does. One
// pick-one roll per winning agent, queued at mission end and granted at the next
// spawn before the full inventory push (same path as achievement rewards).
#include <cstdint>
#include <string>
#include <vector>

namespace MissionLoot {

void EnsureSchema();
// Reward table for a won PvE mission (0 = none): difficulty + party size,
// Double Agent / raid queues by name/type.
int TableFor(uint32_t difficulty, int players, const std::string& queue_name,
             uint32_t queue_type);
// Queue one roll for every agent on players_tf; returns rows queued.
// Agents in `exclude` (e.g. over the queue's reward level cap) get none.
int QueueForInstance(int64_t instance_id, int players_tf, int loot_table_id,
                     const std::vector<int64_t>& exclude = {});
// Queue one roll of loot_table_id for one agent under reference `ref`
// (stored as instance_id; any non-zero key). Same (agent, ref, table) is
// queued once. max_level > 0: only items needing at most that level are
// candidates. Returns 1 if queued, 0 if already queued or invalid.
int QueueForCharacter(int64_t character_id, int64_t ref, int loot_table_id, int max_level = 0);
// Add xp/credits/tokens (each 0..100000, xp capped at the top level) to one
// agent once per reference `ref`. Returns 1 if added, 0 if already done/invalid.
int GrantCurrency(int64_t character_id, int64_t ref, int64_t xp, int64_t credits, int64_t tokens,
                  int64_t ap = 0);
// PvP end-of-match loot - one roll per agent in `paid` (passed the
// PvP reward rules) on the winning side: arena 348, else 231 (lv 1-29) / 473 (lv 30+).
int QueuePvpWin(int64_t instance_id, int winning_task_force, bool arena,
                const std::vector<int64_t>& paid);
// Roll + grant this agent's queued rolls; returns item stacks granted.
// `names` (optional) gets one "Item xN" entry per granted stack (chat summary).
std::string ItemName(int item_id);
// Common/Uncommon/Rare/Epic of a crafted row (stamped quality, else the item's own).
std::string CraftedQualityName(const std::vector<int64_t>& rows, int item_id);
int GrantPending(int64_t character_id, std::vector<std::pair<int, int>>* items = nullptr);

}  // namespace MissionLoot
