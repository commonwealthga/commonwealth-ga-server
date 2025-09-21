/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: TgClient_f_structs.h
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
# Function Structs
# ========================================================================================= #
*/

// Function TgClient.TgAgencyData.GetDefaultRankData
// [0x00820802] ( FUNC_Event )
struct UTgAgencyData_eventGetDefaultRankData_Parms
{
	struct FsAgencyRankData                            ReturnValue;                                      		// 0x0000 (0x0018) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
	// struct FsAgencyRankData                         sInfo;                                            		// 0x0018 (0x0018) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.GetDefaultMemberInfo
// [0x00820802] ( FUNC_Event )
struct UTgAgencyData_eventGetDefaultMemberInfo_Parms
{
	struct FsAgencyMemberInfo                          ReturnValue;                                      		// 0x0000 (0x004C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
	// struct FsAgencyMemberInfo                       sInfo;                                            		// 0x004C (0x004C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.Tick
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.SetTax
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execSetTax_Parms
{
	float                                              fTax;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.SetDescription
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execSetDescription_Parms
{
	struct FString                                     sDescription;                                     		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.SetOfficerComment
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execSetOfficerComment_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     sNote;                                            		// 0x0004 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.SetPublicComment
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execSetPublicComment_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     sNote;                                            		// 0x0004 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.SetMOTD
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execSetMOTD_Parms
{
	struct FString                                     sMOTD;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.TransferLeader
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execTransferLeader_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.Disband
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execDisband_Parms
{
};

// Function TgClient.TgAgencyData.Create
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execCreate_Parms
{
	struct FString                                     sAgencyName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FLinearColor                                AgencyColor;                                      		// 0x000C (0x0010) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.Invite
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execInvite_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.Remove
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execRemove_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.Demote
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execDemote_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.Promote
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execPromote_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.UpdateRecruiting
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execUpdateRecruiting_Parms
{
	struct FString                                     sText;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      bRecruiting : 1;                                  		// 0x000C (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bSubOnly : 1;                                     		// 0x0010 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgAgencyData.CheckFiltered
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execCheckFiltered_Parms
{
	struct FsAgencyMemberInfo                          Info;                                             		// 0x0000 (0x004C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                nFilter;                                          		// 0x004C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0050 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgAgencyData.SetFilter
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execSetFilter_Parms
{
	int                                                nNewFilter;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.GetNumMembersOfRank
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execGetNumMembersOfRank_Parms
{
	int                                                nRankId;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgAgencyData.LocalPlayerHasPermission
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execLocalPlayerHasPermission_Parms
{
	int                                                flag;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgAgencyData.LocalPlayerIsLeader
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execLocalPlayerIsLeader_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgAgencyData.GetLocalPlayerInfo
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execGetLocalPlayerInfo_Parms
{
	struct FsAgencyMemberInfo                          ReturnValue;                                      		// 0x0000 (0x004C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.GetMemberInfo
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execGetMemberInfo_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FsAgencyMemberInfo                          ReturnValue;                                      		// 0x0004 (0x004C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.GetLocalRankData
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execGetLocalRankData_Parms
{
	struct FsAgencyRankData                            ReturnValue;                                      		// 0x0000 (0x0018) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.GetRankData
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execGetRankData_Parms
{
	int                                                nRank;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FsAgencyRankData                            ReturnValue;                                      		// 0x0004 (0x0018) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.ClearData
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execClearData_Parms
{
	int                                                rosterflags;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAgencyData.RequestAgencyInfo
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execRequestAgencyInfo_Parms
{
	int                                                Flags;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgAgencyData.GetSortedInfoArray
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execGetSortedInfoArray_Parms
{
	TArray< struct FsAgencyMemberInfo >                ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgAgencyData.InitAgencyData
// [0x00020400] ( FUNC_Native )
struct UTgAgencyData_execInitAgencyData_Parms
{
};

// Function TgClient.TgAllianceData.GetDefaultMemberInfo
// [0x00820802] ( FUNC_Event )
struct UTgAllianceData_eventGetDefaultMemberInfo_Parms
{
	struct FsAllianceMemberInfo                        ReturnValue;                                      		// 0x0000 (0x0024) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
	// struct FsAllianceMemberInfo                     sInfo;                                            		// 0x0024 (0x0024) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgAllianceData.Disband
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execDisband_Parms
{
};

// Function TgClient.TgAllianceData.Remove
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execRemove_Parms
{
	int                                                nAgencyId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAllianceData.Invite
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execInvite_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAllianceData.Create
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execCreate_Parms
{
	struct FString                                     sName;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgAllianceData.GetMemberInfo
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execGetMemberInfo_Parms
{
	int                                                nAgencyId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FsAllianceMemberInfo                        ReturnValue;                                      		// 0x0004 (0x0024) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgAllianceData.Tick
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execTick_Parms
{
	float                                              DeltaSeconds;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgAllianceData.ClearData
// [0x00024400] ( FUNC_Native )
struct UTgAllianceData_execClearData_Parms
{
	unsigned long                                      bGeneralData : 1;                                 		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bMemberData : 1;                                  		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgAllianceData.RequestAllianceInfo
// [0x00020400] ( FUNC_Native )
struct UTgAllianceData_execRequestAllianceInfo_Parms
{
	int                                                Flags;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgConsole.InputKey
// [0x00024002] 
struct UTgConsole_execInputKey_Parms
{
	int                                                ControllerId;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FName                                       Key;                                              		// 0x0004 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      Event;                                            		// 0x000C (0x0001) [0x0000000000000080]              ( CPF_Parm )
	float                                              AmountDepressed;                                  		// 0x0010 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bGamepad : 1;                                     		// 0x0014 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0018 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgConsole.SetEnabled
// [0x00020400] ( FUNC_Native )
struct UTgConsole_execSetEnabled_Parms
{
	unsigned long                                      bEnabled : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgDataInterface.GetLocalBotList
// [0x00020400] ( FUNC_Native )
struct UTgDataInterface_execGetLocalBotList_Parms
{
	class UTgDataSet*                                  pDataSet;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataInterface.GetInstanceList
// [0x00020400] ( FUNC_Native )
struct UTgDataInterface_execGetInstanceList_Parms
{
	class UTgDataSet*                                  pDataSet;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataInterface.QueryPlayers
// [0x00020400] ( FUNC_Native )
struct UTgDataInterface_execQueryPlayers_Parms
{
	class UTgDataSet*                                  pDataSet;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataInterface.GetCharacterStats
// [0x00020400] ( FUNC_Native )
struct UTgDataInterface_execGetCharacterStats_Parms
{
	class UTgDataSet*                                  pDataSet;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataInterface.GetLootTableItems
// [0x00020400] ( FUNC_Native )
struct UTgDataInterface_execGetLootTableItems_Parms
{
	int                                                nLootTableId;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nProfileId;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgDataSet*                                  pDataSet;                                         		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataInterface.AddCharacter
// [0x00020400] ( FUNC_Native )
struct UTgDataInterface_execAddCharacter_Parms
{
	struct FCharacterInfoStruct                        CharInfo;                                         		// 0x0000 (0x0038) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	class UTgDataSet*                                  pDataSet;                                         		// 0x0038 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataSet.MakeRequest
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execMakeRequest_Parms
{
	struct FString                                     sFuncName;                                        		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.UpdateParam
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execUpdateParam_Parms
{
	struct FString                                     sName;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sValue;                                           		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.AddParam
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execAddParam_Parms
{
	struct FString                                     sName;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sValue;                                           		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.CallRegisteredForEvents
// [0x00020400] ( FUNC_Native )
struct UTgDataSet_execCallRegisteredForEvents_Parms
{
	unsigned char                                      eEventType;                                       		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataSet.RegisterForEvents
// [0x00020400] ( FUNC_Native )
struct UTgDataSet_execRegisterForEvents_Parms
{
	class UObject*                                     pObject;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FName                                       nmFuncName;                                       		// 0x0004 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataSet.GetErrorText
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetErrorText_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.GetErrorColumn
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetErrorColumn_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.GetErrorCode
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetErrorCode_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.AddCurrentRow
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execAddCurrentRow_Parms
{
	class UTgDataSet*                                  pDataSet;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataSet.Sort
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execSort_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      bAscending : 1;                                   		// 0x000C (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgDataSet.DeleteAll
// [0x00024401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execDeleteAll_Parms
{
	unsigned long                                      bUpdateBound : 1;                                 		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgDataSet.DeleteRow
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execDeleteRow_Parms
{
};

// Function TgClient.TgDataSet.AddRow
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execAddRow_Parms
{
};

// Function TgClient.TgDataSet.GoToRowSet
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGoToRowSet_Parms
{
	int                                                nTokenId;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.MoveTo
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execMoveTo_Parms
{
	int                                                nRowNumber;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.MoveLast
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execMoveLast_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.MoveNext
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execMoveNext_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.MoveFirst
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execMoveFirst_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.SetNbrColValue
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execSetNbrColValue_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	float                                              fValue;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgDataSet.SetStrColValue
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execSetStrColValue_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sValue;                                           		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.GetNbrColValue
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetNbrColValue_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	float                                              ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.GetStrColValueByRow
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetStrColValueByRow_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                iRowId;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     ReturnValue;                                      		// 0x0010 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.GetStrColValue
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetStrColValue_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     ReturnValue;                                      		// 0x000C (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.GetStrColText
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetStrColText_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     ReturnValue;                                      		// 0x000C (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgDataSet.GetCurrentRowId
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execGetCurrentRowId_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.ColumnEmptyOrNull
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execColumnEmptyOrNull_Parms
{
	struct FString                                     sColName;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      ReturnValue : 1;                                  		// 0x000C (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.RecordCount
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execRecordCount_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgDataSet.Reinitialize
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execReinitialize_Parms
{
};

// Function TgClient.TgDataSet.Clear
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgDataSet_execClear_Parms
{
};

// Function TgClient.TgGameViewportClient.GameSessionEnded
// [0x00020802] ( FUNC_Event )
struct UTgGameViewportClient_eventGameSessionEnded_Parms
{
};

// Function TgClient.TgGameViewportClient.DrawTransition
// [0x00020000] 
struct UTgGameViewportClient_execDrawTransition_Parms
{
	class UCanvas*                                     Canvas;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgGameViewportClient.Init
// [0x00420802] ( FUNC_Event )
struct UTgGameViewportClient_eventInit_Parms
{
	struct FString                                     OutError;                                         		// 0x0000 (0x000C) [0x0000000000400180]              ( CPF_Parm | CPF_OutParm | CPF_NeedCtorLink )
	unsigned long                                      ReturnValue : 1;                                  		// 0x000C (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgGameViewportClient.NativeGameSessionEnded
// [0x00020400] ( FUNC_Native )
struct UTgGameViewportClient_execNativeGameSessionEnded_Parms
{
};

// Function TgClient.TgGameViewportClient.ClearDeferredManifests
// [0x00020400] ( FUNC_Native )
struct UTgGameViewportClient_execClearDeferredManifests_Parms
{
};

// Function TgClient.TgGameViewportClient.AddDeferredManifest
// [0x00020400] ( FUNC_Native )
struct UTgGameViewportClient_execAddDeferredManifest_Parms
{
	class UTgObjectReferencer*                         ObjReferencer;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgGameViewportClient.DetachHUDSettings
// [0x00020400] ( FUNC_Native )
struct UTgGameViewportClient_execDetachHUDSettings_Parms
{
};

// Function TgClient.TgGameViewportClient.CenterMouseInViewport
// [0x00024400] ( FUNC_Native )
struct UTgGameViewportClient_execCenterMouseInViewport_Parms
{
	unsigned long                                      CenterPreCaptureMousePos : 1;                     		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.ShowPathToAttentionPOI
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execShowPathToAttentionPOI_Parms
{
};

// Function TgClient.TgHUD_Game.StartVoiceMacroTree
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execStartVoiceMacroTree_Parms
{
};

// Function TgClient.TgHUD_Game.ClearTriggeredSceneInfo
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventClearTriggeredSceneInfo_Parms
{
};

// Function TgClient.TgHUD_Game.SetTriggeredSceneInfo
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventSetTriggeredSceneInfo_Parms
{
	class AActor*                                      triggerActor;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.CycleTrackedAssignment
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execCycleTrackedAssignment_Parms
{
};

// Function TgClient.TgHUD_Game.ShowDevNotes
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execShowDevNotes_Parms
{
};

// Function TgClient.TgHUD_Game.BrowseToDevNotes
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execBrowseToDevNotes_Parms
{
};

// Function TgClient.TgHUD_Game.UpdateReleaseTime
// [0x00024002] 
struct ATgHUD_Game_execUpdateReleaseTime_Parms
{
	float                                              fReleaseTime;                                     		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	// class UTgUIReleaseMenus*                        relmenu;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.GetActiveReleaseMenu
// [0x00020002] 
struct ATgHUD_Game_execGetActiveReleaseMenu_Parms
{
	class UTgUIReleaseMenus*                           ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUIReleaseMenus*                        relmenu;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowReleaseDialog
// [0x00024002] 
struct ATgHUD_Game_execShowReleaseDialog_Parms
{
	unsigned long                                      bShow : 1;                                        		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	struct FDeathZoomInfo                              Info;                                             		// 0x0004 (0x001C) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	float                                              fReleaseTime;                                     		// 0x0020 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bCanManuallyRelease : 1;                          		// 0x0024 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	// class UTgUIReleaseMenus*                        relmenu;                                          		// 0x0028 (0x0004) [0x0000000000000000]              
	// class UTgUIScoreboard*                          scoreScene;                                       		// 0x002C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowDevSpawnBotMenu
// [0x00020A02] ( FUNC_Exec | FUNC_Event )
struct ATgHUD_Game_execShowDevSpawnBotMenu_Parms
{
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUIDevSpawnBot*                         sb;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ForceUpdateDeviceBar
// [0x00024002] 
struct ATgHUD_Game_execForceUpdateDeviceBar_Parms
{
	int                                                nActiveDeviceId;                                  		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.HideWorldInteractionText
// [0x00020002] 
struct ATgHUD_Game_execHideWorldInteractionText_Parms
{
};

// Function TgClient.TgHUD_Game.ShowWorldInteractionTextById
// [0x00020002] 
struct ATgHUD_Game_execShowWorldInteractionTextById_Parms
{
	int                                                nMsgId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ShowWorldInteractionText
// [0x00020002] 
struct ATgHUD_Game_execShowWorldInteractionText_Parms
{
	struct FString                                     sMsg;                                             		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_Game.HideProgressBar
// [0x00024002] 
struct ATgHUD_Game_execHideProgressBar_Parms
{
	unsigned long                                      bWorldInteractionBar : 1;                         		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.StartProgressBar
// [0x00024002] 
struct ATgHUD_Game_execStartProgressBar_Parms
{
	float                                              fDuration;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     sLabel;                                           		// 0x0004 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      bWorldInteractionBar : 1;                         		// 0x0010 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.DisplayBadConnectionAlert
// [0x00020002] 
struct ATgHUD_Game_execDisplayBadConnectionAlert_Parms
{
	// class ATgPlayerController*                      PC;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.HideTaskForce
// [0x00020002] 
struct ATgHUD_Game_execHideTaskForce_Parms
{
};

// Function TgClient.TgHUD_Game.ShowTaskForce
// [0x00020002] 
struct ATgHUD_Game_execShowTaskForce_Parms
{
};

// Function TgClient.TgHUD_Game.SetReticuleVisibility
// [0x00020002] 
struct ATgHUD_Game_execSetReticuleVisibility_Parms
{
	unsigned long                                      bCrossHairVisible : 1;                            		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bWeaponStatusVisible : 1;                         		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.Tick
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.NativeTick
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execNativeTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.PlayZoomBlur
// [0x00020002] 
struct ATgHUD_Game_execPlayZoomBlur_Parms
{
};

// Function TgClient.TgHUD_Game.PlayZoomEffect
// [0x00024002] 
struct ATgHUD_Game_execPlayZoomEffect_Parms
{
	unsigned long                                      bEnable : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned char                                      ZoomType;                                         		// 0x0004 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.GetZoomEffectByAspectRatio
// [0x00820002] 
struct ATgHUD_Game_execGetZoomEffectByAspectRatio_Parms
{
	class UMaterialInstanceConstant*                   ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// struct FVector2D                                ViewportSize;                                     		// 0x0004 (0x0008) [0x0000000000000000]              
	// class ATgPlayerController*                      PC;                                               		// 0x000C (0x0004) [0x0000000000000000]              
	// class UTgGameViewportClient*                    Viewport;                                         		// 0x0010 (0x0004) [0x0000000000000000]              
	// float                                           AspectRatio;                                      		// 0x0014 (0x0004) [0x0000000000000000]              
	// class UMaterialInstanceConstant*                baseZoom;                                         		// 0x0018 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.SetZoomScope
// [0x00020002] 
struct ATgHUD_Game_execSetZoomScope_Parms
{
	struct FName                                       ScopeName;                                        		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.DisplayDamage
// [0x00020002] 
struct ATgHUD_Game_execDisplayDamage_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             XL;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
	// int                                             nIconSize;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	// int                                             nTotalIcons;                                      		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.UpdateDamage
// [0x00020002] 
struct ATgHUD_Game_execUpdateDamage_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.CalcHitDirection
// [0x00820002] 
struct ATgHUD_Game_execCalcHitDirection_Parms
{
	struct FVector                                     HitDir;                                           		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	float                                              ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// struct FVector                                  Loc;                                              		// 0x0010 (0x000C) [0x0000000000000000]              
	// struct FRotator                                 Rot;                                              		// 0x001C (0x000C) [0x0000000000000000]              
	// float                                           DirOfHit;                                         		// 0x0028 (0x0004) [0x0000000000000000]              
	// struct FVector                                  AxisX;                                            		// 0x002C (0x000C) [0x0000000000000000]              
	// struct FVector                                  AxisY;                                            		// 0x0038 (0x000C) [0x0000000000000000]              
	// struct FVector                                  AxisZ;                                            		// 0x0044 (0x000C) [0x0000000000000000]              
	// struct FVector                                  ShotDirection;                                    		// 0x0050 (0x000C) [0x0000000000000000]              
	// unsigned long                                   bIsInFront : 1;                                   		// 0x005C (0x0004) [0x0000000000000000] [0x00000001] 
	// struct FVector2D                                AngularDist;                                      		// 0x0060 (0x0008) [0x0000000000000000]              
	// float                                           PositionInQuadrant;                               		// 0x0068 (0x0004) [0x0000000000000000]              
	// float                                           Multiplier;                                       		// 0x006C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.PlayHitEffect
// [0x00020002] 
struct ATgHUD_Game_execPlayHitEffect_Parms
{
	float                                              Damage;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FVector                                     HitDir;                                           		// 0x0004 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	// class ATgPawn*                                  LocalPlayer;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
	// unsigned long                                   PlayerIsZoomed : 1;                               		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
};

// Function TgClient.TgHUD_Game.RadarMapRotates
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execRadarMapRotates_Parms
{
	unsigned long                                      RotatingMapStyle : 1;                             		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.LogBreak
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execLogBreak_Parms
{
	int                                                I;                                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.SelectPreviousDevice
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execSelectPreviousDevice_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.SelectNextDevice
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execSelectNextDevice_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.UpdateSelectedDevice
// [0x00020102] 
struct ATgHUD_Game_execUpdateSelectedDevice_Parms
{
	class ATgDevice*                                   Dev;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.UsePickupDevice
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execUsePickupDevice_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.SelectDeviceBarType
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execSelectDeviceBarType_Parms
{
	unsigned char                                      eType;                                            		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.SelectDeviceBarSlot
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execSelectDeviceBarSlot_Parms
{
	int                                                Slot;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ToggleDrawAllHUD
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execToggleDrawAllHUD_Parms
{
	// class ATgPlayerController*                      PC;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
	// class UTgUIReleaseMenus*                        relmenu;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ToggleDrawPawnHUD
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execToggleDrawPawnHUD_Parms
{
};

// Function TgClient.TgHUD_Game.ShouldDrawPawnHUD
// [0x00020002] 
struct ATgHUD_Game_execShouldDrawPawnHUD_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ToggleTargetExtraInfo
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execToggleTargetExtraInfo_Parms
{
};

// Function TgClient.TgHUD_Game.ShowRaidTab
// [0x00024802] ( FUNC_Event )
struct ATgHUD_Game_eventShowRaidTab_Parms
{
	int                                                nTab;                                             		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.CheckLevelUnlocks
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventCheckLevelUnlocks_Parms
{
	unsigned long                                      bIsTutorial : 1;                                  		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	// class UTgUIUnlockScene*                         unlockScene;                                      		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.OpenIntroScene
// [0x00024802] ( FUNC_Event )
struct ATgHUD_Game_eventOpenIntroScene_Parms
{
	float                                              FadeTime;                                         		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	float                                              TotalTime;                                        		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	class UUIScene*                                    ReturnValue;                                      		// 0x0008 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUIIntroScene*                          introScene;                                       		// 0x000C (0x0004) [0x0000000000000000]              
	// class UUIScene*                                 OpenedScene;                                      		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.TgFxLightsOn
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execTgFxLightsOn_Parms
{
	unsigned long                                      bOn : 1;                                          		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.OpenDevMenu
// [0x00020A02] ( FUNC_Exec | FUNC_Event )
struct ATgHUD_Game_execOpenDevMenu_Parms
{
	// class UTgUIDevInGame*                           devScr;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.LiveTeamPerfStats
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execLiveTeamPerfStats_Parms
{
	// class ATgPlayerController*                      PC;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.IsForValidPlayer
// [0x00020002] 
struct ATgHUD_Game_execIsForValidPlayer_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgPlayerController*                      PC;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
	// class ATgPawn*                                  localPawn;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowEndRaidScreen
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventShowEndRaidScreen_Parms
{
	unsigned long                                      bAttackerWon : 1;                                 		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	// class UTgUIEndRaidScreen*                       endRaidScene;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	// class UTgUIScoreboard*                          scoreScene;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowQuestEndMissionScene
// [0x00020002] 
struct ATgHUD_Game_execShowQuestEndMissionScene_Parms
{
	unsigned long                                      MissionVictor : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      PlayerVictor : 1;                                 		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bTie : 1;                                         		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	// class UTgUIEndQuestMission*                     endMissionScene;                                  		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowEndMissionScreen
// [0x00020002] 
struct ATgHUD_Game_execShowEndMissionScreen_Parms
{
	unsigned long                                      MissionVictor : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      PlayerVictor : 1;                                 		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bTie : 1;                                         		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	// class UTgUIEndMissionScreen*                    endMissionScene;                                  		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.UpdateScoreBoard
// [0x00020002] 
struct ATgHUD_Game_execUpdateScoreBoard_Parms
{
	unsigned long                                      bPlayerVictor : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bTie : 1;                                         		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.CloseConfirmPopup
// [0x00024802] ( FUNC_Event )
struct ATgHUD_Game_eventCloseConfirmPopup_Parms
{
	class UFunction*                                   callbackPtr;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bTellGameClient : 1;                              		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	// class UTgUIConfirmPopup*                        confirmPopup;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.CreateConfirmPopup
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventCreateConfirmPopup_Parms
{
	struct FString                                     Message;                                          		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	class UFunction*                                   callbackPtr;                                      		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// class UTgUIConfirmPopup*                        confirmPopup;                                     		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ViewItemPreview
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventViewItemPreview_Parms
{
	class UTgInventoryObject*                          Item;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUIPreviewScreen*                          ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUIPreviewScreen*                       previewPopup;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ToggleMainMenuScreen
// [0x00020A02] ( FUNC_Exec | FUNC_Event )
struct ATgHUD_Game_execToggleMainMenuScreen_Parms
{
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.OpenSkillScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenSkillScreen_Parms
{
	// class ATgPlayerController*                      PC;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
	// class ATgPawn*                                  localPawn;                                        		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.OpenInventoryLog
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenInventoryLog_Parms
{
};

// Function TgClient.TgHUD_Game.OpenItemModScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenItemModScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenAgentScreen
// [0x00024202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenAgentScreen_Parms
{
	unsigned long                                      bIsTutorial : 1;                                  		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	// class ATgPlayerController*                      PC;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
	// class ATgPawn*                                  localPawn;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	// class UTgUIAgentProfile_Equip*                  profilescene;                                     		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowGametypeInfo
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execShowGametypeInfo_Parms
{
};

// Function TgClient.TgHUD_Game.OpenLFGScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenLFGScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenZoneMissions
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenZoneMissions_Parms
{
};

// Function TgClient.TgHUD_Game.OpenAgencyDirectory
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenAgencyDirectory_Parms
{
};

// Function TgClient.TgHUD_Game.OpenMailScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenMailScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenAuctionScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenAuctionScreen_Parms
{
};

// Function TgClient.TgHUD_Game.MainMenuDisplayed
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventMainMenuDisplayed_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUISceneDriver*                         drv;                                              		// 0x0004 (0x0004) [0x0000000000000000]              
	// unsigned long                                   bDisplayed : 1;                                   		// 0x0008 (0x0004) [0x0000000000000000] [0x00000001] 
};

// Function TgClient.TgHUD_Game.OpenPlayerSearchPrepopulatedById
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventOpenPlayerSearchPrepopulatedById_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// class UTgUITeamMenu*                            teamMenu;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.OpenPlayerSearchPrepopulated
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventOpenPlayerSearchPrepopulated_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sAgencyName;                                      		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sAllianceName;                                    		// 0x0018 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	// class UTgUITeamMenu*                            teamMenu;                                         		// 0x0024 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.SelectAgentTab
// [0x00024802] ( FUNC_Event )
struct ATgHUD_Game_eventSelectAgentTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	class ATgPawn_Character*                           InspectedPawn;                                    		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0008 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// unsigned char                                   menuFunc;                                         		// 0x000C (0x0001) [0x0000000000000000]              
	// struct FName                                    sceneName;                                        		// 0x0010 (0x0008) [0x0000000000000000]              
	// class UTgUIAgentProfile*                        ag;                                               		// 0x0018 (0x0004) [0x0000000000000000]              
	// unsigned long                                   bInspectorMode : 1;                               		// 0x001C (0x0004) [0x0000000000000000] [0x00000001] 
};

// Function TgClient.TgHUD_Game.SelectAgencyTab
// [0x00024802] ( FUNC_Event )
struct ATgHUD_Game_eventSelectAgencyTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	// unsigned char                                   menuFunc;                                         		// 0x0001 (0x0001) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ZoneMap
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execZoneMap_Parms
{
};

// Function TgClient.TgHUD_Game.MissionLog
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execMissionLog_Parms
{
};

// Function TgClient.TgHUD_Game.OpenAgencyMenu
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenAgencyMenu_Parms
{
};

// Function TgClient.TgHUD_Game.OpenNavScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenNavScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenCharacterSkillScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenCharacterSkillScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenCharacterStatsScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenCharacterStatsScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenTeamDetailScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenTeamDetailScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenInventoryScreen
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenInventoryScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenInstanceSelect
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenInstanceSelect_Parms
{
};

// Function TgClient.TgHUD_Game.OpenAchievementsNew
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenAchievementsNew_Parms
{
};

// Function TgClient.TgHUD_Game.OpenProductionPopup
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventOpenProductionPopup_Parms
{
	class UTgUISceneDriver*                            ParentScene;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUIProductionPopup*                        ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUIProductionPopup*                     productionPopup;                                  		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.OpenFilteredInventoryPopup
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventOpenFilteredInventoryPopup_Parms
{
	class UTgUISceneDriver*                            ParentScene;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUIInventoryPopup*                         ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTgUIInventoryPopup*                      inventoryPopup;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.OpenHexAuction
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenHexAuction_Parms
{
	int                                                nHexId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.SelectCampaignTab
// [0x00024802] ( FUNC_Event )
struct ATgHUD_Game_eventSelectCampaignTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	// unsigned char                                   menuFunc;                                         		// 0x0001 (0x0001) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.OpenCombatAlerts
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenCombatAlerts_Parms
{
};

// Function TgClient.TgHUD_Game.OpenHexMap
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenHexMap_Parms
{
};

// Function TgClient.TgHUD_Game.OpenHexLeaderboard
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execOpenHexLeaderboard_Parms
{
};

// Function TgClient.TgHUD_Game.TestSetHexBackground
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestSetHexBackground_Parms
{
	int                                                nResourceId;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// class UTgUICampaignMenu_HexMap*                 hexmap;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.ShowDeviceBarHUD
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execShowDeviceBarHUD_Parms
{
	unsigned long                                      Show : 1;                                         		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.GetInputKeyForCommand
// [0x00020002] 
struct ATgHUD_Game_execGetInputKeyForCommand_Parms
{
	class UPlayerInput*                                Input;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     CommandName;                                      		// 0x0004 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FName                                       ReturnValue;                                      		// 0x0010 (0x0008) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             BindIndex;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.HUDLocalGetBind
// [0x00020002] 
struct ATgHUD_Game_execHUDLocalGetBind_Parms
{
	class UPlayerInput*                                Input;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FName                                       BindName;                                         		// 0x0004 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     ReturnValue;                                      		// 0x000C (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
	// int                                             BindIndex;                                        		// 0x0018 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.GetAmountCurrentlyOffOfTargetAccuracy
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventGetAmountCurrentlyOffOfTargetAccuracy_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgPlayerController*                      PC;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
	// class ATgDevice*                                Dev;                                              		// 0x0008 (0x0004) [0x0000000000000000]              
	// float                                           Delta;                                            		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.GetCurrentDeviceAccuracy
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventGetCurrentDeviceAccuracy_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgPlayerController*                      PC;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
	// class ATgDevice*                                Dev;                                              		// 0x0008 (0x0004) [0x0000000000000000]              
	// float                                           Accuracy;                                         		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.FormatTime
// [0x00020002] 
struct ATgHUD_Game_execFormatTime_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
	// int                                             Minutes;                                          		// 0x000C (0x0004) [0x0000000000000000]              
	// int                                             Hours;                                            		// 0x0010 (0x0004) [0x0000000000000000]              
	// int                                             Seconds;                                          		// 0x0014 (0x0004) [0x0000000000000000]              
	// struct FString                                  Result;                                           		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_Game.ScaleY
// [0x00020003] ( FUNC_Final )
struct ATgHUD_Game_execScaleY_Parms
{
	float                                              Y;                                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ScaleX
// [0x00020003] ( FUNC_Final )
struct ATgHUD_Game_execScaleX_Parms
{
	float                                              X;                                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.InRange
// [0x00020002] 
struct ATgHUD_Game_execInRange_Parms
{
	float                                              X;                                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              lower;                                            		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              upper;                                            		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x000C (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.SizeInt
// [0x00C20102] 
struct ATgHUD_Game_execSizeInt_Parms
{
	int                                                Value;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              XL;                                               		// 0x0004 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	float                                              YL;                                               		// 0x0008 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	float                                              Scale;                                            		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// struct FString                                  S;                                                		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// int                                             I;                                                		// 0x001C (0x0004) [0x0000000000000000]              
	// int                                             V;                                                		// 0x0020 (0x0004) [0x0000000000000000]              
	// int                                             M;                                                		// 0x0024 (0x0004) [0x0000000000000000]              
	// struct FDigitData                               D;                                                		// 0x0028 (0x0010) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.DrawInt
// [0x00820102] 
struct ATgHUD_Game_execDrawInt_Parms
{
	int                                                Value;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              X;                                                		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              Y;                                                		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              Scale;                                            		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// struct FString                                  S;                                                		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// int                                             I;                                                		// 0x001C (0x0004) [0x0000000000000000]              
	// int                                             V;                                                		// 0x0020 (0x0004) [0x0000000000000000]              
	// struct FDigitData                               D;                                                		// 0x0024 (0x0010) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.DrawSpectatorHud
// [0x00020102] 
struct ATgHUD_Game_execDrawSpectatorHud_Parms
{
	// struct FString                                  A;                                                		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// struct FString                                  B;                                                		// 0x000C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// float                                           XL;                                               		// 0x0018 (0x0004) [0x0000000000000000]              
	// float                                           YL;                                               		// 0x001C (0x0004) [0x0000000000000000]              
	// float                                           X;                                                		// 0x0020 (0x0004) [0x0000000000000000]              
	// float                                           Y;                                                		// 0x0024 (0x0004) [0x0000000000000000]              
	// float                                           W;                                                		// 0x0028 (0x0004) [0x0000000000000000]              
	// float                                           H;                                                		// 0x002C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.DrawTargetText
// [0x00020002] 
struct ATgHUD_Game_execDrawTargetText_Parms
{
	struct FString                                     Str;                                              		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_Game.DrawHoverText
// [0x00020002] 
struct ATgHUD_Game_execDrawHoverText_Parms
{
	struct FString                                     Str;                                              		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_Game.DrawHealth
// [0x00020102] 
struct ATgHUD_Game_execDrawHealth_Parms
{
	// float                                           LeftPos;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
	// float                                           TopPos;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	// float                                           Width;                                            		// 0x0008 (0x0004) [0x0000000000000000]              
	// float                                           Height;                                           		// 0x000C (0x0004) [0x0000000000000000]              
	// int                                             H;                                                		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.DrawTextPanel
// [0x00020102] 
struct ATgHUD_Game_execDrawTextPanel_Parms
{
	// int                                             TopPos;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             LeftPos;                                          		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.AddNewDisplayMessage
// [0x00020002] 
struct ATgHUD_Game_execAddNewDisplayMessage_Parms
{
	struct FString                                     sMessage;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_Game.AddNewKilledTarget
// [0x00020002] 
struct ATgHUD_Game_execAddNewKilledTarget_Parms
{
	struct FString                                     KilledName;                                       		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     KillerName;                                       		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      KillerWasPlayer : 1;                              		// 0x0018 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.AddConsoleMessage
// [0x00024400] ( FUNC_Native )
struct ATgHUD_Game_execAddConsoleMessage_Parms
{
	struct FString                                     M;                                                		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	class UClass*                                      InMessageClass;                                   		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class APlayerReplicationInfo*                      PRI;                                              		// 0x0010 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              Lifetime;                                         		// 0x0014 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.DisplayConsoleMessages
// [0x00020000] 
struct ATgHUD_Game_execDisplayConsoleMessages_Parms
{
};

// Function TgClient.TgHUD_Game.CloseGUIItems
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventCloseGUIItems_Parms
{
};

// Function TgClient.TgHUD_Game.OpenGUIItems
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventOpenGUIItems_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayDebug
// [0x00420100] 
struct ATgHUD_Game_execDisplayDebug_Parms
{
	class AHUD*                                        HUD;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              out_YL;                                           		// 0x0004 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	float                                              out_YPos;                                         		// 0x0008 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
};

// Function TgClient.TgHUD_Game.CancelActiveCombat
// [0x00020002] 
struct ATgHUD_Game_execCancelActiveCombat_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayActiveCombat
// [0x00024002] 
struct ATgHUD_Game_execDisplayActiveCombat_Parms
{
	int                                                nStateParam;                                      		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.CancelBeingDetected
// [0x00020002] 
struct ATgHUD_Game_execCancelBeingDetected_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayBeingDetected
// [0x00024002] 
struct ATgHUD_Game_execDisplayBeingDetected_Parms
{
	int                                                nStateParam;                                      		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.CancelBeingTargeted
// [0x00020002] 
struct ATgHUD_Game_execCancelBeingTargeted_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayBeingTargeted
// [0x00020002] 
struct ATgHUD_Game_execDisplayBeingTargeted_Parms
{
};

// Function TgClient.TgHUD_Game.CancelInBombRange
// [0x00020002] 
struct ATgHUD_Game_execCancelInBombRange_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayInBombRange
// [0x00020002] 
struct ATgHUD_Game_execDisplayInBombRange_Parms
{
};

// Function TgClient.TgHUD_Game.CancelBeingLocked
// [0x00020002] 
struct ATgHUD_Game_execCancelBeingLocked_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayBeingLocked
// [0x00020002] 
struct ATgHUD_Game_execDisplayBeingLocked_Parms
{
};

// Function TgClient.TgHUD_Game.CancelBodyUnderAttack
// [0x00020002] 
struct ATgHUD_Game_execCancelBodyUnderAttack_Parms
{
};

// Function TgClient.TgHUD_Game.DisplayBodyUnderAttack
// [0x00020002] 
struct ATgHUD_Game_execDisplayBodyUnderAttack_Parms
{
	int                                                Damage;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// float                                           iDam;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.DisplayHit
// [0x00020002] 
struct ATgHUD_Game_execDisplayHit_Parms
{
	struct FVector                                     HitDir;                                           		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	int                                                Damage;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UClass*                                      DamageType;                                       		// 0x0010 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.PostRender
// [0x00020802] ( FUNC_Event )
struct ATgHUD_Game_eventPostRender_Parms
{
	// class UUISceneClient*                           SceneClient;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.PostRenderScenes
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execPostRenderScenes_Parms
{
};

// Function TgClient.TgHUD_Game.PostRenderSceneTooltips
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execPostRenderSceneTooltips_Parms
{
};

// Function TgClient.TgHUD_Game.GetTgUISceneDriver
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execGetTgUISceneDriver_Parms
{
	class UUIScene*                                    Scene;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UObject*                                     ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.TriggerCloseTgUIScene
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execTriggerCloseTgUIScene_Parms
{
	class UUIScene*                                    Scene;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.TriggerOpenTgUIScene
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execTriggerOpenTgUIScene_Parms
{
	class UUIScene*                                    Scene;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIScene*                                    ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.InitializeTgUIScenes
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execInitializeTgUIScenes_Parms
{
};

// Function TgClient.TgHUD_Game.PostBeginPlay
// [0x00020102] 
struct ATgHUD_Game_execPostBeginPlay_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.DrawCommandInfo
// [0x00020002] 
struct ATgHUD_Game_execDrawCommandInfo_Parms
{
	class ATgPlayerController*                         PC;                                               		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// struct FString                                  sCommand;                                         		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_Game.DrawHUD
// [0x00020002] 
struct ATgHUD_Game_execDrawHUD_Parms
{
	// class ATgPlayerController*                      PC;                                               		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_Game.GetTgGameViewportClient
// [0x00020002] 
struct ATgHUD_Game_execGetTgGameViewportClient_Parms
{
	class UTgGameViewportClient*                       ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.GetMouseHitNormal
// [0x00020002] 
struct ATgHUD_Game_execGetMouseHitNormal_Parms
{
	struct FVector                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.GetMouseHoverLocation
// [0x00020002] 
struct ATgHUD_Game_execGetMouseHoverLocation_Parms
{
	struct FVector                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.TestAgencyRemoveItem
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestAgencyRemoveItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nItemId;                                          		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestAgencyAddItem
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestAgencyAddItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestAgencyUpdateCurrency
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestAgencyUpdateCurrency_Parms
{
	int                                                nCurrencyChange;                                  		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestStrikeForceRemoveItem
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestStrikeForceRemoveItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nTaskTeam;                                        		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestStrikeForceAddItem
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestStrikeForceAddItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nItemId;                                          		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nTaskTeam;                                        		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestRemoveFactoryItem
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestRemoveFactoryItem_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nQueueId;                                         		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nCount;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestAddFactoryItem
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestAddFactoryItem_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInvId;                                           		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nCount;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.TestAddFacility
// [0x00020202] ( FUNC_Exec )
struct ATgHUD_Game_execTestAddFacility_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInvId;                                           		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.CheckStorePrompt
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execCheckStorePrompt_Parms
{
};

// Function TgClient.TgHUD_Game.InitQuestVisuals
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execInitQuestVisuals_Parms
{
};

// Function TgClient.TgHUD_Game.ClientTestAgencyUpdateCurrency
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestAgencyUpdateCurrency_Parms
{
	int                                                nCurrencyChange;                                  		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestAgencyRemoveItem
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestAgencyRemoveItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nItemId;                                          		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestAgencyAddItem
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestAgencyAddItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestRemoveFactoryItem
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestRemoveFactoryItem_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nQueueId;                                         		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nCount;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestAddFactoryItem
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestAddFactoryItem_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInvId;                                           		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nCount;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestAddFacility
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestAddFacility_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInvId;                                           		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestStrikeForceRemoveItem
// [0x00024400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestStrikeForceRemoveItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nTaskTeam;                                        		// 0x0008 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.ClientTestStrikeForceAddItem
// [0x00024400] ( FUNC_Native )
struct ATgHUD_Game_execClientTestStrikeForceAddItem_Parms
{
	int                                                nInvId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nItemId;                                          		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nInstanceCount;                                   		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nTaskTeam;                                        		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.UpdateRangedTargeterState
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateRangedTargeterState_Parms
{
	unsigned char                                      eNewState;                                        		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.UpdateMouseVector
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateMouseVector_Parms
{
};

// Function TgClient.TgHUD_Game.RevealHiddenRewardScreen
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execRevealHiddenRewardScreen_Parms
{
};

// Function TgClient.TgHUD_Game.DismissRewardScreen
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execDismissRewardScreen_Parms
{
};

// Function TgClient.TgHUD_Game.SetRewardScreenReveal
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execSetRewardScreenReveal_Parms
{
	float                                              fSeconds;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.SetRewardScreenAutoDismiss
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execSetRewardScreenAutoDismiss_Parms
{
	float                                              fSeconds;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.IsRewardScreenOpen
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execIsRewardScreenOpen_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.OpenNextRewardScreen
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execOpenNextRewardScreen_Parms
{
};

// Function TgClient.TgHUD_Game.GetControllerAndViewport
// [0x00420400] ( FUNC_Native )
struct ATgHUD_Game_execGetControllerAndViewport_Parms
{
	class ATgPlayerController*                         pController;                                      		// 0x0000 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	class UTgGameViewportClient*                       pViewport;                                        		// 0x0004 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.GetMapTransition
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execGetMapTransition_Parms
{
	class UTgUIMapTransition*                          ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ToggleMapNameDisplay
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execToggleMapNameDisplay_Parms
{
};

// Function TgClient.TgHUD_Game.SetDepthTestingEnabled
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execSetDepthTestingEnabled_Parms
{
	class UCanvas*                                     pCanvas;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bEnable : 1;                                      		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.DrawObjectiveOverlays
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execDrawObjectiveOverlays_Parms
{
	struct FVector                                     CameraPosition;                                   		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	struct FVector                                     CameraDir;                                        		// 0x000C (0x000C) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.DrawActorOverlays
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execDrawActorOverlays_Parms
{
};

// Function TgClient.TgHUD_Game.ResubmitPopups
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execResubmitPopups_Parms
{
};

// Function TgClient.TgHUD_Game.CheckShouldSkipEndMission
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execCheckShouldSkipEndMission_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.HideObjectiveNames
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execHideObjectiveNames_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.HideObjectives
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execHideObjectives_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.HideEnemyTags
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execHideEnemyTags_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ShowCombatText
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execShowCombatText_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ShowDeployableHealthBars
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execShowDeployableHealthBars_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.CheckDisableWorldRendering
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execCheckDisableWorldRendering_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.OpenTeamScreen
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execOpenTeamScreen_Parms
{
};

// Function TgClient.TgHUD_Game.OpenManagedSceneByID
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execOpenManagedSceneByID_Parms
{
	unsigned char                                      funcID;                                           		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ToggleManagedSceneByID
// [0x00024600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execToggleManagedSceneByID_Parms
{
	unsigned char                                      funcID;                                           		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bOpenedFromUIVolume : 1;                          		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0008 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ToggleManagedScene
// [0x00024600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execToggleManagedScene_Parms
{
	struct FName                                       ClassName;                                        		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bOpenedFromUIVolume : 1;                          		// 0x0008 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgHUD_Game.ChatCycleTab
// [0x00024600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatCycleTab_Parms
{
	unsigned long                                      Increment : 1;                                    		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.ChatOpacity
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatOpacity_Parms
{
	float                                              ChatOpacityIn;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ChatEnd
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatEnd_Parms
{
};

// Function TgClient.TgHUD_Game.ChatPageDown
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatPageDown_Parms
{
	unsigned long                                      Active : 1;                                       		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ChatPageUp
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatPageUp_Parms
{
	unsigned long                                      Active : 1;                                       		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ChatToggleMode
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatToggleMode_Parms
{
};

// Function TgClient.TgHUD_Game.ChatToggleOnTop
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatToggleOnTop_Parms
{
};

// Function TgClient.TgHUD_Game.ChatStartReply
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatStartReply_Parms
{
};

// Function TgClient.TgHUD_Game.ChatStartSlash
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatStartSlash_Parms
{
};

// Function TgClient.TgHUD_Game.ChatStartDefault
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execChatStartDefault_Parms
{
};

// Function TgClient.TgHUD_Game.FreemouseOnHUD
// [0x00020600] ( FUNC_Exec | FUNC_Native )
struct ATgHUD_Game_execFreemouseOnHUD_Parms
{
	unsigned long                                      bFree : 1;                                        		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.GetStringDimensions
// [0x00420400] ( FUNC_Native )
struct ATgHUD_Game_execGetStringDimensions_Parms
{
	class UFont*                                       fontToUse;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     checkString;                                      		// 0x0004 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                StringWidth;                                      		// 0x0010 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	int                                                StringHeight;                                     		// 0x0014 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
};

// Function TgClient.TgHUD_Game.UpdateUITextBox
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateUITextBox_Parms
{
	unsigned char                                      TextBoxBitfield;                                  		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	float                                              Duration;                                         		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                messageID;                                        		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.UpdateUITimer
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateUITimer_Parms
{
	unsigned char                                      ClockStateBitfield;                               		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	float                                              TimerDuration;                                    		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.InitializeVoiceMacroTree
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execInitializeVoiceMacroTree_Parms
{
};

// Function TgClient.TgHUD_Game.InitializeCommonDataStores
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execInitializeCommonDataStores_Parms
{
};

// Function TgClient.TgHUD_Game.NativePostBeginPlay
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execNativePostBeginPlay_Parms
{
};

// Function TgClient.TgHUD_Game.UseCrosshairForReticule
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUseCrosshairForReticule_Parms
{
	struct FName                                       CrosshairName;                                    		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.ResetDeviceHudTimersUI
// [0x00024400] ( FUNC_Native )
struct ATgHUD_Game_execResetDeviceHudTimersUI_Parms
{
	int                                                DeviceEqpPoint;                                   		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.UpdateInHandDevicesUI
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateInHandDevicesUI_Parms
{
	class ATgDevice*                                   pDevice;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.UpdateHiddenDevicesUI
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateHiddenDevicesUI_Parms
{
};

// Function TgClient.TgHUD_Game.UpdateDeviceBarUIItems
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateDeviceBarUIItems_Parms
{
};

// Function TgClient.TgHUD_Game.UpdateModeEquipTimer
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateModeEquipTimer_Parms
{
	int                                                nIsActive;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fEquipTime;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgHUD_Game.UpdateDeviceSlotTimers
// [0x00024400] ( FUNC_Native )
struct ATgHUD_Game_execUpdateDeviceSlotTimers_Parms
{
	int                                                nDeviceInstanceId;                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nDeviceMode;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fSetup;                                           		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fRecharge;                                        		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fElaspedCooldown;                                 		// 0x0010 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgHUD_Game.TriggerDeviceRefresh
// [0x00020400] ( FUNC_Native )
struct ATgHUD_Game_execTriggerDeviceRefresh_Parms
{
};

// Function TgClient.TgLoginHUD.ShowDevNotes
// [0x00020202] ( FUNC_Exec )
struct ATgLoginHUD_execShowDevNotes_Parms
{
};

// Function TgClient.TgLoginHUD.BrowseToDevNotes
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execBrowseToDevNotes_Parms
{
};

// Function TgClient.TgLoginHUD.BackToLogin
// [0x00020802] ( FUNC_Event )
struct ATgLoginHUD_eventBackToLogin_Parms
{
	// class UTgUICreateCharA*                         ccmenu;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	// class UTgUICustomHeadMenu*                      chmenu;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	// class UTgUICharacterSelectScene*                csmenu;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	// class UTgUIMapTransition*                       mtmenu;                                           		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgLoginHUD.SendCharCreateErrorCode
// [0x00020802] ( FUNC_Event )
struct ATgLoginHUD_eventSendCharCreateErrorCode_Parms
{
	int                                                ErrorCode;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// class UTgUICustomHeadMenu*                      chmenu;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgLoginHUD.CreateCharacter
// [0x00820802] ( FUNC_Event )
struct ATgLoginHUD_eventCreateCharacter_Parms
{
	// class UTgUICustomHeadMenu*                      chmenu;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	// unsigned long                                   bInvalid : 1;                                     		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
	// struct FCharacterInfoStruct                     charInfoStruct;                                   		// 0x0008 (0x0038) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgLoginHUD.ConditionalSetTutorialFlag
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execConditionalSetTutorialFlag_Parms
{
};

// Function TgClient.TgLoginHUD.LoadClassSelect
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execLoadClassSelect_Parms
{
	class UTgUIClassSelectScene*                       ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginHUD.LoadCustomHeadMenu
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execLoadCustomHeadMenu_Parms
{
	class UTgUICustomHeadMenu*                         ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginHUD.LoadCharCreateMenu
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execLoadCharCreateMenu_Parms
{
	class UTgUICreateCharA*                            ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginHUD.Tick
// [0x00020802] ( FUNC_Event )
struct ATgLoginHUD_eventTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgLoginHUD.NativeTick
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execNativeTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgLoginHUD.InitializeDataInterface
// [0x00020002] 
struct ATgLoginHUD_execInitializeDataInterface_Parms
{
};

// Function TgClient.TgLoginHUD.OpenMenu
// [0x00020002] 
struct ATgLoginHUD_execOpenMenu_Parms
{
	// class ATgLoginInfo*                             tgli;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	// class ATgLoginPlayerController*                 PC;                                               		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgLoginHUD.PostBeginPlay
// [0x00020102] 
struct ATgLoginHUD_execPostBeginPlay_Parms
{
};

// Function TgClient.TgLoginHUD.SetMatineeCamToLogin
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execSetMatineeCamToLogin_Parms
{
	unsigned long                                      bSetToLogin : 1;                                  		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgLoginHUD.LoadCharacterSelectMenu
// [0x00024400] ( FUNC_Native )
struct ATgLoginHUD_execLoadCharacterSelectMenu_Parms
{
	unsigned long                                      bError : 1;                                       		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgLoginHUD.LoadLoginMenu
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execLoadLoginMenu_Parms
{
};

// Function TgClient.TgLoginHUD.InitializeTgUIScenes
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execInitializeTgUIScenes_Parms
{
};

// Function TgClient.TgLoginHUD.GetEarthModel
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execGetEarthModel_Parms
{
	class AInterpActor*                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginHUD.GetCharacterBuilderSkelMeshActor
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execGetCharacterBuilderSkelMeshActor_Parms
{
	class ATgSkeletalMeshActor_CharacterBuilder*       ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginHUD.GetCharMorphSettings
// [0x00420400] ( FUNC_Native )
struct ATgLoginHUD_execGetCharMorphSettings_Parms
{
	struct FCharacterInfoStruct                        CharInfo;                                         		// 0x0000 (0x0038) [0x0000000000400180]              ( CPF_Parm | CPF_OutParm | CPF_NeedCtorLink )
};

// Function TgClient.TgLoginHUD.GetMapTransition
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execGetMapTransition_Parms
{
	class UTgUIMapTransition*                          ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginHUD.AddPlayerCharacterCallback
// [0x00020400] ( FUNC_Native )
struct ATgLoginHUD_execAddPlayerCharacterCallback_Parms
{
	int                                                eType;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgLoginInfo.PostBeginPlay
// [0x00020002] 
struct ATgLoginInfo_execPostBeginPlay_Parms
{
};

// Function TgClient.TgLoginInfo.ShouldSkipLoginHUD
// [0x00020400] ( FUNC_Native )
struct ATgLoginInfo_execShouldSkipLoginHUD_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginInfo.HasSelectedCharacter
// [0x00020400] ( FUNC_Native )
struct ATgLoginInfo_execHasSelectedCharacter_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginInfo.IsInGame
// [0x00020400] ( FUNC_Native )
struct ATgLoginInfo_execIsInGame_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginInfo.IsLoggedIn
// [0x00020400] ( FUNC_Native )
struct ATgLoginInfo_execIsLoggedIn_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgLoginInfo.CreateDebugStructure
// [0x00020400] ( FUNC_Native )
struct ATgLoginInfo_execCreateDebugStructure_Parms
{
};

// Function TgClient.TgLoginPlayerController.StopAltFire
// [0x00024200] ( FUNC_Exec )
struct ATgLoginPlayerController_execStopAltFire_Parms
{
	unsigned char                                      FireModeNum;                                      		// 0x0000 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgLoginPlayerController.StartAltFire
// [0x00024200] ( FUNC_Exec )
struct ATgLoginPlayerController_execStartAltFire_Parms
{
	unsigned char                                      FireModeNum;                                      		// 0x0000 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgLoginPlayerController.StartFire
// [0x00020200] ( FUNC_Exec )
struct ATgLoginPlayerController_execStartFire_Parms
{
};

// Function TgClient.TgLoginPlayerController.NextWeapon
// [0x00020200] ( FUNC_Exec )
struct ATgLoginPlayerController_execNextWeapon_Parms
{
};

// Function TgClient.TgLoginPlayerController.PrevWeapon
// [0x00020200] ( FUNC_Exec )
struct ATgLoginPlayerController_execPrevWeapon_Parms
{
};

// Function TgClient.TgLoginPlayerController.PostBeginPlay
// [0x00020102] 
struct ATgLoginPlayerController_execPostBeginPlay_Parms
{
};

// Function TgClient.TgLoginPlayerController.CheckAutoLogin
// [0x00020400] ( FUNC_Native )
struct ATgLoginPlayerController_execCheckAutoLogin_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgStatsData.Init
// [0x00020802] ( FUNC_Event )
struct UTgStatsData_eventInit_Parms
{
	class ATgHUD_Game*                                 HUD;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgStatsData.GetPlayerCount
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execGetPlayerCount_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgStatsData.GetMyPRI
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execGetMyPRI_Parms
{
	class ATgRepInfo_Player*                           ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgStatsData.PopulatePlayerRankArray
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execPopulatePlayerRankArray_Parms
{
};

// Function TgClient.TgStatsData.GetBestPlayerRankIndex
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execGetBestPlayerRankIndex_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgStatsData.SortScores
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execSortScores_Parms
{
	int                                                eSortField;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgStatsData.CopyScores
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execCopyScores_Parms
{
};

// Function TgClient.TgStatsData.SetGameOver
// [0x00020400] ( FUNC_Native )
struct UTgStatsData_execSetGameOver_Parms
{
	unsigned long                                      bGameover : 1;                                    		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bPlayerVictor : 1;                                		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUILevelMap.InitializeUniformScaleRadarMap
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execInitializeUniformScaleRadarMap_Parms
{
};

// Function TgClient.TgUILevelMap.ChangeDesiredMapOpacity
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execChangeDesiredMapOpacity_Parms
{
	unsigned long                                      Activate : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      Increase : 1;                                     		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUILevelMap.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execPostRender_Parms
{
};

// Function TgClient.TgUILevelMap.GenMapElements
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execGenMapElements_Parms
{
};

// Function TgClient.TgUILevelMap.DrawMapElements
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execDrawMapElements_Parms
{
	class UCanvas*                                     DrawCanvas;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILevelMap.UpdateAnimatingValues
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execUpdateAnimatingValues_Parms
{
	float                                              TimeDelay;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILevelMap.SetOpacity
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execSetOpacity_Parms
{
	float                                              Opacity;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILevelMap.Hide
// [0x00024400] ( FUNC_Native )
struct UTgUILevelMap_execHide_Parms
{
	unsigned long                                      Immediate : 1;                                    		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUILevelMap.Show
// [0x00024400] ( FUNC_Native )
struct UTgUILevelMap_execShow_Parms
{
	unsigned long                                      Immediate : 1;                                    		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUILevelMap.GetWorldTime
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execGetWorldTime_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILevelMap.TickLevelMap
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execTickLevelMap_Parms
{
	float                                              UpdateTime;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILevelMap.InitLevelMap
// [0x00020400] ( FUNC_Native )
struct UTgUILevelMap_execInitLevelMap_Parms
{
	class UMaterialInstanceConstant*                   LevelMapMIC;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class ATgHUD*                                      ParentTgHUD;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.SetSecondaryValue
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execSetSecondaryValue_Parms
{
	float                                              fSecondaryValue;                                  		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.SetSecondaryModeOn
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execSetSecondaryModeOn_Parms
{
	unsigned long                                      bSecondaryModeOn : 1;                             		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.ShowActiveText
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execShowActiveText_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.HideActiveText
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execHideActiveText_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.SetActiveTextById
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execSetActiveTextById_Parms
{
	int                                                nMsgId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.SetActiveText
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execSetActiveText_Parms
{
	struct FString                                     sMessage;                                         		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.UpdateProgressBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execUpdateProgressBar_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.ShowProgressBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execShowProgressBar_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.HideProgressBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execHideProgressBar_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.DeactivateProgressBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execDeactivateProgressBar_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.SetProgressBarText
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execSetProgressBarText_Parms
{
	struct FString                                     newProgressBarText;                               		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.StartProgressBar
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execStartProgressBar_Parms
{
	float                                              TimeDuration;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     ProgressLabel;                                    		// 0x0004 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_ActiveItem.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_ActiveItem_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.CheckQueueUpdate
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execCheckQueueUpdate_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.CheckPlayerUpdate
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execCheckPlayerUpdate_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.UpdatePositioning
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execUpdatePositioning_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.UpdatePlayerValues
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execUpdatePlayerValues_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.UpdateQueueValues
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execUpdateQueueValues_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.UpdateAgencyValues
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execUpdateAgencyValues_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_AgentInfo.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_AgentInfo_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BeaconStatus.UpdateBeaconDirection
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BeaconStatus_execUpdateBeaconDirection_Parms
{
	struct FVector                                     NewLoc;                                           		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BeaconStatus.GetLocalPlayerTaskForce
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BeaconStatus_execGetLocalPlayerTaskForce_Parms
{
	class ATgRepInfo_TaskForce*                        ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_BeaconStatus.UpdateBeaconHUDStatus
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BeaconStatus_execUpdateBeaconHUDStatus_Parms
{
	unsigned char                                      NewStatus;                                        		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BeaconStatus.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BeaconStatus_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BeaconStatus.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BeaconStatus_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.CheckBuffs
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execCheckBuffs_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.Reset
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execReset_Parms
{
	unsigned long                                      bHide : 1;                                        		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.AddBuff
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execAddBuff_Parms
{
	int                                                EffectID;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nIndex;                                           		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              TimeRemaining;                                    		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nNumStacks;                                       		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0010 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.UpdateBuffWidget
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execUpdateBuffWidget_Parms
{
	int                                                WidgetIndex;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_BuffBar.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_BuffBar_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Clock.UpdateTimer
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execUpdateTimer_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Clock.UpdateMissionTimer
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execUpdateMissionTimer_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Clock.Clear
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execClear_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Clock.SetTime
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execSetTime_Parms
{
	float                                              fTimeBase;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fTimeStarted;                                     		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bCountingDown : 1;                                		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Clock.SetStyle
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execSetStyle_Parms
{
	unsigned long                                      bUsingProgressBar : 1;                            		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bUsingAlarmImage : 1;                             		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Clock.UseAsMissionTimer
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execUseAsMissionTimer_Parms
{
	unsigned long                                      bIsMissionTimer : 1;                              		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Clock.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Clock.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Clock.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Clock_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_CombatAlert.UpdateLabels
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_CombatAlert_execUpdateLabels_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_CombatAlert.OnHexAssignmentUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_CombatAlert_execOnHexAssignmentUpdated_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_CombatAlert.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_CombatAlert_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_CombatAlert.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_CombatAlert_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.GetLocalPawn
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execGetLocalPawn_Parms
{
	class ATgPawn*                                     ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.UnregisterForAlerts
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execUnregisterForAlerts_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.RegisterForAlerts
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execRegisterForAlerts_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.InitAlertFade
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execInitAlertFade_Parms
{
	int                                                nAlertIndex;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.UpdateAlertFade
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execUpdateAlertFade_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.UpdateDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execUpdateDisplay_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.RemoveTimedOutAlerts
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execRemoveTimedOutAlerts_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.CancelAllAlerts
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execCancelAllAlerts_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.RemoveAlert
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execRemoveAlert_Parms
{
	int                                                nMsgId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.AddAlert
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execAddAlert_Parms
{
	unsigned char                                      Priority;                                         		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      Type;                                             		// 0x0001 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	float                                              fDuration;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nMsgId;                                           		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     sMessage;                                         		// 0x000C (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DangerBox.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DangerBox_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.GetSlotByEquipPoint
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execGetSlotByEquipPoint_Parms
{
	unsigned char                                      eqpPoint;                                         		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.GetSlotBySubType
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execGetSlotBySubType_Parms
{
	unsigned char                                      subtype;                                          		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.UpdateCooldown
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execUpdateCooldown_Parms
{
	int                                                nEquipPoint;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fStartTime;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              fDuration;                                        		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.HasInhandDevice
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execHasInhandDevice_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.SelectPreviousDevice
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execSelectPreviousDevice_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.SelectNextDevice
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execSelectNextDevice_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.SelectDeviceBarType
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execSelectDeviceBarType_Parms
{
	unsigned char                                      eType;                                            		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.SelectDeviceBarSlot
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execSelectDeviceBarSlot_Parms
{
	int                                                Slot;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.ResetHUDCooldowns
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execResetHUDCooldowns_Parms
{
	int                                                DeviceEqpPt;                                      		// 0x0000 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.UpdateKeyBindings
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execUpdateKeyBindings_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.RefreshUICooldowns
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execRefreshUICooldowns_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.GetActiveDeviceSlotString
// [0x00420400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execGetActiveDeviceSlotString_Parms
{
	struct FString                                     DeviceString;                                     		// 0x0000 (0x000C) [0x0000000000400180]              ( CPF_Parm | CPF_OutParm | CPF_NeedCtorLink )
	unsigned long                                      ReturnValue : 1;                                  		// 0x000C (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.UpdateSelectedDevice
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execUpdateSelectedDevice_Parms
{
	class ATgDevice*                                   Dev;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.ClearAllDeviceBarSlotWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execClearAllDeviceBarSlotWidgets_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.UserFlashOffhand
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execUserFlashOffhand_Parms
{
	int                                                WhichSlot;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.UserSelectedSlot
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execUserSelectedSlot_Parms
{
	int                                                WhichSlot;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.UpdateStatus
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execUpdateStatus_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.PopulateDeviceBarSlots
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execPopulateDeviceBarSlots_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.FindSpawnedPet
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execFindSpawnedPet_Parms
{
	class ATgPawn*                                     PlayerPawn;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class ATgDevice*                                   Dev;                                              		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class AReplicationInfo*                            ReturnValue;                                      		// 0x0008 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.TickTurretHealth
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execTickTurretHealth_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.TickVisuals
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execTickVisuals_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUIReticule*                               HUDReticule;                                      		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DeviceBar.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DeviceBar_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUIReticule*                               HUDReticule;                                      		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DialogQuery.SetDialogType
// [0x00020802] ( FUNC_Event )
struct UTgUIPrimaryHUD_DialogQuery_eventSetDialogType_Parms
{
	unsigned char                                      eType;                                            		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DialogQuery.HideDialog
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DialogQuery_execHideDialog_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_DialogQuery.ShowDialog
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DialogQuery_execShowDialog_Parms
{
	struct FString                                     sDialogText;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	float                                              TimeNow;                                          		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DialogQuery.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DialogQuery_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DialogQuery.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DialogQuery_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_DialogQuery.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_DialogQuery_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_KillDisplay.AddNewDisplayMessage
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_KillDisplay_execAddNewDisplayMessage_Parms
{
	struct FString                                     Message;                                          		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIPrimaryHUD_KillDisplay.AddNewKilledTarget
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_KillDisplay_execAddNewKilledTarget_Parms
{
	struct FString                                     Target;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     KillerName;                                       		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      KillerWasPlayer : 1;                              		// 0x0018 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_KillDisplay.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_KillDisplay_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_KillDisplay.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_KillDisplay_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_KillDisplay.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_KillDisplay_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MiniMap.Show
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MiniMap_execShow_Parms
{
	unsigned long                                      Immediate : 1;                                    		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MiniMap.Hide
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MiniMap_execHide_Parms
{
	unsigned long                                      Immediate : 1;                                    		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MiniMap.InitPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MiniMap_execInitPrimaryHUDElement_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_MiniMap.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MiniMap_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MiniMap.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MiniMap_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MiniMap.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MiniMap_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MissionInfo.RepositionWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MissionInfo_execRepositionWidgets_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_MissionInfo.PopulateMissionInfo
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MissionInfo_execPopulateMissionInfo_Parms
{
	unsigned long                                      bNoResetActive : 1;                               		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MissionInfo.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MissionInfo_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MissionInfo.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MissionInfo_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MissionInfo.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MissionInfo_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_MissionInfo.InitPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_MissionInfo_execInitPrimaryHUDElement_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_QuestTracking.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_QuestTracking_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_QuestTracking.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_QuestTracking_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_RaidTimer.UpdateRaidTicketBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_RaidTimer_execUpdateRaidTicketBar_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_RaidTimer.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_RaidTimer_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_RaidTimer.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_RaidTimer_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_RaidTimer.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_RaidTimer_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.Detach
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execDetach_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Settings.LoadConfigSettings
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execLoadConfigSettings_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Settings.SaveConfigSettings
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execSaveConfigSettings_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Settings.ShouldTick
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execShouldTick_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.IsHidden
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execIsHidden_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.CleanBasePanelList
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execCleanBasePanelList_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_Settings.RemoveScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execRemoveScene_Parms
{
	class UTgUISceneDriver*                            Scene;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.RemoveBasePanel
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execRemoveBasePanel_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.AddBasePanel
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execAddBasePanel_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            SceneDriver;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.IgnoreSettings
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execIgnoreSettings_Parms
{
	unsigned long                                      bIgnore : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_Settings.SetUIVisibility
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_Settings_execSetUIVisibility_Parms
{
	unsigned long                                      Visible : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_SlotWidgets.Init
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_SlotWidgets_execInit_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_SlotWidgets.SetBlank
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_SlotWidgets_execSetBlank_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_SlotWidgets.SetUnavailable
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_SlotWidgets_execSetUnavailable_Parms
{
	unsigned long                                      bUnavailable : 1;                                 		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_SlotWidgets.Show
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_SlotWidgets_execShow_Parms
{
	struct FString                                     IconTexture;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                IconIndex;                                        		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_SlotWidgets.Hide
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_SlotWidgets_execHide_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_TeamPanel.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_TeamPanel_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_TeamPanel.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_TeamPanel_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_TeamPanel.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_TeamPanel_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_VMTree.HandleKeyInput
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execHandleKeyInput_Parms
{
	struct FName                                       KeyInput;                                         		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD_VMTree.DeactivateTreeUI
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execDeactivateTreeUI_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_VMTree.ActivateTreeAndUI
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execActivateTreeAndUI_Parms
{
	class UTgVoiceMacroTree*                           tree;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_VMTree.PopulateTreeDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execPopulateTreeDisplay_Parms
{
};

// Function TgClient.TgUIPrimaryHUD_VMTree.FixupWidgetsAsEmbedded
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execFixupWidgetsAsEmbedded_Parms
{
	class UUIScreenObject*                             BaseScreenObject;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_VMTree.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execFixupWidgets_Parms
{
	class UUIPanel*                                    BasePanel;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            OwnerScene;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD_VMTree.TickPrimaryHUDElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_VMTree_execTickPrimaryHUDElement_Parms
{
	float                                              TimeNow;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.UIListCallbackWrapper
// [0x00020802] ( FUNC_Event )
struct UTgUISceneDriver_eventUIListCallbackWrapper_Parms
{
	class UUIList*                                     Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.OnNotifyEditBoxStateChanged
// [0x00024002] 
struct UTgUISceneDriver_execOnNotifyEditBoxStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISceneDriver.GetFontFromUCByIndex
// [0x00020802] ( FUNC_Event )
struct UTgUISceneDriver_eventGetFontFromUCByIndex_Parms
{
	int                                                FontSize;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UFont*                                       ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.SetDefaultSceneDriverSceneDelegates
// [0x00020802] ( FUNC_Event )
struct UTgUISceneDriver_eventSetDefaultSceneDriverSceneDelegates_Parms
{
};

// Function TgClient.TgUISceneDriver.DefaultSceneDriverSceneDelegate
// [0x00420002] 
struct UTgUISceneDriver_execDefaultSceneDriverSceneDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             bAltModified;                                     		// 0x0028 (0x0004) [0x0000000000000000]              
	// struct FName                                    closeKeyName;                                     		// 0x002C (0x0008) [0x0000000000000000]              
};

// Function TgClient.TgUISceneDriver.TickTgUISceneUC
// [0x00020802] ( FUNC_Event )
struct UTgUISceneDriver_eventTickTgUISceneUC_Parms
{
};

// Function TgClient.TgUISceneDriver.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execPostRender_Parms
{
};

// Function TgClient.TgUISceneDriver.CheckSceneForUIModelDragRelease
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execCheckSceneForUIModelDragRelease_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
};

// Function TgClient.TgUISceneDriver.HandleTgUIModelFixedRotation
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execHandleTgUIModelFixedRotation_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.HandleTgUIModelDragRotation
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execHandleTgUIModelDragRotation_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.SetFocusedWidget
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execSetFocusedWidget_Parms
{
	class UUIScreenObject*                             NewFocus;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.SetNextWidgetFocused
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execSetNextWidgetFocused_Parms
{
};

// Function TgClient.TgUISceneDriver.SetPreviousWidgetFocused
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execSetPreviousWidgetFocused_Parms
{
};

// Function TgClient.TgUISceneDriver.OnNextButton
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execOnNextButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.OnPrevButton
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execOnPrevButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.OnFirstButton
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execOnFirstButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.OnLastButton
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execOnLastButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.OpenTabScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execOpenTabScene_Parms
{
	int                                                nFuncId;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.DefaultTgShouldModulateDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execDefaultTgShouldModulateDelegate_Parms
{
	float                                              AlphaModulationPercent;                           		// 0x0000 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.SetParentUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execSetParentUIScene_Parms
{
	class UTgUISceneDriver*                            Scene;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.TriggerSceneFadeIn
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execTriggerSceneFadeIn_Parms
{
};

// Function TgClient.TgUISceneDriver.CreateDataStoreBindingString
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execCreateDataStoreBindingString_Parms
{
	class UUIDataStore*                                DataStore;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FName                                       SectionName;                                      		// 0x0004 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     ReturnValue;                                      		// 0x000C (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgUISceneDriver.GetCurrentKeyBinding
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_execGetCurrentKeyBinding_Parms
{
	int                                                bAltModified;                                     		// 0x0000 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	struct FName                                       ReturnValue;                                      		// 0x0004 (0x0008) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.DefaultSceneDriverCloseDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execDefaultSceneDriverCloseDelegate_Parms
{
	class UUIScene*                                    DeactivatedScene;                                 		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.SetVisible
// [0x00024400] ( FUNC_Native )
struct UTgUISceneDriver_execSetVisible_Parms
{
	class UUIObject*                                   pObject;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bVisible : 1;                                     		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISceneDriver.UnregisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execUnregisterCallbacks_Parms
{
};

// Function TgClient.TgUISceneDriver.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUISceneDriver.Repaint
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execRepaint_Parms
{
};

// Function TgClient.TgUISceneDriver.SetButtonsEnabled
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execSetButtonsEnabled_Parms
{
	unsigned long                                      bEnabled : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.HideChildControlsByTag
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execHideChildControlsByTag_Parms
{
	class UUIScreenObject*                             Parent;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     sCompareTag;                                      		// 0x0004 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                nCompareTagLen;                                   		// 0x0010 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISceneDriver.CloseTgUIScene
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTgUISceneDriver_execCloseTgUIScene_Parms
{
};

// Function TgClient.TgUISceneDriver.OpenTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execOpenTgUIScene_Parms
{
};

// Function TgClient.TgUISceneDriver.GetLocalTgPawn
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execGetLocalTgPawn_Parms
{
	class ATgPawn_Character*                           ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execPreCloseScene_Parms
{
};

// Function TgClient.TgUISceneDriver.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execPostOpenScene_Parms
{
};

// Function TgClient.TgUISceneDriver.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUICameraSettingsMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICameraSettingsMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICameraSettingsMenu.OnFakeRadioButtonChanged
// [0x00020002] 
struct UTgUICameraSettingsMenu_execOnFakeRadioButtonChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICameraSettingsMenu.OnApplyButtonKey
// [0x00420002] 
struct UTgUICameraSettingsMenu_execOnApplyButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICameraSettingsMenu.OnBackButtonKey
// [0x00420002] 
struct UTgUICameraSettingsMenu_execOnBackButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICameraSettingsMenu.ApplyCameraSettings
// [0x00020400] ( FUNC_Native )
struct UTgUICameraSettingsMenu_execApplyCameraSettings_Parms
{
};

// Function TgClient.TgUICameraSettingsMenu.PopulateWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICameraSettingsMenu_execPopulateWidgets_Parms
{
};

// Function TgClient.TgUICameraSettingsMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICameraSettingsMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUICameraSettingsMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICameraSettingsMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICameraSettingsMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICameraSettingsMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIChallengePopup.OnDeclineButton
// [0x00420400] ( FUNC_Native )
struct UTgUIChallengePopup_execOnDeclineButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChallengePopup.OnAcceptButton
// [0x00420400] ( FUNC_Native )
struct UTgUIChallengePopup_execOnAcceptButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChallengePopup.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIChallengePopup_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChallengePopup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIChallengePopup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIChat.Tick
// [0x00020400] ( FUNC_Native )
struct UTgUIChat_execTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIChat.OnChatScrollButton
// [0x00420400] ( FUNC_Native )
struct UTgUIChat_execOnChatScrollButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChat.OnChatEntrySubmitText
// [0x00020400] ( FUNC_Native )
struct UTgUIChat_execOnChatEntrySubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChat.OnChatTabClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIChat_execOnChatTabClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChat.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIChat_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIChat.ChatEntryLineChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIChat_execChatEntryLineChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIChat.UserSelectedChannel
// [0x00020400] ( FUNC_Native )
struct UTgUIChat_execUserSelectedChannel_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIChat.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIChat_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIChat.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIChat_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIClassSelectScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIClassSelectScene_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIClassSelectScene.OnClassMouseOverDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUIClassSelectScene_execOnClassMouseOverDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIClassSelectScene.OnNextButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIClassSelectScene_execOnNextButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIClassSelectScene.OnBackButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIClassSelectScene_execOnBackButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIClassSelectScene.OnSelectClassGenderDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIClassSelectScene_execOnSelectClassGenderDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIClassSelectScene.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIClassSelectScene_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIClassSelectScene.HighlightClassGender
// [0x00020400] ( FUNC_Native )
struct UTgUIClassSelectScene_execHighlightClassGender_Parms
{
	int                                                ClassIndex;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                genderIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIClassSelectScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIClassSelectScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIConfirmPopup.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIConfirmPopup_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIConfirmPopup.SetConfirmTextAndCallback
// [0x00020400] ( FUNC_Native )
struct UTgUIConfirmPopup_execSetConfirmTextAndCallback_Parms
{
	struct FString                                     Message;                                          		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	class UFunction*                                   callbackPtr;                                      		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIConfirmPopup.HandleConfirmButtonCallback
// [0x00020400] ( FUNC_Native )
struct UTgUIConfirmPopup_execHandleConfirmButtonCallback_Parms
{
	unsigned long                                      accepted : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIConfirmPopup.RemoveConfirmPopupByCallback
// [0x00024400] ( FUNC_Native )
struct UTgUIConfirmPopup_execRemoveConfirmPopupByCallback_Parms
{
	class UFunction*                                   callbackPtr;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bTellGameClient : 1;                              		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIConfirmPopup.SetConfirmDialogText
// [0x00020400] ( FUNC_Native )
struct UTgUIConfirmPopup_execSetConfirmDialogText_Parms
{
};

// Function TgClient.TgUIConfirmPopup.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIConfirmPopup_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIConfirmPopup.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIConfirmPopup_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIConfirmPopup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIConfirmPopup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIConfirmPopup.OnConfirmNo
// [0x00420400] ( FUNC_Native )
struct UTgUIConfirmPopup_execOnConfirmNo_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIConfirmPopup.OnConfirmYes
// [0x00420400] ( FUNC_Native )
struct UTgUIConfirmPopup_execOnConfirmYes_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreateCharA.SendSections
// [0x00820802] ( FUNC_Event )
struct UTgUICreateCharA_eventSendSections_Parms
{
	// struct FCustomCharacterAssembly                 newCustomAssembly;                                		// 0x0000 (0x0044) [0x0000000000000000]              
};

// Function TgClient.TgUICreateCharA.SetSelectHeadDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICreateCharA_eventSetSelectHeadDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICreateCharA.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICreateCharA_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICreateCharA.OnGenderButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreateCharA_execOnGenderButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreateCharA.OnSelectHeadDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreateCharA_execOnSelectHeadDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreateCharA.OnNextButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreateCharA_execOnNextButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreateCharA.OnBackButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreateCharA_execOnBackButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreateCharA.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreateCharA_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreateCharA.SaveMorphSettings
// [0x00020400] ( FUNC_Native )
struct UTgUICreateCharA_execSaveMorphSettings_Parms
{
};

// Function TgClient.TgUICreateCharA.SelectHead
// [0x00020400] ( FUNC_Native )
struct UTgUICreateCharA_execSelectHead_Parms
{
	int                                                headIndex;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICreateCharA.BuildHeadList
// [0x00020400] ( FUNC_Native )
struct UTgUICreateCharA_execBuildHeadList_Parms
{
};

// Function TgClient.TgUICreateCharA.SetupCustomModel
// [0x00020400] ( FUNC_Native )
struct UTgUICreateCharA_execSetupCustomModel_Parms
{
};

// Function TgClient.TgUICreateCharA.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICreateCharA_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICreateCharA.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUICreateCharA_execPreCloseScene_Parms
{
};

// Function TgClient.TgUICreatePlayerScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICreatePlayerScene_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICreatePlayerScene.OnAcceptInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreatePlayerScene_execOnAcceptInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreatePlayerScene.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICreatePlayerScene_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreatePlayerScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICreatePlayerScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICreditsMenu.OnSceneInput
// [0x00420400] ( FUNC_Native )
struct UTgUICreditsMenu_execOnSceneInput_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICreditsMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICreditsMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIDevInGame.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIDevInGame_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIDevInGame.OnSpawnBotButtonKey
// [0x00420002] 
struct UTgUIDevInGame_execOnSpawnBotButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgHUD_Game*                              tghudgame;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIDevInGame.OnLaunchScreenButtonKey
// [0x00420002] 
struct UTgUIDevInGame_execOnLaunchScreenButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevInGame.OnMissionScreenButtonKey
// [0x00420002] 
struct UTgUIDevInGame_execOnMissionScreenButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevInGame.OnCloseButtonKey
// [0x00420002] 
struct UTgUIDevInGame_execOnCloseButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevInGame.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIDevInGame_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIDevInGame.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDevInGame_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevInGame.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDevInGame_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIDevSpawnBot_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.OnBotList18
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList18_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList17
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList17_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList16
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList16_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList15
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList15_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList14
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList14_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList13
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList13_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList12
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList12_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList11
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList11_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList10
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList10_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList9
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList9_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList8
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList8_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList7
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList7_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList6
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList6_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList5
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList5_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList4
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList4_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList3
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList3_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList2
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList2_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotList1
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnBotList1_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnBotListBase
// [0x00020002] 
struct UTgUIDevSpawnBot_execOnBotListBase_Parms
{
	struct FName                                       Key;                                              		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      Event;                                            		// 0x0008 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                Index;                                            		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0010 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnSpawnBotButton
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnSpawnBotButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnCloseButton
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnSpawnBotSceneKey
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnSpawnBotSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnScrollLowerButton
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnScrollLowerButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.OnScrollHigherButton
// [0x00420002] 
struct UTgUIDevSpawnBot_execOnScrollHigherButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.StopScrolling
// [0x00020002] 
struct UTgUIDevSpawnBot_execStopScrolling_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.StartScrolling
// [0x00020002] 
struct UTgUIDevSpawnBot_execStartScrolling_Parms
{
	unsigned long                                      bHigher : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIDevSpawnBot.ScrollChange
// [0x00020002] 
struct UTgUIDevSpawnBot_execScrollChange_Parms
{
	unsigned long                                      bHigher : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIDevSpawnBot.SetupDataInterface
// [0x00020802] ( FUNC_Event )
struct UTgUIDevSpawnBot_eventSetupDataInterface_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.UpdateScrolling
// [0x00020400] ( FUNC_Native )
struct UTgUIDevSpawnBot_execUpdateScrolling_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.AttemptSpawn
// [0x00020400] ( FUNC_Native )
struct UTgUIDevSpawnBot_execAttemptSpawn_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.PopulateWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIDevSpawnBot_execPopulateWidgets_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIDevSpawnBot_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIDevSpawnBot.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDevSpawnBot_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDevSpawnBot.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDevSpawnBot_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIEndRaidScreen.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIEndRaidScreen_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIEndRaidScreen.ShowScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execShowScene_Parms
{
};

// Function TgClient.TgUIEndRaidScreen.HideScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execHideScene_Parms
{
};

// Function TgClient.TgUIEndRaidScreen.OnCloseButtonPressed
// [0x00420400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execOnCloseButtonPressed_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndRaidScreen.SetLabelTextAndColor
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execSetLabelTextAndColor_Parms
{
	unsigned long                                      bAttackersWon : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIEndRaidScreen.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndRaidScreen.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIEndRaidScreen.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIEndRaidScreen.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndRaidScreen_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIEULAMenu.OnScroll
// [0x00020400] ( FUNC_Native )
struct UTgUIEULAMenu_execOnScroll_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              Nudges;                                           		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bMaxedOut : 1;                                    		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x000C (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEULAMenu.OnSceneInput
// [0x00420400] ( FUNC_Native )
struct UTgUIEULAMenu_execOnSceneInput_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEULAMenu.OnButtonClick
// [0x00420400] ( FUNC_Native )
struct UTgUIEULAMenu_execOnButtonClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEULAMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEULAMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIFriendsList.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIFriendsList_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIFriendsList.OnPopupSubmitText
// [0x00020400] ( FUNC_Native )
struct UTgUIFriendsList_execOnPopupSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.OnPopupButtons
// [0x00420400] ( FUNC_Native )
struct UTgUIFriendsList_execOnPopupButtons_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.OnSceneButtons
// [0x00420400] ( FUNC_Native )
struct UTgUIFriendsList_execOnSceneButtons_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.OnSceneInputKey
// [0x00420400] ( FUNC_Native )
struct UTgUIFriendsList_execOnSceneInputKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.OnIgnoreButtons
// [0x00420400] ( FUNC_Native )
struct UTgUIFriendsList_execOnIgnoreButtons_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.OnListItemSelected
// [0x00420400] ( FUNC_Native )
struct UTgUIFriendsList_execOnListItemSelected_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.OnFriendButtons
// [0x00420400] ( FUNC_Native )
struct UTgUIFriendsList_execOnFriendButtons_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIFriendsList.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIFriendsList_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIHexAuction.OnSceneInputKeyDelegate
// [0x00420002] 
struct UTgUIHexAuction_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIHexAuction.SetHexPosition
// [0x00020002] 
struct UTgUIHexAuction_execSetHexPosition_Parms
{
	int                                                nId;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIHexAuction.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIHexAuction_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIHexAuction.OnMissionTypeChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execOnMissionTypeChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIHexAuction.OnSelectMissile
// [0x00420400] ( FUNC_Native )
struct UTgUIHexAuction_execOnSelectMissile_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIHexAuction.OnPlaceBidClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIHexAuction_execOnPlaceBidClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIHexAuction.UpdateButtonCovers
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execUpdateButtonCovers_Parms
{
};

// Function TgClient.TgUIHexAuction.RequestHexAuctionData
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execRequestHexAuctionData_Parms
{
};

// Function TgClient.TgUIHexAuction.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIHexAuction.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIHexAuction.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIHexAuction.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIHexAuction.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIHexAuction_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIInGameMainMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIInGameMainMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIInGameMainMenu.OnDisconnectButtonKey
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnDisconnectButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgPlayerController*                      tgpc;                                             		// 0x0028 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIInGameMainMenu.OnSpawnBotButtonKey
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnSpawnBotButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgHUD_Game*                              tghudgame;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIInGameMainMenu.OnLaunchScreenButtonKey
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnLaunchScreenButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInGameMainMenu.OnSettingsButtonKey
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnSettingsButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgHUD_Game*                              tghudgame;                                        		// 0x0028 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIInGameMainMenu.OnExitGameButtonKey
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnExitGameButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInGameMainMenu.OnReturnToGameButtonKey
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnReturnToGameButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInGameMainMenu.OnEndMissionButton
// [0x00420002] 
struct UTgUIInGameMainMenu_execOnEndMissionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgPlayerController*                      PC;                                               		// 0x0028 (0x0004) [0x0000000000000000]              
	// class AWorldInfo*                               WI;                                               		// 0x002C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIInGameMainMenu.ProcessQuit
// [0x00020400] ( FUNC_Native )
struct UTgUIInGameMainMenu_execProcessQuit_Parms
{
};

// Function TgClient.TgUIInGameMainMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIInGameMainMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIInGameMainMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInGameMainMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIInGameMainMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInGameMainMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInstanceSelect.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIInstanceSelect_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIInstanceSelect.SetSelectInstanceDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUIInstanceSelect_eventSetSelectInstanceDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIInstanceSelect.GetInstanceListCallback
// [0x00020400] ( FUNC_Native )
struct UTgUIInstanceSelect_execGetInstanceListCallback_Parms
{
	int                                                eType;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIInstanceSelect.OnSelectInstanceDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInstanceSelect_execOnSelectInstanceDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInstanceSelect.OnChangeButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInstanceSelect_execOnChangeButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInstanceSelect.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInstanceSelect_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInstanceSelect.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInstanceSelect_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInstanceSelect.SelectInstanceItem
// [0x00020400] ( FUNC_Native )
struct UTgUIInstanceSelect_execSelectInstanceItem_Parms
{
	int                                                ItemIndex;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIInstanceSelect.ResizeInstanceWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIInstanceSelect_execResizeInstanceWidgets_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIInstanceSelect.PopulateInstanceData
// [0x00020400] ( FUNC_Native )
struct UTgUIInstanceSelect_execPopulateInstanceData_Parms
{
};

// Function TgClient.TgUIInstanceSelect.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInstanceSelect_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIInstanceSelect.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIInstanceSelect_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIIntroScene.SetIntroParams
// [0x00020400] ( FUNC_Native )
struct UTgUIIntroScene_execSetIntroParams_Parms
{
	float                                              FadeTime;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              TotalTime;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIIntroScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIIntroScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIIntroScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIIntroScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUILFGPopup.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUILFGPopup_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUILFGPopup.UpdateLFGWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUILFGPopup_execUpdateLFGWidgets_Parms
{
};

// Function TgClient.TgUILFGPopup.OnCancelButton
// [0x00420400] ( FUNC_Native )
struct UTgUILFGPopup_execOnCancelButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGPopup.OnAcceptButton
// [0x00420400] ( FUNC_Native )
struct UTgUILFGPopup_execOnAcceptButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGPopup.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUILFGPopup_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGPopup.OnLFGPopupKey
// [0x00420400] ( FUNC_Native )
struct UTgUILFGPopup_execOnLFGPopupKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGPopup.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILFGPopup_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGPopup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILFGPopup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUILoadItemProfile.OnDialogButtonKey
// [0x00420002] 
struct UTgUILoadItemProfile_execOnDialogButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// struct FString                                  sWidgetTag;                                       		// 0x0028 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// class ATgPlayerController*                      PC;                                               		// 0x0034 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUILoadItemProfile.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUILoadItemProfile_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUILoadItemProfile.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUILoadItemProfile_execFixupWidgets_Parms
{
};

// Function TgClient.TgUILoadItemProfile.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILoadItemProfile_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoadItemProfile.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILoadItemProfile_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIMapTransition.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIMapTransition_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIMapTransition.CanShowCloseButton
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execCanShowCloseButton_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMapTransition.PleaseShowCloseButton
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execPleaseShowCloseButton_Parms
{
	unsigned long                                      bEnablePlease : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIMapTransition.OnCloseButtonClick
// [0x00420400] ( FUNC_Native )
struct UTgUIMapTransition_execOnCloseButtonClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMapTransition.SetBackgroundImage
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execSetBackgroundImage_Parms
{
	struct FString                                     fsImage;                                          		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIMapTransition.ResetMapTransition
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execResetMapTransition_Parms
{
};

// Function TgClient.TgUIMapTransition.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMapTransition.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIMapTransition.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIMapTransition.OpenTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execOpenTgUIScene_Parms
{
};

// Function TgClient.TgUIMapTransition.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMapTransition_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUINavScene.OnShowPath
// [0x00420002] 
struct UTgUINavScene_execOnShowPath_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINavScene.OnSelect
// [0x00420002] 
struct UTgUINavScene_execOnSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINavScene.OnCloseButton
// [0x00420002] 
struct UTgUINavScene_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINavScene.SetRawInputKeyForList
// [0x00020802] ( FUNC_Event )
struct UTgUINavScene_eventSetRawInputKeyForList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUINavScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUINavScene_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUINavScene.ShowPathToSelected
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execShowPathToSelected_Parms
{
};

// Function TgClient.TgUINavScene.UpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execUpdateHighlight_Parms
{
};

// Function TgClient.TgUINavScene.Select
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execSelect_Parms
{
	int                                                nIndex;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINavScene.SelectByButton
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execSelectByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINavScene.BuildPOIList
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execBuildPOIList_Parms
{
};

// Function TgClient.TgUINavScene.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUINavScene_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINavScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUINavScene.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execPreCloseScene_Parms
{
};

// Function TgClient.TgUINavScene.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUINavScene_execPostOpenScene_Parms
{
};

// Function TgClient.TgUINewSettingsMenu.FixupWidgetsUC
// [0x00020800] ( FUNC_Event )
struct UTgUINewSettingsMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUINewSettingsMenu.PopulateWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUINewSettingsMenu_execPopulateWidgets_Parms
{
};

// Function TgClient.TgUINewSettingsMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUINewSettingsMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUINewSettingsMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUINewSettingsMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINewSettingsMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUINewSettingsMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIOmegaMenu.FixupWidgetsUC
// [0x00020800] ( FUNC_Event )
struct UTgUIOmegaMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIOmegaMenu.GetUseMessage
// [0x00024400] ( FUNC_Native )
struct UTgUIOmegaMenu_execGetUseMessage_Parms
{
	int                                                msgId;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     vendorName;                                       		// 0x0004 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     ReturnValue;                                      		// 0x0010 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIOmegaMenu.SetUseMessage
// [0x00024400] ( FUNC_Native )
struct UTgUIOmegaMenu_execSetUseMessage_Parms
{
	int                                                msgId;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     vendorName;                                       		// 0x0004 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIOmegaMenu.SetOmegaVisibility
// [0x00020400] ( FUNC_Native )
struct UTgUIOmegaMenu_execSetOmegaVisibility_Parms
{
	unsigned long                                      bVisible : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIOmegaMenu.TriggerOmegaAlert
// [0x00024400] ( FUNC_Native )
struct UTgUIOmegaMenu_execTriggerOmegaAlert_Parms
{
	unsigned long                                      bForced : 1;                                      		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIOmegaMenu.PopulateOmegaWidgets
// [0x00024400] ( FUNC_Native )
struct UTgUIOmegaMenu_execPopulateOmegaWidgets_Parms
{
	int                                                omegaId;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                omegaMode;                                        		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIOmegaMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIOmegaMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIOmegaMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIOmegaMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIOpenTerritories.OnSceneInputKeyDelegate
// [0x00420002] 
struct UTgUIOpenTerritories_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIOpenTerritories.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIOpenTerritories_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIOpenTerritories.OnOpenClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIOpenTerritories_execOnOpenClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIOpenTerritories.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIOpenTerritories_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIOpenTerritories.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIOpenTerritories_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.OnSubmitText
// [0x00020002] 
struct UTgUIPlayerSearchMenu_execOnSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnResetKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnResetKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnSearchKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnSearchKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.SetRawInputKeyForResultsList
// [0x00020802] ( FUNC_Event )
struct UTgUIPlayerSearchMenu_eventSetRawInputKeyForResultsList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnSendMessageSelected
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnSendMessageSelected_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnInviteSelected
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnInviteSelected_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnTeamStatusHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnTeamStatusHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnLocationHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnLocationHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnLevelHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnLevelHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnFactionHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnFactionHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnAllianceHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnAllianceHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnAgencyHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnAgencyHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnNameHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnNameHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnClassHeaderKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnClassHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnNextPageKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnNextPageKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnPrevPageKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnPrevPageKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnLastPageKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnLastPageKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnFirstPageKey
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnFirstPageKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnSelectResult
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnSelectResult_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.OnCloseButton
// [0x00420002] 
struct UTgUIPlayerSearchMenu_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIPlayerSearchMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.SetParentUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execSetParentUIScene_Parms
{
	class UTgUISceneDriver*                            Scene;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPlayerSearchMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.OnSearchMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execOnSearchMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.NextPage
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execNextPage_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.PrevPage
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPrevPage_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.FirstPage
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execFirstPage_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.LastPage
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execLastPage_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.SendMessageSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execSendMessageSelected_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.PlayerSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPlayerSelected_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.InviteSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execInviteSelected_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.SelectResult
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execSelectResult_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.SelectResultByButton
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execSelectResultByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPlayerSearchMenu.UpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execUpdateHighlight_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.UpdateDetailPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execUpdateDetailPanel_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPlayerSearchMenu.UpdateResultListDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execUpdateResultListDisplay_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.GetPlayerSearchCallback
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execGetPlayerSearchCallback_Parms
{
	int                                                eType;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPlayerSearchMenu.Search
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execSearch_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.SearchFromFields
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execSearchFromFields_Parms
{
};

// Function TgClient.TgUIPlayerSearchMenu.PrepopulatedSearchById
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPrepopulatedSearchById_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPlayerSearchMenu.PrepopulatedSearch
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execPrepopulatedSearch_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sAgencyName;                                      		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sAllianceName;                                    		// 0x0018 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIPlayerSearchMenu.ResetFields
// [0x00020400] ( FUNC_Native )
struct UTgUIPlayerSearchMenu_execResetFields_Parms
{
};

// Function TgClient.TgUIPostCreateLoad.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPostCreateLoad_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPostCreateLoad.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPostCreateLoad_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIPreviewScreen.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIPreviewScreen_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIPreviewScreen.OnRotateDudeStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIPreviewScreen_execOnRotateDudeStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPreviewScreen.OnRotateDudeCCWClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIPreviewScreen_execOnRotateDudeCCWClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPreviewScreen.OnRotateDudeCWClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIPreviewScreen_execOnRotateDudeCWClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPreviewScreen.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIPreviewScreen_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPreviewScreen.OnPreviewScreenDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIPreviewScreen_execOnPreviewScreenDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPreviewScreen.SetPaperDollPreviewItem
// [0x00020400] ( FUNC_Native )
struct UTgUIPreviewScreen_execSetPaperDollPreviewItem_Parms
{
	class UTgInventoryObject*                          Item;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPreviewScreen.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPreviewScreen_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPreviewScreen.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPreviewScreen_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIPreviewScreen.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPreviewScreen_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.FixupUIDeviceBarsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIPrimaryHUD_eventFixupUIDeviceBarsUC_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.OnPrimaryHUDKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execOnPrimaryHUDKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD.UpdateFade
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateFade_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.SetFade
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execSetFade_Parms
{
	unsigned long                                      bEnableFading : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	struct FColor                                      FadeColor;                                        		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	struct FVector2D                                   FadeAlpha;                                        		// 0x0008 (0x0008) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	float                                              FadeTime;                                         		// 0x0010 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.SetOmegaVisibility
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execSetOmegaVisibility_Parms
{
	unsigned long                                      bVisible : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.TempToggleElement
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execTempToggleElement_Parms
{
	unsigned char                                      Element;                                          		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bVisible : 1;                                     		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.UpdateEquipTimer
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateEquipTimer_Parms
{
	unsigned long                                      IsActive : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	float                                              EquipTime;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.TriggerUIBarsFullUpdate
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execTriggerUIBarsFullUpdate_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.UpdateHUDCooldowns
// [0x00024400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateHUDCooldowns_Parms
{
	int                                                DeviceEqpPt;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                DeviceMode;                                       		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              SetupTime;                                        		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              RechargeTime;                                     		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              StartupTime;                                      		// 0x0010 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.UpdateReticleOffset
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateReticleOffset_Parms
{
	float                                              OffsetDistanceFromCenterX;                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              OffsetDistanceFromCenterY;                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.UpdateTargetHealthBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateTargetHealthBar_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.UpdateReticuleLabelValues
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateReticuleLabelValues_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.SwitchReticuleStatusBar
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execSwitchReticuleStatusBar_Parms
{
	int                                                ReticuleToUse;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.SetReticuleVisibility
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execSetReticuleVisibility_Parms
{
	unsigned long                                      CrossHairVisibility : 1;                          		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      WeaponStatusVisibility : 1;                       		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.SetUIClockTimer
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execSetUIClockTimer_Parms
{
	unsigned char                                      ClockStateBitfield;                               		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	float                                              TimerDuration;                                    		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.UpdateAlerts
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execUpdateAlerts_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.OnSpeaking
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execOnSpeaking_Parms
{
	unsigned long                                      bSpeaking : 1;                                    		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIPrimaryHUD.ToggleMapNameDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execToggleMapNameDisplay_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIPrimaryHUD.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIPrimaryHUD.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIPrimaryHUD_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIQueueScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIQueueScene_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIQueueScene.AddFeedbackMessage
// [0x00020400] ( FUNC_Native )
struct UTgUIQueueScene_execAddFeedbackMessage_Parms
{
	struct FString                                     newMessage;                                       		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIQueueScene.OnDevButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnDevButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.OnRefreshQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnRefreshQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.OnLeaveQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnLeaveQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.OnJoinQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnJoinQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.OnSelectQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnSelectQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.OnSceneKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIQueueScene_execOnSceneKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.PopulateQueueList
// [0x00020400] ( FUNC_Native )
struct UTgUIQueueScene_execPopulateQueueList_Parms
{
};

// Function TgClient.TgUIQueueScene.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIQueueScene_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIQueueScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIQueueScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQueueScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIQueueScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIRaidHexMap.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIRaidHexMap_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIRaidHexMap.OnSceneRaidDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnSceneRaidDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnSceneCargoDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnSceneCargoDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnSceneDetailDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnSceneDetailDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnSceneTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnSceneTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnSceneLeaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnSceneLeaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnCloseDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnCloseDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnReturnDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnReturnDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.OnLaunchClickDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnLaunchClickDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.UpdateInfoPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidHexMap_execUpdateInfoPanel_Parms
{
};

// Function TgClient.TgUIRaidHexMap.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidHexMap_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidHexMap_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidHexMap.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidHexMap_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIRaidHexMap.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidHexMap_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIRaidHexMap.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidHexMap_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIRaidMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIRaidMenu_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             J;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIRaidMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIRaidMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRaidMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIRaidMenu.OnSceneMapDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneMapDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnSceneCargoDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneCargoDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnSceneDetailDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneDetailDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnSceneTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnReturnDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnReturnDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnCloseDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnCloseDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnSceneLeaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneLeaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnScenePromoteDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnScenePromoteDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnSceneRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnMoveToDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnMoveToDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnRaidTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnRaidTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRaidMenu.OnRaidMemberDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIRaidMenu_execOnRaidMemberDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIReleaseMenus.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIReleaseMenus_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIReleaseMenus.OnBasicReleaseDialogButtonKey
// [0x00420002] 
struct UTgUIReleaseMenus_execOnBasicReleaseDialogButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIReleaseMenus.SetSpectateText
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execSetSpectateText_Parms
{
};

// Function TgClient.TgUIReleaseMenus.ToggleHideScene
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execToggleHideScene_Parms
{
	unsigned long                                      hideFlag : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIReleaseMenus.IsReleaseDialogActive
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execIsReleaseDialogActive_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIReleaseMenus.SetDeathZoomInfo
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execSetDeathZoomInfo_Parms
{
	struct FDeathZoomInfo                              Info;                                             		// 0x0000 (0x001C) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIReleaseMenus.SetReleaseData
// [0x00024400] ( FUNC_Native )
struct UTgUIReleaseMenus_execSetReleaseData_Parms
{
	unsigned long                                      bActive : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	float                                              fReleaseTime;                                     		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bCanManuallyRelease : 1;                          		// 0x0008 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIReleaseMenus.UpdateReleaseDialog
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execUpdateReleaseDialog_Parms
{
};

// Function TgClient.TgUIReleaseMenus.UserManuallyReleased
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execUserManuallyReleased_Parms
{
};

// Function TgClient.TgUIReleaseMenus.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIReleaseMenus.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIReleaseMenus.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIReleaseMenus.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIReleaseMenus.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIReleaseMenus_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.FixupConfirmWidgets
// [0x00020802] ( FUNC_Event )
struct UTgUISceneDriver_Tooltip_eventFixupConfirmWidgets_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.OpenFeatureUpgradePopup
// [0x00024400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOpenFeatureUpgradePopup_Parms
{
	unsigned long                                      bSubscriberCheck : 1;                             		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OnConfirmItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOnConfirmItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OnAmountChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOnAmountChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OnConfirmOkClicked
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOnConfirmOkClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OpenConfirmInfoPopup
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOpenConfirmInfoPopup_Parms
{
	struct FString                                     Text;                                             		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUISceneDriver_Tooltip.OpenConfirmDeletePopup
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOpenConfirmDeletePopup_Parms
{
	class UTgInventoryObject*                          pItem;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                maxItems;                                         		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISceneDriver_Tooltip.CloseConfirmPopup
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execCloseConfirmPopup_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.ShowConfirmPopup
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execShowConfirmPopup_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.SetUpConfirmEditBoxes
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execSetUpConfirmEditBoxes_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.SetUpConfirmItemPanel
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execSetUpConfirmItemPanel_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.CreateConfirmPopup
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execCreateConfirmPopup_Parms
{
};

// Function TgClient.TgUISceneDriver_Tooltip.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneDriver_Tooltip.OpenTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISceneDriver_Tooltip_execOpenTgUIScene_Parms
{
};

// Function TgClient.TgUIAgencyDirectory.SetRawInputKeyForAgencyButton
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyDirectory_eventSetRawInputKeyForAgencyButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyDirectory.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyDirectory_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyDirectory.OnEnterValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnEnterValueChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyDirectory.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnCancelButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnCancelButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnSendCancelButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnSendCancelButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnSendButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnSendButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnApplyButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnApplyButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnSelectAgencyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnSelectAgencyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnSearchButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnSearchButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.OnSceneKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execOnSceneKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgencyDirectory.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyDirectory.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyDirectory_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu.SelectTab
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_eventSelectTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	// class ATgHUD_Game*                              HUD;                                              		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgencyMenu.OnSelectInventoryTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectInventoryTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectAuditingTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectAuditingTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectRecruitingTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectRecruitingTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectFacilitiesTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectFacilitiesTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectManagementTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectManagementTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectApplicantsTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectApplicantsTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectAllianceTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectAllianceTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectBaseTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectBaseTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnSelectGeneralTab
// [0x00420002] 
struct UTgUIAgencyMenu_execOnSelectGeneralTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.OnCloseButton
// [0x00420002] 
struct UTgUIAgencyMenu_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu.OnAgencyMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_execOnAgencyMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.GetAllianceData
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execGetAllianceData_Parms
{
	class UTgAllianceData*                             ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.GetAgencyData
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execGetAgencyData_Parms
{
	class UTgAgencyData*                               ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu.UpdateViewablePanels
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execUpdateViewablePanels_Parms
{
};

// Function TgClient.TgUIAgencyMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnSelectAllianceMember
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnSelectAllianceMember_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.SetRawInputKeyForAllianceMemberList
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Alliance_eventSetRawInputKeyForAllianceMemberList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnAllianceApplicantInviteSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_Alliance_execOnAllianceApplicantInviteSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnAllianceApplicantInviteButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnAllianceApplicantInviteButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnConfirmCancelKey
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnConfirmCancelKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnConfirmAcceptKey
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnConfirmAcceptKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnMemberLeaveButton
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnMemberLeaveButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnMemberRemoveButton
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnMemberRemoveButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnCreationSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_Alliance_execOnCreationSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.OnCreationButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_Alliance_execOnCreationButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Alliance_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.SelectAllianceMember
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execSelectAllianceMember_Parms
{
	int                                                nAgencyId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.SelectAllianceMemberByButton
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execSelectAllianceMemberByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.UpdateAllianceMemberHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execUpdateAllianceMemberHighlight_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.UpdateDetailPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execUpdateDetailPanel_Parms
{
	struct FsAllianceMemberInfo                        selectedAgency;                                   		// 0x0000 (0x0024) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_Alliance.CloseConfirm
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execCloseConfirm_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.DoConfirm
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execDoConfirm_Parms
{
	unsigned char                                      Mode;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.ResizeAllianceMemberList
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execResizeAllianceMemberList_Parms
{
	int                                                nSize;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.SendAllianceCreate
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execSendAllianceCreate_Parms
{
	struct FString                                     sName;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_Alliance.UpdateViewablePanels
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execUpdateViewablePanels_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.UpdateAllianceDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execUpdateAllianceDisplay_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.SendAllianceLeave
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execSendAllianceLeave_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Alliance.SendAllianceRemove
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execSendAllianceRemove_Parms
{
	int                                                nAgencyId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Alliance.SendAllianceInvite
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execSendAllianceInvite_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_Alliance.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Alliance_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Applicants.OnApplicantInviteSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_Applicants_execOnApplicantInviteSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Applicants.OnApplicantInviteButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_Applicants_execOnApplicantInviteButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Applicants.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Applicants_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Applicants.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Applicants_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Applicants.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Applicants_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Applicants.UpdateViewablePanels
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Applicants_execUpdateViewablePanels_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Applicants.SendAgencyInvite
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Applicants_execSendAgencyInvite_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_Creation.OnCreationSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_Creation_execOnCreationSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Creation.OnCreationButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_Creation_execOnCreationButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Creation.OnGenerateColorKey
// [0x00420002] 
struct UTgUIAgencyMenu_Creation_execOnGenerateColorKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Creation.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Creation_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Creation.GenerateNewColor
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Creation_execGenerateNewColor_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Creation.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Creation_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Creation.SendAgencyCreate
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Creation_execSendAgencyCreate_Parms
{
	struct FString                                     sAgencyName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_Facilities.SetCancelTaskDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Facilities_eventSetCancelTaskDelegate_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Facilities_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.UpdateProductionInformation
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execUpdateProductionInformation_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.UpdateHexInformation
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execUpdateHexInformation_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.UpdateZoneList
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execUpdateZoneList_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnNavButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnNavButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnDefenseApplied
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnDefenseApplied_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnShieldTileClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnShieldTileClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.UpdateDefenseButtons
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execUpdateDefenseButtons_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnCurrentTaskStateChanged
// [0x00024400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnCurrentTaskStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPlayerIndex;                                     		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnCancelCurrentTask
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnCancelCurrentTask_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnCancelTask
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnCancelTask_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnAddTask
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnAddTask_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnAbandonHex
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnAbandonHex_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnTransferHex
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnTransferHex_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnChangeHQ
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnChangeHQ_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnChangeFacility
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnChangeFacility_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnZoneChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnZoneChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.OnAgencyMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execOnAgencyMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.GetHoverHexCellPosition
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execGetHoverHexCellPosition_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.GetSelectedHexCellPosition
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execGetSelectedHexCellPosition_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execPostRender_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.UpdateHexMap
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execUpdateHexMap_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.UpdateToolTipData
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execUpdateToolTipData_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Facilities.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Facilities.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Facilities_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.SetMemberListSortType
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_General_eventSetMemberListSortType_Parms
{
	unsigned char                                      SortType;                                         		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_General.SetRawInputKeyForMemberList
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_General_eventSetRawInputKeyForMemberList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_General.OnChangeOfficerCommentButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnChangeOfficerCommentButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnChangePublicCommentButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnChangePublicCommentButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnChangeMOTDButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnChangeMOTDButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.SubmitPopupEditBox
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_General_eventSubmitPopupEditBox_Parms
{
	// struct FString                                  sNewText;                                         		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_General.OnPopupSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_General_execOnPopupSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnPopupPanelCancel
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnPopupPanelCancel_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnPopupPanelAccept
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnPopupPanelAccept_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.SetupPopupTextEditPanel
// [0x00820802] ( FUNC_Event )
struct UTgUIAgencyMenu_General_eventSetupPopupTextEditPanel_Parms
{
	unsigned char                                      NewMode;                                          		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	// struct FsAgencyMemberInfo                       selectedPlayerInfo;                               		// 0x0004 (0x004C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// unsigned long                                   bFoundPlayer : 1;                                 		// 0x0050 (0x0004) [0x0000000000000000] [0x00000001] 
	// struct FString                                  sNewMessage;                                      		// 0x0054 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// struct FString                                  sNewHeader;                                       		// 0x0060 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_General.OnTeamInviteButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnTeamInviteButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnRemoveButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnRemoveButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnDemoteButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnDemoteButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnPromoteButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnPromoteButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnSearchHotlinkKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnSearchHotlinkKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnStatusHeaderKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnStatusHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnRankHeaderKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnRankHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnNameHeaderKey
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnNameHeaderKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnSelectMember
// [0x00420002] 
struct UTgUIAgencyMenu_General_execOnSelectMember_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.OnShowOfflineCheckBoxChanged
// [0x00020002] 
struct UTgUIAgencyMenu_General_execOnShowOfflineCheckBoxChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// int                                             nCurrentFilter;                                   		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgencyMenu_General.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_General_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.TeamInviteSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execTeamInviteSelected_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.RemoveSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execRemoveSelected_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.DemoteSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execDemoteSelected_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.PromoteSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execPromoteSelected_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.PlayerSearchSelected
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execPlayerSearchSelected_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.OnAgencyMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execOnAgencyMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.SelectMember
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execSelectMember_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.SelectMemberByButton
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execSelectMemberByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_General.UpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execUpdateHighlight_Parms
{
};

// Function TgClient.TgUIAgencyMenu_General.UpdateDetailPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execUpdateDetailPanel_Parms
{
	struct FsAgencyMemberInfo                          selectedPlayer;                                   		// 0x0000 (0x004C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FsAgencyMemberInfo                          LocalPlayer;                                      		// 0x004C (0x004C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_General.UpdateMemberListDisplay
// [0x00024400] ( FUNC_Native )
struct UTgUIAgencyMenu_General_execUpdateMemberListDisplay_Parms
{
	unsigned long                                      bNeedsResort : 1;                                 		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Inventory_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Inventory.SetAgencyItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Inventory_eventSetAgencyItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.SetSelectAgencyItemDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Inventory_eventSetSelectAgencyItemDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.SetInvItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Inventory_eventSetInvItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.SetSelectInventoryItemDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Inventory_eventSetSelectInventoryItemDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnPrevAgencyButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnPrevAgencyButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnNextAgencyButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnNextAgencyButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnAgencyItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnAgencyItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnFilterChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnFilterChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnItemSendToDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnItemSendToDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnDeleteItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnDeleteItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnWithdrawCreditsDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnWithdrawCreditsDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnDepositCreditsDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnDepositCreditsDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnWithdrawItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnWithdrawItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnDepositItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnDepositItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnSortAgencyInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnSortAgencyInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnSortInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnSortInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnSelectAgencyItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnSelectAgencyItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnSelectInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnSelectInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.OnAgencyMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execOnAgencyMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.HideTransferItemPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execHideTransferItemPanel_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Inventory.ShowTransferCreditsPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execShowTransferCreditsPanel_Parms
{
	int                                                creditCount;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bDeposit : 1;                                     		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.ShowTransferItemPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execShowTransferItemPanel_Parms
{
	class UTgInventoryObject*                          pItem;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                ItemCount;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bDeposit : 1;                                     		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.SelectAgencyItem
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execSelectAgencyItem_Parms
{
	int                                                ItemIndex;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.SelectInventoryItem
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execSelectInventoryItem_Parms
{
	int                                                ItemIndex;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.PopulateAgencyItems
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execPopulateAgencyItems_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Inventory.PopulateInventoryItems
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execPopulateInventoryItems_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Inventory.ResizeAgencyPanelWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execResizeAgencyPanelWidgets_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.ResizeInventoryPanelWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execResizeInventoryPanelWidgets_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Inventory.BuildLocalInventoryStore
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execBuildLocalInventoryStore_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Inventory.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Inventory.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Inventory_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_InventoryLog_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.OnSearchButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_InventoryLog_execOnSearchButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.ResizeLogList
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_InventoryLog_execResizeLogList_Parms
{
	int                                                nSize;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.ResetFilters
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_InventoryLog_execResetFilters_Parms
{
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_InventoryLog_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.OpenTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_InventoryLog_execOpenTgUIScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_InventoryLog.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_InventoryLog_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnAgencyDescriptionButton
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnAgencyDescriptionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnAgencyDescriptionSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_Management_execOnAgencyDescriptionSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnTransferLeaderButton
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnTransferLeaderButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnTransferLeaderSubmitText
// [0x00020002] 
struct UTgUIAgencyMenu_Management_execOnTransferLeaderSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankDisabled
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankDisabled_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankValueChanged
// [0x00020002] 
struct UTgUIAgencyMenu_Management_execOnRankValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankDemote
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankDemote_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankPromote
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankPromote_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankRevert
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankRevert_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankSend
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankSend_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankRemove
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankRemove_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankAdd
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnRankAdd_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnAgencyDisband
// [0x00420002] 
struct UTgUIAgencyMenu_Management_execOnAgencyDisband_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Management_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.RegionComboBoxChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execRegionComboBoxChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnAgencyDescriptionValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execOnAgencyDescriptionValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Management.OnRankChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execOnRankChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Management.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.UpdateDetailPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execUpdateDetailPanel_Parms
{
	struct FsAgencyRankData                            rankData;                                         		// 0x0000 (0x0018) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUIAgencyMenu_Management.UpdateRankList
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execUpdateRankList_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.RankValueUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execRankValueUpdated_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Management.DemoteRank
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execDemoteRank_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.PromoteRank
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execPromoteRank_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.RevertRankData
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execRevertRankData_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.SendRankData
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execSendRankData_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.RemoveRank
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execRemoveRank_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.AddRank
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execAddRank_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.Disband
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execDisband_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.TransferLeader
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execTransferLeader_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.SetAgencyRegion
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execSetAgencyRegion_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.SetAgencyDescription
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execSetAgencyDescription_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Management.SelectRank
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execSelectRank_Parms
{
	int                                                nRankId;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Management.UpdateViewablePanels
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Management_execUpdateViewablePanels_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnSubmitButtonKey
// [0x00420002] 
struct UTgUIAgencyMenu_Recruiting_execOnSubmitButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnSelectAgent
// [0x00420002] 
struct UTgUIAgencyMenu_Recruiting_execOnSelectAgent_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.SetRawInputKeyForAgentListMember
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Recruiting_eventSetRawInputKeyForAgentListMember_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnAcceptKey
// [0x00420002] 
struct UTgUIAgencyMenu_Recruiting_execOnAcceptKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnDeclineKey
// [0x00420002] 
struct UTgUIAgencyMenu_Recruiting_execOnDeclineKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgencyMenu_Recruiting_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnEnterValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execOnEnterValueChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnDeclineButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execOnDeclineButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.OnAcceptButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execOnAcceptButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.SendRecruitingUpdate
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execSendRecruitingUpdate_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Recruiting.SelectAgent
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execSelectAgent_Parms
{
	int                                                nCharacterId;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.SelectAgentByButton
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execSelectAgentByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgencyMenu_Recruiting.UpdateViewablePanels
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execUpdateViewablePanels_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Recruiting.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Recruiting.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgencyMenu_Recruiting.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgencyMenu_Recruiting_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgentProfile.SelectTab
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_eventSelectTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	// class ATgHUD_Game*                              HUD;                                              		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgentProfile.OnSelectAchieveTab
// [0x00420002] 
struct UTgUIAgentProfile_execOnSelectAchieveTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.OnSelectInvTab
// [0x00420002] 
struct UTgUIAgentProfile_execOnSelectInvTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.OnSelectStatsTab
// [0x00420002] 
struct UTgUIAgentProfile_execOnSelectStatsTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.OnSelectSkillsTab
// [0x00420002] 
struct UTgUIAgentProfile_execOnSelectSkillsTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.OnSelectEquipTab
// [0x00420002] 
struct UTgUIAgentProfile_execOnSelectEquipTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.OnCloseButton
// [0x00420002] 
struct UTgUIAgentProfile_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgentProfile.OnSelectProfileButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_execOnSelectProfileButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.GetPawnToUse
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execGetPawnToUse_Parms
{
	class ATgPawn_Character*                           ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.CanChangeProfiles
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execCanChangeProfiles_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.IsReadOnlyMode
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execIsReadOnlyMode_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.SetInspectedPawn
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execSetInspectedPawn_Parms
{
	class ATgPawn_Character*                           Pawn;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile.OnAgentMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_execOnAgentMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgentProfile.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgentProfile.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgentProfile.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Achievement_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             J;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgentProfile_Achievement.ClearRawInputKeyForAchievementsList
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Achievement_eventClearRawInputKeyForAchievementsList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Achievement.SetRawInputKeyForCategory
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Achievement_eventSetRawInputKeyForCategory_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Achievement.ActiveTooltipChanged
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execActiveTooltipChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Achievement.OnProgressBarStateChangeDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execOnProgressBarStateChangeDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Achievement.OnAchievementNextButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execOnAchievementNextButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.OnAchievementPrevButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execOnAchievementPrevButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.OnCategoryNextButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execOnCategoryNextButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.OnCategoryPrevButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execOnCategoryPrevButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.OnAchievementCategoryClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execOnAchievementCategoryClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Achievement.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Achievement.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Achievement_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.SelectTab
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSelectTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnCloseButton
// [0x00420002] 
struct UTgUIAgentProfile_Equip_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetEquippedItemRepairClick
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetEquippedItemRepairClick_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetRepairButtonMouseOver
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetRepairButtonMouseOver_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetEquippedItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetEquippedItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetEquippedItemSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetEquippedItemSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetDeviceListStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetDeviceListStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetDeviceListSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetDeviceListSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetInvItemSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetInvItemSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.SetInvItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventSetInvItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Equip_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execPostRender_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.CheckTutorialDisabled
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execCheckTutorialDisabled_Parms
{
	unsigned char                                      Step;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.CheckTutorialStep
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execCheckTutorialStep_Parms
{
	unsigned char                                      Step;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.UpdateTutorialProgress
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execUpdateTutorialProgress_Parms
{
	unsigned char                                      Step;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                NewTutorialType;                                  		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.BeginTutorial
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execBeginTutorial_Parms
{
	unsigned char                                      TutType;                                          		// 0x0000 (0x0001) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnPaperDollUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnPaperDollUpdated_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.UpdatePaperDoll
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execUpdatePaperDoll_Parms
{
	unsigned long                                      bInit : 1;                                        		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.DestroyPaperDoll
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execDestroyPaperDoll_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.SetupPaperDoll
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execSetupPaperDoll_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.OnConfirmOkClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnConfirmOkClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnEquippedItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnEquippedItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnEquippedItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnEquippedItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnInvItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnInvItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnDeviceListSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnDeviceListSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnDeviceListStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnDeviceListStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnRepairButtonMouseOver
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnRepairButtonMouseOver_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnRepairButtonClick
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnRepairButtonClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnPrevDevicePageButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnPrevDevicePageButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnNextDevicePageButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnNextDevicePageButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnSelectAppearanceTab
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnSelectAppearanceTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnSelectArmorTab
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnSelectArmorTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnSelectDeviceTab
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnSelectDeviceTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.OnAgentMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execOnAgentMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.IsReadOnlyMode
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execIsReadOnlyMode_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Equip.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAgentProfile_Equip.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Equip_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Inventory_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAgentProfile_Inventory.SetEquipIconStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Inventory_eventSetEquipIconStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.SetSalvagedItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Inventory_eventSetSalvagedItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.SetInvMainStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Inventory_eventSetInvMainStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.SetInvItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Inventory_eventSetInvItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.SetSelectInventoryItemDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Inventory_eventSetSelectInventoryItemDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.ResizeSalvagedDisplayList
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execResizeSalvagedDisplayList_Parms
{
	int                                                nNewSize;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnEquipIconStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnEquipIconStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnSalvagedItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnSalvagedItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnInvMainStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnInvMainStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnSalvageCloseDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnSalvageCloseDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnActivateInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnActivateInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnSortInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnSortInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnSalvageInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnSalvageInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnDeleteInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnDeleteInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnSelectInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnSelectInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnFilterChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnFilterChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.SelectInventoryItem
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execSelectInventoryItem_Parms
{
	int                                                ItemIndex;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Inventory.PopulateInventoryItems
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execPopulateInventoryItems_Parms
{
};

// Function TgClient.TgUIAgentProfile_Inventory.BuildLocalInventoryStore
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execBuildLocalInventoryStore_Parms
{
};

// Function TgClient.TgUIAgentProfile_Inventory.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Inventory.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Inventory.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Inventory_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Skill.RequestPlayersSkillsFromServer
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Skill_eventRequestPlayersSkillsFromServer_Parms
{
};

// Function TgClient.TgUIAgentProfile_Skill.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Skill_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             J;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
	// int                                             nTreeSize;                                        		// 0x0008 (0x0004) [0x0000000000000000]              
	// int                                             Idx;                                              		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgentProfile_Skill.ValidateRespecSkillFeature
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execValidateRespecSkillFeature_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Skill.OnSkillButtonStateChangeDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execOnSkillButtonStateChangeDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Skill.OnSkillButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execOnSkillButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Skill.OnResetAllDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execOnResetAllDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Skill.OnResetDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execOnResetDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Skill.OnSaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execOnSaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Skill.IsReadOnlyMode
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execIsReadOnlyMode_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Skill.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Skill.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Skill.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Skill.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Skill_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAgentProfile_Stat_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             J;                                                		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIAgentProfile_Stat.OnBadgeIconStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnBadgeIconStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnFlairIconStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnFlairIconStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnUnequipFlairClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnUnequipFlairClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnFlairIconClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnFlairIconClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnPaperDollUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnPaperDollUpdated_Parms
{
};

// Function TgClient.TgUIAgentProfile_Stat.DestroyPaperDoll
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execDestroyPaperDoll_Parms
{
};

// Function TgClient.TgUIAgentProfile_Stat.SetupPaperDoll
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execSetupPaperDoll_Parms
{
};

// Function TgClient.TgUIAgentProfile_Stat.OnNextBadgeButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnNextBadgeButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnPrevBadgeButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnPrevBadgeButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnNextButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnNextButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnPrevButtonClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnPrevButtonClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnSelectIconTab
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnSelectIconTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnSelectSubTab
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnSelectSubTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAgentProfile_Stat.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Stat.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Stat.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAgentProfile_Stat.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAgentProfile_Stat_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.ValidateAuctionFeature
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execValidateAuctionFeature_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.OpenBuyoutConfirmPopup
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOpenBuyoutConfirmPopup_Parms
{
	struct FsAuctionItemStruct                         sItemSelected;                                    		// 0x0000 (0x004C) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
};

// Function TgClient.TgUIAuctionGeneral.OnAuctionItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOnAuctionItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.OnBuyoutButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOnBuyoutButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.OnTabButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOnTabButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionGeneral.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionGeneral_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBidMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBidMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.SetAuctionItemSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBidMenu_eventSetAuctionItemSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBidMenu.SetAuctionItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBidMenu_eventSetAuctionItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBidMenu.OnAuctionTimeStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execOnAuctionTimeStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAuctionBidMenu.OnAuctionItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execOnAuctionItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAuctionBidMenu.OnAuctionItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execOnAuctionItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBidMenu.OnCancelAuctionButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execOnCancelAuctionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBidMenu.OnBidButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execOnBidButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBidMenu.OnSceneKeyPressed
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execOnSceneKeyPressed_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBidMenu.UpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execUpdateHighlight_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.UpdateBidListDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execUpdateBidListDisplay_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.ResizeDisplayList
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execResizeDisplayList_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBidMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBidMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAuctionBidMenu.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBidMenu_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBrowseMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.SetAuctionTimeStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBrowseMenu_eventSetAuctionTimeStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.SetAuctionItemSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBrowseMenu_eventSetAuctionItemSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.SetAuctionItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBrowseMenu_eventSetAuctionItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.SetRawInputKeyForType
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionBrowseMenu_eventSetRawInputKeyForType_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnNotifyEditBoxStateChanged
// [0x00024002] 
struct UTgUIAuctionBrowseMenu_execOnNotifyEditBoxStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnLowestPriceOnlyChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnLowestPriceOnlyChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnTypeCategoryClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnTypeCategoryClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnAuctionTimeStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnAuctionTimeStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnAuctionItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnAuctionItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnAuctionItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnAuctionItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnBidButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnBidButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnSearchButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnSearchButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnResetButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnResetButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.OnSceneKeyPressed
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execOnSceneKeyPressed_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.UpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execUpdateHighlight_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.UpdateBrowseListDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execUpdateBrowseListDisplay_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.ResizeDisplayList
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execResizeDisplayList_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionBrowseMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionBrowseMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIAuctionBrowseMenu.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionBrowseMenu_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIAuctionSellMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIAuctionSellMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIAuctionSellMenu.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIAuctionSellMenu.OnQualityComboChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnQualityComboChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionSellMenu.OnTypeComboChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnTypeComboChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIAuctionSellMenu.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIAuctionSellMenu.OnSortInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnSortInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionSellMenu.OnSelectInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnSelectInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionSellMenu.OnSellButton
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnSellButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionSellMenu.OnSceneKeyPressed
// [0x00420400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execOnSceneKeyPressed_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionSellMenu.UpdateInventoryHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execUpdateInventoryHighlight_Parms
{
};

// Function TgClient.TgUIAuctionSellMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIAuctionSellMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIAuctionSellMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIAuctionSellMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIAuctionSellMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICampaignMenu.SelectTab
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_eventSelectTab_Parms
{
	unsigned char                                      Tab;                                              		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	// class ATgHUD_Game*                              HUD;                                              		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICampaignMenu.OnSelectLeaderboardTab
// [0x00420002] 
struct UTgUICampaignMenu_execOnSelectLeaderboardTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.OnSelectCombatLogTab
// [0x00420002] 
struct UTgUICampaignMenu_execOnSelectCombatLogTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.OnSelectCombatTab
// [0x00420002] 
struct UTgUICampaignMenu_execOnSelectCombatTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.OnSelectHexMapTab
// [0x00420002] 
struct UTgUICampaignMenu_execOnSelectHexMapTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.OnCloseButton
// [0x00420002] 
struct UTgUICampaignMenu_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICampaignMenu.OnHexAssignmentUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execOnHexAssignmentUpdated_Parms
{
};

// Function TgClient.TgUICampaignMenu.OnHexAssignmentChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execOnHexAssignmentChanged_Parms
{
};

// Function TgClient.TgUICampaignMenu.GetAgencyData
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execGetAgencyData_Parms
{
	class UTgAgencyData*                               ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.OnHexDataUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execOnHexDataUpdated_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUICampaignMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUICampaignMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICampaignMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.SetSelectAttackerDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_Combat_eventSetSelectAttackerDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.SetSelectDefenderDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_Combat_eventSetSelectDefenderDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_Combat_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.OnAttackerSetSort
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnAttackerSetSort_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnAttackerJoin
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnAttackerJoin_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnSelectAttacker
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnSelectAttacker_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnAttackerFilterChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnAttackerFilterChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.AttackerUpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execAttackerUpdateHighlight_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.SelectAttacker
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execSelectAttacker_Parms
{
	int                                                nIndex;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.ResizeAttackerList
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execResizeAttackerList_Parms
{
	int                                                nSize;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.UpdateAttackerList
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execUpdateAttackerList_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.OnDefenderSetSort
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnDefenderSetSort_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnDefenderKickTeam
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnDefenderKickTeam_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnDefenderJoin
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnDefenderJoin_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnSelectDefender
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnSelectDefender_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnDefenderFilterChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnDefenderFilterChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.DefenderUpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execDefenderUpdateHighlight_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.SelectDefender
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execSelectDefender_Parms
{
	int                                                nIndex;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.ResizeDefenderList
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execResizeDefenderList_Parms
{
	int                                                nSize;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Combat.UpdateDefenderTimers
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execUpdateDefenderTimers_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.UpdateDefenderList
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execUpdateDefenderList_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_Combat.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Combat.OnACADataUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Combat_execOnACADataUpdated_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_CombatLog.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_CombatLog_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICampaignMenu_CombatLog.OnResetButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_CombatLog_execOnResetButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_CombatLog.OnSearchButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_CombatLog_execOnSearchButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_CombatLog.ResizeLogList
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_CombatLog_execResizeLogList_Parms
{
	int                                                nSize;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_CombatLog.ResetFilters
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_CombatLog_execResetFilters_Parms
{
};

// Function TgClient.TgUICampaignMenu_CombatLog.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_CombatLog_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_CombatLog.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_CombatLog_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnPopupButton
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnPopupButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnOpenTerritories
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnOpenTerritories_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexWithdraw
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnHexWithdraw_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexRaid
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnHexRaid_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexSteal
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnHexSteal_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexCapture
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnHexCapture_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexJoin
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnHexJoin_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexDefend
// [0x00420002] 
struct UTgUICampaignMenu_HexMap_execOnHexDefend_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_HexMap_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.OnNavButton
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnNavButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexBidCancel
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnHexBidCancel_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexBid
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnHexBid_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnZoneChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnZoneChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_HexMap.UpdateHexOptionDisplay
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execUpdateHexOptionDisplay_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.UpdateHexInformation
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execUpdateHexInformation_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.UpdateZoneList
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execUpdateZoneList_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.UpdateToolTipData
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execUpdateToolTipData_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.UpdateHexMap
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execUpdateHexMap_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexAssignmentUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnHexAssignmentUpdated_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.OnHexDataUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnHexDataUpdated_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OpenTerritories
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOpenTerritories_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.HexWithdraw
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execHexWithdraw_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.HexRaid
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execHexRaid_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.HexSteal
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execHexSteal_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.HexCapture
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execHexCapture_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.HexJoin
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execHexJoin_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.HexDefend
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execHexDefend_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.ClosePopupMessage
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execClosePopupMessage_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.OpenPopupMessage
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOpenPopupMessage_Parms
{
	int                                                nMsgId;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_HexMap.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execPostRender_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_HexMap.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_HexMap.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_HexMap_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_Leaderboard.SetRowMouseoverDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_Leaderboard_eventSetRowMouseoverDelegate_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Leaderboard.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICampaignMenu_Leaderboard_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICampaignMenu_Leaderboard.OnAgencyRowStateChange
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execOnAgencyRowStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Leaderboard.OnAgencyValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execOnAgencyValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICampaignMenu_Leaderboard.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUICampaignMenu_Leaderboard.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_Leaderboard.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICampaignMenu_Leaderboard.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICampaignMenu_Leaderboard.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUICampaignMenu_Leaderboard_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUICargoMenu.SetCargoItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUICargoMenu_eventSetCargoItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICargoMenu.SetDropshipItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUICargoMenu_eventSetDropshipItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICargoMenu.SetRawInputKeyForCargoList
// [0x00020802] ( FUNC_Event )
struct UTgUICargoMenu_eventSetRawInputKeyForCargoList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICargoMenu.SetRawInputKeyForMemberList
// [0x00020802] ( FUNC_Event )
struct UTgUICargoMenu_eventSetRawInputKeyForMemberList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICargoMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICargoMenu_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICargoMenu.SelectCargoByButton
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execSelectCargoByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.SelectMemberByButton
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execSelectMemberByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnAmountChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execOnAmountChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICargoMenu.OnCargoItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUICargoMenu_execOnCargoItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUICargoMenu.OnDropshipItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUICargoMenu_execOnDropshipItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUICargoMenu.OnCargoRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnCargoRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnCargoAddDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnCargoAddDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnDropshipRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnDropshipRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnDropshipAddDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnDropshipAddDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSelectSFTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSelectSFTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneMapDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneMapDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneManageDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneManageDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneDetailDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneDetailDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneRaidDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneRaidDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnCargoListDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnCargoListDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnTeamMemberDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnTeamMemberDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneLeaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneLeaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnScenePromoteDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnScenePromoteDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICargoMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUICargoMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICargoMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICargoMenu.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUICargoMenu_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.ClearModelData
// [0x00820802] ( FUNC_Event )
struct UTgUICharacterSelectScene_eventClearModelData_Parms
{
	// struct FCustomCharacterAssembly                 defaultcharassembly;                              		// 0x0000 (0x0044) [0x0000000000000000]              
	// struct FDeviceAttachmentAssembly                defaultdeviceassembly;                            		// 0x0044 (0x001C) [0x0000000000000000]              
	// int                                             I;                                                		// 0x0060 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICharacterSelectScene.UpdateCustomModel
// [0x00020802] ( FUNC_Event )
struct UTgUICharacterSelectScene_eventUpdateCustomModel_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.SelectCharacter
// [0x00020802] ( FUNC_Event )
struct UTgUICharacterSelectScene_eventSelectCharacter_Parms
{
	int                                                Index;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             oldSelection;                                     		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICharacterSelectScene.OpenCreateCharacterScreen
// [0x00020002] 
struct UTgUICharacterSelectScene_execOpenCreateCharacterScreen_Parms
{
	// class ATgLoginHUD*                              loginHUD;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICharacterSelectScene.OnDeleteCancel
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnDeleteCancel_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnErrorOK
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnErrorOK_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnDeleteOK
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnDeleteOK_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.CamZoomToggleButton
// [0x00420002] 
struct UTgUICharacterSelectScene_execCamZoomToggleButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnDeleteSelection
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnDeleteSelection_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnCreateSelection
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnCreateSelection_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectSelection
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectSelection_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnBackButton
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnBackButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgLoginHUD*                              loginHUD;                                         		// 0x0028 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter7
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter7_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter6
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter6_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter5
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter5_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter4
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter4_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter3
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter3_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter2
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter2_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter1
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter1_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectCharacter0
// [0x00420002] 
struct UTgUICharacterSelectScene_execOnSelectCharacter0_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICharacterSelectScene_eventFixupWidgetsUC_Parms
{
	// class ATgLoginHUD*                              loginHUD;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICharacterSelectScene.InitializeDataInterface
// [0x00020802] ( FUNC_Event )
struct UTgUICharacterSelectScene_eventInitializeDataInterface_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.OnSelectNext
// [0x00420400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execOnSelectNext_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSelectPrevious
// [0x00420400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execOnSelectPrevious_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.ValidateCharSlotFeature
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execValidateCharSlotFeature_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.OnDeleteFieldChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execOnDeleteFieldChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICharacterSelectScene.PopulateCurrentMorphSettings
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execPopulateCurrentMorphSettings_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.ZoomOnChar
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execZoomOnChar_Parms
{
	unsigned long                                      bZoomIn : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUICharacterSelectScene.UpdateStatusPanel
// [0x00024400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execUpdateStatusPanel_Parms
{
	unsigned long                                      bEnable : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	struct FString                                     DisplayString;                                    		// 0x0004 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUICharacterSelectScene.UpdateCharacterListCallback
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execUpdateCharacterListCallback_Parms
{
	int                                                eType;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICharacterSelectScene.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.TickDialogueBoxes
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execTickDialogueBoxes_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICharacterSelectScene.ClearCharacterData
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execClearCharacterData_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.ConfirmDeleteCharacter
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execConfirmDeleteCharacter_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.ConfirmSelectCharacter
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execConfirmSelectCharacter_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.UpdateShowHideWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execUpdateShowHideWidgets_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execFixupWidgets_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execPreCloseScene_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICharacterSelectScene.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUICharacterSelectScene_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUICraftScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICraftScene_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICraftScene.SetHoverCraftItemDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICraftScene_eventSetHoverCraftItemDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.SetSelectCraftItemDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICraftScene_eventSetSelectCraftItemDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.ValidateCraftingFeature
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execValidateCraftingFeature_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnPrevPage
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnPrevPage_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnNextPage
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnNextPage_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnCraftItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUICraftScene_execOnCraftItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUICraftScene.OnFilterChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execOnFilterChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.OnCraftButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnCraftButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnSortInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnSortInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnSelectCraftItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnSelectCraftItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICraftScene_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.ResizeCraftDetailsWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execResizeCraftDetailsWidgets_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.ResizeComponentPanelWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execResizeComponentPanelWidgets_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.SelectCraftItem
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execSelectCraftItem_Parms
{
	int                                                ItemIndex;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.PopulateCraftDetails
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execPopulateCraftDetails_Parms
{
};

// Function TgClient.TgUICraftScene.PopulateCraftItems
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execPopulateCraftItems_Parms
{
};

// Function TgClient.TgUICraftScene.ResizeBlueprintPanelWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execResizeBlueprintPanelWidgets_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICraftScene.BuildLocalCraftStore
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execBuildLocalCraftStore_Parms
{
};

// Function TgClient.TgUICraftScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUICraftScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICraftScene.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execPreCloseScene_Parms
{
};

// Function TgClient.TgUICraftScene.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICraftScene_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.ZoomIn
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventZoomIn_Parms
{
	// class ATgPlayerController*                      tgpc;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICustomHeadMenu.SetDetailButtonMouseOverDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSetDetailButtonMouseOverDelegate_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.SetDetailButtonDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSetDetailButtonDelegate_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.SetLockButtonDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSetLockButtonDelegate_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.SetGroupSliderValueChangeDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSetGroupSliderValueChangeDelegate_Parms
{
	class UUISlider*                                   slider;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.SetSliderValueChangeDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSetSliderValueChangeDelegate_Parms
{
	class UUISlider*                                   slider;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.GoToCharacterSelect
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventGoToCharacterSelect_Parms
{
	// class ATgLoginHUD*                              TgLoginHUD;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUICustomHeadMenu.SendSections
// [0x00820802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSendSections_Parms
{
	// struct FCustomCharacterAssembly                 newCustomAssembly;                                		// 0x0000 (0x0044) [0x0000000000000000]              
};

// Function TgClient.TgUICustomHeadMenu.SetCreateHeadVerifyState
// [0x00020802] ( FUNC_Event )
struct UTgUICustomHeadMenu_eventSetCreateHeadVerifyState_Parms
{
	unsigned char                                      stateIn;                                          		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.SaveMorphSettings
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execSaveMorphSettings_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.OnDetailButtonMouseOverDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnDetailButtonMouseOverDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.CreateHeadNextButton
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execCreateHeadNextButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnDetailButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnDetailButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnLockButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnLockButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnRandomAllButton
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnRandomAllButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnBackButton
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnBackButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.OnGroupSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnGroupSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.OnSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOnSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUICustomHeadMenu.OpenTutorialConfirmPopup
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execOpenTutorialConfirmPopup_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.PopulateMorphSliders
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execPopulateMorphSliders_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.BuildNodeData
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execBuildNodeData_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUICustomHeadMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUICustomHeadMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUICustomHeadMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIDeviceModPopup.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIDeviceModPopup_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIDeviceModPopup.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIDeviceModPopup_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIDeviceModPopup.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDeviceModPopup_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceModPopup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDeviceModPopup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIDeviceModPopup.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIDeviceModPopup_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIDeviceModPopup.OnConfirmNo
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceModPopup_execOnConfirmNo_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceModPopup.OnConfirmYes
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceModPopup_execOnConfirmYes_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIDeviceStats_eventFixupWidgetsUC_Parms
{
	// int                                             iField;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             iDev;                                             		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIDeviceStats.OnEndMissionButton
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnEndMissionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.OnMatchStatsButton
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnMatchStatsButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.OnSummaryButton
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnSummaryButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.OnDeviceItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnDeviceItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIDeviceStats.OnFieldClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnFieldClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUIDeviceStats_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIDeviceStats_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIDeviceStats.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDeviceStats_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIDeviceStats.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIDeviceStats_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIEndMissionScreen.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.OnUpgradeButton
// [0x00420400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnUpgradeButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.OnEndMissionButton
// [0x00420400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnEndMissionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.OnDeviceStatsButton
// [0x00420400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnDeviceStatsButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.OnMatchStatsButton
// [0x00420400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnMatchStatsButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.OnLootItemStateChange
// [0x00020400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execOnLootItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIEndMissionScreen.BeginFadeIn
// [0x00020400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execBeginFadeIn_Parms
{
};

// Function TgClient.TgUIEndMissionScreen.SetLabelTextAndColor
// [0x00020400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execSetLabelTextAndColor_Parms
{
	unsigned long                                      MissionVictor : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      PlayerVictor : 1;                                 		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bTie : 1;                                         		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIEndMissionScreen.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndMissionScreen.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIEndMissionScreen.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIEndMissionScreen_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIEndQuestMission.OnEndMissionButton
// [0x00420400] ( FUNC_Native )
struct UTgUIEndQuestMission_execOnEndMissionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndQuestMission.BeginFadeIn
// [0x00020400] ( FUNC_Native )
struct UTgUIEndQuestMission_execBeginFadeIn_Parms
{
};

// Function TgClient.TgUIEndQuestMission.SetLabelTextAndColor
// [0x00020400] ( FUNC_Native )
struct UTgUIEndQuestMission_execSetLabelTextAndColor_Parms
{
	unsigned long                                      MissionVictor : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      PlayerVictor : 1;                                 		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      bTie : 1;                                         		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function TgClient.TgUIEndQuestMission.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndQuestMission_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIEndQuestMission.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIEndQuestMission_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIGameHelpItems.OnSceneDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIGameHelpItems_execOnSceneDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIGameHelpItems.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIGameHelpItems_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIGameHelpItems.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIGameHelpItems_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIGameHelpItems.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIGameHelpItems_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIInventoryPopup.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIInventoryPopup_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIInventoryPopup.OnPrevPage
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnPrevPage_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnNextPage
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnNextPage_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIInventoryPopup.OnSortInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnSortInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnFilterChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnFilterChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIInventoryPopup.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnDeleteInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnDeleteInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnSelectInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnSelectInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnBlockTypeDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnBlockTypeDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnAddButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnAddButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIInventoryPopup_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInventoryPopup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIInventoryPopup.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInventoryPopup_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIInventoryPopup.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInventoryPopup_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIInventoryPopup.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIInventoryPopup_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIInventoryPopup.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIInventoryPopup_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIProductionPopup.OnAddButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIProductionPopup_execOnAddButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIProductionPopup.SetHex
// [0x00020400] ( FUNC_Native )
struct UTgUIProductionPopup_execSetHex_Parms
{
	int                                                nZoneObjectId;                                    		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                nPosition;                                        		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIProductionPopup.SetMaximumTechLevel
// [0x00020400] ( FUNC_Native )
struct UTgUIProductionPopup_execSetMaximumTechLevel_Parms
{
	int                                                nTechLevelValueId;                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIProductionPopup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIProductionPopup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIItemModification.SetOnKitMouseOver
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnKitMouseOver_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnKitClick
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnKitClick_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnKitItemClick
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnKitItemClick_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnKitItemMouseOver
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnKitItemMouseOver_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnDeviceListMouseOver
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnDeviceListMouseOver_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnDeviceListClick
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnDeviceListClick_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnDeviceItemClick
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnDeviceItemClick_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.SetOnDeviceItemMouseOver
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventSetOnDeviceItemMouseOver_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.OnCloseButton
// [0x00420002] 
struct UTgUIItemModification_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIItemModification_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIItemModification.OnFilterChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUIItemModification_execOnFilterChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIItemModification.OnKitClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnKitClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnKitMouseOver
// [0x00024400] ( FUNC_Native )
struct UTgUIItemModification_execOnKitMouseOver_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIItemModification.OnKitItemClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnKitItemClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnKitItemMouseOver
// [0x00024400] ( FUNC_Native )
struct UTgUIItemModification_execOnKitItemMouseOver_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIItemModification.OnDeviceListClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnDeviceListClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnDeviceListMouseOver
// [0x00024400] ( FUNC_Native )
struct UTgUIItemModification_execOnDeviceListMouseOver_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIItemModification.OnDeviceItemClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnDeviceItemClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnDeviceItemMouseOver
// [0x00024400] ( FUNC_Native )
struct UTgUIItemModification_execOnDeviceItemMouseOver_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIItemModification.OnPrevKitPageClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnPrevKitPageClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnNextKitPageClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnNextKitPageClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnPrevDevicePageClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnPrevDevicePageClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnNextDevicePageClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnNextDevicePageClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnCombineClick
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnCombineClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.OnItemModMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIItemModification_execOnItemModMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIItemModification.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIItemModification_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIItemModification.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIItemModification_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIItemModification.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIItemModification_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIItemModification.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIItemModification_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.SetRawInputKeyForMemberList
// [0x00020802] ( FUNC_Event )
struct UTgUILFGMenu_eventSetRawInputKeyForMemberList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILFGMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUILFGMenu_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUILFGMenu.OnLFGCriteriaChanged
// [0x00020400] ( FUNC_Native )
struct UTgUILFGMenu_execOnLFGCriteriaChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILFGMenu.SelectMemberByButton
// [0x00020400] ( FUNC_Native )
struct UTgUILFGMenu_execSelectMemberByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneCargoDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneCargoDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneDetailDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneDetailDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneStrikeForceDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneStrikeForceDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnTeamMemberDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnTeamMemberDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneLeaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneLeaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnScenePromoteDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnScenePromoteDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILFGMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILFGMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILFGMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILFGMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUILoginMenu.OnWeblinkKey
// [0x00420002] 
struct UTgUILoginMenu_execOnWeblinkKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnNotifyPasswordEditBoxStateChanged
// [0x00024002] 
struct UTgUILoginMenu_execOnNotifyPasswordEditBoxStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUILoginMenu.OnNotifyAccountEditBoxStateChanged
// [0x00024002] 
struct UTgUILoginMenu_execOnNotifyAccountEditBoxStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUILoginMenu.OnLoginSubmitText
// [0x00020002] 
struct UTgUILoginMenu_execOnLoginSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnSetFocusToPasswordEditBox
// [0x00420002] 
struct UTgUILoginMenu_execOnSetFocusToPasswordEditBox_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnSetFocusToAccountEditBox
// [0x00420002] 
struct UTgUILoginMenu_execOnSetFocusToAccountEditBox_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnEditBoxInputKey
// [0x00420002] 
struct UTgUILoginMenu_execOnEditBoxInputKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnCheckboxKey
// [0x00420002] 
struct UTgUILoginMenu_execOnCheckboxKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnSettingsCreditsButtonKey
// [0x00420400] ( FUNC_Native )
struct UTgUILoginMenu_execOnSettingsCreditsButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnCloseButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnCloseButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnDisconnectButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnDisconnectButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnQueueButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnQueueButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnQuitButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnQuitButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnLoginButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnLoginButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnStatusCancelButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnStatusCancelButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnStatusOKButtonKey
// [0x00420002] 
struct UTgUILoginMenu_execOnStatusOKButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUILoginMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.TickVisuals
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execTickVisuals_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILoginMenu.OnCrawlScrollOverride
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execOnCrawlScrollOverride_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              Nudges;                                           		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bMaxedOut : 1;                                    		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x000C (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.OnCrawlButtonClick
// [0x00420400] ( FUNC_Native )
struct UTgUILoginMenu_execOnCrawlButtonClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.StartWebLink
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execStartWebLink_Parms
{
};

// Function TgClient.TgUILoginMenu.OnCreateCharacter
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execOnCreateCharacter_Parms
{
};

// Function TgClient.TgUILoginMenu.HandleStatusCancel
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execHandleStatusCancel_Parms
{
};

// Function TgClient.TgUILoginMenu.HandleStatusOK
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execHandleStatusOK_Parms
{
};

// Function TgClient.TgUILoginMenu.UpdateSavedLoginInfo
// [0x00024400] ( FUNC_Native )
struct UTgUILoginMenu_execUpdateSavedLoginInfo_Parms
{
	unsigned long                                      bLog : 1;                                         		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUILoginMenu.UpdateStatusPanel
// [0x00024400] ( FUNC_Native )
struct UTgUILoginMenu_execUpdateStatusPanel_Parms
{
	unsigned long                                      bEnable : 1;                                      		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	struct FString                                     DisplayString;                                    		// 0x0004 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      bFadeOut : 1;                                     		// 0x0010 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUILoginMenu.TickStatusPanel
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execTickStatusPanel_Parms
{
};

// Function TgClient.TgUILoginMenu.TriggerCharCreate
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execTriggerCharCreate_Parms
{
};

// Function TgClient.TgUILoginMenu.TriggerLogin
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execTriggerLogin_Parms
{
};

// Function TgClient.TgUILoginMenu.IsProcessing
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execIsProcessing_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.GetProcessingState
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execGetProcessingState_Parms
{
	unsigned char                                      ReturnValue;                                      		// 0x0000 (0x0001) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.SetProcessingState
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execSetProcessingState_Parms
{
	unsigned char                                      NewState;                                         		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUILoginMenu.CloseQueuePanel
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execCloseQueuePanel_Parms
{
};

// Function TgClient.TgUILoginMenu.ProcessQueueButton
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execProcessQueueButton_Parms
{
};

// Function TgClient.TgUILoginMenu.ProcessDisconnect
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execProcessDisconnect_Parms
{
};

// Function TgClient.TgUILoginMenu.ToggleSaveAccountInfo
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execToggleSaveAccountInfo_Parms
{
};

// Function TgClient.TgUILoginMenu.ProcessQuit
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execProcessQuit_Parms
{
};

// Function TgClient.TgUILoginMenu.GetSavedAccountInfo
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execGetSavedAccountInfo_Parms
{
};

// Function TgClient.TgUILoginMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUILoginMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUILoginMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUILoginMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUILoginMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIMailMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIMailMenu.SetDetailAttachmentStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventSetDetailAttachmentStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.SetAttachmentStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventSetAttachmentStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.SetDetailAttachmentSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventSetDetailAttachmentSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.SetAttachmentSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventSetAttachmentSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.SetInboxItemSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventSetInboxItemSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.SetMailItemSelect
// [0x00020802] ( FUNC_Event )
struct UTgUIMailMenu_eventSetMailItemSelect_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.ValidateMailFeature
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execValidateMailFeature_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.SetFocusedWidget
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execSetFocusedWidget_Parms
{
	class UUIScreenObject*                             NewFocus;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OpenCODConfirmPopup
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execOpenCODConfirmPopup_Parms
{
	int                                                nCurrencyValue;                                   		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.SelectMailTab
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execSelectMailTab_Parms
{
	int                                                SelectTab;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.UpdateAttachmentHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execUpdateAttachmentHighlight_Parms
{
};

// Function TgClient.TgUIMailMenu.UpdateHighlight
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execUpdateHighlight_Parms
{
};

// Function TgClient.TgUIMailMenu.ResizeDisplayList
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execResizeDisplayList_Parms
{
	int                                                newSize;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execFixupWidgets_Parms
{
};

// Function TgClient.TgUIMailMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIMailMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIMailMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIMailMenu.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUIMailMenu.OnSendMoneyCheckboxChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execOnSendMoneyCheckboxChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.OnCODCheckboxChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execOnCODCheckboxChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.OnToSubmitText
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execOnToSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnToValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execOnToValueChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.OnEnterValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIMailMenu_execOnEnterValueChanged_Parms
{
	class UUIObject*                                   ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMailMenu.OnDetailAttachmentStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIMailMenu_execOnDetailAttachmentStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIMailMenu.OnAttachmentStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIMailMenu_execOnAttachmentStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIMailMenu.OnMailItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnMailItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnDetailAttachmentSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnDetailAttachmentSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnAttachmentSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnAttachmentSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnInboxItemSelect
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnInboxItemSelect_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnSelectMailTab
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnSelectMailTab_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnSendButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnSendButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnCancelButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnCancelButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnRemoveButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnRemoveButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnAttachButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnAttachButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnReplyButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnReplyButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnTakeButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnTakeButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnReturnButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnReturnButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnDeleteButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnDeleteButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMailMenu.OnMailMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUIMailMenu_execOnMailMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.ExitQuitMode
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execExitQuitMode_Parms
{
};

// Function TgClient.TgUIMainMenu.EnterQuitMode
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execEnterQuitMode_Parms
{
};

// Function TgClient.TgUIMainMenu.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIMainMenu_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.SetUpMenuButtons
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execSetUpMenuButtons_Parms
{
};

// Function TgClient.TgUIMainMenu.AddContextItem
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execAddContextItem_Parms
{
	int                                                funcID;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMainMenu.ConfigureContextItems
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execConfigureContextItems_Parms
{
};

// Function TgClient.TgUIMainMenu.CountContextItems
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execCountContextItems_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.GetButtonFunctionID
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execGetButtonFunctionID_Parms
{
	class UUIScreenObject*                             buttonObject;                                     		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.GetSceneReferenceByClassName
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execGetSceneReferenceByClassName_Parms
{
	struct FName                                       ClassName;                                        		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	int                                                ReturnValue;                                      		// 0x0008 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.GetBaseFunctionID
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execGetBaseFunctionID_Parms
{
	unsigned char                                      funcID;                                           		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      ReturnValue;                                      		// 0x0001 (0x0001) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.GetOverrideSceneReference
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execGetOverrideSceneReference_Parms
{
	unsigned char                                      funcID;                                           		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.CloseManagedScene
// [0x00024400] ( FUNC_Native )
struct UTgUIMainMenu_execCloseManagedScene_Parms
{
	unsigned long                                      bPreserveVolumeFlag : 1;                          		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIMainMenu.GetOpenManagedScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execGetOpenManagedScene_Parms
{
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.OpenManagedSceneByName
// [0x00024400] ( FUNC_Native )
struct UTgUIMainMenu_execOpenManagedSceneByName_Parms
{
	struct FName                                       ClassName;                                        		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bToggle : 1;                                      		// 0x0008 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bDevForce : 1;                                    		// 0x000C (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0010 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.OpenManagedScene
// [0x00024400] ( FUNC_Native )
struct UTgUIMainMenu_execOpenManagedScene_Parms
{
	int                                                sceneReference;                                   		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bToggle : 1;                                      		// 0x0004 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bDevForce : 1;                                    		// 0x0008 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	unsigned long                                      bIgnoreOverride : 1;                              		// 0x000C (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0010 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.BuildManagedSceneList
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execBuildManagedSceneList_Parms
{
};

// Function TgClient.TgUIMainMenu.OnButtonMouseOverDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUIMainMenu_execOnButtonMouseOverDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIMainMenu.OnMainMenuExitMissionKey
// [0x00420400] ( FUNC_Native )
struct UTgUIMainMenu_execOnMainMenuExitMissionKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.OnMainMenuQuitKey
// [0x00420400] ( FUNC_Native )
struct UTgUIMainMenu_execOnMainMenuQuitKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.OnFixedPurposeButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMainMenu_execOnFixedPurposeButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.OnMainMenuButtonKey
// [0x00420400] ( FUNC_Native )
struct UTgUIMainMenu_execOnMainMenuButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.OnMainMenuSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUIMainMenu_execOnMainMenuSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMainMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIMainMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMainMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIMissionScene.SetMissionReturn
// [0x00020802] ( FUNC_Event )
struct UTgUIMissionScene_eventSetMissionReturn_Parms
{
	// class ATgOmegaVolume*                           vol;                                              		// 0x0000 (0x0004) [0x0000000000000000]              
	// class ATgPawn*                                  MyPawn;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIMissionScene.SetIconButtonDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUIMissionScene_eventSetIconButtonDelegate_Parms
{
	class UUIButton*                                   btn;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMissionScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIMissionScene_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIMissionScene.CheckStopBriefing
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execCheckStopBriefing_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.GetQueueListCallback
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execGetQueueListCallback_Parms
{
	int                                                eType;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMissionScene.ToolTipDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUIMissionScene_execToolTipDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIMissionScene.OnButtonStateChangeDelegate
// [0x00024400] ( FUNC_Native )
struct UTgUIMissionScene_execOnButtonStateChangeDelegate_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIMissionScene.OnLocationPrevButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnLocationPrevButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnLocationNextButton
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnLocationNextButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnSelectDifficultyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnSelectDifficultyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnSelectTeamTypeDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnSelectTeamTypeDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnSelectGameTypeDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnSelectGameTypeDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnTabClickDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnTabClickDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnPreferenceChangeDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnPreferenceChangeDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnBriefingButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnBriefingButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnLeaveQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnLeaveQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnJoinQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnJoinQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnSelectQueueDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnSelectQueueDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.OnSceneKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_execOnSceneKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.SelectTeamType
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execSelectTeamType_Parms
{
	class UUIButton*                                   btn;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMissionScene.ShowMissionPage
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execShowMissionPage_Parms
{
	int                                                nPage;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMissionScene.UpdateInfoPanel
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execUpdateInfoPanel_Parms
{
};

// Function TgClient.TgUIMissionScene.TickVisual
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execTickVisual_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMissionScene.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execPostRender_Parms
{
};

// Function TgClient.TgUIMissionScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIMissionScene.RegisterCallbacks
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execRegisterCallbacks_Parms
{
};

// Function TgClient.TgUIMissionScene.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIMissionScene_Zone.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIMissionScene_Zone_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIMissionScene_Zone.OnTabClickDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execOnTabClickDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene_Zone.OnMapClickDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execOnMapClickDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene_Zone.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene_Zone.OnSceneKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execOnSceneKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene_Zone.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIMissionScene_Zone.TickVisual
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execTickVisual_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIMissionScene_Zone.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIMissionScene_Zone.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIMissionScene_Zone_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUINewPlayerHelp.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUINewPlayerHelp_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUINewPlayerHelp.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUINewPlayerHelp_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIQuestDialog.CheckTutorialTypeDisabled
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestDialog_execCheckTutorialTypeDisabled_Parms
{
	unsigned char                                      eType;                                            		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.CheckTutorialDisabled
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestDialog_execCheckTutorialDisabled_Parms
{
	unsigned char                                      Step;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.CheckTutorialStep
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestDialog_execCheckTutorialStep_Parms
{
	unsigned char                                      Step;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.UpdateTutorialProgress
// [0x00024400] ( FUNC_Native )
struct UTgUIQuestDialog_execUpdateTutorialProgress_Parms
{
	unsigned char                                      Step;                                             		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                NewTutorialType;                                  		// 0x0004 (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIQuestDialog.BeginTutorial
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestDialog_execBeginTutorial_Parms
{
	unsigned char                                      TutType;                                          		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIQuestDialog.ActiveHighlightChanged
// [0x00024400] ( FUNC_Native )
struct UTgUIQuestDialog_execActiveHighlightChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIQuestDialog.ActiveTooltipChanged
// [0x00024400] ( FUNC_Native )
struct UTgUIQuestDialog_execActiveTooltipChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIQuestDialog.DefaultSceneHandler
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestDialog_execDefaultSceneHandler_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.OnBuyNowButton
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestDialog_execOnBuyNowButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.OnQuestButton
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestDialog_execOnQuestButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.OnItemButton
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestDialog_execOnItemButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.OnDialogButton
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestDialog_execOnDialogButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestDialog_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestDialog.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestDialog_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIQuestDialog.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestDialog_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIQuestLog.ActiveTooltipChanged
// [0x00024400] ( FUNC_Native )
struct UTgUIQuestLog_execActiveTooltipChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIQuestLog.DefaultSceneHandler
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestLog_execDefaultSceneHandler_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestLog.OnSceneButton
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestLog_execOnSceneButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestLog.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestLog_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestLog.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIQuestLog_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIQuestLog.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIQuestLog_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIRewardScreen.OnCloseButton
// [0x00420002] 
struct UTgUIRewardScreen_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRewardScreen.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIRewardScreen_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIRewardScreen.RevealHiddenReward
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execRevealHiddenReward_Parms
{
};

// Function TgClient.TgUIRewardScreen.SetTimeToReveal
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execSetTimeToReveal_Parms
{
};

// Function TgClient.TgUIRewardScreen.BeginAutoDismiss
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execBeginAutoDismiss_Parms
{
};

// Function TgClient.TgUIRewardScreen.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIRewardScreen.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIRewardScreen.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIRewardScreen.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIRewardScreen_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIRewardScreen.OnSceneKeyPressed
// [0x00420400] ( FUNC_Native )
struct UTgUIRewardScreen_execOnSceneKeyPressed_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIReceivedItemScreen.SetItemStateChange
// [0x00020802] ( FUNC_Event )
struct UTgUIReceivedItemScreen_eventSetItemStateChange_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIReceivedItemScreen.OnItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIReceivedItemScreen_execOnItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIReceivedItemScreen.RevealHiddenReward
// [0x00020400] ( FUNC_Native )
struct UTgUIReceivedItemScreen_execRevealHiddenReward_Parms
{
};

// Function TgClient.TgUIReceivedItemScreen.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIReceivedItemScreen_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIReceivedItemScreen.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIReceivedItemScreen_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIReceivedItemScreen.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUIReceivedItemScreen_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUISettingsMenu_General_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.OnHitEffectFullChanged
// [0x00020002] 
struct UTgUISettingsMenu_General_execOnHitEffectFullChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_General.OnHitEffectZoomChanged
// [0x00020002] 
struct UTgUISettingsMenu_General_execOnHitEffectZoomChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_General.ActiveStateChanged
// [0x00024400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execActiveStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_General.OnSiteChanged
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnSiteChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_General.OnMouseEditBoxStateChanged
// [0x00024400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnMouseEditBoxStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_General.OnMouseSliderChanged
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnMouseSliderChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_General.OnMouseSensitivitySubmit
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnMouseSensitivitySubmit_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_General.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_General.OnTabClickedDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnTabClickedDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_General.OnApplyButtonKey
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execOnApplyButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_General.PopulateSiteList
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execPopulateSiteList_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.PopulateQueueSettingWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execPopulateQueueSettingWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.PopulateHUDVisibilityWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execPopulateHUDVisibilityWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.PopulateWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execPopulateWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.ApplyHUDVisibilitySettings
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execApplyHUDVisibilitySettings_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.ApplyPrimaryHUDSettings
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execApplyPrimaryHUDSettings_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.ApplyControllerSettings
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execApplyControllerSettings_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.ApplyMouseSettings
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execApplyMouseSettings_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execFixupWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_General.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_General.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_General_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUISettingsMenu_Sound_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.OnEnableAudioChanged
// [0x00020002] 
struct UTgUISettingsMenu_Sound_execOnEnableAudioChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Sound.ShowHideAudioPanel
// [0x00020002] 
struct UTgUISettingsMenu_Sound_execShowHideAudioPanel_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// unsigned long                                   bEnableAudio : 1;                                 		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
};

// Function TgClient.TgUISettingsMenu_Sound.OnEnableVoipChanged
// [0x00020002] 
struct UTgUISettingsMenu_Sound_execOnEnableVoipChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Sound.ShowHideVoipPanel
// [0x00020002] 
struct UTgUISettingsMenu_Sound_execShowHideVoipPanel_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// unsigned long                                   bEnableVoip : 1;                                  		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
};

// Function TgClient.TgUISettingsMenu_Sound.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Sound.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Sound.OpenRestartConfirmPopup
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execOpenRestartConfirmPopup_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Sound.OnTabClickedDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execOnTabClickedDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Sound.OnApplyButtonKey
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execOnApplyButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Sound.ApplyVoiceSliders
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execApplyVoiceSliders_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.ApplyGameSliders
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execApplyGameSliders_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.PopulateWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execPopulateWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execFixupWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Sound.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execPreCloseScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Sound.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Sound_execPostOpenScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Video.OnGammaCorrectionSliderChanged
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execOnGammaCorrectionSliderChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Video.ActiveStateChanged
// [0x00024400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execActiveStateChanged_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Video.CBoxChanged
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execCBoxChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Video.ConfirmHandler
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execConfirmHandler_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Video.ButtonHandler
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execButtonHandler_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Video.TabHandler
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execTabHandler_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Video.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Video.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Video.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Video_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUITeamDetailMenu.SetRawInputKeyForMemberList
// [0x00020802] ( FUNC_Event )
struct UTgUITeamDetailMenu_eventSetRawInputKeyForMemberList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUITeamDetailMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUITeamDetailMenu_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUITeamDetailMenu.SelectMemberByButton
// [0x00020400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execSelectMemberByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneMapDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneMapDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneManageDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneManageDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneCargoDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneCargoDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneTeamDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneTeamDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneRaidDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneRaidDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneStrikeForceDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneStrikeForceDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnTeamMemberDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnTeamMemberDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneLeaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneLeaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnScenePromoteDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnScenePromoteDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamDetailMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUITeamDetailMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUITeamMenu.SetRawInputKeyForMemberList
// [0x00020802] ( FUNC_Event )
struct UTgUITeamMenu_eventSetRawInputKeyForMemberList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUITeamMenu.SetRawInputKeyForPlayerList
// [0x00020802] ( FUNC_Event )
struct UTgUITeamMenu_eventSetRawInputKeyForPlayerList_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUITeamMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUITeamMenu_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUITeamMenu.GetPlayerSearchCallback
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execGetPlayerSearchCallback_Parms
{
	int                                                eType;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUITeamMenu.SelectPlayerByButton
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execSelectPlayerByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.SelectMemberByButton
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execSelectMemberByButton_Parms
{
	class UUIButton*                                   Button;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneSortDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneSortDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneMapDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneMapDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneManageDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneManageDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneDetailDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneDetailDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneCargoDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneCargoDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneRaidDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneRaidDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneStrikeForceDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneStrikeForceDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSelectPlayer
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSelectPlayer_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnTeamMemberDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnTeamMemberDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneLeaveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneLeaveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnScenePromoteDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnScenePromoteDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneRemoveDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneRemoveDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneMessageDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneMessageDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneInviteDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneInviteDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneSearchDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneSearchDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneResetDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneResetDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUITeamMenu_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.SortSearchResults
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execSortSearchResults_Parms
{
};

// Function TgClient.TgUITeamMenu.Search
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execSearch_Parms
{
};

// Function TgClient.TgUITeamMenu.SearchFromFields
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execSearchFromFields_Parms
{
};

// Function TgClient.TgUITeamMenu.ResetFields
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execResetFields_Parms
{
};

// Function TgClient.TgUITeamMenu.PrepopulatedSearchById
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execPrepopulatedSearchById_Parms
{
	int                                                nPlayerId;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUITeamMenu.PrepopulatedSearch
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execPrepopulatedSearch_Parms
{
	struct FString                                     sPlayerName;                                      		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sAgencyName;                                      		// 0x000C (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     sAllianceName;                                    		// 0x0018 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUITeamMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITeamMenu.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUITeamMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUITeamMenu.PostInitSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUITeamMenu_execPostInitSceneDriver_Parms
{
};

// Function TgClient.TgUITutorialItem.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUITutorialItem_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITutorialItem.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUITutorialItem_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUITutorialItem.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUITutorialItem_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIVendorBase.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorBase_execPostOpenScene_Parms
{
};

// Function TgClient.TgUISteamTxnStatus.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISteamTxnStatus_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISteamTxnStatus.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUISteamTxnStatus_execPreCloseScene_Parms
{
};

// Function TgClient.TgUISteamTxnStatus.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUISteamTxnStatus_execPostOpenScene_Parms
{
};

// Function TgClient.TgUISteamTxnStatus.OnButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUISteamTxnStatus_execOnButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnInkSliderChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnInkSliderChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnGroupSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnGroupSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnDetailButtonMouseOver
// [0x00024400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnDetailButtonMouseOver_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnRandomButton
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnRandomButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnDetailButton
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnDetailButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnLockButton
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnLockButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnSubmitButton
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnSubmitButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnCancelButton
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnCancelButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.UpdateCreditsInfo
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execUpdateCreditsInfo_Parms
{
};

// Function TgClient.TgUIUpdateFaceMenu.CompleteTransaction
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execCompleteTransaction_Parms
{
};

// Function TgClient.TgUIUpdateFaceMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIUpdateFaceMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateFaceMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateFaceMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.OnSubmitButton
// [0x00420002] 
struct UTgUIUpdateHairMenu_execOnSubmitButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateHairMenu.OnCancelButton
// [0x00420002] 
struct UTgUIUpdateHairMenu_execOnCancelButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateHairMenu.OnCloseButton
// [0x00420002] 
struct UTgUIUpdateHairMenu_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateHairMenu.InitializeUpdateHairMenu
// [0x00020800] ( FUNC_Event )
struct UTgUIUpdateHairMenu_eventInitializeUpdateHairMenu_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIUpdateHairMenu_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.OnConfirmNoClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnConfirmNoClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateHairMenu.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateHairMenu.CompleteTransaction
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execCompleteTransaction_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.UpdateCreditsInfo
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execUpdateCreditsInfo_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.UpdateExclusiveLabelStatus
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execUpdateExclusiveLabelStatus_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.UpdateSliderDisableStatus
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execUpdateSliderDisableStatus_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.OnSecondaryHairColorSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnSecondaryHairColorSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateHairMenu.OnHairAccessoryColorSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnHairAccessoryColorSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateHairMenu.OnFacialHairSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnFacialHairSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateHairMenu.OnHairColorSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnHairColorSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateHairMenu.OnHairStyleSliderValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execOnHairStyleSliderValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUpdateHairMenu.SubmitButtonPressed
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execSubmitButtonPressed_Parms
{
};

// Function TgClient.TgUIUpdateHairMenu.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUpdateHairMenu.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUpdateHairMenu_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIVendorPaymentPopup.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorPaymentPopup_execPreCloseScene_Parms
{
};

// Function TgClient.TgUIVendorPaymentPopup.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorPaymentPopup_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIVendorPaymentPopup.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIVendorPaymentPopup_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIVendorPaymentPopup.OnButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorPaymentPopup_execOnButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIVendorScene_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIVendorScene.OnTogglePreviewInvButton
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnTogglePreviewInvButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnResetPreviewButton
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnResetPreviewButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnVendorPrevButton
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnVendorPrevButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnVendorNextButton
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnVendorNextButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnInvItemStateChange
// [0x00024400] ( FUNC_Native )
struct UTgUIVendorScene_execOnInvItemStateChange_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    NewlyActiveState;                                 		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class UUIState*                                    PreviouslyActiveState;                            		// 0x000C (0x0004) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIVendorScene.OnResetDoubleClick
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorScene_execOnResetDoubleClick_Parms
{
	class UUIScreenObject*                             Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIVendorScene.OnFilterChangedDelegate
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorScene_execOnFilterChangedDelegate_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIVendorScene.OnConfirmYesClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnConfirmYesClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnSortVendorDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnSortVendorDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnSortInventoryDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnSortInventoryDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnDeleteInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnDeleteInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnCloseButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnCloseButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnSelectInventoryItemDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnSelectInventoryItemDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnVendorItemSellDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnVendorItemSellDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnVendorItemPurchaseDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnVendorItemPurchaseDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnVendorItemSelectDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnVendorItemSelectDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.OnSceneInputKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIVendorScene_execOnSceneInputKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.PostRender
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorScene_execPostRender_Parms
{
};

// Function TgClient.TgUIVendorScene.PostOpenScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorScene_execPostOpenScene_Parms
{
};

// Function TgClient.TgUIVendorScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVendorScene.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVendorScene_execPreCloseScene_Parms
{
};

// Function TgClient.TgUICashShop.OnVendorItemPurchaseDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUICashShop_execOnVendorItemPurchaseDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreboard.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIScoreboard_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIScoreboard.PopulateStructsWithUIPointers
// [0x00020400] ( FUNC_Native )
struct UTgUIScoreboard_execPopulateStructsWithUIPointers_Parms
{
};

// Function TgClient.TgUIScoreboard.UpdateLabels
// [0x00020400] ( FUNC_Native )
struct UTgUIScoreboard_execUpdateLabels_Parms
{
};

// Function TgClient.TgUIScoreboard.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreboard_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreboard.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIScoreboard_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreboard.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIScoreboard_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIScoreDetails.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIScoreDetails_eventFixupWidgetsUC_Parms
{
	// int                                             iField;                                           		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIScoreDetails.OnEndMissionButton
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreDetails_execOnEndMissionButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.OnDeviceStatsButton
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreDetails_execOnDeviceStatsButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.OnSummaryButton
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreDetails_execOnSummaryButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreDetails_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.OnFieldClicked
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreDetails_execOnFieldClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.OnSceneKey
// [0x00420400] ( FUNC_Native )
struct UTgUIScoreDetails_execOnSceneKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIScoreDetails_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIScoreDetails.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIScoreDetails_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISelectionList.SelectSelectionRow9
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow9_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow8
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow8_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow7
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow7_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow6
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow6_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow5
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow5_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow4
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow4_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow3
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow3_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow2
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow2_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow1
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow1_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.SelectSelectionRow0
// [0x00420002] 
struct UTgUISelectionList_execSelectSelectionRow0_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.ModifyScrollPos
// [0x00020002] 
struct UTgUISelectionList_execModifyScrollPos_Parms
{
	int                                                nTimerId;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      eEvent;                                           		// 0x0004 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	// int                                             nMaxScrollPos;                                    		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUISelectionList.ScrollListUp
// [0x00420002] 
struct UTgUISelectionList_execScrollListUp_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.ScrollListDown
// [0x00420002] 
struct UTgUISelectionList_execScrollListDown_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUISelectionList_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUISelectionList.InitSelectionListScene
// [0x00020802] ( FUNC_Event )
struct UTgUISelectionList_eventInitSelectionListScene_Parms
{
};

// Function TgClient.TgUISelectionList.SelectSelectionRow
// [0x00020002] 
struct UTgUISelectionList_execSelectSelectionRow_Parms
{
	struct FName                                       Key;                                              		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned char                                      Event;                                            		// 0x0008 (0x0001) [0x0000000000000080]              ( CPF_Parm )
	int                                                nIndex;                                           		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0010 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.PreCloseScene
// [0x00020400] ( FUNC_Native )
struct UTgUISelectionList_execPreCloseScene_Parms
{
};

// Function TgClient.TgUISelectionList.DrawGrid
// [0x00020400] ( FUNC_Native )
struct UTgUISelectionList_execDrawGrid_Parms
{
};

// Function TgClient.TgUISelectionList.GetSelectionCallback
// [0x00020400] ( FUNC_Native )
struct UTgUISelectionList_execGetSelectionCallback_Parms
{
};

// Function TgClient.TgUISelectionList.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISelectionList_execFixupWidgets_Parms
{
};

// Function TgClient.TgUISelectionList.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISelectionList_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISelectionList.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISelectionList_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Chat.OnSubmitText
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execOnSubmitText_Parms
{
	class UUIEditBox*                                  ebox;                                             		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0008 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Chat.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Chat.OnEboxClicked
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execOnEboxClicked_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Chat.OnTabClickedDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execOnTabClickedDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Chat.OnApplyButtonKey
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execOnApplyButtonKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Chat.OnOpacityValueChanged
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execOnOpacityValueChanged_Parms
{
	class UUIObject*                                   Sender;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                PlayerIndex;                                      		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Chat.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Chat_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUISettingsMenu_Controls_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.SetKeyBindSelectDelegate
// [0x00020802] ( FUNC_Event )
struct UTgUISettingsMenu_Controls_eventSetKeyBindSelectDelegate_Parms
{
	class UUIScreenObject*                             Obj;                                              		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Controls.SetBind
// [0x00824802] ( FUNC_Event )
struct UTgUISettingsMenu_Controls_eventSetBind_Parms
{
	struct FName                                       BindName;                                         		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	struct FString                                     Command;                                          		// 0x0008 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      altKey : 1;                                       		// 0x0014 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
	// struct FKeyBind                                 NewBind;                                          		// 0x0018 (0x0018) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgUISettingsMenu_Controls.OnBindingsUpdated
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnBindingsUpdated_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.GetBind
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execGetBind_Parms
{
	struct FName                                       Key;                                              		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      bAlt : 1;                                         		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	struct FString                                     ReturnValue;                                      		// 0x000C (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgUISettingsMenu_Controls.GetPreviousCommandName
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execGetPreviousCommandName_Parms
{
	struct FString                                     currentBinding;                                   		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     ReturnValue;                                      		// 0x000C (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function TgClient.TgUISettingsMenu_Controls.UnbindCommand
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execUnbindCommand_Parms
{
	struct FString                                     Command;                                          		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function TgClient.TgUISettingsMenu_Controls.CaptureKeyToBind
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execCaptureKeyToBind_Parms
{
	struct FName                                       KeyName;                                          		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUISettingsMenu_Controls.DisableBinding
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execDisableBinding_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.EnableBinding
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execEnableBinding_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.CreateWidgetsFromBindData
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execCreateWidgetsFromBindData_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.PopulateBindDataArray
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execPopulateBindDataArray_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.PopulateWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execPopulateWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.FixupWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execFixupWidgets_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISettingsMenu_Controls.OnTabClickedDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnTabClickedDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.OnCloseButton
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnCloseButton_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.OnKeyBindConfirmNo
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnKeyBindConfirmNo_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.OnKeyBindConfirmYes
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnKeyBindConfirmYes_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.OnKeyBindBase
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnKeyBindBase_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.OnKeyBindDefaults
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnKeyBindDefaults_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISettingsMenu_Controls.OnKeyBindMenuKey
// [0x00420400] ( FUNC_Native )
struct UTgUISettingsMenu_Controls_execOnKeyBindMenuKey_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUnlockScene.RequestPlayersSkillsFromServer
// [0x00020802] ( FUNC_Event )
struct UTgUIUnlockScene_eventRequestPlayersSkillsFromServer_Parms
{
	class ATgPawn_Character*                           MyPawn;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgUIUnlockScene.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIUnlockScene_eventFixupWidgetsUC_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIUnlockScene.OnNavButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIUnlockScene_execOnNavButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUnlockScene.OnEquipButtonDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIUnlockScene_execOnEquipButtonDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUnlockScene.OnSceneKeyDelegate
// [0x00420400] ( FUNC_Native )
struct UTgUIUnlockScene_execOnSceneKeyDelegate_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUnlockScene.GetPOIByMsgId
// [0x00020400] ( FUNC_Native )
struct UTgUIUnlockScene_execGetPOIByMsgId_Parms
{
	int                                                msgId;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class ATgPointOfInterest*                          ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUnlockScene.ProcessUnlockData
// [0x00020400] ( FUNC_Native )
struct UTgUIUnlockScene_execProcessUnlockData_Parms
{
};

// Function TgClient.TgUIUnlockScene.CheckLevelUnlocks
// [0x00024400] ( FUNC_Native )
struct UTgUIUnlockScene_execCheckLevelUnlocks_Parms
{
	unsigned long                                      bNewLevel : 1;                                    		// 0x0000 (0x0004) [0x0000000000000090] [0x00000001] ( CPF_OptionalParm | CPF_Parm )
};

// Function TgClient.TgUIUnlockScene.PopulateUnlockWidgets
// [0x00020400] ( FUNC_Native )
struct UTgUIUnlockScene_execPopulateUnlockWidgets_Parms
{
};

// Function TgClient.TgUIUnlockScene.TickTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUnlockScene_execTickTgUIScene_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIUnlockScene.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIUnlockScene_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUIVoiceSetup.OnApplyButtonClick
// [0x00420002] 
struct UTgUIVoiceSetup_execOnApplyButtonClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class ATgPlayerController*                      PC;                                               		// 0x0028 (0x0004) [0x0000000000000000]              
	// class UTgGameViewportClient*                    vpc;                                              		// 0x002C (0x0004) [0x0000000000000000]              
	// class ATgHUD_Game*                              HUD;                                              		// 0x0030 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgUIVoiceSetup.OnCloseButtonClick
// [0x00420002] 
struct UTgUIVoiceSetup_execOnCloseButtonClick_Parms
{
	struct FInputEventParameters                       EventParms;                                       		// 0x0000 (0x0024) [0x0000000000000182]              ( CPF_Const | CPF_Parm | CPF_OutParm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0024 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUIVoiceSetup.FixupWidgetsUC
// [0x00020802] ( FUNC_Event )
struct UTgUIVoiceSetup_eventFixupWidgetsUC_Parms
{
};

// Function TgClient.TgUIVoiceSetup.FixupTgUIScene
// [0x00020400] ( FUNC_Native )
struct UTgUIVoiceSetup_execFixupTgUIScene_Parms
{
};

// Function TgClient.TgUISceneManager.TickOpenScenes
// [0x00020400] ( FUNC_Native )
struct UTgUISceneManager_execTickOpenScenes_Parms
{
};

// Function TgClient.TgUISceneManager.ReleaseSceneDriverIfNotOpened
// [0x00020400] ( FUNC_Native )
struct UTgUISceneManager_execReleaseSceneDriverIfNotOpened_Parms
{
	class UTgUISceneDriver*                            SceneDriver;                                      		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneManager.GetSceneDriverNameByResource
// [0x00020400] ( FUNC_Native )
struct UTgUISceneManager_execGetSceneDriverNameByResource_Parms
{
	struct FString                                     ResourceName;                                     		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FName                                       ReturnValue;                                      		// 0x000C (0x0008) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneManager.GetSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUISceneManager_execGetSceneDriver_Parms
{
	struct FName                                       DriverClass;                                      		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	unsigned long                                      InitOnDemand : 1;                                 		// 0x0008 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgUISceneManager.OpenSceneDriver
// [0x00020400] ( FUNC_Native )
struct UTgUISceneManager_execOpenSceneDriver_Parms
{
	struct FName                                       DriverClass;                                      		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
	class UTgUISceneDriver*                            ReturnValue;                                      		// 0x0008 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function TgClient.TgEntryHUD.DrawHUD
// [0x00020002] 
struct ATgEntryHUD_execDrawHUD_Parms
{
};

// Function TgClient.TgHUD_TeamGame.DrawCol
// [0x00020102] 
struct ATgHUD_TeamGame_execDrawCol_Parms
{
	float                                              X1;                                               		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              X2;                                               		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	float                                              Y;                                                		// 0x0008 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	class ATgRepInfo_Player*                           prep;                                             		// 0x000C (0x0004) [0x0000000000000080]              ( CPF_Parm )
	// float                                           XL;                                               		// 0x0010 (0x0004) [0x0000000000000000]              
	// float                                           YL;                                               		// 0x0014 (0x0004) [0x0000000000000000]              
	// struct FString                                  S;                                                		// 0x0018 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function TgClient.TgHUD_TeamGame.DrawTeams
// [0x00020002] 
struct ATgHUD_TeamGame_execDrawTeams_Parms
{
	// int                                             I;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             ItemsPerPage;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	// int                                             maxRowCount;                                      		// 0x0008 (0x0004) [0x0000000000000000]              
	// int                                             ScrollTop;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	// class ATgRepInfo_Game*                          GRI;                                              		// 0x0010 (0x0004) [0x0000000000000000]              
	// float                                           LeftPos;                                          		// 0x0014 (0x0004) [0x0000000000000000]              
	// float                                           TopPos;                                           		// 0x0018 (0x0004) [0x0000000000000000]              
	// float                                           RightPos;                                         		// 0x001C (0x0004) [0x0000000000000000]              
	// float                                           Width;                                            		// 0x0020 (0x0004) [0x0000000000000000]              
	// float                                           Height;                                           		// 0x0024 (0x0004) [0x0000000000000000]              
	// float                                           XL;                                               		// 0x0028 (0x0004) [0x0000000000000000]              
	// float                                           YL;                                               		// 0x002C (0x0004) [0x0000000000000000]              
	// float                                           W;                                                		// 0x0030 (0x0004) [0x0000000000000000]              
	// float                                           H;                                                		// 0x0034 (0x0004) [0x0000000000000000]              
	// float                                           Scale;                                            		// 0x0038 (0x0004) [0x0000000000000000]              
	// float                                           InitLeftPos;                                      		// 0x003C (0x0004) [0x0000000000000000]              
	// float                                           ColWidth;                                         		// 0x0040 (0x0004) [0x0000000000000000]              
	// int                                             taskCount;                                        		// 0x0044 (0x0004) [0x0000000000000000]              
	// int                                             Index;                                            		// 0x0048 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgHUD_TeamGame.DrawHUD
// [0x00020002] 
struct ATgHUD_TeamGame_execDrawHUD_Parms
{
};

// Function TgClient.TgLoginPawn.StartUI
// [0x00020002] 
struct ATgLoginPawn_execStartUI_Parms
{
	// class ATgLoginPlayerController*                 P;                                                		// 0x0000 (0x0004) [0x0000000000000000]              
	// class ATgLoginHUD*                              HUD;                                              		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function TgClient.TgLoginPawn.OutsideWorldBounds
// [0x00020920] ( FUNC_Event )
struct ATgLoginPawn_eventOutsideWorldBounds_Parms
{
};

// Function TgClient.TgLoginPawn.FellOutOfWorld
// [0x00020900] ( FUNC_Event )
struct ATgLoginPawn_eventFellOutOfWorld_Parms
{
	class UClass*                                      dmgType;                                          		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgLoginPawn.PostBeginPlay
// [0x00020102] 
struct ATgLoginPawn_execPostBeginPlay_Parms
{
};

// Function TgClient.TgLoginPawn.Tick
// [0x00020802] ( FUNC_Event )
struct ATgLoginPawn_eventTick_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function TgClient.TgLoginPlayerCamera.PostBeginPlay
// [0x00020002] 
struct ATgLoginPlayerCamera_execPostBeginPlay_Parms
{
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif