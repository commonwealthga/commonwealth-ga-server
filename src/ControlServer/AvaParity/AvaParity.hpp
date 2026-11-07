#pragma once
#include "src/ControlServer/AvaZones/AvaZones.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct sqlite3;

// Original AvA rules that our hex war lacked,
// kept in one place so TcpSession / PlayerSessionStore only carry 1-line hooks.
//  - Lockdown after a result (wiki AVA "Lockdown", news 4190): captured hex 2 min,
//    defended production facility 10 min, defended other hex 2 min, empty claim none.
//    Client texts 17408 / 17462 / 39922.
//  - Production soft cap: client text 59925 "Owning more than 5 territories results in all
//    of your territories becoming less efficient ... the most efficient agency will hold
//    around 13 territories." Ours: total = n - (n-5)^2 / 16 (peak at 13), as a factor.
//  - Season end items (1.37/1.39): winner / runner-up / honorable mention (rank 3-10)
//    loot tables into the owner's AvA inventory (ga_ava_items).
//  - Season rewards need 7 days in the agency before the season end (news 4416).
//  - HQ bonus cargo (loot table 136 AVA_HQ_BONUS_CARGO) to the owner after a won HQ defence.
//  - Low-population switches (cs_settings, default = our low-pop rule):
//      ava_zone_schedule  'always' | 'original' (1.36: open 5 h from 20:30 ET, Mon-Thu + Sat-Sun)
//      ava_solo_owners    'on' | 'off'          (off = only agency members may bid/attack/defend)
namespace AvaParity {

enum Action { kAttack, kDefend, kBid };

struct Gate {
	bool refuse = false;
	uint32_t msg_id = 0;   // client message id to show, or 0 = use text
	std::string text;
};

// Called by handle_hex_attack / handle_hex_bid before anything is spent.
Gate Check(int64_t character_id, uint32_t position, Action action);

// Called at the top of PlayerSessionStore::ResolveAvaMatch (before the attack rows are consumed).
void OnAvaResult(int64_t instance_id, int winning_task_force);

// Facility tech tier (0 = empty / no facility, 1-3) of the hex this AvA instance fights over;
// sent to the pvp_reward hook as "ava_tech" (1.36: winners' tokens by facility tier).
int TechTier(int64_t instance_id);

// Account names of the attacked territory's owner (agency members,
// or the solo owner) to tell "An agency territory is under attack!" (client text 39861). The
// attacker's own owner is never alerted.
std::vector<std::string> AttackAlertRecipients(uint32_t position, int64_t attacker_character_id);

// owner id -> output factor (1.0 = no cap) for SettleAvaProduction.
std::map<int64_t, double> SoftCapFactors(sqlite3* db);

// Seconds an agency member must have been in the agency before the season end (0 = off).
int64_t SeasonMemberSeconds();

// Season-end item rewards for a finished season (idempotent per season + owner).
void GrantSeasonItems(int season);

// True while the AvA zone accepts attacks/bids; secs_to_change = seconds until it flips (0 = never).
bool ZoneOpen(int64_t now, int64_t* secs_to_change = nullptr);


// Theft (wiki AVA "Theft", news 4216). Client RE: TgUIHexAuction OnMissionTypeChanged
// sets HEX_ATTACK_TYPE 1 for the Capture checkbox, 2 for the Steal checkbox; the bid carries it.
// A won theft on a held territory transfers nothing: the attackers' owner gets tech from
// *AvA_Theft_Items_Tech1/2/3 (252/255/254, by facility tier), the defender loses 3x that value
// of networth and its territory produces nothing for 2 h. Attacks without a bid = capture.
constexpr uint32_t kAttackCapture = 1, kAttackTheft = 2;
// Called from PlayerSessionStore::ResolveAvaMatch (after OnAvaResult): true = this attacker's
// win was a theft, do not transfer the territory.
bool IsTheft(int64_t instance_id, int64_t character_id);

// Hex trading (wiki AVA "Hex Trading", news 4216). Client RE: HEX_GIVE_OWNERSHIP
// 0x1B3 {ZONE_OBJECT_ID, POSITION, NAME = target player}, HEX_ABANDON 0x1BB {ZONE_OBJECT_ID, POSITION}.
// Facility, mine resource, HQ and lockdown of the hex go; not while the zone is open (schedule
// 'original' only), not while the hex is under attack. Needs leader / Facility Management rank.
struct TradeResult {
	bool ok = false;
	uint32_t msg_id = 0;           // client message to show the caller (0 = none)
	std::string text;              // chat line to the caller
	std::string notify_name;       // account to tell (gift target)
	std::string notify_text;
};
// Call PlayerSessionStore::SettleAvaProduction() first (old owner keeps what it produced).
TradeResult GiveTerritory(int64_t character_id, uint32_t position, const std::string& target,
	const std::vector<std::string>& online_names);
TradeResult AbandonTerritory(int64_t character_id, uint32_t position);

// Net worth (wiki AVA "Networth", news 4190): tech built in a Factory adds its
// production_cost to the owner's season networth (scrapping takes it back); theft cuts it.
// Seasons rank by networth first, then territories (cs_settings ava_rank_by networth|territories);
// Database.cpp adds the ga_ava_seasons.networth column at startup.
void BookNetworth(int64_t owner, double delta);           // handle_hex_factory_item
void SnapshotNetworth(sqlite3* db, uint32_t season);      // ava_snapshot_standings (inside its transaction)
double Networth(int64_t owner, uint32_t season);          // live leaderboard
uint32_t CurrentSeason(int64_t now);
// Seasons (wiki AVA "Seasons"): a season lasts cs_settings ava_season_days (default 14 =
// the original two weeks; was a fixed 7) from Mon 2026-10-26. SeasonEnd (once, from AvaSeasonTick after the payout):
// 1.37 token reward = 1 token per 2 networth (max 15,000) to each active member of every owner with networth, then the
// zone is wiped (cs_settings ava_season_wipe on|off, default on): every hex back to unclaimed, facilities/HQ/shields/
// bids/lockdowns gone; agency inventory and stockpiles stay.
constexpr int64_t kSeasonEpoch = 1792972800;
int64_t SeasonLength();
void SeasonEnd(int season, int64_t from, int64_t to);

// HQ (wiki AVA "HQ", 1.41 "no bonus until 100%"): an HQ builds up after it is set
// (cs_settings ava_hq_build_minutes, default 120 - ours, no original time survives). Returns -1 when
// the hex is no HQ, else 0-100 (sent as HQ_PROGRESS in the territory row; cargo only at 100).
int HqProgress(uint32_t position);

// Sealed bid window (wiki AVA "Bidding"; cs_settings ava_bid_window original|instant).
struct BidResult {
	bool handled = false;         // false = window mode off (or cancel of a won bid): run the old code
	bool wallet_changed = false;
	uint32_t msg_id = 0;          // client text for the caller (0 = none)
	std::string text;
};
int64_t OwnSealedBid(int64_t character_id, uint32_t pos);  // -1 = none
uint32_t AttackTypesFor(uint32_t pos);
BidResult SealedBid(int64_t character_id, uint32_t position, uint32_t amount, uint32_t attack_type, bool cancel);
// Closes due bid windows (winner -> ga_ava_bids or the unclaimed hex, losers refunded) and refunds
// won bids not launched in time. Lazy: called from Check/SealedBid, the zone map and the season tick.
void ResolveBidWindows(int64_t now);
bool BidWindowOpen(uint32_t position, int64_t* secs_left = nullptr);
// Opened won bids. OpenBid sets ga_ava_bids.open_until = now + secs for the owner's won bid
// (returns rows changed). OpenBidOwnerFor = owner of an open won bid on the territory when that is
// NOT the agent's own owner id (0 = none): the agent may launch the attack for that owner.
int OpenBid(int64_t owner, uint32_t position, int64_t secs);
int64_t OpenBidOwnerFor(uint32_t position, int64_t character_id);
// Won bids open to outside fighters right now, soonest to close first (zone_id 0 = every zone).
struct OpenFight { uint32_t position; int64_t attacker, defender; int64_t secs_left; };
std::vector<OpenFight> OpenFights(uint32_t zone_id, int64_t character_id);
// Won bids the agent's own owner attacks or defends, also before they are opened to outsiders.
struct CombatLogRow { int64_t at; uint32_t position, building; std::string owner, attacker; uint32_t outcome; };
std::vector<CombatLogRow> CombatLog(uint32_t zone, const std::string& agency_like, uint32_t outcome, int64_t from, int64_t to);
std::vector<OpenFight> OwnFights(uint32_t zone_id, int64_t character_id);
int64_t OwnerOf(int64_t character_id);
std::string OwnerName(int64_t owner);

// Lab research -> blueprints (loot 153/154/155) inside SettleAvaProduction's
// transaction; Factory: blueprint id -> created item, then blueprint + components consumed
// (scrap returns them). FactoryRecipe refuses with a chat text when something is missing.
int LabBlueprints(sqlite3* db);
uint32_t FactoryTarget(uint32_t item_id);
Gate FactoryRecipe(int64_t owner, uint32_t item_id, uint32_t count, bool remove);

// Regions (cs_settings ava_regions off|on): RegionCount() = rows for
// DATA_SET_ZONE_REGIONS (0 = off), RegionOf(pos) = HEX_ZONE_REGION_ID, RegionSeconds(pos) =
// REGION_SECONDS for the territory row (-1 = off: do not send). Check() refuses bids/attacks
// outside the open region.
int RegionCount();
int RegionOf(uint32_t position);
int OpenRegion(int64_t now, int64_t* secs_left);
int64_t RegionSeconds(uint32_t position);

}  // namespace AvaParity

