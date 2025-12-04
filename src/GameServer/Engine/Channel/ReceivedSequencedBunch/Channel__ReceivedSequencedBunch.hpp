#pragma once

#include "src/Utils/HookBase.hpp"

class Channel__ReceivedSequencedBunch : public HookBase<
	int(__fastcall*)(void*, void*, void*),
	0x10d88980,
	Channel__ReceivedSequencedBunch> {
public:
    static int __fastcall Call(void* Channel, void* edx, void* Bunch);
	static inline int __fastcall CallOriginal(void* Channel, void* edx, void* Bunch) {
		return m_original(Channel, edx, Bunch);
	}
};


