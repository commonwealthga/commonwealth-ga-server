#include "src/GameServer/TgGame/TgPawn/RosterWalker/TgPawn__RosterWalker.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string>
// UObject from SDK is already available via pch.hpp → SdkHeaders.h.

// FUN_11346d30 is UObject::CallFunction — the UE3 VM's function-dispatch
// helper. Signature is (this, FFrame* Frame, void* Result, UFunction* Fn).
// For FUNC_Native functions it invokes via this->vtable[0x104/4] — which
// crashes when `this` is bogus.
//
// Actual crash caller is the opcode handler at 0x11347510 (EX_FinalFunction):
//   MOV EAX,[ESP+4]       ; EAX = FFrame*
//   MOV EDX,[EAX+0x18]    ; EDX = FFrame.Code cursor
//   PUSH ESI              ; save ESI   <-- ONE register save
//   MOV ESI,[EDX]         ; ESI = *cursor = UFunction* from bytecode
//   ADD EDX,4
//   MOV [EAX+0x18],EDX    ; advance cursor
//   MOV EDX,[ESP+0xC]     ; EDX = Result*
//   PUSH ESI / PUSH EDX / PUSH EAX / CALL FUN_11346d30
//   POP ESI / RET 0x8
// Stack from our hook's EBP (one saved reg in caller, so offset 24 not 28):
//   [EBP+0]  saved EBP
//   [EBP+4]  retaddr to orphan (0x1134752c)
//   [EBP+8]  FFrame*
//   [EBP+12] Result*
//   [EBP+16] UFunction*
//   [EBP+20] orphan's saved ESI_old
//   [EBP+24] retaddr from orphan to VM dispatch loop  <-- KEY
//
// Memory-range filter: Wine user stack lives around 0x0020xxxx; heap can
// be anywhere >=0x00100000. Use 0x00100000 as the low bound so stack FFrames
// get read.

