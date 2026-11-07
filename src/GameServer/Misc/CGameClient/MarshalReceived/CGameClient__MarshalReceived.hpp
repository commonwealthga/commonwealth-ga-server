#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CGameClient__MarshalReceived : public HookBase<
	uint8_t(__fastcall*)(void*, void*, void*),
	0x1091a7c0,
	CGameClient__MarshalReceived> {
public:
	static uint8_t __fastcall Call(void* GameClient, void* edx, void* InBunch);
	static inline uint8_t __fastcall CallOriginal(void* GameClient, void* edx, void* InBunch) {
		return m_original(GameClient, edx, InBunch);
	};
	// Fire the map's quest Kismet once the control server has decided
	// (QUEST_KISMET). requirement_id 0 -> TgSeqEvent_QuestUpdated output `link`
	// (0 Accepted, 1 Requirements Complete, 2 Turned In, 3 Abandoned);
	// requirement_id > 0 -> TgSeqEvent_QuestRequirementUpdated for that requirement.
	static void FireQuestKismet(int quest_id, int link, int requirement_id);
};

