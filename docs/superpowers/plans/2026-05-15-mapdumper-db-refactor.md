# MapDataDumper DB-Only Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the file+DB hybrid map dumper with a DB-only one that writes every actor into a hierarchy of `map_*` tables (one row per ancestor class).

**Architecture:** Migration v33 creates 34 tables (32 class + 2 array). `MapDataDumper::DumpAll()` is kept as the entry point but its body is rewritten to dispatch each actor to a chain of per-class writer functions under `Writers/<ClassName>.{hpp,cpp}`, each of which INSERTs one row into its own table.

**Tech Stack:** C++ (UE3 SDK casts, SQLite via `sqlite3_*` C API), `Makefile` (explicit cpp list), no test infra (user verifies manually after build).

**Spec:** `docs/superpowers/specs/2026-05-15-mapdumper-db-refactor-design.md`. Read first.

**Build constraint:** Claude MUST NOT run `make` or any build command. User builds themselves. Every task ends in a `git add` + `git commit`, no compile step.

---

## File structure

**New files** (created in this plan):

```
src/GameServer/Engine/MapDataDumper/
    Writers/
        WriterCommon.hpp                       -- BindCommonCols, ExtractClassName, FNameStr helpers
        WriterCommon.cpp
        TgTeamPlayerStart.{hpp,cpp}
        TgStartPoint.{hpp,cpp}
        TgMissionObjective.{hpp,cpp}
        TgMissionObjective_Bot.{hpp,cpp}
        TgMissionObjective_Kismet.{hpp,cpp}
        TgMissionObjective_Proximity.{hpp,cpp}
        TgBaseObjective_CTFBot.{hpp,cpp}
        TgBaseObjective_KOTH.{hpp,cpp}
        TgMissionObjective_Escort.{hpp,cpp}
        TgOmegaVolume.{hpp,cpp}
        TgModifyPawnPropertiesVolume.{hpp,cpp}
        TgDeviceVolume.{hpp,cpp}
        TgFlagCaptureVolume.{hpp,cpp}
        TgHelpAlertVolume.{hpp,cpp}
        TgMissionListVolume.{hpp,cpp}
        TgActorFactory.{hpp,cpp}
        TgBotFactory.{hpp,cpp}
        TgBotFactorySpawnable.{hpp,cpp}
        TgBeaconFactory.{hpp,cpp}
        TgDeployableFactory.{hpp,cpp}
        TgDestructibleFactory.{hpp,cpp}
        TgHexItemFactory.{hpp,cpp}
        TgNavigationPoint.{hpp,cpp}
        TgBotStart.{hpp,cpp}
        TgActionPoint.{hpp,cpp}
        TgAlarmPoint.{hpp,cpp}
        TgCoverPoint.{hpp,cpp}
        TgDefensePoint.{hpp,cpp}
        TgHoldSpot.{hpp,cpp}
        TgNavigationPointSpawnable.{hpp,cpp}
        TgPointOfInterest.{hpp,cpp}
        TgTeleporter.{hpp,cpp}
        TgBotFactoryLocationList.{hpp,cpp}     -- array writer
        TgBotFactoryPatrolPath.{hpp,cpp}       -- array writer
```

**Modified files:**

```
src/Database/Database.cpp                       -- add `if (version < 33) { ... }` block + bump UPDATE
src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp  -- gut, replace with dispatch shell
Makefile                                        -- add 35 new .cpp paths
```

`MapDataDumper.hpp` keeps the same `static void DumpAll();` surface — no change to the call site at `World__BeginPlay.cpp:20`.

---

## Common writer template

Every class writer follows this exact shape. Vary only the `INSERT` SQL columns and the own-scalar bindings.

`Writers/<ClassName>.hpp`:

```cpp
#pragma once
#include "src/pch.hpp"
#include <string>

namespace MapDumpWriters {
    void Write<ClassName>(sqlite3* db, AActor* actor,
                          const std::string& mapName,
                          const std::string& className,
                          int mapObjectId);
}
```

`Writers/<ClassName>.cpp`:

```cpp
#include "src/GameServer/Engine/MapDataDumper/Writers/<ClassName>.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {
void Write<ClassName>(sqlite3* db, AActor* actor,
                      const std::string& mapName,
                      const std::string& className,
                      int mapObjectId) {
    auto* a = static_cast<ATg<ClassName>*>(actor);

    const char* kSql =
        "INSERT INTO map_<snake_class_name> ("
        "map_name, map_object_id, class_name, tag, \"group\","
        " location_x, location_y, location_z,"
        " rotation_pitch, rotation_yaw, rotation_roll"
        // -- append own scalars here
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
        // -- append same number of placeholders here
        ")";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Log("mapdump", "prepare failed for map_<snake>: %s\n", sqlite3_errmsg(db));
        return;
    }
    BindCommonCols(stmt, actor, mapObjectId, mapName, className);
    // Bind own scalars starting at index 12:
    //   sqlite3_bind_int   (stmt, 12, a->myScalar);
    //   sqlite3_bind_double(stmt, 13, a->myFloat);
    //   ...
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Logger::Log("mapdump", "insert failed for map_<snake>: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}
}  // namespace MapDumpWriters
```

**Type-to-bind mapping:**

| Field type | Bind call |
|---|---|
| `int` | `sqlite3_bind_int(stmt, idx, a->field)` |
| `float` | `sqlite3_bind_double(stmt, idx, (double)a->field)` |
| `bool` (in-class bitfield) | `sqlite3_bind_int(stmt, idx, a->field ? 1 : 0)` |
| `byte` (incl. enum) | `sqlite3_bind_int(stmt, idx, (int)a->field)` |
| `FName` | `sqlite3_bind_text(stmt, idx, FNameStr(a->field), -1, SQLITE_TRANSIENT)` |
| `FString` | `auto s = FStringToUtf8(a->field); sqlite3_bind_text(stmt, idx, s.c_str(), -1, SQLITE_TRANSIENT)` |
| `FVector` | three binds: `bind_double` for X, Y, Z to consecutive idx |
| `FRotator` | three binds: `bind_int` for Pitch, Yaw, Roll to consecutive idx |

---

## Dispatch logic in `MapDataDumper.cpp`

After every per-hierarchy writer task, append the corresponding branches here. By the final task this `if/else` chain covers every leaf class in scope.

```cpp
void WriteByClass(sqlite3* db, AActor* actor, const std::string& mapName) {
    if (!actor || !actor->Class) return;
    std::string cls = ExtractClassName(actor->Class->GetFullName());
    if (cls.empty()) return;

    // dispatch branches inserted incrementally per task
}
```

---

## Task 1: Migration v33 — create all 34 `map_*` tables

**Files:**
- Modify: `src/Database/Database.cpp` (insert new block after the `if (version < 32) { ... }` block; bump the final `UPDATE version_info SET version = 32` → `33`)

- [ ] **Step 1: Add `if (version < 33)` block above the version-bump line**

Insert immediately before `result = sqlite3_exec(db, "UPDATE version_info SET version = 32", ...);`.
Use one `sqlite3_exec` call per CREATE TABLE + CREATE INDEX pair (35 logical chunks) so any failure log identifies the offending table.

Worked example for one table — the rest follow the same pattern. **Every** table has the same 11 common columns (`map_name`, `map_object_id`, `class_name`, `tag`, `"group"`, `location_x`, `location_y`, `location_z`, `rotation_pitch`, `rotation_yaw`, `rotation_roll`); only the own-class columns differ.

```cpp
if (version < 33) {
    const char* kV33_team_player_start =
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
    result = sqlite3_exec(db, kV33_team_player_start, nullptr, nullptr, &err);
    if (result != SQLITE_OK) {
        Logger::Log("db", "Failed v33 (map_tg_team_player_start): %s\n", err);
        return;
    }

    // ... 33 more pairs in order: tg_start_point, tg_mission_objective,
    // tg_mission_objective_bot, tg_mission_objective_kismet,
    // tg_mission_objective_proximity, tg_base_objective_ctf_bot,
    // tg_base_objective_koth, tg_mission_objective_escort,
    // tg_omega_volume, tg_modify_pawn_properties_volume, tg_device_volume,
    // tg_flag_capture_volume, tg_help_alert_volume, tg_mission_list_volume,
    // tg_actor_factory, tg_bot_factory, tg_bot_factory_spawnable,
    // tg_beacon_factory, tg_deployable_factory, tg_destructible_factory,
    // tg_hex_item_factory, tg_navigation_point, tg_bot_start,
    // tg_action_point, tg_alarm_point, tg_cover_point, tg_defense_point,
    // tg_hold_spot, tg_navigation_point_spawnable, tg_point_of_interest,
    // tg_teleporter, tg_bot_factory_location_list,
    // tg_bot_factory_patrol_path.

    Logger::Log("db", "v33: created 34 map_* tables for raw map dump data\n");
}
```

