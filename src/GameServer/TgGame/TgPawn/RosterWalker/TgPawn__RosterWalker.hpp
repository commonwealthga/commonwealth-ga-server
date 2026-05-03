#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// The recurring crash function FUN_11326f90 (body 0x11326f90 - 0x1132874c).
// Signature per Ghidra: __thiscall(void* this, uint param_1, uint param_2).
//
// Crash is at 0x11327865:
//     MOV ECX, dword ptr [EAX + 0x8]   ; EAX=0x1d5 (loaded from [EDI])
// which reads at 0x1d5+8=0x1dd — matches the "read at 0x1dd" crash exactly.
//
// Code context checks EAX against GObjects native-pool range
// ([0x13465a18], [0x13465a1c]) — classic UE3 UObject iteration. Almost
// certainly garbage-collection or FArchive reference traversal over an
// object's serialized property refs. One of our server-created objects
// has an integer (0x1d5 = 469) written into a slot that UE3 treats as a
// UObject*.
//
// Hook entry logs `this` so we can see which object is being processed
// when the bad ref is encountered.
// Target FUN_11346d30 — crash at 0x11346dc1 (this=EBX=0x1d5 → MOV EAX,[EBX]).
// __thiscall(this, int, void*, int) — under __fastcall with edx: 5 formal args.
class TgPawn__RosterWalker : public HookBase<
    void(__fastcall*)(void* /*this*/, void* /*edx*/,
                      int /*p1*/, void* /*p2*/, int /*p3*/),
    0x11346d30,
    TgPawn__RosterWalker> {
public:
    static void __fastcall Call(void* pThis, void* edx,
                                int p1, void* p2, int p3);
    static inline void __fastcall CallOriginal(void* pThis, void* edx,
                                               int p1, void* p2, int p3) {
        m_original(pThis, edx, p1, p2, p3);
    }
};
