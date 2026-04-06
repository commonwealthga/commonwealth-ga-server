
SOURCE_FILES= \
			  $(SRC_DIR)/Config/Config.cpp \
			  \
			  $(SRC_DIR)/Utils/Logger/Logger/FileLogger.cpp \
			  $(SRC_DIR)/Utils/CommandLineParser/CommandLineParser.cpp \
			  $(SRC_DIR)/Utils/DebugWindow/DebugWindow.cpp \
			  \
			  $(SRC_DIR)/Database/Database.cpp \
			  $(SRC_DIR)/GameServer/Storage/PawnSessions/PawnSessions.cpp \
			  \
			  $(SRC_DIR)/IpcClient/IpcClient.cpp \
			  \
			  $(SRC_DIR)/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.cpp \
			  $(SRC_DIR)/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.cpp \
			  $(SRC_DIR)/GameServer/Utils/ClassPreloader/ClassPreloader.cpp \
			  $(SRC_DIR)/GameServer/Engine/GameEngine/Init/GameEngine__Init.cpp \
			  $(SRC_DIR)/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.cpp \
			  $(SRC_DIR)/GameServer/Engine/World/BeginPlay/World__BeginPlay.cpp \
			  $(SRC_DIR)/GameServer/Engine/SeqActNullGuard/SeqActNullGuard.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/Spawn/Actor__Spawn.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/Tick/Actor__Tick.cpp \
			  $(SRC_DIR)/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.cpp \
			  $(SRC_DIR)/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.cpp \
			  $(SRC_DIR)/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.cpp \
			  $(SRC_DIR)/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.cpp \
			  $(SRC_DIR)/GameServer/Storage/TeamsData/TeamsData.cpp \
			  $(SRC_DIR)/GameServer/Storage/PlayerRegistry/PlayerRegistry.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp \
			  $(SRC_DIR)/GameServer/Engine/ActorChannel/ReceivedBunch/CanExecute/ActorChannel__ReceivedBunch__CanExecute.cpp \
			  $(SRC_DIR)/GameServer/Engine/Channel/ReceivedSequencedBunch/Channel__ReceivedSequencedBunch.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/IsReadyForStart/TgPlayerController__IsReadyForStart.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/SetSoundMode/TgPlayerController__SetSoundMode.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/CanPlayerUseVolume/TgPlayerController__CanPlayerUseVolume.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/GetViewTarget/TgPlayerController__GetViewTarget.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerAcceptNewProfileFromEquipScreen/TgPlayerController__ServerAcceptNewProfileFromEquipScreen.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter__GiveJetpack.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter__GiveAgonizer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnBotPawn/TgGame__SpawnBotPawn.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SendMissionTimerEvent/TgGame__SendMissionTimerEvent.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Arena/FinalizeRoundScore/TgGame_Arena__FinalizeRoundScore.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Arena/FinalizeGameScore/TgGame_Arena__FinalizeGameScore.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_PointRotation/CalcNextObjective/TgGame_PointRotation__CalcNextObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_PointRotation/UnlockObjective/TgGame_PointRotation__UnlockObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/CheckRandomObjectives/TgGame__CheckRandomObjectives.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/UnlockObjective/TgGame__UnlockObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/LockoutObjectives/TgGame__LockoutObjectives.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/IsFinalObjective/TgGame__IsFinalObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SetObjectivesInactive/TgGame__SetObjectivesInactive.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SetObjectivesOvertimeNotify/TgGame__SetObjectivesOvertimeNotify.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/GetFinalObjectivesList/TgGame__GetFinalObjectivesList.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgMissionObjective/SetObjectiveActive/TgMissionObjective__SetObjectiveActive.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SwapAttachedDeviceMaterials/TgPawn__SwapAttachedDeviceMaterials.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.cpp \
			  $(SRC_DIR)/GameServer/Inventory/Inventory.cpp \
			  $(SRC_DIR)/GameServer/Constants/DeviceIds.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepListV2.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnBot/TgGame__SpawnBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDeviceFire/CustomFire/TgDeviceFire__CustomFire.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDeviceFire/Deploy/TgDeviceFire__Deploy.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/HasEnoughPowerPool/TgDevice__HasEnoughPowerPool.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/HasMinimumPowerPool/TgDevice__HasMinimumPowerPool.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/ApplyDamage/TgEffectManager__ApplyDamage.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.cpp \
			  $(SRC_DIR)/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.cpp \
			  $(SRC_DIR)/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetGuid/CMarshal__GetGuid.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmBot/LoadBotSpawnTableMarshal/CAmBot__LoadBotSpawnTableMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmDeviceModel/LoadDeviceMarshal/CAmDeviceModel__LoadDeviceMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmDeviceModel/LoadDeviceModeMarshal/CAmDeviceModel__LoadDeviceModeMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmEffectModel/LoadEffectGroupMarshal/CAmEffectModel__LoadEffectGroupMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmEffectModel/LoadEffectMarshal/CAmEffectModel__LoadEffectMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.cpp \
			  \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/ClearQueue/TgBotFactory__ClearQueue.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnBotAdjusted/TgBotFactory__SpawnBotAdjusted.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnBotId/TgBotFactory__SpawnBotId.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/UseSpawnTable/TgBotFactory__UseSpawnTable.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDeployable/AddProperty/TgDeployable__AddProperty.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDeployable/InitializeDefaultProps/TgDeployable__InitializeDefaultProps.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/ApplyInventoryEquipEffects/TgDevice__ApplyInventoryEquipEffects.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/ClearInstigatorEquippedDevices/TgDevice__ClearInstigatorEquippedDevices.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/PopulateInstigatorEquippedDevices/TgDevice__PopulateInstigatorEquippedDevices.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/GetSkillBasedEffectGroup/TgEffectManager__GetSkillBasedEffectGroup.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/IsStrongest/TgEffectManager__IsStrongest.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Arena/AdjustBeaconForwardSpawn/TgGame_Arena__AdjustBeaconForwardSpawn.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Control/CalcAttackerReviveTime/TgGame_Control__CalcAttackerReviveTime.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Control/CalcDefenderReviveTime/TgGame_Control__CalcDefenderReviveTime.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Control/SendCountdownRemainingMessages/TgGame_Control__SendCountdownRemainingMessages.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Control/TickCountdownCalculation/TgGame_Control__TickCountdownCalculation.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/CacheKismetConfiguration/TgGame_Defense__CacheKismetConfiguration.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/CheckWinGame/TgGame_Defense__CheckWinGame.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/CheckWinRound/TgGame_Defense__CheckWinRound.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/FinalizeRoundScore/TgGame_Defense__FinalizeRoundScore.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/LoadGameConfig/TgGame_Defense__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/LockoutObjectives/TgGame_Defense__LockoutObjectives.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/SendNewRoundMessage/TgGame_Defense__SendNewRoundMessage.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/TickWaveNodes/TgGame_Defense__TickWaveNodes.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Defense/UnlockObjective/TgGame_Defense__UnlockObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Ticket/AwardTickets/TgGame_Ticket__AwardTickets.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Ticket/BeginEndMission/TgGame_Ticket__BeginEndMission.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Ticket/LoadGameConfig/TgGame_Ticket__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Ticket/TickTicketsCalculation/TgGame_Ticket__TickTicketsCalculation.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/CalcAwardMedal/TgGame__CalcAwardMedal.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/DbSaveReward/TgGame__DbSaveReward.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/DbUpdateQuests/TgGame__DbUpdateQuests.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/FinishEndMission/TgGame__FinishEndMission.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/GetDifficultyModifier/TgGame__GetDifficultyModifier.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/Loot/TgGame__Loot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/NotifyPostCommitMapChange/TgGame__NotifyPostCommitMapChange.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnAllHenchman/TgGame__SpawnAllHenchman.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnLandMarks/TgGame__SpawnLandMarks.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnTemplatePlayer/TgGame__SpawnTemplatePlayer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/TgFindPlayerSpawnLocation/TgGame__TgFindPlayerSpawnLocation.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/UpdateMissionTimerEventWinVar/TgGame__UpdateMissionTimerEventWinVar.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/ApplyAllEnhancementEffects/TgInventoryManager__ApplyAllEnhancementEffects.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/GetDeviceByInstanceId/TgInventoryManager__GetDeviceByInstanceId.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/InventoryCleanup/TgInventoryManager__InventoryCleanup.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/SetInventoryDirty/TgInventoryManager__SetInventoryDirty.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/ApplyItemEffects/TgPawn_Character__ApplyItemEffects.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/CraftItem/TgPawn_Character__CraftItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/DismissVanityPet/TgPawn_Character__DismissVanityPet.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/ReapplyLoadoutEffects/TgPawn_Character__ReapplyLoadoutEffects.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/RemoveCharacterSkillTree/TgPawn_Character__RemoveCharacterSkillTree.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/SendCharacterSkillMarshal/TgPawn_Character__SendCharacterSkillMarshal.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/ServerOnSetPlayerMesh/TgPawn_Character__ServerOnSetPlayerMesh.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/ServerPurchaseItem/TgPawn_Character__ServerPurchaseItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/ServerRemoveAllCharSkills/TgPawn_Character__ServerRemoveAllCharSkills.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/SetCurrentItemProfile/TgPawn_Character__SetCurrentItemProfile.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/SpawnVanityPet/TgPawn_Character__SpawnVanityPet.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/UpdateDurability/TgPawn_Character__UpdateDurability.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn_Character/VanityPetDestroyed/TgPawn_Character__VanityPetDestroyed.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/AddProperty/TgPawn__AddProperty.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackBotHealing/TgPawn__TrackBotHealing.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackCompleteKillInfo/TgPawn__TrackCompleteKillInfo.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackDamagedBot/TgPawn__TrackDamagedBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackDamagedPlayer/TgPawn__TrackDamagedPlayer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackHealing/TgPawn__TrackHealing.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackHit/TgPawn__TrackHit.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerApplyFlair/TgPlayerController__ServerApplyFlair.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerTestSystemMailItem/TgPlayerController__ServerTestSystemMailItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgRepInfo_Player/OnAllFlairManifestsLoaded/TgRepInfo_Player__OnAllFlairManifestsLoaded.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgRepInfo_Player/OnProfileChanged/TgRepInfo_Player__OnProfileChanged.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgRepInfo_Player/UpdateScoreBoard/TgRepInfo_Player__UpdateScoreBoard.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgMeshAssembly/ForceNetRelevant/TgMeshAssembly__ForceNetRelevant.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDynamicSMActor/ForceNetRelevant/TgDynamicSMActor__ForceNetRelevant.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ApplyDye/TgPawn__ApplyDye.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ApplyJetpackTrail/TgPawn__ApplyJetpackTrail.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/BeginStats/TgPawn__BeginStats.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/CanMove/TgPawn__CanMove.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/CheckKillQuestCredit/TgPawn__CheckKillQuestCredit.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/CheckUseQuestCredit/TgPawn__CheckUseQuestCredit.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/EndStats/TgPawn__EndStats.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GetDyeItemId/TgPawn__GetDyeItemId.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GetJetpackTrailId/TgPawn__GetJetpackTrailId.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GetMoraleDevice/TgPawn__GetMoraleDevice.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GiveKillXp/TgPawn__GiveKillXp.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/InitSpawnPets/TgPawn__InitSpawnPets.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/MakeInvulnerable/TgPawn__MakeInvulnerable.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/OnPetSpawned/TgPawn__OnPetSpawned.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/OnProjectileExploded/TgPawn__OnProjectileExploded.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ReapplyCharacterSkillTree/TgPawn__ReapplyCharacterSkillTree.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ReapplyLoadoutEffects/TgPawn__ReapplyLoadoutEffects.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/RemoveTrackFired/TgPawn__RemoveTrackFired.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ServerOnEquipCharDevice/TgPawn__ServerOnEquipCharDevice.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ServerOnEquipCharDevices/TgPawn__ServerOnEquipCharDevices.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ServerOnRequestMission/TgPawn__ServerOnRequestMission.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ServerOnSetPlayerLevel/TgPawn__ServerOnSetPlayerLevel.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ServerOnSetPlayerMesh/TgPawn__ServerOnSetPlayerMesh.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SetDeploySensorDetectedStealthLightup/TgPawn__SetDeploySensorDetectedStealthLightup.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SetDyeItemId/TgPawn__SetDyeItemId.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SetJetpackTrailId/TgPawn__SetJetpackTrailId.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SpawnLoot/TgPawn__SpawnLoot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/StatsCleanup/TgPawn__StatsCleanup.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackBoost/TgPawn__TrackBoost.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackBuff/TgPawn__TrackBuff.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackDeath/TgPawn__TrackDeath.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackDefense/TgPawn__TrackDefense.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackEscortObjective/TgPawn__TrackEscortObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackFired/TgPawn__TrackFired.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackFromPlayerDeath/TgPawn__TrackFromPlayerDeath.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackKilledBot/TgPawn__TrackKilledBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackKilledPlayer/TgPawn__TrackKilledPlayer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackMyBeaconUsed/TgPawn__TrackMyBeaconUsed.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackObjective/TgPawn__TrackObjective.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackObjectivePoints/TgPawn__TrackObjectivePoints.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackObjectivePointsByProgress/TgPawn__TrackObjectivePointsByProgress.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackReleaseTime/TgPawn__TrackReleaseTime.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackSelfDamage/TgPawn__TrackSelfDamage.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackSelfKill/TgPawn__TrackSelfKill.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackTeamDamage/TgPawn__TrackTeamDamage.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/TrackTeamKill/TgPawn__TrackTeamKill.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdateBuffer/TgPawn__UpdateBuffer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdateControllerVisBasedProperties/TgPawn__UpdateControllerVisBasedProperties.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdateDamagers/TgPawn__UpdateDamagers.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdateDebuffer/TgPawn__UpdateDebuffer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdateHUDScores/TgPawn__UpdateHUDScores.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdateHealer/TgPawn__UpdateHealer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/UpdatePRIAssetRefs/TgPawn__UpdatePRIAssetRefs.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/ValidateStatsTracker/TgPawn__ValidateStatsTracker.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/DebugFn/TgPlayerController__DebugFn.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/FinalSave/TgPlayerController__FinalSave.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerAbandonAssignment/TgPlayerController__ServerAbandonAssignment.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerActivateInvItem/TgPlayerController__ServerActivateInvItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerAddHZPoints/TgPlayerController__ServerAddHZPoints.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerAddToken/TgPlayerController__ServerAddToken.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerChangeCoalition/TgPlayerController__ServerChangeCoalition.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerChangeTaskForce/TgPlayerController__ServerChangeTaskForce.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerClearProfiles/TgPlayerController__ServerClearProfiles.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerClearSkillsAndDevices/TgPlayerController__ServerClearSkillsAndDevices.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerCombineItems/TgPlayerController__ServerCombineItems.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerCraftItem/TgPlayerController__ServerCraftItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerDestroyInvItem/TgPlayerController__ServerDestroyInvItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerDevGiveXP/TgPlayerController__ServerDevGiveXP.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerGMGiven/TgPlayerController__ServerGMGiven.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerLoadItemProfile/TgPlayerController__ServerLoadItemProfile.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerLogSpeedHack/TgPlayerController__ServerLogSpeedHack.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerMarkSpawnReturn/TgPlayerController__ServerMarkSpawnReturn.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerObama/TgPlayerController__ServerObama.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerRepairAllUpgrades/TgPlayerController__ServerRepairAllUpgrades.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerRepairInvItem/TgPlayerController__ServerRepairInvItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerRequestAssignment/TgPlayerController__ServerRequestAssignment.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerRequestBeaconNetworkHop/TgPlayerController__ServerRequestBeaconNetworkHop.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerSalvageInvItem/TgPlayerController__ServerSalvageInvItem.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerSetLevel/TgPlayerController__ServerSetLevel.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerSetPawnAlwaysRelevant/TgPlayerController__ServerSetPawnAlwaysRelevant.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerSetSpawnAtMe/TgPlayerController__ServerSetSpawnAtMe.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ServerSetTaskForceLead/TgPlayerController__ServerSetTaskForceLead.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/SetHomeMapGame/TgPlayerController__SetHomeMapGame.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/ShouldAutoKick/TgPlayerController__ShouldAutoKick.cpp \
			  $(SRC_DIR)/dllmain.cpp

