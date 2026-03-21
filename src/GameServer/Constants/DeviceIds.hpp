#pragma once
#include <string>

namespace GA {
namespace DeviceId {

    // -----------------------------------------------------------------------
    // Device ID constants — verified from:
    //   SELECT i.item_id, i.name_msg_translated
    //   FROM asm_data_set_items i
    //   JOIN asm_data_set_devices d ON d.device_id = i.item_id
    // -----------------------------------------------------------------------

    // -- Assault --
    namespace Assault {
        constexpr int ImpactHammer      = 5801;  // melee, slot 389
        constexpr int RhinoSMG          = 5788;  // ranged, slot 388
        constexpr int InfernoXCannon    = 2914;  // specialty, slot 981
        constexpr int PowerStim         = 3699;  // offhand, slot 390
        constexpr int ConcussionGrenade = 2498;  // offhand, slot 390
        constexpr int RangeShield       = 2013;  // offhand, slot 390
        constexpr int SuperSmashBoost   = 5775;  // morale, slot 476
    }

    // -- Medic --
    namespace Medic {
        constexpr int LifeStealer       = 5800;  // melee, slot 389
        constexpr int Agonizer          = 2991;  // ranged, slot 388
        constexpr int AdrenalineGun     = 6898;  // specialty, slot 981
        constexpr int HealingGrenade    = 2531;  // offhand, slot 390
        constexpr int FrenzyWave        = 3645;  // offhand, slot 390
        constexpr int PoisonGrenade     = 2168;  // offhand, slot 390
        constexpr int HealingBoost      = 2773;  // morale, slot 476
    }

    // -- Recon --
    namespace Recon {
        constexpr int DualDaggers       = 5799;  // melee, slot 389
        constexpr int Ballista          = 2110;  // ranged, slot 388
        constexpr int SpringStealth     = 3023;  // specialty, slot 981
        constexpr int EMPBomb           = 2219;  // offhand, slot 390
        constexpr int VisualScanner     = 2176;  // offhand, slot 390
        constexpr int Decoy             = 2129;  // offhand, slot 390
        constexpr int ShatterBombBoost  = 2113;  // morale, slot 476
    }

    // -- Robotic --
    namespace Robotic {
        constexpr int MaceAndShield     = 5802;  // melee, slot 389
        constexpr int ColonyEnergyRifle = 6885;  // ranged, slot 388
        constexpr int FocusedRepairArm  = 2918;  // specialty, slot 981
        constexpr int PersonalTurret    = 2300;  // offhand, slot 390
        constexpr int MedicalStation    = 2066;  // offhand, slot 390
        constexpr int Sensor            = 2326;  // offhand, slot 390
        constexpr int DomeShieldBoost   = 2886;  // morale, slot 476
    }

    // -- Jetpacks (all classes) --
    namespace Jetpack {
        constexpr int Assault  = 7031;  // slot 806
        constexpr int Medic    = 7032;  // slot 806
        constexpr int Recon    = 7033;  // slot 806
        constexpr int Robotic  = 7034;  // slot 806
    }

    // -- Shared --
    constexpr int RestDevice = 864;  // HUMAN BASE ATTRIBUTES, slot 392

    // -----------------------------------------------------------------------
    // Runtime lookup by device name string (queries DB).
    // Uses asm_data_set_items.name_msg_translated column.
    // Returns device_id or 0 if not found.
    // -----------------------------------------------------------------------
    int Lookup(const std::string& deviceName);

} // namespace DeviceId
} // namespace GA
