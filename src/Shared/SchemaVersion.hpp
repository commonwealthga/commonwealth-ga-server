#pragma once

// One schema version for the whole project.
//
// The control server owns the database schema. It installs `dbbase/base.db` on a
// database that has no schema yet, then applies `migrations/NNNN-*.sql` files in
// ascending order. The game DLL never migrates: it reads this constant only to
// complain loudly when it is pointed at a database older than the code expects.
//
// Before this existed, the DLL (target 173) and the control server (floor-write
// 19) both migrated the same database through the same `version_info` counter.
// A database created by the control server therefore had the DLL's entire
// migration ladder replayed against it on every instance start, including
// `INSERT OR REPLACE` seeds that reset every column they did not mention.
namespace Schema {
// Bump this in the same commit as any migration that adds migrations/NNNN-*.sql:
// it is what the game DLL compares a database against before trusting it, and
// what the control server warns about when bootstrap could not run.
constexpr int kVersion = 175;
}
