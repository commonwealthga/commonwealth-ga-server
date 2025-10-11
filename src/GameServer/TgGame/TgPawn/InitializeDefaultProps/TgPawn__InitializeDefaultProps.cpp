#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"

void __fastcall* TgPawn__InitializeDefaultProps::Call(ATgPawn* Pawn, void* edx) {
	// LogToFile("C:\\mylog.txt", "MINE TgPawn::InitializeDefaultProps START");

	Pawn->r_nPhysicalType = 860;
	Pawn->ReplicatedCollisionType = Pawn->CollisionType;
	Pawn->r_nHealthMaximum = 1300;
	Pawn->r_nProfileId = 567; // medic
	Pawn->r_bDisableAllDevices = 0;
	Pawn->r_bEnableEquip = 1;
	Pawn->r_bEnableSkills = 1;
	Pawn->r_bEnableCrafting = 1;
	Pawn->r_bIsStealthed = 0;
	Pawn->r_bIsBot = 0;
	Pawn->r_fCurrentPowerPool = 100;
	Pawn->r_fMaxPowerPool = 100;
	Pawn->r_nXp = 999999;
	Pawn->Health = 1300;
	Pawn->HealthMax = 1300;

	if (strcmp(Pawn->Class->GetFullName(), "Class TgGame.TgPawn_Character") == 0) {
		ATgPawn_Character* newpawnchar = (ATgPawn_Character*)Pawn;
		newpawnchar->r_nBodyMeshAsmId = 1225;//0x5cc;
		newpawnchar->r_CustomCharacterAssembly.SuitMeshId = 1225;
		newpawnchar->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
		newpawnchar->r_CustomCharacterAssembly.HairMeshId = 1974;
		newpawnchar->r_CustomCharacterAssembly.HelmetMeshId = -1;
		newpawnchar->r_CustomCharacterAssembly.SkinToneParameterId = 0;
		newpawnchar->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
		newpawnchar->r_CustomCharacterAssembly.EyeColorParameterId = 0;
		newpawnchar->r_CustomCharacterAssembly.bBald = false;
		newpawnchar->r_CustomCharacterAssembly.bHideHelmet = false;
		newpawnchar->r_CustomCharacterAssembly.bValidCustomAssembly = true;
		newpawnchar->r_CustomCharacterAssembly.bHalfHelmet = false;
		newpawnchar->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
		newpawnchar->r_CustomCharacterAssembly.HeadFlairId = -1;
		newpawnchar->r_CustomCharacterAssembly.SuitFlairId = -1;
		newpawnchar->r_CustomCharacterAssembly.JetpackTrailId = 7638;
		newpawnchar->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
		newpawnchar->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
		newpawnchar->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
		newpawnchar->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
		newpawnchar->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;
		newpawnchar->r_nSkillGroupSetId = GA_G::SKILL_GROUP_SET_ID_MEDIC;
		newpawnchar->s_nCharacterId = 373;

	}

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
}

