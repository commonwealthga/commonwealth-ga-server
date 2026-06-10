#include "src/GameServer/TgGame/TgEffectSensor/GetSensorVisConfig/TgEffectSensor__GetSensorVisConfig.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

namespace {
struct VisConfig { int see; int display; };
// Keyed by visibility_config_id; the table is 13 static rows, cache forever.
std::unordered_map<int, VisConfig> g_visConfigCache;

bool LookupVisConfig(int cfgId, VisConfig& out) {
	auto it = g_visConfigCache.find(cfgId);
	if (it != g_visConfigCache.end()) {
		out = it->second;
		return out.see != -1;
	}

	VisConfig vc{ -1, -1 };
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(db,
			"SELECT see_flags, display_flags FROM asm_data_set_visibility_configs "
			"WHERE visibility_config_id = ? LIMIT 1",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, cfgId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				vc.see     = sqlite3_column_int(stmt, 0);
				vc.display = sqlite3_column_int(stmt, 1);
			}
			sqlite3_finalize(stmt);
		}
	}
	g_visConfigCache[cfgId] = vc;
	out = vc;
	return vc.see != -1;
}
}

void __fastcall TgEffectSensor__GetSensorVisConfig::Call(
	UTgEffectSensor* Effect, void* edx, int* outSeeFlag, int* outDisplayFlag)
{
	CallOriginal(Effect, edx, outSeeFlag, outDisplayFlag);  // stub no-op

	if (outSeeFlag)     *outSeeFlag = 0;
	if (outDisplayFlag) *outDisplayFlag = 0;
	if (!Effect) return;

	const int cfgId = (int)Effect->m_fBase;  // prop-221 effect: base_value = visibility_config_id
	VisConfig vc;
	const bool found = LookupVisConfig(cfgId, vc);
	if (found) {
		if (outSeeFlag)     *outSeeFlag = vc.see;
		if (outDisplayFlag) *outDisplayFlag = vc.display;
	}

	// Marker-radius cap. The client's LocalProcessScanner stamps markers in a
	// sphere of Scanner_Range RAW unreal units (no ft→uu ×16 — only AI
	// SightRadius gets ×16). Targeting System / Visual Scanner carry range
	// 3000 → 3000uu... but their data unit is feet, so the HUD marker sphere
	// effectively covers every relevant pawn. asm_* data is read-only, so cap
	// the replicated Scanner_Range server-side for marker-bearing configs
	// (display flags non-zero); AI-vision-only configs are left alone.
	// 1600uu ≈ 32 m ≈ "nearby". Prop-13 ran before prop-221 in row order, so
	// the slot's range is already written when we run.
	const int kMarkerRangeCapUU = 1600;
	if (found && vc.display != 0 && Effect->m_EffectGroup) {
		ATgPawn* Target = (ATgPawn*)Effect->m_EffectGroup->m_Target;
		const int egId = Effect->m_EffectGroup->m_nEffectGroupId;
		if (Target) {
			for (int i = 0; i < 2; i++) {
				FScanner_Settings& S = Target->r_ScannerSettings[i];
				if (S.EffectGroupId == egId && S.Scanner_Range > kMarkerRangeCapUU) {
					Logger::Log("stealth", "[sensorvis] cap pawn=%d slot=%d range %d -> %d\n",
						Target->r_nPawnId, i, S.Scanner_Range, kMarkerRangeCapUU);
					S.Scanner_Range = kMarkerRangeCapUU;
					Target->bNetDirty = 1;
				}
			}
		}
	}

	Logger::Log("stealth", "[sensorvis] cfg=%d found=%d see=0x%X display=0x%X\n",
		cfgId, (int)found, found ? vc.see : 0, found ? vc.display : 0);
}
