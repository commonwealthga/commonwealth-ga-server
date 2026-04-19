#include "src/GameServer/TgGame/TgDeployable/IsFriendlyWithLocalPawn/TgDeployable__IsFriendlyWithLocalPawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Native "get local pawn" at 0x1096ebf0 — returns APawn* for the current
// viewer.  Same one the native IsFriendlyWithLocalPawn calls.
typedef APawn* (__cdecl* GetLocalPawnFn)(void);
static const GetLocalPawnFn GetLocalPawn = (GetLocalPawnFn)0x1096ebf0;

// Log the native's answer vs what our Instigator-chain comparison would say.
// Prior "force=1" test confirmed this hook IS on the render path — we just
// need to know why CallOriginal returns 0 despite the DRI's r_TaskforceInfo
// matching the Instigator's PRI.r_TaskForce.  The RESULT log will compare
// the native's answer against the local pawn's task force so we can see
// exactly which side is wrong.
unsigned int __fastcall TgDeployable__IsFriendlyWithLocalPawn::Call(
	ATgDeployable* deployable, void* edx)
{
	unsigned int orig = CallOriginal(deployable, edx);

	if (deployable && deployable->Class) {
		const char* cn = deployable->Class->GetFullName();
		if (cn && strstr(cn, "TgDeploy") != nullptr) {
			static std::unordered_map<ATgDeployable*, int> s_cnt;
			int& cnt = s_cnt[deployable];
			if ((cnt++ % 300) == 0) {
				APawn* localPawn = GetLocalPawn ? GetLocalPawn() : nullptr;
				ATgRepInfo_Player* localPri =
					localPawn ? (ATgRepInfo_Player*)localPawn->PlayerReplicationInfo : nullptr;
				ATgRepInfo_TaskForce* localTf = localPri ? localPri->r_TaskForce : nullptr;

				APawn* inst = (APawn*)deployable->Instigator;
				ATgRepInfo_Player* depPri =
					inst ? (ATgRepInfo_Player*)inst->PlayerReplicationInfo : nullptr;
				ATgRepInfo_TaskForce* depTf = depPri ? depPri->r_TaskForce : nullptr;

				ATgRepInfo_Deployable* dri = deployable->r_DRI;
				ATgRepInfo_TaskForce* driTf = dri ? dri->r_TaskforceInfo : nullptr;

				Logger::Log("team_colors",
					"[IsFriendlyWithLocalPawn CMP] deployable=0x%p id=%d  native_return=%u\n"
					"                        localPawn=0x%p  localPRI=0x%p  localTf=0x%p\n"
					"                        dep.Instigator=0x%p  depPRI=0x%p  depTf=0x%p\n"
					"                        dep.r_DRI=0x%p  dri.r_TaskforceInfo=0x%p\n"
					"                        instigator-chain match? %s   DRI match? %s\n",
					deployable, deployable->r_nDeployableId, orig,
					localPawn, localPri, localTf,
					inst, depPri, depTf,
					dri, driTf,
					(depTf && localTf && depTf == localTf) ? "YES (should be friendly)" : "NO",
					(driTf && localTf && driTf == localTf) ? "YES (should be friendly)" : "NO");
			}
		}
	}
	return orig;
}
