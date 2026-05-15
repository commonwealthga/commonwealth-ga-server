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
