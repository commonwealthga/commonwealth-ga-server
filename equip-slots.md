# Equip Slots

Authoritative mapping of player equip slots: **slot value ID** ↔ **engine equip
point** ↔ **DB label** ↔ **what it actually holds**.

`ATgPawn::m_EquippedDevices[25]` covers indices 0..24. Slot 0 is implicit/empty;
the 24 named slots come from `asm_data_set_valid_values` group **48**
("Equip Slots"). The runtime mapper that translates a slot value ID to an
engine equip point is `FUN_109a1320` in the client binary.

## Engine equip points

| Engine | slot_value_id | DB label                  | C++ constant                | Backing            |
|-------:|--------------:|---------------------------|-----------------------------|--------------------|
| 1      | 221           | ES1  Melee                | `SVID_MELEE`                | Device             |
| 2      | 198           | ES2  Ranged               | `SVID_RANGED`               | Device             |
| 3      | 200           | ES3  Specialty            | `SVID_SPECIALTY`            | Device             |
| 4      | 199           | ES4  ** Unused **         | `SVID_UNUSED_4`             | —                  |
| 5      | 201           | ES5  Travel (Jetpack)     | `SVID_JETPACK`              | Device             |
| 6      | 202           | ES6  Suit                 | `SVID_SUIT`                 | Device             |
| 7      | 203           | ES7  Offhand 1            | `SVID_OFFHAND1`             | Device             |
| 8      | 204           | ES8  Offhand 2            | `SVID_OFFHAND2`             | Device             |
| 9      | 385           | ES9  Offhand 3            | `SVID_OFFHAND3`             | Device             |
| 10     | 386           | ES10 Morale               | `SVID_MORALE`               | Device             |
| 11     | 499           | ES11 Beacon               | `SVID_BEACON`               | Device             |
| 12     | 500           | ES12 Helmet               | `SVID_HELMET`               | Device             |
| 13     | 501           | ES13 Consumable           | `SVID_CONSUMABLE`           | Device             |
| 14     | 502           | ES14 Class Device         | `SVID_CLASS_DEVICE`         | Device             |
| 15     | 823           | ES15 ** Unused **         | `SVID_UNUSED_15`            | —                  |
| 16     | 996           | ES16 Dye Primary          | `SVID_DYE_PRIMARY`          | Item only (no dev) |
| 17     | 997           | ES17 Dye Secondary        | `SVID_DYE_SECONDARY`        | Item only          |
| 18     | 998           | ES18 Dye Emissive         | `SVID_DYE_EMISSIVE`         | Item only          |
| 19     | 999           | ES19 Dye Weapon Primary   | `SVID_DYE_WEAPON_PRIMARY`   | Item only          |
| 20     | 1000          | ES20 Dye Weapon Emissive  | `SVID_DYE_WEAPON_EMISSIVE`  | Item only          |
| 21     | 1001          | ES21 Jetpack Trail Dye    | `SVID_DYE_JETPACK_TRAIL`    | Item only          |
| 22     | 1002          | ES22 ** Unused **         | `SVID_UNUSED_22`            | —                  |
| 23     | 1003          | ES23 ** Unused **         | `SVID_UNUSED_23`            | —                  |
| 24     | 1004          | ES24 ** Unused **         | `SVID_UNUSED_24`            | —                  |

## Item-type taxonomy

`asm_data_set_items.item_type_value_id` partitions everything that can sit in a
slot. **Armor and cosmetics are item-only** — `ref_device_id = 0` — so they do
**not** flow through the device-spawn pipeline used by `ClassLoadouts.cpp`.
They need their own persistence/replication path.

| item_type | label         | n    | ref_device_id | Drives slot           |
|----------:|---------------|-----:|---------------|-----------------------|
| 217       | Weapon        | 1548 | non-zero      | ES1, ES2, ES3, ES7-9  |
| 1106      | Armor         | 124  | 0 (item only) | (no engine slot)      |
| 950       | Appearance    | 412  | 0             | (cosmetic skin)       |
| 1020      | Dye           | 157  | 0             | ES16-ES21             |
| 1612      | Jetpack Trail | 24   | 0             | ES21                  |
| 1412      | Consumable    | 12   | non-zero      | ES13                  |
| 1441      | Armor Mod     | 8    | 0             | applies to armor      |
| 1434      | Weapon Mod    | 7    | 0             | applies to weapon     |
| 686       | Blueprint     | 1343 | n/a           | crafting input        |
| 1171      | Component     | 153  | n/a           | crafting input        |

### Armor body regions
`item_subtype_value_id` on Armor (item_type=1106) gives the body region:

`Head Armor` · `Shoulder Armor` · `Arm Armor` · `Hand Armor` ·
`Chest Armor` · `Leg Armor` · `Feet Armor`

Plus jetpack-internals (also under "Armor" in the DB):
`Analyser` · `Condenser` · `Focus` · `Compressor` · `Repeater` · `Magnifier` ·
`Thruster`.

### Appearance subtypes
Appearance (item_type=950) covers `Helmet` and `Suit` cosmetic skins.

## Device categorization

`asm_data_set_devices.slot_used_value_id` is a *device category* tag, not the
equip slot itself — it indicates which engine slot a device is eligible for.
Useful when sweeping all devices of one kind.

| slot_used_vid | label                 | n   | Typical engine slot     |
|--------------:|-----------------------|----:|-------------------------|
| 388           | Ranged Weapon         | 28  | ES2                     |
| 389           | Melee Weapon          | 25  | ES1                     |
| 390           | Off-Hand              | 68  | ES3 / ES7-9             |
| 392           | Base Human Attributes | 2   | ES14 (devs 864, 7514)   |
| 393           | Player Sensor         | 1   | ?                       |
| 394           | Suit                  | 12  | ES6                     |
| 476           | Morale Device         | 12  | ES10                    |
| 670           | BOT_Ranged            | 282 | (bots, not players)     |
| 671           | BOT_Melee             | 76  | (bots)                  |
| 672           | BOT_Other             | 179 | (bots)                  |
| 731           | BOT_Armor             | 71  | (bots)                  |
| 741           | Helmet                | 3   | ES12                    |
| 806           | Jetpack               | 29  | ES5                     |
| 952           | ITEMS FOR DELETION    | 237 | —                       |
| 956           | SubComponents         | 326 | —                       |
| 981           | Specialty Device      | 30  | ES3 / ES7-9             |
| 1482          | Consumable            | 12  | ES13                    |

## Notes

- **Slot 0** of `m_EquippedDevices[25]` is reserved/empty; engine equip points
  start at **1**.
- **Slots 4, 15, 22-24** are explicitly `** Unused **` in the DB.
- The `SLOT_USED_VALUE_ID` on a device tells you what kind of device it is, not
  where it ends up. The actual slot is determined at equip time via the
  `slot_value_id` passed alongside the device (i.e. the value put in
  `ClassLoadouts::GearSlot::slot_value_id`).
- **Equipping armor or cosmetics** is a separate code path from
  `ClassLoadouts.cpp` — that pipeline only equips device-backed gear via
  `CreateEquipDevice`. Armor (item_type=1106), Appearance (950), and Dye
  (1020) all have `ref_device_id = 0` and need their own replication path
  (likely `ga_characters` / inventory packet, not pawn equip array).
