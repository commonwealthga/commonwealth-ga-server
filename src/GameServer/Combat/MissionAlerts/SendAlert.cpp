#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Misc/CMarshal/SetInt32/CMarshal__SetInt32.hpp"
#include "src/GameServer/Misc/CMarshal/SetBool/CMarshal__SetBool.hpp"
#include "src/GameServer/Misc/CMarshal/SetFloat/CMarshal__SetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/SetWcharT/CMarshal__SetWcharT.hpp"
#include "src/GameServer/Misc/CMarshal/Destroy/CMarshal__Destroy.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>

// Builds and sends one CHAT_MESSAGE marshal carrying the alert fields.
// Mirrors SendKillAlert's field layout — see reference_chat_message_alert_piggyback.md.
void SendAlert::Send(UNetConnection* Connection,
                     int msgId,
                     unsigned char priority,
                     unsigned char type,
                     float duration,
                     const wchar_t* name,
                     const wchar_t* playerName) {
	if (!Connection) return;
	if ((uintptr_t)Connection < 0x10000) return;  // not a real pointer

	uint8_t MarshalStorage[0x80] = {0};
	void* Marshal = MarshalStorage;
	CMarshalObject__Create::CallOriginal(Marshal);
	*(void**)Marshal = CMarshalObject__Create::CMarshal_vftable;
	*(uint16_t*)((uint8_t*)Marshal + 0x26) = GA_U::CHAT_MESSAGE;  // 0x0073

	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::CHAT_CH_TYPE,     1);
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::MSG_ID,           msgId);
	CMarshal__SetBool ::CallOriginal(Marshal, nullptr, GA_T::DISPLAY_AS_ALERT, 1);
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::ALERT_PRIORITY,   priority);
	CMarshal__SetInt32::CallOriginal(Marshal, nullptr, GA_T::ALERT_TYPE,       type);
	CMarshal__SetFloat::CallOriginal(Marshal, nullptr, GA_T::FLOAT_VALUE,      duration);
	if (name && name[0]) {
		CMarshal__SetWcharT::CallOriginal(Marshal, nullptr, GA_T::NAME,        (wchar_t*)name);
	}
	if (playerName && playerName[0]) {
		CMarshal__SetWcharT::CallOriginal(Marshal, nullptr, GA_T::PLAYER_NAME, (wchar_t*)playerName);
	}

	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, Marshal);

	// Force-drain the marshal channel inline so this alert ships now instead
	// of waiting for the next UNetConnection::Tick. Save + restore QueuedBytes
	// verbatim (no delta accounting) — see TgPawn_Character__SendMarshal for
	// the full why; short version: UChannel::IsReadyToSend calls IsNetReady
	// with an indeterminate Saturate flag (no args), which sometimes
	// clobbers our -1B fake bank with `QueuedBytes = -BunchBytes`. The
	// delta would then be ≈ +1B and writing that back permanently exhausts
	// the connection's budget, starving actor rep — the symptom is severe
	// server-wide lag after the first such call.
	{
		void* MarshalChannel = *(void**)((char*)Connection + 0x4FB4);
		if (MarshalChannel) {
			volatile int32_t* QueuedBytesPtr =
				(volatile int32_t*)((char*)Connection + 0x138);
			constexpr int32_t kFakeBank = -1000000000;
			const int32_t QbOriginal = *QueuedBytesPtr;
			*QueuedBytesPtr = kFakeBank;
			typedef void(__thiscall* TickFn)(void*);
			TickFn Tick = (TickFn)((*(void***)MarshalChannel)[0x130 / 4]);
			Tick(MarshalChannel);
			*QueuedBytesPtr = QbOriginal;
		}
	}

	NetConnection__FlushNet::CallOriginal(Connection);
	CMarshal__Destroy::CallOriginal(Marshal);
}

void SendAlert::Broadcast(int msgId,
                          unsigned char priority,
                          unsigned char type,
                          float duration,
                          const wchar_t* name,
                          const wchar_t* playerName) {
	int sent = 0;
	for (auto& pair : GClientConnectionsData) {
		const ClientConnectionData& conn = pair.second;
		if (conn.bClosed) continue;
		UNetConnection* Connection = (UNetConnection*)(intptr_t)pair.first;
		Send(Connection, msgId, priority, type, duration, name, playerName);
		sent++;
	}
	if (Logger::IsChannelEnabled("announcer")) {
		Logger::Log("announcer", "[Broadcast] msgId=0x%04X pri=%d type=%d → %d connection(s)\n",
			msgId, (int)priority, (int)type, sent);
	}
}

void SendAlert::BroadcastToTaskforce(int tf,
                                     int msgId,
                                     unsigned char priority,
                                     unsigned char type,
                                     float duration,
                                     const wchar_t* name,
                                     const wchar_t* playerName) {
	if (tf != 1 && tf != 2) return;
	int sent = 0;
	for (auto& pair : GClientConnectionsData) {
		const ClientConnectionData& conn = pair.second;
		if (conn.bClosed) continue;
		if ((int)conn.PlayerInfo.task_force != tf) continue;
		UNetConnection* Connection = (UNetConnection*)(intptr_t)pair.first;
		Send(Connection, msgId, priority, type, duration, name, playerName);
		sent++;
	}
	if (Logger::IsChannelEnabled("announcer")) {
		Logger::Log("announcer", "[Broadcast→tf%d] msgId=0x%04X pri=%d type=%d → %d connection(s)\n",
			tf, msgId, (int)priority, (int)type, sent);
	}
}

void SendAlert::BroadcastToTaskforce(ATgRepInfo_TaskForce* tf,
                                     int msgId,
                                     unsigned char priority,
                                     unsigned char type,
                                     float duration,
                                     const wchar_t* name,
                                     const wchar_t* playerName) {
	if (tf == nullptr) return;
	if (Globals::Get().GWorld == nullptr) return;
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WI == nullptr) return;

	int sent = 0;
	int scanned = 0;
	for (AController* C = WI->ControllerList; C != nullptr; C = C->NextController) {
		scanned++;
		if (C->PlayerReplicationInfo == nullptr) continue;
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)C->PlayerReplicationInfo;
		if (PRI->r_TaskForce != tf) continue;

		// PlayerControllers carry the network channel in `Player`. Bots / AI
		// controllers have null Player — skip them.
		const char* clsName = C->Class ? C->Class->GetFullName() : nullptr;
		if (!clsName || strstr(clsName, "PlayerController") == nullptr) continue;

		APlayerController* PC = (APlayerController*)C;
		if (PC->Player == nullptr) continue;
		UNetConnection* Connection = (UNetConnection*)PC->Player;
		if ((uintptr_t)Connection < 0x10000) continue;

		Send(Connection, msgId, priority, type, duration, name, playerName);
		sent++;
	}
	if (Logger::IsChannelEnabled("announcer")) {
		Logger::Log("announcer", "[Broadcast→TF%p tf=%d] msgId=0x%04X pri=%d type=%d → %d of %d controller(s)\n",
			(void*)tf, (int)tf->r_nTaskForce, msgId, (int)priority, (int)type, sent, scanned);
	}
}
