#pragma once

#include <cstdint>
#include <string>

// Defence shields and Missile Complex.
//
// Original rules (final client texts; the backend numbers are gone):
//  43938 "A defense facility can project shields around two adjacent allied territories, protecting
//        them from attack. Defense facilities themselves cannot be shielded however, and are always
//        vulnerable striking points."  43935 changing the shield set drops the shields while the new
//        ones are built (SHIELD_PROGRESS, 41440 "Shield Progress").  43940 "You have chosen invalid
//        shield directions."  1.4: the HQ is the only defense facility (WIKI-Patch_1.4).
//  39075 "The territory is protected, a missile with a valid range is required."  38933/39319: a
//        Missile Complex gives range to Tactical Missiles and is required to strike shielded territory.
//        Tactical Missile I/II/III (items 5058/5059/5060, built in a Factory for 300/400/600) have range
//        2/3/4 (texts 38862/38864/38866) = hex distance from the attacker's closest Missile Complex.
//
// Client RE (GlobalAgenda.exe): HEX_ASSIGN_DEFENSIVE_SHIELDS 0x1B8 {ZONE_OBJECT_ID, POSITION,
// SHIELD_POSITION_FLAGS} from the Facilities dialog (UTgUIAgencyMenu_Facilities, native
// OnShieldTileClicked 0x113f8210: tile i -> flags ^= 1 << i, at most 2 set (vt+0x258 0x113f8130)).
// Tiles i = 0..5 are Button_Hex_3,4,5,6,1,2 = the diagram's NW, N, NE, SE, S, SW (Menus_Agency.upk
// HexDiagram positions). The board is an odd-row-offset grid (odd rows shifted right, per the tile
// dump), so it has W/E neighbours, not N/S: ours maps the diagram clockwise onto it as
// bit 0 NW, 1 NE, 2 E, 3 SE, 4 SW, 5 W (reconstruction - the client never computes neighbours).
//
// Ours (no original number survives): shield build time cs_settings ava_shield_build_minutes (60),
// a missile opens the target for the launching owner for 30 min (ava_missile_breach_minutes),
// switch ava_shields on|off (off = no shields enforced, dialog ignored).
namespace AvaShields {

// Factory-built deeds move from the agency stock to a manager's bag, where Change Facility can pick them. true = bag changed.
bool DeedsToBag(int64_t cid);
// Blueprint item of a bag row of this agent, 0 if it is not one.
uint32_t BagBlueprint(int64_t cid, int64_t row);

struct Result {
	bool refuse = false;     // gates: true = refuse the action
	bool handled = false;    // AddFacility: true = this module answered the request
	uint32_t msg_id = 0;     // client text to show (team system message), 0 = none
	std::string text;        // chat line to the caller
	bool zone_changed = false;
};

// Board geometry (20x11, odd rows shifted right). -1 = off the board.
int Neighbour(uint32_t pos, int dir);
int Distance(uint32_t a, uint32_t b);

// HEX_ASSIGN_DEFENSIVE_SHIELDS.
Result Assign(int64_t character_id, uint32_t position, uint32_t flags);

// Territory row fields; 0 = do not send.
struct Row { uint32_t position_flags = 0, origin_flags = 0; int progress = -1; };
Row RowFor(uint32_t position);
bool IsShielded(uint32_t position);  // an active (100 %) shield covers it

// HEX_BID: refuse a bid on a shielded hex the bidder cannot strike (remembers the chosen missile).
Result CheckBid(int64_t character_id, uint32_t position, uint32_t inventory_id);
// HEX_CAPTURE: check before queueing (consume = false), then consume once the attack is queued.
Result CheckAttack(int64_t character_id, uint32_t position, bool consume);

// HEX_ADD_FACILITY with a deed item (Missile Complex 5075 / building 15 on a No Facility hex; Tech II-III
// Mine/Lab/Factory deeds 5112-5117 upgrade the facility one tier; 5763 Defense deed refused, 1.4):
// uses one deed built in a Factory (production_cost + blueprint components).
Result AddFacility(int64_t character_id, uint32_t position, uint32_t id);

}  // namespace AvaShields
