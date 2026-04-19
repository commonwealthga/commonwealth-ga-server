#include "src/GameServer/TgGame/TgDeployable/NotifyGroupChanged/TgDeployable__NotifyGroupChanged.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Diagnostic tap — does not modify behaviour, just logs state and calls original.
// Fires on both server and client since the hook is installed in both DLLs,
// but we only care about the client (server doesn't render).  The channel name
// is `heal_tick` to reuse the existing enable flag.
void __fastcall TgDeployable__NotifyGroupChanged::Call(ATgDeployable* Deployable, void* edx) {
	if (Deployable) {
		unsigned int bits = *(unsigned int*)((char*)Deployable + 0x1D0);
		ATgRepInfo_Deployable* dri = Deployable->r_DRI;

		// Guard check: NotifyGroupChanged early-returns unless c_bInitialized
		// (bit 0) AND (NOT m_bInDestroyedState (bit 5) OR s_bDestroyedThisTick
		// (bit 6)).  Logging the inputs lets us tell whether the early-return
		// fires (and why).
		bool c_bInitialized        = (bits & 0x01) != 0;
		bool m_bInDestroyedState   = (bits & 0x20) != 0;
		bool s_bDestroyedThisTick  = (bits & 0x40) != 0;
		bool r_bInitialIsEnemy     = (bits & 0x00100000) != 0;
		bool guardPasses           = c_bInitialized && (!m_bInDestroyedState || s_bDestroyedThisTick);

		Logger::Log("heal_tick",
			"[NotifyGroupChanged] deployable=0x%p id=%d class=%s  guard=%s\n"
			"                   c_bInitialized=%d  m_bInDestroyedState=%d  s_bDestroyedThisTick=%d  r_bInitialIsEnemy=%d\n"
			"                   r_DRI=0x%p  c_Mesh=0x%p\n",
			Deployable, Deployable->r_nDeployableId,
			Deployable->Class ? Deployable->Class->GetFullName() : "<null>",
			guardPasses ? "PASS" : "FAIL",
			(int)c_bInitialized, (int)m_bInDestroyedState,
			(int)s_bDestroyedThisTick, (int)r_bInitialIsEnemy,
			dri, Deployable->c_Mesh);

		if (dri) {
			// r_bOwnedByTaskforce is bit 0 of the dword at +0x1EC on the DRI.
			bool r_bOwnedByTaskforce = (*(unsigned int*)((char*)dri + 0x1EC) & 0x01) != 0;
			Logger::Log("heal_tick",
				"                   DRI.r_bOwnedByTaskforce=%d  r_TaskforceInfo=0x%p  r_InstigatorInfo=0x%p  r_DeployableOwner=0x%p\n",
				(int)r_bOwnedByTaskforce, dri->r_TaskforceInfo, dri->r_InstigatorInfo,
				dri->r_DeployableOwner);
		}
	}

	CallOriginal(Deployable, edx);
}
