#pragma once

#include "src/Utils/HookBase.hpp"

// CMarshal::set_int32_t @ 0x10938470.
// Stores an int32 at the given key on a CMarshal. The serializer chooses wire
// width from the key registry (e.g. UINT8 fields like ALERT_PRIORITY are
// truncated at emit time), so using this setter for narrower fields matches
// what the binary itself does — see FUN_109649f0 which feeds set_int32_t
// into ALERT_PRIORITY/ALERT_TYPE.
class CMarshal__SetInt32 : public HookBase<
	int(__fastcall*)(void*, void*, int, int),
	0x10938470,
	CMarshal__SetInt32> {
public:
	static int __fastcall Call(void* Marshal, void* edx, int Key, int Value);
	static inline int __fastcall CallOriginal(void* Marshal, void* edx, int Key, int Value) {
		return m_original(Marshal, edx, Key, Value);
	};
};
