# MapDataDumper DB-only Refactor (v33)

Status: design — brainstorming complete, awaiting user spec review before writing-plans hand-off.
Date: 2026-05-15

## Goal

Replace the current file-and-DB hybrid `MapDataDumper` with a DB-only logger that writes one row per actor per ancestor class into a new family of `map_*` tables. The new tables hold **raw original** map data; the existing `obj_*` tables continue to hold **manual gameplay overrides** (e.g. boss factory re-pointing, fallback `bot_id` / `spawn_table_id` / `task_force_number`). The two are linked by `map_object_id` for joins.

Triggered by the existing `-dumpmapdata=1` server-launch switch via `MapDataDumper::DumpAll()` called from `World__BeginPlay`. No new entry point.

## Non-goals

- No change to the `obj_*` tables, the `-dumpmapdata` chat command wiring, or any LoadObjectConfig path.
- No re-import of the existing `obj_map_object` data into the new schema. Both schemas coexist.
- No tracking of object-reference vars (pointers to other actors) — scalars only.
- No tracking of arrays beyond the two the user listed (`TgBotFactory.LocationList`, `TgBotFactory.PatrolPath`).

## Class set & inheritance

User-listed classes marked ★. Other classes are surfaced because they declare `m_nMapObjectId` directly (or inherit it through a declaring parent). `TgTrigger_Use` is the only dropped class — genuinely has no `m_nMapObjectId` anywhere in its chain.

```
★ TgTeamPlayerStart → TgStartPoint ★

★ TgMissionObjective
   ├── TgMissionObjective_Bot
   │     └── TgBaseObjective_CTFBot
   ├── TgMissionObjective_Kismet
   └── TgMissionObjective_Proximity
         ├── TgBaseObjective_KOTH
         └── TgMissionObjective_Escort

★ TgStartPoint
   └── TgTeamPlayerStart

★ TgOmegaVolume                     (no subclasses)
★ TgModifyPawnPropertiesVolume      (no subclasses)
★ TgDeviceVolume                    (no subclasses)
★ TgTeleporter                      (no subclasses)

★ TgActorFactory
   ├── TgBeaconFactory
   ├── TgBotFactory
   │     └── TgBotFactorySpawnable     (no own vars)
   ├── TgDeployableFactory
   ├── TgDestructibleFactory           (no own scalar vars)
   └── TgHexItemFactory

   TgNavigationPoint                       -- declares m_nMapObjectId
   ├── ★ TgBotStart                        -- inherits m_nMapObjectId (resurrected — user's original list)
   ├── TgActionPoint
   ├── TgAlarmPoint
   ├── TgCoverPoint
   ├── TgDefensePoint
   │     └── TgHoldSpot
   ├── TgNavigationPointSpawnable
   └── TgPointOfInterest

   TgFlagCaptureVolume                 (no subclasses; Volume-derived)
   TgHelpAlertVolume                   (no subclasses; Volume-derived)
   TgMissionListVolume                 (no subclasses; Volume-derived)

DROPPED entirely (no m_nMapObjectId anywhere in chain):
- TgTrigger_Use
```

**Multi-table writes** — a leaf-class instance writes one row in **every** ancestor table up to the root listed class. Example: a `TgBaseObjective_CTFBot` instance writes to `map_tg_mission_objective` AND `map_tg_mission_objective_bot` AND `map_tg_base_objective_ctf_bot`, each row carrying the same `map_object_id`.

## Table set (34 total)

32 class tables + 2 array tables.

### Common columns (every `map_<class>` table)

```sql
id              INTEGER PRIMARY KEY AUTOINCREMENT
map_name        TEXT NOT NULL
map_object_id   INTEGER             -- from actor->m_nMapObjectId; the linker
class_name      TEXT NOT NULL       -- leaf class, distinguishes subclasses in parent tables
tag             TEXT                -- Actor.Tag (FName as string)
"group"         TEXT                -- Actor.Group (quoted, reserved word)
location_x      REAL
location_y      REAL
location_z      REAL
rotation_pitch  INTEGER
rotation_yaw    INTEGER
rotation_roll   INTEGER
```

`CREATE INDEX idx_<table>_map_object_id ON <table>(map_object_id);` on every table.

### Class-specific scalar columns

