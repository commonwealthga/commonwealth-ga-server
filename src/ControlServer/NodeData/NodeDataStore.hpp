#pragma once

#include <string>

// NodeDataStore.hpp -- brings a node's own database up to the master's content
// snapshot.
//
// The design doc originally had the content tables exposed as read-only VIEWs over
// an attached snapshot. That does not work: the game DLL WRITES several of them
// at runtime (asm_data_set_devices / _effects / _effect_groups / _bots and
// asm_data_set_devices_data_set_device_modes are caches its marshal hooks fill in,
// and map_tg_* is regenerated per map by MapDataDumper). Read-only views would
// make every one of those writes fail.
//
// So the snapshot is COPIED into the node's database as ordinary writable tables:
// one ~5 MB write per content version, per node, and afterwards the node's DB is a
// completely standalone server.db-shaped file. That is also why the node needs no
// DATABASE change to work -- from the DLL's point of view nothing about this
// exists.
//
// Not safe to run against a database that live instances are using: it drops and
// refills tables they are reading. The caller gates it on "no instances running".

class NodeDataStore {
public:
    // Replace the content tables in the node's database with the snapshot's.
    // No-op when the stored hash already matches `content_hash`.
    // Uses the process-wide Database::GetConnection(), so Database::Init() must
    // have run first.
    static bool SyncContent(const std::string& content_db_path,
                            const std::string& content_hash,
                            std::string& err);

    // Content hash currently applied to the node's database; "" when none.
    static std::string AppliedContentHash();
};
