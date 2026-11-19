#pragma once

// Migrations are compiled in: scripts/embed_migrations.py turns migrations/*.sql
// into EmbeddedMigrations.gen.cpp at build time, so a deployed control server
// carries its own migrations and cannot be started without them. Only
// `dbbase/base.db` remains a runtime file, and only for a database that does not
// exist yet.
namespace EmbeddedMigrations {

struct Migration {
    int         version;   // the version the database is at after this file runs
    const char* name;      // file name, for logging
    const char* sql;       // file contents
};

extern const Migration kMigrations[];
extern const int       kCount;

}  // namespace EmbeddedMigrations
