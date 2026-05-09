#include "src/GameServer/TgGame/BuffEffectRegistry/ModifierProps.hpp"

namespace ModifierProps {

// ---------------------------------------------------------------------------
// IsModifierProp — union of FUN_109e5220 OUTPUT propIds across all contexts.
//
// Source of truth: ConvertPropToPropList @ 0x109e5220. Re-extract via Ghidra
// `decompile_function_by_address(0x109e5220)` if the binary changes.
//
// 412 (HEALTH_MAX_MODIFIER) intentionally NOT here: nothing in the engine
// currently calls GetBuffedProperty(BUFF_PAWN, 304 HEALTH_MAX) where the
// registry entry would be consulted, so ReapplyCharacterSkillTree redirects
// 412 → 304 directly. Drop that redirect once end-to-end Health Max scaling
// lands and add 412 here.
// ---------------------------------------------------------------------------
bool IsModifierProp(int pid) {
	switch (pid) {
		// BUFF_DEVICE outputs (context=3, queried by GetPropertyValueById)
		case 113:  // Damage Modifier (generic, BUFF_DEVICE expansion of props 10/0xf5/0xf6/0xf9)
		case 114:  // Device Range Modifier
		case 203:  // Recharge Time Modifier
		case 207:  // Device Effective Range Modifier
		case 256:  // Accuracy Correction Rate Modifier
		case 284:  // Max Control Range Modifier
		case 339:  // Required Points to Fire (BUFF_DEVICE self-passthrough)
		case 349:  // Remote Activation Time Modifier
		case 352:  // AOE Radius Modifier (also Effective Radius via 343)
		case 355:  // Pet LifeSpan Modifier
		case 356:  // Projectile Speed Modifier
		case 357:  // Required Morale Points Modifier
		case 391:  // Time To Deploy Modifier
		// Damage modifier family (BUFF_PAWN expansions of prop 51/65)
		case  65:  // Effect Damage Modifier (also BUFF_OTHER identity for rolled mods)
		case 212:  // Damage Modifier - Melee
		case 214:  // Damage Modifier - Range
		case 321:  // Damage Modifier - AoE
		case 361:  // Buff - Damage Buff Modifier
		case 362:  // Buff - Damage AOE Modifier
		case 363:  // Buff - Damage Melee Modifier
		case 364:  // Buff - Damage Ranged Modifier
		case 369:  // Buff - Damage Situational Modifier
		case 370:  // Buff - Situational Damage (final stage)
		case 388:  // Damage Modifier - Mechanical
		case 389:  // Damage Modifier - Energy
		case 390:  // Damage Type Modifier (BUFF_PAWN expansion of prop 304)
		// Effect-on-pawn modifiers (BUFF_PAWN expansions)
		case  66:  // Effect GroundSpeed Modifier (Super Tank's shield-speed cancel)
		case 208:  // Effect Lifetime Modifier
		case 210:  // Effect Heal Modifier (Self) (BUFF_2 expansion of prop 51)
		case 261:  // Effect Repair Modifier (BUFF_2 expansion of prop 260)
		case 280:  // Critical Hit Modifier
		case 316:  // Health Restored Modifier (BUFF_2 expansion of prop 51)
		case 330:  // Effect Healing Modifier
		case 376:  // Effect Potency Modifier (POTENCY flag tail)
		case 385:  // Heal Output Modifier (almost universal "secondary" output)
		case 386:  // Buff Resist Modifier (BUFF_PAWN self)
		case 421:  // Threat Modifier
		// BUFF_OTHER identity outputs (queried directly with the prop)
		case 350:  // Pet Damage Modifier
		case 366:  // Pet Max Health Modifier
		// Hand-curated additions (not in FUN_109e5220 outputs but observed on skills)
		case 202:  // Clip Reload Modifier
		case 213:  // Attack Rating Modifier - Melee
		case 215:  // Attack Rating Modifier - Ranged
		case 231:  // Attack Rate Modifier - Melee
		case 232:  // Attack Rate Modifier - Ranged
		case 360:  // DeployRate Modifier Buff
			return true;
		default:
			return false;
	}
}

// ---------------------------------------------------------------------------
// kBuffDeviceModMap — inverse of ConvertPropToPropList(BUFF_DEVICE, *).
//
// Each row: { modifierPropId, { deployable-side base propIds it scales } }.
// First int is the buff-registry key on the player; targets[] is the list of
// deployable s_Properties slots it should scale. -1 terminates the targets
// list. Multiple targets are valid (prop 113 covers four damage variants;
// prop 352 hits both damage-radius and effective-radius; prop 385 hits both
// HP-Max-Deployables and Required-Points-to-Fire).
//
// At deploy time, ApplyPlayerModsToDeployable walks this table, gathers each
// modifierPropId's entries from the deploying player's m_EffectBuffInfo, and
// applies the layered formula to whichever target props the deployable
// actually carries (deployables that don't carry the prop just skip — no-op).
//
// NOTE: this map handles modifiers that target a deployable's s_Properties
// directly. The "heal output" semantic where the deployer's buff scales an
// effect's m_fBase (e.g. medical station's heal amount, which lives on
// asm_data_set_effects.base_value, not a property) is handled separately by
// CloneEffectGroup's [EFFECT-BUFF] block walking m_Instigator back to the
// deployer when the firing actor is a deployable.
// ---------------------------------------------------------------------------
const BuffDeviceMapping kBuffDeviceModMap[] = {
	// AOE Radius Modifier → Damage Radius / Effective Radius
	{ 352, { 6, 343, -1 } },
	// Recharge Rate Modifier → Cooldown
	{ 203, { 4, -1 } },
	// Device Range Modifier → Range
	{ 114, { 5, -1 } },
	// Damage Modifier (generic) → Damage variants
	{ 113, { 10, 245, 246, 249, -1 } },
	// Remote Activation Time Modifier → Activation Time
	{ 349, { 7, -1 } },
	// Prop-46 Modifier
	{ 356, { 46, -1 } },
	// Prop-153 Modifier (Effective Range)
	{ 207, { 153, -1 } },
	// Accuracy Correction Rate Modifier → 247/248
	{ 256, { 247, 248, -1 } },
	// Time To Deploy Modifier → Deploy Time
	{ 391, { 279, -1 } },
	// Max Control Range Modifier → 283
	{ 284, { 283, -1 } },
	// Pet LifeSpan Modifier → 354
	{ 355, { 354, -1 } },
	// Required Points to Fire (self-passthrough; the binary's BUFF_DEVICE
	// expansion of input 339 emits 339 itself plus 385 — i.e. a buff
	// registered under propId 339 directly affects the deployable's prop 339).
	{ 339, { 339, -1 } },
	// Required Morale Points Modifier → Required Points to Fire
	// (binary's BUFF_DEVICE input 318 emits 357 + 385).
	{ 357, { 318, -1 } },
	// Heal Output Modifier — appears as a SECONDARY output in BUFF_DEVICE
	// expansions of prop 339 and prop 318. So a player's prop-385 buff
	// (gained from heal-boosting skills) scales the deployable's
	// HP-Max-Deployables AND Required-Points-to-Fire.
	//
	// This is the "healer ecosystem" pattern: skills that boost heal output
	// also boost the survivability of the healing stations the player
	// deploys, AND reduce their morale-point cost. Mirrors the binary
	// faithfully — semantically odd-looking but engine-correct.
	{ 385, { 339, 318, -1 } },
};

const int kBuffDeviceModMapCount =
	(int)(sizeof(kBuffDeviceModMap) / sizeof(kBuffDeviceModMap[0]));

}  // namespace ModifierProps