// Defined in TcpSession.cpp next to ava_building(): building id at a zone-1 position (0 = none).
uint32_t AvaParityBuildingAt(uint32_t position);
uint32_t AvaParityLayoutAt(uint32_t position);
namespace AvaParity {
// One-time move of every ga_ava_* position from the 7x5 to the 20x11 War Zone (cs_settings ava_zone_layout =
// '20x11' marks it done; also switches ava_regions on unless set) + writes ga_ava_layout (position, hex_building_id).
constexpr uint32_t kZoneW = AvaZones::kW, kZoneH = AvaZones::kH;
// One time, before the server opens its ports: the boards moved to 12x30, so position-based AvA state is cleared and every
// open bid refunded. Season history, wallets and agency records stay. false = do not start: a player (not a service agent)
// holds something tied to a position, or the backup failed; nothing was changed.
bool MigrateZoneLayout();
}
// Territory resource pick / facility build for an owner id (agency id), no client session.
std::string AvaServiceAssignResource(int64_t owner, uint32_t position, uint32_t item_id);
std::string AvaServiceAddFacility(int64_t owner, uint32_t position, uint32_t id);
// Factory build/scrap for an owner id (agency id), no client session; *changed = items moved.
std::string AvaServiceFactoryItem(int64_t owner, uint32_t position, uint32_t item_id, uint32_t count, bool remove,
	bool* changed);
