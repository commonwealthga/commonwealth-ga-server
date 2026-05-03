#include "src/GameServer/TgGame/TgPawn/RosterWalker/TgPawn__RefIter.hpp"
#include "src/Utils/Logger/Logger.hpp"

// FUN_11326f90 is UE3's garbage collector (UObject::CollectGarbage, per
// `.\Src\UnObjGC.cpp` references in the decompile). Crash variant at
// 0x11327865 reads `[EAX+8]` with EAX=0x1d5 — some UObject* field of some
// live object contains the integer 469 instead of a valid pointer.
//
// An earlier scanner version iterated GObjObjects and called GetFullName()
// on each; that crashed the home map (likely freed objects in GObjObjects
// slots, or GetFullName's shared buffer is not re-entrant at GC entry).
// Reverted to minimal entry logging; root-cause analysis continues in
// source review rather than runtime scans.

void __fastcall TgPawn__RefIter::Call(void* pThis, void* edx,
                                      unsigned p1, unsigned p2) {
    CallOriginal(pThis, edx, p1, p2);
}
