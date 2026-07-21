#pragma once

#include "src/pch.hpp"
#include <cstdint>

// Generic mid-game alert sender — center-screen toast + sound, via the
// CHAT_MESSAGE (0x0073) marshal piggyback on the chat dispatcher. See
// `reference_chat_message_alert_piggyback.md` for the wire-shape rationale.
//
// Two send modes:
//   - Send(conn, ...)      → one connection
//   - Broadcast(...)       → every connected player
//   - BroadcastToTaskforce → only members of the given taskforce (1=att,2=def)
//
// Priority / Type are the TgObject.AlertPriority / AlertType enums (see
// TgObject.uc):
//   AlertPriority: 0 Minimal | 1 Normal | 2 High | 3 Critical
//   AlertType:     0 Regular | 1 Beneficial | 2 Detrimental | 3 Important
class SendAlert {
public:
	// Send to one connection. Name/PlayerName fill @@name@@ / @@player_name@@
	// template slots; pass nullptr if the chosen msgId has no slots.
	static void Send(UNetConnection* Connection,
	                 int msgId,
	                 unsigned char priority,
	                 unsigned char type,
	                 float duration = 3.0f,
	                 const wchar_t* name = nullptr,
	                 const wchar_t* playerName = nullptr);

	// Send free text to one connection. The text travels in GA_T::MESSAGE with
	// no MSG_ID, so the client renders it as a CHAT LINE ("Instance" channel),
	// NOT the center-screen toast — MSG_ID presence gates the alert path
	// (playtest 2026-07-20). Good for command replies (/coords etc.); use
	// BroadcastText for a real toast.
	static void SendText(UNetConnection* Connection,
	                     const wchar_t* message,
	                     unsigned char priority,
	                     unsigned char type,
	                     float duration = 3.0f);

	// Free-text CENTER-SCREEN toast to every connected player. Rides the bare
	// "@@text_value@@" msg template (23083) with the text in GA_T::TEXT_VALUE,
	// so the dispatcher takes the real alert path (unlike SendText's chat line).
	static void BroadcastText(const wchar_t* message,
	                          unsigned char priority,
	                          unsigned char type,
	                          float duration = 3.0f);

	// Send to every connected player (skips closed connections + ones with no Pawn).
	static void Broadcast(int msgId,
	                      unsigned char priority,
	                      unsigned char type,
	                      float duration = 3.0f,
	                      const wchar_t* name = nullptr,
	                      const wchar_t* playerName = nullptr);

	// Send only to players whose ClientConnectionData.PlayerInfo.task_force matches tf.
	// Stale-state risk: this reads the cached join-time task_force and won't reflect
	// /changeteam if the chat command didn't keep it in sync. Prefer the TF-pointer
	// overload below for game-state alerts (lead changes etc.).
	static void BroadcastToTaskforce(int tf,
	                                 int msgId,
	                                 unsigned char priority,
	                                 unsigned char type,
	                                 float duration = 3.0f,
	                                 const wchar_t* name = nullptr,
	                                 const wchar_t* playerName = nullptr);

	// Send only to players whose PRI.r_TaskForce pointer matches `tf`. Walks
	// WorldInfo->ControllerList and uses PRI as the source of truth — this is
	// what the player actually IS right now, not what they were on join, so
	// /changeteam etc. are honoured.
	static void BroadcastToTaskforce(class ATgRepInfo_TaskForce* tf,
	                                 int msgId,
	                                 unsigned char priority,
	                                 unsigned char type,
	                                 float duration = 3.0f,
	                                 const wchar_t* name = nullptr,
	                                 const wchar_t* playerName = nullptr);
};
