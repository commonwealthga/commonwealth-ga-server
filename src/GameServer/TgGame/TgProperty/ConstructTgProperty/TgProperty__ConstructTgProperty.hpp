#pragma once

#include "src/Utils/HookBase.hpp"

class TgProperty__ConstructTgProperty : public HookBase<
	void*(__cdecl*)(void*, int, int, int, int, int, int, int, int*), 
	0x109A9190,
	TgProperty__ConstructTgProperty> {
public:
	static void* Call(void* param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7,int param_8,int* param_9);
	static inline void* CallOriginal(void* param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7,int param_8,int* param_9) {
		return m_original(param_1,param_2,param_3,param_4,param_5,param_6,param_7,param_8,param_9);
	}
};

