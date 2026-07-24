#include "src/GameServer/Audio/BotVoice/BotVoice.hpp"

#include "src/Database/Database.hpp"
#include "src/GameServer/Core/LoadObject/Core__LoadObject.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace BotVoice {
namespace {

// (bodyMeshAsmId, cueId) -> SoundCue. A cached nullptr is a NEGATIVE result:
// "already tried, don't re-query and re-attempt the load on every bark".
std::map<uint64_t, UObject*> s_cueCache;

inline uint64_t CacheKey(int asmId, int cueId) {
	return ((uint64_t)(uint32_t)asmId << 32) | (uint32_t)cueId;
}

// asm_data_set_sound_cues is keyed on (audio_group_id, sound_cue_id), and the
// group comes from the pawn's mesh — so the same slot id yields THIS bot's
// rendition. asm_data_set_resources maps the cue's res id to its package path
// (e.g. 4992 -> "VOX_B_NPC_Elites.A_CUE_Elite_Assn_Vox_Patrol"), which is what
// StaticLoadObject wants. Same res_id->asset pattern LookupPawnClassFromDb uses.
UObject* ResolveCue(int asmId, int cueId) {
	const uint64_t key = CacheKey(asmId, cueId);
	auto it = s_cueCache.find(key);
	if (it != s_cueCache.end()) return it->second;

	std::wstring path;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT r.name FROM asm_data_set_sound_cues c "
			"JOIN asm_data_set_asm_mesh_audio_groups g ON g.audio_group_id = c.audio_group_id "
			"JOIN asm_data_set_resources r ON r.res_id = c.sound_cue_res_id "
			"WHERE g.asm_id = ? AND c.sound_cue_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, asmId);
			sqlite3_bind_int(stmt, 2, cueId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const unsigned char* n = sqlite3_column_text(stmt, 0);
				if (n) {
					const std::string s(reinterpret_cast<const char*>(n));
					path.assign(s.begin(), s.end());   // asset paths are ASCII
				}
			}
			sqlite3_finalize(stmt);
		}
	}

	UObject* cue = nullptr;
	if (path.empty()) {
		Logger::Log("botvoice",
			"ResolveCue: no cue row for asm=%d slot=%d (this bot's audio group "
			"probably doesn't implement that slot)\n", asmId, cueId);
	} else {
		UClass* cueClass = ClassPreloader::GetClass("Class Engine.SoundCue");
		if (!cueClass) {
			Logger::Log("botvoice", "ResolveCue: Engine.SoundCue class not resident\n");
		} else {
			cue = (UObject*)Core__LoadObject::CallOriginal(
				cueClass, nullptr, const_cast<wchar_t*>(path.c_str()),
				nullptr, 0, nullptr, 0);
			Logger::Log("botvoice",
				"ResolveCue: asm=%d slot=%d -> '%ls' load=%p\n",
				asmId, cueId, path.c_str(), (void*)cue);
		}
	}

	s_cueCache[key] = cue;
	return cue;
}

}  // namespace

int LookupBodyAsmId(int botId) {
	static std::map<int, int> s_cache;
	auto it = s_cache.find(botId);
	if (it != s_cache.end()) return it->second;

	int asmId = 0;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT body_asm_id FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, botId);
			if (sqlite3_step(stmt) == SQLITE_ROW) asmId = sqlite3_column_int(stmt, 0);
			sqlite3_finalize(stmt);
		}
	}
	if (asmId == 0)
		Logger::Log("botvoice", "LookupBodyAsmId: no body asm for bot %d\n", botId);
	s_cache[botId] = asmId;
	return asmId;
}

bool PlayPitched(ATgPawn* pawn, int cueId, float pitch, int voiceAsmId) {
	if (!pawn || cueId == 0) return false;
	AWorldInfo* wi = pawn->WorldInfo;
	if (!wi) return false;

	// voiceAsmId > 0 = voice transplant: resolve the slot against that body
	// mesh's audio group instead of the pawn's own.
	UObject* cue = ResolveCue(voiceAsmId > 0 ? voiceAsmId : pawn->r_nBodyMeshAsmId, cueId);
	if (!cue) return false;

	static UFunction* fn = nullptr;
	if (!fn) {
		fn = (UFunction*)ObjectCache::Find(
			"Function Engine.PlayerController.Kismet_ClientPlaySound");
	}
	if (!fn) {
		Logger::Log("botvoice", "PlayPitched: Kismet_ClientPlaySound not found\n");
		return false;
	}

	// Hand-rolled Parms. The SDK-generated struct declares the two trailing
	// bools as `unsigned long :1` at explicit offsets 0x14/0x18; C++ packs both
	// into ONE dword, so the struct is short and every offset from 0x14 on
	// desyncs from what UC reads (reference_sdk_bitfield_params_bug). One
	// uint32_t per bool matches UC's real layout. Trailing AC is the UC local —
	// kept so the frame is the size UC expects.
	struct {
		void*    ASound;
		void*    SourceActor;
		float    VolumeMultiplier;
		float    PitchMultiplier;
		float    FadeInTime;
		uint32_t bSuppressSubtitles;
		uint32_t bSuppressSpatialization;   // 0 -> stays spatialized
		void*    AC;
	} parms = { (void*)cue, (void*)pawn, 1.0f, pitch, 0.0f, 0, 0, nullptr };

	int sent = 0;
	for (AController* c = wi->ControllerList; c; c = c->NextController) {
		if (!ObjectClassCache::ClassNameContains(c, "PlayerController")) continue;
		// Plain ProcessEvent — this is a script `reliable client` event, NOT a
		// native, so do NOT set FUNC_Native (0x400) the way the stub-native
		// wrappers do. Matches the ClientSetCinematicMode dispatch in
		// MarshalChannel__NotifyControlMessage.
		c->ProcessEvent(fn, &parms, nullptr);
		sent++;
	}
	return sent > 0;
}

}  // namespace BotVoice