SOURCE_FILES_CLIENT= \
			  $(SRC_DIR)/Utils/Logger/Logger/FileLogger.cpp \
			  $(SRC_DIR)/Utils/DebugWindow/DebugWindow.cpp \
			  $(SRC_DIR)/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.cpp \
			  $(SRC_DIR)/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceivedClient.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetIntEnum/CMarshal__GetIntEnum.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgHUD_Game/NativePostBeginPlay/TgHUD_Game__NativePostBeginPlay.cpp \
			  $(SRC_DIR)/dllmainclient.cpp


# JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
# MAKEFLAGS += -j$(JOBS)
MAKEFLAGS += -j4
CC=i686-w64-mingw32-g++
CFLAGS=-std=c++17 -pthread -I. -I./lib/detours -I./lib/asio-1.34.2/include -I./lib/sqlite3 -L/usr/i686-w64-mingw32/lib -shared -static -static-libgcc -static-libstdc++ -fpermissive -s -w
LDFLAGS=-lkernel32 -luser32 -ladvapi32 -lws2_32 -lpsapi -lstdc++ -lgdi32 -Wl,--allow-multiple-definition
SRC_DIR=src
LIB_DIR=lib
DATA_DIR=data
OBJ_DIR=obj
OUT_DIR=out
OUT_CLIENT_DIR=out/client

