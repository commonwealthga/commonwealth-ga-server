#pragma once

#include "src/pch.hpp"

// ATgChestActor::OpenChest / ::RespawnChest — both vtable slots of the
// TgChestActor vtable (0x116bc6b8) hold shared empty stubs in this build
// (+0x37c -> 0x109af320 `ret 4`, +0x378 -> 0x109af330 `ret`). SpawnChest
// (+0x374), LootChest (+0x380) and CloseChest (+0x384) are intact.
// Reached from TgPlayerController.ServerPerformedUseAction (Use key on a
// chest the pawn is touching) and from the chest's own 'RespawnChest' timer
// (PostBeginPlay arms it with m_MinRespawnTime). The stubs are shared with
// unrelated classes, so the slots are patched directly, not detoured.
//
// r_eChestState: 0 = closed (Touch offers Use), 1 = open, 2 = gone
// (CloseChest hides the mesh and sets 2).
class TgChestActor__OpenChest {
public:
	static void Install();
	static void __fastcall Open(ATgChestActor* Chest, void* edx, ATgPawn* aPawn);
	static void __fastcall Respawn(ATgChestActor* Chest, void* edx);
};
