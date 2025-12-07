#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnPlayerCharacter : public HookBase<
	ATgPawn_Character*(__fastcall*)(ATgGame*, void*, ATgPlayerController*, FVector),
	0x10AD9AF0,
	TgGame__SpawnPlayerCharacter> {
public:
	static bool bEnemyGearSpawned;
	static inline int GetEquipPointByType(int slotUsedValueId) {
		int equipPoint = 0;
		if (slotUsedValueId == 198) {
			equipPoint = 1;
		} else if (slotUsedValueId == 200) {
			equipPoint = 2;
		} else if (slotUsedValueId == 199) {
			equipPoint = 3;
		} else if (slotUsedValueId == 201) {
			equipPoint = 4;
		} else if (slotUsedValueId == 202) {
			equipPoint = 5;
		} else if (slotUsedValueId == 203) {
			equipPoint = 6;
		} else if (slotUsedValueId == 204) {
			equipPoint = 7;
		} else if (slotUsedValueId == 385) {
			equipPoint = 8;
		} else if (slotUsedValueId == 386) {
			equipPoint = 9;
		} else if (slotUsedValueId == 499) {
			equipPoint = 10;
		} else if (slotUsedValueId == 500) {
			equipPoint = 11;
		} else if (slotUsedValueId == 501) {
			equipPoint = 12;
		} else if (slotUsedValueId == 502) {
			equipPoint = 13;
		} else if (slotUsedValueId == 823) {
			equipPoint = 14;
		} else if (slotUsedValueId == 996) {
			equipPoint = 15;
		} else if (slotUsedValueId == 997) {
			equipPoint = 16;
		} else if (slotUsedValueId == 998) {
			equipPoint = 17;
		} else if (slotUsedValueId == 999) {
			equipPoint = 18;
		} else if (slotUsedValueId == 1000) {
			equipPoint = 19;
		} else if (slotUsedValueId == 1001) {
			equipPoint = 20;
		} else if (slotUsedValueId == 1002) {
			equipPoint = 21;
		} else if (slotUsedValueId == 1003) {
			equipPoint = 22;
		} else if (slotUsedValueId == 1004) {
			equipPoint = 23;
		} else if (slotUsedValueId == 242) {
			equipPoint = 24;
		}
		return equipPoint;
	};
	static ATgPawn_Character* __fastcall Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation);
	static inline ATgPawn_Character* __fastcall CallOriginal(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {
		return m_original(Game, edx, PlayerController, SpawnLocation);
	};
	static ATgDevice* GiveJetpack(ATgPawn_Character* Pawn, ATgRepInfo_Player* PlayerReplicationInfo, ATgPlayerController* PlayerController, int nInventoryId);
};