| Table | Own-class scalars |
|---|---|
| `map_tg_team_player_start` | `m_n_task_force` (INTEGER, byte), `m_e_coalition` (INTEGER, enum), `m_n_priority`, `n_prev_priority`, `m_n_min_level` |
| `map_tg_mission_objective` | `b_has_no_location`, `r_b_use_pending_state`, `s_b_change_coalition_when_captured`, `m_b_capture_only_once`, `r_b_has_been_captured_once`, `s_b_end_overtime_on_defender_progress`, `b_enabled`, `r_b_is_locked`, `r_b_is_active`, `r_b_is_pending`, `m_b_pause_on_capture`, `c_b_local_pawn_is_attacker`, `m_b_block_advance`, `m_b_is_base_objective`, `c_b_is_local_player_attacker`, `c_b_local_player_attacker_cached`, `m_b_in_matinee_update`, `m_b_start_objective`, `s_b_random_picked`, `m_b_teleport_bots`, `n_priority`, `n_message_id`, `n_default_owner_task_force`, `n_objective_id`, `hex_bonus_direction` (enum), `r_open_world_player_default_role` (enum), `r_e_default_coalition` (enum), `r_e_status` (enum), `r_e_owning_coalition` (enum), `s_open_world_bot_task_force`, `m_f_proximity_radius`, `m_f_proximity_height`, `m_f_time_to_capture`, `m_f_time_to_hold`, `m_n_points_per_second`, `m_n_type_id`, `m_n_desc_msg_id`, `m_n_icon_id`, `m_n_min_agents`, `m_n_cooldown_seconds`, `m_n_points`, `m_n_credits`, `m_n_reward_xp`, `m_n_loot_table_id`, `m_f_curr_capture_time`, `m_n_curr_owner_taskforce`, `r_n_owner_task_force`, `r_f_curr_capture_time`, `m_n_capture_time_ext_secs`, `m_n_capture_time_reset_secs`, `s_f_attacker_captured_at`, `r_f_last_completed_time`, `c_f_percentage`, `s_f_previous_play_rate`, `m_f_start_time`, `m_f_stop_time`, `s_n_previous_priority`, `s_n_current_spawn_order` |
| `map_tg_mission_objective_bot` | `s_n_bot_id`, `s_n_spawn_table_id`, `s_b_auto_spawn`, `b_patrol_loop`, `f_balance`, `n_global_alarm_id` |
| `map_tg_mission_objective_kismet` | — (common cols only) |
| `map_tg_mission_objective_proximity` | `s_b_allow_ai_bot_interaction`, `s_b_allow_remote_control_bot_interaction`, `m_f_def_capture_rate`, `m_f_capture_accel_rate`, `m_f_capture_accel_rate_cap`, `r_f_capture_rate`, `m_f_overtime_accel_rate` |
| `map_tg_base_objective_ctf_bot` | `m_f_flag_respawn_delay`, `m_f_scoring_radius`, `m_b_spawn_unaligned_bot`, `m_b_capture_on_death` |
| `map_tg_base_objective_koth` | `c_f_prev_client_capt_time` |
| `map_tg_mission_objective_escort` | — (common cols only) |
| `map_tg_start_point` | `m_n_start_group`, `m_n_return_map_type`, `m_n_return_map_game_id`, `m_f_start_rating`, `m_f_current_rating`, `m_f_reset_rating`, `m_f_decrease_rate`, `availability_quest_group_id` |
| `map_tg_omega_volume` | `m_n_omega_alert_id`, `m_n_subzone_name_msg_id`, `m_n_subzone_secondary_name_msg_id`, `m_n_omega_priority`, `m_n_bilboard_key`, `m_b_enable_equip`, `m_b_enable_skills`, `m_b_enable_crafting`, `m_b_auto_kick_if_idle`, `m_e_visual_cue` (enum) |
| `map_tg_modify_pawn_properties_volume` | `m_b_disable_jump`, `m_b_disable_block_actors`, `m_b_disable_hanging`, `m_b_disable_all_devices`, `m_b_trigger_use_event`, `m_b_one_way_movement`, `m_v_onew_way_pitch`, `m_v_onew_way_yaw`, `m_v_onew_way_roll`, `s_n_loot_table_id` |
| `map_tg_device_volume` | `b_pain_causing`, `s_b_device_active`, `s_n_device_id`, `s_n_team_number`, `s_n_task_force` |
| `map_tg_actor_factory` | `s_b_auto_spawn`, `s_n_team_number`, `s_n_task_force`, `s_e_coalition` (enum), `s_e_selection_method` (enum), `s_n_selection_list_id`, `s_n_selected_object_id`, `m_n_selection_list_prop_id`, `s_n_name_id`, `s_n_cur_list_index` |
| `map_tg_bot_factory` | `location_selection` (enum), `type_selection` (enum), `s_n_cur_location_index`, `b_patrol_loop`, `b_always_patrol`, `b_spawn_on_alarm`, `b_auto_spawn`, `m_b_first_spawn`, `b_bulk_spawn`, `b_respawn`, `m_b_ignore_collision_on_spawn`, `n_global_alarm_id`, `n_bot_count`, `n_current_count`, `n_active_count`, `n_total_spawns`, `n_spawn_table_id`, `n_default_spawn_table_id`, `f_spawn_delay`, `m_n_last_group`, `n_priority`, `n_prev_priority`, `f_last_kill_time`, `f_balance`, `f_respawn_delay`, `m_n_spawn_order` |
| `map_tg_bot_factory_spawnable` | — (common cols only) |
| `map_tg_beacon_factory` | `m_n_priority`, `m_n_prev_priority`, `m_b_beacon_exit`, `m_b_is_fallback` |
| `map_tg_deployable_factory` | `n_current_count`, `s_f_last_spawn_time`, `s_b_spawn_once` |
| `map_tg_destructible_factory` | — (common cols only) |
| `map_tg_hex_item_factory` | `s_b_needs_spawn` |
| `map_tg_teleporter` | `m_n_map_id`, `m_n_preload`, `m_b_set_task_force`, `m_b_balance_task_force`, `m_b_ignore_non_members`, `m_b_use_player_start`, `m_b_request_mission`, `m_n_start_group`, `m_n_task_force` |
| `map_tg_flag_capture_volume` | `r_n_task_force`, `r_e_coalition` (enum) |
| `map_tg_help_alert_volume` | `m_n_help_id` |
| `map_tg_mission_list_volume` | `s_n_queue_table_id`, `s_n_queue_table_msg_id` |
| `map_tg_navigation_point` | — (common cols only; declares m_nMapObjectId, no other scalars) |
| `map_tg_bot_start` | — (common cols only) |
| `map_tg_action_point` | `action_type` (enum), `n_objective_num`, `n_task_force`, `b_use_rotation` |
| `map_tg_alarm_point` | — (common cols only) |
| `map_tg_cover_point` | `m_b_lean_left`, `m_b_lean_right`, `m_b_allow_popup`, `m_b_allow_mantle`, `m_v_lean_left_x/y/z`, `m_v_lean_right_x/y/z`, `m_v_pop_up_x/y/z` |
| `map_tg_defense_point` | `b_first_script`, `b_sniping`, `b_dont_change_position`, `b_avoid`, `b_disabled`, `b_not_in_vehicle`, `priority`, `num_checked` |
| `map_tg_hold_spot` | — (common cols only; HoldVehicle is a ref, skipped) |
| `map_tg_navigation_point_spawnable` | — (common cols only) |
| `map_tg_point_of_interest` | `m_n_name_msg_id`, `m_s_debug_name`, `m_quest_radius_uu`, `m_b_show_when_quest_complete` |