**Full column lists for each table** (use these verbatim — all 11 common columns first, then own scalars):

| Table | Own-scalar columns (after the 11 common) |
|---|---|
| `map_tg_team_player_start` | `m_n_task_force INTEGER, m_e_coalition INTEGER, m_n_priority INTEGER, n_prev_priority INTEGER, m_n_min_level INTEGER` |
| `map_tg_start_point` | `m_n_start_group INTEGER, m_n_return_map_type INTEGER, m_n_return_map_game_id INTEGER, m_f_start_rating REAL, m_f_current_rating REAL, m_f_reset_rating REAL, m_f_decrease_rate REAL, availability_quest_group_id INTEGER` |
| `map_tg_mission_objective` | `b_has_no_location INTEGER, r_b_use_pending_state INTEGER, s_b_change_coalition_when_captured INTEGER, m_b_capture_only_once INTEGER, r_b_has_been_captured_once INTEGER, s_b_end_overtime_on_defender_progress INTEGER, b_enabled INTEGER, r_b_is_locked INTEGER, r_b_is_active INTEGER, r_b_is_pending INTEGER, m_b_pause_on_capture INTEGER, c_b_local_pawn_is_attacker INTEGER, m_b_block_advance INTEGER, m_b_is_base_objective INTEGER, c_b_is_local_player_attacker INTEGER, c_b_local_player_attacker_cached INTEGER, m_b_in_matinee_update INTEGER, m_b_start_objective INTEGER, s_b_random_picked INTEGER, m_b_teleport_bots INTEGER, n_priority INTEGER, n_message_id INTEGER, n_default_owner_task_force INTEGER, n_objective_id INTEGER, hex_bonus_direction INTEGER, r_open_world_player_default_role INTEGER, r_e_default_coalition INTEGER, r_e_status INTEGER, r_e_owning_coalition INTEGER, s_open_world_bot_task_force INTEGER, m_f_proximity_radius INTEGER, m_f_proximity_height REAL, m_f_time_to_capture REAL, m_f_time_to_hold REAL, m_n_points_per_second INTEGER, m_n_type_id INTEGER, m_n_desc_msg_id INTEGER, m_n_icon_id INTEGER, m_n_min_agents INTEGER, m_n_cooldown_seconds INTEGER, m_n_points INTEGER, m_n_credits INTEGER, m_n_reward_xp INTEGER, m_n_loot_table_id INTEGER, m_f_curr_capture_time REAL, m_n_curr_owner_taskforce INTEGER, r_n_owner_task_force INTEGER, r_f_curr_capture_time REAL, m_n_capture_time_ext_secs INTEGER, m_n_capture_time_reset_secs INTEGER, s_f_attacker_captured_at REAL, r_f_last_completed_time REAL, c_f_percentage REAL, s_f_previous_play_rate REAL, m_f_start_time REAL, m_f_stop_time REAL, s_n_previous_priority INTEGER, s_n_current_spawn_order INTEGER` |
| `map_tg_mission_objective_bot` | `s_n_bot_id INTEGER, s_n_spawn_table_id INTEGER, s_b_auto_spawn INTEGER, b_patrol_loop INTEGER, f_balance REAL, n_global_alarm_id INTEGER` |
| `map_tg_mission_objective_kismet` | — (common cols only) |
| `map_tg_mission_objective_proximity` | `s_b_allow_ai_bot_interaction INTEGER, s_b_allow_remote_control_bot_interaction INTEGER, m_f_def_capture_rate REAL, m_f_capture_accel_rate REAL, m_f_capture_accel_rate_cap REAL, r_f_capture_rate REAL, m_f_overtime_accel_rate REAL` |
| `map_tg_base_objective_ctf_bot` | `m_f_flag_respawn_delay REAL, m_f_scoring_radius REAL, m_b_spawn_unaligned_bot INTEGER, m_b_capture_on_death INTEGER` |
| `map_tg_base_objective_koth` | `c_f_prev_client_capt_time REAL` |
| `map_tg_mission_objective_escort` | — (common cols only) |
| `map_tg_omega_volume` | `m_n_omega_alert_id INTEGER, m_n_subzone_name_msg_id INTEGER, m_n_subzone_secondary_name_msg_id INTEGER, m_n_omega_priority INTEGER, m_n_bilboard_key INTEGER, m_b_enable_equip INTEGER, m_b_enable_skills INTEGER, m_b_enable_crafting INTEGER, m_b_auto_kick_if_idle INTEGER, m_e_visual_cue INTEGER` |
| `map_tg_modify_pawn_properties_volume` | `m_b_disable_jump INTEGER, m_b_disable_block_actors INTEGER, m_b_disable_hanging INTEGER, m_b_disable_all_devices INTEGER, m_b_trigger_use_event INTEGER, m_b_one_way_movement INTEGER, m_v_onew_way_pitch INTEGER, m_v_onew_way_yaw INTEGER, m_v_onew_way_roll INTEGER, s_n_loot_table_id INTEGER` |
| `map_tg_device_volume` | `b_pain_causing INTEGER, s_b_device_active INTEGER, s_n_device_id INTEGER, s_n_team_number INTEGER, s_n_task_force INTEGER` |
| `map_tg_flag_capture_volume` | `r_n_task_force INTEGER, r_e_coalition INTEGER` |
| `map_tg_help_alert_volume` | `m_n_help_id INTEGER` |
| `map_tg_mission_list_volume` | `s_n_queue_table_id INTEGER, s_n_queue_table_msg_id INTEGER` |
| `map_tg_actor_factory` | `s_b_auto_spawn INTEGER, s_n_team_number INTEGER, s_n_task_force INTEGER, s_e_coalition INTEGER, s_e_selection_method INTEGER, s_n_selection_list_id INTEGER, s_n_selected_object_id INTEGER, m_n_selection_list_prop_id INTEGER, s_n_name_id INTEGER, s_n_cur_list_index INTEGER` |
| `map_tg_bot_factory` | `location_selection INTEGER, type_selection INTEGER, s_n_cur_location_index INTEGER, b_patrol_loop INTEGER, b_always_patrol INTEGER, b_spawn_on_alarm INTEGER, b_auto_spawn INTEGER, m_b_first_spawn INTEGER, b_bulk_spawn INTEGER, b_respawn INTEGER, m_b_ignore_collision_on_spawn INTEGER, n_global_alarm_id INTEGER, n_bot_count INTEGER, n_current_count INTEGER, n_active_count INTEGER, n_total_spawns INTEGER, n_spawn_table_id INTEGER, n_default_spawn_table_id INTEGER, f_spawn_delay REAL, m_n_last_group INTEGER, n_priority INTEGER, n_prev_priority INTEGER, f_last_kill_time REAL, f_balance REAL, f_respawn_delay REAL, m_n_spawn_order INTEGER` |
| `map_tg_bot_factory_spawnable` | — (common cols only) |
| `map_tg_beacon_factory` | `m_n_priority INTEGER, m_n_prev_priority INTEGER, m_b_beacon_exit INTEGER, m_b_is_fallback INTEGER` |
| `map_tg_deployable_factory` | `n_current_count INTEGER, s_f_last_spawn_time REAL, s_b_spawn_once INTEGER` |
| `map_tg_destructible_factory` | — (common cols only) |
| `map_tg_hex_item_factory` | `s_b_needs_spawn INTEGER` |
| `map_tg_navigation_point` | — (common cols only) |
| `map_tg_bot_start` | — (common cols only) |
| `map_tg_action_point` | `action_type INTEGER, n_objective_num INTEGER, n_task_force INTEGER, b_use_rotation INTEGER` |
| `map_tg_alarm_point` | — (common cols only) |
| `map_tg_cover_point` | `m_b_lean_left INTEGER, m_b_lean_right INTEGER, m_b_allow_popup INTEGER, m_b_allow_mantle INTEGER, m_v_lean_left_x REAL, m_v_lean_left_y REAL, m_v_lean_left_z REAL, m_v_lean_right_x REAL, m_v_lean_right_y REAL, m_v_lean_right_z REAL, m_v_pop_up_x REAL, m_v_pop_up_y REAL, m_v_pop_up_z REAL` |
| `map_tg_defense_point` | `b_first_script INTEGER, b_sniping INTEGER, b_dont_change_position INTEGER, b_avoid INTEGER, b_disabled INTEGER, b_not_in_vehicle INTEGER, priority INTEGER, num_checked REAL` |
| `map_tg_hold_spot` | — (common cols only) |
| `map_tg_navigation_point_spawnable` | — (common cols only) |
| `map_tg_point_of_interest` | `m_n_name_msg_id INTEGER, m_s_debug_name TEXT, m_quest_radius_uu REAL, m_b_show_when_quest_complete INTEGER` |
| `map_tg_teleporter` | `m_n_map_id INTEGER, m_n_preload INTEGER, m_b_set_task_force INTEGER, m_b_balance_task_force INTEGER, m_b_ignore_non_members INTEGER, m_b_use_player_start INTEGER, m_b_request_mission INTEGER, m_n_start_group INTEGER, m_n_task_force INTEGER` |
| `map_tg_bot_factory_location_list` | `array_index INTEGER NOT NULL, location_x REAL, location_y REAL, location_z REAL, nav_point_tag TEXT` *(note: skip the rotation columns from common — see Task 4 for full schema)* |
| `map_tg_bot_factory_patrol_path` | (same as `_location_list`) |

