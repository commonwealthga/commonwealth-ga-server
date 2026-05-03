#include "src/Database/AsmDataCapture/AsmDataCapture.hpp"

#include "src/Database/Database.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>
#include <unordered_map>

bool AsmDataCapture::bPopulateDatabase = false;

namespace {
    // Global CMarshal translator singleton (DAT_119a1c08 in Ghidra — the first arg
    // every CMarshal__Translate call passes).
    void* const kTranslateContext = reinterpret_cast<void*>(0x119a1c08);

    // ---- Field getters (direct trampoline calls, bypass our own hooks) ----

    inline uint32_t Byte(void* row, int field) {
        uint32_t v = 0;
        CMarshal__GetByte::CallOriginal(row, nullptr, field, &v);
        return v;
    }
    inline uint32_t Int32(void* row, int field) {
        uint32_t v = 0;
        CMarshal__GetInt32t::CallOriginal(row, nullptr, field, &v);
        return v;
    }
    inline float Float(void* row, int field) {
        float v = 0.0f;
        CMarshal__GetFloat::CallOriginal(row, nullptr, field, &v);
        return v;
    }
    inline uint8_t Flag(void* row, int field) {
        uint8_t v = 0;
        CMarshal__GetFlag::CallOriginal(row, nullptr, field, &v);
        return v;
    }