### Array tables

```sql
CREATE TABLE map_tg_bot_factory_location_list (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    map_name        TEXT NOT NULL,
    map_object_id   INTEGER NOT NULL,  -- parent factory's m_nMapObjectId
    array_index     INTEGER NOT NULL,
    location_x      REAL,
    location_y      REAL,
    location_z      REAL,
    nav_point_tag   TEXT               -- the nav point's own Tag
);
CREATE INDEX idx_map_tg_bot_factory_location_list_map_object_id
    ON map_tg_bot_factory_location_list(map_object_id);

CREATE TABLE map_tg_bot_factory_patrol_path (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    map_name        TEXT NOT NULL,
    map_object_id   INTEGER NOT NULL,
    array_index     INTEGER NOT NULL,
    location_x      REAL,
    location_y      REAL,
    location_z      REAL,
    nav_point_tag   TEXT
);
CREATE INDEX idx_map_tg_bot_factory_patrol_path_map_object_id
    ON map_tg_bot_factory_patrol_path(map_object_id);
```

## Type mappings

| UC type | SQLite type | Notes |
|---|---|---|
| `int` | `INTEGER` | |
| `float` | `REAL` | |
| `bool` | `INTEGER` | 0 / 1 |
| `byte` | `INTEGER` | |
| `byte` enum (e.g. `TG_Coalition`, `eSelectionMethod`) | `INTEGER` | Stores numeric value. No name lookup table; query against enum constants in code. |
| `name` (FName) | `TEXT` | Stored as the resolved name string. |
| `string` | `TEXT` | |
| `Vector` | three `REAL` columns | Suffix `_x`, `_y`, `_z`. |
| `Rotator` | three `INTEGER` columns | Suffix `_pitch`, `_yaw`, `_roll`. UE3 rotator units (65536 / full turn). |

