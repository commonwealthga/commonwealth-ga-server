#pragma once

#include "src/pch.hpp"

class ClassPreloader {
private:
	static bool bClassesPreloaded;
	static void PreloadClasses();

	static std::map <std::string, UClass*> Classes;
public:
	static UClass* GetTgPawnCharacterClass();
	static UClass* GetTgPropertyClass();
	static UClass* GetTgRepInfoTaskForceClass();
	static UClass* GetTgDeployBeaconClass();
	static UClass* GetTgDeployBeaconEntranceClass();
};

