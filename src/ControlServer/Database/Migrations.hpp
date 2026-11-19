#pragma once

#include <string>

struct sqlite3;

namespace Migrations {

struct Result {
    bool        ok             = false;
    bool        installed_base = false;
    int         from_version   = 0;
    int         to_version     = 0;
    int         applied        = 0;
    std::string error;
};

// Brings `db` up to the current schema version:
//   1. if the database has no schema at all, copy `base_path` over it;
//   2. read version_info;
//   3. apply every compiled-in migration whose version is above the recorded one,
//      in ascending order, one transaction each.
//
// The migrations are embedded at build time (EmbeddedMigrations.gen.cpp), so a
// deployed binary needs no .sql files beside it. `base_path` is still a file: it
// is a 7 MB database, not a script.
//
// `db_path` is used only for logging. The caller owns the connection.
Result Run(sqlite3* db, const std::string& db_path, const std::string& base_path);

}  // namespace Migrations
