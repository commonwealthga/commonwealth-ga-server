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
			if (Obj->NetIndex >= exportCount) {
				if (Logger::IsChannelEnabled("packagemap")) {
					char* raw = Obj->GetFullName();
					const std::string name(raw ? raw : "<null>");
					Logger::Log("packagemap",
						"[ObjectToIndex] POISON REF SUPPRESSED obj=%s netIndex=%d exportCount=%d wireIndex=%d\n",
						name.c_str(), Obj->NetIndex, exportCount, result);
				}
				return -1;
			}
		}
	}

	return result;
}
