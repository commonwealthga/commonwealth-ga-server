#pragma once

#include <cstdint>
#include <string>

// ContentSnapshot.hpp -- the master's export of the immutable game data a node
// needs to run an instance.
//
// A node cannot run a match without the asm_data_set_* tables: the game DLL reads
// them synchronously while playing (effect groups on every hit, device modes,
// items, bots). They are also completely static, so the cheapest correct answer is
// to ship them once as their own SQLite file and let the node query it locally at
// full speed.
//
// What is IN the snapshot: every asm_* table, mod_data_set_*, obj_*, map_game_info
// and map_object_config. What is NOT: msg_translations (the DLL never queries it;
// it is ~1 MB of client-side text), the map_tg_* dump tables (the DLL's own
// MapDataDumper regenerates those per map), and everything ga_* (player and match
// data -- master-owned).
//
// The snapshot is content-addressed: ContentSnapshot::Ensure() rebuilds into a
// temp file every start, hashes it, and keeps the result only if that hash is not
// already on disk. Rebuild cost is a second or two at startup; always rebuilding
// means a schema or content change can never be missed by a cache key.

class ContentSnapshot {
public:
    struct Info {
        bool        ok = false;
        std::string hash;    // BLAKE2b-256, 64 lowercase hex chars
        std::string path;    // <dir>/content-<hash>.db
        std::int64_t size = 0;
        std::string error;   // set when !ok
    };

    // Build (or reuse) the snapshot under `dir`. Call once at master startup.
    // Failure is not fatal to the master -- a master with no nodes still works --
    // but it is logged and nodes cannot be given content until it succeeds.
    static Info Ensure(const std::string& db_path, const std::string& dir);

    static const Info& Current();

    // Read up to `len` bytes at `offset` for transfer to a node. Returns false
    // when the snapshot is missing or the range is out of bounds. Not thread-safe
    // against Ensure(); both are only ever called from the io thread.
    static bool ReadChunk(std::int64_t offset, std::int64_t len,
                          std::string& out, std::string& err);
};
