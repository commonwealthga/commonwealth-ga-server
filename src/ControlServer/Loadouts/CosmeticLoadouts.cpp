#include "src/ControlServer/Loadouts/CosmeticLoadouts.hpp"

namespace CosmeticLoadouts {

// IMPORTANT: these are asm_data_set_items.item_id values (game-logical ids),
// NOT asm_data_set_items.id (the row PK shown in cosmetics.md's listing).
// The engine natives (HeadFlairId / SuitFlairId / SetDyeItemId /
// SetJetpackTrailId) expect game item_ids, as does the wire protocol.
//
// Resolved via: SELECT id, item_id, name FROM asm_data_set_items WHERE …
//
// 3524 = Dye - None       (placeholder; player picks real colors)
// 7546 = Trail - White    (subtle default; any of the 24 trails works)

// 567 — Medic
static const CosmeticDefaults kMedic = {
	/*helmet*/        3147,   // Helm - Medic Acolyte           (.id=690)
	/*suit*/          3139,   // Suit - Medic Acolyte           (.id=684)
	/*dye_primary*/   3524,
	/*dye_secondary*/ 3524,
	/*dye_emissive*/  3524,
	/*dye_weapon_pri*/3524,
	/*dye_weapon_emi*/3524,
	/*jetpack_trail*/ 7546,
};

// 679 — Robotics
static const CosmeticDefaults kRobotics = {
	/*helmet*/        4771,   // Helm - Robotics Operative      (.id=1371)
	/*suit*/          4770,   // Suit - Robotics Operative      (.id=1370)
	/*dye_primary*/   3524,
	/*dye_secondary*/ 3524,
	/*dye_emissive*/  3524,
	/*dye_weapon_pri*/3524,
	/*dye_weapon_emi*/3524,
	/*jetpack_trail*/ 7546,
};

// 680 — Assault
static const CosmeticDefaults kAssault = {
	/*helmet*/        3162,   // Helm - Assault Heavy           (.id=704)
	/*suit*/          3160,   // Suit - Assault Heavy           (.id=702)
	/*dye_primary*/   3524,
	/*dye_secondary*/ 3524,
	/*dye_emissive*/  3524,
	/*dye_weapon_pri*/3524,
	/*dye_weapon_emi*/3524,
	/*jetpack_trail*/ 7546,
};

// 681 — Recon
static const CosmeticDefaults kRecon = {
	/*helmet*/        3199,   // Helm - Recon Wraith            (.id=734)
	/*suit*/          3196,   // Suit - Recon Wraith            (.id=731)
	/*dye_primary*/   3524,
	/*dye_secondary*/ 3524,
	/*dye_emissive*/  3524,
	/*dye_weapon_pri*/3524,
	/*dye_weapon_emi*/3524,
	/*jetpack_trail*/ 7546,
};

const CosmeticDefaults& GetDefaultsForProfile(uint32_t profile_id) {
	switch (profile_id) {
		case 567: return kMedic;
		case 679: return kRobotics;
		case 680: return kAssault;
		case 681: return kRecon;
		default:  return kAssault;
	}
}

}  // namespace CosmeticLoadouts