Skipped: object pointers, components, materials, textures, arrays not on the user's whitelist.

## Code architecture

```
src/GameServer/Engine/MapDataDumper/
    MapDataDumper.hpp                     -- public class MapDataDumper { static void DumpAll(); }  (signature unchanged)
    MapDataDumper.cpp                     -- entry point, dispatch table, transaction wrap
    Writers/
        TgTeamPlayerStart.{hpp,cpp}
        TgMissionObjective.{hpp,cpp}
        TgMissionObjective_Bot.{hpp,cpp}
        TgMissionObjective_Kismet.{hpp,cpp}
        TgMissionObjective_Proximity.{hpp,cpp}
        TgBaseObjective_CTFBot.{hpp,cpp}
        TgBaseObjective_KOTH.{hpp,cpp}
        TgMissionObjective_Escort.{hpp,cpp}
        TgStartPoint.{hpp,cpp}
        TgOmegaVolume.{hpp,cpp}
        TgModifyPawnPropertiesVolume.{hpp,cpp}
        TgDeviceVolume.{hpp,cpp}
        TgActorFactory.{hpp,cpp}
        TgBotFactory.{hpp,cpp}
        TgBotFactorySpawnable.{hpp,cpp}
        TgBeaconFactory.{hpp,cpp}
        TgDeployableFactory.{hpp,cpp}
        TgDestructibleFactory.{hpp,cpp}
        TgHexItemFactory.{hpp,cpp}
        TgTeleporter.{hpp,cpp}
        TgFlagCaptureVolume.{hpp,cpp}
        TgHelpAlertVolume.{hpp,cpp}
        TgMissionListVolume.{hpp,cpp}
        TgNavigationPoint.{hpp,cpp}
        TgBotStart.{hpp,cpp}
        TgActionPoint.{hpp,cpp}
        TgAlarmPoint.{hpp,cpp}
        TgCoverPoint.{hpp,cpp}
        TgDefensePoint.{hpp,cpp}
        TgHoldSpot.{hpp,cpp}
        TgNavigationPointSpawnable.{hpp,cpp}
        TgPointOfInterest.{hpp,cpp}
        TgBotFactoryLocationList.{hpp,cpp}      -- array writer
        TgBotFactoryPatrolPath.{hpp,cpp}        -- array writer
```

**Writer signature** (uniform):

```cpp
namespace MapDumpWriters {
    void WriteTgMissionObjective(sqlite3* db, AActor* actor, const std::string& mapName);
    // ... one per file
}
```

Each writer:
1. Casts the `AActor*` to its UC type.
2. Holds a `static const char*` INSERT SQL string.
3. Calls `sqlite3_prepare_v2` → bind common cols + own scalars → `sqlite3_step` → `sqlite3_finalize`.
4. Reads `m_nMapObjectId` from the actor (offset known per the SDK).

Array writers (`WriteTgBotFactoryLocationList`, `...PatrolPath`) iterate the array, one INSERT per element, all sharing the parent factory's `map_object_id`.

**Dispatch** (in `MapDataDumper.cpp`, anonymous namespace):

```cpp
void WriteByClass(sqlite3* db, AActor* actor, const std::string& mapName) {
    std::string cls = ExtractClassName(actor->Class->GetFullName());

    if (cls == "TgBaseObjective_CTFBot") {
        Writers::WriteTgMissionObjective(db, actor, mapName);
        Writers::WriteTgMissionObjective_Bot(db, actor, mapName);
        Writers::WriteTgBaseObjective_CTFBot(db, actor, mapName);
    }
    else if (cls == "TgBaseObjective_KOTH") {
        Writers::WriteTgMissionObjective(db, actor, mapName);
        Writers::WriteTgMissionObjective_Proximity(db, actor, mapName);
        Writers::WriteTgBaseObjective_KOTH(db, actor, mapName);
    }
    else if (cls == "TgHoldSpot") {
        Writers::WriteTgNavigationPoint(db, actor, mapName);
        Writers::WriteTgDefensePoint(db, actor, mapName);
        Writers::WriteTgHoldSpot(db, actor, mapName);
    }
    else if (cls == "TgBotStart" || cls == "TgActionPoint" ||
             cls == "TgAlarmPoint" || cls == "TgCoverPoint" ||
             cls == "TgDefensePoint" || cls == "TgNavigationPointSpawnable" ||
             cls == "TgPointOfInterest") {
        Writers::WriteTgNavigationPoint(db, actor, mapName);
        // dispatch to the own-class writer (TgBotStart → WriteTgBotStart, etc.)
        // ... one branch each, omitted for brevity
    }
    // ... etc, one branch per leaf class in scope
}
```

