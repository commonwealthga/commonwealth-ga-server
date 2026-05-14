#include "src/GameServer/Engine/PackageMap/Compute/PackageMap__Compute.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

// Filter out TgClient-rooted entries from PackageMap.List before Compute runs.
//
// Why:
//   GA_Menu_FA (and other forced exports under TgClient) carry per-parent-package
//   forced-export GUIDs that diverge from the standalone .upk header GUID. The
//   client's in-memory UPackage->Guid stamp depends on which parent first loaded
//   the forced export, and that stamp can mutate between map transitions while
//   the server's stays put — producing "Package 'X' version mismatch [package
//   Y, info Z]" errors on reconnect (and immediately when our FixForcedExportGuids
//   "fixes" the server side toward the disk-header GUID).
//
//   TgClient is the client's UI package tree (menus, HUD widgets, fonts). The
//   server never replicates any objects rooted there over the wire, so it has
//   no reason to advertise the tree in USES at all. Dropping it removes both
//   the failure mode and its root cause.
//
// Alignment:
//   PackageMap.List indices are used as the "package index" half of every
//   NetGUID the server serializes. If we filtered only the wire send while
//   keeping the server-internal list, every NetGUID for a package after
//   TgClient in the list would resolve to the wrong package on the client
//   (actors landing in wrong channels — the prior FixForcedExportGuids
//   regression). Filtering at Compute time — before the +0x48 lookup map is
//   rebuilt from List — keeps both consumers in sync: the wire path
//   (SendPackageMap walks List) and the runtime path (NetGUID serialization
//   reads +0x48, which Compute repopulates from filtered List).
//
//   Per-connection scope (PackageMap belongs to Connection at +0xBC), no
//   global UPackage mutation, no cross-session bleed. Compute is called once
//   at HELLO and once per HAVE message; filter is idempotent (TgClient
//   entries are gone after the first call, second call finds nothing to drop).

void __fastcall PackageMap__Compute::Call(void* PackageMap) {
	// Resolve "TgClient" FName once. FName interning means any FName for
	// "TgClient" anywhere in the process has the same Index, including the
	// ForcedExportBasePackageName slot we want to test against.
	static int sTgClientNameIdx = -1;
	if (sTgClientNameIdx == -1) {
		sTgClientNameIdx = FName("TgClient").Index;
	}

	char** ListPtr      = (char**)((char*)PackageMap + 0x3C);
	int*   ListCountPtr = (int*)  ((char*)PackageMap + 0x40);
	char*  List         = *ListPtr;
	int    ListCount    = *ListCountPtr;

	int writeIdx = 0;
	int dropped  = 0;

	for (int readIdx = 0; readIdx < ListCount; readIdx++) {
		char*    SrcEntry = List + readIdx * 0x44;
		UObject* Parent   = *(UObject**)(SrcEntry + 0x08);

		bool drop = false;
		if (Parent) {
			// Drop TgClient itself.
			const char* name = Parent->GetName();
			if (name && strcmp(name, "TgClient") == 0) {
				drop = true;
			} else {
				// Drop forced exports rooted at TgClient. UPackage stores its
				// ForcedExportBasePackageName at +0x90 (see FixForcedExportGuids
				// in MarshalChannel__NotifyControlMessage.cpp for the same field).
				FName* forcedBase = (FName*)((char*)Parent + 0x90);
				if (forcedBase->Index == sTgClientNameIdx) {
					drop = true;
				}
			}
		}

		if (drop) {
			dropped++;
		} else {
			if (writeIdx != readIdx) {
				memcpy(List + writeIdx * 0x44, SrcEntry, 0x44);
			}
			writeIdx++;
		}
	}

	if (dropped > 0) {
		*ListCountPtr = writeIdx;
		Logger::Log("packagemap",
			"[PackageMap::Compute] dropped %d TgClient-rooted entr%s; new count=%d (was %d)\n",
			dropped, dropped == 1 ? "y" : "ies", writeIdx, ListCount);
	}

	CallOriginal(PackageMap);
}
