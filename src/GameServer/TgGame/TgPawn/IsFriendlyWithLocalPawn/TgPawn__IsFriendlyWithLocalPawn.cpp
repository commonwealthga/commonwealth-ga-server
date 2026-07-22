#include "src/GameServer/TgGame/TgPawn/IsFriendlyWithLocalPawn/TgPawn__IsFriendlyWithLocalPawn.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Channel: `team_colors` — matches the existing TgDeployable diagnostic
// channel so the deployable + pawn pictures stay readable in one grep.

unsigned int __fastcall TgPawn__IsFriendlyWithLocalPawn::Call(ATgPawn* pawn, void* edx)
{
	unsigned int orig = CallOriginal(pawn, edx);

	if (Logger::IsChannelEnabled("team_colors") && pawn) {
		static std::unordered_map<ATgPawn*, int> s_cnt;
		int& cnt = s_cnt[pawn];
		if ((cnt++ % 300) == 0) {
			ATgPlayerController* localPC = pawn->c_LocalPC;
			APawn* localPawn = localPC ? localPC->Pawn : nullptr;
			APlayerReplicationInfo* localPri = localPC ? localPC->PlayerReplicationInfo : nullptr;
			APlayerReplicationInfo* targetPri = pawn->PlayerReplicationInfo;

			// Resolve "other"'s task force the same way the native does: prefer
			// the local pawn's PRI, fall back to the local PC's own PRI when
			// there's no local pawn (our spectator's exact case).
			ATgRepInfo_TaskForce* localTf = nullptr;
			if (localPawn) {
				ATgRepInfo_Player* p = (ATgRepInfo_Player*)localPawn->PlayerReplicationInfo;
				localTf = p ? p->r_TaskForce : nullptr;
			} else if (localPri) {
				localTf = ((ATgRepInfo_Player*)localPri)->r_TaskForce;
			}
			ATgRepInfo_TaskForce* targetTf = targetPri ? ((ATgRepInfo_Player*)targetPri)->r_TaskForce : nullptr;

			Logger::Log("team_colors",
				"[TgPawn IsFriendlyWithLocalPawn] target=0x%p class=%s  native_return=%u\n"
				"                            localPC=0x%p  localPC.Pawn=0x%p  localPC.PRI=0x%p  localTf=0x%p\n"
				"                            target.PRI=0x%p  targetTf=0x%p  sameTf=%s\n",
				pawn, ObjectClassCache::GetClassName(pawn).c_str(), orig,
				(void*)localPC, (void*)localPawn, (void*)localPri, (void*)localTf,
				(void*)targetPri, (void*)targetTf,
				(localTf && targetTf && localTf == targetTf) ? "YES" : "NO");
		}
	}

	return orig;
}