**Top-level `DumpAll`** (in `MapDataDumper.cpp`):

```cpp
void MapDataDumper::DumpAll() {
    UWorld* world = (UWorld*)Globals::Get().GWorld;
    if (!world) return;

    sqlite3* db = Database::GetConnection();
    std::string mapName = ExtractMapName(world);   // strips package prefix from world->GetFullName()

    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);

    // 1. Wipe prior rows for this map across every map_* table.
    DeletePriorRows(db, mapName);

    // 2. Walk actors.
    for (each level in world->Levels)
        for (each actor in level->Actors)
            if (actor) WriteByClass(db, actor, mapName);

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
}
```

`DeletePriorRows` issues one `DELETE FROM <table> WHERE map_name = ?` per `map_*` table — table list is a compile-time `static const char* kTables[] = { ... }` in `MapDataDumper.cpp`.

**Removal of file logging**: the current dumper writes a text log alongside its DB inserts. The refactor deletes all `fopen` / `fprintf` / `fclose` calls. No new file output of any kind.

## Migration v33

Single migration in `src/Database/Database.cpp`, immediately after the `if (version < 32)` block. Follows the existing pattern: plain concatenated `const char*` SQL, one `sqlite3_exec` call per logical chunk, error-checked, with the final `UPDATE version_info SET version = 33` outside the block (the existing v32 line gets bumped to 33).

Concrete example for one table — the rest follow the same shape:

```cpp
if (version < 33) {
    const char* kV33_create_tg_team_player_start =
        "CREATE TABLE map_tg_team_player_start ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  map_name TEXT NOT NULL,"
        "  map_object_id INTEGER,"
        "  class_name TEXT NOT NULL,"
        "  tag TEXT,"
        "  \"group\" TEXT,"
        "  location_x REAL, location_y REAL, location_z REAL,"
        "  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
        "  m_n_task_force INTEGER,"
        "  m_e_coalition INTEGER,"
        "  m_n_priority INTEGER,"
        "  n_prev_priority INTEGER,"
        "  m_n_min_level INTEGER"
        ");"
        "CREATE INDEX idx_map_tg_team_player_start_map_object_id "
        "  ON map_tg_team_player_start(map_object_id);";
    result = sqlite3_exec(db, kV33_create_tg_team_player_start, nullptr, nullptr, &err);
    if (result != SQLITE_OK) {
        Logger::Log("db", "Failed v33 (map_tg_team_player_start): %s\n", err);
        return;
    }

    // ... 33 more CREATE TABLE + CREATE INDEX statements,
    // one sqlite3_exec call per logical group, each with the same
    // error-check pattern. Final per-table list lives in the implementation
    // plan to keep this spec readable.

    Logger::Log("db", "v33: created 34 map_* tables for raw map dump data\n");
}

// Existing version-bump line at end of MigrateDatabase — change the literal:
result = sqlite3_exec(db, "UPDATE version_info SET version = 33", nullptr, nullptr, &err);
```

No DROP / no data migration — the new tables exist alongside the existing `obj_*` and `obj_map_object`. Re-dumping populates them.

## Open questions / future work

- A future enum-name lookup table could make the `INTEGER` enum columns more readable in ad-hoc queries. Not required now.
- A future migration could re-import the existing `obj_map_object` rows into the new tables for archival, but the user is OK leaving that be.
- If maps add new classes outside this set later, they're silently ignored. Add a new writer file + a dispatch branch.

## Verification plan

**All steps are user-verified manually. Claude must NOT run `make` or any build command — the user builds and tests themselves.**

1. User builds with `make`; build succeeds.
2. User boots the server; no DB errors; `PRAGMA user_version` returns 33.
3. After joining `1P_CPLab05_P` with `-dumpmapdata=1`: every `map_*` table that the map actually uses has rows for `map_name = '1P_CPLab05_P'` (some tables stay empty if the map has no such actors — that's expected). Counts match what the legacy file dump produced (modulo TgTrigger_Use, intentionally dropped — no `m_nMapObjectId`).
4. Restarting the server with `-dumpmapdata=1` on the **same** map results in the same row count for that map (DELETE-then-INSERT is idempotent per `map_name`).
5. Restarting with `-dumpmapdata=1` on a **different** map adds rows for the new `map_name` without touching the first map's rows.
6. `SELECT location_x, location_y, location_z, class_name FROM map_tg_bot_factory WHERE map_name = '1P_CPLab05_P';` returns the same coords as the previously-written `obj_map_object` rows for those factory ids.

---

End of design.
