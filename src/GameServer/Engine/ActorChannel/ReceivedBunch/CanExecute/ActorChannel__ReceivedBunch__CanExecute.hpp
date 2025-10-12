#pragma once

#include "src/Utils/HookBase.hpp"

class ActorChannel__ReceivedBunch__CanExecute : public HookBase<
	bool(__cdecl*)(void*, void*, int),
	0x1090FE50,
	ActorChannel__ReceivedBunch__CanExecute> {
public:
	static bool __cdecl Call(void* param_1, void* param_2, int param_3);
	static inline bool __cdecl CallOriginal(void* param_1, void* param_2, int param_3) {
		return m_original(param_1, param_2, param_3);
	}
};