**Array tables** use a slightly different common schema. Their full CREATE TABLE for both:

```cpp
const char* kV33_loc_list =
    "CREATE TABLE map_tg_bot_factory_location_list ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  map_name TEXT NOT NULL,"
    "  map_object_id INTEGER NOT NULL,"
    "  array_index INTEGER NOT NULL,"
    "  location_x REAL, location_y REAL, location_z REAL,"
    "  nav_point_tag TEXT"
    ");"
    "CREATE INDEX idx_map_tg_bot_factory_location_list_map_object_id "
    "  ON map_tg_bot_factory_location_list(map_object_id);";
// kV33_patrol_path is identical except the table/index names.
```

- [ ] **Step 2: Change the version-bump literal at the bottom of `MigrateDatabase`**

The very last `sqlite3_exec(db, "UPDATE version_info SET version = 32", ...);` becomes `... SET version = 33"`.

- [ ] **Step 3: Commit**

```bash
git add src/Database/Database.cpp
git commit -m "db: migration v33 — create 34 map_* tables for raw map dump data"
```

---

## Task 2: `WriterCommon.{hpp,cpp}` — shared helpers

**Files:**
- Create: `src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp`
- Create: `src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.cpp`
- Modify: `Makefile` (add `WriterCommon.cpp`)

- [ ] **Step 1: Write `WriterCommon.hpp`**

```cpp
#pragma once
#include "src/pch.hpp"
#include <string>

namespace MapDumpWriters {

// Render an FName as its name-table string, or "(none)/(oob)/(null)" sentinels.
// Returned pointer is only valid until the next FName::Names() shuffle, so
// caller must SQLITE_TRANSIENT-copy when binding to SQL.
const char* FNameStr(const FName& fn);

// Convert an FString (wide chars) to UTF-8 std::string. Returns "" for empty
// or "(non-utf8)" if the conversion fails.
std::string FStringToUtf8(const FString& s);

// Strip "Class <package>." or "<package>." prefix from a UE3 GetFullName()
// output, returning just the bare class name (e.g. "TgMissionObjective_Bot").
std::string ExtractClassName(const char* fullName);

// Bind the 11 common columns of any map_<class> table to a prepared INSERT
// statement. Binding starts at index 1; first own-scalar bind index is 12.
//
// Layout:
//   1  map_name        TEXT
//   2  map_object_id   INTEGER
//   3  class_name      TEXT
//   4  tag             TEXT (from actor->Tag, FName -> str)
//   5  "group"         TEXT (from actor->Group, FName -> str)
//   6  location_x      REAL
//   7  location_y      REAL
//   8  location_z      REAL
//   9  rotation_pitch  INTEGER
//   10 rotation_yaw    INTEGER
//   11 rotation_roll   INTEGER
void BindCommonCols(sqlite3_stmt* stmt, AActor* actor, int mapObjectId,
                    const std::string& mapName, const std::string& className);

}  // namespace MapDumpWriters
```

- [ ] **Step 2: Write `WriterCommon.cpp`**

```cpp
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include <cstdlib>
#include <cstring>

namespace MapDumpWriters {

const char* FNameStr(const FName& fn) {
    if (fn.Index <= 0) return "(none)";
    TArray<FNameEntry*>* names = FName::Names();
    if (!names || fn.Index >= names->Count) return "(oob)";
    FNameEntry* entry = names->Data[fn.Index];
    return (entry && entry->Name) ? entry->Name : "(null)";
}

std::string FStringToUtf8(const FString& s) {
    if (!s.Data || s.Count <= 0) return "";
    std::size_t needed = wcstombs(nullptr, s.Data, 0);
    if (needed == static_cast<std::size_t>(-1)) return "(non-utf8)";
    std::string out(needed, '\0');
    wcstombs(out.data(), s.Data, needed + 1);
    return out;
}

std::string ExtractClassName(const char* fullName) {
    if (!fullName) return "";
    // GetFullName() returns e.g. "Class TgGame.TgMissionObjective_Bot".
    // The bare class name is everything after the last '.'.
    const char* dot = std::strrchr(fullName, '.');
    return dot ? std::string(dot + 1) : std::string(fullName);
}

void BindCommonCols(sqlite3_stmt* stmt, AActor* actor, int mapObjectId,
                    const std::string& mapName, const std::string& className) {
    sqlite3_bind_text  (stmt, 1, mapName.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt, 2, mapObjectId);
    sqlite3_bind_text  (stmt, 3, className.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 4, FNameStr(actor->Tag),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 5, FNameStr(actor->Group), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 6, actor->Location.X);
    sqlite3_bind_double(stmt, 7, actor->Location.Y);
    sqlite3_bind_double(stmt, 8, actor->Location.Z);
    sqlite3_bind_int   (stmt, 9,  actor->Rotation.Pitch);
    sqlite3_bind_int   (stmt, 10, actor->Rotation.Yaw);
    sqlite3_bind_int   (stmt, 11, actor->Rotation.Roll);
}

}  // namespace MapDumpWriters
```

- [ ] **Step 3: Add to Makefile**

Find the existing `$(SRC_DIR)/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \` line and add immediately after:

```make
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/WriterCommon.cpp \
```

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp \
        src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.cpp \
        Makefile
git commit -m "mapdump: add Writers/WriterCommon helpers (BindCommonCols, FNameStr, ExtractClassName)"
```

---

## Task 3: `MapDataDumper.cpp` refactor — entry + dispatch shell, drop file logging

**Files:**
- Modify: `src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp` (full rewrite)

`MapDataDumper.hpp` is unchanged. The file goes from 334 lines to ~120 lines.

- [ ] **Step 1: Replace `MapDataDumper.cpp` with the new shell**

```cpp
#include "src/GameServer/Engine/MapDataDumper/MapDataDumper.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Config/Config.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

using namespace MapDumpWriters;

namespace {

// All map_* tables that need DELETE-then-INSERT for idempotent re-dumps.
// Order doesn't matter — each DELETE is independent.
const char* const kMapTables[] = {
    "map_tg_team_player_start",
    "map_tg_start_point",
    "map_tg_mission_objective",
    "map_tg_mission_objective_bot",
    "map_tg_mission_objective_kismet",
    "map_tg_mission_objective_proximity",
    "map_tg_base_objective_ctf_bot",
    "map_tg_base_objective_koth",
    "map_tg_mission_objective_escort",
    "map_tg_omega_volume",
    "map_tg_modify_pawn_properties_volume",
    "map_tg_device_volume",
    "map_tg_flag_capture_volume",
    "map_tg_help_alert_volume",
    "map_tg_mission_list_volume",
    "map_tg_actor_factory",
    "map_tg_bot_factory",
    "map_tg_bot_factory_spawnable",
    "map_tg_beacon_factory",
    "map_tg_deployable_factory",
    "map_tg_destructible_factory",
    "map_tg_hex_item_factory",
    "map_tg_navigation_point",
    "map_tg_bot_start",
    "map_tg_action_point",
    "map_tg_alarm_point",
    "map_tg_cover_point",
    "map_tg_defense_point",
    "map_tg_hold_spot",
    "map_tg_navigation_point_spawnable",
    "map_tg_point_of_interest",
    "map_tg_teleporter",
    "map_tg_bot_factory_location_list",
    "map_tg_bot_factory_patrol_path",
};
constexpr int kMapTableCount = sizeof(kMapTables) / sizeof(kMapTables[0]);

void DeletePriorRows(sqlite3* db, const std::string& mapName) {
    for (int i = 0; i < kMapTableCount; i++) {
        std::string sql = "DELETE FROM ";
        sql += kMapTables[i];
        sql += " WHERE map_name = ?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("mapdump", "DELETE prepare failed for %s: %s\n",
                kMapTables[i], sqlite3_errmsg(db));
            continue;
        }
        sqlite3_bind_text(stmt, 1, mapName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

void WriteByClass(sqlite3* db, AActor* actor, const std::string& mapName) {
    if (!actor || !actor->Class) return;
    std::string cls = ExtractClassName(actor->Class->GetFullName());
    if (cls.empty()) return;

    // Dispatch branches are appended by tasks 4..10. Until those tasks run,
    // this function silently ignores every actor.

    // === BEGIN DISPATCH BRANCHES (appended by writer tasks) ===
    // === END DISPATCH BRANCHES ===
}

}  // namespace

void MapDataDumper::DumpAll() {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("mapdump", "DB connection unavailable — aborting dump\n");
        return;
    }

    std::string mapName = Config::GetMapNameChar();
    Logger::Log("mapdump", "=== map dump BEGIN map=%s ===\n", mapName.c_str());

    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    DeletePriorRows(db, mapName);

    TArray<UObject*>* objs = UObject::GObjObjects();
    if (!objs) {
        Logger::Log("mapdump", "GObjObjects() is null, aborting\n");
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return;
    }

    int scanned = 0;
    for (int i = 0; i < objs->Count; i++) {
        UObject* obj = objs->Data[i];
        if (!obj || !obj->Class) continue;

        // Skip CDOs — they have synthetic "Default__X" names and zero locs.
        const char* fullName = obj->GetFullName();
        if (fullName && std::strstr(fullName, "Default__")) continue;

        // We only care about actors (everything in scope is AActor-derived).
        // Cheapest filter: try the cast and let WriteByClass's class-name
        // check decide. UObject is the base of AActor in this SDK; the
        // GObjObjects table holds plenty of non-actors so the dispatch's
        // class-name match is what gates the actual writes.
        AActor* actor = static_cast<AActor*>(obj);
        WriteByClass(db, actor, mapName);
        scanned++;
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    Logger::Log("mapdump", "=== map dump END map=%s (scanned %d) ===\n",
        mapName.c_str(), scanned);
}
```

