#include "src/pch.hpp"

#include "src/GameServer/Engine/PackageMapLevel/CanSerializeObject/PackageMapLevel__CanSerializeObject.hpp"

bool operator==(const FCustomCharacterAssembly& lhs, const FCustomCharacterAssembly& rhs)
{
	return lhs.SuitMeshId == rhs.SuitMeshId &&
		lhs.HeadMeshId == rhs.HeadMeshId &&
		lhs.HairMeshId == rhs.HairMeshId &&
		lhs.HelmetMeshId == rhs.HelmetMeshId &&
		lhs.SkinToneParameterId == rhs.SkinToneParameterId &&
		lhs.SkinRaceParameterId == rhs.SkinRaceParameterId &&
		lhs.EyeColorParameterId == rhs.EyeColorParameterId &&
		lhs.bBald == rhs.bBald &&
		lhs.bHideHelmet == rhs.bHideHelmet &&
		lhs.bValidCustomAssembly == rhs.bValidCustomAssembly &&
		lhs.bHalfHelmet == rhs.bHalfHelmet &&
		lhs.nGenderTypeId == rhs.nGenderTypeId &&
		lhs.HeadFlairId == rhs.HeadFlairId &&
		lhs.SuitFlairId == rhs.SuitFlairId &&
		lhs.JetpackTrailId == rhs.JetpackTrailId &&
		lhs.DyeList[0] == rhs.DyeList[0] &&
		lhs.DyeList[1] == rhs.DyeList[1] &&
		lhs.DyeList[2] == rhs.DyeList[2] &&
		lhs.DyeList[3] == rhs.DyeList[3] &&
		lhs.DyeList[4] == rhs.DyeList[4];
}
bool operator!=(const FCustomCharacterAssembly& lhs, const FCustomCharacterAssembly& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FEffectListEntry& lhs, const FEffectListEntry& rhs)
{
	return lhs.nEffectGroupID == rhs.nEffectGroupID &&
		lhs.byNumStacks == rhs.byNumStacks &&
		lhs.fInitTimeRemaining == rhs.fInitTimeRemaining &&
		lhs.nExtraInfo == rhs.nExtraInfo;
}
bool operator!=(const FEffectListEntry& lhs, const FEffectListEntry& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FScanner_Settings& lhs, const FScanner_Settings& rhs)
{
	return lhs.EffectGroupId == rhs.EffectGroupId &&
		lhs.Scanner_Range == rhs.Scanner_Range &&
		lhs.Scanner_FOV == rhs.Scanner_FOV &&
		lhs.Scanner_RequiresLOS == rhs.Scanner_RequiresLOS &&
		lhs.Scanner_SeeFlag[ 0 ] == rhs.Scanner_SeeFlag[ 0 ] &&
		lhs.Scanner_SeeFlag[ 1 ] == rhs.Scanner_SeeFlag[ 1 ] &&
		lhs.Scanner_SeeFlag[ 2 ] == rhs.Scanner_SeeFlag[ 2 ] &&
		lhs.Scanner_SeeFlag[ 3 ] == rhs.Scanner_SeeFlag[ 3 ] &&
		lhs.Scanner_SeeFlag[ 4 ] == rhs.Scanner_SeeFlag[ 4 ] &&
		lhs.Scanner_SeeFlag[ 5 ] == rhs.Scanner_SeeFlag[ 5 ] &&
		lhs.Scanner_SeeFlag[ 6 ] == rhs.Scanner_SeeFlag[ 6 ] &&
		lhs.Scanner_SeeFlag[ 7 ] == rhs.Scanner_SeeFlag[ 7 ] &&
		lhs.Scanner_DisplayFlag[ 0 ] == rhs.Scanner_DisplayFlag[ 0 ] &&
		lhs.Scanner_DisplayFlag[ 1 ] == rhs.Scanner_DisplayFlag[ 1 ] &&
		lhs.Scanner_DisplayFlag[ 2 ] == rhs.Scanner_DisplayFlag[ 2 ] &&
		lhs.Scanner_DisplayFlag[ 3 ] == rhs.Scanner_DisplayFlag[ 3 ] &&
		lhs.Scanner_DisplayFlag[ 4 ] == rhs.Scanner_DisplayFlag[ 4 ] &&
		lhs.Scanner_DisplayFlag[ 5 ] == rhs.Scanner_DisplayFlag[ 5 ] &&
		lhs.Scanner_DisplayFlag[ 6 ] == rhs.Scanner_DisplayFlag[ 6 ];
}
bool operator!=(const FScanner_Settings& lhs, const FScanner_Settings& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FEffectQueueEntry& lhs, const FEffectQueueEntry& rhs)
{
	return lhs.nEffectGroupID == rhs.nEffectGroupID &&
		lhs.nExtraInfo == rhs.nExtraInfo;
}
bool operator!=(const FEffectQueueEntry& lhs, const FEffectQueueEntry& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FMiniMapInfo& lhs, const FMiniMapInfo& rhs)
{
	return lhs.MinX == rhs.MinX &&
		lhs.MinY == rhs.MinY &&
		lhs.MaxX == rhs.MaxX &&
		lhs.MaxY == rhs.MaxY &&
		lhs.TextureResId == rhs.TextureResId &&
		lhs.TextureUL == rhs.TextureUL &&
		lhs.TextureVL == rhs.TextureVL;
}
bool operator!=(const FMiniMapInfo& lhs, const FMiniMapInfo& rhs)
{
	return !(lhs == rhs);
}
	
