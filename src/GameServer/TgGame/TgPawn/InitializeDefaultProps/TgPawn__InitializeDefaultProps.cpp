#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/Utils/Logger/Logger.hpp"

UTgProperty* TgPawn__InitializeDefaultProps::InitializeProperty(ATgPawn* Pawn, int nPropertyId, float fBase, float fRaw, float fMinimum, float fMaximum) {
	UTgProperty* Property = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
		ClassPreloader::GetTgPropertyClass(), -1, 0, 0,0,0,0,0,0);

	Property->m_nPropertyId = nPropertyId;
	Property->m_fBase = fBase;
	Property->m_fRaw = fRaw;
	Property->m_fMinimum = fMinimum;
	Property->m_fMaximum = fMaximum;

	UTgProperty*** PropertiesDataPtrPtr = (UTgProperty***)((char*)Pawn + 0x3F4);
	int* PropertiesCountPtr = (int*)((char*)Pawn + 0x3F8);
	int* PropertiesMaxPtr   = (int*)((char*)Pawn + 0x3FC);

	// Initialize if not allocated yet
	if (*PropertiesDataPtrPtr == nullptr)
	{
		*PropertiesDataPtrPtr = (UTgProperty**)malloc(sizeof(UTgProperty*) * 512);
		memset(*PropertiesDataPtrPtr, 0, sizeof(UTgProperty*) * 512);
		*PropertiesCountPtr = 0;
		*PropertiesMaxPtr = 512;
	}

	// Add the property
	int Count = *PropertiesCountPtr;
	(*PropertiesDataPtrPtr)[Count] = Property;
	*PropertiesCountPtr = Count + 1;

	Pawn->SetProperty(nPropertyId, fRaw);

	return Property;
}

void __fastcall* TgPawn__InitializeDefaultProps::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();

	if ((char*)Pawn + 0x400 == nullptr) {
		TMap__Allocate::CallOriginal((void*)((char*)Pawn + 0x400));
	}

	InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH, 1300, 1300, 0, 1300);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH_MAX, 1300, 1300, 0, 1300);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL, 100, 100, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_MAX, 100, 100, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_COST, 0, 0, 0, 0);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_MIN_COST, 0, 0, 0, 0);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_RECHARGE_RATE, 10, 10, 0, 10);

	LogCallEnd();
}