Notes:
- All `Logger::Log` calls for per-row property dumping have been removed. Only lifecycle markers (`BEGIN`/`END`) and error logs remain. No file output of dumped data.
- `WriteByClass` is intentionally empty in this task. Each subsequent writer task inserts its branches between the `BEGIN DISPATCH BRANCHES` / `END DISPATCH BRANCHES` markers.
- The cast `static_cast<AActor*>(obj)` is safe-by-precondition: only classes that match a dispatch branch get any work done; the static_cast itself is just a pointer reinterpret and won't fault on non-actor UObjects, because WriteByClass returns early when `cls` doesn't match any branch.

- [ ] **Step 2: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp
git commit -m "mapdump: refactor entry to DB-only with empty dispatch shell"
```

---

## Task 4: Array writers — `TgBotFactoryLocationList` + `TgBotFactoryPatrolPath`

These are called inline from `WriteTgBotFactory` (Task 5). Build them first so the factory writer has them available.

**Files:**
- Create: `src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryLocationList.{hpp,cpp}`
- Create: `src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryPatrolPath.{hpp,cpp}`
- Modify: `Makefile`

- [ ] **Step 1: Write `TgBotFactoryLocationList.hpp`**

```cpp
#pragma once
#include "src/pch.hpp"
#include <string>

namespace MapDumpWriters {
    void WriteTgBotFactoryLocationList(sqlite3* db, ATgBotFactory* factory,
                                        const std::string& mapName,
                                        int mapObjectId);
}
```

- [ ] **Step 2: Write `TgBotFactoryLocationList.cpp`**

```cpp
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryLocationList.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {
void WriteTgBotFactoryLocationList(sqlite3* db, ATgBotFactory* factory,
                                    const std::string& mapName,
                                    int mapObjectId) {
    if (!factory || factory->LocationList.Data == nullptr) return;

    const char* kSql =
        "INSERT INTO map_tg_bot_factory_location_list ("
        "map_name, map_object_id, array_index,"
        " location_x, location_y, location_z, nav_point_tag"
        ") VALUES (?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Log("mapdump", "prepare failed for location_list: %s\n", sqlite3_errmsg(db));
        return;
    }

    for (int i = 0; i < factory->LocationList.Num(); i++) {
        ANavigationPoint* np = factory->LocationList.Data[i];
        if (!np) continue;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_text  (stmt, 1, mapName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int   (stmt, 2, mapObjectId);
        sqlite3_bind_int   (stmt, 3, i);
        sqlite3_bind_double(stmt, 4, np->Location.X);
        sqlite3_bind_double(stmt, 5, np->Location.Y);
        sqlite3_bind_double(stmt, 6, np->Location.Z);
        sqlite3_bind_text  (stmt, 7, FNameStr(np->Tag), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Logger::Log("mapdump", "location_list insert failed factory=%d idx=%d: %s\n",
                mapObjectId, i, sqlite3_errmsg(db));
        }
    }
    sqlite3_finalize(stmt);
}
}  // namespace
```

- [ ] **Step 3: Write `TgBotFactoryPatrolPath.{hpp,cpp}` — identical pattern**

`TgBotFactoryPatrolPath.hpp`:

```cpp
#pragma once
#include "src/pch.hpp"
#include <string>

namespace MapDumpWriters {
    void WriteTgBotFactoryPatrolPath(sqlite3* db, ATgBotFactory* factory,
                                      const std::string& mapName,
                                      int mapObjectId);
}
```

`TgBotFactoryPatrolPath.cpp` is the same code as `LocationList.cpp` except:
- function name: `WriteTgBotFactoryPatrolPath`
- table name: `map_tg_bot_factory_patrol_path`
- iterates `factory->PatrolPath.Data` / `factory->PatrolPath.Num()` instead of `LocationList`

- [ ] **Step 4: Add both to Makefile**

After the `WriterCommon.cpp` line:

```make
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryLocationList.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryPatrolPath.cpp \
```

- [ ] **Step 5: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryLocationList.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryPatrolPath.{hpp,cpp} \
        Makefile
git commit -m "mapdump: add TgBotFactory array writers (LocationList, PatrolPath)"
```

---

## Task 5: TgActorFactory hierarchy writers (7 classes)

**Files:** Create writer pairs for each, edit `MapDataDumper.cpp` dispatch + `Makefile`.

**Classes to implement, with own-scalar binds:**

For each, follow the **Common writer template** at the top of this plan. Below are the per-class specifics (SDK type, snake table name, own-scalar bind list). All binds start at index 12 unless noted.

### `TgActorFactory.{hpp,cpp}`
- SDK type: `ATgActorFactory`
- Table: `map_tg_actor_factory`
- Own scalars (in order):
  ```cpp
  sqlite3_bind_int(stmt, 12, a->s_bAutoSpawn ? 1 : 0);
  sqlite3_bind_int(stmt, 13, a->s_nTeamNumber);
  sqlite3_bind_int(stmt, 14, (int)a->s_nTaskForce);
  sqlite3_bind_int(stmt, 15, (int)a->s_eCoalition);
  sqlite3_bind_int(stmt, 16, (int)a->s_eSelectionMethod);
  sqlite3_bind_int(stmt, 17, a->s_nSelectionListId);
  sqlite3_bind_int(stmt, 18, a->s_nSelectedObjectId);
  sqlite3_bind_int(stmt, 19, a->m_nSelectionListPropId);
  sqlite3_bind_int(stmt, 20, a->s_nNameId);
  sqlite3_bind_int(stmt, 21, a->s_nCurListIndex);
  ```
- SQL columns after the 11 common: `s_b_auto_spawn, s_n_team_number, s_n_task_force, s_e_coalition, s_e_selection_method, s_n_selection_list_id, s_n_selected_object_id, m_n_selection_list_prop_id, s_n_name_id, s_n_cur_list_index`

### `TgBotFactory.{hpp,cpp}`
- SDK type: `ATgBotFactory`
- Table: `map_tg_bot_factory`
- **Includes the two array-writer calls** at the end (before `sqlite3_finalize`):
  ```cpp
  WriteTgBotFactoryLocationList(db, a, mapName, mapObjectId);
  WriteTgBotFactoryPatrolPath  (db, a, mapName, mapObjectId);
  ```
  Add the appropriate `#include "...TgBotFactoryLocationList.hpp"` / `...PatrolPath.hpp` at the top.