bool operator==(const FMapLogoResIdInfo& lhs, const FMapLogoResIdInfo& rhs)
{
	return lhs.m_MapOwnerTextureResId == rhs.m_MapOwnerTextureResId &&
		lhs.m_MapContenderTextureResId == rhs.m_MapContenderTextureResId;
}
bool operator!=(const FMapLogoResIdInfo& lhs, const FMapLogoResIdInfo& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FTgPlayAnimation& lhs, const FTgPlayAnimation& rhs)
{
	return lhs.nAnimationId == rhs.nAnimationId &&
		lhs.bFullBody == rhs.bFullBody &&
		lhs.fSeconds == rhs.fSeconds;
}
bool operator!=(const FTgPlayAnimation& lhs, const FTgPlayAnimation& rhs)
{
	return !(lhs == rhs);
}

// struct FTraceHitInfo
// {
// 	class UMaterial*                                   Material;                                         		// 0x0000 (0x0004) [0x0000000000100000]              
// 	class UPhysicalMaterial*                           PhysMaterial;                                     		// 0x0004 (0x0004) [0x0000000000100000]              
// 	int                                                Item;                                             		// 0x0008 (0x0004) [0x0000000000100000]              
// 	int                                                LevelIndex;                                       		// 0x000C (0x0004) [0x0000000000100000]              
// 	struct FName                                       BoneName;                                         		// 0x0010 (0x0008) [0x0000000000100000]              
// 	class UPrimitiveComponent*                         HitComponent;                                     		// 0x0018 (0x0004) [0x0000000004180008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
// };
bool operator==(const FTraceHitInfo& lhs, const FTraceHitInfo& rhs)
{
	return lhs.Material == rhs.Material &&
		lhs.PhysMaterial == rhs.PhysMaterial &&
		lhs.Item == rhs.Item &&
		lhs.LevelIndex == rhs.LevelIndex &&
		lhs.BoneName == rhs.BoneName &&
		lhs.HitComponent == rhs.HitComponent;
}
bool operator!=(const FTraceHitInfo& lhs, const FTraceHitInfo& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FMusicTrackStruct& lhs, const FMusicTrackStruct& rhs)
{
	return lhs.TheSoundCue == rhs.TheSoundCue &&
		lhs.bAutoPlay == rhs.bAutoPlay &&
		lhs.bPersistentAcrossLevels == rhs.bPersistentAcrossLevels &&
		lhs.FadeInTime == rhs.FadeInTime &&
		lhs.FadeInVolumeLevel == rhs.FadeInVolumeLevel &&
		lhs.FadeOutTime == rhs.FadeOutTime &&
		lhs.FadeOutVolumeLevel == rhs.FadeOutVolumeLevel;
}
bool operator!=(const FMusicTrackStruct& lhs, const FMusicTrackStruct& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FQuat& lhs, const FQuat& rhs)
{
	return lhs.X == rhs.X &&
		lhs.Y == rhs.Y &&
		lhs.Z == rhs.Z &&
		lhs.W == rhs.W;
}

