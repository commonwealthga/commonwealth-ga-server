#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__LowLevelGetRemoteAddress {
public:
	static void __fastcall Call(UNetConnection* NetConnection, void* edx, void* Out);
};