- Own scalars (in order, matching the SQL column list):
  ```cpp
  sqlite3_bind_int   (stmt, 12, (int)a->LocationSelection);
  sqlite3_bind_int   (stmt, 13, (int)a->TypeSelection);
  sqlite3_bind_int   (stmt, 14, a->s_nCurLocationIndex);
  sqlite3_bind_int   (stmt, 15, a->bPatrolLoop ? 1 : 0);
  sqlite3_bind_int   (stmt, 16, a->bAlwaysPatrol ? 1 : 0);
  sqlite3_bind_int   (stmt, 17, a->bSpawnOnAlarm ? 1 : 0);
  sqlite3_bind_int   (stmt, 18, a->bAutoSpawn ? 1 : 0);
  sqlite3_bind_int   (stmt, 19, a->m_bFirstSpawn ? 1 : 0);
  sqlite3_bind_int   (stmt, 20, a->bBulkSpawn ? 1 : 0);
  sqlite3_bind_int   (stmt, 21, a->bRespawn ? 1 : 0);
  sqlite3_bind_int   (stmt, 22, a->m_bIgnoreCollisionOnSpawn ? 1 : 0);
  sqlite3_bind_int   (stmt, 23, a->nGlobalAlarmId);
  sqlite3_bind_int   (stmt, 24, a->nBotCount);
  sqlite3_bind_int   (stmt, 25, a->nCurrentCount);
  sqlite3_bind_int   (stmt, 26, a->nActiveCount);
  sqlite3_bind_int   (stmt, 27, a->nTotalSpawns);
  sqlite3_bind_int   (stmt, 28, a->nSpawnTableId);
  sqlite3_bind_int   (stmt, 29, a->nDefaultSpawnTableId);
  sqlite3_bind_double(stmt, 30, a->fSpawnDelay);
  sqlite3_bind_int   (stmt, 31, a->m_nLastGroup);
  sqlite3_bind_int   (stmt, 32, a->nPriority);
  sqlite3_bind_int   (stmt, 33, a->nPrevPriority);
  sqlite3_bind_double(stmt, 34, a->fLastKillTime);
  sqlite3_bind_double(stmt, 35, a->fBalance);
  sqlite3_bind_double(stmt, 36, a->fRespawnDelay);
  sqlite3_bind_int   (stmt, 37, a->m_nSpawnOrder);
  ```

### `TgBotFactorySpawnable.{hpp,cpp}`
- SDK type: `ATgBotFactorySpawnable`
- Table: `map_tg_bot_factory_spawnable`
- No own scalars — body is just `BindCommonCols` + `step` + `finalize`.

### `TgBeaconFactory.{hpp,cpp}`
- SDK type: `ATgBeaconFactory`
- Table: `map_tg_beacon_factory`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->m_nPriority);
  sqlite3_bind_int(stmt, 13, a->m_nPrevPriority);
  sqlite3_bind_int(stmt, 14, a->m_bBeaconExit ? 1 : 0);
  sqlite3_bind_int(stmt, 15, a->m_bIsFallback ? 1 : 0);
  ```

### `TgDeployableFactory.{hpp,cpp}`
- SDK type: `ATgDeployableFactory`
- Table: `map_tg_deployable_factory`
- Own scalars:
  ```cpp
  sqlite3_bind_int   (stmt, 12, a->nCurrentCount);
  sqlite3_bind_double(stmt, 13, a->s_fLastSpawnTime);
  sqlite3_bind_int   (stmt, 14, a->s_bSpawnOnce ? 1 : 0);
  ```

### `TgDestructibleFactory.{hpp,cpp}`
- SDK type: `ATgDestructibleFactory`
- Table: `map_tg_destructible_factory`
- No own scalars.

### `TgHexItemFactory.{hpp,cpp}`
- SDK type: `ATgHexItemFactory`
- Table: `map_tg_hex_item_factory`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->s_bNeedsSpawn ? 1 : 0);
  ```

- [ ] **Step 1: Create all 7 writer pairs** per the template + per-class spec above

- [ ] **Step 2: Add dispatch branches in `MapDataDumper.cpp`**

Append between the `BEGIN DISPATCH BRANCHES` / `END DISPATCH BRANCHES` markers. Be sure to `#include` each writer hpp at the top of `MapDataDumper.cpp`.

```cpp
if (cls == "TgBotFactory") {
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory(db, actor, mapName, cls, id);
    WriteTgBotFactory  (db, actor, mapName, cls, id);
}
else if (cls == "TgBotFactorySpawnable") {
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory       (db, actor, mapName, cls, id);
    WriteTgBotFactory         (db, actor, mapName, cls, id);
    WriteTgBotFactorySpawnable(db, actor, mapName, cls, id);
}
else if (cls == "TgBeaconFactory") {
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory (db, actor, mapName, cls, id);
    WriteTgBeaconFactory(db, actor, mapName, cls, id);
}
else if (cls == "TgDeployableFactory") {
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory    (db, actor, mapName, cls, id);
    WriteTgDeployableFactory(db, actor, mapName, cls, id);
}
else if (cls == "TgDestructibleFactory") {
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory      (db, actor, mapName, cls, id);
    WriteTgDestructibleFactory(db, actor, mapName, cls, id);
}
else if (cls == "TgHexItemFactory") {
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory(db, actor, mapName, cls, id);
    WriteTgHexItemFactory(db, actor, mapName, cls, id);
}
else if (cls == "TgActorFactory") {  // bare base — unlikely in maps but valid
    int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
    WriteTgActorFactory(db, actor, mapName, cls, id);
}
```

- [ ] **Step 3: Add all 7 cpp paths to Makefile**

```make
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgActorFactory.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBotFactory.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBotFactorySpawnable.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBeaconFactory.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgDeployableFactory.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgDestructibleFactory.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgHexItemFactory.cpp \
```

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/Tg{ActorFactory,BotFactory,BotFactorySpawnable,BeaconFactory,DeployableFactory,DestructibleFactory,HexItemFactory}.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \
        Makefile