# Ensure folders exist
$(shell mkdir -p $(OBJ_DIR) $(OUT_DIR) $(OUT_CLIENT_DIR))

SDK_CPP_SRC=$(SRC_DIR)/SDK/SdkHeaders.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/Core_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/Engine_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/GameFramework_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/UnrealScriptTest_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/TgGame_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/TgNetDrv_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/TgClient_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/XAudio2_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/ALAudio_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/WinDrv_functions.cpp

VERSION_CPP_SRC=$(LIB_DIR)/detours/modules.cpp \
				$(LIB_DIR)/detours/disasm.cpp \
				$(LIB_DIR)/detours/detours.cpp \
				$(SDK_CPP_SRC) \
				$(SOURCE_FILES)

VERSION_CPP_CLIENT_SRC=$(LIB_DIR)/detours/modules.cpp \
				$(LIB_DIR)/detours/disasm.cpp \
				$(LIB_DIR)/detours/detours.cpp \
				$(SDK_CPP_SRC) \
				$(SOURCE_FILES_CLIENT)

# Object file mapping
# VERSION_OBJS=$(VERSION_CPP_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

VERSION_CPP_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(VERSION_CPP_SRC))
VERSION_OBJS := $(VERSION_CPP_OBJS) $(OBJ_DIR)/lib/sqlite3/sqlite3.o
VERSION_CLIENT_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/client/%.o,$(VERSION_CPP_CLIENT_SRC))

