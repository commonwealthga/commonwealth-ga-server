#include "src/GameServer/Engine/World/BeginPlay/World__BeginPlay.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall World__BeginPlay::Call(void *World, void *edx, void *Url, int bResetTime) {
	Logger::Log("debug", "World__BeginPlay::Call");
	Globals::Get().GWorld = World;
	
	World__BeginPlay::CallOriginal(World, edx, Url, bResetTime);
}

