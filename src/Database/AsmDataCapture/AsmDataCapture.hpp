#pragma once

#include "src/pch.hpp"

// Centralised capture of every data set loaded by CAssemblyManager::LoadAssemblyDatFile
// (the master at 0x10951030) into asm_* SQLite tables.
//
// Mechanism: CMarshal__GetArray::Call dispatches to AsmDataCapture::OnGetArray after
// the game's original get_array returns. Each recognised data-set ID has a walker
// that iterates the returned linked list, reads fields via CMarshal__Get*::CallOriginal
// (bypassing our own hooks to avoid recursion), and inserts rows.
//
// This replaces the per-row HookBase subclasses in
// src/GameServer/Misc/CAm*/Load*Marshal/*.  Those hooks still exist for gameplay-side
// pointer capture (m_ItemPointers, m_BotPointers, ...), but all DB INSERT logic lives
// here.
//
// Flip AsmDataCapture::bPopulateDatabase = true in dllmain, run the game once,
// flip back.
namespace AsmDataCapture {
    // Master populate flag. Default: false. Set in dllmain.
    extern bool bPopulateDatabase;

    // Called from CMarshal__GetArray::Call after CallOriginal.
    // field        = GA_T::DATA_SET_* id
    // arrayPtr     = uint32_t returned in *Out by the game (head-holder struct)
    // marshal      = the marshal/node on which get_array was invoked (may be the root
    //                marshal or a parent row for nested data sets).
    void OnGetArray(void* marshal, int field, uint32_t arrayPtr);
}
