#include "src/GameServer/Combat/SendKillAlert/SendKillAlert.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/Utils/Logger/Logger.hpp"

// asm_data_set_msg_translations entries (English):
//   0x5A1E "You defeated @@name@@!"
//   0x5A1F "Your @@player_name@@ defeated @@name@@!"
// Pet-kill template uses @@player_name@@ for the pet's name (fills PLAYER_NAME
// = 0x03C5); direct kill ignores it. NAME (0x0370) is the killed target's name
// in both templates.
static constexpr int kMsgIdPlayerKill = 0x5A1E;
static constexpr int kMsgIdPetKill    = 0x5A1F;

// Kill alerts use AlertPriority APT_Normal (1) + AlertType ATT_Beneficial (1)
// — matches the binary's AddKillAlert (vtable[0x734] call args).
void SendKillAlert::Call(UNetConnection* Connection, wchar_t* VictimName, wchar_t* PetName) {
	const bool isPetKill = (PetName && PetName[0]);
	const int  msgId     = isPetKill ? kMsgIdPetKill : kMsgIdPlayerKill;

	if (Logger::IsChannelEnabled("kills")) {
		Logger::Log("kills", "[SendKillAlert] msgId=0x%04X victim='%ls' pet='%ls' → conn=%p\n",
			msgId,
			VictimName ? VictimName : L"<null>",
			isPetKill ? PetName : L"",
			(void*)Connection);
	}

	SendAlert::Send(Connection, msgId,
	                /*priority=APT_Normal*/    1,
	                /*type=ATT_Beneficial*/   1,
	                /*duration*/              2.0f,
	                VictimName,
	                isPetKill ? PetName : nullptr);
}