git commit -m "mapdump: add TgActorFactory hierarchy writers (7 classes) + dispatch"
```

---

## Task 6: TgMissionObjective hierarchy writers (7 classes)

**Classes:** TgMissionObjective, TgMissionObjective_Bot, TgMissionObjective_Kismet, TgMissionObjective_Proximity, TgBaseObjective_CTFBot, TgBaseObjective_KOTH, TgMissionObjective_Escort.

### `TgMissionObjective.{hpp,cpp}`
- SDK type: `ATgMissionObjective`
- Table: `map_tg_mission_objective`
- Own scalars (58 binds, indices 12..69, in the order listed in the migration column table):

```cpp
sqlite3_bind_int(stmt, 12, a->bHasNoLocation ? 1 : 0);
sqlite3_bind_int(stmt, 13, a->r_bUsePendingState ? 1 : 0);
sqlite3_bind_int(stmt, 14, a->s_bChangeCoalitionWhenCaptured ? 1 : 0);
sqlite3_bind_int(stmt, 15, a->m_bCaptureOnlyOnce ? 1 : 0);
sqlite3_bind_int(stmt, 16, a->r_bHasBeenCapturedOnce ? 1 : 0);
sqlite3_bind_int(stmt, 17, a->s_bEndOvertimeOnDefenderProgress ? 1 : 0);
sqlite3_bind_int(stmt, 18, a->bEnabled ? 1 : 0);
sqlite3_bind_int(stmt, 19, a->r_bIsLocked ? 1 : 0);
sqlite3_bind_int(stmt, 20, a->r_bIsActive ? 1 : 0);
sqlite3_bind_int(stmt, 21, a->r_bIsPending ? 1 : 0);
sqlite3_bind_int(stmt, 22, a->m_bPauseOnCapture ? 1 : 0);
sqlite3_bind_int(stmt, 23, a->c_bLocalPawnIsAttacker ? 1 : 0);
sqlite3_bind_int(stmt, 24, a->m_bBlockAdvance ? 1 : 0);
sqlite3_bind_int(stmt, 25, a->m_bIsBaseObjective ? 1 : 0);
sqlite3_bind_int(stmt, 26, a->c_bIsLocalPlayerAttacker ? 1 : 0);
sqlite3_bind_int(stmt, 27, a->c_bLocalPlayerAttackerCached ? 1 : 0);
sqlite3_bind_int(stmt, 28, a->m_bInMatineeUpdate ? 1 : 0);
sqlite3_bind_int(stmt, 29, a->m_bStartObjective ? 1 : 0);
sqlite3_bind_int(stmt, 30, a->s_bRandomPicked ? 1 : 0);
sqlite3_bind_int(stmt, 31, a->m_bTeleportBots ? 1 : 0);
sqlite3_bind_int(stmt, 32, a->nPriority);
sqlite3_bind_int(stmt, 33, a->nMessageId);
sqlite3_bind_int(stmt, 34, a->nDefaultOwnerTaskForce);
sqlite3_bind_int(stmt, 35, a->nObjectiveId);
sqlite3_bind_int(stmt, 36, (int)a->HexBonusDirection);
sqlite3_bind_int(stmt, 37, (int)a->r_OpenWorldPlayerDefaultRole);
sqlite3_bind_int(stmt, 38, (int)a->r_eDefaultCoalition);
sqlite3_bind_int(stmt, 39, (int)a->r_eStatus);
sqlite3_bind_int(stmt, 40, (int)a->r_eOwningCoalition);
sqlite3_bind_int(stmt, 41, a->s_OpenWorldBotTaskForce);
sqlite3_bind_int   (stmt, 42, a->m_fProximityRadius);
sqlite3_bind_double(stmt, 43, a->m_fProximityHeight);
sqlite3_bind_double(stmt, 44, a->m_fTimeToCapture);
sqlite3_bind_double(stmt, 45, a->m_fTimeToHold);
sqlite3_bind_int   (stmt, 46, a->m_nPointsPerSecond);
sqlite3_bind_int   (stmt, 47, a->m_nTypeId);
sqlite3_bind_int   (stmt, 48, a->m_nDescMsgId);
sqlite3_bind_int   (stmt, 49, a->m_nIconId);
sqlite3_bind_int   (stmt, 50, a->m_nMinAgents);
sqlite3_bind_int   (stmt, 51, a->m_nCooldownSeconds);
sqlite3_bind_int   (stmt, 52, a->m_nPoints);
sqlite3_bind_int   (stmt, 53, a->m_nCredits);
sqlite3_bind_int   (stmt, 54, a->m_nRewardXP);
sqlite3_bind_int   (stmt, 55, a->m_nLootTableId);
sqlite3_bind_double(stmt, 56, a->m_fCurrCaptureTime);
sqlite3_bind_int   (stmt, 57, a->m_nCurrOwnerTaskforce);
sqlite3_bind_int   (stmt, 58, a->r_nOwnerTaskForce);
sqlite3_bind_double(stmt, 59, a->r_fCurrCaptureTime);
sqlite3_bind_int   (stmt, 60, a->m_nCaptureTimeExtSecs);
sqlite3_bind_int   (stmt, 61, a->m_nCaptureTimeResetSecs);
sqlite3_bind_double(stmt, 62, a->s_fAttackerCapturedAt);
sqlite3_bind_double(stmt, 63, a->r_fLastCompletedTime);
sqlite3_bind_double(stmt, 64, a->c_fPercentage);
sqlite3_bind_double(stmt, 65, a->s_fPreviousPlayRate);
sqlite3_bind_double(stmt, 66, a->m_fStartTime);
sqlite3_bind_double(stmt, 67, a->m_fStopTime);
sqlite3_bind_int   (stmt, 68, a->s_nPreviousPriority);
sqlite3_bind_int   (stmt, 69, a->s_nCurrentSpawnOrder);
```

> Note: `m_fProximityRadius` is declared `int` in the UC source (`var int m_fProximityRadius`) despite the `m_f` prefix — bind as int.

### `TgMissionObjective_Bot.{hpp,cpp}`
- SDK type: `ATgMissionObjective_Bot`
- Table: `map_tg_mission_objective_bot`
- Own scalars:
  ```cpp
  sqlite3_bind_int   (stmt, 12, a->s_nBotId);
  sqlite3_bind_int   (stmt, 13, a->s_nSpawnTableId);
  sqlite3_bind_int   (stmt, 14, a->s_bAutoSpawn ? 1 : 0);
  sqlite3_bind_int   (stmt, 15, a->bPatrolLoop ? 1 : 0);
  sqlite3_bind_double(stmt, 16, a->fBalance);
  sqlite3_bind_int   (stmt, 17, a->nGlobalAlarmId);
  ```

### `TgMissionObjective_Kismet.{hpp,cpp}`
- SDK type: `ATgMissionObjective_Kismet`
- Table: `map_tg_mission_objective_kismet`
- No own scalars.

### `TgMissionObjective_Proximity.{hpp,cpp}`
- SDK type: `ATgMissionObjective_Proximity`
- Table: `map_tg_mission_objective_proximity`
- Own scalars:
  ```cpp
  sqlite3_bind_int   (stmt, 12, a->s_bAllowAIBotInteraction ? 1 : 0);
  sqlite3_bind_int   (stmt, 13, a->s_bAllowRemoteControlBotInteraction ? 1 : 0);
  sqlite3_bind_double(stmt, 14, a->m_fDefCaptureRate);
  sqlite3_bind_double(stmt, 15, a->m_fCaptureAccelRate);
  sqlite3_bind_double(stmt, 16, a->m_fCaptureAccelRateCap);
  sqlite3_bind_double(stmt, 17, a->r_fCaptureRate);
  sqlite3_bind_double(stmt, 18, a->m_fOvertimeAccelRate);
  ```

### `TgBaseObjective_CTFBot.{hpp,cpp}`
- SDK type: `ATgBaseObjective_CTFBot`
- Table: `map_tg_base_objective_ctf_bot`
- Own scalars:
  ```cpp
  sqlite3_bind_double(stmt, 12, a->m_fFlagRespawnDelay);
  sqlite3_bind_double(stmt, 13, a->m_fScoringRadius);
  sqlite3_bind_int   (stmt, 14, a->m_bSpawnUnalignedBot ? 1 : 0);
  sqlite3_bind_int   (stmt, 15, a->m_bCaptureOnDeath ? 1 : 0);
  ```

### `TgBaseObjective_KOTH.{hpp,cpp}`
- SDK type: `ATgBaseObjective_KOTH`
- Table: `map_tg_base_objective_koth`
- Own scalars:
  ```cpp
  sqlite3_bind_double(stmt, 12, a->c_fPrevClientCaptTime);
  ```

### `TgMissionObjective_Escort.{hpp,cpp}`
- SDK type: `ATgMissionObjective_Escort`
- Table: `map_tg_mission_objective_escort`
- No own scalars.

- [ ] **Step 1: Create all 7 writer pairs**

- [ ] **Step 2: Add dispatch branches**

```cpp
else if (cls == "TgMissionObjective_Bot") {
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective    (db, actor, mapName, cls, id);
    WriteTgMissionObjective_Bot(db, actor, mapName, cls, id);
}
else if (cls == "TgBaseObjective_CTFBot") {
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective    (db, actor, mapName, cls, id);
    WriteTgMissionObjective_Bot(db, actor, mapName, cls, id);
    WriteTgBaseObjective_CTFBot(db, actor, mapName, cls, id);
}
else if (cls == "TgMissionObjective_Kismet") {
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective       (db, actor, mapName, cls, id);
    WriteTgMissionObjective_Kismet(db, actor, mapName, cls, id);
}
else if (cls == "TgMissionObjective_Proximity") {
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective          (db, actor, mapName, cls, id);
    WriteTgMissionObjective_Proximity(db, actor, mapName, cls, id);
}
else if (cls == "TgBaseObjective_KOTH") {
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective          (db, actor, mapName, cls, id);
    WriteTgMissionObjective_Proximity(db, actor, mapName, cls, id);
    WriteTgBaseObjective_KOTH        (db, actor, mapName, cls, id);
}
else if (cls == "TgMissionObjective_Escort") {
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective          (db, actor, mapName, cls, id);
    WriteTgMissionObjective_Proximity(db, actor, mapName, cls, id);
    WriteTgMissionObjective_Escort   (db, actor, mapName, cls, id);
}
else if (cls == "TgMissionObjective") {  // bare base
    int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
    WriteTgMissionObjective(db, actor, mapName, cls, id);
}
```

- [ ] **Step 3: Add 7 cpp paths to Makefile**

```make
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Bot.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Kismet.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Proximity.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBaseObjective_CTFBot.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgBaseObjective_KOTH.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Escort.cpp \
```

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/Tg{MissionObjective,MissionObjective_Bot,MissionObjective_Kismet,MissionObjective_Proximity,BaseObjective_CTFBot,BaseObjective_KOTH,MissionObjective_Escort}.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \
        Makefile
git commit -m "mapdump: add TgMissionObjective hierarchy writers (7 classes) + dispatch"
```

---

## Task 7: TgStartPoint + TgTeamPlayerStart writers (2 classes)

### `TgStartPoint.{hpp,cpp}`
- SDK type: `ATgStartPoint`
- Table: `map_tg_start_point`
- Own scalars:
  ```cpp
  sqlite3_bind_int   (stmt, 12, a->m_nStartGroup);
  sqlite3_bind_int   (stmt, 13, a->m_nReturnMapType);
  sqlite3_bind_int   (stmt, 14, a->m_nReturnMapGameId);
  sqlite3_bind_double(stmt, 15, a->m_fStartRating);
  sqlite3_bind_double(stmt, 16, a->m_fCurrentRating);
  sqlite3_bind_double(stmt, 17, a->m_fResetRating);
  sqlite3_bind_double(stmt, 18, a->m_fDecreaseRate);
  sqlite3_bind_int   (stmt, 19, a->AvailabilityQuestGroupId);
  ```

