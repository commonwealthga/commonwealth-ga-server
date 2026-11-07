#include "src/ControlServer/Loadouts/StarterOffhands.hpp"

#include <algorithm>

#include "src/ControlServer/Logger.hpp"

namespace StarterOffhands {

namespace {

struct Offhand { int device_id; int level; };  // level = asm_data_set_items.required_level

struct ClassOffhands { uint32_t profile_id; Offhand o[3]; };

// asm_data_set_items required_level 3/4/5, quality Common (item type 217).
const ClassOffhands kOffhands[] = {
	{ Loadouts::PROFILE_MEDIC,    { { 2246, 3 }, { 2531, 4 }, { 2168, 5 } } },  // Regeneration, Healing Grenade, Poison Grenade
	{ Loadouts::PROFILE_ASSAULT,  { { 2013, 3 }, { 2022, 4 }, { 3708, 5 } } },  // Range Shield, EMP Grenade, Perfect Target
	{ Loadouts::PROFILE_RECON,    { { 2368, 3 }, { 3056, 4 }, { 2225, 5 } } },  // Bionics, Fire Bomb, Standard Mine
	{ Loadouts::PROFILE_ROBOTICS, { { 2051, 3 }, { 2300, 4 }, { 2675, 5 } } },  // Force Wall, Personal Turret, Eye Drone
};

struct Slot { int svid; int equip_point; };
const Slot kSlots[3] = { { Loadouts::SVID_OFFHAND1, 7 }, { Loadouts::SVID_OFFHAND2, 8 }, { Loadouts::SVID_OFFHAND3, 9 } };

bool IsOffhand(int svid) {
	return svid == Loadouts::SVID_OFFHAND1 || svid == Loadouts::SVID_OFFHAND2 || svid == Loadouts::SVID_OFFHAND3;
}

}  // namespace

void Apply(uint32_t profile_id, int max_level, std::vector<Loadouts::GearSlot>& kit) {
	const ClassOffhands* c = nullptr;
	for (const auto& e : kOffhands) if (e.profile_id == profile_id) { c = &e; break; }
	if (!c) return;
	kit.erase(std::remove_if(kit.begin(), kit.end(),
		[](const Loadouts::GearSlot& g) { return IsOffhand(g.slot_value_id); }), kit.end());
	int n = 0;
	for (const Offhand& o : c->o) {
		if (o.level > max_level) continue;
		Loadouts::GearSlot g{};
		g.device_id     = o.device_id;
		g.equip_slot    = kSlots[n].equip_point;
		g.slot_value_id = kSlots[n].svid;
		g.quality       = Loadouts::Q_COMMON;
		g.mods          = Mods::Result{};
		kit.push_back(g);
		++n;
	}
	Logger::Log("db", "[StarterOffhands] profile=%u offhands=%d (max_level=%d)\n", profile_id, n, max_level);
}

}  // namespace StarterOffhands