$(info VERSION_CPP_SRC = $(VERSION_CPP_SRC))
$(info VERSION_OBJS    = $(VERSION_OBJS))

VERSION_DEF=$(DATA_DIR)/version.def
VERSION_OUT=$(OUT_DIR)/version.dll
VERSION_CLIENT_OUT=$(OUT_CLIENT_DIR)/version.dll

obj/pch.hpp.gch: src/pch.hpp
	i686-w64-mingw32-g++ -std=c++17 -I. -I./lib/detours -I./lib/asio-1.34.2/include -x c++-header src/pch.hpp -o obj/pch.hpp.gch


# Compile any src .cpp -> obj/src/... .o (with PCH)
$(OBJ_DIR)/src/%.o: src/%.cpp obj/pch.hpp.gch
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client/src/%.o: src/%.cpp obj/pch.hpp.gch
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile any lib .cpp -> obj/lib/... .o (without PCH)
$(OBJ_DIR)/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/lib/sqlite3/sqlite3.o: lib/sqlite3/sqlite3.c
	@mkdir -p $(dir $@)
	i686-w64-mingw32-gcc -O2 -I./lib/sqlite3 -c $< -o $@

# Default target
all: $(VERSION_OUT) $(VERSION_CLIENT_OUT)

# Build version.dll
$(VERSION_OUT): $(VERSION_OBJS) $(VERSION_DEF)
	$(CC) $(CFLAGS) -o $@ $(VERSION_OBJS) $(VERSION_DEF) $(LDFLAGS)