### `TgTeamPlayerStart.{hpp,cpp}`
- SDK type: `ATgTeamPlayerStart`
- Table: `map_tg_team_player_start`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, (int)a->m_nTaskForce);
  sqlite3_bind_int(stmt, 13, (int)a->m_eCoalition);
  sqlite3_bind_int(stmt, 14, a->m_nPriority);
  sqlite3_bind_int(stmt, 15, a->nPrevPriority);
  sqlite3_bind_int(stmt, 16, a->m_nMinLevel);
  ```

- [ ] **Step 1: Create 2 writer pairs**

- [ ] **Step 2: Add dispatch branches**

```cpp
else if (cls == "TgTeamPlayerStart") {
    int id = static_cast<ATgStartPoint*>(actor)->m_nMapObjectId;
    WriteTgStartPoint     (db, actor, mapName, cls, id);
    WriteTgTeamPlayerStart(db, actor, mapName, cls, id);
}
else if (cls == "TgStartPoint") {
    int id = static_cast<ATgStartPoint*>(actor)->m_nMapObjectId;
    WriteTgStartPoint(db, actor, mapName, cls, id);
}
```

- [ ] **Step 3: Add 2 cpp paths to Makefile**

```make
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgStartPoint.cpp \
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgTeamPlayerStart.cpp \
```

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/Tg{StartPoint,TeamPlayerStart}.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \
        Makefile
git commit -m "mapdump: add TgStartPoint + TgTeamPlayerStart writers + dispatch"
```

---

## Task 8: Volume-based writers (6 classes)

**Classes:** TgOmegaVolume, TgModifyPawnPropertiesVolume, TgDeviceVolume, TgFlagCaptureVolume, TgHelpAlertVolume, TgMissionListVolume.

These are not in a shared TgGame parent chain — each declares `m_nMapObjectId` itself, so the dispatcher casts directly to the leaf class.

### `TgOmegaVolume.{hpp,cpp}`
- SDK type: `ATgOmegaVolume`
- Table: `map_tg_omega_volume`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->m_nOmegaAlertId);
  sqlite3_bind_int(stmt, 13, a->m_nSubzoneNameMsgId);
  sqlite3_bind_int(stmt, 14, a->m_nSubzoneSecondaryNameMsgId);
  sqlite3_bind_int(stmt, 15, a->m_nOmegaPriority);
  sqlite3_bind_int(stmt, 16, a->m_nBilboardKey);
  sqlite3_bind_int(stmt, 17, a->m_bEnableEquip ? 1 : 0);
  sqlite3_bind_int(stmt, 18, a->m_bEnableSkills ? 1 : 0);
  sqlite3_bind_int(stmt, 19, a->m_bEnableCrafting ? 1 : 0);
  sqlite3_bind_int(stmt, 20, a->m_bAutoKickIfIdle ? 1 : 0);
  sqlite3_bind_int(stmt, 21, (int)a->m_eVisualCue);
  ```

### `TgModifyPawnPropertiesVolume.{hpp,cpp}`
- SDK type: `ATgModifyPawnPropertiesVolume`
- Table: `map_tg_modify_pawn_properties_volume`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->m_bDisableJump ? 1 : 0);
  sqlite3_bind_int(stmt, 13, a->m_bDisableBlockActors ? 1 : 0);
  sqlite3_bind_int(stmt, 14, a->m_bDisableHanging ? 1 : 0);
  sqlite3_bind_int(stmt, 15, a->m_bDisableAllDevices ? 1 : 0);
  sqlite3_bind_int(stmt, 16, a->m_bTriggerUseEvent ? 1 : 0);
  sqlite3_bind_int(stmt, 17, a->m_bOneWayMovement ? 1 : 0);
  sqlite3_bind_int(stmt, 18, a->m_vOnewWay.Pitch);
  sqlite3_bind_int(stmt, 19, a->m_vOnewWay.Yaw);
  sqlite3_bind_int(stmt, 20, a->m_vOnewWay.Roll);
  sqlite3_bind_int(stmt, 21, a->s_nLootTableId);
  ```

### `TgDeviceVolume.{hpp,cpp}`
- SDK type: `ATgDeviceVolume`
- Table: `map_tg_device_volume`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->bPainCausing ? 1 : 0);
  sqlite3_bind_int(stmt, 13, a->s_bDeviceActive ? 1 : 0);
  sqlite3_bind_int(stmt, 14, a->s_nDeviceId);
  sqlite3_bind_int(stmt, 15, a->s_nTeamNumber);
  sqlite3_bind_int(stmt, 16, (int)a->s_nTaskForce);
  ```

### `TgFlagCaptureVolume.{hpp,cpp}`
- SDK type: `ATgFlagCaptureVolume`
- Table: `map_tg_flag_capture_volume`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, (int)a->r_nTaskForce);
  sqlite3_bind_int(stmt, 13, (int)a->r_eCoalition);
  ```

### `TgHelpAlertVolume.{hpp,cpp}`
- SDK type: `ATgHelpAlertVolume`
- Table: `map_tg_help_alert_volume`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->m_nHelpId);
  ```

### `TgMissionListVolume.{hpp,cpp}`
- SDK type: `ATgMissionListVolume`
- Table: `map_tg_mission_list_volume`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->s_nQueueTableId);
  sqlite3_bind_int(stmt, 13, a->s_nQueueTableMsgId);
  ```

- [ ] **Step 1: Create 6 writer pairs**

- [ ] **Step 2: Add dispatch branches** — each casts directly to its leaf type for the id read:

```cpp
else if (cls == "TgOmegaVolume") {
    int id = static_cast<ATgOmegaVolume*>(actor)->m_nMapObjectId;
    WriteTgOmegaVolume(db, actor, mapName, cls, id);
}
else if (cls == "TgModifyPawnPropertiesVolume") {
    int id = static_cast<ATgModifyPawnPropertiesVolume*>(actor)->m_nMapObjectId;
    WriteTgModifyPawnPropertiesVolume(db, actor, mapName, cls, id);
}
else if (cls == "TgDeviceVolume") {
    int id = static_cast<ATgDeviceVolume*>(actor)->m_nMapObjectId;
    WriteTgDeviceVolume(db, actor, mapName, cls, id);
}
else if (cls == "TgFlagCaptureVolume") {
    int id = static_cast<ATgFlagCaptureVolume*>(actor)->m_nMapObjectId;
    WriteTgFlagCaptureVolume(db, actor, mapName, cls, id);
}
else if (cls == "TgHelpAlertVolume") {
    int id = static_cast<ATgHelpAlertVolume*>(actor)->m_nMapObjectId;
    WriteTgHelpAlertVolume(db, actor, mapName, cls, id);
}
else if (cls == "TgMissionListVolume") {
    int id = static_cast<ATgMissionListVolume*>(actor)->m_nMapObjectId;
    WriteTgMissionListVolume(db, actor, mapName, cls, id);
}
```

- [ ] **Step 3: Add 6 cpp paths to Makefile**

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/Tg{OmegaVolume,ModifyPawnPropertiesVolume,DeviceVolume,FlagCaptureVolume,HelpAlertVolume,MissionListVolume}.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \
        Makefile
git commit -m "mapdump: add Volume-based writers (6 classes) + dispatch"
```

---

## Task 9: TgNavigationPoint hierarchy writers (9 classes)

**Classes:** TgNavigationPoint, TgBotStart, TgActionPoint, TgAlarmPoint, TgCoverPoint, TgDefensePoint, TgHoldSpot, TgNavigationPointSpawnable, TgPointOfInterest.

### `TgNavigationPoint.{hpp,cpp}`
- SDK type: `ATgNavigationPoint`
- Table: `map_tg_navigation_point`
- No own scalars.

### `TgBotStart.{hpp,cpp}`
- SDK type: `ATgBotStart`
- Table: `map_tg_bot_start`
- No own scalars.

### `TgActionPoint.{hpp,cpp}`
- SDK type: `ATgActionPoint`
- Table: `map_tg_action_point`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, (int)a->ActionType);
  sqlite3_bind_int(stmt, 13, (int)a->nObjectiveNum);
  sqlite3_bind_int(stmt, 14, (int)a->nTaskForce);
  sqlite3_bind_int(stmt, 15, a->bUseRotation ? 1 : 0);
  ```

### `TgAlarmPoint.{hpp,cpp}`
- SDK type: `ATgAlarmPoint`
- Table: `map_tg_alarm_point`
- No own scalars.

