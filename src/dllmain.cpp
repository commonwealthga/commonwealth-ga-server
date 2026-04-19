#include "src/pch.hpp"

#include "src/Utils/Logger/Logger.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/DebugWindow/DebugWindow.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp"
#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"
#include "src/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.hpp"
#include "src/GameServer/Engine/World/BeginPlay/World__BeginPlay.hpp"
#include "src/GameServer/Engine/SeqActNullGuard/SeqActNullGuard.hpp"
#include "src/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.hpp"
#include "src/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.hpp"
#include "src/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.hpp"
#include "src/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.hpp"
#include "src/GameServer/Engine/NetConnection/SendPackageMap/NetConnection__SendPackageMap.hpp"
#include "src/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.hpp"
#include "src/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/GameServer/Engine/ActorChannel/ReceivedBunch/CanExecute/ActorChannel__ReceivedBunch__CanExecute.hpp"
#include "src/GameServer/Engine/Channel/ReceivedSequencedBunch/Channel__ReceivedSequencedBunch.hpp"
#include "src/GameServer/TgGame/TgPlayerController/IsReadyForStart/TgPlayerController__IsReadyForStart.hpp"
#include "src/GameServer/TgGame/TgPlayerController/SetSoundMode/TgPlayerController__SetSoundMode.hpp"
#include "src/GameServer/TgGame/TgPlayerController/CanPlayerUseVolume/TgPlayerController__CanPlayerUseVolume.hpp"
#include "src/GameServer/TgGame/TgPlayerController/GetViewTarget/TgPlayerController__GetViewTarget.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerAcceptNewProfileFromEquipScreen/TgPlayerController__ServerAcceptNewProfileFromEquipScreen.hpp"
#include "src/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotPawn/TgGame__SpawnBotPawn.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.hpp"
#include "src/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.hpp"
#include "src/GameServer/TgGame/TgGame/SendMissionTimerEvent/TgGame__SendMissionTimerEvent.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/FinalizeRoundScore/TgGame_Arena__FinalizeRoundScore.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/FinalizeGameScore/TgGame_Arena__FinalizeGameScore.hpp"
#include "src/GameServer/TgGame/TgGame_PointRotation/CalcNextObjective/TgGame_PointRotation__CalcNextObjective.hpp"
#include "src/GameServer/TgGame/TgGame_PointRotation/UnlockObjective/TgGame_PointRotation__UnlockObjective.hpp"
#include "src/GameServer/TgGame/TgGame/CheckRandomObjectives/TgGame__CheckRandomObjectives.hpp"
#include "src/GameServer/TgGame/TgGame/UnlockObjective/TgGame__UnlockObjective.hpp"
#include "src/GameServer/TgGame/TgGame/LockoutObjectives/TgGame__LockoutObjectives.hpp"
#include "src/GameServer/TgGame/TgGame/IsFinalObjective/TgGame__IsFinalObjective.hpp"
#include "src/GameServer/TgGame/TgGame/SetObjectivesInactive/TgGame__SetObjectivesInactive.hpp"
#include "src/GameServer/TgGame/TgGame/SetObjectivesOvertimeNotify/TgGame__SetObjectivesOvertimeNotify.hpp"
#include "src/GameServer/TgGame/TgGame/GetFinalObjectivesList/TgGame__GetFinalObjectivesList.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/SetObjectiveActive/TgMissionObjective__SetObjectiveActive.hpp"
#include "src/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.hpp"
#include "src/GameServer/TgGame/TgPawn/SwapAttachedDeviceMaterials/TgPawn__SwapAttachedDeviceMaterials.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepList.hpp"
#include "src/GameServer/Engine/Actor/Spawn/Actor__Spawn.hpp"
#include "src/GameServer/Engine/Actor/Tick/Actor__Tick.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.hpp"
#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBot/TgGame__SpawnBot.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetPropertyValueById/TgDeviceFire__GetPropertyValueById.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/InitializeProjectile/TgDeviceFire__InitializeProjectile.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/CustomFire/TgDeviceFire__CustomFire.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/Deploy/TgDeviceFire__Deploy.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/GameServer/TgGame/TgDevice/HasEnoughPowerPool/TgDevice__HasEnoughPowerPool.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/TgEffectManager/ApplyDamage/TgEffectManager__ApplyDamage.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/GameServer/TgGame/TgDevice/HasMinimumPowerPool/TgDevice__HasMinimumPowerPool.hpp"
#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/GameServer/Misc/CMarshal/GetGuid/CMarshal__GetGuid.hpp"
#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotSpawnTableMarshal/CAmBot__LoadBotSpawnTableMarshal.hpp"
#include "src/GameServer/Misc/CAmDeviceModel/LoadDeviceMarshal/CAmDeviceModel__LoadDeviceMarshal.hpp"
#include "src/GameServer/Misc/CAmDeviceModel/LoadDeviceModeMarshal/CAmDeviceModel__LoadDeviceModeMarshal.hpp"
#include "src/GameServer/Misc/CAmEffectModel/LoadEffectGroupMarshal/CAmEffectModel__LoadEffectGroupMarshal.hpp"
#include "src/GameServer/Misc/CAmEffectModel/LoadEffectMarshal/CAmEffectModel__LoadEffectMarshal.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.hpp"
#include "src/Database/AsmDataCapture/AsmDataCapture.hpp"
#include "src/GameServer/Misc/CMarshal/GetName/CMarshal__GetName.hpp"
#include "src/GameServer/Misc/CMarshal/GetWcharT/CMarshal__GetWcharT.hpp"

