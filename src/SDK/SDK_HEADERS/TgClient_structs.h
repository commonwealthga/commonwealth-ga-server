/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: TgClient_structs.h
# ========================================================================================= #
# Credits: uNrEaL, Tamimego, SystemFiles, R00T88, _silencer, the1domo, K@N@VEL
# Thanks: HOOAH07, lowHertz
# Forums: www.uc-forum.com, www.gamedeception.net
#############################################################################################
*/

#ifdef _MSC_VER
	#pragma pack ( push, 0x4 )
#endif

/*
# ========================================================================================= #
# Script Structs
# ========================================================================================= #
*/

// ScriptStruct TgClient.TgAchievementData.TgClientPlayerAchievementInfo
// 0x0014
struct FTgClientPlayerAchievementInfo
{
	unsigned long                                      m_bCompleted : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_nProgress;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FString                                     m_Date;                                           		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgAgencyData.sAgencyMemberInfo
// 0x004C
struct FsAgencyMemberInfo
{
	struct FString                                     sMemberName;                                      		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                nMemberRankId;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                nMemberRankLevel;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned char                                      eMemberStatus;                                    		// 0x0014 (0x0001) [0x0000000000000000]              
	int                                                nMemberLocationId;                                		// 0x0018 (0x0004) [0x0000000000000000]              
	struct FString                                     sMemberLocationString;                            		// 0x001C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                nMemberPlayerId;                                  		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                nMemberLevel;                                     		// 0x002C (0x0004) [0x0000000000000000]              
	int                                                nMemberProfileId;                                 		// 0x0030 (0x0004) [0x0000000000000000]              
	struct FString                                     sPublicComment;                                   		// 0x0034 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FString                                     sOfficerComment;                                  		// 0x0040 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgAgencyData.sAgencyRankData
// 0x0018
struct FsAgencyRankData
{
	int                                                nRankId;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nRankLevel;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nRankPermissions;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FString                                     sRankName;                                        		// 0x000C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgAllianceData.sAllianceMemberInfo
// 0x0024
struct FsAllianceMemberInfo
{
	struct FString                                     sMemberName;                                      		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FString                                     sLeaderName;                                      		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nMemberAgencyId;                                  		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                nMemberCount;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nTerritoryCount;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgDataInterface.CharacterInfoStruct
// 0x0038
struct FCharacterInfoStruct
{
	struct FString                                     sName;                                            		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nProfileId;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                nRoleId;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                nHeadAsmId;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                nSkinMatParamId;                                  		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                nEyeMatParamId;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nHairAsmId;                                       		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                nGenderTypeId;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                nMapGameId;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
	TArray< int >                                      nMorphSettings;                                   		// 0x002C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgHUD_Game.DamageDirectionInfo
// 0x0018
struct FDamageDirectionInfo
{
	float                                              FadeTime;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              FadeValue;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   MatConstant;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FVector                                     OriginalHitDirection;                             		// 0x000C (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgHUD_Game.DigitData
// 0x0010
struct FDigitData
{
	int                                                LeftPos;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                TopPos;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                Width;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                Height;                                           		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMainMenu.sMainMenuButton
// 0x0010
struct FsMainMenuButton
{
	class UUIButton*                                   mmbButton;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    mmbLabel;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    mmbKeyBind;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                mmbFunctionID;                                    		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMainMenu.sContextItemData
// 0x0010
struct FsContextItemData
{
	struct FString                                     cidString;                                        		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                cidFunctionID;                                    		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIReleaseMenus.sBasicReleaseDialog
// 0x001C
struct FsBasicReleaseDialog
{
	unsigned long                                      m_bActive : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIPanel*                                    m_Panel;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              m_TimeReleaseDataSet;                             		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              m_SecondsBeforeAutorelease;                       		// 0x000C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bCanReleaseNow : 1;                             		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIButton*                                   m_ReleaseNowButton;                               		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TimeLabel;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgStatsData.Scores
// 0x006C
struct FScores
{
	int                                                Score[ 0xC ];                                     		// 0x0000 (0x0030) [0x0000000000000000]              
	struct FString                                     sPlayerName;                                      		// 0x0030 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     sClassName;                                       		// 0x003C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     sAgencyName;                                      		// 0x0048 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nTaskForce;                                       		// 0x0054 (0x0004) [0x0000000000000000]              
	unsigned long                                      bHighlight : 1;                                   		// 0x0058 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bAlive : 1;                                       		// 0x0058 (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                eBonusType;                                       		// 0x005C (0x0004) [0x0000000000000000]              
	int                                                nTeamIndex;                                       		// 0x0060 (0x0004) [0x0000000000000000]              
	int                                                nLevel;                                           		// 0x0064 (0x0004) [0x0000000000000000]              
	int                                                nClassID;                                         		// 0x0068 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneDriver.sTgUIRotatorInit
// 0x0020
struct FsTgUIRotatorInit
{
	class AActor*                                      targetModel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     targetUISkelMesh;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIScreenObject*                             dragRegionObject;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	unsigned long                                      bEnableYaw : 1;                                   		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bEnablePitch : 1;                                 		// 0x000C (0x0004) [0x0000000000000000] [0x00000002] 
	class UUIButton*                                   yawLeft;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   yawRight;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   pitchUp;                                          		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   pitchDown;                                        		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneDriver.sTgUIRotator
// 0x002C
struct FsTgUIRotator
{
	struct FsTgUIRotatorInit                           rotInit;                                          		// 0x0000 (0x0020) [0x0000000000000000]              
	unsigned long                                      bValid : 1;                                       		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bActive : 1;                                      		// 0x0020 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bStaticActive : 1;                                		// 0x0020 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      bStaticNegative : 1;                              		// 0x0020 (0x0004) [0x0000000000000000] [0x00000008] 
	int                                                nlastMoveYaw;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                nlastMovePitch;                                   		// 0x0028 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneDriver.sPagingPanel
// 0x0018
struct FsPagingPanel
{
	class UUIPanel*                                    PagingPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PagingLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   LastButton;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   FirstButton;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   PrevButton;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   NextButton;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneDriver_Tooltip.sToolTipData
// 0x0030
struct FsToolTipData
{
	struct FString                                     sToolTipName;                                     		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FString                                     sToolTipValue;                                    		// 0x000C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                nToolTipType;                                     		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                nSortBy;                                          		// 0x001C (0x0004) [0x0000000000000000]              
	struct FLinearColor                                colorNameOverride;                                		// 0x0020 (0x0010) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneDriver_Tooltip.sTooltipMsg
// 0x000C
struct FsTooltipMsg
{
	int                                                m_nHeaderId;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_nBodyId;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bRequiresRestart : 1;                           		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUISceneDriver_Tooltip.sToolTip
// 0x002C
struct FsToolTip
{
	class UUIPrefabInstance*                           wToolTip;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    wToolTipPanel;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wToolTipHeader;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wToolTipDescription;                              		// 0x000C (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             wToolTipBaseStatSL;                               		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wToolTipDescription2;                             		// 0x0014 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             wToolTipRndStatModSL;                             		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    wToolTipBottom;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	TArray< struct FsToolTipData >                     sToolTipData;                                     		// 0x0020 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUISceneDriver_Tooltip.EffectValue
// 0x0008
struct FEffectValue
{
	float                                              val;                                              		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FPointer                                    pEffect;                                          		// 0x0004 (0x0004) [0x0000000000001000]              ( CPF_Native )
};

// ScriptStruct TgClient.TgUISceneDriver_Tooltip.sConfirmPopupStruct
// 0x0060
struct FsConfirmPopupStruct
{
	class UUIPrefabInstance*                           m_wConfirmPopup;                                  		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wConfirmPanel;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wConfirmText;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wItemPanel;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wItemSizePanel;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_SlotWidgets*                 m_wItem;                                          		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wItemLabel;                                     		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wAmountPanel;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wAmountSizePanel;                               		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_wLeftArrowImage;                                		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_wRightArrowImage;                               		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wAmountTypeLabel;                               		// 0x002C (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wAmountEditBox;                                 		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wAmountMaxLabel;                                		// 0x0034 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wTextPanel;                                     		// 0x0038 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wTextTypeLabel;                                 		// 0x003C (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wTextEditBox;                                   		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_wOptionPanel;                                   		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wConfirmYesButton;                              		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wConfirmYesLabel;                               		// 0x004C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wConfirmNoButton;                               		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wConfirmNoLabel;                                		// 0x0054 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wConfirmOkButton;                               		// 0x0058 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wConfirmOkLabel;                                		// 0x005C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneDriver_Tooltip.sConfirmDataStruct
// 0x001C
struct FsConfirmDataStruct
{
	int                                                m_nConfirmMode;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_nConfirmModeIndex;                              		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_nMaxItems;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                m_nNumItems;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                m_nHexPosition;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_pItem;                                          		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                m_nItemId;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyDirectory.AgencyDirectoryStruct
// 0x0038
struct FAgencyDirectoryStruct
{
	class UUIPrefabInstance*                           AgencyInstance;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   AgencyButton;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AgencyHighlight;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nAgencyId;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgencyName;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgencyMembers;                                    		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgencyRank;                                       		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AllianceName;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AllianceMembers;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    NonSubAllowed;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
	struct FString                                     AgencyMessage;                                    		// 0x0028 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bRecruiting : 1;                                  		// 0x0034 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bSubOnly : 1;                                     		// 0x0034 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct TgClient.TgUIAgencyMenu_Alliance.AllianceMemberDisplayStruct
// 0x0014
struct FAllianceMemberDisplayStruct
{
	class UUIPrefabInstance*                           AllianceMemberPanelInstance;                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   AllianceMemberButton;                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AllianceMemberName;                               		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AllianceMemberHighlight;                          		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                nMemberAgencyId;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Facilities.HexInfoStruct
// 0x0040
struct FHexInfoStruct
{
	class UUIPanel*                                    HexInfoPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    HeaderLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    BuildingLabel;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   BuildingButton;                                   		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    OutputValueLabel;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    HqPenaltyLabel;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ResourceBonusLabel;                               		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ResourceBonusTypeLabel;                           		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AdjacencyBonusLabel;                              		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AdjustedOutputLabel;                              		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    InefficiencyLabel;                                		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TotalOutputLabel;                                 		// 0x002C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   ChangeFacilityButton;                             		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   ChangeHQButton;                                   		// 0x0034 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   TransferButton;                                   		// 0x0038 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   AbandonButton;                                    		// 0x003C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Facilities.FacilityMemberStruct
// 0x0024
struct FFacilityMemberStruct
{
	class UUILabel*                                    memberName;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    memberCount;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    memberCost;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    memberHighlight;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   memberCancelButton;                               		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                nHpQueId;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                nItemId;                                          		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                nCount;                                           		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nCost;                                            		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Facilities.FacilityQueueStruct
// 0x0044
struct FFacilityQueueStruct
{
	class UUIPanel*                                    QueueInfoPanel;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    HeaderLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    CurrentTaskLabel;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_SlotWidgets*                 CurrentTaskItem;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    CurrentTaskProgressBar;                           		// 0x0010 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   CurrentTaskProgressBarMIC;                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    CurrentTaskProgressLabel;                         		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   CurrentTaskCancelButton;                          		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nActivenHpQueId;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                nActiveItemId;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                nActiveCount;                                     		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                nActiveCost;                                      		// 0x002C (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             PendingTaskScrollList;                            		// 0x0030 (0x0004) [0x0000000000000000]              
	TArray< struct FFacilityMemberStruct >             PendingTaskMembers;                               		// 0x0034 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   AddTaskButton;                                    		// 0x0040 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Facilities.DefensiveConfigStruct
// 0x0024
struct FDefensiveConfigStruct
{
	class UUIPanel*                                    DefenseInfoPanel;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   DefenseButtons[ 0x6 ];                            		// 0x0004 (0x0018) [0x0000000000000000]              
	class UUIButton*                                   ApplyButton;                                      		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nLocalDefensePosition;                            		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_General.sAgencyMemberDisplay
// 0x0024
struct FsAgencyMemberDisplay
{
	class UUIImage*                                    memberClass;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    memberName;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    memberRank;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    memberStatus;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    memberLevel;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   memberPanelButton;                                		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    memberHighlight;                                  		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIPrefabInstance*                           memberPanelInstance;                              		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nMemberPlayerId;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_General.sCommentPanel
// 0x000C
struct FsCommentPanel
{
	class UUIPanel*                                    panel;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   editButton;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    textLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_General.sMemberDetailPanel
// 0x0048
struct FsMemberDetailPanel
{
	class UUILabel*                                    nameLabel;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    detailPanel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    historyLabel;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rankValueLabel;                                   		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    joinedValueLabel;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wlValueLabel;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    lastOnlineValueLabel;                             		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   playerSearchHotlinkButton;                        		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   promoteMemberButton;                              		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   demoteMemberButton;                               		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   removeMemberButton;                               		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   teaminviteMemberButton;                           		// 0x002C (0x0004) [0x0000000000000000]              
	struct FsCommentPanel                              publicCommentPanel;                               		// 0x0030 (0x000C) [0x0000000000000000]              
	struct FsCommentPanel                              officerCommentPanel;                              		// 0x003C (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_General.sPopupTextEditPanel
// 0x0014
struct FsPopupTextEditPanel
{
	class UUIPanel*                                    popupPanel;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    popupHeaderLabel;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  popupEditBox;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   popupAcceptButton;                                		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   popupCancelButton;                                		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Inventory.sInventoryListItem
// 0x0028
struct FsInventoryListItem
{
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          pItem;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InvMain;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvCount;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvName;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvType;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvGen;                                         		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvQuality;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_InvHighlight;                                   		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                m_nItemCount;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Inventory.sAgencyInvPanel
// 0x0044
struct FsAgencyInvPanel
{
	class UUIPanel*                                    m_InventoryPanel;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditLabel;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_InventoryScrollFrame;                           		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ItemButton;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CreditButton;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeleteButton;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                m_nSortType;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bSortAscending : 1;                             		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIButton*                                   m_HeaderName;                                     		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderQuantity;                                 		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                m_nPageNum;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x002C (0x0018) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_InventoryLog.InventoryLogStruct
// 0x001C
struct FInventoryLogStruct
{
	class UUIPrefabInstance*                           LogInstance;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    DateLabel;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TimeLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PlayerLabel;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    EventLabel;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ItemLabel;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    QuantityLabel;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Management.sRankDisplay
// 0x0014
struct FsRankDisplay
{
	class UUILabel*                                    rankNameLabel;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   rankButton;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    rankHighlight;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPrefabInstance*                           rankPanelInstance;                                		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                rankIndex;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Management.sRankDetailPanel
// 0x0048
struct FsRankDetailPanel
{
	class UUIPanel*                                    detailPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   detailPromoteButton;                              		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   detailDemoteButton;                               		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   detailRemoveButton;                               		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    detailCoverPanel;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   detailCoverButton;                                		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  detailNameEditBox;                                		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailOfficerCommentEditCheckBox;                 		// 0x001C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailOfficerCommentViewCheckBox;                 		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailPublicCommentEditCheckBox;                  		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailMOTDEditCheckBox;                           		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailDescriptionEditCheckBox;                    		// 0x002C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailInvitePlayerCheckBox;                       		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailPromotePlayerCheckBox;                      		// 0x0034 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailDemotePlayerCheckBox;                       		// 0x0038 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailRemovePlayerCheckBox;                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailFacilityMgmtCheckBox;                       		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 detailRemoveInvCheckBox;                          		// 0x0044 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgencyMenu_Recruiting.AgentSubmissionStruct
// 0x004C
struct FAgentSubmissionStruct
{
	class UUIPrefabInstance*                           AgentInstance;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   AgentButton;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgentName;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgentClass;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgentDate;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgentLevel;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgentSkill;                                       		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AgentClassIcon;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AgentSkillStars;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AgentHighlight;                                   		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                AgentCharacterId;                                 		// 0x0028 (0x0004) [0x0000000000000000]              
	struct FString                                     AgentMessage;                                     		// 0x002C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     AgentNameString;                                  		// 0x0038 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FDouble                                     ApplicationDate;                                  		// 0x0044 (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Achievement.AchievementData
// 0x0010
struct FAchievementData
{
	struct FPointer                                    m_AchSetup;                                       		// 0x0000 (0x0004) [0x0000000000101000]              ( CPF_Native )
	struct FPointer                                    m_PlayerAch;                                      		// 0x0004 (0x0004) [0x0000000000101000]              ( CPF_Native )
	unsigned long                                      m_bValid : 1;                                     		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_nAchievementId;                                 		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Achievement.AchievementGroupData
// 0x0014
struct FAchievementGroupData
{
	int                                                m_nGroupId;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_Group;                                          		// 0x0004 (0x0004) [0x0000000000101000]              ( CPF_Native )
	TArray< struct FAchievementData >                  m_Achievements;                                   		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIAgentProfile_Achievement.AchievedBG
// 0x000C
struct FAchievedBG
{
	class UUIPanel*                                    m_BasePanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_Background;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SelectedPanel;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Achievement.AchievementItem
// 0x005C
struct FAchievementItem
{
	class UUIButton*                                   m_BaseButton;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BadgeButton;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BadgeIcon;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RewardPanel;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_RewardSlot;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RewardLabel;                                    		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NameLabel;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_DescrLabel;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ProgressButton;                                 		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ProgressBar;                                    		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AchievedDateLabel;                              		// 0x0028 (0x0004) [0x0000000000000000]              
	struct FAchievedBG                                 m_AchievedBG;                                     		// 0x002C (0x000C) [0x0000000000000000]              
	struct FAchievedBG                                 m_NotAchievedBG;                                  		// 0x0038 (0x000C) [0x0000000000000000]              
	struct FAchievementData                            m_AchievementData;                                		// 0x0044 (0x0010) [0x0000000000000000]              
	class UTgInventoryObject*                          m_RewardImposter;                                 		// 0x0054 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_ProgressBarMIC;                                 		// 0x0058 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Achievement.AchievementGroup
// 0x000C
struct FAchievementGroup
{
	class UUIButton*                                   m_CatButton;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CatLabel;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_nGroupId;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Equip.EquippedItemStruct
// 0x0010
struct FEquippedItemStruct
{
	class UUIPanel*                                    devicePanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              deviceWidget;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIProgressBar*                              deviceDurabilityBar;                              		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   deviceRepairButton;                               		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Equip.DeviceListItemStruct
// 0x0020(0x0030 - 0x0010)
struct FDeviceListItemStruct : FEquippedItemStruct
{
	class UUIButton*                                   deviceButton;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceNameLabel;                                  		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceSkillLabel;                                 		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceGenLabel;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceGenTitleLabel;                              		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceDurTitleLabel;                              		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Equip.sTutTypePanels
// 0x0010
struct FsTutTypePanels
{
	int                                                SlotIndex;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    StepPanels[ 0x3 ];                                		// 0x0004 (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Inventory.sSalvagedItem
// 0x0014
struct FsSalvagedItem
{
	class UTgUIInvWidget*                              wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          pItem;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InvMain;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvCount;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvName;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Inventory.sInvListItem
// 0x002C
struct FsInvListItem
{
	class UTgUIInvWidget*                              wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EquipIcon;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InvMain;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvCount;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvName;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvType;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvGen;                                         		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InvQuality;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HoverPanel;                                     		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SelectedPanel;                                  		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                m_nItemCount;                                     		// 0x0028 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.SSkill
// 0x0028
struct FSSkill
{
	class UUIPanel*                                    SkillPanel;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    ArrowAvailPanel;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    ArrowUnavailPanel;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ArrowReferenceImage;                              		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    HasChildAvailImage;                               		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    HasChildUnavailImage;                             		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PointsLabel;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   IconButton;                                       		// 0x001C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   IconMIC;                                          		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                nSkillIdReference;                                		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.SSkillTreeRow
// 0x00A4
struct FSSkillTreeRow
{
	class UUIPanel*                                    RowParentPanel;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FSSkill                                     Skills[ 0x4 ];                                    		// 0x0004 (0x00A0) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.SSkillTreePanel
// 0x0490
struct FSSkillTreePanel
{
	class UUILabel*                                    PtsAllocLabel;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FSSkillTreeRow                              SkillRows[ 0x7 ];                                 		// 0x0004 (0x047C) [0x0000000000000000]              
	class UUIPanel*                                    TreePanel;                                        		// 0x0480 (0x0004) [0x0000000000000000]              
	int                                                SkillGroupId;                                     		// 0x0484 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TreeNameLabel;                                    		// 0x0488 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    TreeBackground;                                   		// 0x048C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.sSkillGroupName
// 0x0008
struct FsSkillGroupName
{
	int                                                nSkillGroupId;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nSkillGroupNameMsgId;                             		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.SRankInfo
// 0x0024
struct FSRankInfo
{
	int                                                nRank;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nLevelRequired;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned long                                      bAutoTrained : 1;                                 		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FString                                     sDescription;                                     		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      EquipEffectGroupList;                             		// 0x0018 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.SSkillInfo
// 0x005C
struct FSSkillInfo
{
	int                                                nSkillGroupId;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nSkillId;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nCategory;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FString                                     sName;                                            		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     sDescription;                                     		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nNbrOfRanks;                                      		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                nRankCurrent;                                     		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                nRankOriginal;                                    		// 0x002C (0x0004) [0x0000000000000000]              
	int                                                nNbrOfATRanks;                                    		// 0x0030 (0x0004) [0x0000000000000000]              
	int                                                nLocX;                                            		// 0x0034 (0x0004) [0x0000000000000000]              
	int                                                nLocY;                                            		// 0x0038 (0x0004) [0x0000000000000000]              
	int                                                nIconId;                                          		// 0x003C (0x0004) [0x0000000000000000]              
	int                                                nPrereqSkillId;                                   		// 0x0040 (0x0004) [0x0000000000000000]              
	int                                                nPrereqSkillPoints;                               		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                nPrereqGroupPoints;                               		// 0x0048 (0x0004) [0x0000000000000000]              
	TArray< struct FSRankInfo >                        Ranks;                                            		// 0x004C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      bLockedPrereqSkill : 1;                           		// 0x0058 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bLockedLevel : 1;                                 		// 0x0058 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bLockedGroup : 1;                                 		// 0x0058 (0x0004) [0x0000000000000000] [0x00000004] 
};

// ScriptStruct TgClient.TgUIAgentProfile_Skill.SSkillTreeInfo
// 0x0024
struct FSSkillTreeInfo
{
	TArray< struct FSSkillInfo >                       SkillInfoData;                                    		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                nSkillGroupId;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	struct FString                                     sSkillGroupName;                                  		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nPointsSpent;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nPointsAT;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Stat.BadgeSetStruct
// 0x00F0
struct FBadgeSetStruct
{
	class UUIPanel*                                    BadgePanel;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icons[ 0x8 ];                                     		// 0x0004 (0x0020) [0x0000000000000000]              
	class UUIButton*                                   PrevButton;                                       		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   NextButton;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                TipNameMsgId[ 0x8 ];                              		// 0x002C (0x0020) [0x0000000000000000]              
	int                                                TipDescMsgId[ 0x8 ];                              		// 0x004C (0x0020) [0x0000000000000000]              
	int                                                NbrAchieved[ 0x8 ];                               		// 0x006C (0x0020) [0x0000000000000000]              
	struct FString                                     CompletedDate[ 0x8 ];                             		// 0x008C (0x0060) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                CurrPage;                                         		// 0x00EC (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Stat.FlairSetStruct
// 0x0058
struct FFlairSetStruct
{
	class UUIPanel*                                    FlairPanel;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              Icons[ 0xF ];                                     		// 0x0004 (0x003C) [0x0000000000000000]              
	class UUIButton*                                   PrevButton;                                       		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   NextButton;                                       		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                CurrPage;                                         		// 0x0048 (0x0004) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_HeadFlairs;                                     		// 0x004C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIAgentProfile_Stat.CraftingSkillStruct
// 0x001C
struct FCraftingSkillStruct
{
	class UUIButton*                                   ParentButton;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   ItemSlot;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    nameLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    SkillAmountLabel;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ProgressImage;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                nSkillId;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_CraftingProgMIC;                                		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Stat.sBoostInfo
// 0x000C
struct FsBoostInfo
{
	class UUILabel*                                    boostType;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    boostPercentage;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    boostTime;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Stat.sStatTab
// 0x000C
struct FsStatTab
{
	class UUIButton*                                   subTabButton;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    subTabLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    subTabPanel;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAgentProfile_Stat.SortInfoStruct
// 0x0008
struct FSortInfoStruct
{
	int                                                SortOrder;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                NbrAchieved;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAuctionGeneral.sAuctionItemStruct
// 0x004C
struct FsAuctionItemStruct
{
	class UUIPrefabInstance*                           wMemberPrefab;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   wMemberButton;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    wCheckMark;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wName;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wQuantity;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wTimeLeft;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wCurrentBid;                                      		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wBuyout;                                          		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wMultiUse;                                        		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    wHighlight;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wPrice;                                           		// 0x002C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    wSelectedPanel;                                   		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    wLineSeparator;                                   		// 0x0034 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          InvItem;                                          		// 0x0038 (0x0004) [0x0000000000000000]              
	int                                                nAuctionItemId;                                   		// 0x003C (0x0004) [0x0000000000000000]              
	int                                                nItemId;                                          		// 0x0040 (0x0004) [0x0000000000000000]              
	int                                                nStartPrice;                                      		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                nQualityValueId;                                  		// 0x0048 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAuctionGeneral.sAuctionBidTab
// 0x000C
struct FsAuctionBidTab
{
	class UUIButton*                                   TabButton;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TabLabel;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    TabPanel;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAuctionBrowseMenu.TypeCategoryTemplate
// 0x000C
struct FTypeCategoryTemplate
{
	class UUIButton*                                   m_BaseButton;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Label;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_CatTreeEntry;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIAuctionBrowseMenu.ItemType
// 0x0010
struct FItemType
{
	int                                                m_nTypeId;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     m_sTypeName;                                      		// 0x0004 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIAuctionBrowseMenu.TypeCategoryTreeNode
// 0x0020
struct FTypeCategoryTreeNode
{
	struct FItemType                                   m_Type;                                           		// 0x0000 (0x0010) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_Parent;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	TArray< int >                                      m_Children;                                       		// 0x0014 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIAuctionBrowseMenu.sAuctionItemFilter
// 0x0018
struct FsAuctionItemFilter
{
	int                                                nQuality;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nTypeCode;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nSubTypeCode;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FString                                     sName;                                            		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIAuctionSellMenu.sAuctionInvItem
// 0x0018
struct FsAuctionInvItem
{
	class UTgUIInvWidget*                              InvWidget;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Button;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    nameLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    UnitsLabel;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICampaignMenu_Combat.sCombatAlertDefender
// 0x002C
struct FsCombatAlertDefender
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    Highlight;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ZoneName;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AttackerName;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    FacilityName;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    HexPosition;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PlayerCount;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Time;                                             		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nZoneId;                                          		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                nHexPosition;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
	unsigned long                                      bAuction : 1;                                     		// 0x0028 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUICampaignMenu_Combat.sCombatAlertAttacker
// 0x002C
struct FsCombatAlertAttacker
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    Highlight;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ZoneName;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    DefenderName;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    FacilityName;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    HexPosition;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PlayerCount;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Status;                                           		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nZoneId;                                          		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                nHexPosition;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
	unsigned long                                      bAuction : 1;                                     		// 0x0028 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUICampaignMenu_CombatLog.CombatLogStruct
// 0x0028
struct FCombatLogStruct
{
	class UUIPrefabInstance*                           LogInstance;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Date;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Time;                                             		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Facility;                                         		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    HexNumber;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    OwnersAlliance;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    OwnersAgency;                                     		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AttackedByAlliance;                               		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AttackedByAgency;                                 		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Outcome;                                          		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICampaignMenu_Leaderboard.AgencyStatsStruct
// 0x0024
struct FAgencyStatsStruct
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Rank;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    AgencyName;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    NetWorth;                                         		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Territories;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ProductionCapacity;                               		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nAgencyId;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICargoMenu.sSFItem
// 0x0010
struct FsSFItem
{
	int                                                m_nInventoryId;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_nItemId;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_nInstanceCount;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                m_nTaskTeam;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUITeamMenu.sTabButtonStruct
// 0x0018
struct FsTabButtonStruct
{
	class UUIPanel*                                    m_TabPanel;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SearchTabButton;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DetailTabButton;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CargoTabButton;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ManageTabButton;                                		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MapTabButton;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUITeamMenu.sInvitePlayerStruct
// 0x0028
struct FsInvitePlayerStruct
{
	class UUIPrefabInstance*                           ipsInviteInstance;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   ipsInviteButton;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ipsInviteIcon;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ipsInviteName;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ipsInviteAgency;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ipsInviteAlliance;                                		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ipsInviteLevel;                                   		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ipsInviteSkill;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ipsInviteStatus;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ipsInviteHighlight;                               		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUITeamMenu.sTeamMemberStruct
// 0x0020
struct FsTeamMemberStruct
{
	class UUIButton*                                   tmsMemberButton;                                  		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    tmsMemberIcon;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    tmsMemberDisconnect;                              		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    tmsMemberName;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    tmsMemberLeader;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    tmsMemberHighlight;                               		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                tmsCharacterId;                                   		// 0x0018 (0x0004) [0x0000000000000000]              
	unsigned long                                      tmsMemberSelected : 1;                            		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUICargoMenu.sCargoItem
// 0x0018
struct FsCargoItem
{
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wCargoButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wAmountLabel;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wNameLabel;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wSizeLabel;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_wHighlight;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICargoMenu.sDropshipItem
// 0x0030
struct FsDropshipItem
{
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FsSFItem                                    sItem;                                            		// 0x0004 (0x0010) [0x0000000000000000]              
	int                                                nBuildingId;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderLabel;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wAddButton;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wDescLabel;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wPlayerLabel;                                   		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wCargoLabel;                                    		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICargoMenu.sSFSelectStruct
// 0x000C
struct FsSFSelectStruct
{
	class UUIButton*                                   m_wSFButton;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wSFLabel;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_wSFImage;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorBase.sCostDisplayItems
// 0x0010
struct FsCostDisplayItems
{
	int                                                StdPrice;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                OrigStdPrice;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                APPrice;                                          		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                OrigAPPrice;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorScene.sVendorItem
// 0x0074
struct FsVendorItem
{
	class UTgUIInvWidget*                              InvWidget;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Button;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    nameLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    InfoLabel;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    RequiredLabel;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    StdCostLabel;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    APCostLabel;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icon_AP;                                          		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icon_Credits;                                     		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icon_Tokens;                                      		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icon_USD;                                         		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icon_EUR;                                         		// 0x002C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Icon_GBP;                                         		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    PriceSeparator;                                   		// 0x0034 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    PromoImage;                                       		// 0x0038 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PromoLabel;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    OnSaleImage;                                      		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    OnSaleLabel;                                      		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    OnSalePctLabel;                                   		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   OnSale_Icon_AP;                                   		// 0x004C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   OnSale_Icon_Credits;                              		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   OnSale_Icon_Tokens;                               		// 0x0054 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   OnSale_Icon_USD;                                  		// 0x0058 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   OnSale_Icon_EUR;                                  		// 0x005C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   OnSale_Icon_GBP;                                  		// 0x0060 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   pStdSaleIcon;                                     		// 0x0064 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    Bling;                                            		// 0x0068 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x006C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0070 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorScene.sVendorInvItem
// 0x0018
struct FsVendorInvItem
{
	class UTgUIInvWidget*                              InvWidget;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Button;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    nameLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    UnitsLabel;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorScene.sVendorEnticement
// 0x0010
struct FsVendorEnticement
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_Item;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_InvObject;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemName;                                       		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorScene.TextPreviewPanels
// 0x0040
struct FTextPreviewPanels
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_APPanel;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_EliteAgentPanel;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_EliteNoEnticementPanel;                         		// 0x000C (0x0004) [0x0000000000000000]              
	struct FsVendorEnticement                          m_EliteEnticement;                                		// 0x0010 (0x0010) [0x0000000000000000]              
	class UUIPanel*                                    m_BoostPanel;                                     		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BoostNoEnticementPanel;                         		// 0x0024 (0x0004) [0x0000000000000000]              
	struct FsVendorEnticement                          m_BoostEnticement;                                		// 0x0028 (0x0010) [0x0000000000000000]              
	class UUIPanel*                                    m_BoostRemainingPanel;                            		// 0x0038 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BoostTime;                                      		// 0x003C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorScene.ItemPreview
// 0x008C
struct FItemPreview
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     m_PaperDollMeshWidget;                            		// 0x0004 (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActor_EquipScreen*            m_PaperDoll;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActor*                        m_SkelMeshActor;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UAnimTree*                                   m_PreviewAnimTree;                                		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DragButton;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ResetLabel;                                     		// 0x0018 (0x0004) [0x0000000000000000]              
	struct FTextPreviewPanels                          m_StorePanels;                                    		// 0x001C (0x0040) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_EquippedItemList;                               		// 0x005C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_PaperDollFx;                                    		// 0x0068 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FName >                             m_PaperDollFxDisplayGroupNames;                   		// 0x0074 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UTgSpecialFx* >                      m_ActiveSFX;                                      		// 0x0080 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUICharacterSelectScene.CharacterSelectEntry
// 0x0018
struct FCharacterSelectEntry
{
	class UUIButton*                                   cseButton;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    cseClass;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    cseLevel;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    cseLocation;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    cseIcon;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    cseHighlight;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICharacterSelectScene.CharacterDeleteBox
// 0x001C
struct FCharacterDeleteBox
{
	class UUIPanel*                                    cdcPanel;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    cdcIconImage;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    cdcClassLabel;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    cdcLevelLabel;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  cdcDeleteEdit;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   cdcOK;                                            		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   cdcCancel;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICharacterSelectScene.CharacterErrorBox
// 0x0008
struct FCharacterErrorBox
{
	class UUIPanel*                                    cebPanel;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   cebOK;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIChat.sChatChannelGroupLogic
// 0x0018
struct FsChatChannelGroupLogic
{
	int                                                m_FilterBits;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FString                                     m_TabName;                                        		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_TabNameMsgId;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIChat.sChatChannelGroupDisplay
// 0x000C
struct FsChatChannelGroupDisplay
{
	int                                                m_TabLogicIndex;                                  		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_Button;                                         		// 0x0004 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UUILabel*                                    m_Label;                                          		// 0x0008 (0x0004) [0x0000000000002000]              ( CPF_Transient )
};

// ScriptStruct TgClient.TgUIChat.sOuterPanels
// 0x0010
struct FsOuterPanels
{
	class UUIPanel*                                    m_MainOP;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ChannelGroupsOP;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ChatEntryOP;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_LabelsOP;                                       		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIChat.sChatScrollButtons
// 0x0014
struct FsChatScrollButtons
{
	class UUIButton*                                   m_UpButton;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DownButton;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EndButton;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_EndButtonGlow;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_ButtonGlowMIC;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIClassSelectScene.sClassSelectStruct
// 0x0018
struct FsClassSelectStruct
{
	class UUIPanel*                                    cssClassPanel;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   cssClassButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   cssMaleButton;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   cssFemaleButton;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    cssClassIcon;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    cssClassName;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIConfirmPopup.sConfirmPopup
// 0x0010
struct FsConfirmPopup
{
	struct FString                                     m_ConfirmString;                                  		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UFunction*                                   m_ConfirmCallback;                                		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICraftScene.sCraftItemStruct
// 0x0020
struct FsCraftItemStruct
{
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          pItem;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ItemMain;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemCount;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemName;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemGen;                                        		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ItemHighlight;                                  		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                m_nNumItems;                                      		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICraftScene.sCraftComponentStruct
// 0x0024
struct FsCraftComponentStruct
{
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_nItemId;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ComponentMain;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ComponentCheck;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ComponentName;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ComponentRequired;                              		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ComponentOwned;                                 		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                m_nNumItems;                                      		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                m_nNumRequired;                                   		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICraftScene.sBPModStruct
// 0x0010
struct FsBPModStruct
{
	float                                              m_BPModChance;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_BPModEGId;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BPModAmount;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BPModName;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICraftScene.sBPModCategoryStruct
// 0x0014
struct FsBPModCategoryStruct
{
	int                                                m_BPModCategoryId;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BPModCategoryHeader;                            		// 0x0004 (0x0004) [0x0000000000000000]              
	TArray< struct FsBPModStruct >                     m_BPModItemList;                                  		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUICraftScene.sBPHeaderStruct
// 0x0014
struct FsBPHeaderStruct
{
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          pItem;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HeaderMain;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderName;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderGen;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICreateCharA.sPresetHeadArchetype
// 0x0010
struct FsPresetHeadArchetype
{
	class UUIPrefabInstance*                           m_PresetHeadInstance;                             		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PresetHeadButton;                               		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PresetHeadLabel;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_PresetHeadHighlight;                            		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICreateCharA.sGenderButtonStruct
// 0x000C
struct FsGenderButtonStruct
{
	class UUIButton*                                   m_GenderButton;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_SelectImage;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_UnselectImage;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICustomHeadMenu.sCustomHeadNodeStruct
// 0x0030
struct FsCustomHeadNodeStruct
{
	struct FName                                       chnsPoseID;                                       		// 0x0000 (0x0008) [0x0000000000000000]              
	unsigned char                                      chnsGroupID;                                      		// 0x0008 (0x0001) [0x0000000000000000]              
	unsigned char                                      chnsClassID;                                      		// 0x0009 (0x0001) [0x0000000000000000]              
	float                                              chnsNodeWeight;                                   		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                chnsNodeCount;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	struct FString                                     chnsNodeName;                                     		// 0x0014 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                chnsNodeOrder;                                    		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIPrefabInstance*                           chnsNodePrefab;                                   		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   chnsNodeSlider;                                   		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    chnsNodeLabel;                                    		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUICustomHeadMenu.sCustomHeadGroupStruct
// 0x0010
struct FsCustomHeadGroupStruct
{
	int                                                chgsGroupID;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< struct FsCustomHeadNodeStruct >            chgsNodeList;                                     		// 0x0004 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUICustomHeadMenu.sCustomHeadPresetStruct
// 0x0028
struct FsCustomHeadPresetStruct
{
	unsigned long                                      chpsPresetLocked : 1;                             		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIButton*                                   chpsPresetLockButton;                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   chpsPresetDetails;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    chpsPresetSelect;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    chpsPresetUnselect;                               		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    chpsPresetHover;                                  		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   chpsPresetSlider;                                 		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                chpsPresetCount;                                  		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                chpsPresetIndex;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeMasterSlider*                    chpsMasterSlider;                                 		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIDeviceStats.sDeviceItemStruct
// 0x0008
struct FsDeviceItemStruct
{
	class UTgUIPrimaryHUD_SlotWidgets*                 disItem;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                disItemID;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIDeviceStats.DeviceFields
// 0x001C
struct FDeviceFields
{
	class UUILabel*                                    FieldLabels[ 0x7 ];                               		// 0x0000 (0x001C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIDevSpawnBot.sBotScrollButton
// 0x001C
struct FsBotScrollButton
{
	class UUIButton*                                   m_ScrollHigher;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ScrollLower;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentStartIdx;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                m_ActiveSelectPos;                                		// 0x000C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bScrollTriggered : 1;                           		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bScrollingActive : 1;                           		// 0x0010 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bScrollingHigher : 1;                           		// 0x0010 (0x0004) [0x0000000000000000] [0x00000004] 
	float                                              m_fTimeStarted;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfLastScroll;                              		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIEndMissionScreen.sMissionBasicStats
// 0x0018
struct FsMissionBasicStats
{
	class UUILabel*                                    m_XPHeader;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_XP;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditsHeader;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Credits;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TokensHeader;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Tokens;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIEndMissionScreen.sBasicRewardPanel
// 0x0040
struct FsBasicRewardPanel
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_EligibleCheck;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_IneligibleCross;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Header;                                         		// 0x000C (0x0004) [0x0000000000000000]              
	struct FsMissionBasicStats                         m_sEarned;                                        		// 0x0010 (0x0018) [0x0000000000000000]              
	struct FsMissionBasicStats                         m_sTotal;                                         		// 0x0028 (0x0018) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIEndMissionScreen.sAchievementPanel
// 0x0024
struct FsAchievementPanel
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NoneEarned;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_EarnedListOuter;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_EarnedList[ 0x6 ];                              		// 0x000C (0x0018) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIEndMissionScreen.sBonusRewardPanel
// 0x0050
struct FsBonusRewardPanel
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_IneligibleBlackout;                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Header;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_XP;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Credits;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Tokens;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ItemPanel;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_Item;                                           		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemName;                                       		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemReqLevel;                                   		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BuyNowPanel;                                    		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MissingOut;                                     		// 0x002C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BuyNowExplanation;                              		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BuyNowButton;                                   		// 0x0034 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BuyNowButtonLabel;                              		// 0x0038 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_YouHaveItExplanation;                           		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bHintToHideBlast : 1;                           		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	class UMaterialInstanceConstant*                   m_RandomLootMIC;                                  		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_RandomLootImage;                                		// 0x0048 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bHintToExpectLoot : 1;                          		// 0x004C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsEnticement : 1;                              		// 0x004C (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct TgClient.TgUIFriendsList.sFriendElement
// 0x0024
struct FsFriendElement
{
	class UUIPrefabInstance*                           m_PrefabInstance;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_OuterButton;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ClassIcon;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Name;                                           		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Level;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Location;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_Highlight;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                m_Id;                                             		// 0x001C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bOnline : 1;                                    		// 0x0020 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUIFriendsList.sFriendButtons
// 0x001C
struct FsFriendButtons
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_AddFriend;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_RemoveFriend;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_SendMessage;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_InviteFriend;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_IgnoreTab;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_IgnoreTabLabel;                                 		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIFriendsList.sIgnoreButtons
// 0x0014
struct FsIgnoreButtons
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_IgnorePlayer;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_RemoveIgnorePlayer;                             		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FriendsTab;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FriendsTabLabel;                                		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIFriendsList.sFriendNotes
// 0x0008
struct FsFriendNotes
{
	class UUILabel*                                    m_DisplayLabel;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_UpdateButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIFriendsList.sSelectedFriend
// 0x0014
struct FsSelectedFriend
{
	struct FString                                     m_Name;                                           		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_Id;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                m_SelectedIdx;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIFriendsList.sFriendPopup
// 0x0018
struct FsFriendPopup
{
	class UUIPanel*                                    m_PopupPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_ConfirmButton;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_CancelButton;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderLabel;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_TextEntryField;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BlockerButton;                                  		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUITutorialItem.sTutItemInfo
// 0x0008
struct FsTutItemInfo
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Key;                                            		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIInstanceSelect.sInstanceStruct
// 0x0018
struct FsInstanceStruct
{
	class UUIButton*                                   m_InstanceMain;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_InstanceCheck;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InstanceName;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_InstancePop;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_InstanceHighlight;                              		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                m_InstanceID;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIInventoryPopup.sInventoryPopupItem
// 0x0018
struct FsInventoryPopupItem
{
	class UTgUIInvWidget*                              InvWidget;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Button;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    nameLabel;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    UnitsLabel;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIItemModification.BaseItemModStruct
// 0x0018
struct FBaseItemModStruct
{
	class UTgUIInvWidget*                              deviceWidget;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   deviceButton;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceNameLabel;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceSkillLabel;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    selectedHLPanel;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mouseOverPanel;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIItemModification.ExtendedItemModStruct
// 0x0010(0x0028 - 0x0018)
struct FExtendedItemModStruct : FBaseItemModStruct
{
	class UUILabel*                                    deviceGenTitleLabel;                              		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceDurTitleLabel;                              		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    deviceGenLabel;                                   		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIProgressBar*                              deviceDurabilityBar;                              		// 0x0024 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIItemModification.PagingStruct
// 0x000C
struct FPagingStruct
{
	class UUIButton*                                   PrevButton;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   NextButton;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    PagingLabel;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUILevelMap.sPlayerWidgetData
// 0x0044
struct FsPlayerWidgetData
{
	struct FString                                     m_PlayerName;                                     		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UMaterialInstanceConstant*                   m_IconToUse;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	struct FVector                                     m_Location;                                       		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FRotator                                    m_Rotation;                                       		// 0x001C (0x000C) [0x0000000000000000]              
	struct FLinearColor                                m_Color;                                          		// 0x0028 (0x0010) [0x0000000000000000]              
	unsigned char                                      m_RotMode;                                        		// 0x0038 (0x0001) [0x0000000000000000]              
	float                                              m_Opacity;                                        		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bIsLocalPlayer : 1;                             		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsFriendly : 1;                                		// 0x0040 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bIsBot : 1;                                     		// 0x0040 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bIsSameTeam : 1;                                		// 0x0040 (0x0004) [0x0000000000000000] [0x00000008] 
};

// ScriptStruct TgClient.TgUILevelMap.sDeployableWidgetData
// 0x003C
struct FsDeployableWidgetData
{
	struct FString                                     m_DeployableName;                                 		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nCounter;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsFriendly : 1;                                		// 0x0010 (0x0004) [0x0000000000000000] [0x00000002] 
	struct FVector                                     m_Location;                                       		// 0x0014 (0x000C) [0x0000000000000000]              
	struct FRotator                                    m_Rotation;                                       		// 0x0020 (0x000C) [0x0000000000000000]              
	float                                              m_Opacity;                                        		// 0x002C (0x0004) [0x0000000000000000]              
	int                                                m_nType;                                          		// 0x0030 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_RotMode;                                        		// 0x0034 (0x0001) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconToUse;                                      		// 0x0038 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUILevelMap.sObjectiveWidgetData
// 0x0034
struct FsObjectiveWidgetData
{
	struct FString                                     m_ObjectiveName;                                  		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned long                                      m_bActive : 1;                                    		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsFriendly : 1;                                		// 0x000C (0x0004) [0x0000000000000000] [0x00000002] 
	struct FVector                                     m_Location;                                       		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FRotator                                    m_Rotation;                                       		// 0x001C (0x000C) [0x0000000000000000]              
	float                                              m_Opacity;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_RotMode;                                        		// 0x002C (0x0001) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconToUse;                                      		// 0x0030 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUILevelMap.sQuestVolumeWidgetData
// 0x0034
struct FsQuestVolumeWidgetData
{
	struct FString                                     m_Name;                                           		// 0x0000 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned long                                      m_bActive : 1;                                    		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	struct FVector                                     m_Location;                                       		// 0x0010 (0x000C) [0x0000000000000000]              
	struct FRotator                                    m_Rotation;                                       		// 0x001C (0x000C) [0x0000000000000000]              
	float                                              m_Opacity;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_RotMode;                                        		// 0x002C (0x0001) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconToUse;                                      		// 0x0030 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUILFGPopup.sLFGPlayerStruct
// 0x0018
struct FsLFGPlayerStruct
{
	class UUIPanel*                                    m_LFGPlayerPanel;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_LFGPlayerIcon;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LFGPlayerName;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LFGPlayerAgency;                                		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LFGPlayerLevel;                                 		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LFGPlayerStatus;                                		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUILFGPopup.sLFGDataStruct
// 0x0020
struct FsLFGDataStruct
{
	int                                                m_LFGProfileId;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     m_LFGPlayerName;                                  		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_LFGAgencyId;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                m_LFGLevel;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                m_LFGStatusId;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                m_LFGTaskforce;                                   		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUILoginMenu.sLoginStatusPanel
// 0x001C
struct FsLoginStatusPanel
{
	unsigned long                                      m_Active : 1;                                     		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_EnableFade : 1;                                 		// 0x0000 (0x0004) [0x0000000000000000] [0x00000002] 
	float                                              m_fTimeStarted;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MainPanel;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_StatusLabel[ 0x2 ];                             		// 0x000C (0x0008) [0x0000000000000000]              
	class UUIButton*                                   m_OKButton;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelButton;                                   		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMailMenu.sAttachmentInfo
// 0x000C
struct FsAttachmentInfo
{
	int                                                inventoryId;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                instanceCount;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          inventoryObject;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMailMenu.sMailItem
// 0x0044
struct FsMailItem
{
	class UUIPrefabInstance*                           mailItemPrefab;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   mailItemButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    mailItemCheck;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    mailItemIcon;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mailItemPanel;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    mailItemSender;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    mailItemSubject;                                  		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    mailItemExpire;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    mailItemHighlight;                                		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   mailSelectButton;                                 		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                mailItemID;                                       		// 0x0028 (0x0004) [0x0000000000000000]              
	unsigned long                                      hasAttachment : 1;                                		// 0x002C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                mailCurrency;                                     		// 0x0030 (0x0004) [0x0000000000000000]              
	int                                                mailStateID;                                      		// 0x0034 (0x0004) [0x0000000000000000]              
	int                                                mailTypeCode;                                     		// 0x0038 (0x0004) [0x0000000000000000]              
	int                                                mailSourceCode;                                   		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      bSystemMail : 1;                                  		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUIMailMenu.sMailTab
// 0x000C
struct FsMailTab
{
	class UUIButton*                                   mailTabButton;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    mailTabLabel;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    mailTabPanel;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMailMenu.sMailAttachment
// 0x0020
struct FsMailAttachment
{
	class UUIButton*                                   attachmentButton;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    attachmentPanel;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    attachmentLabel;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    unitLabel;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    attachmentHighlight;                              		// 0x0010 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_SlotWidgets*                 wItem;                                            		// 0x0014 (0x0004) [0x0000000000000000]              
	int                                                inventoryId;                                      		// 0x0018 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          inventoryObject;                                  		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMissionScene.GameTypeButtonStruct
// 0x0038
struct FGameTypeButtonStruct
{
	struct FString                                     HexButtonTag;                                     		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     NameButtonTag;                                    		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   HexButton;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   NameButton;                                       		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    NameButtonLabel;                                  		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TimerLabel;                                       		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   ConquestTokenIcon;                                		// 0x0028 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   MercenaryTokenIcon;                               		// 0x002C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   HexMIC;                                           		// 0x0030 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AlarmFx;                                          		// 0x0034 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIMissionScene.MissionRowStruct
// 0x0044
struct FMissionRowStruct
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Button;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 Checkbox;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    Icon;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Name;                                             		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   BriefingButton;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    HoverPanel;                                       		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    SelectedPanel;                                    		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   ConquestTokenIcon;                                		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   MercenaryTokenIcon;                               		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    AlarmFx;                                          		// 0x0028 (0x0004) [0x0000000000000000]              
	unsigned long                                      bOpen : 1;                                        		// 0x002C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nExpires;                                         		// 0x0030 (0x0004) [0x0000000000000000]              
	struct FString                                     sMissionName;                                     		// 0x0034 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                nQueueVideoResID;                                 		// 0x0040 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUINavScene.sNavDisplay
// 0x001C
struct FsNavDisplay
{
	class UUIPrefabInstance*                           PrefabInstance;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   Button;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    Highlight;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    Icon;                                             		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Label;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    Description;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	class ATgPointOfInterest*                          POI;                                              		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIOmegaMenu.sOmegaAlertStruct
// 0x0010
struct FsOmegaAlertStruct
{
	class UUIPanel*                                    m_AlertPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AlertLabel;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_UseMessagePanel;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_UseMessageLabel;                                		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPlayerSearchMenu.sSearchResultDisplay
// 0x0030
struct FsSearchResultDisplay
{
	class UUIPrefabInstance*                           resultPrefabInstance;                             		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   resultButton;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    resultHighlight;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    resultClass;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultName;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultAgency;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultAlliance;                                   		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultFaction;                                    		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultLevel;                                      		// 0x0020 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultLocation;                                   		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    resultTeamStatus;                                 		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                resultPlayerId;                                   		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPlayerSearchMenu.sSearchResultDetailEntry
// 0x0008
struct FsSearchResultDetailEntry
{
	class UUILabel*                                    detailEntryField;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    detailEntryValue;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPlayerSearchMenu.sSearchResultDetailDisplay
// 0x0088
struct FsSearchResultDetailDisplay
{
	class UUIPanel*                                    detailPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    detailName;                                       		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FsSearchResultDetailEntry                   detailEntries[ 0x10 ];                            		// 0x0008 (0x0080) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD.WeaponStatusStruct
// 0x0024
struct FWeaponStatusStruct
{
	class UUIImage*                                    wssReticuleWeaponStatus;                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wssReticuleHealth;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wssReticuleEnergy;                                		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wssReticuleHealthAlert;                           		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wssReticuleEnergyAlert;                           		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wssReticuleRange;                                 		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wssReticuleTargetName;                            		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    wssReticuleTargetHealth;                          		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    wssReticuleTargetShield;                          		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD.sAttentionBar
// 0x0010
struct FsAttentionBar
{
	class UUIPanel*                                    m_Panel;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Notice;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Instructions;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUIPrimaryHUD.sAttentionPanel
// 0x0024
struct FsAttentionPanel
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FsAttentionBar                              m_AttentionBar[ 0x2 ];                            		// 0x0004 (0x0020) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_Settings.BasePanelStruct
// 0x0008
struct FBasePanelStruct
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            SceneDriver;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_BuffBar.sBuffWidgetData
// 0x0024
struct FsBuffWidgetData
{
	unsigned long                                      bActive : 1;                                      		// 0x0000 (0x0004) [0x0000000000100000] [0x00000001] 
	class UUIButton*                                   BaseButton;                                       		// 0x0004 (0x0004) [0x0000000000100000]              
	class UUIImage*                                    BuffIconImage;                                    		// 0x0008 (0x0004) [0x0000000000100000]              
	class UUIProgressBar*                              BuffProgressBar;                                  		// 0x000C (0x0004) [0x0000000000100000]              
	class UUILabel*                                    BuffName;                                         		// 0x0010 (0x0004) [0x0000000000100000]              
	class UUILabel*                                    BuffStackLabel;                                   		// 0x0014 (0x0004) [0x0000000000100000]              
	int                                                BuffID;                                           		// 0x0018 (0x0004) [0x0000000000100000]              
	int                                                BuffStacks;                                       		// 0x001C (0x0004) [0x0000000000100000]              
	float                                              fTimeRemaining;                                   		// 0x0020 (0x0004) [0x0000000000100000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_DangerBox.AlertInfo
// 0x0030
struct FAlertInfo
{
	unsigned char                                      ePriority;                                        		// 0x0000 (0x0001) [0x0000000000000000]              
	unsigned char                                      eType;                                            		// 0x0001 (0x0001) [0x0000000000000000]              
	float                                              fDuration;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	float                                              fInitTime;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	float                                              fFadeInitTime;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              fOpacity;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                nMessageId;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
	struct FString                                     sMessage;                                         		// 0x0018 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                nAlertNumber;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                nSoundResId;                                      		// 0x0028 (0x0004) [0x0000000000000000]              
	unsigned long                                      bHasPlayedSound : 1;                              		// 0x002C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      bFadeOut : 1;                                     		// 0x002C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      bForceUpdate : 1;                                 		// 0x002C (0x0004) [0x0000000000000000] [0x00000004] 
};

// ScriptStruct TgClient.TgUIPrimaryHUD_DangerBox.AlertPanel
// 0x0010
struct FAlertPanel
{
	class UUIPanel*                                    wDangerPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    wDangerLabel[ 0x3 ];                              		// 0x0004 (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_DeviceBar.sDeviceSlot
// 0x0018
struct FsDeviceSlot
{
	class UTgUIInvWidget*                              Item;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nSlotNum;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    KeyLabel;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIProgressBar*                              HealthBar;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	float                                              fHighlightEnd;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned long                                      bHighlightActive : 1;                             		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveTeamStruct
// 0x0010
struct FObjectiveTeamStruct
{
	class UUIPanel*                                    otsTeamPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    otsTeamLabel;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    otsTeamIcons[ 0x2 ];                              		// 0x0008 (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveCTFRStruct
// 0x0014
struct FObjectiveCTFRStruct
{
	class UUIPanel*                                    ocsCTFRPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ocsCTFRIcons[ 0x4 ];                              		// 0x0004 (0x0010) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveBarStruct
// 0x0024
struct FObjectiveBarStruct
{
	class UUIPanel*                                    obsBarPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    obsBarLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    obsBarActiveImg;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    obsBarImg;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   obsBarMIC;                                        		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    obsBarOwnerIcons[ 0x4 ];                          		// 0x0014 (0x0010) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveListStruct
// 0x00E0
struct FObjectiveListStruct
{
	class UUIPanel*                                    olsObjPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FObjectiveBarStruct                         olsObjList[ 0x6 ];                                		// 0x0004 (0x00D8) [0x0000000000000000]              
	class UUIImage*                                    olsObjTileImg;                                    		// 0x00DC (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.HexStatusIndicatorStruct
// 0x0010
struct FHexStatusIndicatorStruct
{
	class UUIPanel*                                    hsiPanel;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    hsiStatusTextLabel;                               		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    hsiStatusLabel;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    hsiHexImage;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.HexMultiStatusIndicatorHorzStuct
// 0x007C
struct FHexMultiStatusIndicatorHorzStuct
{
	class UUIPanel*                                    hmsihPanel;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    hmsihRoute[ 0x6 ];                                		// 0x0004 (0x0018) [0x0000000000000000]              
	class UUIImage*                                    hmsihSpawn[ 0x6 ];                                		// 0x001C (0x0018) [0x0000000000000000]              
	class UUIImage*                                    hmsihTurret[ 0x6 ];                               		// 0x0034 (0x0018) [0x0000000000000000]              
	class UUIImage*                                    hmsihHexImage[ 0x6 ];                             		// 0x004C (0x0018) [0x0000000000000000]              
	class UUILabel*                                    hmsihTeam[ 0x6 ];                                 		// 0x0064 (0x0018) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveTicketStruct
// 0x0024
struct FObjectiveTicketStruct
{
	class UUIPanel*                                    otsTicketPanel;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    otsTicketRateLabel[ 0x2 ];                        		// 0x0004 (0x0008) [0x0000000000000000]              
	class UUIImage*                                    otsTicketBarImg[ 0x2 ];                           		// 0x000C (0x0008) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   otsTicketBarMIC[ 0x2 ];                           		// 0x0014 (0x0008) [0x0000000000000000]              
	class UUILabel*                                    otsTicketPointsLabel[ 0x2 ];                      		// 0x001C (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveBotPointsStruct
// 0x0034
struct FObjectiveBotPointsStruct
{
	class UUIPanel*                                    obpsBotPointsPanel;                               		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    obpsBotPointsAllyLightsOn[ 0x3 ];                 		// 0x0004 (0x000C) [0x0000000000000000]              
	class UUIImage*                                    obpsBotPointsEnemyLightsOn[ 0x3 ];                		// 0x0010 (0x000C) [0x0000000000000000]              
	class UUIImage*                                    obpsBotPointsAllyLightsOff[ 0x3 ];                		// 0x001C (0x000C) [0x0000000000000000]              
	class UUIImage*                                    obpsBotPointsEnemyLightsOff[ 0x3 ];               		// 0x0028 (0x000C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveVictoryBonusStruct
// 0x0010
struct FObjectiveVictoryBonusStruct
{
	class UUIPanel*                                    ovbsVictoryBonusPanel;                            		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ovbsDeathsProgressBar;                            		// 0x0004 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   ovbsDeathsProgressMIC;                            		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    ovbsDeathsProgressLabel;                          		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_MissionInfo.ObjectiveClockStruct
// 0x0008
struct FObjectiveClockStruct
{
	class UUIPanel*                                    ocsClockPanel;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_Clock*                       ocsMissionClock;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_QuestTracking.QTrackIds
// 0x0008
struct FQTrackIds
{
	int                                                Id;                                               		// 0x0000 (0x0004) [0x0000000000004000]              ( CPF_Config )
	int                                                Idx;                                              		// 0x0004 (0x0004) [0x0000000000004000]              ( CPF_Config )
};

// ScriptStruct TgClient.TgUIPrimaryHUD_RaidTimer.RaidClockStruct
// 0x0024
struct FRaidClockStruct
{
	class UUIPanel*                                    rcsPanel;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    rcsBonusPanel;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcsLabel;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    rcsDefImage;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    rcsAtkImage;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcsDefBonusTextLabel;                             		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcsDefBonusLabel;                                 		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcsAtkBonusTextLabel;                             		// 0x001C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcsAtkBonusLabel;                                 		// 0x0020 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIPrimaryHUD_RaidTimer.RaidTicketStruct
// 0x0018
struct FRaidTicketStruct
{
	class UUIPanel*                                    rtsTicketPanel;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rtsTicketHeaderLabel;                             		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rtsTicketLabel;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    rtsTicketBarImage;                                		// 0x000C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   rtsTicketBarMIC;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	unsigned long                                      rtsBonusPanelAdjusted : 1;                        		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUIPrimaryHUD_TeamPanel.sTeamHealthStruct
// 0x001C
struct FsTeamHealthStruct
{
	class UUIPanel*                                    m_PlayerPanel;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PlayerNameLabel;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_PlayerHealthImage;                              		// 0x0008 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_PlayerHealthMIC;                                		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PlayerHealthLabel;                              		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_PlayerIconImage;                                		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_PlayerStatusImage;                              		// 0x0018 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sQTutPanels
// 0x0010
struct FsQTutPanels
{
	class UUIPanel*                                    StepPanels[ 0x4 ];                                		// 0x0000 (0x0010) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sRewardLineItem
// 0x0018
struct FsRewardLineItem
{
	class UUIButton*                                   m_OuterButton;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Name;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MouseOverPanel;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SelectedPanel;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_Item;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CountLabel;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sAutoRewards
// 0x00C0
struct FsAutoRewards
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BasicRewards;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ItemsPanel;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_FourItemPanel;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TwoItemPanel;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_OneItemPanel;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	struct FsRewardLineItem                            m_RewardItems_Four[ 0x4 ];                        		// 0x0018 (0x0060) [0x0000000000000000]              
	struct FsRewardLineItem                            m_RewardItems_Two[ 0x2 ];                         		// 0x0078 (0x0030) [0x0000000000000000]              
	struct FsRewardLineItem                            m_RewardItems_One;                                		// 0x00A8 (0x0018) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sSelectableRewards
// 0x004C
struct FsSelectableRewards
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FsRewardLineItem                            m_Selectables[ 0x3 ];                             		// 0x0004 (0x0048) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sBoostRewards
// 0x002C
struct FsBoostRewards
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BasicRewards;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ItemsPanel;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FsRewardLineItem                            m_RewardItem;                                     		// 0x000C (0x0018) [0x0000000000000000]              
	class UUIImage*                                    m_RedOut;                                         		// 0x0024 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_RedOutItem;                                     		// 0x0028 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sEliteRewards
// 0x000C
struct FsEliteRewards
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BasicRewards;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_RedOut;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sCompletionWidgets
// 0x0168
struct FsCompletionWidgets
{
	class UUIPanel*                                    m_MainPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FinishButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BackButton;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestName;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Dialog;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RewardPanel;                                    		// 0x0014 (0x0004) [0x0000000000000000]              
	struct FsAutoRewards                               m_EarnAllRewards;                                 		// 0x0018 (0x00C0) [0x0000000000000000]              
	struct FsSelectableRewards                         m_EarnOneReward;                                  		// 0x00D8 (0x004C) [0x0000000000000000]              
	struct FsBoostRewards                              m_BoostOnlyReward;                                		// 0x0124 (0x002C) [0x0000000000000000]              
	struct FsEliteRewards                              m_EliteOnlyReward;                                		// 0x0150 (0x000C) [0x0000000000000000]              
	class UUIPanel*                                    m_ChooseOneHelp;                                  		// 0x015C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BuyNowButton;                                   		// 0x0160 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BuyNowLabel;                                    		// 0x0164 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sQuestRequirement
// 0x0014
struct FsQuestRequirement
{
	class UUIButton*                                   m_OuterButton;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Name;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Count;                                          		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_IncompleteHL;                                   		// 0x000C (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_Item;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sQuestTaskPanel
// 0x0054
struct FsQuestTaskPanel
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FsQuestRequirement                          m_Requirements[ 0x4 ];                            		// 0x0004 (0x0050) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sQuestReqItemPanel
// 0x0054
struct FsQuestReqItemPanel
{
	class UUIPanel*                                    m_OuterPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FsQuestRequirement                          m_LineItems[ 0x4 ];                               		// 0x0004 (0x0050) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sTurninWidgets
// 0x00BC
struct FsTurninWidgets
{
	class UUIPanel*                                    m_MainPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FinishButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BackButton;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestName;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Dialog;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	struct FsQuestTaskPanel                            m_Tasks;                                          		// 0x0014 (0x0054) [0x0000000000000000]              
	struct FsQuestReqItemPanel                         m_ReqItems;                                       		// 0x0068 (0x0054) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sOfferWidgets
// 0x0128
struct FsOfferWidgets
{
	class UUIPanel*                                    m_MainPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AcceptButton;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeclineButton;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestName;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestInstructions;                              		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Dialog;                                         		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RewardPanel;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
	struct FsAutoRewards                               m_EarnAllRewards;                                 		// 0x001C (0x00C0) [0x0000000000000000]              
	struct FsSelectableRewards                         m_EarnOneReward;                                  		// 0x00DC (0x004C) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sQuestLink
// 0x0018
struct FsQuestLink
{
	class UUIButton*                                   m_OuterButton;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestName;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_QuestIcon;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MouseOverPanel;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SelectedPanel;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pQuest;                                         		// 0x0014 (0x0004) [0x0000000000103000]              ( CPF_Native | CPF_Transient )
};

// ScriptStruct TgClient.TgUIQuestDialog.sIntroWidgets
// 0x0104
struct FsIntroWidgets
{
	class UUIPanel*                                    m_MainPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NPCSmalltalk;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AvailableQuestsPanel;                           		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FsQuestLink                                 m_AvailableQuests[ 0xA ];                         		// 0x000C (0x00F0) [0x0000000000000000]              
	class UUIButton*                                   m_NextButton;                                     		// 0x00FC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NextButtonLabel;                                		// 0x0100 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestDialog.sQuestErrMsg
// 0x0010
struct FsQuestErrMsg
{
	unsigned long                                      bValid : 1;                                       		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                nQuestId;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	int                                                nErrType;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nSecondary;                                       		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestLog.sTrackingSelection
// 0x0010
struct FsTrackingSelection
{
	class UUIPanel*                                    m_TrackPanel;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FeedbackLabel;                                  		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_Button;                                         		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ButtonLabel;                                    		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestLog.sActiveWidgets
// 0x01DC
struct FsActiveWidgets
{
	class UUIPanel*                                    m_MainPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestName;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Instructions;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FsQuestTaskPanel                            m_Tasks;                                          		// 0x000C (0x0054) [0x0000000000000000]              
	struct FsQuestReqItemPanel                         m_ReqItems;                                       		// 0x0060 (0x0054) [0x0000000000000000]              
	class UUIPanel*                                    m_RewardPanel;                                    		// 0x00B4 (0x0004) [0x0000000000000000]              
	struct FsAutoRewards                               m_EarnAllRewards;                                 		// 0x00B8 (0x00C0) [0x0000000000000000]              
	struct FsSelectableRewards                         m_EarnOneReward;                                  		// 0x0178 (0x004C) [0x0000000000000000]              
	class UUILabel*                                    m_Feedback;                                       		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AbandonButton;                                  		// 0x01C8 (0x0004) [0x0000000000000000]              
	struct FsTrackingSelection                         m_TrackingSelection;                              		// 0x01CC (0x0010) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQuestLog.sQuestLogElement
// 0x000C
struct FsQuestLogElement
{
	class UUIButton*                                   m_OuterButton;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestLabel;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_QuestStateIcon;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQueueScene.QueueRowStruct
// 0x0034
struct FQueueRowStruct
{
	class UUIButton*                                   qrsQueueButton;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueStatus;                                   		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueType;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueMission;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueWaiting;                                  		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueReward;                                   		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueTeams;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    qrsQueueActive;                                   		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    qrsQueueHighlight;                                		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                qrsQueueID;                                       		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                qrsQueueLevelMin;                                 		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                qrsQueueLevelMax;                                 		// 0x002C (0x0004) [0x0000000000000000]              
	int                                                qrsQueueSideMax;                                  		// 0x0030 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIQueueScene.QueueFeedbackStruct
// 0x0010
struct FQueueFeedbackStruct
{
	class UUILabel*                                    qfsFeedbackLabel;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     qfsFeedbackString;                                		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUIRaidHexMap.MissionDelayStruct
// 0x0014
struct FMissionDelayStruct
{
	class UUIPanel*                                    MissionDelayPanel;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    MissionDelayLabel;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    MissionTimeLabel;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
	int                                                nBaseWorldTime;                                   		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                nTimeRemaining;                                   		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIRaidHexMap.ButtonStruct
// 0x0008
struct FButtonStruct
{
	class UUIButton*                                   bsButton;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    bsLabel;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIRaidHexMap.RaidControlPanelStruct
// 0x0020
struct FRaidControlPanelStruct
{
	class UUIPanel*                                    rcpContainerPanel;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    rcpClassIcon;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcpNameLabel;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    rcpTeamNumber;                                    		// 0x000C (0x0004) [0x0000000000000000]              
	struct FButtonStruct                               rcpLaunchButtonStruct;                            		// 0x0010 (0x0008) [0x0000000000000000]              
	struct FButtonStruct                               rcpReturnButtonStruct;                            		// 0x0018 (0x0008) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIRaidMenu.MemberStruct
// 0x0034
struct FMemberStruct
{
	class UUIButton*                                   MemberButton;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    MemberPanel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    ClassIconImage;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    MemberNameLabel;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    SelectedImage;                                    		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    TeamLeadImage;                                    		// 0x0014 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    RaidLeadImage;                                    		// 0x0018 (0x0004) [0x0000000000000000]              
	int                                                nMemberIndex;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                nTeamIndex;                                       		// 0x0020 (0x0004) [0x0000000000000000]              
	int                                                nTaskTeamNum;                                     		// 0x0024 (0x0004) [0x0000000000000000]              
	int                                                nCharId;                                          		// 0x0028 (0x0004) [0x0000000000000000]              
	int                                                nProfileId;                                       		// 0x002C (0x0004) [0x0000000000000000]              
	unsigned long                                      bIsTeamLead : 1;                                  		// 0x0030 (0x0004) [0x0000000000000000] [0x00000001] 
};

// ScriptStruct TgClient.TgUIRaidMenu.RaidTeamStruct
// 0x0290
struct FRaidTeamStruct
{
	class UUIPanel*                                    TeamPanel;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TeamLabel;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TeamNumberLabel;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    TeamGraphicImage;                                 		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   TeamButton;                                       		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    TeamMemberPanel;                                  		// 0x0014 (0x0004) [0x0000000000000000]              
	struct FMemberStruct                               Members[ 0xC ];                                   		// 0x0018 (0x0270) [0x0000000000000000]              
	int                                                nTeamIndex;                                       		// 0x0288 (0x0004) [0x0000000000000000]              
	int                                                nTaskTeamNum;                                     		// 0x028C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIRaidMenu.TotalAgentsStruct
// 0x000C
struct FTotalAgentsStruct
{
	class UUIPanel*                                    TotalAgentsPanel;                                 		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TotalAgentsLabel;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    TotalNumberLabel;                                 		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIRaidMenu.SelectedSlot
// 0x0008
struct FSelectedSlot
{
	int                                                nTeamIndex;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                nMemberIndex;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISceneManager.sSceneDriverInfo
// 0x0028
struct FsSceneDriverInfo
{
	class UClass*                                      m_DriverClass;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     m_SceneResource;                                  		// 0x0004 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FName                                       m_SpecificName;                                   		// 0x0010 (0x0008) [0x0000000000000000]              
	unsigned long                                      m_bCacheDriver : 1;                               		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
	class UTgUISceneDriver*                            m_DriverInst;                                     		// 0x001C (0x0004) [0x0000000000000000]              
	int                                                m_GCFlags;                                        		// 0x0020 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_CachedAppPtr;                                   		// 0x0024 (0x0004) [0x0000000000001000]              ( CPF_Native )
};

// ScriptStruct TgClient.TgUIScoreboard.sScorePlayerStruct
// 0x0018
struct FsScorePlayerStruct
{
	class UUIImage*                                    spsStatusIcon;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    spsScoreLabels[ 0x5 ];                            		// 0x0004 (0x0014) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIScoreboard.sScoreTeamStruct
// 0x00F8
struct FsScoreTeamStruct
{
	class UUILabel*                                    stsTeamTitle;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    stsTeamIcon;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FsScorePlayerStruct                         stsPlayerList[ 0xA ];                             		// 0x0008 (0x00F0) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIScoreDetails.PlayerField
// 0x0038
struct FPlayerField
{
	class UUIImage*                                    PlayerClassIcon;                                  		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    FieldLabels[ 0xD ];                               		// 0x0004 (0x0034) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Chat.CC_ConfigElement
// 0x000C
struct FCC_ConfigElement
{
	unsigned long                                      m_bActive : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUILabel*                                    m_Label;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_Checkbox;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Chat.CC_Group_Configuration
// 0x0090
struct FCC_Group_Configuration
{
	unsigned long                                      m_bActive : 1;                                    		// 0x0000 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIPanel*                                    m_Panel;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_GroupNameEditBox;                               		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FCC_ConfigElement                           m_Channels[ 0xB ];                                		// 0x000C (0x0084) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Controls.sKeyBindButton
// 0x0010
struct FsKeyBindButton
{
	class UUIButton*                                   kbbKeyButton;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    kbbKeyLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    kbbHighlightBG;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    kbbNormalBG;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Controls.sKeyBindStruct
// 0x0030
struct FsKeyBindStruct
{
	class UUIPrefabInstance*                           kbsRowInstance;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    kbsRowPanel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    kbsCommandLabel;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	struct FsKeyBindButton                             kbsKeyBindButton[ 0x2 ];                          		// 0x000C (0x0020) [0x0000000000000000]              
	int                                                kbsCategoryID;                                    		// 0x002C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Controls.sKeyBindCategory
// 0x0018
struct FsKeyBindCategory
{
	class UUIPrefabInstance*                           kbcCategoryHeader;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    kbcCategoryLabel;                                 		// 0x0004 (0x0004) [0x0000000000000000]              
	TArray< struct FsKeyBindStruct >                   kbcKeyBindList;                                   		// 0x0008 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                kbcCategoryID;                                    		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Controls.sCommandInfo
// 0x0010
struct FsCommandInfo
{
	int                                                nCommandMsgId;                                    		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FString                                     sCommandMapping;                                  		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct TgClient.TgUISettingsMenu_Controls.sKeyBind
// 0x000C
struct FsKeyBind
{
	struct FName                                       nmKeyName;                                        		// 0x0000 (0x0008) [0x0000000000000000]              
	int                                                nKeyMsgId;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Video.sTgAspectRatioInfo
// 0x000C
struct FsTgAspectRatioInfo
{
	float                                              m_AspectRatio;                                    		// 0x0000 (0x0004) [0x0000000000100000]              
	struct FName                                       m_ArName;                                         		// 0x0004 (0x0008) [0x0000000000100000]              
};

// ScriptStruct TgClient.TgUISettingsMenu_Video.sVideoConfirm
// 0x001C
struct FsVideoConfirm
{
	class UUIPanel*                                    m_ConfirmPanel;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ConfirmText;                                    		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmOK;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmYes;                                     		// 0x000C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmNo;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmBlocker;                                 		// 0x0014 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bIsResetDialog : 1;                             		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsActive : 1;                                  		// 0x0018 (0x0004) [0x0000000000000000] [0x00000002] 
};

// ScriptStruct TgClient.TgUISettingsMenu_Video.sOptionValues
// 0x0080
struct FsOptionValues
{
	int                                                m_AspectRatioIndex;                               		// 0x0000 (0x0004) [0x0000000000000000]              
	int                                                m_ResolutionIndex;                                		// 0x0004 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_eWindowType;                                    		// 0x0008 (0x0001) [0x0000000000000000]              
	unsigned long                                      m_bCustomRes : 1;                                 		// 0x000C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_CheckboxOptions[ 0x13 ];                        		// 0x0010 (0x004C) [0x0000000000000000]              
	int                                                m_ComboBoxOptions[ 0x8 ];                         		// 0x005C (0x0020) [0x0000000000000000]              
	float                                              m_GammaCorrection;                                		// 0x007C (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIUnlockScene.sUnlockItemPanel
// 0x0018
struct FsUnlockItemPanel
{
	class UUIPanel*                                    m_ItemPanel;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemLabel;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ItemNavButton;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ItemEquipButton;                                		// 0x000C (0x0004) [0x0000000000000000]              
	int                                                m_nItemType;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	int                                                m_nItemCount;                                     		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIUpdateFaceMenu.sUFPStruct
// 0x0014
struct FsUFPStruct
{
	class UUIImage*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_APIcon;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_TokenIcon;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CurrencyLabel;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Value;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIUpdateHairMenu.sUHPStruct
// 0x0014
struct FsUHPStruct
{
	class UUIImage*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_APIcon;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_TokenIcon;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CurrencyLabel;                                  		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Value;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorPaymentPopup.OneCurrencyOption
// 0x0018
struct FOneCurrencyOption
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_APIcon;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CreditIcon;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TokenIcon;                                      		// 0x000C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Price;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Currency;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorPaymentPopup.TwoCurrencyOption
// 0x0034
struct FTwoCurrencyOption
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FOneCurrencyOption                          m_Currency1;                                      		// 0x0004 (0x0018) [0x0000000000000000]              
	struct FOneCurrencyOption                          m_Currency2;                                      		// 0x001C (0x0018) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorPaymentPopup.StdPaymentOptionPanel
// 0x0058
struct FStdPaymentOptionPanel
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FOneCurrencyOption                          m_OneCurrency;                                    		// 0x0004 (0x0018) [0x0000000000000000]              
	struct FTwoCurrencyOption                          m_TwoCurrency;                                    		// 0x001C (0x0034) [0x0000000000000000]              
	class UUILabel*                                    m_PurchaseButtonLabel;                            		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PurchaseButton;                                 		// 0x0054 (0x0004) [0x0000000000000000]              
};

// ScriptStruct TgClient.TgUIVendorPaymentPopup.APPaymentOptionPanel
// 0x0024
struct FAPPaymentOptionPanel
{
	class UUIPanel*                                    m_Outer;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FOneCurrencyOption                          m_OneCurrency;                                    		// 0x0004 (0x0018) [0x0000000000000000]              
	class UUILabel*                                    m_PurchaseButtonLabel;                            		// 0x001C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PurchaseButton;                                 		// 0x0020 (0x0004) [0x0000000000000000]              
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif