#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgPawn__InitializeDefaultProps::Call(ATgPawn* Pawn, void* edx) {
	// LogToFile("C:\\mylog.txt", "MINE TgPawn::InitializeDefaultProps START");
	Logger::Log("debug", "MINE TgPawn::InitializeDefaultProps START\n");


	UTgProperty* property_health = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
		ClassPreloader::GetTgPropertyClass(), -1, 0, 0,0,0,0,0,0);

	property_health->m_nPropertyId = 51;
	property_health->m_fBase = 1300;
	property_health->m_fRaw = 1300;
	property_health->m_fMinimum = 0;
	property_health->m_fMaximum = 1300;

	UTgProperty*** PropertiesDataPtrPtr = (UTgProperty***)((char*)Pawn + 0x3F4);
	int* PropertiesCountPtr = (int*)((char*)Pawn + 0x3F8);
	int* PropertiesMaxPtr   = (int*)((char*)Pawn + 0x3FC);

	// Initialize if not allocated yet
	if (*PropertiesDataPtrPtr == nullptr)
	{
		*PropertiesDataPtrPtr = (UTgProperty**)malloc(sizeof(UTgProperty*) * 128);
		memset(*PropertiesDataPtrPtr, 0, sizeof(UTgProperty*) * 128);
		*PropertiesCountPtr = 0;
		*PropertiesMaxPtr = 128;
	}

	// Add the property
	int Count = *PropertiesCountPtr;
	(*PropertiesDataPtrPtr)[Count] = property_health;
	*PropertiesCountPtr = Count + 1;

	Pawn->SetProperty(51, property_health->m_fMaximum);

	UTgProperty* property_power = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
		ClassPreloader::GetTgPropertyClass(), -1, 0, 0,0,0,0,0,0);

	property_power->m_nPropertyId = 243;
	property_power->m_fBase = 100;
	property_power->m_fRaw = 100;
	property_power->m_fMinimum = 0;
	property_power->m_fMaximum = 100;

	// Add the property
	(*PropertiesDataPtrPtr)[*PropertiesCountPtr] = property_power;
	*PropertiesCountPtr = *PropertiesCountPtr + 1;

	Pawn->SetProperty(243, property_power->m_fMaximum);

	UTgProperty* property_health_max = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
		ClassPreloader::GetTgPropertyClass(), -1, 0, 0,0,0,0,0,0);

	property_health_max->m_nPropertyId = 304;
	property_health_max->m_fBase = 1300;
	property_health_max->m_fRaw = 1300;
	property_health_max->m_fMinimum = 0;
	property_health_max->m_fMaximum = 1300;

	// Add the property
	(*PropertiesDataPtrPtr)[*PropertiesCountPtr] = property_health_max;
	*PropertiesCountPtr = *PropertiesCountPtr + 1;

	Pawn->SetProperty(304, property_health_max->m_fMaximum);
	Logger::Log("debug", "MINE TgPawn::InitializeDefaultProps END\n");
}

