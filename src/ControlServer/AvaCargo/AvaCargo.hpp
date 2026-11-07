#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// AvA tech in matches - control server half.
//
// Original (wiki AVA "Participation"/"Tech"/"Dropships"): a strike force equips a dropship and
// loads it with AvA equipment; beacons are consumed and must be brought; every item uses cargo
// space; cargo over the dropship's capacity does not spawn in the instance. In the match the tech
// waits at the dropship and agents pick it up / deploy / enter it with the beacon key (B); an agent
// who dies carrying tech loses it. Data: prop 393 "Cargo Space Available" (Basic 25, Falcon 50,
// Hawk 75, Avenger 100), prop 394 "Item Footprint" (beacons 1, hubs 2, platforms/EMP/nuke/siege 10),
// hex building defensive_cargo_space 25/50/75 = what the defenders may bring (client texts 38846
// "too much cargo for specified dropship" / 38845 "... for the specified territory").
//
// Client RE (GlobalAgenda.exe, final client): scene GA_Menu_Team_StrikeForce_Cargo (class
// TgUICargoMenu, vtable 0x11889228) lists the DATA_SET_STRIKE_FORCE_INVENTORY rows whose TASK_TEAM =
// the selected task team (+1); the dropship row (subtype 1302) fills the dropship slot.
// C->S ADD_STRIKE_FORCE_TEAM_ITEM 0x14E {INVENTORY_ID, ITEM_ID, INSTANCE_COUNT, TASK_TEAM} (dropped
// row, vt+0x19c 0x1144e0f0; a dropship always count 1) and REMOVE_STRIKE_FORCE_TEAM_ITEM 0x14F
// {INVENTORY_ID, INSTANCE_COUNT, TASK_TEAM} (vt+0x240/0x248). Server state rows: EHexItemState
// HIS_NotSpawned/Factory/PickedUp/Placed/Destroyed/DoNotSpawn (TgHexItemInstance, natives stripped).
//
// Ours: AvA items are agency stock (ga_ava_items), so by default everything the owner holds is
// loaded on task team 1 (what the client showed before); the cargo menu unloads (0x14F, kept at
// home in ga_ava_cargo_held) and loads back (0x14E). The equipped dropship = the best loaded one
// (Basic Dropship 5111 always). At match start (INSTANCE_READY of AvA queues 40-45) the loaded
// devices (type 1298 subtype 1301) are packed in row order up to the capacity (attackers: dropship
// prop 393; defenders: the territory's defensive_cargo_space), taken out of ga_ava_items into
// ga_ava_cargo_out (escrow) and sent to the instance as AVA_CARGO. The game DLL spawns each item's
// deployable "Pack" at the side's TgHexItemFactory spots and, before MISSION_ENDED, reports the
// items still untouched there (AVA_CARGO_RESULT) -> returned. No result (instance died, older DLL)
// -> everything is returned. Siege vehicles (subtype 1300) stay home (no vehicle spawn yet).
// Switch: cs_settings ava_cargo on|off (off = no cargo leaves home, AVA_CARGO not sent).
namespace AvaCargo {

struct Row { uint32_t inventory_id, item_id, count, task_team; };

// DATA_SET_STRIKE_FORCE_INVENTORY rows for this character (row ids: 1 = Basic Dropship, then the
// owner's ga_ava_items by item id - the numbering AvaShields::ItemForInventoryId relies on).
std::vector<Row> InventoryRows(int64_t character_id);

// 0x14E / 0x14F. Returns a chat line for the caller ("" = nothing to say).
std::string Load(int64_t character_id, uint32_t inventory_id, uint32_t item_id, uint32_t count);
std::string Unload(int64_t character_id, uint32_t inventory_id, uint32_t count);
// Chat command "-cargo" [load|unload|list]: list / load everything / unload everything.
std::string Command(int64_t character_id, const std::string& args);

// IpcServer INSTANCE_READY for an AvA queue: build + escrow + send AVA_CARGO. guid -> task force.
void OnMatchReady(int64_t instance_id, uint32_t queue_id,
                  const std::unordered_map<std::string, int>& task_forces);
// IpcServer AVA_CARGO_RESULT {instance_id, returns:[{task_force,item_id,count}]}.
void OnResult(int64_t instance_id, const std::string& json);
// IpcServer MISSION_ENDED for an AvA queue (after the result): close the escrow.
void OnMatchEnded(int64_t instance_id);

}  // namespace AvaCargo
