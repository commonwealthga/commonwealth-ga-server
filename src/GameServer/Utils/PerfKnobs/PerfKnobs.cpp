#include "src/GameServer/Utils/PerfKnobs/PerfKnobs.hpp"

#include "src/pch.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace PerfKnobs {

namespace {

struct Knob {
	const char* key;
	int value;
	int def;
	int lo, hi;
};

Knob g_knobs[] = {
	{"perf_iscrewed_native", 1, 1, 0, 2},
	{"perf_aictrl_lod",      1, 1, 0, 1},
};
DWORD g_readAt = 0;
bool g_read = false;

void Refresh() {
	const DWORD now = GetTickCount();
	if (g_read && now - g_readAt < 60000) return;
	g_read = true;
	g_readAt = now;
	sqlite3* db = Database::GetConnection();
	if (db == nullptr) return;
	for (Knob& k : g_knobs) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?", -1, &st, nullptr) != SQLITE_OK) return;
		sqlite3_bind_text(st, 1, k.key, -1, SQLITE_STATIC);
		int v = k.def;
		if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) v = sqlite3_column_int(st, 0);
		sqlite3_finalize(st);
		if (v < k.lo || v > k.hi) v = k.def;
		if (v != k.value) Logger::Log("perf", "[PerfKnobs] %s=%d\n", k.key, v);
		k.value = v;
	}
}

}  // namespace

int IsCrewedNative() { Refresh(); return g_knobs[0].value; }
int AiCtrlLod()      { Refresh(); return g_knobs[1].value; }

}  // namespace PerfKnobs
