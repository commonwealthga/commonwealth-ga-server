// Client-side implementation of UObject__ProcessEvent::Call.
//
// The server-side cpp in this directory (UObject__ProcessEvent.cpp) carries a
// lot of machinery that only makes sense on the authoritative side
// (SetPawnProperty, stealth effect-group refresh, bot-death factory notifies,
// etc.) and pulling all of that into the client DLL drags in sqlite3 and
// every effect-group native just to satisfy the linker.  The client only
// needs generic replicated-event logging + unconditional CallOriginal.
//
// The Makefile picks which cpp lands in which DLL: server uses
// UObject__ProcessEvent.cpp, client uses this file.  Both provide
// `UObject__ProcessEvent::Call` satisfying the shared .hpp.

#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/Utils/Logger/Logger.hpp"

// bDisableGarbageCollection is declared in the shared hpp; the server cpp
// owns its own definition.  Define it here too (client build) since the
// client binary doesn't link the server cpp.
bool UObject__ProcessEvent::bDisableGarbageCollection = false;

void __fastcall UObject__ProcessEvent::Call(
	UObject* Object, void* edx, UFunction* Function, void* Params, void* Result)
{
	if (Object && Function && Params) {
		// GetFullName() uses a shared static buffer — two raw const-char*
		// pointers from back-to-back calls alias the same memory
		// (reference_getfullname_shared_buffer.md).  Copy into std::string
		// before using both.
		const char* fnRaw = Function->GetFullName();
		std::string fnName = fnRaw ? fnRaw : "";
		const char* objRaw = Object->GetFullName();
		std::string objName = objRaw ? objRaw : "";

		// Log every Actor.ReplicatedEvent call with its VarName.  Param
		// offset 0 is FName VarName — see AActor_eventReplicatedEvent_Parms.
		// Channel `replicated_event` enabled by dllmainclient.
		static const std::string kSuffix = ".ReplicatedEvent";
		if (fnName.size() >= kSuffix.size()
			&& fnName.compare(fnName.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
			FName* VarName = (FName*)Params;
			char* varStr = VarName->GetName();
			Logger::Log("replicated_event", "RE: var=%s  obj=%s  fn=%s\n",
				varStr ? varStr : "<null>", objName.c_str(), fnName.c_str());

			// Extra diagnostic for DRI repnotifies: log the DRI's entire
			// team/owner state at the exact moment ReplicatedEvent fires so
			// we can tell whether r_DeployableOwner resolved to a real
			// actor (which is what the UC guards require before calling
			// NotifyGroupChanged on the deployable).  Channel `team_colors`.
			if (objName.find("TgRepInfo_Deployable") != std::string::npos
				|| objName.find("TgRepInfo_Beacon") != std::string::npos) {
				ATgRepInfo_Deployable* dri = (ATgRepInfo_Deployable*)Object;
				// DRI bitfield dword at +0x1EC:
				//   bit 0 (0x01): r_bOwnedByTaskforce
				//   bit 1 (0x02): c_bReceivedOwner ← the flag that guards
				//                 NotifyGroupChanged dispatch from the DRI's
				//                 ReplicatedEvent handler.
				unsigned int bits = *(unsigned int*)((char*)dri + 0x1EC);
				Logger::Log("team_colors",
					"[DRI ReplicatedEvent] var=%s  dri=0x%p\n"
					"                   r_bOwnedByTaskforce=%d  c_bReceivedOwner=%d\n"
					"                   r_TaskforceInfo=0x%p  r_InstigatorInfo=0x%p  r_DeployableOwner=0x%p\n",
					varStr ? varStr : "<null>", dri,
					(int)(bits & 0x01), (int)((bits >> 1) & 0x01),
					dri->r_TaskforceInfo, dri->r_InstigatorInfo,
					dri->r_DeployableOwner);
			}
		}
	}

	CallOriginal(Object, edx, Function, Params, Result);
}