// --- stub hook includes (logging only) ---
#include "src/GameServer/TgGame/TgBotFactory/ClearQueue/TgBotFactory__ClearQueue.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnBotAdjusted/TgBotFactory__SpawnBotAdjusted.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnBotId/TgBotFactory__SpawnBotId.hpp"
#include "src/GameServer/TgGame/TgBotFactory/UseSpawnTable/TgBotFactory__UseSpawnTable.hpp"
#include "src/GameServer/TgGame/TgDeployable/AddProperty/TgDeployable__AddProperty.hpp"
#include "src/GameServer/TgGame/TgDeployable/InitializeDefaultProps/TgDeployable__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgDeployable/NotifyGroupChanged/TgDeployable__NotifyGroupChanged.hpp"
#include "src/GameServer/TgAssemblyMisc/LoadAssetRefs/TgAssemblyMisc__LoadAssetRefs.hpp"
#include "src/GameServer/Core/LoadObject/Core__LoadObject.hpp"
#include "src/GameServer/TgGame/TgDevice/ApplyInventoryEquipEffects/TgDevice__ApplyInventoryEquipEffects.hpp"
#include "src/GameServer/TgGame/TgDevice/ClearInstigatorEquippedDevices/TgDevice__ClearInstigatorEquippedDevices.hpp"
#include "src/GameServer/TgGame/TgDevice/PopulateInstigatorEquippedDevices/TgDevice__PopulateInstigatorEquippedDevices.hpp"
#include "src/GameServer/TgGame/TgEffectManager/GetSkillBasedEffectGroup/TgEffectManager__GetSkillBasedEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/IsStrongest/TgEffectManager__IsStrongest.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/CloneEffectGroup/TgEffectGroup__CloneEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/TgGame_Arena/AdjustBeaconForwardSpawn/TgGame_Arena__AdjustBeaconForwardSpawn.hpp"
#include "src/GameServer/TgGame/TgGame_Control/CalcAttackerReviveTime/TgGame_Control__CalcAttackerReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame_Control/CalcDefenderReviveTime/TgGame_Control__CalcDefenderReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame_Control/SendCountdownRemainingMessages/TgGame_Control__SendCountdownRemainingMessages.hpp"
#include "src/GameServer/TgGame/TgGame_Control/TickCountdownCalculation/TgGame_Control__TickCountdownCalculation.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/CacheKismetConfiguration/TgGame_Defense__CacheKismetConfiguration.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/CheckWinGame/TgGame_Defense__CheckWinGame.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/CheckWinRound/TgGame_Defense__CheckWinRound.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/FinalizeRoundScore/TgGame_Defense__FinalizeRoundScore.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/LoadGameConfig/TgGame_Defense__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/LockoutObjectives/TgGame_Defense__LockoutObjectives.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/SendNewRoundMessage/TgGame_Defense__SendNewRoundMessage.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/TickWaveNodes/TgGame_Defense__TickWaveNodes.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/UnlockObjective/TgGame_Defense__UnlockObjective.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/AwardTickets/TgGame_Ticket__AwardTickets.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/BeginEndMission/TgGame_Ticket__BeginEndMission.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/LoadGameConfig/TgGame_Ticket__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/TickTicketsCalculation/TgGame_Ticket__TickTicketsCalculation.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.hpp"
#include "src/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.hpp"
#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame/CalcAwardMedal/TgGame__CalcAwardMedal.hpp"
#include "src/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.hpp"
#include "src/GameServer/TgGame/TgGame/DbSaveReward/TgGame__DbSaveReward.hpp"
#include "src/GameServer/TgGame/TgGame/DbUpdateQuests/TgGame__DbUpdateQuests.hpp"
#include "src/GameServer/TgGame/TgGame/FinishEndMission/TgGame__FinishEndMission.hpp"
#include "src/GameServer/TgGame/TgGame/GetDifficultyModifier/TgGame__GetDifficultyModifier.hpp"
#include "src/GameServer/TgGame/TgGame/Loot/TgGame__Loot.hpp"
#include "src/GameServer/TgGame/TgGame/NotifyPostCommitMapChange/TgGame__NotifyPostCommitMapChange.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnAllHenchman/TgGame__SpawnAllHenchman.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnLandMarks/TgGame__SpawnLandMarks.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnTemplatePlayer/TgGame__SpawnTemplatePlayer.hpp"
#include "src/GameServer/TgGame/TgGame/TgFindPlayerSpawnLocation/TgGame__TgFindPlayerSpawnLocation.hpp"
#include "src/GameServer/TgGame/TgGame/UpdateMissionTimerEventWinVar/TgGame__UpdateMissionTimerEventWinVar.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/ApplyAllEnhancementEffects/TgInventoryManager__ApplyAllEnhancementEffects.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/GetDeviceByInstanceId/TgInventoryManager__GetDeviceByInstanceId.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/InventoryCleanup/TgInventoryManager__InventoryCleanup.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/SetInventoryDirty/TgInventoryManager__SetInventoryDirty.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ApplyItemEffects/TgPawn_Character__ApplyItemEffects.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/CraftItem/TgPawn_Character__CraftItem.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/DismissVanityPet/TgPawn_Character__DismissVanityPet.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ReapplyLoadoutEffects/TgPawn_Character__ReapplyLoadoutEffects.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/RemoveCharacterSkillTree/TgPawn_Character__RemoveCharacterSkillTree.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/SendCharacterSkillMarshal/TgPawn_Character__SendCharacterSkillMarshal.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ServerOnSetPlayerMesh/TgPawn_Character__ServerOnSetPlayerMesh.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ServerPurchaseItem/TgPawn_Character__ServerPurchaseItem.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/ServerRemoveAllCharSkills/TgPawn_Character__ServerRemoveAllCharSkills.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/SetCurrentItemProfile/TgPawn_Character__SetCurrentItemProfile.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/SpawnVanityPet/TgPawn_Character__SpawnVanityPet.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/UpdateDurability/TgPawn_Character__UpdateDurability.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/VanityPetDestroyed/TgPawn_Character__VanityPetDestroyed.hpp"
#include "src/GameServer/TgGame/TgPawn/AddProperty/TgPawn__AddProperty.hpp"
#include "src/GameServer/TgGame/TgPawn/AddDamageInfo/TgPawn__AddDamageInfo.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackBotHealing/TgPawn__TrackBotHealing.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackCompleteKillInfo/TgPawn__TrackCompleteKillInfo.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDamagedBot/TgPawn__TrackDamagedBot.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDamagedPlayer/TgPawn__TrackDamagedPlayer.hpp"
#include "src/GameServer/TgGame/TgPawn_Character/SendCombatMessage/TgPawn_Character__SendCombatMessage.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackHealing/TgPawn__TrackHealing.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackHit/TgPawn__TrackHit.hpp"
#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerApplyFlair/TgPlayerController__ServerApplyFlair.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerTestSystemMailItem/TgPlayerController__ServerTestSystemMailItem.hpp"
#include "src/GameServer/TgGame/TgRepInfo_Player/OnAllFlairManifestsLoaded/TgRepInfo_Player__OnAllFlairManifestsLoaded.hpp"
#include "src/GameServer/TgGame/TgRepInfo_Player/OnProfileChanged/TgRepInfo_Player__OnProfileChanged.hpp"
#include "src/GameServer/TgGame/TgRepInfo_Player/UpdateScoreBoard/TgRepInfo_Player__UpdateScoreBoard.hpp"
#include "src/GameServer/TgGame/TgMeshAssembly/ForceNetRelevant/TgMeshAssembly__ForceNetRelevant.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/TgGame/TgDynamicSMActor/ForceNetRelevant/TgDynamicSMActor__ForceNetRelevant.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyDye/TgPawn__ApplyDye.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyJetpackTrail/TgPawn__ApplyJetpackTrail.hpp"
#include "src/GameServer/TgGame/TgPawn/BeginStats/TgPawn__BeginStats.hpp"
#include "src/GameServer/TgGame/TgPawn/CanMove/TgPawn__CanMove.hpp"
#include "src/GameServer/TgGame/TgPawn/CheckKillQuestCredit/TgPawn__CheckKillQuestCredit.hpp"
#include "src/GameServer/TgGame/TgPawn/CheckUseQuestCredit/TgPawn__CheckUseQuestCredit.hpp"
#include "src/GameServer/TgGame/TgPawn/EndStats/TgPawn__EndStats.hpp"
#include "src/GameServer/TgGame/TgPawn/GetDyeItemId/TgPawn__GetDyeItemId.hpp"
#include "src/GameServer/TgGame/TgPawn/GetJetpackTrailId/TgPawn__GetJetpackTrailId.hpp"
#include "src/GameServer/TgGame/TgPawn/GetMoraleDevice/TgPawn__GetMoraleDevice.hpp"
#include "src/GameServer/TgGame/TgPawn/GiveKillXp/TgPawn__GiveKillXp.hpp"
#include "src/GameServer/TgGame/TgPawn/InitSpawnPets/TgPawn__InitSpawnPets.hpp"
#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/GameServer/TgGame/TgPawn/MakeInvulnerable/TgPawn__MakeInvulnerable.hpp"
#include "src/GameServer/TgGame/TgPawn/OnPetSpawned/TgPawn__OnPetSpawned.hpp"
#include "src/GameServer/TgGame/TgPawn/OnProjectileExploded/TgPawn__OnProjectileExploded.hpp"
#include "src/GameServer/TgGame/TgPawn/ReapplyCharacterSkillTree/TgPawn__ReapplyCharacterSkillTree.hpp"
#include "src/GameServer/TgGame/TgPawn/ReapplyLoadoutEffects/TgPawn__ReapplyLoadoutEffects.hpp"
#include "src/GameServer/TgGame/TgPawn/RemoveTrackFired/TgPawn__RemoveTrackFired.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnEquipCharDevice/TgPawn__ServerOnEquipCharDevice.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnEquipCharDevices/TgPawn__ServerOnEquipCharDevices.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnRequestMission/TgPawn__ServerOnRequestMission.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnSetPlayerLevel/TgPawn__ServerOnSetPlayerLevel.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnSetPlayerMesh/TgPawn__ServerOnSetPlayerMesh.hpp"
#include "src/GameServer/TgGame/TgPawn/SetDeploySensorDetectedStealthLightup/TgPawn__SetDeploySensorDetectedStealthLightup.hpp"
#include "src/GameServer/TgGame/TgPawn/SetDyeItemId/TgPawn__SetDyeItemId.hpp"
#include "src/GameServer/TgGame/TgPawn/SetJetpackTrailId/TgPawn__SetJetpackTrailId.hpp"
#include "src/GameServer/TgGame/TgPawn/SpawnLoot/TgPawn__SpawnLoot.hpp"
#include "src/GameServer/TgGame/TgPawn/StatsCleanup/TgPawn__StatsCleanup.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackBoost/TgPawn__TrackBoost.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackBuff/TgPawn__TrackBuff.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDeath/TgPawn__TrackDeath.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDefense/TgPawn__TrackDefense.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackEscortObjective/TgPawn__TrackEscortObjective.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackFired/TgPawn__TrackFired.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackFromPlayerDeath/TgPawn__TrackFromPlayerDeath.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackKilledBot/TgPawn__TrackKilledBot.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackKilledPlayer/TgPawn__TrackKilledPlayer.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackMyBeaconUsed/TgPawn__TrackMyBeaconUsed.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackObjective/TgPawn__TrackObjective.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackObjectivePoints/TgPawn__TrackObjectivePoints.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackObjectivePointsByProgress/TgPawn__TrackObjectivePointsByProgress.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackReleaseTime/TgPawn__TrackReleaseTime.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackSelfDamage/TgPawn__TrackSelfDamage.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackSelfKill/TgPawn__TrackSelfKill.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackTeamDamage/TgPawn__TrackTeamDamage.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackTeamKill/TgPawn__TrackTeamKill.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdateBuffer/TgPawn__UpdateBuffer.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdateControllerVisBasedProperties/TgPawn__UpdateControllerVisBasedProperties.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdateDamagers/TgPawn__UpdateDamagers.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdateDebuffer/TgPawn__UpdateDebuffer.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdateHUDScores/TgPawn__UpdateHUDScores.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdateHealer/TgPawn__UpdateHealer.hpp"
#include "src/GameServer/TgGame/TgPawn/UpdatePRIAssetRefs/TgPawn__UpdatePRIAssetRefs.hpp"
#include "src/GameServer/TgGame/TgPawn/ValidateStatsTracker/TgPawn__ValidateStatsTracker.hpp"
#include "src/GameServer/TgGame/TgPlayerController/DebugFn/TgPlayerController__DebugFn.hpp"
#include "src/GameServer/TgGame/TgPlayerController/FinalSave/TgPlayerController__FinalSave.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerAbandonAssignment/TgPlayerController__ServerAbandonAssignment.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerActivateInvItem/TgPlayerController__ServerActivateInvItem.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerAddHZPoints/TgPlayerController__ServerAddHZPoints.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerAddToken/TgPlayerController__ServerAddToken.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerChangeCoalition/TgPlayerController__ServerChangeCoalition.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerChangeTaskForce/TgPlayerController__ServerChangeTaskForce.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerClearProfiles/TgPlayerController__ServerClearProfiles.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerClearSkillsAndDevices/TgPlayerController__ServerClearSkillsAndDevices.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerCombineItems/TgPlayerController__ServerCombineItems.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerCraftItem/TgPlayerController__ServerCraftItem.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerDestroyInvItem/TgPlayerController__ServerDestroyInvItem.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerDevGiveXP/TgPlayerController__ServerDevGiveXP.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerGMGiven/TgPlayerController__ServerGMGiven.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerLoadItemProfile/TgPlayerController__ServerLoadItemProfile.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerLogSpeedHack/TgPlayerController__ServerLogSpeedHack.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerMarkSpawnReturn/TgPlayerController__ServerMarkSpawnReturn.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerObama/TgPlayerController__ServerObama.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerRepairAllUpgrades/TgPlayerController__ServerRepairAllUpgrades.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerRepairInvItem/TgPlayerController__ServerRepairInvItem.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerRequestAssignment/TgPlayerController__ServerRequestAssignment.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerRequestBeaconNetworkHop/TgPlayerController__ServerRequestBeaconNetworkHop.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerSalvageInvItem/TgPlayerController__ServerSalvageInvItem.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerSetLevel/TgPlayerController__ServerSetLevel.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerSetPawnAlwaysRelevant/TgPlayerController__ServerSetPawnAlwaysRelevant.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerSetSpawnAtMe/TgPlayerController__ServerSetSpawnAtMe.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ServerSetTaskForceLead/TgPlayerController__ServerSetTaskForceLead.hpp"
#include "src/GameServer/TgGame/TgPlayerController/SetHomeMapGame/TgPlayerController__SetHomeMapGame.hpp"
#include "src/GameServer/TgGame/TgPlayerController/ShouldAutoKick/TgPlayerController__ShouldAutoKick.hpp"