$(VERSION_CLIENT_OUT): $(VERSION_CLIENT_OBJS) $(VERSION_DEF)
	$(CC) $(CFLAGS) -o $@ $(VERSION_CLIENT_OBJS) $(VERSION_DEF) $(LDFLAGS)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(OBJ_CLIENT_DIR) $(OUT_DIR) $(OUT_CLIENT_DIR)

cleanserver:
	rm -rf $(OBJ_DIR) $(OUT_DIR)/version.dll

cleanclient:
	rm -rf $(OBJ_CLIENT_DIR) $(OUT_CLIENT_DIR)

# ── Control Server ──────────────────────────────────────────────────────────
CS_SRC_DIR=src/ControlServer
CS_CPP_SOURCES= \
	$(CS_SRC_DIR)/main.cpp \
	$(CS_SRC_DIR)/Constants/DeviceIds.cpp \
	$(CS_SRC_DIR)/Config/ControlServerConfig.cpp \
	$(CS_SRC_DIR)/Database/Database.cpp \
	$(CS_SRC_DIR)/PlayerSessionStore/PlayerSessionStore.cpp \
	$(CS_SRC_DIR)/TcpSession/TcpSession.cpp \
	$(CS_SRC_DIR)/ChatSession/ChatSession.cpp \
	$(CS_SRC_DIR)/IpcServer/IpcServer.cpp \
	$(CS_SRC_DIR)/InstanceRegistry/InstanceRegistry.cpp \
	$(CS_SRC_DIR)/InstanceSpawner/InstanceSpawner.cpp \
	$(CS_SRC_DIR)/QuestStore/QuestStore.cpp

