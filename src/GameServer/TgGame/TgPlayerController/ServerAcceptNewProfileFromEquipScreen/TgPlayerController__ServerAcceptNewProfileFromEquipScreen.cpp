#include "src/GameServer/TgGame/TgPlayerController/ServerAcceptNewProfileFromEquipScreen/TgPlayerController__ServerAcceptNewProfileFromEquipScreen.hpp"

void __fastcall TgPlayerController__ServerAcceptNewProfileFromEquipScreen::Call(ATgPlayerController* PlayerController, void* edx, int nProfileId, FTGEQUIP_SLOTS_STRUCT DeviceArray) {
	LogCallBegin();

	Logger::Log(GetLogChannel(), "nProfileId: %d\n", nProfileId);

// struct FTGEQUIP_SLOTS_STRUCT
// {
// 	int                                                SlotIndices[ 0x19 ];                              		// 0x0000 (0x0064) [0x0000000000000000]              
// 	int                                                MiscItems[ 0x19 ];                                		// 0x0064 (0x0064) [0x0000000000000000]              
// };

	for (int i = 0; i < 0x19; i++) {
		Logger::Log(GetLogChannel(), "SlotIndices[%d]: %d\n", i, DeviceArray.SlotIndices[i]);
		Logger::Log(GetLogChannel(), "MiscItems[%d]: %d\n", i, DeviceArray.MiscItems[i]);
	}
	
	LogCallEnd();
}

