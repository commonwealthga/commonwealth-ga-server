#pragma once

#include "src/ControlServer/Constants/DeviceIds.hpp"

// ---------------------------------------------------------------------------
// ClassConfig — per-class identity and equipment constants.
// Used by SpawnPlayerCharacter (Phase 4) and loadout assignment (Phase 5).
// ---------------------------------------------------------------------------

struct ClassConfig {
    uint32_t profileId;
    uint32_t skillGroupSetId;
    // Device IDs for Phase 5 equipment loading
    int meleeDeviceId;
    int rangedDeviceId;
    int specialtyDeviceId;
    int jetpackDeviceId;
    int offhand1DeviceId;
    int offhand2DeviceId;
    int offhand3DeviceId;
    int moraleDeviceId;
};

inline const ClassConfig& GetClassConfig(uint32_t profileId) {
    static const ClassConfig configs[] = {
        // Assault — profileId=680
        {
            680, 19,
            GA::DeviceId::Assault::ImpactHammer,
            GA::DeviceId::Assault::RhinoSMG,
            GA::DeviceId::Assault::InfernoXCannon,
            GA::DeviceId::Jetpack::Assault,
            GA::DeviceId::Assault::PowerStim,
            GA::DeviceId::Assault::ConcussionGrenade,
            GA::DeviceId::Assault::RangeShield,
            GA::DeviceId::Assault::SuperSmashBoost,
        },
        // Medic — profileId=567
        {
            567, 11,
            GA::DeviceId::Medic::LifeStealer,
            GA::DeviceId::Medic::Agonizer,
            GA::DeviceId::Medic::AdrenalineGun,
            GA::DeviceId::Jetpack::Medic,
            GA::DeviceId::Medic::HealingGrenade,
            GA::DeviceId::Medic::FrenzyWave,
            GA::DeviceId::Medic::PoisonGrenade,
            GA::DeviceId::Medic::HealingBoost,
        },
        // Recon — profileId=681
        {
            681, 17,
            GA::DeviceId::Recon::DualDaggers,
            GA::DeviceId::Recon::Ballista,
            GA::DeviceId::Recon::SpringStealth,
            GA::DeviceId::Jetpack::Recon,
            GA::DeviceId::Recon::EMPBomb,
            GA::DeviceId::Recon::VisualScanner,
            GA::DeviceId::Recon::Decoy,
            GA::DeviceId::Recon::ShatterBombBoost,
        },
        // Robotic — profileId=679
        {
            679, 18,
            GA::DeviceId::Robotic::MaceAndShield,
            GA::DeviceId::Robotic::ColonyEnergyRifle,
            GA::DeviceId::Robotic::FocusedRepairArm,
            GA::DeviceId::Jetpack::Robotic,
            GA::DeviceId::Robotic::PersonalTurret,
            GA::DeviceId::Robotic::MedicalStation,
            GA::DeviceId::Robotic::Sensor,
            GA::DeviceId::Robotic::DomeShieldBoost,
        },
    };

    switch (profileId) {
        case 680: return configs[0]; // Assault
        case 567: return configs[1]; // Medic
        case 681: return configs[2]; // Recon
        case 679: return configs[3]; // Robotic
        default:  return configs[0]; // Default to Assault
    }
}

namespace GA_G {
	enum GA_G {
		PROFILE_ID_ASSAULT = 0x2A8, // 680
		PROFILE_ID_MEDIC = 0x237, // 567
		PROFILE_ID_RECON = 0x2A9, // 681
		PROFILE_ID_ROBOTIC = 0x2A7, // 679

		SKILL_GROUP_SET_ID_ASSAULT = 19,
		SKILL_GROUP_SET_ID_MEDIC = 11,
		SKILL_GROUP_SET_ID_RECON = 17,
		SKILL_GROUP_SET_ID_ROBOTIC = 18,
		SKILL_GROUP_SET_ID_DEVCLASS = 5,
		SKILL_GROUP_SET_ID_NEW_CHARACTER = 23,
		
		HEAD_ASM_ID_TROLL = 2842,

		DYE_ID_NONE_MORE_BLACK = 7552,

		GENDER_TYPE_ID_FEMALE = 852,
		GENDER_TYPE_ID_MALE = 853,

