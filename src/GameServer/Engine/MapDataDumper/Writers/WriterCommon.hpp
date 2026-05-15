#pragma once
#include "src/pch.hpp"
#include <string>

// Shared helpers used by every map_* table writer. Render UE names/strings,
// strip the package prefix off Class->GetFullName(), and bind the 11 common
// columns shared by every map_<class> table.
namespace MapDumpWriters {

// Render an FName as its name-table string, or "(none)/(oob)/(null)" sentinels.
// Returned pointer is into a shared static buffer; caller must SQLITE_TRANSIENT
// when binding to SQL.
const char* FNameStr(const FName& fn);

// Convert an FString (wide chars) to UTF-8 std::string. Returns "" for empty
// or "(non-utf8)" if conversion fails.
std::string FStringToUtf8(const FString& s);

// Strip "Class <package>." / "<package>." prefix from Class->GetFullName(),
// returning just the bare class name ("TgMissionObjective_Bot").
std::string ExtractClassName(const char* fullName);

// Bind the 11 common columns (positions 1..11). First own-scalar bind index is 12.
//
//   1  map_name        TEXT
//   2  map_object_id   INTEGER
//   3  class_name      TEXT
//   4  tag             TEXT (FName -> str)
//   5  "group"         TEXT (FName -> str)
//   6  location_x      REAL
//   7  location_y      REAL
//   8  location_z      REAL
//   9  rotation_pitch  INTEGER
//   10 rotation_yaw    INTEGER
//   11 rotation_roll   INTEGER
void BindCommonCols(sqlite3_stmt* stmt, AActor* actor, int mapObjectId,
                    const std::string& mapName, const std::string& className);

}  // namespace MapDumpWriters
