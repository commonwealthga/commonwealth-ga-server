#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Sister hook to TgPawn__RosterWalker. Covers the OTHER recurring crash
// variant at EIP=0x11327865 (MOV ECX, [EAX+8] with EAX=0x1d5), inside
// FUN_11326f90 — UE3 internal ref iterator (GC / serializer family).
//
// Same filtered-logging strategy: only emit when `this` looks implausible,
// capture caller return address so the crash dump names the upstream site
// that passed the bad pointer.
class TgPawn__RefIter : public HookBase<
    void(__fastcall*)(void* /*this*/, void* /*edx*/,
                      unsigned /*p1*/, unsigned /*p2*/),
    0x11326f90,
    TgPawn__RefIter> {
public:
    static void __fastcall Call(void* pThis, void* edx,
                                unsigned p1, unsigned p2);
    static inline void __fastcall CallOriginal(void* pThis, void* edx,
                                               unsigned p1, unsigned p2) {
        m_original(pThis, edx, p1, p2);
    }
};
