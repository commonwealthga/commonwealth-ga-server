#include "src/GameServer/Core/UClass/Bind/UClass__Bind.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstdio>
#include <string>

void __fastcall UClass__Bind::Call(UClass* Class, void* edx)
{
	// Same precondition the binary asserts on (UnClass.cpp): a class with no
	// SuperClass outside the editor. Core.Object is the one legitimate case.
	if (*Globals::Get().GIsEditor == 0 && Class && Class->SuperField == nullptr) {
		const std::string name(
			(Class->Class && Class->Outer) ? Class->GetFullName() : Class->GetName());
		if (name != "Class Core.Object" && name != "Object") {
			Logger::Log("bind", "UClass::Bind missing SuperClass: %s\n", name.c_str());
			char msg[512];
			snprintf(msg, sizeof(msg),
				"UClass::Bind: '%s' has no SuperClass.\n"
				"The engine assert (UnClass.cpp:1511) will crash right after this box.",
				name.c_str());
			MessageBoxA(NULL, msg, "UClass::Bind", MB_OK | MB_ICONERROR);
		}
	}

	CallOriginal(Class, edx);
}
