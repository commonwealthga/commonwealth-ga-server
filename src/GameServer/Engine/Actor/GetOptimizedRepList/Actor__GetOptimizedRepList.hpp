#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"
#include "src/GameServer/Utils/Neq.hpp"

class Actor__GetOptimizedRepList : public HookBase<
	int*(__fastcall*)(void*, void*, int, void*, int*, int*, int),
	0x10C5F870,
	Actor__GetOptimizedRepList> {
public:
	static int* __fastcall Call(void* thisxx, void* edx_dummy, int param_1, void* param_2, int* param_3, int* param_4, int param_5);
	static inline int* __fastcall CallOriginal(void* thisxx, void* edx_dummy, int param_1, void* param_2, int* param_3, int* param_4, int param_5) {
		return m_original(thisxx, edx_dummy, param_1, param_2, param_3, param_4, param_5);
	}

	static std::map<std::string, std::map<std::string, bool>> ObjectIsACache;
	static bool ObjectIsA(UObject* obj, const char* ClassFullName);

	static bool bRepListCached;
};


// uint32_t* __fastcall Mine_Actor_GetOptimizedRepList(void* thisxx, void* edx_dummy, int param_1, void* param_2, int* param_3, int* param_4, int param_5) {