bool operator!=(const FQuat& lhs, const FQuat& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FVector& lhs, const FVector& rhs)
{
	return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
}
bool operator!=(const FVector& lhs, const FVector& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FRigidBodyState& lhs, const FRigidBodyState& rhs)
{
	return lhs.Position == rhs.Position &&
		lhs.Quaternion == rhs.Quaternion &&
		lhs.LinVel == rhs.LinVel &&
		lhs.AngVel == rhs.AngVel &&
		lhs.bNewData == rhs.bNewData;
}
bool operator!=(const FRigidBodyState& lhs, const FRigidBodyState& rhs)
{
	return !(lhs == rhs);
}
bool operator==(const FVehicleState& lhs, const FVehicleState& rhs)
{
	return lhs.RBState == rhs.RBState &&
		lhs.ServerBrake == rhs.ServerBrake &&
		lhs.ServerGas == rhs.ServerGas &&
		lhs.ServerSteering == rhs.ServerSteering &&
		lhs.ServerRise == rhs.ServerRise &&
		lhs.bServerHandbrake == rhs.bServerHandbrake &&
		lhs.ServerView == rhs.ServerView;
}
bool operator!=(const FVehicleState& lhs, const FVehicleState& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FQWord& lhs, const FQWord& rhs)
{
	return lhs.A == rhs.A && lhs.B == rhs.B;
}
bool operator!=(const FQWord& lhs, const FQWord& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FGuid& lhs, const FGuid& rhs) {
	return lhs.A == rhs.A && lhs.B == rhs.B && lhs.C == rhs.C && lhs.D == rhs.D;
}
bool operator!=(const FGuid& lhs, const FGuid& rhs) {
	return !(lhs == rhs);
}

bool operator==(const FUniqueNetId& lhs, const FUniqueNetId& rhs)
{
	return lhs.Uid == rhs.Uid;
}
bool operator!=(const FUniqueNetId& lhs, const FUniqueNetId& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const FVector2D& lhs, const FVector2D& rhs) {
	return lhs.X == rhs.X && lhs.Y == rhs.Y;
}
bool operator!=(const FVector2D& lhs, const FVector2D& rhs) {
	return !(lhs == rhs);
}

