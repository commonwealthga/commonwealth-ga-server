#pragma once
// AvA war zones (patch 1140). The original ran six independent zones from v1.36 (2 each for NA, EU, PAC; Steam news 4190)
// with their own leaderboards and winners, and an agency held territory in one zone only (Steam news 3986). The names and
// hex counts below are the client's own texts (msgs 59839-59850), so the map's Zone dropdown needs no client change.
//
// A territory is stored under one number: global = (zone - 1) * kStride + the hex position the client uses, so zone 1
// is unchanged and every AvA table stays keyed by position (their zone_id column stays 1). The client only ever sees
// the local hex: incoming packets go through ToGlobal, the zone map reply through Local.
#include <cstdint>
#include <string>

namespace AvaZones {
constexpr uint32_t kStride = 1000;
constexpr uint32_t kW = 12, kH = 30;  // shared grid of every zone (client WIDTH x HEIGHT); outlines in AvaBoards.hpp
constexpr uint32_t kBad = 0xFFFFFFFFu;

struct Zone { uint32_t id, name_msg, desc_msg, hexes; const char* name; };

int Count();                          // cs_settings ava_zones, 1..6 (default 6)
const Zone* Find(uint32_t zone_id);   // nullptr = no such zone (or above Count)
inline uint32_t ZoneOf(uint32_t g) { return g / kStride + 1; }
inline uint32_t Local(uint32_t g) { return g % kStride; }
inline uint32_t Base(uint32_t zone) { return (zone - 1) * kStride; }
bool Valid(uint32_t g);               // a usable hex of a live zone's board (blank tiles are not)
int Region(uint32_t zone_id, uint32_t local);  // 1..5, 0 = blank tile or off the grid
uint32_t Background(uint32_t zone_id);         // BACKGROUND_IMAGE_RES_ID of the zone's board
uint32_t ToGlobal(uint32_t zone_id, uint32_t local);  // kBad when the zone or hex does not exist
std::string Label(uint32_t g);        // for chat: "130" in zone 1, "45 in EU Echelon" elsewhere
void SetView(const std::string& guid, uint32_t zone);  // the zone on this session's AvA map
uint32_t ViewOf(const std::string& guid);              // 1 until the map was opened
void SetBattle(const std::string& guid, uint32_t g);   // the fight offered on this session's AvA map (kBad = none)
uint32_t BattleOf(const std::string& guid);
}  // namespace AvaZones
