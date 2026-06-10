#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgAIController__RadioAlarm : public HookBase<
	void(__fastcall*)(ATgAIController*, void*),
	0x10a7e890,
	TgAIController__RadioAlarm> {
public:
	static void __fastcall Call(ATgAIController* AIC, void* edx);
};
