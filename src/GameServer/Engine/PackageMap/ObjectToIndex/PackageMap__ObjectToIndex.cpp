#include "src/GameServer/Engine/PackageMap/ObjectToIndex/PackageMap__ObjectToIndex.hpp"

#include "src/Utils/Logger/Logger.hpp"

#include <string>

// See hpp. Confirmed culprit (2026-07-25 playtest): SetPhase 0->1 sends
// "SoundCue SND_WEP_Boss_ThinkTank.A_Cue_WEP_Boss_ThinkTank_Shield_R1_Activate_Global"
// with netIndex=46 while the package has 40 static exports (the cue exists on
// disk as export 5 — the serialized instance is a runtime-created DUPLICATE).
// Client-side CreateExport(46) then asserts Array.h:560 and kills the client.
//
// Guard: any object whose NetIndex is past its own linker's ExportMap count
// cannot exist on the client's disk, so serialize it as NULL (-1) instead of
// poisoning the client. Legit exports always have NetIndex < their linker's
// export count, and objects without a linker (level actors, script objects
// under seekfree linker detach) are untouched. This keeps clients alive; the
// root fix is stopping whatever creates the duplicate cues (see memory:
// project_thinktank_boss_client_crash).
int __fastcall PackageMap__ObjectToIndex::Call(void* PackageMap, void* edx, UObject* Obj) {
	const int result = CallOriginal(PackageMap, edx, Obj);

	if (result != -1 && Obj != nullptr) {
		void* linker = (void*)Obj->Linker;
		if (linker != nullptr) {
			// ULinkerLoad::ExportMap count (linker+0xDC; data at +0xD8) — the
			// same bound the client-side CreateExport range assert enforces.
			const int exportCount = *(const int*)((const char*)linker + 0xDC);
			// A genuine on-disk export (ExportMap[LinkerIndex].Object == Obj,
			// stride 0x5C, Object at +0x30 - see FireSockets RawExport) is
			// loaded by the client from the same file with the same NetIndex,
			// even when that NetIndex is past the cooked export count (seekfree
			// device/cinematic packages: Skl_*_Jetpack, Wep_AssaultAxe meshes,
			// intro cutscene meshes). Only objects that are NOT their linker's
			// export (runtime duplicates like the ThinkTank cue) are poison.
			const int linkerIndex = *(const int*)((const char*)Obj + 0x20);
			bool genuine = false;
			if (linkerIndex >= 0 && linkerIndex < exportCount) {
				const char* exports = *(const char* const*)((const char*)linker + 0xD8);
				genuine = exports && *(UObject* const*)(exports + linkerIndex * 0x5C + 0x30) == Obj;
			}
			if (Obj->NetIndex >= exportCount && genuine) {
				if (Logger::IsChannelEnabled("packagemap")) {
					char* raw = Obj->GetFullName();
					Logger::Log("packagemap",
						"[ObjectToIndex] past-count genuine export ALLOWED obj=%s netIndex=%d exportCount=%d linkerIndex=%d\n",
						raw ? raw : "<null>", Obj->NetIndex, exportCount, linkerIndex);
				}
			} else if (Obj->NetIndex >= exportCount) {
				if (Logger::IsChannelEnabled("packagemap")) {
					char* raw = Obj->GetFullName();
					const std::string name(raw ? raw : "<null>");
					Logger::Log("packagemap",
						"[ObjectToIndex] POISON REF SUPPRESSED obj=%s netIndex=%d exportCount=%d wireIndex=%d linkerIndex=%d\n",
						name.c_str(), Obj->NetIndex, exportCount, result, linkerIndex);
				}
				return -1;
			}
		}
	}

	return result;
}