bool NEQ(int A,int B,void* Map,void* Channel) {
	return A!=B;
}
bool NEQ(float& A,float& B,void* Map,void* Channel) {
	return *(int*)&A!=*(int*)&B;
}
bool NEQ(FVector& A,FVector& B,void* Map,void* Channel) {
	return ((int*)&A)[0]!=((int*)&B)[0] || ((int*)&A)[1]!=((int*)&B)[1] || ((int*)&A)[2]!=((int*)&B)[2];
}
bool NEQ(FRotator& A,FRotator& B,void* Map,void* Channel) {
	return A.Pitch>>8 != B.Pitch>>8 || A.Yaw>>8 != B.Yaw>>8 || A.Roll>>8 != B.Roll>>8;
}
bool NEQ(UObject* A,UObject* B,void* Map,void* Channel) {
	if (PackageMapLevel__CanSerializeObject::CallOriginal(Map, nullptr, (int)A)) {
		return A!=B;
	}
	*(int*)((char*)Channel + 0x94) = *(int*)((char*)Channel + 0x94) | 4;
	return (B!=NULL);
}
bool NEQ(FName& A,FName B,void* Map,void* Channel) {
	return *(int*)&A!=*(int*)&B;
}
bool NEQ(FColor& A,FColor& B,void* Map,void* Channel) {
	return *(int*)&A!=*(int*)&B;
}
bool NEQ(FLinearColor& A,FLinearColor& B,void* Map,void* Channel) {
	return A.R!=B.R || A.G!=B.G || A.B!=B.B || A.A!=B.A;
}
bool NEQ(FPlane& A,FPlane& B,void* Map,void* Channel) {
	return ((int*)&A)[0]!=((int*)&B)[0] || ((int*)&A)[1]!=((int*)&B)[1] ||
		((int*)&A)[2]!=((int*)&B)[2] || ((int*)&A)[3]!=((int*)&B)[3];
}
bool NEQ(FString& A,FString& B,void* Map,void* Channel) {
	return A!=B;
}
bool NEQ(FVehicleState& A,FVehicleState& B,void* Map,void* Channel) {
	return 1;
}
bool NEQ(FRigidBodyState& A,FRigidBodyState& B,void* Map,void* Channel)
{
	return 1;
	// return ((A.Position - B.Position).SizeSquared() > 0.4f || (A.Quaternion - B.Quaternion).SizeSquared() > 0.001f || A.bNewData != B.bNewData);
}
bool NEQ(FDeviceStatInfo& A,FDeviceStatInfo& B,void* Map,void* Channel) {
	return A.Stats[0] != B.Stats[0] || A.Stats[1] != B.Stats[1] || A.Stats[2] != B.Stats[2] || A.Stats[3] != B.Stats[3]
	|| A.Stats[4] != B.Stats[4] || A.Stats[5] != B.Stats[5] || A.Stats[6] != B.Stats[6] || A.Stats[7] != B.Stats[7]
	|| A.Stats[8] != B.Stats[8] || A.Stats[9] != B.Stats[9] || A.Stats[10] != B.Stats[10];
}
bool NEQ(FEquipDeviceInfo& A,FEquipDeviceInfo& B,void* Map,void* Channel) {
	return 1;
	// return A.nDeviceId != B.nDeviceId || A.nDeviceInstanceId != B.nDeviceInstanceId || A.nQualityValueId != B.nQualityValueId;
}
bool NEQ(FEffectListEntry& A,FEffectListEntry& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FScanner_Settings& A,FScanner_Settings& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FEffectQueueEntry& A,FEffectQueueEntry& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FVector2D& A,FVector2D& B,void* Map,void* Channel) {return A.X!=B.X || A.Y!=B.Y;}
bool NEQ(FGuid& A,FGuid& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FCustomCharacterAssembly& A,FCustomCharacterAssembly& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FMiniMapInfo& A,FMiniMapInfo& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FMapLogoResIdInfo& A,FMapLogoResIdInfo& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FTgPlayAnimation& A,FTgPlayAnimation& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FTraceHitInfo& A,FTraceHitInfo& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FMusicTrackStruct& A,FMusicTrackStruct& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FQuat& A,FQuat& B,void* Map,void* Channel) {return A != B;}
// bool NEQ(FRigidBodyState& A,FRigidBodyState& B,void* Map,void* Channel) {return A != B;}
// bool NEQ(FVehicleState& A,FVehicleState& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FQWord& A,FQWord& B,void* Map,void* Channel) {return A != B;}
bool NEQ(FUniqueNetId& A,FUniqueNetId& B,void* Map,void* Channel) {return A != B;}
// bool NEQ(FVector2D& A,FVector2D& B,void* Map,void* Channel) {return A != B;}
// bool NEQ(FGuid& A,FGuid& B,void* Map,void* Channel) {return A != B;}
