#pragma once

#include <cstdint>
#include <string>

// PlayerBundle.hpp -- the mutable per-player rows a game instance needs, moved
// between the master and a node.
//
// Why this exists: the game DLL queries player data synchronously while playing
// (character, skills, equipped devices, inventory). On a remote node those rows
// have to be present locally, so the master ships them down before the instance is
// told about the player, and takes them back afterwards.
//
// The same code serves both directions because both processes have "a database"
// attached via Database::GetConnection():
//   * master Build(Down) -> PLAYER_BUNDLE -> node Apply(Down)   (before the register)
//   * node   Build(Up)   -> PLAYER_BUNDLE -> master Apply(Up)   (while/after playing)
// Rows keep their explicit ids in both directions, because
// ga_character_devices.inventory_id references ga_players_inventory.id and the DLL
// uses those ids directly.
//
// ga_users is DOWN ONLY, and only its id/username columns: the DLL just maps
// username -> id. Sending the password verifier to a volunteer's machine would be
// indefensible, and a node creating a user row locally would invent an id the
// master does not have.
//
// ga_user_preferences travels both ways, but only the keys the DLL owns may come
// back up: sys_site_id, solo_mode and the operator's dashboard keys are the
// master's, and a node's snapshot of them (taken when the player arrived) must
// never be able to revert a newer master write.

class PlayerBundle {
public:
    enum class Direction { Down, Up };

    // Which tables a bundle carries.
    //   All            -- everything kSpecs lists for the direction (register hand-off
    //                     and the periodic write-back).
    //   EquipAndSkills -- ga_character_devices + ga_character_skills only, sent down
    //                     again when the master re-writes one of them for a player
    //                     who is on a remote node. Both tables are always present,
    //                     even with zero rows, so the node's Down reconcile can
    //                     clear them.
    enum class Scope { All, EquipAndSkills };

    // Serialise the rows for one player. `character_id` may be 0 to ship only the
    // user-scoped tables. Returns false with `err` set on a database error.
    static bool Build(std::int64_t user_id, std::int64_t character_id,
                      Direction dir, std::string& json_out, std::string& err);
    static bool Build(std::int64_t user_id, std::int64_t character_id,
                      Direction dir, Scope scope,
                      std::string& json_out, std::string& err);

    // Upsert the bundle into the attached database. Rows are replaced by primary
    // key, and for the tables where the DLL deletes rows locally
    // (ga_character_devices / _skills, ga_players_inventory) rows that are no
    // longer present are removed, so a loadout change cannot leave ghosts behind.
    static bool Apply(const std::string& json_bundle, Direction dir, std::string& err);

    // Tables that are carried in each direction, for logging and tests.
    static std::string DescribeTables(Direction dir);
};