CS_SOURCES=$(CS_CPP_SOURCES)

CS_CXXFLAGS=-std=c++17 -pthread -I. -I./lib/asio-1.34.2/include -I./lib/sqlite3 -DASIO_STANDALONE -O2 -Wall
CS_CFLAGS_SQLITE=-O2 -I./lib/sqlite3
CS_LDFLAGS_LINUX=-lpthread -ldl
CS_LDFLAGS_WIN=-lws2_32 -static

CS_OUT_LINUX=out/control-server
CS_OUT_WIN=out/control-server.exe
CS_SQLITE_OBJ_LINUX=out/sqlite3_cs_linux.o
CS_SQLITE_OBJ_WIN=out/sqlite3_cs_win.o

$(CS_SQLITE_OBJ_LINUX): lib/sqlite3/sqlite3.c
	gcc $(CS_CFLAGS_SQLITE) -c lib/sqlite3/sqlite3.c -o $(CS_SQLITE_OBJ_LINUX)

$(CS_SQLITE_OBJ_WIN): lib/sqlite3/sqlite3.c
	x86_64-w64-mingw32-gcc $(CS_CFLAGS_SQLITE) -c lib/sqlite3/sqlite3.c -o $(CS_SQLITE_OBJ_WIN)

control-server-linux: $(CS_CPP_SOURCES) $(CS_SQLITE_OBJ_LINUX)
	g++ $(CS_CXXFLAGS) -o $(CS_OUT_LINUX) $(CS_CPP_SOURCES) $(CS_SQLITE_OBJ_LINUX) $(CS_LDFLAGS_LINUX)

control-server-win: $(CS_CPP_SOURCES) $(CS_SQLITE_OBJ_WIN)
	x86_64-w64-mingw32-g++ $(CS_CXXFLAGS) -o $(CS_OUT_WIN) $(CS_CPP_SOURCES) $(CS_SQLITE_OBJ_WIN) $(CS_LDFLAGS_WIN)

control-server: control-server-linux control-server-win

cleancs:
	rm -f $(CS_OUT_LINUX) $(CS_OUT_WIN) $(CS_SQLITE_OBJ_LINUX) $(CS_SQLITE_OBJ_WIN)