    // Read a wchar field (NAME, UI_CODE, SOUND_CUE_NAME, etc.) via the game's
    // CMarshal__get_wchar_2 (hooked as CMarshal__GetString2). CallOriginal copies the
    // string into our buffer without triggering our own hooks.
    void GetWcharName(void* row, uint32_t field, char* out, size_t outSize) {
        if (outSize == 0) return;
        out[0] = '\0';
        wchar_t buf[1024] = {0};
        uint32_t bufSize = 1024;
        int ok = CMarshal__GetString2::CallOriginal(row, nullptr, field, buf, &bufSize);
        if (ok) {
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, out, (int)outSize - 1, NULL, NULL);
            out[outSize - 1] = '\0';
        }
    }

    // Translate a message id to UTF-8 via the game's Translate function.
    // out must be writable for at least outSize bytes; guaranteed NUL-terminated.
    void Translate(uint32_t msgId, void* row, char* out, size_t outSize) {
        if (outSize == 0) return;
        out[0] = '\0';
        if (msgId == 0) return;
        wchar_t* w = CMarshal__Translate::CallOriginal(kTranslateContext, nullptr, msgId, row);
        if (w) {
            WideCharToMultiByte(CP_UTF8, 0, w, -1, out, (int)outSize - 1, NULL, NULL);
            out[outSize - 1] = '\0';
        }
    }

    // ---- Array iteration helper ----
    //
    // The array-head struct returned in *Out of get_array has its first element at
    // (arrayPtr + 4), and each node's next pointer at (row + 8).  Matches the pattern
    // used by every dispatcher in the binary.
    template <class Fn>
    void WalkArray(uint32_t arrayPtr, Fn fn) {
        if (arrayPtr == 0) return;
        void* head = *(void**)((char*)arrayPtr + 4);
        for (void* row = head; row; row = *(void**)((char*)row + 8)) {
            fn(row);
        }
    }

    // ========================================================================
    //                               WALKERS
    // ========================================================================
    //
    // One walker per data-set id. Wrap bulk inserts in a transaction — sqlite is
    // otherwise ~100x slower on these write-heavy loads.

    struct Txn {
        sqlite3* db;
        explicit Txn(sqlite3* d) : db(d) { sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr); }
        ~Txn() { sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr); }
    };

    // Per-table idempotency guard.
    //
    // Rationale: bPopulateDatabase is flipped on in dllmain and stays on, so every
    // game launch re-invokes every walker.  Without a guard, each launch would
    // duplicate every asm_* row.  ShouldWalk checks once (cached for the session)
    // whether the target table is empty; populated tables short-circuit, freshly
    // created tables (v17 migration adds several) populate on the next run.
    //
    // Nested walkers fire many times per session (once per parent row). The cache
    // makes each invocation O(1) after the first; the "empty at session start"
    // flag is sticky so child walkers keep inserting throughout the run rather
    // than stopping the moment their own first row lands.
    bool ShouldWalk(const char* table) {
        static std::unordered_map<std::string, bool> cache;
        auto it = cache.find(table);
        if (it != cache.end()) return it->second;

        sqlite3* db = Database::GetConnection();
        if (!db) { cache[table] = false; return false; }

        bool empty = true;
        char sql[256];
        snprintf(sql, sizeof(sql), "SELECT 1 FROM %s LIMIT 1", table);
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK) {
            empty = (sqlite3_step(s) != SQLITE_ROW);
            sqlite3_finalize(s);
        } else {
            // Table missing or otherwise unreadable — don't attempt to write.
            empty = false;
        }
        cache[table] = empty;
        if (empty) {
            Logger::Log("db", "AsmDataCapture: will populate %s\n", table);
        }
        return empty;
    }

    // For child data sets, the outer parent's iteration populates m_values with its
    // primary key via get_byte/get_int32_t before the game reaches get_array for the
    // child.  Child walkers read the parent key from those caches — matches the
    // pattern the existing per-row hooks already relied on.
    inline uint32_t CachedByte (int field) { return CMarshal__GetByte   ::m_values[field]; }
    inline uint32_t CachedInt32(int field) { return CMarshal__GetInt32t::m_values[field]; }

    // ---------- Items (DATA_SET_ITEMS = 0x0165) ----------
    //
    // Schema: asm_data_set_items (created in Database::Init at version < 5).
    // Column/field mapping matches existing CAmItem__LoadItemMarshal exactly.
    void WalkItems(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        static const char* kSql =
            "INSERT INTO asm_data_set_items ("
            "  name_msg_id, name_msg_translated,"
            "  desc_msg_id, desc_msg_translated,"
            "  class_res_id, item_id, item_type_value_id, item_subtype_value_id,"
            "  skill_id, sub_skill_id, skill_level_min, quantity, icon_id,"
            "  weight, time_to_live_secs, quality_value_id,"
            "  required_achievement_id, required_achievement_points,"
            "  ref_bot_id, ref_deployable_id, ref_device_id,"
            "  item_bind_type_value_id, production_cost, required_level,"
            "  wear_flair_start_date, wear_flair_duration,"
            "  purchased_value, bundle_loot_table_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkItems prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        char nameBuf[4096], descBuf[4096], wearBuf[1024];

        WalkArray(arr, [&](void* row) {
            uint32_t nameMsgId = Int32(row, GA_T::NAME_MSG_ID);
            uint32_t descMsgId = Int32(row, GA_T::DESC_MSG_ID);

            Translate(nameMsgId, row, nameBuf, sizeof(nameBuf));
            Translate(descMsgId, row, descBuf, sizeof(descBuf));
            // WEAR_FLAIR_START_DATE is a wchar field on item rows (loader uses
            // get_wchar_t at FUN_1094f1b0). Most items leave it empty.
            GetWcharName(row, GA_T::WEAR_FLAIR_START_DATE, wearBuf, sizeof(wearBuf));

            sqlite3_bind_int   (stmt,  1, (int)nameMsgId);
            sqlite3_bind_text  (stmt,  2, nameBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (stmt,  3, (int)descMsgId);
            sqlite3_bind_text  (stmt,  4, descBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (stmt,  5, (int)Int32(row, GA_T::CLASS_RES_ID));
            sqlite3_bind_int   (stmt,  6, (int)Byte (row, GA_T::ITEM_ID));
            sqlite3_bind_int   (stmt,  7, (int)Byte (row, GA_T::ITEM_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt,  8, (int)Byte (row, GA_T::ITEM_SUBTYPE_VALUE_ID));
            sqlite3_bind_int   (stmt,  9, (int)Byte (row, GA_T::SKILL_ID));
            sqlite3_bind_int   (stmt, 10, (int)Byte (row, GA_T::SUB_SKILL_ID));
            sqlite3_bind_int   (stmt, 11, (int)Byte (row, GA_T::SKILL_LEVEL_MIN));
            sqlite3_bind_int   (stmt, 12, (int)Byte (row, GA_T::QUANTITY));
            sqlite3_bind_int   (stmt, 13, (int)Byte (row, GA_T::ICON_ID));
            sqlite3_bind_double(stmt, 14, (double)Float(row, GA_T::WEIGHT));
            sqlite3_bind_double(stmt, 15, (double)Float(row, GA_T::TIME_TO_LIVE_SECS));
            sqlite3_bind_int   (stmt, 16, (int)Byte (row, GA_T::QUALITY_VALUE_ID));
            sqlite3_bind_int   (stmt, 17, (int)Byte (row, GA_T::REQUIRED_ACHIEVEMENT_ID));
            sqlite3_bind_int   (stmt, 18, (int)Byte (row, GA_T::REQUIRED_ACHIEVEMENT_POINTS));
            sqlite3_bind_int   (stmt, 19, (int)Byte (row, GA_T::REF_BOT_ID));
            sqlite3_bind_int   (stmt, 20, (int)Byte (row, GA_T::REF_DEPLOYABLE_ID));
            sqlite3_bind_int   (stmt, 21, (int)Byte (row, GA_T::REF_DEVICE_ID));
            sqlite3_bind_int   (stmt, 22, (int)Byte (row, GA_T::ITEM_BIND_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 23, (int)Byte (row, GA_T::PRODUCTION_COST));
            sqlite3_bind_int   (stmt, 24, (int)Byte (row, GA_T::REQUIRED_LEVEL));
            sqlite3_bind_text  (stmt, 25, wearBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (stmt, 26, (int)Byte (row, GA_T::WEAR_FLAIR_DURATION));
            sqlite3_bind_int   (stmt, 27, (int)Byte (row, GA_T::PURCHASED_VALUE));
            sqlite3_bind_int   (stmt, 28, (int)Byte (row, GA_T::BUNDLE_LOOT_TABLE_ID));

            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ---------- Bots (DATA_SET_BOTS = 0x0124) ----------
    //
    // Fields mirror the existing asm_data_set_bots schema (migration versions 1 + 9).
    // Nested DATA_SET_BOT_DEVICES (0x0129) is handled by WalkBotDevices via the
    // separate dispatch — the game calls get_array(row, 0x129) inside its own per-bot
    // processing and our hook fires with the parent BOT_ID already in m_values.
    void WalkBots(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        static const char* kSql =
            "INSERT INTO asm_data_set_bots ("
            "  bot_id, reference_name, name_msg_id, desc_msg_id, level,"
            "  pawn_class_res_id, controller_class_res_id, behavior_id,"
            "  head_asm_id, body_asm_id, movement_asm_id, hit_points,"
            "  bot_type_value_id, physical_type_value_id, default_slot_value_id,"
            "  default_sensor_range, default_aggro_range, default_help_range,"
            "  hearing_range, default_speed, walk_speed_pct, crouch_speed_pct,"
            "  chase_range, chase_time_sec, stealth_sensor_range, stealth_aggro_range,"
            "  hibernate_on_idle_sec, hibernate_delay_rate, icon_id,"
            "  bot_rank_value_id, target_only_physical_type_value_id,"
            "  skill_group_id, skill_group_set_id, fixed_fov_degrees, loot_table_id,"
            "  default_power_pool, rotation_rate, class_type_value_id,"
            "  device_slot_unlock_group_id, pickup_device_id, xp_value, currency_value,"
            "  squad_role_value_id, default_posture_value_id, acceleration_rate,"
            "  accuracy_override, bot_balance_multiplier, power_pool_regen_per_sec,"
            "  crew_control_radius,"
            "  hibernate_invulnerability_flag, can_jump_flag, can_climb_ladders_flag,"
            "  path_only_flag, always_load_on_server_flag, destroy_on_owner_death_flag"
            ") VALUES ("
            "?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,"
            "?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkBots prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            // reference_name: captured by CMarshal__GetString2 into its m_values map
            // during the game's own per-bot processing.  Read after the bot row is
            // walked — see below; we keep the same pattern as the old hook.
            const char* refName = CMarshal__GetString2::m_values[GA_T::REFERENCE_NAME];

            sqlite3_bind_int   (stmt,  1, (int)Byte (row, GA_T::BOT_ID));
            sqlite3_bind_text  (stmt,  2, refName, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (stmt,  3, (int)Int32(row, GA_T::NAME_MSG_ID));
            sqlite3_bind_int   (stmt,  4, (int)Int32(row, GA_T::DESC_MSG_ID));
            sqlite3_bind_int   (stmt,  5, (int)Int32(row, GA_T::LEVEL));
            sqlite3_bind_int   (stmt,  6, (int)Int32(row, GA_T::PAWN_CLASS_RES_ID));
            sqlite3_bind_int   (stmt,  7, (int)Int32(row, GA_T::CONTROLLER_CLASS_RES_ID));
            sqlite3_bind_int   (stmt,  8, (int)Byte (row, GA_T::BEHAVIOR_ID));
            sqlite3_bind_int   (stmt,  9, (int)Byte (row, GA_T::HEAD_ASM_ID));
            sqlite3_bind_int   (stmt, 10, (int)Byte (row, GA_T::BODY_ASM_ID));
            sqlite3_bind_int   (stmt, 11, (int)Byte (row, GA_T::MOVEMENT_ASM_ID));
            sqlite3_bind_int   (stmt, 12, (int)Byte (row, GA_T::HIT_POINTS));
            sqlite3_bind_int   (stmt, 13, (int)Byte (row, GA_T::BOT_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 14, (int)Byte (row, GA_T::PHYSICAL_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 15, (int)Byte (row, GA_T::DEFAULT_SLOT_VALUE_ID));
            sqlite3_bind_double(stmt, 16, (double)Float(row, GA_T::DEFAULT_SENSOR_RANGE));
            sqlite3_bind_int   (stmt, 17, (int)Byte (row, GA_T::DEFAULT_AGGRO_RANGE));
            sqlite3_bind_int   (stmt, 18, (int)Byte (row, GA_T::DEFAULT_HELP_RANGE));
            sqlite3_bind_double(stmt, 19, (double)Float(row, GA_T::HEARING_RANGE));
            sqlite3_bind_double(stmt, 20, (double)Float(row, GA_T::DEFAULT_SPEED));
            sqlite3_bind_double(stmt, 21, (double)Float(row, GA_T::WALK_SPEED_PCT));
            sqlite3_bind_double(stmt, 22, (double)Float(row, GA_T::CROUCH_SPEED_PCT));
            sqlite3_bind_int   (stmt, 23, (int)Byte (row, GA_T::CHASE_RANGE));
            sqlite3_bind_double(stmt, 24, (double)Float(row, GA_T::CHASE_TIME_SEC));
            sqlite3_bind_int   (stmt, 25, (int)Byte (row, GA_T::STEALTH_SENSOR_RANGE));
            sqlite3_bind_int   (stmt, 26, (int)Byte (row, GA_T::STEALTH_AGGRO_RANGE));
            sqlite3_bind_int   (stmt, 27, (int)Byte (row, GA_T::HIBERNATE_ON_IDLE_SEC));
            sqlite3_bind_double(stmt, 28, (double)Float(row, GA_T::HIBERNATE_DELAY_RATE));
            sqlite3_bind_int   (stmt, 29, (int)Byte (row, GA_T::ICON_ID));
            sqlite3_bind_int   (stmt, 30, (int)Byte (row, GA_T::BOT_RANK_VALUE_ID));
            sqlite3_bind_int   (stmt, 31, (int)Byte (row, GA_T::TARGET_ONLY_PHYSICAL_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 32, (int)Byte (row, GA_T::SKILL_GROUP_ID));
            sqlite3_bind_int   (stmt, 33, (int)Byte (row, GA_T::SKILL_GROUP_SET_ID));
            sqlite3_bind_int   (stmt, 34, (int)Byte (row, GA_T::FIXED_FOV_DEGREES));
            sqlite3_bind_int   (stmt, 35, (int)Byte (row, GA_T::LOOT_TABLE_ID));
            sqlite3_bind_int   (stmt, 36, (int)Byte (row, GA_T::DEFAULT_POWER_POOL));
            sqlite3_bind_int   (stmt, 37, (int)Byte (row, GA_T::ROTATION_RATE));
            sqlite3_bind_int   (stmt, 38, (int)Byte (row, GA_T::CLASS_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 39, (int)Byte (row, GA_T::DEVICE_SLOT_UNLOCK_GROUP_ID));
            sqlite3_bind_int   (stmt, 40, (int)Byte (row, GA_T::PICKUP_DEVICE_ID));
            sqlite3_bind_int   (stmt, 41, (int)Byte (row, GA_T::XP_VALUE));
            sqlite3_bind_int   (stmt, 42, (int)Byte (row, GA_T::CURRENCY_VALUE));
            sqlite3_bind_int   (stmt, 43, (int)Byte (row, GA_T::SQUAD_ROLE_VALUE_ID));
            sqlite3_bind_int   (stmt, 44, (int)Byte (row, GA_T::DEFAULT_POSTURE_VALUE_ID));
            sqlite3_bind_double(stmt, 45, (double)Float(row, GA_T::ACCELERATION_RATE));
            sqlite3_bind_double(stmt, 46, (double)Float(row, GA_T::ACCURACY_OVERRIDE));
            sqlite3_bind_double(stmt, 47, (double)Float(row, GA_T::BOT_BALANCE_MULTIPLIER));
            sqlite3_bind_double(stmt, 48, (double)Float(row, GA_T::POWER_POOL_REGEN_PER_SEC));
            sqlite3_bind_double(stmt, 49, (double)Float(row, GA_T::CREW_CONTROL_RADIUS));
            sqlite3_bind_int   (stmt, 50, (int)Flag (row, GA_T::HIBERNATE_INVULNERABILITY_FLAG));
            sqlite3_bind_int   (stmt, 51, (int)Flag (row, GA_T::CAN_JUMP_FLAG));
            sqlite3_bind_int   (stmt, 52, (int)Flag (row, GA_T::CAN_CLIMB_LADDERS_FLAG));
            sqlite3_bind_int   (stmt, 53, (int)Flag (row, GA_T::PATH_ONLY_FLAG));
            sqlite3_bind_int   (stmt, 54, (int)Flag (row, GA_T::ALWAYS_LOAD_ON_SERVER_FLAG));
            sqlite3_bind_int   (stmt, 55, (int)Flag (row, GA_T::DESTROY_ON_OWNER_DEATH_FLAG));

            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ---------- Bot Devices (DATA_SET_BOT_DEVICES = 0x0129) ----------
    //
    // Nested child of bots.  Our hook fires when the game calls
    // get_array(bot_row, 0x129) inside its per-bot processing — at which point
    // m_values[BOT_ID] holds the current parent bot id.
    void WalkBotDevices(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t botId = CachedByte(GA_T::BOT_ID);

        static const char* kSql =
            "INSERT INTO asm_data_set_bots_data_set_bot_devices ("
            "  bot_id, device_id, slot_used_value_id"
            ") VALUES (?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkBotDevices prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(stmt, 1, (int)botId);
            sqlite3_bind_int(stmt, 2, (int)Byte(row, GA_T::DEVICE_ID));
            sqlite3_bind_int(stmt, 3, (int)Byte(row, GA_T::SLOT_USED_VALUE_ID));
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ---------- Bot Spawn Tables (DATA_SET_BOT_SPAWN_TABLES = 0x012C) ----------
    void WalkBotSpawnTables(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        static const char* kSql =
            "INSERT INTO asm_data_set_bot_spawn_tables ("
            "  bot_spawn_table_id, difficulty_value_id, player_profile_id,"
            "  spawn_group, enemy_bot_id, bot_count, spawn_chance, team_size,"
            "  multiple_class_flag, bot_balance_multiplier,"
            "  spawn_group_min, spawn_group_max, spawn_group_respawn_sec"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkBotSpawnTables prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int   (stmt,  1, (int)Byte (row, GA_T::BOT_SPAWN_TABLE_ID));
            sqlite3_bind_int   (stmt,  2, (int)Byte (row, GA_T::DIFFICULTY_VALUE_ID));
            sqlite3_bind_int   (stmt,  3, (int)Byte (row, GA_T::PLAYER_PROFILE_ID));
            sqlite3_bind_int   (stmt,  4, (int)Byte (row, GA_T::SPAWN_GROUP));
            sqlite3_bind_int   (stmt,  5, (int)Byte (row, GA_T::ENEMY_BOT_ID));
            sqlite3_bind_int   (stmt,  6, (int)Byte (row, GA_T::BOT_COUNT));
            sqlite3_bind_double(stmt,  7, (double)Float(row, GA_T::SPAWN_CHANCE));
            sqlite3_bind_int   (stmt,  8, (int)Byte (row, GA_T::TEAM_SIZE));
            sqlite3_bind_int   (stmt,  9, (int)Flag (row, GA_T::MULTIPLE_CLASS_FLAG));
            sqlite3_bind_double(stmt, 10, (double)Float(row, GA_T::BOT_BALANCE_MULTIPLIER));
            sqlite3_bind_int   (stmt, 11, (int)Byte (row, GA_T::SPAWN_GROUP_MIN));
            sqlite3_bind_int   (stmt, 12, (int)Byte (row, GA_T::SPAWN_GROUP_MAX));
            sqlite3_bind_int   (stmt, 13, (int)Byte (row, GA_T::SPAWN_GROUP_RESPAWN_SEC));
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ---------- Devices (DATA_SET_DEVICES = 0x0140) ----------
    void WalkDevices(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        static const char* kSql =
            "INSERT INTO asm_data_set_devices ("
            "  device_id, form_class_res_id, mount_socket_res_id, time_to_equip_secs,"
            "  container_skill_group_id, right_click_behavior_type_value_id,"
            "  slot_used_value_id, in_hand_device_flag"
            ") VALUES (?,?,?,?,?,?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkDevices prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            // The outer dispatcher (FUN_1094e1e0) reads DEVICE_ID via get_int32_t
            // before handing the row off; we mirror that by also reading via Int32.
            sqlite3_bind_int   (stmt, 1, (int)Int32(row, GA_T::DEVICE_ID));
            sqlite3_bind_int   (stmt, 2, (int)Int32(row, GA_T::FORM_CLASS_RES_ID));
            sqlite3_bind_int   (stmt, 3, (int)Int32(row, GA_T::MOUNT_SOCKET_RES_ID));
            sqlite3_bind_double(stmt, 4, (double)Float(row, GA_T::TIME_TO_EQUIP_SECS));
            sqlite3_bind_int   (stmt, 5, (int)Byte (row, GA_T::CONTAINER_SKILL_GROUP_ID));
            sqlite3_bind_int   (stmt, 6, (int)Byte (row, GA_T::RIGHT_CLICK_BEHAVIOR_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 7, (int)Byte (row, GA_T::SLOT_USED_VALUE_ID));
            sqlite3_bind_int   (stmt, 8, (int)Flag (row, GA_T::IN_HAND_DEVICE_FLAG));
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ---------- Device Modes (DATA_SET_DEVICE_MODES = 0x0145) ----------
    //
    // Nested child of devices.  The outer dispatcher populates m_values[DEVICE_ID]
    // before calling get_array(device_row, 0x145) which lands here.
    //
    // Full column set pulled from CAssemblyManager::LoadDevices +
    // CAmDeviceModel::LoadDeviceModeMarshal (FUN_1094ae20 @ 0x1094ae20).
    void WalkDeviceModes(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        static const char* kSql =
            "INSERT INTO asm_data_set_devices_data_set_device_modes ("
            "  device_id, device_mode_id, name_msg_id, name_msg_translated,"
            "  class_res_id, damage_class_res_id,"
            "  device_projectile_id, deployable_id, bot_id, impact_fx_group_id,"
            "  damage_type_value_id, offhand_anim_res_id,"
            "  target_type_value_id, target_type_affect_value_id,"
            "  attack_type_value_id, remote_type_value_id, camera_type_value_id,"
            "  reticule_res_id, scope_material_res_id,"
            "  scale_fire_anim_flag, interrupt_fire_anim_on_refire_flag,"
            "  require_los_flag, continuous_fire_flag,"
            "  restrict_in_combat_flag, require_aimmode_flag, do_not_pause_ai_flag,"
            "  restrict_firing_flags, restrict_physics_firing_flags,"
            "  return_to_idle_anim_secs, icon_id, attack_rating, fire_mode_sequence"
            ") VALUES ("
            "  ?,?,?,?, ?,?, ?,?,?,?, ?,?, ?,?, ?,?,?, ?,?,"
            "  ?,?, ?,?, ?,?,?, ?,?, ?,?,?,? )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkDeviceModes prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        char nameBuf[4096];
        WalkArray(arr, [&](void* row) {
            uint32_t nameMsgId = Int32(row, GA_T::NAME_MSG_ID);
            Translate(nameMsgId, row, nameBuf, sizeof(nameBuf));

            int c = 1;
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::DEVICE_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::DEVICE_MODE_ID));
            sqlite3_bind_int (stmt, c++, (int)nameMsgId);
            sqlite3_bind_text(stmt, c++, nameBuf, -1, SQLITE_TRANSIENT);

            sqlite3_bind_int (stmt, c++, (int)Int32(row, GA_T::CLASS_RES_ID));
            sqlite3_bind_int (stmt, c++, (int)Int32(row, GA_T::DAMAGE_CLASS_RES_ID));

            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::DEVICE_PROJECTILE_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::DEPLOYABLE_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::BOT_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::IMPACT_FX_GROUP_ID));

            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::DAMAGE_TYPE_VALUE_ID));
            sqlite3_bind_int (stmt, c++, (int)Int32(row, GA_T::OFFHAND_ANIM_RES_ID));

            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::TARGET_TYPE_VALUE_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::TARGET_TYPE_AFFECT_VALUE_ID));

            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::ATTACK_TYPE_VALUE_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::REMOTE_TYPE_VALUE_ID));
            sqlite3_bind_int (stmt, c++, (int)Byte (row, GA_T::CAMERA_TYPE_VALUE_ID));

            sqlite3_bind_int (stmt, c++, (int)Int32(row, GA_T::RETICULE_RES_ID));
            sqlite3_bind_int (stmt, c++, (int)Int32(row, GA_T::SCOPE_MATERIAL_RES_ID));

            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::SCALE_FIRE_ANIM_FLAG));
            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::INTERRUPT_FIRE_ANIM_ON_REFIRE_FLAG));

            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::REQUIRE_LOS_FLAG));
            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::CONTINUOUS_FIRE_FLAG));

            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::RESTRICT_IN_COMBAT_FLAG));
            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::REQUIRE_AIMMODE_FLAG));
            sqlite3_bind_int (stmt, c++, (int)Flag (row, GA_T::DO_NOT_PAUSE_AI_FLAG));

            sqlite3_bind_int   (stmt, c++, (int)Byte (row, GA_T::RESTRICT_FIRING_FLAGS));
            sqlite3_bind_int   (stmt, c++, (int)Byte (row, GA_T::RESTRICT_PHYSICS_FIRING_FLAGS));

            sqlite3_bind_double(stmt, c++, (double)Float(row, GA_T::RETURN_TO_IDLE_ANIM_SECS));
            sqlite3_bind_int   (stmt, c++, (int)Byte (row, GA_T::ICON_ID));
            sqlite3_bind_int   (stmt, c++, (int)Byte (row, GA_T::ATTACK_RATING));
            sqlite3_bind_int   (stmt, c++, (int)Int32(row, GA_T::FIRE_MODE_SEQUENCE));

            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ---------- Effect Groups (DATA_SET_EFFECT_GROUPS = 0x014E) ----------
    void WalkEffectGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        static const char* kSql =
            "INSERT INTO asm_data_set_effect_groups ("
            "  effect_group_id, lifetime_sec, apply_interval_sec,"
            "  target_fx_id, fx_display_group_res_id, icon_id,"
            "  application_value_id, category_value_id, application_value,"
            "  application_chance, situational_type_value_id,"
            "  required_category_value_id, required_skill_id,"
            "  effect_group_type_value_id, health, situational_value,"
            "  stack_count_max, buff_value, contagion_flag, device_specific_flag,"
            "  posture_type_value_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkEffectGroups prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int   (stmt,  1, (int)Int32(row, GA_T::EFFECT_GROUP_ID));
            sqlite3_bind_double(stmt,  2, (double)Float(row, GA_T::LIFETIME_SEC));
            sqlite3_bind_double(stmt,  3, (double)Float(row, GA_T::APPLY_INTERVAL_SEC));
            sqlite3_bind_int   (stmt,  4, (int)Int32(row, GA_T::TARGET_FX_ID));
            sqlite3_bind_int   (stmt,  5, (int)Int32(row, GA_T::FX_DISPLAY_GROUP_RES_ID));
            sqlite3_bind_int   (stmt,  6, (int)Int32(row, GA_T::ICON_ID));
            sqlite3_bind_int   (stmt,  7, (int)Byte (row, GA_T::APPLICATION_VALUE_ID));
            sqlite3_bind_int   (stmt,  8, (int)Byte (row, GA_T::CATEGORY_VALUE_ID));
            sqlite3_bind_double(stmt,  9, (double)Float(row, GA_T::APPLICATION_VALUE));
            sqlite3_bind_double(stmt, 10, (double)Float(row, GA_T::APPLICATION_CHANCE));
            sqlite3_bind_int   (stmt, 11, (int)Byte (row, GA_T::SITUATIONAL_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 12, (int)Byte (row, GA_T::REQUIRED_CATEGORY_VALUE_ID));
            sqlite3_bind_int   (stmt, 13, (int)Byte (row, GA_T::REQUIRED_SKILL_ID));
            sqlite3_bind_int   (stmt, 14, (int)Byte (row, GA_T::EFFECT_GROUP_TYPE_VALUE_ID));
            sqlite3_bind_int   (stmt, 15, (int)Byte (row, GA_T::HEALTH));
            sqlite3_bind_double(stmt, 16, (double)Float(row, GA_T::SITUATIONAL_VALUE));
            sqlite3_bind_int   (stmt, 17, (int)Byte (row, GA_T::STACK_COUNT_MAX));
            sqlite3_bind_int   (stmt, 18, (int)Byte (row, GA_T::BUFF_VALUE));
            sqlite3_bind_int   (stmt, 19, (int)Flag (row, GA_T::CONTAGION_FLAG));
            sqlite3_bind_int   (stmt, 20, (int)Flag (row, GA_T::DEVICE_SPECIFIC_FLAG));
            sqlite3_bind_int   (stmt, 21, (int)Byte (row, GA_T::POSTURE_TYPE_VALUE_ID));
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // Small utility: prepare+step+reset helper reduces boilerplate in walkers that
    // don't need per-row logic beyond binding fields.
    struct Stmt {
        sqlite3_stmt* s = nullptr;
        sqlite3* db;
        Stmt(sqlite3* d, const char* sql) : db(d) {
            if (sqlite3_prepare_v2(d, sql, -1, &s, nullptr) != SQLITE_OK) {
                Logger::Log("db", "prepare failed: %s\n  sql=%s\n", sqlite3_errmsg(d), sql);
                s = nullptr;
            }
        }
        ~Stmt() { if (s) sqlite3_finalize(s); }
        void step() {
            if (!s) return;
            sqlite3_step(s);
            sqlite3_reset(s);
            sqlite3_clear_bindings(s);
        }
    };

    // ---------- Projectiles (0x018C) ----------
    void WalkProjectiles(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_projectiles ("
            " device_projectile_id, class_res_id, asm_id, explosion_fx_id,"
            " impact_fx_group_id, toss_z, acceleration_rate, draw_scale,"
            " max_nbr_of_bounces, spawn_item_id, spawn_bot_id, spawn_deployable_id,"
            " delay_track_secs, rotation_follows_velocity_flag, stick_to_wall_flag,"
            " track_target_flag, track_to_world_pos_flag, aim_from_socket_flag"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s,  1, (int)Int32(r, GA_T::DEVICE_PROJECTILE_ID));
            sqlite3_bind_int   (st.s,  2, (int)Int32(r, GA_T::CLASS_RES_ID));
            sqlite3_bind_int   (st.s,  3, (int)Byte (r, GA_T::ASM_ID));
            sqlite3_bind_int   (st.s,  4, (int)Byte (r, GA_T::EXPLOSION_FX_ID));
            sqlite3_bind_int   (st.s,  5, (int)Byte (r, GA_T::IMPACT_FX_GROUP_ID));
            sqlite3_bind_double(st.s,  6, (double)Float(r, GA_T::TOSS_Z));
            sqlite3_bind_double(st.s,  7, (double)Float(r, GA_T::ACCELERATION_RATE));
            sqlite3_bind_double(st.s,  8, (double)Float(r, GA_T::DRAW_SCALE));
            sqlite3_bind_int   (st.s,  9, (int)Byte (r, GA_T::MAX_NBR_OF_BOUNCES));
            sqlite3_bind_int   (st.s, 10, (int)Byte (r, GA_T::SPAWN_ITEM_ID));
            sqlite3_bind_int   (st.s, 11, (int)Byte (r, GA_T::SPAWN_BOT_ID));
            sqlite3_bind_int   (st.s, 12, (int)Byte (r, GA_T::SPAWN_DEPLOYABLE_ID));
            sqlite3_bind_double(st.s, 13, (double)Float(r, GA_T::DELAY_TRACK_SECS));
            sqlite3_bind_int   (st.s, 14, (int)Flag (r, GA_T::ROTATION_FOLLOWS_VELOCITY_FLAG));
            sqlite3_bind_int   (st.s, 15, (int)Flag (r, GA_T::STICK_TO_WALL_FLAG));
            sqlite3_bind_int   (st.s, 16, (int)Flag (r, GA_T::TRACK_TARGET_FLAG));
            sqlite3_bind_int   (st.s, 17, (int)Flag (r, GA_T::TRACK_TO_WORLD_POS_FLAG));
            sqlite3_bind_int   (st.s, 18, (int)Flag (r, GA_T::AIM_FROM_SOCKET_FLAG));
            st.step();
        });
    }

    // ---------- Deployables (0x013B) ----------
    void WalkDeployables(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db,
            "INSERT INTO asm_data_set_deployables ("
            " deployable_id, class_res_id, name_msg_id, name_msg_translated,"
            " desc_msg_id, desc_msg_translated, deployable_type_value_id, asm_id,"
            " health, device_id, death_fx_id, pickup_device_id, use_device_chance,"
            " vulnerable_to_attack_flags, loot_table_id, physical_type_value_id,"
            " artillery_target_type_value_id, destroy_on_owner_death_flag,"
            " delay_deploy_flag, require_ammo_to_live_flag, show_countdown_timer_flag"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_deployables_visibility_configs ("
            " deployable_id, visibility_config_id, child_deployable_id, proximity_distance"
            ") VALUES (?,?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        char nameBuf[4096], descBuf[4096];
        WalkArray(arr, [&](void* r) {
            uint32_t depId = Byte(r, GA_T::DEPLOYABLE_ID);
            uint32_t nameMsg = 0, descMsg = 0;
            // names/descriptions are translated via Translate(); msg_id isn't stored
            // separately on the row — pass 0 and store translated only. The game uses
            // CMarshal__Translate(ctx, NAME_MSG_ID, row) internally; we mirror.
            nameBuf[0] = '\0'; descBuf[0] = '\0';
            wchar_t* wn = CMarshal__Translate::CallOriginal(kTranslateContext, nullptr, GA_T::NAME_MSG_ID, r);
            if (wn) WideCharToMultiByte(CP_UTF8, 0, wn, -1, nameBuf, sizeof(nameBuf)-1, NULL, NULL);
            wchar_t* wd = CMarshal__Translate::CallOriginal(kTranslateContext, nullptr, GA_T::DESC_MSG_ID, r);
            if (wd) WideCharToMultiByte(CP_UTF8, 0, wd, -1, descBuf, sizeof(descBuf)-1, NULL, NULL);

            sqlite3_bind_int   (parent.s,  1, (int)depId);
            sqlite3_bind_int   (parent.s,  2, (int)Int32(r, GA_T::CLASS_RES_ID));
            sqlite3_bind_int   (parent.s,  3, (int)nameMsg);
            sqlite3_bind_text  (parent.s,  4, nameBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (parent.s,  5, (int)descMsg);
            sqlite3_bind_text  (parent.s,  6, descBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (parent.s,  7, (int)Byte (r, GA_T::DEPLOYABLE_TYPE_VALUE_ID));
            sqlite3_bind_int   (parent.s,  8, (int)Byte (r, GA_T::ASM_ID));
            sqlite3_bind_int   (parent.s,  9, (int)Byte (r, GA_T::HEALTH));
            sqlite3_bind_int   (parent.s, 10, (int)Byte (r, GA_T::DEVICE_ID));
            sqlite3_bind_int   (parent.s, 11, (int)Byte (r, GA_T::DEATH_FX_ID));
            sqlite3_bind_int   (parent.s, 12, (int)Byte (r, GA_T::PICKUP_DEVICE_ID));
            sqlite3_bind_double(parent.s, 13, (double)Float(r, GA_T::USE_DEVICE_CHANCE));
            sqlite3_bind_int   (parent.s, 14, (int)Byte (r, GA_T::VULNERABLE_TO_ATTACK_FLAGS));
            sqlite3_bind_int   (parent.s, 15, (int)Byte (r, GA_T::LOOT_TABLE_ID));
            sqlite3_bind_int   (parent.s, 16, (int)Byte (r, GA_T::PHYSICAL_TYPE_VALUE_ID));
            sqlite3_bind_int   (parent.s, 17, (int)Byte (r, GA_T::ARTILLERY_TARGET_TYPE_VALUE_ID));
            sqlite3_bind_int   (parent.s, 18, (int)Flag (r, GA_T::DESTROY_ON_OWNER_DEATH_FLAG));
            sqlite3_bind_int   (parent.s, 19, (int)Flag (r, GA_T::DELAY_DEPLOY_FLAG));
            sqlite3_bind_int   (parent.s, 20, (int)Flag (r, GA_T::REQUIRE_AMMO_TO_LIVE_FLAG));
            sqlite3_bind_int   (parent.s, 21, (int)Flag (r, GA_T::SHOW_COUNTDOWN_TIMER_FLAG));
            parent.step();

            // Nested DATA_SET_DEVICE_DEPLOY_SENSOR_CONFIG_LIST (0x0143)
            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_DEVICE_DEPLOY_SENSOR_CONFIG_LIST, &childArr);
            if (childArr) {
                WalkArray(childArr, [&](void* cr) {
                    sqlite3_bind_int   (child.s, 1, (int)depId);
                    sqlite3_bind_int   (child.s, 2, (int)Byte (cr, GA_T::VISIBILITY_CONFIG_ID));
                    sqlite3_bind_int   (child.s, 3, (int)Byte (cr, GA_T::DEPLOYABLE_ID));
                    sqlite3_bind_double(child.s, 4, (double)Float(cr, GA_T::PROXIMITY_DISTANCE));
                    child.step();
                });
            }
        });
    }

    // ---------- Destructibles (0x013F) ----------
    void WalkDestructibles(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_destructibles ("
            " destructible_object_id, name_msg_id, name_msg_translated,"
            " desc_msg_id, desc_msg_translated, asm_id, health"
            ") VALUES (?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nameBuf[4096], descBuf[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nameBuf, sizeof(nameBuf));
            Translate(GA_T::DESC_MSG_ID, r, descBuf, sizeof(descBuf));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::DESTRUCTIBLE_OBJECT_ID));
            sqlite3_bind_int (st.s, 2, 0);
            sqlite3_bind_text(st.s, 3, nameBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 4, 0);
            sqlite3_bind_text(st.s, 5, descBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 6, (int)Byte(r, GA_T::ASM_ID));
            sqlite3_bind_int (st.s, 7, (int)Byte(r, GA_T::HEALTH));
            st.step();
        });
    }

    // ---------- AssemblyMeshes (0x011C) ----------
    //
    // Per-row init FUN_1094b470 packs ~20 named flag bits into row+0x74. We
    // capture each as its own INTEGER (0/1) column for easier querying.
    void WalkAssemblyMeshes(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_assembly_meshes ("
            " asm_id, name, mesh_res_id, anim_tree_res_id, physics_asset_res_id,"
            " socket_res_id, aim_offset_profile_res_id, socket_offset_info_res_id,"
            " asm_mesh_type_value_id, scale, cull_distance,"
            " scale_3d_x, scale_3d_y, scale_3d_z,"
            " translation_x, translation_y, translation_z,"
            " rotator_pitch, rotator_yaw, rotator_roll,"
            " collision_height, collision_radius, collision_depth, crouch_height,"
            " hit_collision_height, hit_collision_radius,"
            " physics_weight, life_after_death_secs, destroyed_asm_id,"
            " material_res_group_id, race_material_parameter_id,"
            " accept_decals_flag, accept_decals_runtime_flag, accept_lights_flag,"
            " allow_approx_occlusion_flag, block_actors_flag,"
            " block_non_zero_extent_flag, block_rigid_body_flag,"
            " block_zero_extent_flag, cast_dynamic_shadow_flag, cast_shadow_flag,"
            " collide_actors_flag, force_dir_light_map_flag,"
            " has_physics_asset_inst_flag, is_female_flag,"
            " notify_rigid_body_collision_flag, only_owner_see_flag,"
            " owner_no_see_flag, update_joints_from_anim_flag"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,"
            "          ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nameBuf[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::NAME, nameBuf, sizeof(nameBuf));
            // (asm_id is actually the int32 ASM_ID = 0x40 — game reads int32 here)
            sqlite3_bind_int   (st.s,  1, (int)Int32(r, GA_T::ASM_ID));
            sqlite3_bind_text  (st.s,  2, nameBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (st.s,  3, (int)Int32(r, GA_T::MESH_RES_ID));
            sqlite3_bind_int   (st.s,  4, (int)Int32(r, GA_T::ANIM_TREE_RES_ID));
            sqlite3_bind_int   (st.s,  5, (int)Int32(r, GA_T::PHYSICS_ASSET_RES_ID));
            sqlite3_bind_int   (st.s,  6, (int)Int32(r, GA_T::SOCKET_RES_ID));
            sqlite3_bind_int   (st.s,  7, (int)Int32(r, GA_T::AIM_OFFSET_PROFILE_RES_ID));
            sqlite3_bind_int   (st.s,  8, (int)Int32(r, GA_T::SOCKET_OFFSET_INFO_RES_ID));
            sqlite3_bind_int   (st.s,  9, (int)Int32(r, GA_T::ASM_MESH_TYPE_VALUE_ID));
            sqlite3_bind_double(st.s, 10, (double)Float(r, GA_T::SCALE));
            sqlite3_bind_double(st.s, 11, (double)Float(r, GA_T::CULL_DISTANCE));
            sqlite3_bind_double(st.s, 12, (double)Float(r, GA_T::SCALE_3D_X));
            sqlite3_bind_double(st.s, 13, (double)Float(r, GA_T::SCALE_3D_Y));
            sqlite3_bind_double(st.s, 14, (double)Float(r, GA_T::SCALE_3D_Z));
            sqlite3_bind_double(st.s, 15, (double)Float(r, GA_T::TRANSLATION_X));
            sqlite3_bind_double(st.s, 16, (double)Float(r, GA_T::TRANSLATION_Y));
            sqlite3_bind_double(st.s, 17, (double)Float(r, GA_T::TRANSLATION_Z));
            sqlite3_bind_int   (st.s, 18, (int)Byte (r, GA_T::ROTATOR_PITCH));
            sqlite3_bind_int   (st.s, 19, (int)Byte (r, GA_T::ROTATOR_YAW));
            sqlite3_bind_int   (st.s, 20, (int)Byte (r, GA_T::ROTATOR_ROLL));
            sqlite3_bind_double(st.s, 21, (double)Float(r, GA_T::COLLISION_HEIGHT));
            sqlite3_bind_double(st.s, 22, (double)Float(r, GA_T::COLLISION_RADIUS));
            sqlite3_bind_double(st.s, 23, (double)Float(r, GA_T::COLLISION_DEPTH));
            sqlite3_bind_double(st.s, 24, (double)Float(r, GA_T::CROUCH_HEIGHT));
            sqlite3_bind_double(st.s, 25, (double)Float(r, GA_T::HIT_COLLISION_HEIGHT));
            sqlite3_bind_double(st.s, 26, (double)Float(r, GA_T::HIT_COLLISION_RADIUS));
            sqlite3_bind_double(st.s, 27, (double)Float(r, GA_T::PHYSICS_WEIGHT));
            sqlite3_bind_double(st.s, 28, (double)Float(r, GA_T::LIFE_AFTER_DEATH_SECS));
            sqlite3_bind_int   (st.s, 29, (int)Byte (r, GA_T::DESTROYED_ASM_ID));
            sqlite3_bind_int   (st.s, 30, (int)Byte (r, GA_T::MATERIAL_RES_GROUP_ID));
            sqlite3_bind_int   (st.s, 31, (int)Byte (r, GA_T::RACE_MATERIAL_PARAMETER_ID));
            sqlite3_bind_int   (st.s, 32, (int)Flag (r, GA_T::ACCEPT_DECALS_FLAG));
            sqlite3_bind_int   (st.s, 33, (int)Flag (r, GA_T::ACCEPT_DECALS_RUNTIME_FLAG));
            sqlite3_bind_int   (st.s, 34, (int)Flag (r, GA_T::ACCEPT_LIGHTS_FLAG));
            sqlite3_bind_int   (st.s, 35, (int)Flag (r, GA_T::ALLOW_APPROX_OCCLUSION_FLAG));
            sqlite3_bind_int   (st.s, 36, (int)Flag (r, GA_T::BLOCK_ACTORS_FLAG));
            sqlite3_bind_int   (st.s, 37, (int)Flag (r, GA_T::BLOCK_NON_ZERO_EXTENT_FLAG));
            sqlite3_bind_int   (st.s, 38, (int)Flag (r, GA_T::BLOCK_RIGID_BODY_FLAG));
            sqlite3_bind_int   (st.s, 39, (int)Flag (r, GA_T::BLOCK_ZERO_EXTENT_FLAG));
            sqlite3_bind_int   (st.s, 40, (int)Flag (r, GA_T::CAST_DYNAMIC_SHADOW_FLAG));
            sqlite3_bind_int   (st.s, 41, (int)Flag (r, GA_T::CAST_SHADOW_FLAG));
            sqlite3_bind_int   (st.s, 42, (int)Flag (r, GA_T::COLLIDE_ACTORS_FLAG));
            sqlite3_bind_int   (st.s, 43, (int)Flag (r, GA_T::FORCE_DIR_LIGHT_MAP_FLAG));
            sqlite3_bind_int   (st.s, 44, (int)Flag (r, GA_T::HAS_PHYSICS_ASSET_INST_FLAG));
            sqlite3_bind_int   (st.s, 45, (int)Flag (r, GA_T::IS_FEMALE_FLAG));
            sqlite3_bind_int   (st.s, 46, (int)Flag (r, GA_T::NOTIFY_RIGID_BODY_COLLISION_FLAG));
            sqlite3_bind_int   (st.s, 47, (int)Flag (r, GA_T::ONLY_OWNER_SEE_FLAG));
            sqlite3_bind_int   (st.s, 48, (int)Flag (r, GA_T::OWNER_NO_SEE_FLAG));
            sqlite3_bind_int   (st.s, 49, (int)Flag (r, GA_T::UPDATE_JOINTS_FROM_ANIM_FLAG));
            st.step();
        });
    }

    // ---------- FX (0x019F) ----------
    //
    // Field 0x370 (NAME) is read by the per-row init (FUN_1094a800) via
    // get_int32_t and resolved through DAT_119a1c80 — i.e. it's a
    // resource-id, not a wchar string. Storing as `name_res_id` (INTEGER)
    // lets consumers JOIN against asm_data_set_resources to recover the
    // human-readable name. The earlier `name` column populated by
    // GetWcharName likely held empty strings.
    void WalkFx(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_fx ("
            " fx_id, name_res_id, priority_value_id, mic_res_id, transition_sec"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)Byte (r, GA_T::FX_ID));
            sqlite3_bind_int   (st.s, 2, (int)Int32(r, GA_T::NAME));
            sqlite3_bind_int   (st.s, 3, (int)Byte (r, GA_T::PRIORITY_VALUE_ID));
            sqlite3_bind_int   (st.s, 4, (int)Int32(r, GA_T::MIC_RES_ID));
            sqlite3_bind_double(st.s, 5, (double)Float(r, GA_T::TRANSITION_SEC));
            st.step();
        });
    }

    // ---------- MaterialResGroups (0x0177) ----------
    void WalkMaterialResGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_material_res_groups (material_res_group_id, name) VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::NAME, nb, sizeof(nb));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::MATERIAL_RES_GROUP_ID));
            sqlite3_bind_text(st.s, 2, nb, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- MaterialParameters (0x0175) ----------
    void WalkMaterialParameters(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_material_parameters ("
            " material_parameter_id, param_type_value_id,"
            " linear_color_r, linear_color_g, linear_color_b, linear_color_a"
            ") VALUES (?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)Byte (r, GA_T::MATERIAL_PARAMETER_ID));
            sqlite3_bind_int   (st.s, 2, (int)Byte (r, GA_T::PARAM_TYPE_VALUE_ID));
            sqlite3_bind_double(st.s, 3, (double)Float(r, GA_T::LINEAR_COLOR_R));
            sqlite3_bind_double(st.s, 4, (double)Float(r, GA_T::LINEAR_COLOR_G));
            sqlite3_bind_double(st.s, 5, (double)Float(r, GA_T::LINEAR_COLOR_B));
            sqlite3_bind_double(st.s, 6, (double)Float(r, GA_T::LINEAR_COLOR_A));
            st.step();
        });
    }

    // ---------- VisibilityConfigs (0x01C5) ----------
    void WalkVisibilityConfigs(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_visibility_configs (visibility_config_id, display_flags, see_flags) VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Byte(r, GA_T::VISIBILITY_CONFIG_ID));
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::DISPLAY_FLAGS));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::SEE_FLAGS));
            st.step();
        });
    }

    // ---------- Icons (0x015E) ----------
    void WalkIcons(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_icons (icon_id, texture_res_id, icon_index) VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Byte(r, GA_T::ICON_ID));
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::TEXTURE_RES_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::ICON_INDEX));
            st.step();
        });
    }

    // ---------- Properties (0x018D) ----------
    void WalkProperties(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_properties ("
            " prop_id, name, prop_type_value_id, prop_uom_value_id,"
            " ui_name_msg_id, ui_code"
            ") VALUES (?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], cb[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::NAME,    nb, sizeof(nb));
            GetWcharName(r, GA_T::UI_CODE, cb, sizeof(cb));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::PROP_ID));
            sqlite3_bind_text(st.s, 2, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 3, (int)Byte(r, GA_T::PROP_TYPE_VALUE_ID));
            sqlite3_bind_int (st.s, 4, (int)Byte(r, GA_T::PROP_UOM_VALUE_ID));
            sqlite3_bind_int (st.s, 5, (int)Byte(r, GA_T::UI_NAME_MSG_ID));
            sqlite3_bind_text(st.s, 6, cb, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- HexBuildings (0x0158) ----------
    void WalkHexBuildings(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_hex_buildings ("
            " hex_building_id, name_msg_id, desc_msg_id,"
            " building_type_value_id, building_subtype_value_id, map_game_id,"
            " mesh_res_id, bonus_value_id, price, output_value,"
            " defensive_cargo_space, defensive_player_capacity, icon_id, tech_level_value_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s,  1, (int)Byte(r, GA_T::HEX_BUILDING_ID));
            sqlite3_bind_int(st.s,  2, (int)Byte(r, GA_T::NAME_MSG_ID));
            sqlite3_bind_int(st.s,  3, (int)Byte(r, GA_T::DESC_MSG_ID));
            sqlite3_bind_int(st.s,  4, (int)Byte(r, GA_T::BUILDING_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s,  5, (int)Byte(r, GA_T::BUILDING_SUBTYPE_VALUE_ID));
            sqlite3_bind_int(st.s,  6, (int)Byte(r, GA_T::MAP_GAME_ID));
            sqlite3_bind_int(st.s,  7, (int)Byte(r, GA_T::MESH_RES_ID));
            sqlite3_bind_int(st.s,  8, (int)Byte(r, GA_T::BONUS_VALUE_ID));
            sqlite3_bind_int(st.s,  9, (int)Byte(r, GA_T::PRICE));
            sqlite3_bind_int(st.s, 10, (int)Byte(r, GA_T::OUTPUT_VALUE));
            sqlite3_bind_int(st.s, 11, (int)Byte(r, GA_T::DEFENSIVE_CARGO_SPACE));
            sqlite3_bind_int(st.s, 12, (int)Byte(r, GA_T::DEFENSIVE_PLAYER_CAPACITY));
            sqlite3_bind_int(st.s, 13, (int)Byte(r, GA_T::ICON_ID));
            sqlite3_bind_int(st.s, 14, (int)Byte(r, GA_T::TECH_LEVEL_VALUE_ID));
            st.step();
        });
    }

    // ---------- Objectives (0x017B) + nested props ----------
    void WalkObjectives(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db,
            "INSERT INTO asm_data_set_objectives ("
            " objective_id, class_res_id, name_msg_id, name_msg_translated,"
            " asm_id, proximity_distance, proximity_height, text_msg_id,"
            " bot_id, objective_type_value_id, icon_id, desc_msg_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_objectives_props (objective_id, prop_id, value) VALUES (?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        char nameBuf[4096];
        WalkArray(arr, [&](void* r) {
            uint32_t objId = Byte(r, GA_T::OBJECTIVE_ID);
            Translate(GA_T::NAME_MSG_ID, r, nameBuf, sizeof(nameBuf));
            sqlite3_bind_int   (parent.s,  1, (int)objId);
            sqlite3_bind_int   (parent.s,  2, (int)Int32(r, GA_T::CLASS_RES_ID));
            sqlite3_bind_int   (parent.s,  3, 0);
            sqlite3_bind_text  (parent.s,  4, nameBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (parent.s,  5, (int)Byte (r, GA_T::ASM_ID));
            sqlite3_bind_double(parent.s,  6, (double)Float(r, GA_T::PROXIMITY_DISTANCE));
            sqlite3_bind_double(parent.s,  7, (double)Float(r, GA_T::PROXIMITY_HEIGHT));
            sqlite3_bind_int   (parent.s,  8, (int)Byte (r, GA_T::TEXT_MSG_ID));
            sqlite3_bind_int   (parent.s,  9, (int)Byte (r, GA_T::BOT_ID));
            sqlite3_bind_int   (parent.s, 10, (int)Byte (r, GA_T::OBJECTIVE_TYPE_VALUE_ID));
            sqlite3_bind_int   (parent.s, 11, (int)Byte (r, GA_T::ICON_ID));
            sqlite3_bind_int   (parent.s, 12, (int)Byte (r, GA_T::DESC_MSG_ID));
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_OBJECTIVE_PROPS, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int   (child.s, 1, (int)objId);
                sqlite3_bind_int   (child.s, 2, (int)Byte (cr, GA_T::PROP_ID));
                sqlite3_bind_double(child.s, 3, (double)Float(cr, GA_T::VALUE));
                child.step();
            });
        });
    }

    // ---------- XpLevels (0x01CB) ----------
    void WalkXpLevels(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_xp_levels ("
            " xp_level_id, level, xp_value, skill_points, device_points, protection_points"
            ") VALUES (?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Byte(r, GA_T::XP_LEVEL_ID));
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::LEVEL));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::XP_VALUE));
            sqlite3_bind_int(st.s, 4, (int)Byte(r, GA_T::SKILL_POINTS));
            sqlite3_bind_int(st.s, 5, (int)Byte(r, GA_T::DEVICE_POINTS));
            sqlite3_bind_int(st.s, 6, (int)Byte(r, GA_T::PROTECTION_POINTS));
            st.step();
        });
    }

    // ---------- DeviceSlotUnlockGroups (0x0148) + child list ----------
    void WalkDeviceSlotUnlockGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db,
            "INSERT INTO asm_data_set_device_slot_unlock_groups (device_slot_unlock_group_id, name) VALUES (?,?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_device_slot_unlock_list ("
            " device_slot_unlock_group_id, slot_value_id, required_xp_level_id, unlock_type_value_id"
            ") VALUES (?,?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            uint32_t gid = Byte(r, GA_T::DEVICE_SLOT_UNLOCK_GROUP_ID);
            GetWcharName(r, GA_T::NAME, nb, sizeof(nb));
            sqlite3_bind_int (parent.s, 1, (int)gid);
            sqlite3_bind_text(parent.s, 2, nb, -1, SQLITE_TRANSIENT);
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_DEVICE_SLOT_UNLOCK_LIST, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)gid);
                sqlite3_bind_int(child.s, 2, (int)Byte(cr, GA_T::SLOT_VALUE_ID));
                sqlite3_bind_int(child.s, 3, (int)Byte(cr, GA_T::REQUIRED_XP_LEVEL_ID));
                sqlite3_bind_int(child.s, 4, (int)Byte(cr, GA_T::UNLOCK_TYPE_VALUE_ID));
                child.step();
            });
        });
    }

    // ---------- UiVolumes (0x01C3) ----------
    void WalkUiVolumes(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_ui_volumes ("
            " ui_volume_id, volume_type_value_id,"
            " name_msg_id, name_msg_translated,"
            " summary_msg_id, summary_msg_translated,"
            " desc_msg_id, desc_msg_translated,"
            " parent_help_ui_volume_id, use_msg_id, ui_scene_res_id,"
            " map_game_id, loot_table_id, queue_selection_list_id, quest_group_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], sb[4096], db1[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID,    r, nb, sizeof(nb));
            Translate(GA_T::SUMMARY_MSG_ID, r, sb, sizeof(sb));
            Translate(GA_T::DESC_MSG_ID,    r, db1, sizeof(db1));
            sqlite3_bind_int (st.s,  1, (int)Int32(r, GA_T::UI_VOLUME_ID));
            sqlite3_bind_int (st.s,  2, (int)Int32(r, GA_T::VOLUME_TYPE_VALUE_ID));
            sqlite3_bind_int (st.s,  3, 0);
            sqlite3_bind_text(st.s,  4, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  5, 0);
            sqlite3_bind_text(st.s,  6, sb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  7, 0);
            sqlite3_bind_text(st.s,  8, db1, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  9, (int)Int32(r, GA_T::PARENT_HELP_UI_VOLUME_ID));
            sqlite3_bind_int (st.s, 10, (int)Int32(r, GA_T::USE_MSG_ID));
            sqlite3_bind_int (st.s, 11, (int)Int32(r, GA_T::UI_SCENE_RES_ID));
            sqlite3_bind_int (st.s, 12, (int)Int32(r, GA_T::MAP_GAME_ID));
            sqlite3_bind_int (st.s, 13, (int)Int32(r, GA_T::LOOT_TABLE_ID));
            sqlite3_bind_int (st.s, 14, (int)Int32(r, GA_T::QUEUE_SELECTION_LIST_ID));
            sqlite3_bind_int (st.s, 15, (int)Int32(r, GA_T::QUEST_GROUP_ID));
            st.step();
        });
    }

    // ---------- Quests (0x018E) ----------
    void WalkQuests(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_quests ("
            " quest_id, quest_type_value_id, level_min, repeatable_flag,"
            " repeatable_cooldown_type_value_id, reward_is_pooled_flag,"
            " name_msg_id, name_msg_translated, desc_msg_id, desc_msg_translated,"
            " short_desc_msg_id, turnin_ui_volume_id, turnin_msg_id,"
            " completed_msg_id, xp_reward_value, currency_reward_value,"
            " loot_table_id, field_0x613"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], db1[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb,  sizeof(nb));
            Translate(GA_T::DESC_MSG_ID, r, db1, sizeof(db1));
            sqlite3_bind_int (st.s,  1, (int)Int32(r, GA_T::QUEST_ID));
            sqlite3_bind_int (st.s,  2, (int)Byte (r, GA_T::QUEST_TYPE_VALUE_ID));
            sqlite3_bind_int (st.s,  3, (int)Byte (r, GA_T::LEVEL_MIN));
            sqlite3_bind_int (st.s,  4, (int)Flag (r, GA_T::REPEATABLE_FLAG));
            sqlite3_bind_int (st.s,  5, (int)Byte (r, GA_T::REPEATABLE_COOLDOWN_TYPE_VALUE_ID));
            sqlite3_bind_int (st.s,  6, (int)Flag (r, GA_T::REWARD_IS_POOLED_FLAG));
            sqlite3_bind_int (st.s,  7, (int)Byte (r, GA_T::NAME_MSG_ID));
            sqlite3_bind_text(st.s,  8, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  9, (int)Byte (r, GA_T::DESC_MSG_ID));
            sqlite3_bind_text(st.s, 10, db1, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 11, (int)Byte (r, GA_T::SHORT_DESC_MSG_ID));
            sqlite3_bind_int (st.s, 12, (int)Byte (r, GA_T::TURNIN_UI_VOLUME_ID));
            sqlite3_bind_int (st.s, 13, (int)Byte (r, GA_T::TURNIN_MSG_ID));
            sqlite3_bind_int (st.s, 14, (int)Byte (r, GA_T::COMPLETED_MSG_ID));
            sqlite3_bind_int (st.s, 15, (int)Byte (r, GA_T::XP_REWARD_VALUE));
            sqlite3_bind_int (st.s, 16, (int)Byte (r, GA_T::CURRENCY_REWARD_VALUE));
            sqlite3_bind_int (st.s, 17, (int)Byte (r, GA_T::LOOT_TABLE_ID));
            sqlite3_bind_int (st.s, 18, (int)Byte (r, 0x613));
            st.step();
        });
    }

    // ---------- QuestGroups (0x018F) + lists ----------
    void WalkQuestGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db,
            "INSERT INTO asm_data_set_quest_groups (quest_group_id, name_msg_id) VALUES (?,?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_quest_group_lists ("
            " quest_group_id, quest_id, disregard_quest_flag, quest_status_type_value_id"
            ") VALUES (?,?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t gid = Int32(r, GA_T::QUEST_GROUP_ID);
            sqlite3_bind_int(parent.s, 1, (int)gid);
            sqlite3_bind_int(parent.s, 2, (int)Byte(r, GA_T::NAME_MSG_ID));
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_QUEST_GROUP_LISTS, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)gid);
                sqlite3_bind_int(child.s, 2, (int)Byte(cr, GA_T::QUEST_ID));
                sqlite3_bind_int(child.s, 3, (int)Flag(cr, GA_T::DISREGARD_QUEST_FLAG));
                sqlite3_bind_int(child.s, 4, (int)Int32(cr, GA_T::QUEST_STATUS_TYPE_VALUE_ID));
                child.step();
            });
        });
    }

    // ==================== INLINE MASTER DATA SETS ====================

    // ---------- Resources (0x0194) ----------
    void WalkResources(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_resources (res_id, name, res_type_value_id) VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::NAME, nb, sizeof(nb));
            sqlite3_bind_int (st.s, 1, (int)Int32(r, GA_T::RES_ID));
            sqlite3_bind_text(st.s, 2, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 3, (int)Int32(r, GA_T::RES_TYPE_VALUE_ID));
            st.step();
        });
    }

    // ---------- ValidValues (0x01C4) ----------
    void WalkValidValues(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_valid_values ("
            " valid_value_group_id, value_id, text_msg_id, text_msg_translated, sort_order"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char tb[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::TEXT_MSG_ID, r, tb, sizeof(tb));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::VALID_VALUE_GROUP_ID));
            sqlite3_bind_int (st.s, 2, (int)Byte(r, GA_T::VALUE_ID));
            sqlite3_bind_int (st.s, 3, (int)Byte(r, GA_T::TEXT_MSG_ID));
            sqlite3_bind_text(st.s, 4, tb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 5, (int)Byte(r, GA_T::SORT_ORDER));
            st.step();
        });
    }

    // ---------- ProductionDevices (0x0187) + children (0x014B) ----------
    void WalkProductionDevices(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db, "INSERT INTO asm_data_set_production_devices (device_id) VALUES (?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_production_devices_used_resources ("
            " device_id, res_id, always_load_on_server_flag"
            ") VALUES (?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t did = Byte(r, GA_T::DEVICE_ID);
            sqlite3_bind_int(parent.s, 1, (int)did);
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_DEVICE_USED_RESOURCES, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)did);
                sqlite3_bind_int(child.s, 2, (int)Int32(cr, GA_T::RES_ID));
                sqlite3_bind_int(child.s, 3, (int)Flag (cr, GA_T::ALWAYS_LOAD_ON_SERVER_FLAG));
                child.step();
            });
        });
    }

    // ---------- ProductionFlairs (0x0188) + children (0x0151) ----------
    void WalkProductionFlairs(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db, "INSERT INTO asm_data_set_production_flairs (item_id) VALUES (?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_production_flairs_used_resources ("
            " item_id, res_id, always_load_on_server_flag"
            ") VALUES (?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t iid = Byte(r, GA_T::ITEM_ID);
            sqlite3_bind_int(parent.s, 1, (int)iid);
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_FLAIR_USED_RESOURCES, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)iid);
                sqlite3_bind_int(child.s, 2, (int)Int32(cr, GA_T::RES_ID));
                sqlite3_bind_int(child.s, 3, (int)Flag (cr, GA_T::ALWAYS_LOAD_ON_SERVER_FLAG));
                child.step();
            });
        });
    }

    // ---------- PetUsedResources (0x05BE) ----------
    void WalkPetUsedResources(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_pet_used_resources (item_id, res_id) VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Byte (r, GA_T::ITEM_ID));
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::RES_ID));
            st.step();
        });
    }

    // ---------- ImpactFx (0x015F) ----------
    void WalkImpactFx(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_impact_fx ("
            " impact_fx_group_id, material_type_res_id, decal_material_res_id,"
            " decal_height, decal_width, sound_res_id, particle_res_id, fx_id, flag_0x1d9"
            ") VALUES (?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)Int32(r, GA_T::IMPACT_FX_GROUP_ID));
            sqlite3_bind_int   (st.s, 2, (int)Int32(r, GA_T::MATERIAL_TYPE_RES_ID));
            sqlite3_bind_int   (st.s, 3, (int)Int32(r, GA_T::DECAL_MATERIAL_RES_ID));
            sqlite3_bind_double(st.s, 4, (double)Float(r, GA_T::DECAL_HEIGHT));
            sqlite3_bind_double(st.s, 5, (double)Float(r, GA_T::DECAL_WIDTH));
            sqlite3_bind_int   (st.s, 6, (int)Int32(r, GA_T::SOUND_RES_ID));
            sqlite3_bind_int   (st.s, 7, (int)Int32(r, GA_T::PARTICLE_RES_ID));
            sqlite3_bind_int   (st.s, 8, (int)Int32(r, GA_T::FX_ID));
            sqlite3_bind_int   (st.s, 9, (int)Flag (r, GA_T::DECAL_RANDOM_ROTATION_FLAG));
            st.step();
        });
    }

    // ---------- SkillGroupRanks (0x019A) ----------
    //
    // Master loader processes each rank row by reading SKILL_GROUP_ID + SKILL_ID
    // inline, then handing the row to FUN_1094c160 → CAmSkillRank vtable[1]
    // (FUN_1094c250) which reads 8 more scalar fields. The earlier walker only
    // stored the two parent-key fields; v22 captures the full row.
    void WalkSkillGroupRanks(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_skill_group_ranks ("
            "  skill_group_id, skill_id, rank_id, name_msg_translated,"
            "  desc_msg_translated, rank, training_map_game_id,"
            "  desc_texture_res_id, required_xp_level_id, auto_allocate_flag"
            ") VALUES (?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nameBuf[4096], descBuf[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nameBuf, sizeof(nameBuf));
            Translate(GA_T::DESC_MSG_ID, r, descBuf, sizeof(descBuf));
            sqlite3_bind_int (st.s,  1, (int)Byte (r, GA_T::SKILL_GROUP_ID));
            sqlite3_bind_int (st.s,  2, (int)Byte (r, GA_T::SKILL_ID));
            sqlite3_bind_int (st.s,  3, (int)Byte (r, GA_T::RANK_ID));
            sqlite3_bind_text(st.s,  4, nameBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(st.s,  5, descBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  6, (int)Byte (r, GA_T::RANK));
            sqlite3_bind_int (st.s,  7, (int)Byte (r, GA_T::TRAINING_MAP_GAME_ID));
            sqlite3_bind_int (st.s,  8, (int)Int32(r, GA_T::DESC_TEXTURE_RES_ID));
            sqlite3_bind_int (st.s,  9, (int)Byte (r, GA_T::REQUIRED_XP_LEVEL_ID));
            sqlite3_bind_int (st.s, 10, (int)Flag (r, GA_T::AUTO_ALLOCATE_FLAG));
            st.step();
        });
    }

    // ---------- AchievementGroups (0x010E) ----------
    void WalkAchievementGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_achievement_groups ("
            " achievement_group_id, group_type_value_id,"
            " name_msg_id, name_msg_translated, parent_achievement_group_id"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb, sizeof(nb));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::ACHIEVEMENT_GROUP_ID));
            sqlite3_bind_int (st.s, 2, (int)Byte(r, GA_T::GROUP_TYPE_VALUE_ID));
            sqlite3_bind_int (st.s, 3, (int)Byte(r, GA_T::NAME_MSG_ID));
            sqlite3_bind_text(st.s, 4, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 5, (int)Byte(r, GA_T::PARENT_ACHIEVEMENT_GROUP_ID));
            st.step();
        });
    }

    // ---------- Achievements (0x010D) ----------
    void WalkAchievements(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_achievements ("
            " achievement_id, achievement_group_id,"
            " name_msg_id, name_msg_translated,"
            " desc_msg_id, desc_msg_translated,"
            " icon_id, loot_table_id, title_msg_id, points_earned"
            ") VALUES (?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], db1[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb,  sizeof(nb));
            Translate(GA_T::DESC_MSG_ID, r, db1, sizeof(db1));
            sqlite3_bind_int (st.s,  1, (int)Byte (r, GA_T::ACHIEVEMENT_ID));
            sqlite3_bind_int (st.s,  2, (int)Byte (r, GA_T::ACHIEVEMENT_GROUP_ID));
            sqlite3_bind_int (st.s,  3, (int)Byte (r, GA_T::NAME_MSG_ID));
            sqlite3_bind_text(st.s,  4, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  5, (int)Byte (r, GA_T::DESC_MSG_ID));
            sqlite3_bind_text(st.s,  6, db1, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  7, (int)Byte (r, GA_T::ICON_ID));
            sqlite3_bind_int (st.s,  8, (int)Int32(r, GA_T::LOOT_TABLE_ID));
            sqlite3_bind_int (st.s,  9, (int)Byte (r, GA_T::TITLE_MSG_ID));
            sqlite3_bind_int (st.s, 10, (int)Byte (r, GA_T::POINTS_EARNED));
            st.step();
        });
    }

    // ---------- Tips (0x01C1) ----------
    void WalkTips(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_tips (game_tip_id, tip_msg_id, tip_msg_translated) VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char tb[4096];
        WalkArray(arr, [&](void* r) {
            uint32_t tipId = Int32(r, GA_T::TIP_MSG_ID);
            Translate(tipId, r, tb, sizeof(tb));
            sqlite3_bind_int (st.s, 1, (int)Int32(r, GA_T::GAME_TIP_ID));
            sqlite3_bind_int (st.s, 2, (int)tipId);
            sqlite3_bind_text(st.s, 3, tb, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- EventRewards (0x0150) ----------
    void WalkEventRewards(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_event_rewards ("
            " event_reward_id, reward_event_value_id, xp_reward_value,"
            " token_reward_value, currency_reward_value, difficulty_value_id, process_order"
            ") VALUES (?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)Byte (r, GA_T::EVENT_REWARD_ID));
            sqlite3_bind_int   (st.s, 2, (int)Byte (r, GA_T::REWARD_EVENT_VALUE_ID));
            sqlite3_bind_double(st.s, 3, (double)Float(r, GA_T::XP_REWARD_VALUE));
            sqlite3_bind_double(st.s, 4, (double)Float(r, GA_T::TOKEN_REWARD_VALUE));
            sqlite3_bind_double(st.s, 5, (double)Float(r, GA_T::CURRENCY_REWARD_VALUE));
            sqlite3_bind_int   (st.s, 6, (int)Byte (r, GA_T::DIFFICULTY_VALUE_ID));
            sqlite3_bind_int   (st.s, 7, (int)Byte (r, GA_T::PROCESS_ORDER));
            st.step();
        });
    }

    // ---------- PlayerUsedResources (0x0185) ----------
    void WalkPlayerUsedResources(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_player_used_resources (res_id, always_load_on_server_flag) VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Int32(r, GA_T::RES_ID));
            sqlite3_bind_int(st.s, 2, (int)Flag (r, GA_T::ALWAYS_LOAD_ON_SERVER_FLAG));
            st.step();
        });
    }

    // ---------- ProductionMapGameList (0x0189) + children (0x0153) ----------
    void WalkProductionMapGameList(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db, "INSERT INTO asm_data_set_production_map_game_list (map_game_id) VALUES (?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_game_used_resources ("
            " map_game_id, res_id, always_load_on_server_flag"
            ") VALUES (?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t mgid = Byte(r, GA_T::MAP_GAME_ID);
            sqlite3_bind_int(parent.s, 1, (int)mgid);
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_GAME_USED_RESOURCES, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)mgid);
                sqlite3_bind_int(child.s, 2, (int)Int32(cr, GA_T::RES_ID));
                sqlite3_bind_int(child.s, 3, (int)Flag (cr, GA_T::ALWAYS_LOAD_ON_SERVER_FLAG));
                child.step();
            });
        });
    }

    // ---------- DeviceUsedAnimsets (0x014A) ----------
    void WalkDeviceUsedAnimsets(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_device_used_animsets (res_id, always_load_on_server_flag) VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Int32(r, GA_T::RES_ID));
            sqlite3_bind_int(st.s, 2, (int)Flag (r, GA_T::ALWAYS_LOAD_ON_SERVER_FLAG));
            st.step();
        });
    }

    // ---------- HardcodedResources (0x0154) ----------
    void WalkHardcodedResources(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_hardcoded_resources (res_id) VALUES (?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Int32(r, GA_T::RES_ID));
            st.step();
        });
    }

    // ---------- WorldObjects (0x01C8) + children (0x01C9) ----------
    void WalkWorldObjects(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db, "INSERT INTO asm_data_set_world_objects (asm_id) VALUES (?)");
        Stmt child(db, "INSERT INTO asm_data_set_world_object_resources (asm_id, res_id) VALUES (?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t aid = Byte(r, GA_T::ASM_ID);
            sqlite3_bind_int(parent.s, 1, (int)aid);
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_WORLD_OBJECT_RESOURCES, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)aid);
                sqlite3_bind_int(child.s, 2, (int)Int32(cr, GA_T::RES_ID));
                child.step();
            });
        });
    }

    // ---------- ObjectiveBots (0x017C) + children (0x017D) ----------
    void WalkObjectiveBots(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db, "INSERT INTO asm_data_set_objective_bots (objective_id) VALUES (?)");
        Stmt child(db, "INSERT INTO asm_data_set_objective_bot_resources (objective_id, res_id) VALUES (?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t oid = Byte(r, GA_T::OBJECTIVE_ID);
            sqlite3_bind_int(parent.s, 1, (int)oid);
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_OBJECTIVE_BOT_RESOURCES, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)oid);
                sqlite3_bind_int(child.s, 2, (int)Int32(cr, GA_T::RES_ID));
                child.step();
            });
        });
    }

    // ---------- SoundCues (0x019E) ----------
    void WalkSoundCues(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_sound_cues ("
            " audio_group_id, sound_cue_id, sound_cue_res_id, sound_cue_name"
            ") VALUES (?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::SOUND_CUE_NAME, nb, sizeof(nb));
            sqlite3_bind_int (st.s, 1, (int)Byte (r, GA_T::AUDIO_GROUP_ID));
            sqlite3_bind_int (st.s, 2, (int)Byte (r, GA_T::SOUND_CUE_ID));
            sqlite3_bind_int (st.s, 3, (int)Int32(r, GA_T::SOUND_CUE_RES_ID));
            sqlite3_bind_text(st.s, 4, nb, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- Blueprints (0x0120, virtual vtable[2] -> FUN_1094f670) ----------
    void WalkBlueprints(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_blueprints ("
            " blueprint_id, created_item_id, generation_value_id, durability,"
            " quantity, loot_table_group_id, override_name_msg_id, destroy_on_use_flag"
            ") VALUES (?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Int32(r, GA_T::BLUEPRINT_ID));
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::CREATED_ITEM_ID));
            sqlite3_bind_int(st.s, 3, (int)Int32(r, GA_T::GENERATION_VALUE_ID));
            sqlite3_bind_int(st.s, 4, (int)Int32(r, GA_T::DURABILITY));
            sqlite3_bind_int(st.s, 5, (int)Int32(r, GA_T::QUANTITY));
            sqlite3_bind_int(st.s, 6, (int)Int32(r, GA_T::LOOT_TABLE_GROUP_ID));
            sqlite3_bind_int(st.s, 7, (int)Byte (r, GA_T::OVERRIDE_NAME_MSG_ID));
            sqlite3_bind_int(st.s, 8, (int)Flag (r, GA_T::DESTROY_ON_USE_FLAG));
            st.step();
        });
    }

    // ---------- LootTables (0x016A, virtual vtable[1] -> FUN_1093db20) ----------
    void WalkLootTables(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_loot_tables ("
            " loot_table_id, name_msg_translated, desc_msg_translated,"
            " loot_table_type_value_id, system_craft_skill_level"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], db1[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb,  sizeof(nb));
            Translate(GA_T::DESC_MSG_ID, r, db1, sizeof(db1));
            sqlite3_bind_int (st.s, 1, (int)Int32(r, GA_T::LOOT_TABLE_ID));
            sqlite3_bind_text(st.s, 2, nb,  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(st.s, 3, db1, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 4, (int)Byte(r, GA_T::LOOT_TABLE_TYPE_VALUE_ID));
            sqlite3_bind_int (st.s, 5, (int)Byte(r, GA_T::SYSTEM_CRAFT_SKILL_LEVEL));
            st.step();
        });
    }

    // ---------- LootTableGroups (0x0575, virtual vtable[1] -> FUN_1094cdf0) ----------
    void WalkLootTableGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_loot_table_groups ("
            " loot_table_group_id, name_msg_translated, desc_msg_translated"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], db1[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb,  sizeof(nb));
            Translate(GA_T::DESC_MSG_ID, r, db1, sizeof(db1));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::LOOT_TABLE_GROUP_ID));
            sqlite3_bind_text(st.s, 2, nb,  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(st.s, 3, db1, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- SkillGroupSets (0x019B, virtual vtable[1] -> CGameClient__Skills @ 0x1094bf20) + nested group list ----------
    void WalkSkillGroupSets(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt parent(db,
            "INSERT INTO asm_data_set_skill_group_sets ("
            " skill_group_set_id, name, total_points_available, bot_id"
            ") VALUES (?,?,?,?)");
        Stmt child(db,
            "INSERT INTO asm_data_set_skill_group_set_groups ("
            " skill_group_set_id, skill_group_id, primary_flag"
            ") VALUES (?,?,?)");
        if (!parent.s || !child.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            uint32_t setId = Byte(r, GA_T::SKILL_GROUP_SET_ID);
            GetWcharName(r, GA_T::NAME, nb, sizeof(nb));
            sqlite3_bind_int (parent.s, 1, (int)setId);
            sqlite3_bind_text(parent.s, 2, nb, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (parent.s, 3, (int)Byte(r, GA_T::TOTAL_POINTS_AVAILABLE));
            sqlite3_bind_int (parent.s, 4, (int)Byte(r, GA_T::BOT_ID));
            parent.step();

            uint32_t childArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_SKILL_GROUP_SET_GROUPS, &childArr);
            if (childArr) WalkArray(childArr, [&](void* cr) {
                sqlite3_bind_int(child.s, 1, (int)setId);
                sqlite3_bind_int(child.s, 2, (int)Byte(cr, GA_T::SKILL_GROUP_ID));
                sqlite3_bind_int(child.s, 3, (int)Flag(cr, GA_T::PRIMARY_FLAG));
                child.step();
            });
        });
    }

    // ---------- SkillGroups (0x0199, virtual vtable[1] -> FUN_1094c000) ----------
    // Nested DATA_SET_SKILL_GROUP_SKILLS (0x019D) is itself virtual-dispatched; its
    // per-skill walker would need another vtable resolution pass. Captured at the
    // parent granularity for now.
    void WalkSkillGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db, "INSERT INTO asm_data_set_skill_groups (skill_group_id, name) VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::NAME, nb, sizeof(nb));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::SKILL_GROUP_ID));
            sqlite3_bind_text(st.s, 2, nb, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- SkillGroupSkills (0x019D, nested under SkillGroups; vtable[1] -> FUN_1093d680) ----------
    void WalkSkillGroupSkills(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t skillGroupId = CachedByte(GA_T::SKILL_GROUP_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_skill_group_skills ("
            " skill_group_id, skill_id, name_msg_translated, desc_msg_translated,"
            " prereq_group_points, prereq_skill_id, prereq_skill_points,"
            " sort_order, type_value_id, group_loc_x, group_loc_y, icon_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096], db1[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb,  sizeof(nb));
            Translate(GA_T::DESC_MSG_ID, r, db1, sizeof(db1));
            sqlite3_bind_int (st.s,  1, (int)skillGroupId);
            sqlite3_bind_int (st.s,  2, (int)Byte(r, GA_T::SKILL_ID));
            sqlite3_bind_text(st.s,  3, nb,  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(st.s,  4, db1, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s,  5, (int)Byte(r, GA_T::PREREQ_GROUP_POINTS));
            sqlite3_bind_int (st.s,  6, (int)Byte(r, GA_T::PREREQ_SKILL_ID));
            sqlite3_bind_int (st.s,  7, (int)Byte(r, GA_T::PREREQ_SKILL_POINTS));
            sqlite3_bind_int (st.s,  8, (int)Byte(r, GA_T::SORT_ORDER));
            sqlite3_bind_int (st.s,  9, (int)Byte(r, GA_T::TYPE_VALUE_ID));
            sqlite3_bind_int (st.s, 10, (int)Byte(r, GA_T::GROUP_LOC_X));
            sqlite3_bind_int (st.s, 11, (int)Byte(r, GA_T::GROUP_LOC_Y));
            sqlite3_bind_int (st.s, 12, (int)Byte(r, GA_T::ICON_ID));
            st.step();
        });
    }

    // ---------- SubSkills (0x01BD; vtable[1] -> FUN_1093d640) ----------
    void WalkSubSkills(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_sub_skills ("
            " sub_skill_id, skill_id, name_msg_translated"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nb[4096];
        WalkArray(arr, [&](void* r) {
            Translate(GA_T::NAME_MSG_ID, r, nb, sizeof(nb));
            sqlite3_bind_int (st.s, 1, (int)Byte(r, GA_T::SUB_SKILL_ID));
            sqlite3_bind_int (st.s, 2, (int)Byte(r, GA_T::SKILL_ID));
            sqlite3_bind_text(st.s, 3, nb, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    // ---------- BlueprintItems (0x0121, nested under Blueprints; vtable[1] -> FUN_1093f090) ----------
    void WalkBlueprintItems(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t blueprintId = CachedInt32(GA_T::BLUEPRINT_ID);
        Stmt st(db, "INSERT INTO asm_data_set_blueprint_items (blueprint_id, item_id, quantity) VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)blueprintId);
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::ITEM_ID));
            sqlite3_bind_int(st.s, 3, (int)Int32(r, GA_T::QUANTITY));
            st.step();
        });
    }

    // ---------- BlueprintItemMods (0x0122; vtable[1] -> FUN_1094b280) ----------
    void WalkBlueprintItemMods(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_blueprint_item_mods ("
            " blueprint_id, blueprint_mod_id, name_msg_id, quality_value_id, icon_id"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)Int32(r, GA_T::BLUEPRINT_ID));
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::BLUEPRINT_MOD_ID));
            sqlite3_bind_int(st.s, 3, (int)Int32(r, GA_T::NAME_MSG_ID));
            sqlite3_bind_int(st.s, 4, (int)Int32(r, GA_T::QUALITY_VALUE_ID));
            sqlite3_bind_int(st.s, 5, (int)Int32(r, GA_T::ICON_ID));
            st.step();
        });
    }

    // ---------- LootTableItems (0x016B, nested under LootTables; vtable[0] of item ctor -> FUN_1094fe80) ----------
    void WalkLootTableItems(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t lootTableId = CachedInt32(GA_T::LOOT_TABLE_ID);
        Stmt item(db,
            "INSERT INTO asm_data_set_loot_table_items ("
            " loot_table_id, loot_table_item_id, item_id, sub_loot_table_id,"
            " quantity, drop_chance, sort_order, quality_value_id, quality_ranking,"
            " max_quality_value_id, auto_create_flag, use_player_components_flag"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
        Stmt price(db,
            "INSERT INTO asm_data_set_loot_table_item_prices ("
            " loot_table_item_id, price, currency"
            ") VALUES (?,?,?)");
        if (!item.s || !price.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t itemRowId = Byte(r, GA_T::LOOT_TABLE_ITEM_ID);
            sqlite3_bind_int   (item.s,  1, (int)lootTableId);
            sqlite3_bind_int   (item.s,  2, (int)itemRowId);
            sqlite3_bind_int   (item.s,  3, (int)Byte (r, GA_T::ITEM_ID));
            sqlite3_bind_int   (item.s,  4, (int)Byte (r, GA_T::SUB_LOOT_TABLE_ID));
            sqlite3_bind_int   (item.s,  5, (int)Byte (r, GA_T::QUANTITY));
            sqlite3_bind_double(item.s,  6, (double)Float(r, GA_T::DROP_CHANCE));
            sqlite3_bind_int   (item.s,  7, (int)Byte (r, GA_T::SORT_ORDER));
            sqlite3_bind_int   (item.s,  8, (int)Byte (r, GA_T::QUALITY_VALUE_ID));
            sqlite3_bind_int   (item.s,  9, (int)Byte (r, GA_T::QUALITY_RANKING));
            sqlite3_bind_int   (item.s, 10, (int)Byte (r, GA_T::MAX_QUALITY_VALUE_ID));
            sqlite3_bind_int   (item.s, 11, (int)Flag (r, GA_T::AUTO_CREATE_FLAG));
            sqlite3_bind_int   (item.s, 12, (int)Flag (r, GA_T::USE_PLAYER_COMPONENTS_FLAG));
            item.step();

            // Nested DATA_SET_PRICES (0x05F4) — each row is inline (PRICE, CURRENCY 0x5F5).
            uint32_t priceArr = 0;
            CMarshal__GetArray::CallOriginal(r, nullptr, GA_T::DATA_SET_PRICES, &priceArr);
            if (priceArr) WalkArray(priceArr, [&](void* pr) {
                sqlite3_bind_int(price.s, 1, (int)itemRowId);
                sqlite3_bind_int(price.s, 2, (int)Int32(pr, GA_T::PRICE));
                sqlite3_bind_int(price.s, 3, (int)Int32(pr, 0x5F5));
                price.step();
            });
        });
    }

    // ---------- BlueprintModEffectGroups (0x0123, nested under BlueprintItemMods; vtable[1] -> FUN_1093f0d0) ----------
    void WalkBlueprintModEffectGroups(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_blueprint_mod_effect_groups ("
            " blueprint_mod_id, effect_group_id, make_chance"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)Int32(r, GA_T::BLUEPRINT_MOD_ID));
            sqlite3_bind_int   (st.s, 2, (int)Int32(r, GA_T::EFFECT_GROUP_ID));
            sqlite3_bind_double(st.s, 3, (double)Float(r, GA_T::MAKE_CHANCE));
            st.step();
        });
    }

    // ---------- LootTableGroupItems (0x0576, nested under LootTableGroups; vtable[0] -> FUN_1093da80) ----------
    void WalkLootTableGroupItems(void*, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t groupId = CachedByte(GA_T::LOOT_TABLE_GROUP_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_loot_table_group_items ("
            " loot_table_group_id, sort_order, loot_table_id"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)groupId);
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::SORT_ORDER));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::LOOT_TABLE_ID));
            st.step();
        });
    }

    // ---------- Effects (DATA_SET_EFFECTS = 0x014D, nested under EffectGroups) ----------
    //
    // The outer effect-group iteration populates m_values[EFFECT_GROUP_ID] via
    // get_int32_t before calling get_array(effect_group_row, 0x14D) which lands here.
    void WalkEffects(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t effectGroupId = CachedInt32(GA_T::EFFECT_GROUP_ID);

        static const char* kSql =
            "INSERT INTO asm_data_set_effects ("
            "  effect_group_id, effect_id, class_res_id,"
            "  base_value, min_value, max_value, calc_method_value_id,"
            "  prop_id, property_value_id, apply_on_interval_flag"
            ") VALUES (?,?,?,?,?,?,?,?,?,?)";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
            Logger::Log("db", "WalkEffects prepare failed: %s\n", sqlite3_errmsg(db));
            return;
        }

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int   (stmt,  1, (int)effectGroupId);
            sqlite3_bind_int   (stmt,  2, (int)Int32(row, GA_T::EFFECT_ID));
            sqlite3_bind_int   (stmt,  3, (int)Int32(row, GA_T::CLASS_RES_ID));
            sqlite3_bind_double(stmt,  4, (double)Float(row, GA_T::BASE_VALUE));
            sqlite3_bind_double(stmt,  5, (double)Float(row, GA_T::MIN_VALUE));
            sqlite3_bind_double(stmt,  6, (double)Float(row, GA_T::MAX_VALUE));
            sqlite3_bind_int   (stmt,  7, (int)Int32(row, GA_T::CALC_METHOD_VALUE_ID));
            sqlite3_bind_int   (stmt,  8, (int)Byte (row, GA_T::PROP_ID));
            sqlite3_bind_int   (stmt,  9, (int)Int32(row, GA_T::PROPERTY_VALUE_ID));
            sqlite3_bind_int   (stmt, 10, (int)Flag (row, GA_T::APPLY_ON_INTERVAL_FLAG));
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        });

        sqlite3_finalize(stmt);
    }

    // ========================================================================
    //                    NEW WALKERS — v17 (nested data sets)
    // ========================================================================

    // ---------- Device Effect Groups (DATA_SET_DEVICE_EFFECT_GROUPS = 0x0144) ----------
    //
    // Nested child of devices. Dispatcher reads DEVICE_ID via get_int32_t before
    // the device's own marshal loader fires this get_array, so we key on
    // CachedInt32(DEVICE_ID).
    //
    // The binary uses only the first row of this array (device+0xE0); we capture
    // every row to keep the data model faithful.
    void WalkDeviceEffectGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t deviceId = CachedInt32(GA_T::DEVICE_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_device_effect_groups ("
            "  device_id, effect_group_id"
            ") VALUES (?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)deviceId);
            sqlite3_bind_int(st.s, 2, (int)Byte(row, GA_T::EFFECT_GROUP_ID));
            st.step();
        });
    }

    // ---------- Device Mode Effect Groups (DATA_SET_DEVICE_MODE_EFFECT_GROUPS = 0x0146) ----------
    //
    // Nested inside each device mode row. The mode loader reads DEVICE_ID and
    // DEVICE_MODE_ID via get_byte before this get_array fires.
    void WalkDeviceModeEffectGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t deviceId = CachedByte(GA_T::DEVICE_ID);
        uint32_t modeId   = CachedByte(GA_T::DEVICE_MODE_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_device_mode_effect_groups ("
            "  device_id, device_mode_id, effect_group_id, effect_group_type_value_id"
            ") VALUES (?,?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)deviceId);
            sqlite3_bind_int(st.s, 2, (int)modeId);
            sqlite3_bind_int(st.s, 3, (int)Byte(row, GA_T::EFFECT_GROUP_ID));
            sqlite3_bind_int(st.s, 4, (int)Byte(row, GA_T::EFFECT_GROUP_TYPE_VALUE_ID));
            st.step();
        });
    }

    // ---------- Device Mode Properties (DATA_SET_DEVICE_MODE_PROPERTIES = 0x0147) ----------
    //
    // Per-mode property-override list.  Each row contributes a TgProperty descriptor
    // attached to the mode's property list.  The original walker (FUN_109a7d20 in the
    // client binary) reads FOUR fields per row:
    //   PROP_ID   (0x03E7, TCP_UINT32) → descriptor + 0x3C
    //   BASE_VALUE(0x0067, TCP_FLOAT)  → descriptor + 0x40   (also cloned to +0x44 as "raw")
    //   MIN_VALUE (0x035E, TCP_FLOAT)  → descriptor + 0x48
    //   MAX_VALUE (0x034B, TCP_FLOAT)  → descriptor + 0x4C
    //
    // The earlier version of this walker read a single `VALUE` float (which doesn't
    // exist in this data set) and used `Byte` for PROP_ID (which is TCP_UINT32, not
    // a byte).  Result: every row landed with 0.0 and PROP_ID truncated to 8 bits.
    // Fixed for real here — see also v20 migration that drops the old `value`
    // column and re-captures the table on next game launch.
    void WalkDeviceModeProperties(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t deviceId = CachedByte(GA_T::DEVICE_ID);
        uint32_t modeId   = CachedByte(GA_T::DEVICE_MODE_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_device_mode_properties ("
            "  device_id, device_mode_id, prop_id, base_value, min_value, max_value"
            ") VALUES (?,?,?,?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int   (st.s, 1, (int)deviceId);
            sqlite3_bind_int   (st.s, 2, (int)modeId);
            sqlite3_bind_int   (st.s, 3, (int)Int32(row, GA_T::PROP_ID));
            sqlite3_bind_double(st.s, 4, (double)Float(row, GA_T::BASE_VALUE));
            sqlite3_bind_double(st.s, 5, (double)Float(row, GA_T::MIN_VALUE));
            sqlite3_bind_double(st.s, 6, (double)Float(row, GA_T::MAX_VALUE));
            st.step();
        });
    }

    // ---------- Device Anim Set Groups (DATA_SET_DEVICE_ANIM_SET_GROUPS = 0x0142) ----------
    //
    // Device-level: per (gender, dest-type) animation set resource reference.
    // DEVICE_ID is the outer-dispatcher int32 cache.
    void WalkDeviceAnimSetGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t deviceId = CachedInt32(GA_T::DEVICE_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_device_anim_set_groups ("
            "  device_id, gender_type_value_id, anim_set_dest_type_value_id, anim_set_res_id"
            ") VALUES (?,?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)deviceId);
            sqlite3_bind_int(st.s, 2, (int)Byte (row, GA_T::GENDER_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte (row, GA_T::ANIM_SET_DEST_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 4, (int)Int32(row, GA_T::ANIM_SET_RES_ID));
            st.step();
        });
    }

    // ---------- Item Effect Groups (DATA_SET_ITEM_EFFECT_GROUPS = 0x0166) ----------
    //
    // Nested under items.  Item loader caches ITEM_ID via get_byte.
    void WalkItemEffectGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t itemId = CachedByte(GA_T::ITEM_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_item_effect_groups ("
            "  item_id, effect_group_id, effect_group_type_value_id"
            ") VALUES (?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)itemId);
            sqlite3_bind_int(st.s, 2, (int)Byte(row, GA_T::EFFECT_GROUP_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte(row, GA_T::EFFECT_GROUP_TYPE_VALUE_ID));
            st.step();
        });
    }

    // ---------- Item Props (DATA_SET_ITEM_PROPS = 0x0168) ----------
    //
    // Nested under items.  Per-item {prop_id, value} stat overrides.
    void WalkItemProps(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t itemId = CachedByte(GA_T::ITEM_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_item_props ("
            "  item_id, prop_id, value"
            ") VALUES (?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int   (st.s, 1, (int)itemId);
            sqlite3_bind_int   (st.s, 2, (int)Byte (row, GA_T::PROP_ID));
            sqlite3_bind_double(st.s, 3, (double)Float(row, GA_T::VALUE));
            st.step();
        });
    }

    // ---------- Item Mesh Asm Groups (DATA_SET_ITEM_MESH_ASM_GROUPS = 0x0167) ----------
    //
    // Nested under items. Per-(gender, mesh-type, quality) → asm_id reference.
    // If the row omits QUALITY_VALUE_ID the game defaults to 0x48D; we store
    // whatever the marshal returns (0 on missing), same as other walkers.
    void WalkItemMeshAsmGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t itemId = CachedByte(GA_T::ITEM_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_item_mesh_asm_groups ("
            "  item_id, gender_type_value_id, item_mesh_asm_type_value_id,"
            "  quality_value_id, asm_id"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)itemId);
            sqlite3_bind_int(st.s, 2, (int)Byte(row, GA_T::GENDER_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte(row, GA_T::ITEM_MESH_ASM_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 4, (int)Byte(row, GA_T::QUALITY_VALUE_ID));
            sqlite3_bind_int(st.s, 5, (int)Byte(row, GA_T::ASM_ID));
            st.step();
        });
    }

    // ---------- Prices (DATA_SET_PRICES = 0x05F4) ----------
    //
    // Nested under items.  Store price rows keyed by currency. PRICE is int32,
    // CURRENCY_TYPE_VALUE_ID is int32 in the marshal (see LoadItemMarshal).
    void WalkPrices(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        uint32_t itemId = CachedByte(GA_T::ITEM_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_prices ("
            "  item_id, price, currency_type_value_id"
            ") VALUES (?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)itemId);
            sqlite3_bind_int(st.s, 2, (int)Int32(row, GA_T::PRICE));
            sqlite3_bind_int(st.s, 3, (int)Int32(row, GA_T::CURRENCY_TYPE_VALUE_ID));
            st.step();
        });
    }

    // ========================================================================
    //                  NEW WALKERS — v18 (complete-audit additions)
    //
    // Every walker below was identified by the systematic audit in
    // .planning/asm-dat-audit/. Parent-key caches match the loader context
    // documented in GAP_SUMMARY.md § "Walker parent-key resolution".
    // ========================================================================

    // ---------- Assembly-mesh sub-tables ----------
    //
    // All five are walked by the asm-mesh row init (FUN_1094b470). The mesh
    // row's primary key is ASM_ID read via get_int32_t by the outer dispatcher
    // (FUN_1094dee0), so CachedInt32(ASM_ID) resolves the parent key.

    void WalkAsmMeshAnimSets(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t asmId = CachedInt32(GA_T::ASM_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_asm_mesh_anim_sets ("
            "  asm_id, anim_set_res_id, sort_order"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)asmId);
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::ANIM_SET_RES_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte (r, GA_T::SORT_ORDER));
            st.step();
        });
    }

    void WalkAsmMeshAudioGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t asmId = CachedInt32(GA_T::ASM_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_asm_mesh_audio_groups ("
            "  asm_id, audio_group_id"
            ") VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            // Field is TCP_UINT32; the asm-mesh loader (FUN_1094b470) uses
            // get_int32_t. Earlier capture used Byte and would silently
            // truncate any audio_group_id ≥ 256.
            sqlite3_bind_int(st.s, 1, (int)asmId);
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::AUDIO_GROUP_ID));
            st.step();
        });
    }

    void WalkAsmMeshFxs(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t asmId = CachedInt32(GA_T::ASM_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_asm_mesh_fxs ("
            "  asm_id, socket_res_id, display_group_res_id, display_mode,"
            "  fx_id, display_order, slot_value_id"
            ") VALUES (?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)asmId);
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::SOCKET_RES_ID));
            sqlite3_bind_int(st.s, 3, (int)Int32(r, GA_T::DISPLAY_GROUP_RES_ID));
            sqlite3_bind_int(st.s, 4, (int)Byte (r, GA_T::DISPLAY_MODE));
            sqlite3_bind_int(st.s, 5, (int)Byte (r, GA_T::FX_ID));
            sqlite3_bind_int(st.s, 6, (int)Byte (r, GA_T::DISPLAY_ORDER));
            sqlite3_bind_int(st.s, 7, (int)Byte (r, GA_T::SLOT_VALUE_ID));
            st.step();
        });
    }

    void WalkAsmMeshImpactGrps(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t asmId = CachedInt32(GA_T::ASM_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_asm_mesh_impact_grps ("
            "  asm_id, impact_fx_group_id, impact_fx_group_type_value_id"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)asmId);
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::IMPACT_FX_GROUP_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::IMPACT_FX_GROUP_TYPE_VALUE_ID));
            st.step();
        });
    }

    void WalkAsmMeshMorphSets(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t asmId = CachedInt32(GA_T::ASM_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_asm_mesh_morph_sets ("
            "  asm_id, morph_set_res_id, sort_order"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)asmId);
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::MORPH_SET_RES_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte (r, GA_T::SORT_ORDER));
            st.step();
        });
    }

    // ---------- Special-FX sub-tables ----------
    //
    // Parent (FX row) reads FX_ID via get_byte in FUN_1094a800, so CachedByte.

    void WalkSpecialFxMaterials(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t fxId = CachedByte(GA_T::FX_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_special_fx_materials ("
            "  fx_id, fx_material_id, fx_material_type_value_id,"
            "  material_res_id, same_team_flag"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)fxId);
            sqlite3_bind_int(st.s, 2, (int)Byte (r, GA_T::FX_MATERIAL_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte (r, GA_T::FX_MATERIAL_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 4, (int)Int32(r, GA_T::MATERIAL_RES_ID));
            sqlite3_bind_int(st.s, 5, (int)Flag (r, GA_T::SAME_TEAM_FLAG));
            st.step();
        });
    }

    void WalkSpecialFxPsc(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t fxId = CachedByte(GA_T::FX_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_special_fx_psc ("
            "  fx_id, fx_psc_id, psc_template_res_id, psc_loops_flag,"
            "  scale, scaling_type_value_id, fx_display_group_res_id"
            ") VALUES (?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)fxId);
            sqlite3_bind_int   (st.s, 2, (int)Byte (r, GA_T::FX_PSC_ID));
            sqlite3_bind_int   (st.s, 3, (int)Int32(r, GA_T::PSC_TEMPLATE_RES_ID));
            sqlite3_bind_int   (st.s, 4, (int)Flag (r, GA_T::PSC_LOOPS_FLAG));
            sqlite3_bind_double(st.s, 5, (double)Float(r, GA_T::SCALE));
            sqlite3_bind_int   (st.s, 6, (int)Byte (r, GA_T::SCALING_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, 7, (int)Int32(r, GA_T::FX_DISPLAY_GROUP_RES_ID));
            st.step();
        });
    }

    void WalkSpecialFxSounds(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t fxId = CachedByte(GA_T::FX_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_special_fx_sounds ("
            "  fx_id, fx_sound_id, sound_cue_res_id, allow_sound_to_finish_flag"
            ") VALUES (?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)fxId);
            sqlite3_bind_int(st.s, 2, (int)Byte (r, GA_T::FX_SOUND_ID));
            sqlite3_bind_int(st.s, 3, (int)Int32(r, GA_T::SOUND_CUE_RES_ID));
            sqlite3_bind_int(st.s, 4, (int)Flag (r, GA_T::ALLOW_SOUND_TO_FINISH_FLAG));
            st.step();
        });
    }

    // ---------- Material resources hierarchy ----------
    //
    // MATERIAL_RESOURCES nests under MATERIAL_RES_GROUPS (parent reads
    // MATERIAL_RES_GROUP_ID via get_byte). MAT_RES_PARAMS nests under each
    // MATERIAL_RESOURCES row (parent reads MATERIAL_RES_ID via get_byte).

    void WalkMaterialResources(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t groupId = CachedByte(GA_T::MATERIAL_RES_GROUP_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_material_resources ("
            "  material_res_group_id, material_res_id,"
            "  material_res_sub_type_value_id, material_res_type_value_id,"
            "  res_id, sort_order"
            ") VALUES (?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            // The loader (FUN_1094a650) reads MATERIAL_RESOURCE_ID = 0x033B as
            // the row's primary key, NOT MATERIAL_RES_ID = 0x033D. Earlier
            // captures stored garbage here — see v21 migration.
            sqlite3_bind_int(st.s, 1, (int)groupId);
            sqlite3_bind_int(st.s, 2, (int)Byte (r, GA_T::MATERIAL_RESOURCE_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte (r, GA_T::MATERIAL_RES_SUB_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 4, (int)Byte (r, GA_T::MATERIAL_RES_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s, 5, (int)Int32(r, GA_T::RES_ID));
            sqlite3_bind_int(st.s, 6, (int)Byte (r, GA_T::SORT_ORDER));
            st.step();
        });
    }

    void WalkMatResParams(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        // Parent (MATERIAL_RESOURCES) reads MATERIAL_RESOURCE_ID via get_byte
        // before this nested array fires. Read from that cache slot, not
        // MATERIAL_RES_ID (a different field id, 0x033D).
        uint32_t materialResId = CachedByte(GA_T::MATERIAL_RESOURCE_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_mat_res_params ("
            "  material_res_id, mat_res_param_id,"
            "  material_parameter_id, parameter_res_id"
            ") VALUES (?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)materialResId);
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::MAT_RES_PARAM_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::MATERIAL_PARAMETER_ID));
            sqlite3_bind_int(st.s, 4, (int)Byte(r, GA_T::PARAMETER_RES_ID));
            st.step();
        });
    }

    // ---------- XP Caps (DATA_SET_XP_CAPS = 0x01CA) ----------
    //
    // Nested under xp_levels. Parent reads XP_LEVEL_ID via get_byte.
    void WalkXpCaps(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t xpLevelId = CachedByte(GA_T::XP_LEVEL_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_xp_caps ("
            "  xp_level_id, xp_mission_cap_id, event_reward_id,"
            "  xp_value, reward_event_value_id"
            ") VALUES (?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)xpLevelId);
            sqlite3_bind_int(st.s, 2, (int)Byte(r, GA_T::XP_MISSION_CAP_ID));
            sqlite3_bind_int(st.s, 3, (int)Byte(r, GA_T::EVENT_REWARD_ID));
            sqlite3_bind_int(st.s, 4, (int)Byte(r, GA_T::XP_VALUE));
            sqlite3_bind_int(st.s, 5, (int)Byte(r, GA_T::REWARD_EVENT_VALUE_ID));
            st.step();
        });
    }

    // ---------- Quest requirement chain ----------

    void WalkQuestRequirements(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t questId = CachedInt32(GA_T::QUEST_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_quest_requirements ("
            "  quest_id, quest_requirement_id, count, map_game_id,"
            "  requirement_type_value_id, gameplay_type_value_id,"
            "  difficulty_value_id, name_msg_id,"
            "  target_bot_id, target_item_id, target_ui_volume_id"
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            uint32_t reqType = Byte(r, GA_T::REQUIREMENT_TYPE_VALUE_ID);
            sqlite3_bind_int(st.s,  1, (int)questId);
            sqlite3_bind_int(st.s,  2, (int)Int32(r, GA_T::QUEST_REQUIREMENT_ID));
            sqlite3_bind_int(st.s,  3, (int)Byte (r, GA_T::COUNT));
            sqlite3_bind_int(st.s,  4, (int)Byte (r, GA_T::MAP_GAME_ID));
            sqlite3_bind_int(st.s,  5, (int)reqType);
            sqlite3_bind_int(st.s,  6, (int)Byte (r, GA_T::GAMEPLAY_TYPE_VALUE_ID));
            sqlite3_bind_int(st.s,  7, (int)Byte (r, GA_T::DIFFICULTY_VALUE_ID));
            sqlite3_bind_int(st.s,  8, (int)Byte (r, GA_T::NAME_MSG_ID));
            // Type-tagged target_id: the requirement's target depends on type.
            // Source: FUN_10949ca0 case switch at offsets 0x593-0x597.
            sqlite3_bind_int(st.s,  9, reqType == 0x594 ? (int)Byte(r, GA_T::BOT_ID)        : 0);
            sqlite3_bind_int(st.s, 10, reqType == 0x595 ? (int)Byte(r, GA_T::ITEM_ID)       : 0);
            sqlite3_bind_int(st.s, 11, reqType == 0x597 ? (int)Byte(r, GA_T::UI_VOLUME_ID)  : 0);
            st.step();
        });
    }

    void WalkQuestPrerequisites(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t questId = CachedInt32(GA_T::QUEST_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_quest_prerequisites ("
            "  quest_id, prereq_quest_id, quest_status_type_value_id"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)questId);
            sqlite3_bind_int(st.s, 2, (int)Byte (r, GA_T::PREREQ_QUEST_ID));
            sqlite3_bind_int(st.s, 3, (int)Int32(r, GA_T::QUEST_STATUS_TYPE_VALUE_ID));
            st.step();
        });
    }

    void WalkQuestReqObjectives(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t reqId = CachedInt32(GA_T::QUEST_REQUIREMENT_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_quest_req_objectives ("
            "  quest_requirement_id, objective_id"
            ") VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int(st.s, 1, (int)reqId);
            sqlite3_bind_int(st.s, 2, (int)Int32(r, GA_T::OBJECTIVE_ID));
            st.step();
        });
    }

    // ---------- Bot AI hierarchy ----------
    //
    // BEHAVIORS is a top-level list. ACTIONS nest under behaviors (parent
    // caches BEHAVIOR_ID via get_int32_t in the outer dispatcher FUN_10950220).
    // TESTS nest under each action (action init caches ACTION_ID via get_byte).

    void WalkBotBehaviors(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT INTO asm_data_set_bot_behaviors ("
            "  behavior_id, name"
            ") VALUES (?,?)");
        if (!st.s) return;
        Txn txn(db);
        char nameBuf[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::NAME, nameBuf, sizeof(nameBuf));
            sqlite3_bind_int (st.s, 1, (int)Int32(r, GA_T::BEHAVIOR_ID));
            sqlite3_bind_text(st.s, 2, nameBuf, -1, SQLITE_TRANSIENT);
            st.step();
        });
    }

    void WalkBotActions(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t behaviorId = CachedInt32(GA_T::BEHAVIOR_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_bot_actions ("
            "  behavior_id, action_id, action_type_value_id, description,"
            "  movement_type_value_id, destination_type_value_id,"
            "  pace_type_value_id, target_type_value_id, look_at_type_value_id,"
            "  sound_cue_id, audio_chance, animation_res_id, animation_seconds,"
            "  alarm_bot_spawn_table_id, generic_event_number, sub_behavior_id,"
            "  custom_param_string, emote_interruptible_flag,"
            "  reevaluate_tests_flag, fire_mode_sequence, slot_used_value_id,"
            "  secondary_fire_mode_sequence, secondary_slot_used_value_id,"
            "  posture_type_value_id"
            ") VALUES ("
            "  ?,?,?,?, ?,?, ?,?,?, ?,?,?,?, ?,?,?, ?,?, ?,?,?, ?,?, ? )");
        if (!st.s) return;
        Txn txn(db);
        char descBuf[4096], paramBuf[4096];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::DESCRIPTION,         descBuf,  sizeof(descBuf));
            GetWcharName(r, GA_T::CUSTOM_PARAM_STRING, paramBuf, sizeof(paramBuf));
            int c = 1;
            sqlite3_bind_int   (st.s, c++, (int)behaviorId);
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::ACTION_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::ACTION_TYPE_VALUE_ID));
            sqlite3_bind_text  (st.s, c++, descBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::MOVEMENT_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::DESTINATION_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::PACE_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::TARGET_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::LOOK_AT_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::SOUND_CUE_ID));
            sqlite3_bind_double(st.s, c++, (double)Float(r, GA_T::AUDIO_CHANCE));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::ANIMATION_RES_ID));
            sqlite3_bind_double(st.s, c++, (double)Float(r, GA_T::ANIMATION_SECONDS));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::ALARM_BOT_SPAWN_TABLE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::GENERIC_EVENT_NUMBER));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::SUB_BEHAVIOR_ID));
            sqlite3_bind_text  (st.s, c++, paramBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int   (st.s, c++, (int)Flag (r, GA_T::EMOTE_INTERRUPTIBLE_FLAG));
            sqlite3_bind_int   (st.s, c++, (int)Flag (r, GA_T::REEVALUATE_TESTS_FLAG));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::FIRE_MODE_SEQUENCE));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::SLOT_USED_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::SECONDARY_FIRE_MODE_SEQUENCE));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::SECONDARY_SLOT_USED_VALUE_ID));
            sqlite3_bind_int   (st.s, c++, (int)Byte (r, GA_T::POSTURE_TYPE_VALUE_ID));
            st.step();
        });
    }

    void WalkBotTests(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t actionId = CachedByte(GA_T::ACTION_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_bot_tests ("
            "  action_id, test_type_value_id,"
            "  comparison_type_value_id, comparison_value"
            ") VALUES (?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s, 1, (int)actionId);
            sqlite3_bind_int   (st.s, 2, (int)Byte (r, GA_T::TEST_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s, 3, (int)Byte (r, GA_T::COMPARISON_TYPE_VALUE_ID));
            sqlite3_bind_double(st.s, 4, (double)Float(r, GA_T::COMPARISON_VALUE));
            st.step();
        });
    }

    // ---------- Skill Effect Groups (DATA_SET_SKILL_EFFECT_GROUPS = 0x0198) ----------
    //
    // Nested somewhere in the skill-tree hierarchy.  The exact parent depends on
    // whether the effect is attached to a skill_group, skill, or sub_skill — the
    // game's loader may fire this from more than one level.  We capture all three
    // cached keys and let the consumer filter by which is non-zero.
    // ---------- Message Translations (DATA_SET_MSG_TRANSLATIONS = 0x017A) ----------
    //
    // Loaded from lang_*.dat (NOT assembly.dat). FUN_1093ebb0 fires this for
    // the player's locale-specific file; FUN_1093d1a0 fires it for the
    // English fallback when locale ≠ English. Per-row schema mirrors
    // FUN_10939430 in the binary: {msg_id (I 0x36E), message (W 0x355),
    // sound_res_id (I 0x493)}.
    //
    // Uses INSERT OR REPLACE keyed on msg_id so that a second pass (English
    // fallback after a localized first pass) overwrites with the English
    // text. Result: final table is always English regardless of locale.
    void WalkMessageTranslations(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        Stmt st(db,
            "INSERT OR REPLACE INTO asm_data_set_msg_translations ("
            "  msg_id, message, sound_res_id"
            ") VALUES (?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        // Translation strings can be long (paragraphs of mission briefing
        // text). 8 KB is comfortable for the longest known entries.
        char msgBuf[8192];
        WalkArray(arr, [&](void* r) {
            GetWcharName(r, GA_T::MESSAGE, msgBuf, sizeof(msgBuf));
            sqlite3_bind_int (st.s, 1, (int)Int32(r, GA_T::MSG_ID));
            sqlite3_bind_text(st.s, 2, msgBuf, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (st.s, 3, (int)Int32(r, GA_T::SOUND_RES_ID));
            st.step();
        });
    }

    // ---------- Achievement Reqs (DATA_SET_ACHIEVEMENT_REQS = 0x010F) ----------
    //
    // Nested under DATA_SET_ACHIEVEMENTS. The achievement row init
    // (FUN_1094d1b0) reads ACHIEVEMENT_ID via get_byte before issuing
    // get_array(row, 0x10F) — child rows arrive here with that cache populated.
    //
    // Field schema is split across two functions in the binary:
    //   FUN_1093f110 reads REQUIREMENT_ID (B 0x435), ACHIEVEMENT_ID (B 0xD),
    //                      REQUIREMENT_LIST_ID (B 0x436), REQUIREMENT_VALUE (F 0x438)
    //                      and then dispatches to FUN_1093dbf0 on row+0x14.
    //   FUN_1093dbf0 reads REQUIREMENT_ID (B 0x435 again), METRIC_VALUE_ID (B 0x357),
    //                      MAP_GAME_ID (B 0x322), GAMEPLAY_TYPE_VALUE_ID (B 0x5C4),
    //                      CLASS_VALUE_ID (B 0x5C5), DIFFICULTY_VALUE_ID (B 0x210),
    //                      TEAM_SIZE (B 0x4F2).
    void WalkAchievementReqs(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection(); if (!db) return;
        uint32_t achievementId = CachedByte(GA_T::ACHIEVEMENT_ID);
        Stmt st(db,
            "INSERT INTO asm_data_set_achievement_reqs ("
            "  achievement_id, requirement_id, requirement_list_id,"
            "  requirement_value, metric_value_id, map_game_id,"
            "  gameplay_type_value_id, class_value_id, difficulty_value_id,"
            "  team_size"
            ") VALUES (?,?,?,?,?,?,?,?,?,?)");
        if (!st.s) return;
        Txn txn(db);
        WalkArray(arr, [&](void* r) {
            sqlite3_bind_int   (st.s,  1, (int)achievementId);
            sqlite3_bind_int   (st.s,  2, (int)Byte (r, GA_T::REQUIREMENT_ID));
            sqlite3_bind_int   (st.s,  3, (int)Byte (r, GA_T::REQUIREMENT_LIST_ID));
            sqlite3_bind_double(st.s,  4, (double)Float(r, GA_T::REQUIREMENT_VALUE));
            sqlite3_bind_int   (st.s,  5, (int)Byte (r, GA_T::METRIC_VALUE_ID));
            sqlite3_bind_int   (st.s,  6, (int)Byte (r, GA_T::MAP_GAME_ID));
            sqlite3_bind_int   (st.s,  7, (int)Byte (r, GA_T::GAMEPLAY_TYPE_VALUE_ID));
            sqlite3_bind_int   (st.s,  8, (int)Byte (r, GA_T::CLASS_VALUE_ID));
            sqlite3_bind_int   (st.s,  9, (int)Byte (r, GA_T::DIFFICULTY_VALUE_ID));
            sqlite3_bind_int   (st.s, 10, (int)Byte (r, GA_T::TEAM_SIZE));
            st.step();
        });
    }

    void WalkSkillEffectGroups(void* /*marshal*/, uint32_t arr) {
        sqlite3* db = Database::GetConnection();
        if (!db) return;

        // SKILL_EFFECT_GROUPS can be nested under skill_group / skill /
        // sub_skill / skill_group_rank — the loader sets each parent's
        // primary key into get_byte's m_values cache before issuing the
        // nested get_array. We capture all four cache slots so the consumer
        // can pick the non-zero one for their context.
        uint32_t skillGroupId = CachedByte(GA_T::SKILL_GROUP_ID);
        uint32_t skillId      = CachedByte(GA_T::SKILL_ID);
        uint32_t subSkillId   = CachedByte(GA_T::SUB_SKILL_ID);
        uint32_t rankId       = CachedByte(GA_T::RANK_ID);

        Stmt st(db,
            "INSERT INTO asm_data_set_skill_effect_groups ("
            "  skill_group_id, skill_id, sub_skill_id, rank_id,"
            "  effect_group_id, effect_group_type_value_id"
            ") VALUES (?,?,?,?,?,?)");
        if (!st.s) return;

        Txn txn(db);
        WalkArray(arr, [&](void* row) {
            sqlite3_bind_int(st.s, 1, (int)skillGroupId);
            sqlite3_bind_int(st.s, 2, (int)skillId);
            sqlite3_bind_int(st.s, 3, (int)subSkillId);
            sqlite3_bind_int(st.s, 4, (int)rankId);
            sqlite3_bind_int(st.s, 5, (int)Byte(row, GA_T::EFFECT_GROUP_ID));
            sqlite3_bind_int(st.s, 6, (int)Byte(row, GA_T::EFFECT_GROUP_TYPE_VALUE_ID));
            st.step();
        });
    }
}

namespace {
    // dataset-id → primary target table, consulted by OnGetArray before
    // dispatching to the walker. Skips any walker whose table already holds
    // rows, so re-running with bPopulateDatabase = true is idempotent.
    const std::unordered_map<int, const char*>& TargetTableMap() {
        static const std::unordered_map<int, const char*> m = {
            // existing
            { GA_T::DATA_SET_ITEMS,                       "asm_data_set_items" },
            { GA_T::DATA_SET_BOTS,                        "asm_data_set_bots" },
            { GA_T::DATA_SET_BOT_DEVICES,                 "asm_data_set_bots_data_set_bot_devices" },
            { GA_T::DATA_SET_BOT_SPAWN_TABLES,            "asm_data_set_bot_spawn_tables" },
            { GA_T::DATA_SET_DEVICES,                     "asm_data_set_devices" },
            { GA_T::DATA_SET_DEVICE_MODES,                "asm_data_set_devices_data_set_device_modes" },
            { GA_T::DATA_SET_EFFECT_GROUPS,               "asm_data_set_effect_groups" },
            { GA_T::DATA_SET_EFFECTS,                     "asm_data_set_effects" },
            { GA_T::DATA_SET_PROJECTILES,                 "asm_data_set_projectiles" },
            { GA_T::DATA_SET_DEPLOYABLES,                 "asm_data_set_deployables" },
            { GA_T::DATA_SET_DESTRUCTIBLES,               "asm_data_set_destructibles" },
            { GA_T::DATA_SET_ASSEMBLY_MESHES,             "asm_data_set_assembly_meshes" },
            { GA_T::DATA_SET_SPECIAL_FX,                  "asm_data_set_fx" },
            { GA_T::DATA_SET_MAT_RES_GROUPS,              "asm_data_set_material_res_groups" },
            { GA_T::DATA_SET_MATERIAL_PARAMETERS,         "asm_data_set_material_parameters" },
            { GA_T::DATA_SET_VISIBILITY_CONFIGS,          "asm_data_set_visibility_configs" },
            { GA_T::DATA_SET_ICONS,                       "asm_data_set_icons" },
            { GA_T::DATA_SET_PROPERTIES,                  "asm_data_set_properties" },
            { GA_T::DATA_SET_HEX_BUILDINGS,               "asm_data_set_hex_buildings" },
            { GA_T::DATA_SET_OBJECTIVES,                  "asm_data_set_objectives" },
            { GA_T::DATA_SET_XP_LEVELS,                   "asm_data_set_xp_levels" },
            { GA_T::DATA_SET_DEVICE_SLOT_UNLOCK_GRP,      "asm_data_set_device_slot_unlock_groups" },
            { GA_T::DATA_SET_UI_VOLUMES,                  "asm_data_set_ui_volumes" },
            { GA_T::DATA_SET_QUESTS,                      "asm_data_set_quests" },
            { GA_T::DATA_SET_QUEST_GROUPS,                "asm_data_set_quest_groups" },
            { GA_T::DATA_SET_RESOURCES,                   "asm_data_set_resources" },
            { GA_T::DATA_SET_VALID_VALUES,                "asm_data_set_valid_values" },
            { GA_T::DATA_SET_PRODUCTION_DEVICES,          "asm_data_set_production_devices" },
            { GA_T::DATA_SET_PRODUCTION_FLAIRS,           "asm_data_set_production_flairs" },
            { GA_T::DATA_SET_PET_USED_RESOURCES,          "asm_data_set_pet_used_resources" },
            { GA_T::DATA_SET_IMPACT_FX,                   "asm_data_set_impact_fx" },
            { GA_T::DATA_SET_SKILL_GROUP_RANKS,           "asm_data_set_skill_group_ranks" },
            { GA_T::DATA_SET_ACHIEVEMENT_GROUPS,          "asm_data_set_achievement_groups" },
            { GA_T::DATA_SET_ACHIEVEMENTS,                "asm_data_set_achievements" },
            { GA_T::DATA_SET_TIPS,                        "asm_data_set_tips" },
            { GA_T::DATA_SET_EVENT_REWARDS,               "asm_data_set_event_rewards" },
            { GA_T::DATA_SET_PLAYER_USED_RESOURCES,       "asm_data_set_player_used_resources" },
            { GA_T::DATA_SET_PRODUCTION_MAP_GAME_LIST,    "asm_data_set_production_map_game_list" },
            { GA_T::DATA_SET_DEVICE_USED_ANIMSETS,        "asm_data_set_device_used_animsets" },
            { GA_T::DATA_SET_HARDCODED_RESOURCES,         "asm_data_set_hardcoded_resources" },
            { GA_T::DATA_SET_WORLD_OBJECTS,               "asm_data_set_world_objects" },
            { GA_T::DATA_SET_OBJECTIVE_BOTS,              "asm_data_set_objective_bots" },
            { GA_T::DATA_SET_SOUND_CUES,                  "asm_data_set_sound_cues" },
            { GA_T::DATA_SET_BLUEPRINTS,                  "asm_data_set_blueprints" },
            { GA_T::DATA_SET_LOOT_TABLES,                 "asm_data_set_loot_tables" },
            { GA_T::DATA_SET_LOOT_TABLE_GROUPS,           "asm_data_set_loot_table_groups" },
            { GA_T::DATA_SET_SKILL_GROUP_SETS,            "asm_data_set_skill_group_sets" },
            { GA_T::DATA_SET_SKILL_GROUPS,                "asm_data_set_skill_groups" },
            { GA_T::DATA_SET_SKILL_GROUP_SKILLS,          "asm_data_set_skill_group_skills" },
            { GA_T::DATA_SET_SUB_SKILLS,                  "asm_data_set_sub_skills" },
            { GA_T::DATA_SET_BLUEPRINT_ITEMS,             "asm_data_set_blueprint_items" },
            { GA_T::DATA_SET_BLUEPRINT_ITEM_MODS,         "asm_data_set_blueprint_item_mods" },
            { GA_T::DATA_SET_LOOT_TABLE_ITEMS,            "asm_data_set_loot_table_items" },
            { GA_T::DATA_SET_LOOT_TABLE_GROUP_ITEMS,      "asm_data_set_loot_table_group_items" },
            { GA_T::DATA_SET_BLUEPRNT_MOD_EFFECT_GRPS,    "asm_data_set_blueprint_mod_effect_groups" },

            // v17 additions
            { GA_T::DATA_SET_DEVICE_EFFECT_GROUPS,        "asm_data_set_device_effect_groups" },
            { GA_T::DATA_SET_DEVICE_MODE_EFFECT_GROUPS,   "asm_data_set_device_mode_effect_groups" },
            { GA_T::DATA_SET_DEVICE_MODE_PROPERTIES,      "asm_data_set_device_mode_properties" },
            { GA_T::DATA_SET_DEVICE_ANIM_SET_GROUPS,      "asm_data_set_device_anim_set_groups" },
            { GA_T::DATA_SET_ITEM_EFFECT_GROUPS,          "asm_data_set_item_effect_groups" },
            { GA_T::DATA_SET_ITEM_PROPS,                  "asm_data_set_item_props" },
            { GA_T::DATA_SET_ITEM_MESH_ASM_GROUPS,        "asm_data_set_item_mesh_asm_groups" },
            { GA_T::DATA_SET_PRICES,                      "asm_data_set_prices" },
            { GA_T::DATA_SET_SKILL_EFFECT_GROUPS,         "asm_data_set_skill_effect_groups" },

            // v18 additions (complete-audit nested datasets)
            { GA_T::DATA_SET_ASM_MESH_ANIM_SETS,          "asm_data_set_asm_mesh_anim_sets" },
            { GA_T::DATA_SET_ASM_MESH_AUDIO_GROUPS,       "asm_data_set_asm_mesh_audio_groups" },
            { GA_T::DATA_SET_ASM_MESH_FXS,                "asm_data_set_asm_mesh_fxs" },
            { GA_T::DATA_SET_ASM_MESH_IMPACT_GRPS,        "asm_data_set_asm_mesh_impact_grps" },
            { GA_T::DATA_SET_ASM_MESH_MORPH_SETS,         "asm_data_set_asm_mesh_morph_sets" },
            { GA_T::DATA_SET_SPECIAL_FX_MATERIALS,        "asm_data_set_special_fx_materials" },
            { GA_T::DATA_SET_SPECIAL_FX_PSC,              "asm_data_set_special_fx_psc" },
            { GA_T::DATA_SET_SPECIAL_FX_SOUNDS,           "asm_data_set_special_fx_sounds" },
            { GA_T::DATA_SET_MATERIAL_RESOURCES,          "asm_data_set_material_resources" },
            { GA_T::DATA_SET_MAT_RES_PARAMS,              "asm_data_set_mat_res_params" },
            { GA_T::DATA_SET_XP_CAPS,                     "asm_data_set_xp_caps" },
            { GA_T::DATA_SET_QUEST_REQUIREMENTS,          "asm_data_set_quest_requirements" },
            { GA_T::DATA_SET_QUEST_PREREQUISITES,         "asm_data_set_quest_prerequisites" },
            { GA_T::DATA_SET_QUEST_REQ_OBJECTIVES,        "asm_data_set_quest_req_objectives" },
            { GA_T::DATA_SET_BOT_BEHAVIORS,               "asm_data_set_bot_behaviors" },
            { GA_T::DATA_SET_BOT_ACTIONS,                 "asm_data_set_bot_actions" },
            { GA_T::DATA_SET_BOT_TESTS,                   "asm_data_set_bot_tests" },

            // v21 additions (re-verification audit, .planning/asm-dat-audit/)
            { GA_T::DATA_SET_ACHIEVEMENT_REQS,            "asm_data_set_achievement_reqs" },

            // v23 — translations from lang_English.dat (NOT assembly.dat,
            // but loaded via the same CMarshal__get_array pipeline).
            { GA_T::DATA_SET_MSG_TRANSLATIONS,            "asm_data_set_msg_translations" },
        };
        return m;
    }
}

void AsmDataCapture::OnGetArray(void* marshal, int field, uint32_t arrayPtr) {
    if (!bPopulateDatabase) return;
    if (arrayPtr == 0) return;

    // Per-table idempotency: if this dataset already populated its target table
    // on a previous run, skip the walker entirely. See ShouldWalk() for cache
    // semantics (cache is populated on first check, reused for the session).
    {
        const auto& m = TargetTableMap();
        auto it = m.find(field);
        if (it != m.end() && !ShouldWalk(it->second)) return;
    }

    switch (field) {
        // existing data sets
        case GA_T::DATA_SET_ITEMS:                      WalkItems                (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BOTS:                       WalkBots                 (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BOT_DEVICES:                WalkBotDevices           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BOT_SPAWN_TABLES:           WalkBotSpawnTables       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICES:                    WalkDevices              (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICE_MODES:               WalkDeviceModes          (marshal, arrayPtr); break;
        case GA_T::DATA_SET_EFFECT_GROUPS:              WalkEffectGroups         (marshal, arrayPtr); break;
        case GA_T::DATA_SET_EFFECTS:                    WalkEffects              (marshal, arrayPtr); break;

        // new helper-dispatched data sets
        case GA_T::DATA_SET_PROJECTILES:                WalkProjectiles          (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEPLOYABLES:                WalkDeployables          (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DESTRUCTIBLES:              WalkDestructibles        (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ASSEMBLY_MESHES:            WalkAssemblyMeshes       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SPECIAL_FX:                 WalkFx                   (marshal, arrayPtr); break;
        case GA_T::DATA_SET_MAT_RES_GROUPS:             WalkMaterialResGroups    (marshal, arrayPtr); break;
        case GA_T::DATA_SET_MATERIAL_PARAMETERS:        WalkMaterialParameters   (marshal, arrayPtr); break;
        case GA_T::DATA_SET_VISIBILITY_CONFIGS:         WalkVisibilityConfigs    (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ICONS:                      WalkIcons                (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PROPERTIES:                 WalkProperties           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_HEX_BUILDINGS:              WalkHexBuildings         (marshal, arrayPtr); break;
        case GA_T::DATA_SET_OBJECTIVES:                 WalkObjectives           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_XP_LEVELS:                  WalkXpLevels             (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICE_SLOT_UNLOCK_GRP:     WalkDeviceSlotUnlockGroups(marshal, arrayPtr); break;
        case GA_T::DATA_SET_UI_VOLUMES:                 WalkUiVolumes            (marshal, arrayPtr); break;
        case GA_T::DATA_SET_QUESTS:                     WalkQuests               (marshal, arrayPtr); break;
        case GA_T::DATA_SET_QUEST_GROUPS:               WalkQuestGroups          (marshal, arrayPtr); break;

        // inline master data sets (fired from within CAssemblyManager::LoadAssemblyDatFile)
        case GA_T::DATA_SET_RESOURCES:                  WalkResources            (marshal, arrayPtr); break;
        case GA_T::DATA_SET_VALID_VALUES:               WalkValidValues          (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PRODUCTION_DEVICES:         WalkProductionDevices    (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PRODUCTION_FLAIRS:          WalkProductionFlairs     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PET_USED_RESOURCES:         WalkPetUsedResources     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_IMPACT_FX:                  WalkImpactFx             (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SKILL_GROUP_RANKS:          WalkSkillGroupRanks      (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ACHIEVEMENT_GROUPS:         WalkAchievementGroups    (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ACHIEVEMENTS:               WalkAchievements         (marshal, arrayPtr); break;
        case GA_T::DATA_SET_TIPS:                       WalkTips                 (marshal, arrayPtr); break;
        case GA_T::DATA_SET_EVENT_REWARDS:              WalkEventRewards         (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PLAYER_USED_RESOURCES:      WalkPlayerUsedResources  (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PRODUCTION_MAP_GAME_LIST:   WalkProductionMapGameList(marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICE_USED_ANIMSETS:       WalkDeviceUsedAnimsets   (marshal, arrayPtr); break;
        case GA_T::DATA_SET_HARDCODED_RESOURCES:        WalkHardcodedResources   (marshal, arrayPtr); break;
        case GA_T::DATA_SET_WORLD_OBJECTS:              WalkWorldObjects         (marshal, arrayPtr); break;
        case GA_T::DATA_SET_OBJECTIVE_BOTS:             WalkObjectiveBots        (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SOUND_CUES:                 WalkSoundCues            (marshal, arrayPtr); break;

        // virtual-dispatched (vtable target resolved in ctor disasm)
        case GA_T::DATA_SET_BLUEPRINTS:                 WalkBlueprints           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_LOOT_TABLES:                WalkLootTables           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_LOOT_TABLE_GROUPS:          WalkLootTableGroups      (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SKILL_GROUP_SETS:           WalkSkillGroupSets       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SKILL_GROUPS:               WalkSkillGroups          (marshal, arrayPtr); break;

        // deep-nested virtual-dispatched
        case GA_T::DATA_SET_SKILL_GROUP_SKILLS:         WalkSkillGroupSkills     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SUB_SKILLS:                 WalkSubSkills            (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BLUEPRINT_ITEMS:            WalkBlueprintItems       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BLUEPRINT_ITEM_MODS:        WalkBlueprintItemMods    (marshal, arrayPtr); break;
        case GA_T::DATA_SET_LOOT_TABLE_ITEMS:           WalkLootTableItems       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_LOOT_TABLE_GROUP_ITEMS:     WalkLootTableGroupItems  (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BLUEPRNT_MOD_EFFECT_GRPS:   WalkBlueprintModEffectGroups(marshal, arrayPtr); break;

        // v17: device/item/skill nested data sets that were previously walked by
        // the game but never captured — these feed the "device → effect group"
        // lookup, store pricing, per-mode fire classes, and skill-driven effects.
        case GA_T::DATA_SET_DEVICE_EFFECT_GROUPS:       WalkDeviceEffectGroups     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICE_MODE_EFFECT_GROUPS:  WalkDeviceModeEffectGroups (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICE_MODE_PROPERTIES:     WalkDeviceModeProperties   (marshal, arrayPtr); break;
        case GA_T::DATA_SET_DEVICE_ANIM_SET_GROUPS:     WalkDeviceAnimSetGroups    (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ITEM_EFFECT_GROUPS:         WalkItemEffectGroups       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ITEM_PROPS:                 WalkItemProps              (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ITEM_MESH_ASM_GROUPS:       WalkItemMeshAsmGroups      (marshal, arrayPtr); break;
        case GA_T::DATA_SET_PRICES:                     WalkPrices                 (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SKILL_EFFECT_GROUPS:        WalkSkillEffectGroups      (marshal, arrayPtr); break;

        // v18: complete-audit nested datasets. See .planning/asm-dat-audit/
        case GA_T::DATA_SET_ASM_MESH_ANIM_SETS:         WalkAsmMeshAnimSets        (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ASM_MESH_AUDIO_GROUPS:      WalkAsmMeshAudioGroups     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ASM_MESH_FXS:               WalkAsmMeshFxs             (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ASM_MESH_IMPACT_GRPS:       WalkAsmMeshImpactGrps      (marshal, arrayPtr); break;
        case GA_T::DATA_SET_ASM_MESH_MORPH_SETS:        WalkAsmMeshMorphSets       (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SPECIAL_FX_MATERIALS:       WalkSpecialFxMaterials     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SPECIAL_FX_PSC:             WalkSpecialFxPsc           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_SPECIAL_FX_SOUNDS:          WalkSpecialFxSounds        (marshal, arrayPtr); break;
        case GA_T::DATA_SET_MATERIAL_RESOURCES:         WalkMaterialResources      (marshal, arrayPtr); break;
        case GA_T::DATA_SET_MAT_RES_PARAMS:             WalkMatResParams           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_XP_CAPS:                    WalkXpCaps                 (marshal, arrayPtr); break;
        case GA_T::DATA_SET_QUEST_REQUIREMENTS:         WalkQuestRequirements      (marshal, arrayPtr); break;
        case GA_T::DATA_SET_QUEST_PREREQUISITES:        WalkQuestPrerequisites     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_QUEST_REQ_OBJECTIVES:       WalkQuestReqObjectives     (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BOT_BEHAVIORS:              WalkBotBehaviors           (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BOT_ACTIONS:                WalkBotActions             (marshal, arrayPtr); break;
        case GA_T::DATA_SET_BOT_TESTS:                  WalkBotTests               (marshal, arrayPtr); break;

        // v21: re-verification audit. See .planning/asm-dat-audit/REVERIFY_2026-05-02.md
        case GA_T::DATA_SET_ACHIEVEMENT_REQS:           WalkAchievementReqs        (marshal, arrayPtr); break;

        // v23: lang_English.dat translation table.
        case GA_T::DATA_SET_MSG_TRANSLATIONS:           WalkMessageTranslations    (marshal, arrayPtr); break;

        default: break;
    }
}
