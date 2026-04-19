#include "src/GameServer/TgGame/TgRepInfo_Game/GetTaskForceFor/TgRepInfo_Game__GetTaskForceFor.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Channel: `team_colors` — matches the existing server-side diagnostic channel
// so the full server+client picture stays readable in one grep.  Client DLL
// enables it via dllmainclient.cpp.

ATgRepInfo_TaskForce* __fastcall TgRepInfo_Game__GetTaskForceFor::Call(
	ATgRepInfo_Game* pThis, void* edx, AActor* target)
{
	// Unconditional entry log (rate-limited) so we can see if this hook
	// fires at all for deployables.  If we never see a deployable target
	// here, the client's friendship check isn't actually routing through
	// this function — need to find the real one.
	static int s_entryCount = 0;
	if ((s_entryCount++ % 300) == 0) {
		Logger::Log("team_colors",
			"[GetTaskForceFor ENTRY] target=0x%p class=%s  (call #%d)\n",
			target,
			(target && target->Class) ? target->Class->GetFullName() : "<null>",
			s_entryCount);
	}

	ATgRepInfo_TaskForce* orig = CallOriginal(pThis, edx, target);

	// Deployable-targeted calls: log what the native returned + DRI state so
	// we can see whether the "original returned non-null" case gives us the
	// correct task force or the wrong one.  Rate-limited per target.
	if (target && target->Class) {
		const char* targetClass = target->Class->GetFullName();
		if (targetClass && strstr(targetClass, "TgDeploy") != nullptr) {
			static std::unordered_map<AActor*, int> s_cnt;
			int& cnt = s_cnt[target];
			if ((cnt++ % 120) == 0) {
				ATgDeployable* dep = (ATgDeployable*)target;
				ATgRepInfo_Deployable* dri = dep->r_DRI;
				unsigned int driBits = dri ? *(unsigned int*)((char*)dri + 0x1EC) : 0;
				APawn* inst = (APawn*)dep->Instigator;
				ATgRepInfo_Player* instPri = inst ? (ATgRepInfo_Player*)inst->PlayerReplicationInfo : nullptr;
				Logger::Log("team_colors",
					"[GetTaskForceFor RESULT] target=0x%p id=%d  orig=0x%p  r_DRI=0x%p\n"
					"                     DRI.r_bOwnedByTaskforce=%d  r_TaskforceInfo=0x%p  r_InstigatorInfo=0x%p\n"
					"                     deployable.Instigator=0x%p  Instigator.PRI=0x%p  PRI.r_TaskForce=0x%p\n",
					target, dep->r_nDeployableId, orig, dri,
					dri ? (int)(driBits & 1) : -1,
					dri ? dri->r_TaskforceInfo : nullptr,
					dri ? dri->r_InstigatorInfo : nullptr,
					inst, instPri,
					instPri ? instPri->r_TaskForce : nullptr);
			}
		}
	}

	if (orig) return orig;  // Native path worked — use that answer.

	if (!target || !target->Class) return nullptr;

	// Only re-route deployables.  Prefix match on the class full name because
	// SDK StaticClass() is unreliable on this binary
	// (reference_sdk_staticclass_misalignment.md).  Covers TgDeployable, all
	// TgDeploy_* subclasses.
	const char* clsName = target->Class->GetFullName();
	if (!clsName) return nullptr;
	if (strstr(clsName, "TgDeploy") == nullptr) return nullptr;

	// Walk Instigator → PlayerReplicationInfo → r_TaskForce.  All three are
	// CPF_Net and verified as replicating reliably on the client (Instigator
	// is AActor +0xD4, PRI replicates via PlayerController owner chain,
	// r_TaskForce is in the PRI's rep block and fires repnotify client-side).
	APawn* inst = (APawn*)target->Instigator;
	if (!inst) return nullptr;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)inst->PlayerReplicationInfo;
	if (!pri) return nullptr;
	ATgRepInfo_TaskForce* tf = pri->r_TaskForce;

	Logger::Log("team_colors",
		"[GetTaskForceFor fallback] target=%s  Instigator=%s  PRI=0x%p  tf=0x%p\n",
		clsName,
		inst->GetFullName() ? inst->GetFullName() : "<null>",
		pri, tf);

	return tf;  // nullptr if r_TaskForce hasn't replicated yet — native fallback
}