### `TgCoverPoint.{hpp,cpp}`
- SDK type: `ATgCoverPoint`
- Table: `map_tg_cover_point`
- Own scalars:
  ```cpp
  sqlite3_bind_int   (stmt, 12, a->m_bLeanLeft ? 1 : 0);
  sqlite3_bind_int   (stmt, 13, a->m_bLeanRight ? 1 : 0);
  sqlite3_bind_int   (stmt, 14, a->m_bAllowPopup ? 1 : 0);
  sqlite3_bind_int   (stmt, 15, a->m_bAllowMantle ? 1 : 0);
  sqlite3_bind_double(stmt, 16, a->m_vLeanLeft.X);
  sqlite3_bind_double(stmt, 17, a->m_vLeanLeft.Y);
  sqlite3_bind_double(stmt, 18, a->m_vLeanLeft.Z);
  sqlite3_bind_double(stmt, 19, a->m_vLeanRight.X);
  sqlite3_bind_double(stmt, 20, a->m_vLeanRight.Y);
  sqlite3_bind_double(stmt, 21, a->m_vLeanRight.Z);
  sqlite3_bind_double(stmt, 22, a->m_vPopUp.X);
  sqlite3_bind_double(stmt, 23, a->m_vPopUp.Y);
  sqlite3_bind_double(stmt, 24, a->m_vPopUp.Z);
  ```

### `TgDefensePoint.{hpp,cpp}`
- SDK type: `ATgDefensePoint`
- Table: `map_tg_defense_point`
- Own scalars:
  ```cpp
  sqlite3_bind_int   (stmt, 12, a->bFirstScript ? 1 : 0);
  sqlite3_bind_int   (stmt, 13, a->bSniping ? 1 : 0);
  sqlite3_bind_int   (stmt, 14, a->bDontChangePosition ? 1 : 0);
  sqlite3_bind_int   (stmt, 15, a->bAvoid ? 1 : 0);
  sqlite3_bind_int   (stmt, 16, a->bDisabled ? 1 : 0);
  sqlite3_bind_int   (stmt, 17, a->bNotInVehicle ? 1 : 0);
  sqlite3_bind_int   (stmt, 18, (int)a->Priority);
  sqlite3_bind_double(stmt, 19, a->NumChecked);
  ```

### `TgHoldSpot.{hpp,cpp}`
- SDK type: `ATgHoldSpot`
- Table: `map_tg_hold_spot`
- No own scalars.

### `TgNavigationPointSpawnable.{hpp,cpp}`
- SDK type: `ATgNavigationPointSpawnable`
- Table: `map_tg_navigation_point_spawnable`
- No own scalars.

### `TgPointOfInterest.{hpp,cpp}`
- SDK type: `ATgPointOfInterest`
- Table: `map_tg_point_of_interest`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->m_nNameMsgId);
  auto debugName = FStringToUtf8(a->m_sDebugName);
  sqlite3_bind_text(stmt, 13, debugName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 14, a->m_QuestRadiusUU);
  sqlite3_bind_int(stmt, 15, a->m_bShowWhenQuestComplete ? 1 : 0);
  ```

- [ ] **Step 1: Create 9 writer pairs**

- [ ] **Step 2: Add dispatch branches**

```cpp
else if (cls == "TgBotStart") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgBotStart       (db, actor, mapName, cls, id);
}
else if (cls == "TgActionPoint") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgActionPoint    (db, actor, mapName, cls, id);
}
else if (cls == "TgAlarmPoint") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgAlarmPoint     (db, actor, mapName, cls, id);
}
else if (cls == "TgCoverPoint") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgCoverPoint     (db, actor, mapName, cls, id);
}
else if (cls == "TgDefensePoint") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgDefensePoint   (db, actor, mapName, cls, id);
}
else if (cls == "TgHoldSpot") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgDefensePoint   (db, actor, mapName, cls, id);
    WriteTgHoldSpot       (db, actor, mapName, cls, id);
}
else if (cls == "TgNavigationPointSpawnable") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint         (db, actor, mapName, cls, id);
    WriteTgNavigationPointSpawnable(db, actor, mapName, cls, id);
}
else if (cls == "TgPointOfInterest") {
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
    WriteTgPointOfInterest(db, actor, mapName, cls, id);
}
else if (cls == "TgNavigationPoint") {  // bare base
    int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
    WriteTgNavigationPoint(db, actor, mapName, cls, id);
}
```

- [ ] **Step 3: Add 9 cpp paths to Makefile**

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/Tg{NavigationPoint,BotStart,ActionPoint,AlarmPoint,CoverPoint,DefensePoint,HoldSpot,NavigationPointSpawnable,PointOfInterest}.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \
        Makefile
git commit -m "mapdump: add TgNavigationPoint hierarchy writers (9 classes) + dispatch"
```

---

## Task 10: TgTeleporter writer (1 class)

### `TgTeleporter.{hpp,cpp}`
- SDK type: `ATgTeleporter`
- Table: `map_tg_teleporter`
- Own scalars:
  ```cpp
  sqlite3_bind_int(stmt, 12, a->m_nMapId);
  sqlite3_bind_int(stmt, 13, a->m_nPreload ? 1 : 0);
  sqlite3_bind_int(stmt, 14, a->m_bSetTaskForce ? 1 : 0);
  sqlite3_bind_int(stmt, 15, a->m_bBalanceTaskForce ? 1 : 0);
  sqlite3_bind_int(stmt, 16, a->m_bIgnoreNonMembers ? 1 : 0);
  sqlite3_bind_int(stmt, 17, a->m_bUsePlayerStart ? 1 : 0);
  sqlite3_bind_int(stmt, 18, a->m_bRequestMission ? 1 : 0);
  sqlite3_bind_int(stmt, 19, (int)a->m_nStartGroup);
  sqlite3_bind_int(stmt, 20, (int)a->m_nTaskForce);
  ```

- [ ] **Step 1: Create writer pair**

- [ ] **Step 2: Add dispatch branch**

```cpp
else if (cls == "TgTeleporter") {
    int id = static_cast<ATgTeleporter*>(actor)->m_nMapObjectId;
    WriteTgTeleporter(db, actor, mapName, cls, id);
}
```

- [ ] **Step 3: Add cpp path to Makefile**

```make
              $(SRC_DIR)/GameServer/Engine/MapDataDumper/Writers/TgTeleporter.cpp \
```

- [ ] **Step 4: Commit**

```bash
git add src/GameServer/Engine/MapDataDumper/Writers/TgTeleporter.{hpp,cpp} \
        src/GameServer/Engine/MapDataDumper/MapDataDumper.cpp \
        Makefile
git commit -m "mapdump: add TgTeleporter writer + dispatch (final dispatch branch)"
```

---

## Post-implementation handoff

After Task 10 commits, the implementation is complete. **Claude does NOT run `make`.** Hand back to the user with a summary of what was done and the verification checklist from the spec:

1. User builds with `make`.
2. User boots the server; checks `PRAGMA user_version` is 33.
3. User joins `1P_CPLab05_P` with `-dumpmapdata=1` and inspects the `map_*` tables.
4. User confirms idempotency by restarting with the same flag (row counts unchanged).
5. User confirms `map_tg_bot_factory.location_x/y/z` matches the existing `obj_map_object` rows.

If any step fails, surface the failure and either fix the bug or escalate.

---

## Self-review checklist

Performed mentally as the final step of writing this plan:

**Spec coverage** — each spec section maps to one or more tasks:
- ✅ 34 tables → Task 1 migration
- ✅ Common columns → BindCommonCols in Task 2, applied by every writer task
- ✅ Class-specific scalars → per-class bind lists in Tasks 5–10
- ✅ Array tables → Task 4
- ✅ Type mappings → bind-type table in the common writer template
- ✅ Dispatch logic → incrementally appended in Tasks 5–10
- ✅ Idempotent re-dumps via DELETE-then-INSERT in transaction → Task 3
- ✅ File logging removed → Task 3
- ✅ DumpAll signature unchanged → Task 3
- ✅ Writers/ subdir structure → Task 2 onwards
- ✅ TgTrigger_Use dropped, TgBotStart restored → Task 9 includes TgBotStart, no task touches TgTrigger_Use

**No placeholders:** every step has either concrete code or a per-class bind list. No `TBD`, `TODO`, `implement later`, `add appropriate error handling`.

**Type consistency:**
- ✅ Writer signature: `(sqlite3* db, AActor* actor, const std::string& mapName, const std::string& className, int mapObjectId)` everywhere (except array writers which take `ATgBotFactory*` directly)
- ✅ `BindCommonCols` signature matches between hpp and cpp
- ✅ Namespace `MapDumpWriters` used consistently
- ✅ Dispatch branches in `WriteByClass` all cast to the closest ancestor that declares `m_nMapObjectId`

Plan is implementation-ready.