unsigned long ModuleThread( void* ) {
	// Logger::EnabledChannels.push_back("hook_calltree");
	// Logger::EnabledChannels.push_back("combat-trace");
	Logger::EnabledChannels.push_back("asset_load");
	Logger::EnabledChannels.push_back("deploy_phase");
	Logger::EnabledChannels.push_back("team_colors");
	Logger::EnabledChannels.push_back("pet_spawn");
	Logger::EnabledChannels.push_back("bomb");
	Logger::EnabledChannels.push_back("deployable_props");
	Logger::EnabledChannels.push_back("heal_tick");
	Logger::EnabledChannels.push_back("skills");
	Logger::EnabledChannels.push_back("skills-trace");
	Logger::EnabledChannels.push_back("skills_debug");
	// Logger::EnabledChannels.push_back("ipc");
	// Logger::EnabledChannels.push_back("kismet");
	// Logger::EnabledChannels.push_back("tgbotfactory");
	Logger::EnabledChannels.push_back("effects");
	// Logger::EnabledChannels.push_back("firedeploy_m_pAmSetup");
	// Logger::EnabledChannels.push_back("firedeploy_m_pFireModeSetup");
	// Logger::EnabledChannels.push_back("firespawnpet_m_pAmSetup");
	// Logger::EnabledChannels.push_back("firespawnpet_m_pFireModeSetup");
	// Logger::EnabledChannels.push_back("tcp");
	// Logger::EnabledChannels.push_back("db");
	// Logger::EnabledChannels.push_back("process_event");
	// Logger::EnabledChannels.push_back("damage-info");
	// Logger::EnabledChannels.push_back("debug");
	// Logger::EnabledChannels.push_back("quest_update");
	// Logger::EnabledChannels.push_back("session_guid");
	// Logger::EnabledChannels.push_back("chat");
	// Logger::EnabledChannels.push_back("matchmaking");
	// Logger::EnabledChannels.push_back("packagemap");
	// Logger::EnabledChannels.push_back("stealth");



	Database::Init();

	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());

	// low-level engine functions
	GameEngine__Init::Install();
	// UObject__CollectGarbage::bDisableGarbageCollection = true;
	UObject__CollectGarbage::Install();
	UObject__ProcessEvent::Install();
	FMallocWindows__Free::Install();
	World__BeginPlay::Install();
	SeqActNullGuard::Install();
	ConstructCommandletObject::Install();
	ServerCommandlet__Main::Install();
	GameEngine__SpawnServerActors::Install();
	UdpNetDriver__InitListen::Install();
	UdpNetDriver__TickDispatch::Install();
	ClientConnection__SendMarshal::Install();
	NetConnection__LowLevelSend::Install();
	NetConnection__Cleanup::Install();
	NetConnection__CleanupActor::Install();
	MarshalChannel__MarshalReceived::Install();
	MarshalChannel__NotifyControlMessage::Install();
	NetConnection__SendPackageMap::Install();
	ActorChannel__ReceivedBunch__CanExecute::bLogEnabled = true;
	ActorChannel__ReceivedBunch__CanExecute::Install();
	Channel__ReceivedSequencedBunch::Install();
	Actor__GetOptimizedRepList::Install();
	Actor__Spawn::Install();
	Actor__Tick::Install();

	// game functions
	TgPlayerController__IsReadyForStart::Install();
	TgPlayerController__SetSoundMode::Install();
	TgPlayerController__CanPlayerUseVolume::Install();
	TgPlayerController__GetViewTarget::Install();
	TgPlayerController__ServerAcceptNewProfileFromEquipScreen::Install();
	// TgGame__TgFindPlayerStart::Install();
	TgGame__SpawnPlayerCharacter::Install();
	TgGame__SpawnBotPawn::Install();
	TgGame__LoadGameConfig::Install();
	TgGame__InitGameRepInfo::Install();
	TgGame_Arena__LoadGameConfig::Install();
	TgGame_Arena__FinalizeRoundScore::Install();
	TgGame_Arena__FinalizeGameScore::Install();
	TgGame_PointRotation__CalcNextObjective::Install();
	TgGame_PointRotation__UnlockObjective::Install();
	TgMissionObjective__SetObjectiveActive::Install();
	TgGame__CheckRandomObjectives::Install();
	TgGame__UnlockObjective::Install();
	TgGame__LockoutObjectives::Install();
	TgGame__IsFinalObjective::Install();
	TgGame__SetObjectivesInactive::Install();
	TgGame__SetObjectivesOvertimeNotify::Install();
	TgGame__GetFinalObjectivesList::Install();
	TgPawn__InitializeDefaultProps::Install();
	TgPawn__GetProperty::Install();
	TgPawn__SetTaskForceNumber::Install();
	// TgPawn__SwapAttachedDeviceMaterials::Install();
	TgTeamBeaconManager__SpawnNewBeaconForTeam::Install();
	TgBeaconFactory__SpawnObject::Install();
	TgInventoryManager__NonPersistAddDevice::Install();
	TgInventoryManager__NonPersistRemoveDevice::Install();
	TgBotFactory__LoadObjectConfig::Install();
	// TgBotFactory__SpawnBot::Install();
	TgBotFactory__SpawnNextBot::Install();
	TgBotFactory__SpawnWave::Install();
	TgBotFactory__ResetQueue::Install();
	TgGame__SpawnBot::Install();
	TgGame__SpawnBotById::Install();
	TgGame__RegisterForWaveRevive::Install();
	TgGame__GetReviveTimeRemaining::Install();
	TgGame__ReviveAttackersTimer::Install();
	TgGame__ReviveDefendersTimer::Install();
	TgGame__MissionTimeRemaining::Install();
	TgGame__SendMissionTimerEvent::Install();
	TgDeviceFire__GetEffectGroup::Install();
	TgDeviceFire__GetPropertyValueById::Install();
	TgDeviceFire__InitializeProjectile::Install();
	TgDeviceFire__CustomFire::Install();
	TgDeviceFire__Deploy::Install();
	TgDeviceFire__SpawnPet::Install();
	TgDevice__HasMinimumPowerPool::Install();
	// TgEffectManager__ApplyDamage::Install();
	TgEffectManager__RemoveAllEffectGroups::Install();
	TgEffectManager__RemoveAllEffects::Install();
	TgDevice__HasEnoughPowerPool::Install();
	TgProj_Deployable__SpawnDeployable::Install();
	TgMissionObjective_Bot__SpawnObjectiveBot::Install();

	// Diagnostic hooks for the ApplyDeployableSetup → LoadObject crash.
	// Both log on the "asset_load" channel. Remove once the crash is fixed.
	TgAssemblyMisc__LoadAssetRefs::Install();
	Core__LoadObject::Install();

	// --- stub hooks (logging only) ---
	TgBotFactory__ClearQueue::Install();
	TgBotFactory__SpawnBotAdjusted::Install();
	TgBotFactory__SpawnBotId::Install();
	TgBotFactory__UseSpawnTable::Install();
	TgDeployable__AddProperty::Install();
	TgDeployable__InitializeDefaultProps::Install();
	TgDeployable__NotifyGroupChanged::Install();
	TgDevice__ApplyInventoryEquipEffects::Install();
	TgDevice__ClearInstigatorEquippedDevices::Install();
	TgDevice__PopulateInstigatorEquippedDevices::Install();
	TgEffectGroup__CloneEffectGroup::Install();
	TgEffectGroup__RemoveEffects::Install();
	TgEffectManager__GetSkillBasedEffectGroup::Install();
	TgEffectManager__IsStrongest::Install();
	TgEffectManager__RemoveEffectGroup::Install();
	TgEffectManager__RemoveEffectGroupsByCategory::Install();
	TgGame_Arena__AdjustBeaconForwardSpawn::Install();
	TgGame_Control__CalcAttackerReviveTime::Install();
	TgGame_Control__CalcDefenderReviveTime::Install();
	TgGame_Control__SendCountdownRemainingMessages::Install();
	TgGame_Control__TickCountdownCalculation::Install();
	TgGame_Defense__CacheKismetConfiguration::Install();
	TgGame_Defense__CheckWinGame::Install();
	TgGame_Defense__CheckWinRound::Install();
	TgGame_Defense__FinalizeRoundScore::Install();
	TgGame_Defense__LoadGameConfig::Install();
	TgGame_Defense__LockoutObjectives::Install();
	TgGame_Defense__SendNewRoundMessage::Install();
	TgGame_Defense__TickWaveNodes::Install();
	TgGame_Defense__UnlockObjective::Install();
	TgGame_Ticket__AwardTickets::Install();
	TgGame_Ticket__BeginEndMission::Install();
	TgGame_Ticket__LoadGameConfig::Install();
	TgGame_Ticket__TickTicketsCalculation::Install();
	TgGame_Ticket__UpdateGameWinState::Install();
	TgGame__AdjustBeaconForwardSpawn::Install();
	TgGame__BeginEndMission::Install();
	TgGame__CalcAttackerReviveTime::Install();
	TgGame__CalcAwardMedal::Install();
	TgGame__CalcDefenderReviveTime::Install();
	TgGame__DbSaveReward::Install();
	TgGame__DbUpdateQuests::Install();
	TgGame__FinishEndMission::Install();
	TgGame__GetDifficultyModifier::Install();
	TgGame__Loot::Install();
	TgGame__NotifyPostCommitMapChange::Install();
	TgGame__SpawnAllHenchman::Install();
	TgGame__SpawnLandMarks::Install();
	TgGame__SpawnTemplatePlayer::Install();
	TgGame__TgFindPlayerSpawnLocation::Install();
	TgGame__UpdateMissionTimerEventWinVar::Install();
	TgInventoryManager__ApplyAllEnhancementEffects::Install();
	TgInventoryManager__GetDeviceByInstanceId::Install();
	TgInventoryManager__InventoryCleanup::Install();
	TgInventoryManager__SetInventoryDirty::Install();
	TgPawn_Character__ApplyItemEffects::Install();
	TgPawn_Character__CraftItem::Install();
	TgPawn_Character__DismissVanityPet::Install();
	TgPawn_Character__ReapplyCharacterSkillTree::Install();
	TgPawn_Character__ReapplyLoadoutEffects::Install();
	TgPawn_Character__RemoveCharacterSkillTree::Install();
	TgPawn_Character__SendCharacterSkillMarshal::Install();
	TgPawn_Character__ServerOnSetPlayerMesh::Install();
	TgPawn_Character__ServerPurchaseItem::Install();
	TgPawn_Character__ServerRemoveAllCharSkills::Install();
	TgPawn_Character__SetCurrentItemProfile::Install();
	TgPawn_Character__SpawnVanityPet::Install();
	TgPawn_Character__UpdateDurability::Install();
	TgPawn_Character__VanityPetDestroyed::Install();
	TgPawn__AddProperty::Install();
	TgPawn__AddDamageInfo::Install();
	TgPawn__ApplyBuff::Install();
	TgPawn__TrackBotHealing::Install();
	TgPawn__TrackCompleteKillInfo::Install();
	TgPawn__TrackDamagedBot::Install();
	TgPawn__TrackDamagedPlayer::Install();
	TgPawn_Character__SendCombatMessage::Install();
	TgPawn__TrackHealing::Install();
	TgPawn__TrackHit::Install();
	TgPawn__TickMakeVisibleCalculation::Install();
	TgPlayerController__ServerApplyFlair::Install();
	TgPlayerController__ServerTestSystemMailItem::Install();
	TgRepInfo_Player__OnAllFlairManifestsLoaded::Install();
	TgRepInfo_Player__OnProfileChanged::Install();
	TgRepInfo_Player__UpdateScoreBoard::Install();
	TgMeshAssembly__ForceNetRelevant::Install();
	TgMissionObjective__RegisterSelf::Install();
	TgDynamicSMActor__ForceNetRelevant::Install();
	TgPawn__ApplyDye::Install();
	TgPawn__ApplyJetpackTrail::Install();
	TgPawn__BeginStats::Install();
	TgPawn__CanMove::Install();
	TgPawn__CheckKillQuestCredit::Install();
	TgPawn__CheckUseQuestCredit::Install();
	TgPawn__EndStats::Install();
	TgPawn__GetDyeItemId::Install();
	TgPawn__GetJetpackTrailId::Install();
	TgPawn__GetMoraleDevice::Install();
	TgPawn__GiveKillXp::Install();
	TgPawn__InitSpawnPets::Install();
	TgPawn__KillDeployables::Install();
	TgPawn__MakeInvulnerable::Install();
	TgPawn__OnPetSpawned::Install();
	TgPawn__OnProjectileExploded::Install();
	TgPawn__ReapplyCharacterSkillTree::Install();
	TgPawn__ReapplyLoadoutEffects::Install();
	TgPawn__RemoveTrackFired::Install();
	TgPawn__ServerOnEquipCharDevice::Install();
	TgPawn__ServerOnEquipCharDevices::Install();
	TgPawn__ServerOnRequestMission::Install();
	TgPawn__ServerOnSetPlayerLevel::Install();
	TgPawn__ServerOnSetPlayerMesh::Install();
	TgPawn__SetDeploySensorDetectedStealthLightup::Install();
	TgPawn__SetDyeItemId::Install();
	TgPawn__SetJetpackTrailId::Install();
	TgPawn__SpawnLoot::Install();
	TgPawn__StatsCleanup::Install();
	TgPawn__TrackBoost::Install();
	TgPawn__TrackBuff::Install();
	TgPawn__TrackDamageTaken::Install();
	TgPawn__TrackDeath::Install();
	TgPawn__TrackDefense::Install();
	TgPawn__TrackEscortObjective::Install();
	TgPawn__TrackFired::Install();
	TgPawn__TrackFromPlayerDeath::Install();
	TgPawn__TrackKill::Install();
	TgPawn__TrackKilledBot::Install();
	TgPawn__TrackKilledPlayer::Install();
	TgPawn__TrackMyBeaconUsed::Install();
	TgPawn__TrackObjective::Install();
	TgPawn__TrackObjectivePoints::Install();
	TgPawn__TrackObjectivePointsByProgress::Install();
	TgPawn__TrackReleaseTime::Install();
	TgPawn__TrackSelfDamage::Install();
	TgPawn__TrackSelfKill::Install();
	TgPawn__TrackTeamDamage::Install();
	TgPawn__TrackTeamKill::Install();
	TgPawn__UpdateBuffer::Install();
	TgPawn__UpdateControllerVisBasedProperties::Install();
	TgPawn__UpdateDamagers::Install();
	TgPawn__UpdateDebuffer::Install();
	TgPawn__UpdateHUDScores::Install();
	TgPawn__UpdateHealer::Install();
	TgPawn__UpdatePRIAssetRefs::Install();
	TgPawn__ValidateStatsTracker::Install();
	TgPlayerController__DebugFn::Install();
	TgPlayerController__FinalSave::Install();
	TgPlayerController__ServerAbandonAssignment::Install();
	TgPlayerController__ServerActivateInvItem::Install();
	TgPlayerController__ServerAddHZPoints::Install();
	TgPlayerController__ServerAddToken::Install();
	TgPlayerController__ServerChangeCoalition::Install();
	TgPlayerController__ServerChangeTaskForce::Install();
	TgPlayerController__ServerClearProfiles::Install();
	TgPlayerController__ServerClearSkillsAndDevices::Install();
	TgPlayerController__ServerCombineItems::Install();
	TgPlayerController__ServerCraftItem::Install();
	TgPlayerController__ServerDestroyInvItem::Install();
	TgPlayerController__ServerDevGiveXP::Install();
	TgPlayerController__ServerGMGiven::Install();
	TgPlayerController__ServerLoadItemProfile::Install();
	TgPlayerController__ServerLogSpeedHack::Install();
	TgPlayerController__ServerMarkSpawnReturn::Install();
	TgPlayerController__ServerObama::Install();
	TgPlayerController__ServerRepairAllUpgrades::Install();
	TgPlayerController__ServerRepairInvItem::Install();
	TgPlayerController__ServerRequestAssignment::Install();
	TgPlayerController__ServerRequestBeaconNetworkHop::Install();
	TgPlayerController__ServerSalvageInvItem::Install();
	TgPlayerController__ServerSetLevel::Install();
	TgPlayerController__ServerSetPawnAlwaysRelevant::Install();
	TgPlayerController__ServerSetSpawnAtMe::Install();
	TgPlayerController__ServerSetTaskForceLead::Install();
	TgPlayerController__SetHomeMapGame::Install();
	TgPlayerController__ShouldAutoKick::Install();

	// data collection
	CGameClient__MarshalReceived::Install();
	CMarshal__GetByte::Install();
	CMarshal__GetInt32t::Install();
	CMarshal__GetString2::Install();
	CMarshal__GetName::Install();
	CMarshal__GetWcharT::Install();
	CMarshal__GetFloat::Install();
	CMarshal__GetFlag::Install();
	CMarshal__GetGuid::Install();
	CMarshal__Translate::Install();
	CMarshal__GetArray::Install();
	CAmBot__LoadBotMarshal::bPopulateDatabaseBots = false;
	CAmBot__LoadBotMarshal::bPopulateDatabaseBotDevices = false;
	CAmBot__LoadBotMarshal::Install();
	CAmBot__LoadBotBehaviorMarshal::Install();
	CAmBot__LoadBotSpawnTableMarshal::bPopulateDatabase = false;
	CAmBot__LoadBotSpawnTableMarshal::Install();
	CAmDeviceModel__LoadDeviceMarshal::bPopulateDatabaseDevices = false;
	CAmDeviceModel__LoadDeviceMarshal::Install();
	CAmDeviceModel__LoadDeviceModeMarshal::bPopulateDatabaseDeviceModes = false;
	CAmDeviceModel__LoadDeviceModeMarshal::Install();
	CAmEffectModel__LoadEffectGroupMarshal::bPopulateDatabaseEffectGroups = false;
	CAmEffectModel__LoadEffectGroupMarshal::Install();
	CAmEffectModel__LoadEffectMarshal::bPopulateDatabaseEffects = false;
	CAmEffectModel__LoadEffectMarshal::Install();
	CAmItem__LoadItemMarshal::bPopulateDatabaseItems = false;
	CAmItem__LoadItemMarshal::Install();
	CAmOmegaVolume__LoadOmegaVolumeMarshal::Install();

	// Unified asm_* data-set capture via CMarshal__GetArray dispatch.
	// Flip to true for a single game run to populate, then back to false.
	AsmDataCapture::bPopulateDatabase = true;

	::DetourTransactionCommit();

	IpcClient::Init(Config::GetIpcHost(), Config::GetIpcPort(), Config::GetInstanceId());

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ModuleThread), 0, 0, 0 );
			
			// DebugWindow::WindowTitle = "SERVER";
			// CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(DebugWindow::ModuleThread), 0, 0, 0 );
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

