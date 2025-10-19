#include "src/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/Logger/Logger.hpp"

std::map<uint32_t, int> CAmOmegaVolume__LoadOmegaVolumeMarshal::m_OmegaVolumePointers;

void __fastcall CAmOmegaVolume__LoadOmegaVolumeMarshal::Call(void* CAmOmegaVolumeRow, void* edx, void* Marshal) {
	CallOriginal(CAmOmegaVolumeRow, edx, Marshal);

	// void** tmp = (void**)Marshal;

	m_OmegaVolumePointers[CMarshal__GetInt32t::m_values[GA_T::GA_T::UI_VOLUME_ID]] = (int)(CAmOmegaVolumeRow);

	// Logger::Log("marshalbot_data", "\n\n%d: 0x%p\n", CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID], Marshal);

	// Logger::DumpMemory("marshalbot_data", CAmOmegaVolumeRow, 0x300, 0);

	// uint32_t* botId;
	// CMarshal__GetByte::CallOriginal(Marshal, edx, GA_T::GA_T::BOT_ID, botId);
	//
	// Logger::Log("marshalbot", "%d\n", *botId);
}