		QUEUE_TYPE_VALUE_ID_1 = 0x58C,
		QUEUE_TYPE_VALUE_ID_2 = 0x58D,
		QUEUE_TYPE_VALUE_ID_3 = 0x5AE,
		QUEUE_TYPE_VALUE_ID_4 = 0x3FD,
		QUEUE_TYPE_VALUE_ID_5 = 0x3FE,

		// TgDevice::m_nDeviceType
		TGDT_RANGED = 388,
		TGDT_SPECIALTY = 981,
		TGDT_MELEE = 389,
		TGDT_OFF_HAND = 390,
		TGDT_BASE_HUMAN_ATTRIB = 392, // REST device
		TGDT_PLAYER_SENSOR = 393, // beacon slot perhaps?
		TGDT_MORALE = 476,
		TGDT_TRAVEL = 806, // jetpack

		DEVICE_ID_ASSAULT_CJP = 7031,
		DEVICE_ID_MEDIC_CJP = 7032,
		DEVICE_ID_RECON_CJP = 7033,
		DEVICE_ID_ROBOTIC_CJP = 7034,

		DEVICE_ID_AGONIZER = 2991,

		DEVICE_TYPE_ID_DEVICE = 217,

		EQUIP_POINT_JETPACK_ID = 5,

		BOT_TYPE_VALUE_ID_AGENT = 373,
		BOT_TYPE_VALUE_ID_ELITE = 1101,
		BOT_TYPE_VALUE_ID_BOSS = 1100,
		BOT_TYPE_VALUE_ID_GUARDIAN = 1099,

		BODY_ASM_ID_AGENT = 2685,
		BODY_ASM_ID_SUPPORT_GUARDIAN = 757,
		BODY_ASM_ID_ELITE_TECHRO = 2179,
		BODY_ASM_ID_ELITE_ASSASSIN = 2176,
		BODY_ASM_ID_ELITE_HELOT = 2177,
		BODY_ASM_ID_THINKTANK_BOSS = 2105,

		BOT_ID_SUPPORT_GUARDIAN = 1384,
		BOT_ID_ELITE_TECHRO = 1432,
		BOT_ID_ELITE_ASSASSIN = 1320,
		BOT_ID_ELITE_HELOT = 1321,
		BOT_ID_THINKTANK_BOSS = 1417,
		BOT_ID_COMMONWEALTH_DREADNAUGHT = 1143,
		
		BOT_ID_COLONY_CORE = 1604,
		BOT_ID_COLONY_ELECTROCUTIONER = 1587,
		BOT_ID_COLONY_OVERLORD = 1625,
		BOT_ID_COLONY_ANDROID = 1468,
		BOT_ID_COLONY_ANT = 1515,
		BOT_ID_COLONY_DREADNOUGHT = 1609,
		BOT_ID_COLONY_GUARDIAN = 1461,
		BOT_ID_COLONY_GUARDIAN_HALLOWEEN = 1593,
		BOT_ID_COLONY_HUNTER_SPIDER = 1472,
		BOT_ID_COLONY_JUGGERNAUT = 1574,
		BOT_ID_COLONY_REPAIR_DRONE = 1576,
		BOT_ID_COLONY_REPAIR_DRONE_NOLOOT = 1616,
		BOT_ID_COLONY_SAND_SPIDER = 1458,
		BOT_ID_COLONY_SCAVENGER = 1626,
		BOT_ID_COLONY_SCORPION = 1606,
		BOT_ID_COLONY_SENTRY = 1536,
		BOT_ID_COLONY_SOLDIER = 1492,
		BOT_ID_COLONY_SOLDIER_RC = 1578,
		BOT_ID_COLONY_TICK = 1463,
		BOT_ID_COLONY_WASP = 1535,
		BOT_ID_COLONY_WASP_MK2 = 1617,

		PHYSICAL_TYPE_VALUE_ID_AGENT = 860,
		PHYSICAL_TYPE_VALUE_ID_THINKTANK = 973,
		PHYSICAL_TYPE_VALUE_ID_GUARDIAN = 861,

		CURRENCY_TYPE_VALUE_ID_CREDITS = 1603,
		CURRENCY_TYPE_VALUE_ID_TOKENS = 1604,
		CURRENCY_TYPE_VALUE_ID_AGENDA_PTS = 1605,
	};
};