void __fastcall TgPawn__RosterWalker::Call(void* pThis, void* edx,
                                           int p1, void* p2, int p3) {
    const uintptr_t t = (uintptr_t)pThis;
    const bool thisLooksBogus = (t < 0x00400000u) || (t >= 0x80000000u);

    if (thisLooksBogus) {
        // EBP is the frame pointer of THIS function. Layout (from our hook's
        // perspective entering via the Detours trampoline, which preserves
        // the orphan's CALL-pushed return address as our top stack element):
        //   [EBP+0]  saved EBP
        //   [EBP+4]  retaddr to orphan (0x1134752c)
        //   [EBP+8]  p1 = Frame*
        //   [EBP+12] p2 = Result*
        //   [EBP+16] p3 = UFunction*
        //   [EBP+20] orphan's saved EDI (= orphan-caller's EDI)
        //   [EBP+24] orphan's saved ESI (= orphan-caller's ESI)
        //   [EBP+28] retaddr from orphan back to ITS caller (the VM dispatcher)
        //   [EBP+32+] the VM dispatcher's stack
        const uint32_t* ebp = (const uint32_t*)__builtin_frame_address(0);
        void* ret0 = __builtin_return_address(0);

        // Copy 16 dwords (64 bytes) of stack above EBP into a local snapshot.
        uint32_t stk_copy[16];
        for (int i = 0; i < 16; ++i) stk_copy[i] = ebp[i];

        // Read FFrame fields via p1 (Frame ptr). FFrame layout in UE3:
        //   +0x00 Node (UStruct*), +0x04 Object (UObject* — VM's `this`),
        //   +0x08 Code (BYTE*), +0x0C Locals, +0x10 PreviousFrame,
        //   +0x14 Out_Parms, +0x18 Code (cursor).
        uint32_t frame_node = 0, frame_object = 0, frame_code = 0,
                 frame_locals = 0, frame_prev = 0, frame_outparms = 0,
                 frame_cursor = 0;
        if ((uint32_t)p1 >= 0x00100000u && (uint32_t)p1 < 0x80000000u) {
            const uint32_t* fr = (const uint32_t*)(uintptr_t)p1;
            frame_node     = fr[0];
            frame_object   = fr[1];
            frame_code     = fr[2];
            frame_locals   = fr[3];
            frame_prev     = fr[4];
            frame_outparms = fr[5];
            frame_cursor   = fr[6];  // offset 0x18
        }

        // Read UFunction fields via p3.
        uint32_t fn_vtbl = 0, fn_flags = 0, fn_numparms = 0, fn_name_idx = 0;
        if ((uintptr_t)p3 >= 0x00100000u && (uintptr_t)p3 < 0x80000000u) {
            const uint32_t* fn = (const uint32_t*)(uintptr_t)p3;
            fn_vtbl     = fn[0];
            fn_name_idx = fn[0x2C/4];  // UObject::Name at +0x2C (FName.Index)
            fn_numparms = fn[0x88/4];
            fn_flags    = fn[0x90/4];
        }

        // Now emit logs from the snapshot.
        Logger::Log("roster_walker",
            "BAD this=%p retaddr=%p p1(Frame)=%p p2(Result)=%p p3(Fn)=%p",
            pThis, ret0, (void*)(uintptr_t)p1, p2, (void*)(uintptr_t)p3);

        Logger::Log("roster_walker",
            "  FFrame: Node=%08x Object=%08x Code=%08x Locals=%08x Prev=%08x OutParms=%08x Cursor=%08x",
            frame_node, frame_object, frame_code, frame_locals,
            frame_prev, frame_outparms, frame_cursor);

        Logger::Log("roster_walker",
            "  UFunction: vtbl=%08x name_idx=%u numparms=%u flags=%08x",
            fn_vtbl, fn_name_idx, fn_numparms, fn_flags);

        // Resolve UFunction full name via SDK (copy before next Log).
        std::string fn_name = "?";
        if ((uintptr_t)p3 >= 0x00100000u && (uintptr_t)p3 < 0x80000000u) {
            fn_name = ((UObject*)(uintptr_t)p3)->GetFullName();
        }
        Logger::Log("roster_walker", "  UFunction name: %s", fn_name.c_str());

        // Dump bytecode around the cursor. execContext advanced FFrame.Code
        // past [inner_opcode_1 + 4-byte header + inner_opcode_2 start].
        // The byte pattern "1B ..." or "19 ..." before cursor should identify
        // the Context opcode + the inner expression that produced 0x1d5.
        if ((uintptr_t)frame_cursor >= 0x00100000u &&
            (uintptr_t)frame_cursor < 0x80000000u) {
            const uint8_t* bc = (const uint8_t*)(uintptr_t)frame_cursor;
            Logger::Log("roster_walker",
                "  bytecode @cursor-0x20..cursor:  "
                "%02x %02x %02x %02x %02x %02x %02x %02x  "
                "%02x %02x %02x %02x %02x %02x %02x %02x  "
                "%02x %02x %02x %02x %02x %02x %02x %02x  "
                "%02x %02x %02x %02x %02x %02x %02x %02x",
                bc[-32], bc[-31], bc[-30], bc[-29], bc[-28], bc[-27], bc[-26], bc[-25],
                bc[-24], bc[-23], bc[-22], bc[-21], bc[-20], bc[-19], bc[-18], bc[-17],
                bc[-16], bc[-15], bc[-14], bc[-13], bc[-12], bc[-11], bc[-10], bc[-9],
                bc[-8],  bc[-7],  bc[-6],  bc[-5],  bc[-4],  bc[-3],  bc[-2],  bc[-1]);
        }

        // Orphan caller's retaddr — single most important value.
        // Orphan at 0x11347510 only pushes ESI (one reg), so retaddr is at
        // EBP+24 = stk_copy[6]. The older orphan at 0x113474d0 pushes both
        // ESI and EDI, so its retaddr would be at EBP+28 = stk_copy[7].
        // Log both candidates so we can disambiguate.
        Logger::Log("roster_walker",
            "  outer_retaddr candidates: ebp+24=%08x  ebp+28=%08x",
            stk_copy[6], stk_copy[7]);

        for (int row = 0; row < 4; ++row) {
            Logger::Log("roster_walker",
                "  ebp[+0x%02x]: %08x %08x %08x %08x",
                row * 16,
                stk_copy[row*4 + 0], stk_copy[row*4 + 1],
                stk_copy[row*4 + 2], stk_copy[row*4 + 3]);
        }
    }

    CallOriginal(pThis, edx, p1, p2, p3);
}
