#include "src/GameServer/Combat/SendKillAlert/SendKillAlert.hpp"
#include "src/GameServer/Misc/CMarshal/SetInt32/CMarshal__SetInt32.hpp"
#include "src/GameServer/Misc/CMarshal/SetBool/CMarshal__SetBool.hpp"
#include "src/GameServer/Misc/CMarshal/SetFloat/CMarshal__SetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/SetWcharT/CMarshal__SetWcharT.hpp"
#include "src/GameServer/Misc/CMarshal/Destroy/CMarshal__Destroy.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/Utils/Logger/Logger.hpp"

// asm_data_set_msg_translations entries (English):
//   0x5A1E "You defeated @@name@@!"
//   0x5A1F "Your @@player_name@@ defeated @@name@@!"
// Pet-kill template uses @@player_name@@ for the pet's name (fills the
// PLAYER_NAME = 0x03C5 marshal field); direct kill ignores it. NAME (0x0370)
// is the killed target's name in both templates.
static constexpr int kMsgIdPlayerKill = 0x5A1E;
static constexpr int kMsgIdPetKill    = 0x5A1F;

void SendKillAlert::Call(UNetConnection* Connection, wchar_t* VictimName, wchar_t* PetName) {
	if (!Connection) return;

	const bool isPetKill = (PetName && PetName[0]);
	const int  msgId     = isPetKill ? kMsgIdPetKill : kMsgIdPlayerKill;

	uint8_t MarshalStorage[0x80] = {0};
	void* Marshal = MarshalStorage;
	CMarshalObject__Create::CallOriginal(Marshal);
	*(void**)Marshal = CMarshalObject__Create::CMarshal_vftable;
	*(uint16_t*)((uint8_t*)Marshal + 0x26) = GA_U::CHAT_MESSAGE;  // 0x0073

	// Field set mirrors AddKillAlert's local CMarshal (FUN_10964810 + 0x109649f0)
	// + the two routing fields FUN_109027e0 demands for the dispatcher branch
	// (CHAT_CH_TYPE + MSG_ID).
	//
	// CHAT_CH_TYPE = 1 satisfies FUN_109025f0's validate (any int in 0..15
	// works; FUN_109025f0 is void and only builds a debug "All<channel>"
	// string). Channel value doesn't affect the alert path.
	//
	// set_int32_t on ALERT_PRIORITY/ALERT_TYPE despite them being UINT8 in
	// the key registry — wire width comes from the registry at serialize
	// time, not from the setter. FUN_109649f0 does the same.
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::CHAT_CH_TYPE, 1);
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::MSG_ID, msgId);
	CMarshal__SetBool ::CallOriginal(Marshal, nullptr, GA_T::DISPLAY_AS_ALERT, 1);
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::ALERT_PRIORITY, 1);
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::ALERT_TYPE, 1);
	CMarshal__SetFloat::CallOriginal(Marshal, nullptr, GA_T::FLOAT_VALUE, 2.0f);
	if (VictimName && VictimName[0]) {
		CMarshal__SetWcharT::CallOriginal(Marshal, nullptr, GA_T::NAME, VictimName);
	}
	if (isPetKill) {
		CMarshal__SetWcharT::CallOriginal(Marshal, nullptr, GA_T::PLAYER_NAME, PetName);
	}

	if (Logger::IsChannelEnabled("kills")) {
		Logger::Log("kills", "[SendKillAlert] msgId=0x%04X victim='%ls' pet='%ls' → conn=%p\n",
			msgId,
			VictimName ? VictimName : L"<null>",
			isPetKill ? PetName : L"",
			(void*)Connection);
	}

	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, Marshal);
	NetConnection__FlushNet::CallOriginal(Connection);
	CMarshal__Destroy::CallOriginal(Marshal);
}
