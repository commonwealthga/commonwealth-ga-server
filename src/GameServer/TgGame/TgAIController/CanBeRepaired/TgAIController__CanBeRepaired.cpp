#include "src/GameServer/TgGame/TgAIController/CanBeRepaired/TgAIController__CanBeRepaired.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Throttled log: this fires every AI tick on every bot, so unconditional
// logging would flood. One log per AI on first hit + sparse periodic logs.
static std::map<int, int> s_callCount;

// Sanity-check a candidate UObject*. Many of the controller's "target" fields
// can carry stale pointers to freed objects. Dereferencing those crashes us.
// We require: non-null, plausibly in user heap (> 0x100000), 4-byte aligned,
// AND the vtable pointer is in the game binary's code/data range.
static bool LooksLikeValidUObject(void* ptr) {
	uintptr_t p = (uintptr_t)ptr;
	if (p < 0x100000 || (p & 3) != 0) return false;
	uintptr_t vt = *(uintptr_t*)ptr;
	// GlobalAgenda.exe loads at 0x10900000; binary is well under 0x20000000.
	if (vt < 0x10000000 || vt > 0x20000000) return false;
	return true;
}

// Safely read class name + r_nPhysicalType from a potential pawn pointer.
static void DescribeTarget(void* ptr, char* outBuf, size_t bufSize) {
	if (!ptr) { snprintf(outBuf, bufSize, "<null>"); return; }
	if (!LooksLikeValidUObject(ptr)) {
		snprintf(outBuf, bufSize, "%p <bogus>", ptr);
		return;
	}
	UObject* obj = (UObject*)ptr;
	void* cls = obj->Class;
	const char* clsName = "<no-class>";
	if (cls && LooksLikeValidUObject(cls)) {
		clsName = ((UObject*)cls)->GetFullName();
	}
	int phys_pawn = *(int*)((char*)ptr + 0x6D8);
	int phys_dep  = *(int*)((char*)ptr + 0x1D8);
	snprintf(outBuf, bufSize, "%p (%s) phys[+6D8]=%d phys[+1D8]=%d",
		ptr, clsName, phys_pawn, phys_dep);
}

int __fastcall TgAIController__CanBeRepaired::Call(ATgAIController* aic, void* edx) {
	// Run the real impl first. It calls vtable[0x440] on `this` to retrieve
	// SOME target reference, then checks target->r_nPhysicalType for {861, 973}.
	int real = CallOriginal(aic, edx);

	if (!aic) return 1;

	ATgPawn* pawn = aic->Pawn ? (ATgPawn*)aic->Pawn : nullptr;
	if (!pawn || !pawn->r_bIsBot) {
		return real;
	}

	int& n = s_callCount[(int)aic];
	n++;

	// Throttle by ~1Hz per controller so we capture state evolution without flooding.
	if (n == 1 || (n % 30) == 0) {
		char* p = (char*)aic;
		// Controller's various target/enemy/focus fields. Names from Engine SDK
		// AController + TgAIController.
		void* enemy           = *(void**)(p + 0x0330);   // AController::Enemy
		void* shotTarget      = *(void**)(p + 0x0304);   // AController::ShotTarget
		void* focus           = *(void**)(p + 0x0258);   // AController::Focus
		void* moveTarget      = *(void**)(p + 0x036C);   // m_pMovementTarget
		void* lastAttacker    = *(void**)(p + 0x03A4);   // m_pLastAttacker
		void* cmdTarget       = *(void**)(p + 0x03E0);   // m_pCommandTarget
		void* triggerTarget   = *(void**)(p + 0x0458);   // m_pTriggerTarget
		int   cachedPhys      = *(int*) (p + 0x04D8);    // m_nTargetPhysicalType

		char enemyDesc[256], shotDesc[256], focusDesc[256];
		DescribeTarget(enemy, enemyDesc, sizeof(enemyDesc));
		DescribeTarget(shotTarget, shotDesc, sizeof(shotDesc));
		DescribeTarget(focus, focusDesc, sizeof(focusDesc));

		Logger::Log("can_be_repaired",
			"[CanBeRepaired call#%d] aic=%p self=%s real=%d cachedPhys=%d\n"
			"    Enemy        = %s\n"
			"    ShotTarget   = %s\n"
			"    Focus        = %s\n"
			"    MoveTgt=%p  CmdTgt=%p  TrigTgt=%p  LastAttacker=%p\n",
			n, (void*)aic, pawn->GetFullName(), real, cachedPhys,
			enemyDesc, shotDesc, focusDesc,
			moveTarget, cmdTarget, triggerTarget, lastAttacker);
	}

	return 1;
}
