#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/Logger/Logger.hpp"

bool CMarshal__GetByte::bIsLoadingBots = false;
std::map<int, uint32_t> CMarshal__GetByte::m_values;
std::map<int, std::vector<DeviceIdSlotPair>> CMarshal__GetByte::m_botDevices;
bool CMarshal__GetByte::bLogEnabled = false;

int __fastcall CMarshal__GetByte::Call(void* CMarshal, void* edx, int Field, uint32_t* Out) {
	if (bLogEnabled) {
		LogCallBegin();
	}

	int result = CallOriginal(CMarshal, edx, Field, Out);
	m_values[Field] = *Out;

	if (bIsLoadingBots) {
		if (Field == GA_T::SLOT_USED_VALUE_ID) {
			int DeviceId = CMarshal__GetByte::m_values[GA_T::DEVICE_ID];
			int SlotUsedValueId = CMarshal__GetByte::m_values[GA_T::SLOT_USED_VALUE_ID];
			int BotId = CMarshal__GetByte::m_values[GA_T::BOT_ID];

			m_botDevices[BotId].push_back({ DeviceId, SlotUsedValueId });
		}
	}

	if (bLogEnabled) {
		Logger::Log(GetLogChannel(), "Field: 0x%04X value: %d\n", Field, *Out);
		LogCallEnd();
	}

	return result;
}

