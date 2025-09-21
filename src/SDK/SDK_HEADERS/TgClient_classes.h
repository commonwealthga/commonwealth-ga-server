/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: TgClient_classes.h
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
# Constants
# ========================================================================================= #
*/

#define CONST_AGENCYFILTER_OFFLINE                               0x1
#define CONST_AGENCYFILTER_ONLINE                                0x2
#define CONST_TG_AGENCY_REQUEST_THROTTLE_TIMER                   2.0f
#define CONST_MAX_RANK_NAME_LENGTH                               25
#define CONST_TG_ALLIANCE_REQUEST_THROTTLE_TIMER                 5.0f
#define CONST_MATCH_TYPE_PVE                                     1021
#define CONST_MATCH_TYPE_PVP                                     1022
#define CONST_MATCH_TYPE_TRAINING                                1188
#define CONST_TG_CLASS_LIST_DATASTORE_NAME                       "ClassesList"
#define CONST_TG_ANY_YES_NO_DATASTORE_NAME                       "AnyYesNoList"
#define CONST_NUM_OBJECTIVES_TO_POSTRENDER                       6
#define CONST_ED_LEFT_MARGIN                                     5
#define CONST_ED_LEFT_MARGIN_OFFSET                              150
#define CONST_ED_LINE_SPACE                                      12
#define CONST_ED_TOP_MARGIN                                      200
#define CONST_ED_TAB                                             120
#define CONST_NBR_PROFILE_BUTTONS                                5
#define CONST_MENU_THROTTLE_TIME                                 0.5f
#define CONST_NUM_VARIABLE_BUTTONS                               10
#define CONST_TGSCBD_REQUEST_INTERVAL                            1
#define CONST_MAX_PLAYERS_PER_MATCH                              24
#define CONST_MAX_PLAYERS_PER_TEAM                               12
#define CONST_SCORE_FIELDS                                       12
#define CONST_NUM_RANKED_FIELDS                                  10
#define CONST_CLASS_SCORE_ASSAULT                                0
#define CONST_CLASS_SCORE_MEDIC                                  1
#define CONST_CLASS_SCORE_RECON                                  2
#define CONST_CLASS_SCORE_ROBOTICS                               3
#define CONST_LUA_ASSUMED_ICONS_PER_ROWCOL                       5
#define CONST_EQ_DOLL_ROTATION_VALUE                             7.0f
#define CONST_TG_MIN_ALLIANCE_PLAYERS                            20
#define CONST_TG_MAX_ENTRIES_PER_PAGE                            50
#define CONST_TG_AGENCY_RESOURCE_EVENTS_VVG                      140
#define CONST_NUM_OF_CATEGORY_PER_PAGE                           22
#define CONST_NUM_OF_ACHIEVEMENT_PER_PAGE                        9
#define CONST_AP_NBR_OF_EQUIPPED_FLAIR                           8
#define CONST_AP_NBR_OF_EQUIPPED_UPGRADES                        7
#define CONST_AP_NBR_OF_EQUIPPED_DEVICES                         9
#define CONST_MAX_INV_INLIST                                     18
#define CONST_MAX_COL_SKILLS                                     4
#define CONST_MAX_ROW_SKILLS                                     7
#define CONST_MAX_TREE_SKILLS                                    3
#define CONST_NUM_OF_CRAFTING_SKILLS                             6
#define CONST_NUM_CRAFTING_SKILLS                                16
#define CONST_NUM_SKILL_STARS                                    5
#define CONST_NUM_OF_SKILL_STARS                                 5
#define CONST_NUM_OF_FLAIR_SETS                                  2
#define CONST_NUM_OF_BADGE_SETS                                  2
#define CONST_NUM_OF_FLAIRS_PER_SET                              15
#define CONST_NUM_OF_BADGES_PER_SET                              8
#define CONST_NUM_OF_SUBTABS                                     2
#define CONST_NUM_OF_ICONTABS                                    2
#define CONST_STAR_TEX_WID                                       182
#define CONST_STAR_TEX_HGT                                       36
#define CONST_NUM_OF_BOOSTS                                      4
#define CONST_TG_AUCTION_DEPOSIT                                 4
#define CONST_AUCTION_BID_REFRESH_TIME                           15.0f
#define CONST_AUCTION_BROWSE_REFRESH_TIME                        15.0f
#define CONST_TG_ITEM_QUALITY_TYPE_VVG                           133
#define CONST_TG_ITEM_TYPE_VVG                                   50
#define CONST_TG_BLUEPRINT_SUBTYPE_VVG                           143
#define CONST_TG_HEXITEM_SUBTYPE_VVG                             142
#define CONST_TG_HEXBOARD_SUBTYPE_VVG                            148
#define CONST_TG_UPGRADE_SUBTYPE_VVG                             106
#define CONST_TG_COMPONENT_SUBTYPE_VVG                           149
#define CONST_TG_MAX_NUM_INV_ITEMS                               18
#define CONST_TG_AUCTION_DURATION_VVG                            121
#define CONST_TG_ITEM_COMBAT_OUTCOME_VVG                         151
#define CONST_NUM_AVA_LEADERBOARD_ROWS                           19
#define CONST_NUM_SEARCH_SORT_FIELDS                             6
#define CONST_NUM_TAB_PANELS                                     4
#define CONST_RM_MAX_TEAM_COUNT                                  6
#define CONST_MAX_VENDOR_STACK_SIZE                              10000
#define CONST_TG_MAX_VENDOR_VENDOR_ITEMS                         8
#define CONST_TG_MAX_VENDOR_PLAYER_ITEMS                         18
#define CONST_TGCS_MAX_CHARACTERS                                8
#define CONST_TGCS_NO_SELECTION                                  -1
#define CONST_NUM_CHAT_CHANNELGROUPS                             5
#define CONST_NUM_CHAT_LINES_DISPLAYED                           9
#define CONST_NUM_PARTS_IN_CHAT_BG_IMAGE                         9
#define CONST_NUM_SELECT_CLASSES                                 4
#define CONST_SELECT_LOCKOUT_TIME                                3.5f
#define CONST_MORPH_HEAD_MALE_MESH_ID                            1605
#define CONST_MORPH_HEAD_FEMALE_MESH_ID                          1593
#define CONST_DEFAULT_MALE_HAIR_MESH_ID                          1937
#define CONST_DEFAULT_FEMALE_HAIR_MESH_ID                        1783
#define CONST_DEFAULT_CLASS_ID                                   1148
#define CONST_NUM_CUSTOM_HEAD_SECTIONS                           3
#define CONST_HARDCODE_DUPLICATE_CHARNAME                        9
#define CONST_CREATE_CHAR_TIMEOUT                                30.0f
#define CONST_NUM_CUSTOM_CATEGORIES                              8
#define CONST_HAIR_STYLE_MASTER_SLIDER_IDX                       1
#define CONST_MAX_STAT_DEVICE_FIELDS                             7
#define CONST_MAX_STAT_DEVICES                                   8
#define CONST_NUM_VISIBLE_BOTSLOTS                               18
#define CONST_MINSECS_FOR_RANDOMLOOT                             2.7
#define CONST_NUM_ENDMISSION_ACHIEVS                             6
#define CONST_NUM_OVERALL_RAID_STATS                             3
#define CONST_NUM_ALLIANCE_RAID_STATS                            4
#define CONST_NUM_INTRO_LABELS                                   3
#define CONST_INTRO_FADE_TIME                                    2.0f
#define CONST_INTRO_TOTAL_TIME                                   15.0f
#define CONST_TG_MAX_INVENTORY_POPUP_ITEMS                       18
#define CONST_NUM_SECS_FOR_TRANSITION_FADE                       0.4
#define CONST_NUM_SECS_FOR_HELPTEXT                              3.0
#define CONST_PURERADAR_MAP_ZOOMBASE                             250000
#define CONST_PURERADAR_PRESUMED_IMGSIZE                         1024
#define CONST_MAX_MINIMAP_DIST                                   1000
#define CONST_MAX_NUM_TEAMMATES                                  20
#define CONST_MAX_NUM_OBJECTIVES                                 10
#define CONST_LFG_PLAYERS_PER_TEAM                               6
#define CONST_LFG_ALERT_TIME                                     20
#define CONST_LFG_RESET_TIME                                     5
#define CONST_NUM_MAIL_TABS                                      2
#define CONST_NUM_MAX_ITEM_ATTACHMENTS                           4
#define CONST_NUM_MAX_DETAIL_ITEM_ATTACHMENTS                    5
#define CONST_NUM_QUEUE_CLASSES                                  4
#define CONST_GLOBE_MAX                                          -0.5
#define CONST_CLOUD_MAX                                          -1
#define CONST_GLOBE_ROTATION_RATE                                -0.00018
#define CONST_CLOUD_ROTATION_RATE                                -0.0009
#define CONST_BONUS_REWARD_EVENT_ID                              1353
#define CONST_DEFAULT_SITE_ID                                    1
#define CONST_FILTER_ON                                          2
#define CONST_FILTER_OFF                                         1
#define CONST_NUM_GAME_TYPES                                     6
#define CONST_MAX_QUEUE_COUNT                                    50
#define CONST_MISSION_LIST_PAGE_HEIGHT                           10
#define CONST_MISSION_LIST_PAGE_HEIGHT_PVE                       6
#define CONST_VVG_LOCATION                                       160
#define CONST_VVG_DIFFICULTY                                     116
#define CONST_TE_WIN_PVE                                         1
#define CONST_TE_WIN_ANY                                         2
#define CONST_TE_LOSE_ANY                                        3
#define CONST_TE_WIN_MERC                                        4
#define CONST_TE_WIN_WARZONE                                     5
#define CONST_TE_WIN_OPEN_ZONE_PVE                               6
#define CONST_TE_HOLD_TERRITORY                                  10
#define CONST_TE_WIN_PVE_MERC                                    11
#define CONST_TE_LOSE_PVE                                        12
#define CONST_TE_WIN_ARENA                                       14
#define CONST_TE_WIN_ARENA_MERC                                  15
#define CONST_NUM_OMEGA_PANELS                                   2
#define CONST_TG_NUM_SEARCH_RESULT_ENTRIES                       16
#define CONST_NUM_WEAPON_STATUS_BARS                             2
#define CONST_PROGRESS_TIMESHOWINGMAX                            0.25
#define CONST_PROGRESS_MAX                                       1.0
#define CONST_NUM_UIBUFFSLOTS                                    10
#define CONST_TG_MAX_ALERT_NUMBER                                1000
#define CONST_TG_MAX_ALERT_DISPLAYS                              5
#define CONST_ALERT_FADE_DURATION                                0.5f
#define CONST_NUM_DEVICE_SLOTS                                   11
#define CONST_NUM_INHAND_DEVICES                                 4
#define CONST_NO_SLOT_ACTIVE                                     -1
#define CONST_TIME_TO_SHOW_INVITE                                30
#define CONST_RADARMAP_SIZE_PLAYER                               15
#define CONST_RADARMAP_SIZE_ENEMY                                12
#define CONST_RADARMAP_SIZE_FRIENDLY                             12
#define CONST_NUM_PRIMARY_OBJECTIVES_IN_UI                       6
#define CONST_NUM_TEAM_ICONS                                     2
#define CONST_NUM_ROBOT_ICONS                                    4
#define CONST_NUM_OWNER_ICONS                                    4
#define CONST_NUM_ROBOT_POINT_LIGHTS                             3
#define CONST_NUM_HEX_NUMBER                                     6
#define CONST_NUM_TEAMS                                          6
#define CONST_NUM_QUEST_TRACKABLE_REQUIREMENTS                   4
#define CONST_SLOT_FLASH_DURATION                                0.3f
#define CONST_NUM_TEAM_PANEL_MEMBERS                             12
#define CONST_NUM_VISIBLE_NEXTNODES                              9
#define CONST_NUM_QUESTS_IN_UI                                   10
#define CONST_NUM_AUTO_QUEST_REWARDS_IN_UI                       4
#define CONST_TWO_AUTO_QUEST_REWARDS_IN_UI                       2
#define CONST_ONE_AUTO_QUEST_REWARD_IN_UI                        1
#define CONST_NUM_SEL_QUEST_REWARDS_IN_UI                        3
#define CONST_NUM_QUEST_REQUIRED_ELEMENTS_IN_UI                  4
#define CONST_NUM_QUEST_REQUIRED_ITEMS_IN_UI                     4
#define CONST_NUM_QUESTS_SUPPORTED_IN_LOG                        20
#define CONST_MAX_NUM_QUEUES                                     50
#define CONST_QUEUE_QUICK_REFRESH_TIME                           5.0f
#define CONST_QUEUE_REFRESH_TIME                                 15.0f
#define CONST_QUEUE_REFRESH_CAP                                  1.0f
#define CONST_NUM_FEEDBACK_LABELS                                6
#define CONST_MAX_HEXES                                          61
#define CONST_HEX_POLL_FREQUENCY                                 120
#define CONST_RM_MAX_MEMBER_COUNT                                12
#define CONST_KILLED_BY_FADE_START                               4.0f
#define CONST_KILLED_BY_FADE_TIME                                1.0f
#define CONST_NUM_SECS_FOR_WEAPON_COOLDOWN_FADE                  0.5
#define CONST_TIME_BETWEEN_SCENE_TICKS                           0.05
#define CONST_TGSCBD_MAX_PLAYERS_PER_MATCH                       20
#define CONST_TGSCBD_MAX_PLAYERS_PER_TEAM                        10
#define CONST_NUM_PLAYER_SCORE_LABELS                            5
#define CONST_ALLY_INDEX                                         0
#define CONST_ENEMY_INDEX                                        1
#define CONST_NUM_STATUS_ITEMS                                   4
#define CONST_TGSCBD_NUM_SCOREBOARD_FIELDS                       13
#define CONST_NUM_NON_SCORE_FIELDS                               3
#define CONST_TAS_MAX_SELECTION_DISPLAY                          10
#define CONST_TAS_SCROLL_DOWN_TIMER                              0
#define CONST_TAS_SCROLL_UP_TIMER                                1
#define CONST_NUM_SHOWN_CONFIGURABLE_CHANNELS                    11
#define CONST_NUM_CONFIGURABLE_CHANNELGROUPS                     4
#define CONST_NUM_BINDING_CATEGORIES                             6
#define CONST_MAX_ITEMS_PER_CATEGORY                             20
#define CONST_NUM_GAME_SOUND_SLIDERS                             3
#define CONST_NUM_VOICE_SOUND_SLIDERS                            3
#define CONST_MIDLINE_COMPAT_VALUE                               3
#define CONST_INVALID_STORE_INDEX                                -1
#define CONST_RANK_UNLOCK_SKILL_GROUP                            139
#define CONST_NUM_UNLOCK_TYPES                                   5

/*
# ========================================================================================= #
# Enums
# ========================================================================================= #
*/

// Enum TgClient.TgAgencyData.TG_AGENCY_STATUS
/*enum TG_AGENCY_STATUS
{
	AGENCY_MEMBER_ONLINE                               = 0,
	AGENCY_MEMBER_OFFLINE                              = 1,
	AGENCY_MEMBER_MAX                                  = 2
};*/

// Enum TgClient.TgAgencyData.TG_AGENCY_SORT
/*enum TG_AGENCY_SORT
{
	AGENCY_SORT_NAME                                   = 0,
	AGENCY_SORT_NAME_REVERSED                          = 1,
	AGENCY_SORT_RANK                                   = 2,
	AGENCY_SORT_RANK_REVERSED                          = 3,
	AGENCY_SORT_STATUS                                 = 4,
	AGENCY_SORT_STATUS_REVERSED                        = 5,
	AGENCY_SORT_LEVEL                                  = 6,
	AGENCY_SORT_LEVEL_REVERSED                         = 7,
	AGENCY_SORT_MAX                                    = 8
};*/

// Enum TgClient.TgDataSet.EEventTypes
/*enum EEventTypes
{
	EET_Post                                           = 0,
	EET_Response                                       = 1,
	EET_MAX                                            = 2
};*/

// Enum TgClient.TgHUD_Game.eUIVolumeQuestVisType
/*enum eUIVolumeQuestVisType
{
	eHudVisNone                                        = 0,
	eHudVisQuestAvl                                    = 1,
	eHudVisQuestTurnin                                 = 2,
	eHudVisQuestObj                                    = 3,
	eUIVolumeQuestVisType_MAX                          = 4
};*/

// Enum TgClient.TgUIMainMenu.eMenuFunctions
/*enum eMenuFunctions
{
	FUNCTION_SETTINGS_GENERAL                          = 0,
	FUNCTION_SETTINGS_VIDEO                            = 1,
	FUNCTION_SETTINGS_SOUND                            = 2,
	FUNCTION_SETTINGS_CONTROLS                         = 3,
	FUNCTION_SETTINGS_CHAT                             = 4,
	FUNCTION_MISSIONSCREEN                             = 5,
	FUNCTION_TEAM_SEARCH                               = 6,
	FUNCTION_TEAM_DETAILS                              = 7,
	FUNCTION_TEAM_CARGO                                = 8,
	FUNCTION_AGENCYSCREEN                              = 9,
	FUNCTION_AGENCYSCREEN_GENERAL                      = 10,
	FUNCTION_AGENCYSCREEN_ALLIANCE                     = 11,
	FUNCTION_AGENCYSCREEN_APPLICANTS                   = 12,
	FUNCTION_AGENCYSCREEN_CREATION                     = 13,
	FUNCTION_AGENCYSCREEN_MANAGEMENT                   = 14,
	FUNCTION_AGENCYSCREEN_FACILITIES                   = 15,
	FUNCTION_AGENCYSCREEN_INVENTORY                    = 16,
	FUNCTION_PLAYERSEARCH                              = 17,
	FUNCTION_VENDORSCREEN                              = 18,
	FUNCTION_RAID                                      = 19,
	FUNCTION_RAID_MAP                                  = 20,
	FUNCTION_DEVICE_STATS                              = 21,
	FUNCTION_MISSION_STATS                             = 22,
	FUNCTION_AUCTION_BROWSE                            = 23,
	FUNCTION_AUCTION_BID                               = 24,
	FUNCTION_AUCTION_SELL                              = 25,
	FUNCTION_MAIL                                      = 26,
	FUNCTION_LFG                                       = 27,
	FUNCTION_NAV                                       = 28,
	FUNCTION_CRAFTING                                  = 29,
	FUNCTION_INSTANCE_SELECT                           = 30,
	FUNCTION_LEVEL_UNLOCK                              = 31,
	FUNCTION_GLOBALMISSIONSCREEN                       = 32,
	FUNCTION_HEXAUCTION                                = 33,
	FUNCTION_AGENCYSCREEN_RECRUITING                   = 34,
	FUNCTION_AGENCY_DIRECTORY                          = 35,
	FUNCTION_CAMPAIGN_HEXMAP                           = 36,
	FUNCTION_CAMPAIGN_COMBAT                           = 37,
	FUNCTION_CAMPAIGN_COMBAT_LOG                       = 38,
	FUNCTION_CAMPAIGN_LEADERBOARD                      = 39,
	FUNCTION_FRIENDS_LIST                              = 40,
	FUNCTION_ZONEMISSIONSCREEN                         = 41,
	FUNCTION_DEFAULTMISSIONSCREEN                      = 42,
	FUNCTION_AGENCYSCREEN_INVENTORY_LOG                = 43,
	FUNCTION_AGENTPROFILE                              = 44,
	FUNCTION_AGENTPROFILE_EQUIP                        = 45,
	FUNCTION_AGENTPROFILE_STAT                         = 46,
	FUNCTION_QUESTDIALOG                               = 47,
	FUNCTION_ITEM_MOD                                  = 48,
	FUNCTION_QUEST_LOG                                 = 49,
	FUNCTION_UPDATE_HAIR                               = 50,
	FUNCTION_AGENTPROFILE_SKILL                        = 51,
	FUNCTION_AGENTPROFILE_INVENTORY                    = 52,
	FUNCTION_UPDATE_FACE                               = 53,
	FUNCTION_AGENTPROFILE_ACHIEVEMENT                  = 54,
	FUNCTION_STORE_FRONT                               = 55,
	FUNCTION_CASH_SHOP                                 = 56,
	FUNCTION_EXIT_MISSION                              = 57,
	FUNCTION_HELP                                      = 58,
	FUNCTION_TELEPORT                                  = 59,
	FUNCTION_MAX                                       = 60
};*/

// Enum TgClient.TgUICampaignMenu.TG_CAMPAIGN_TABS
/*enum TG_CAMPAIGN_TABS
{
	CAMPAIGN_PAGE_NONE                                 = 0,
	CAMPAIGN_PAGE_HEXMAP                               = 1,
	CAMPAIGN_PAGE_COMBAT                               = 2,
	CAMPAIGN_PAGE_COMBAT_LOG                           = 3,
	CAMPAIGN_PAGE_LEADERBOARD                          = 4,
	CAMPAIGN_PAGE_MAX                                  = 5
};*/

// Enum TgClient.TgUIAgencyMenu.TG_AGENCY_TABS
/*enum TG_AGENCY_TABS
{
	AGENCY_NONE_PAGE                                   = 0,
	AGENCY_GENERAL_PAGE                                = 1,
	AGENCY_BASES_PAGE                                  = 2,
	AGENCY_ALLIANCE_PAGE                               = 3,
	AGENCY_APPLICANTS_PAGE                             = 4,
	AGENCY_CREATION_PAGE                               = 5,
	AGENCY_MANAGEMENT_PAGE                             = 6,
	AGENCY_FACILITIES_PAGE                             = 7,
	AGENCY_INVENTORY_PAGE                              = 8,
	AGENCY_RECRUITING_PAGE                             = 9,
	AGENCY_AUDIT_PAGE                                  = 10,
	AGENCY_MAX                                         = 11
};*/

// Enum TgClient.TgUIAgentProfile.TG_AGENT_TABS
/*enum TG_AGENT_TABS
{
	AGENT_NONE_PAGE                                    = 0,
	AGENT_EQUIP_PAGE                                   = 1,
	AGENT_SKILLS_PAGE                                  = 2,
	AGENT_STATS_PAGE                                   = 3,
	AGENT_INVENTORY_PAGE                               = 4,
	AGENT_ACHIEVEMENTS_PAGE                            = 5,
	AGENT_MAX                                          = 6
};*/

// Enum TgClient.TgUISceneDriver_Tooltip.eToolTipTypes
/*enum eToolTipTypes
{
	TIP_PART_BASE_STAT_NS                              = 0,
	TIP_PART_BASE_STAT_SN                              = 1,
	TIP_PART_EXT_STAT                                  = 2,
	TIP_PART_EXT_STAT_DROP                             = 3,
	TIP_PART_EXT_STAT_MOD                              = 4,
	TIP_PART_FOOTER                                    = 5,
	TIP_PART_MAX                                       = 6
};*/

// Enum TgClient.TgUISceneDriver_Tooltip.eConfirmTypes
/*enum eConfirmTypes
{
	CONFIRM_TYPE_NONE                                  = 0,
	CONFIRM_TYPE_INFO                                  = 1,
	CONFIRM_TYPE_DEPOSIT                               = 2,
	CONFIRM_TYPE_WITHDRAW                              = 3,
	CONFIRM_TYPE_ADD                                   = 4,
	CONFIRM_TYPE_REMOVE                                = 5,
	CONFIRM_TYPE_DELETE                                = 6,
	CONFIRM_TYPE_CONFIRM                               = 7,
	CONFIRM_TYPE_TEXT                                  = 8,
	CONFIRM_TYPE_MAX                                   = 9
};*/

// Enum TgClient.TgUIAgencyMenu_Alliance.AllianceConfirmMode
/*enum AllianceConfirmMode
{
	ALLIANCE_CONFIRM_NONE                              = 0,
	ALLIANCE_CONFIRM_LEAVE                             = 1,
	ALLIANCE_CONFIRM_REMOVE                            = 2,
	ALLIANCE_CONFIRM_MAX                               = 3
};*/

// Enum TgClient.TgUIAgencyMenu_Facilities.FacilityConfirmIndex
/*enum FacilityConfirmIndex
{
	FCI_Headquarters                                   = 0,
	FCI_DefenseConfig                                  = 1,
	FCI_Transfer                                       = 2,
	FCI_Abandon                                        = 3,
	FCI_MAX                                            = 4
};*/

// Enum TgClient.TgUIAgencyMenu_General.AgencyPopupEditBoxMode
/*enum AgencyPopupEditBoxMode
{
	POPUP_NONE                                         = 0,
	POPUP_MOTD                                         = 1,
	POPUP_PCOMMENT                                     = 2,
	POPUP_OCOMMENT                                     = 3,
	POPUP_MAX                                          = 4
};*/

// Enum TgClient.TgUIAgentProfile_Achievement.eAchievementDataState
/*enum eAchievementDataState
{
	ADS_REQUEST                                        = 0,
	ADS_WAITING                                        = 1,
	ADS_RECEIVED                                       = 2,
	ADS_COMPLETE                                       = 3,
	ADS_MAX                                            = 4
};*/

// Enum TgClient.TgUIAgentProfile_Equip.Repair_ConfirmTypes
/*enum Repair_ConfirmTypes
{
	REPAIR_CONFIRM                                     = 0,
	REPAIR_CONFIRM01                                   = 1,
	REPAIR_CONFIRM02                                   = 2,
	REPAIR_CONFIRM03                                   = 3,
	REPAIR_CONFIRM04                                   = 4,
	REPAIR_CONFIRM_MAX                                 = 5
};*/

// Enum TgClient.TgUIAgentProfile_Equip.SupportedEquipTutorials
/*enum SupportedEquipTutorials
{
	STut_RANGED                                        = 0,
	STut_OFFHAND                                       = 1,
	STut_SPECIALTY                                     = 2,
	STut_OFFHAND2_DEFAULT                              = 3,
	STut_OFFHAND2_ROBO                                 = 4,
	STut_OFFHAND2_MEDIC                                = 5,
	STut_MAX                                           = 6
};*/

// Enum TgClient.TgUIAgentProfile_Equip.TutorialStep
/*enum TutorialStep
{
	TS_SLOT_SELECT                                     = 0,
	TS_DEVICE_SELECT                                   = 1,
	TS_CLOSE                                           = 2,
	TS_MAX                                             = 3
};*/

// Enum TgClient.TgUIAgentProfile_Inventory.InventoryConfirmIndex
/*enum InventoryConfirmIndex
{
	ICI_DELETE                                         = 0,
	ICI_SALVAGE                                        = 1,
	ICI_ACTIVATE                                       = 2,
	ICI_MAX                                            = 3
};*/

// Enum TgClient.TgUIAgentProfile_Stat.ePaperDollState
/*enum ePaperDollState
{
	DOLL_FULLVIEW                                      = 0,
	DOLL_HEADVIEW                                      = 1,
	DOLL_MAX                                           = 2
};*/

// Enum TgClient.TgUIAgentProfile_Stat.eFlairSetType
/*enum eFlairSetType
{
	FLAIRSET_RECENT                                    = 0,
	FLAIRSET_RARE                                      = 1,
	FLAIRSET_MAX                                       = 2
};*/

// Enum TgClient.TgUIAgentProfile_Stat.eBadgeSetType
/*enum eBadgeSetType
{
	BADGESET_RECENT                                    = 0,
	BADGESET_RARE                                      = 1,
	BADGESET_MAX                                       = 2
};*/

// Enum TgClient.TgUIAgentProfile_Stat.eStatTabs
/*enum eStatTabs
{
	SUBTAB_INFO                                        = 0,
	SUBTAB_CRAFTING                                    = 1,
	SUBTAB_MAX                                         = 2
};*/

// Enum TgClient.TgUIAgentProfile_Stat.eIconTabs
/*enum eIconTabs
{
	ICONTAB_BADGE                                      = 0,
	ICONTAB_FLAIR                                      = 1,
	ICONTAB_MAX                                        = 2
};*/

// Enum TgClient.TgUIAuctionGeneral.AuctionSceneMode
/*enum AuctionSceneMode
{
	AUC_MODE_MYAUCTIONS                                = 0,
	AUC_MODE_MYBIDS                                    = 1,
	AUC_MODE_MAX                                       = 2
};*/

// Enum TgClient.TgUIAuctionGeneral.AuctionMode
/*enum AuctionMode
{
	AUCTION_MODE_BID                                   = 0,
	AUCTION_MODE_BUYOUT                                = 1,
	AUCTION_MODE_MAX                                   = 2
};*/

// Enum TgClient.TgUIAuctionBrowseMenu.AchievmentCategoryArchetypeOrder
/*enum AchievmentCategoryArchetypeOrder
{
	ACH_CAT_UNSEL                                      = 0,
	ACH_CAT_SEL                                        = 1,
	ACH_CAT_SUBCAT_UNSEL                               = 2,
	ACH_CAT_SUBCAT_SEL                                 = 3,
	ACH_CAT_MAX                                        = 4
};*/

// Enum TgClient.TgUICampaignMenu_Combat.eCombatSortType
/*enum eCombatSortType
{
	CST_FACILITY                                       = 0,
	CST_DEFENDER_NAME                                  = 1,
	CST_ATTACKER_NAME                                  = 2,
	CST_ZONE_NAME                                      = 3,
	CST_NUM_DEFENDERS                                  = 4,
	CST_NUM_ATTACKERS                                  = 5,
	CST_TIME                                           = 6,
	CST_ATTACK_STATUS                                  = 7,
	CST_MAX                                            = 8
};*/

// Enum TgClient.TgUICampaignMenu_HexMap.eMapPanDirections
/*enum eMapPanDirections
{
	PanLeft                                            = 0,
	PanRight                                           = 1,
	PanUp                                              = 2,
	PanDown                                            = 3,
	eMapPanDirections_MAX                              = 4
};*/

// Enum TgClient.TgUITeamMenu.ePlayerSearchSort
/*enum ePlayerSearchSort
{
	SORT_CLASS                                         = 0,
	SORT_NAME                                          = 1,
	SORT_ALLIANCE                                      = 2,
	SORT_AGENCY                                        = 3,
	SORT_LEVEL                                         = 4,
	SORT_SKILL                                         = 5,
	SORT_STATUS                                        = 6,
	SORT_MAX                                           = 7
};*/

// Enum TgClient.TgUIVendorBase.VendorType
/*enum VendorType
{
	TG_VT_Currency                                     = 0,
	TG_VT_Token                                        = 1,
	TG_VT_HZPoints                                     = 2,
	TG_VT_RealWorldCurrency                            = 3,
	TG_VT_MAX                                          = 4
};*/

// Enum TgClient.TgUIVendorBase.ePurchaseMethod
/*enum ePurchaseMethod
{
	PurchaseMethod_None                                = 0,
	PurchaseMethod_AP                                  = 1,
	PurchaseMethod_Std                                 = 2,
	PurchaseMethod_MAX                                 = 3
};*/

// Enum TgClient.TgUIVendorScene.eItemPreviewState
/*enum eItemPreviewState
{
	IPS_Inv                                            = 0,
	IPS_Text                                           = 1,
	IPS_Generic                                        = 2,
	IPS_Char                                           = 3,
	IPS_MAX                                            = 4
};*/

// Enum TgClient.TgUICharacterSelectScene.CHAR_SELECT_STATES
/*enum CHAR_SELECT_STATES
{
	CSS_SELECT_CHARACTER                               = 0,
	CSS_WAITING_FOR_RESPONSE                           = 1,
	CSS_CREATING_CHARACTER                             = 2,
	CSS_DELETING_CHARACTER                             = 3,
	CSS_CLONING_CHARACTER                              = 4,
	CSS_MAX                                            = 5
};*/

// Enum TgClient.TgUIChat.eUIChatState
/*enum eUIChatState
{
	ChatState_Passive                                  = 0,
	ChatState_EnteringText                             = 1,
	ChatState_MAX                                      = 2
};*/

// Enum TgClient.TgUIChat.eUIChatOpenings
/*enum eUIChatOpenings
{
	ChatOpening_Default                                = 0,
	ChatOpening_Slash                                  = 1,
	ChatOpening_Reply                                  = 2,
	ChatOpening_MAX                                    = 3
};*/

// Enum TgClient.TgUIClassSelectScene.eClassSelectIndex
/*enum eClassSelectIndex
{
	CSI_ASSAULT                                        = 0,
	CSI_MEDIC                                          = 1,
	CSI_RECON                                          = 2,
	CSI_ROBOTICS                                       = 3,
	CSI_MAX                                            = 4
};*/

// Enum TgClient.TgUIClassSelectScene.eGenderSelectIndex
/*enum eGenderSelectIndex
{
	GSI_MALE                                           = 0,
	GSI_FEMALE                                         = 1,
	GSI_MAX                                            = 2
};*/

// Enum TgClient.TgUICraftScene.BPModArchetypeOrder
/*enum BPModArchetypeOrder
{
	ARCH_BPMOD_HEADER                                  = 0,
	ARCH_BPMOD_PROPERTY                                = 1,
	ARCH_BPMOD_MAX                                     = 2
};*/

// Enum TgClient.TgUICustomHeadMenu.CREATEHEAD_VERIFY_STATES
/*enum CREATEHEAD_VERIFY_STATES
{
	CHCN_NONE                                          = 0,
	CHCN_WAITING_FOR_NAME_VERIFY                       = 1,
	CHCN_NAME_OK                                       = 2,
	CHCN_NAME_ERROR                                    = 3,
	CHCN_CLIENTSIDE_NAME_ERROR                         = 4,
	CHCN_CREATING_CHARACTER                            = 5,
	CHCN_CREATING_CHARACTER_ERROR                      = 6,
	CHCN_CREATING_CHARACTER_SUCCESS                    = 7,
	CHCN_MAX                                           = 8
};*/

// Enum TgClient.TgUICustomHeadMenu.EHeadMorphClass
/*enum EHeadMorphClass
{
	HMC_POSE                                           = 0,
	HMC_BLEND                                          = 1,
	HMC_LIST                                           = 2,
	HMC_MAX                                            = 3
};*/

// Enum TgClient.TgUIDeviceStats.DEVICE_STAT_FIELD
/*enum DEVICE_STAT_FIELD
{
	DVFIELD_ID                                         = 0,
	DVFIELD_DAMAGE                                     = 1,
	DVFIELD_HEALING                                    = 2,
	DVFIELD_PLAYER_KILLS                               = 3,
	DVFIELD_BOT_KILLS                                  = 4,
	DVFIELD_DPM                                        = 5,
	DVFIELD_HPM                                        = 6,
	DVFIELD_MAX                                        = 7
};*/

// Enum TgClient.TgUIEndMissionScreen.eBonusRewardTypes
/*enum eBonusRewardTypes
{
	eBonusType_Elite                                   = 0,
	eBonusType_Boost                                   = 1,
	eBonusType_MAX                                     = 2
};*/

// Enum TgClient.TgUIFriendsList.eFLState
/*enum eFLState
{
	ShowFriends                                        = 0,
	ShowIgnores                                        = 1,
	eFLState_MAX                                       = 2
};*/

// Enum TgClient.TgUIFriendsList.eFLPopupContext
/*enum eFLPopupContext
{
	FLNone                                             = 0,
	FLAdd                                              = 1,
	FLIgnore                                           = 2,
	FLNote                                             = 3,
	eFLPopupContext_MAX                                = 4
};*/

// Enum TgClient.TgUIGameHelpItems.EGameHelpItemTypes
/*enum EGameHelpItemTypes
{
	TutGH_UIVolumes                                    = 0,
	TutGH_UIQuestGlyphs                                = 1,
	TutGH_BeaconNetwork                                = 2,
	TutGH_Merc_PVP                                     = 3,
	TutGH_Beacon                                       = 4,
	TutGH_MAX                                          = 5
};*/

// Enum TgClient.TgUILevelMap.EMapRotationMode
/*enum EMapRotationMode
{
	MapRotation_Off                                    = 0,
	MapRotation_On                                     = 1,
	MapRotation_ApplyLocal                             = 2,
	MapRotation_MAX                                    = 3
};*/

// Enum TgClient.TgUILoginMenu.LoginScreenProcessingState
/*enum LoginScreenProcessingState
{
	LSPS_NONE                                          = 0,
	LSPS_LOGGING_IN                                    = 1,
	LSPS_CREATING_CHAR                                 = 2,
	LSPS_MAX                                           = 3
};*/

// Enum TgClient.TgUIMailMenu.eMailTabs
/*enum eMailTabs
{
	TAB_INBOX                                          = 0,
	TAB_COMPOSE                                        = 1,
	TAB_MAX                                            = 2
};*/

// Enum TgClient.TgUIMainMenu.eButtonStatus
/*enum eButtonStatus
{
	ButtonStatus_Enabled                               = 0,
	ButtonStatus_Disabled                              = 1,
	ButtonStatus_MAX                                   = 2
};*/

// Enum TgClient.TgUIMapTransition.eMapTransitionState
/*enum eMapTransitionState
{
	MapTransState_OpeningCutsceneTrigger               = 0,
	MapTransState_OpeningCutscenePlaying               = 1,
	MapTransState_TakeoffVideoTrigger                  = 2,
	MapTransState_TakeoffVideoPlaying                  = 3,
	MapTransState_BriefingUITrigger                    = 4,
	MapTransState_BriefingUIShowing                    = 5,
	MapTransState_MapLoading                           = 6,
	MapTransState_LandingVideoTrigger                  = 7,
	MapTransState_LandingVideoPlaying                  = 8,
	MapTransState_AwaitingMapInfo                      = 9,
	MapTransState_MAX                                  = 10
};*/

// Enum TgClient.TgUIOmegaMenu.eOmegaPanels
/*enum eOmegaPanels
{
	OMEGA_PANEL_ALERT                                  = 0,
	OMEGA_PANEL_TITLE                                  = 1,
	OMEGA_PANEL_MAX                                    = 2
};*/

// Enum TgClient.TgUIPlayerSearchMenu.TG_PLAYERSEARCH_SORT
/*enum TG_PLAYERSEARCH_SORT
{
	PLAYERSEARCH_SORT_CLASS                            = 0,
	PLAYERSEARCH_SORT_NAME                             = 1,
	PLAYERSEARCH_SORT_AGENCY                           = 2,
	PLAYERSEARCH_SORT_ALLIANCE                         = 3,
	PLAYERSEARCH_SORT_FACTION                          = 4,
	PLAYERSEARCH_SORT_LEVEL                            = 5,
	PLAYERSEARCH_SORT_LOCATION                         = 6,
	PLAYERSEARCH_SORT_TEAMSTATUS                       = 7,
	PLAYERSEARCH_SORT_MAX                              = 8
};*/

// Enum TgClient.TgUIPlayerSearchMenu.TG_PLAYERSEARCH_DETAILINDEX
/*enum TG_PLAYERSEARCH_DETAILINDEX
{
	DETAIL_INDEX_CLASS                                 = 0,
	DETAIL_INDEX_LEVEL                                 = 1,
	DETAIL_INDEX_AGENCY                                = 2,
	DETAIL_INDEX_ALLIANCE                              = 3,
	DETAIL_INDEX_LOCATION                              = 4,
	DETAIL_INDEX_HISTORY_HEADER                        = 5,
	DETAIL_INDEX_HISTORY_CURRENT                       = 6,
	DETAIL_INDEX_HISTORY_PREV1                         = 7,
	DETAIL_INDEX_HISTORY_PREV2                         = 8,
	DETAIL_INDEX_HISTORY_PREV3                         = 9,
	DETAIL_INDEX_MAX                                   = 10
};*/

// Enum TgClient.TgUIPrimaryHUD.eATTENTIONBAR_TYPE
/*enum eATTENTIONBAR_TYPE
{
	ONELINE_ATTENTION                                  = 0,
	TWOLINE_ATTENTION                                  = 1,
	eATTENTIONBAR_TYPE_MAX                             = 2
};*/

// Enum TgClient.TgUIPrimaryHUD_Settings.VisibilityConfigType
/*enum VisibilityConfigType
{
	VCT_User                                           = 0,
	VCT_Managed                                        = 1,
	VCT_MAX                                            = 2
};*/

// Enum TgClient.TgUIPrimaryHUD_DialogQuery.HUDDialogType
/*enum HUDDialogType
{
	DialogInviteToTeam                                 = 0,
	DialogInviteToAgency                               = 1,
	HUDDialogType_MAX                                  = 2
};*/

// Enum TgClient.TgUIPrimaryHUD_MissionInfo.eTeamIcons
/*enum eTeamIcons
{
	ICON_ATTACKER                                      = 0,
	ICON_DEFENDER                                      = 1,
	ICON_MAX                                           = 2
};*/

// Enum TgClient.TgUIPrimaryHUD_MissionInfo.eRobotIcons
/*enum eRobotIcons
{
	ROBOT_UNALIGNED                                    = 0,
	ROBOT_FRIENDLY                                     = 1,
	ROBOT_ENEMY                                        = 2,
	ROBOT_DESTROYED                                    = 3,
	ROBOT_MAX                                          = 4
};*/

// Enum TgClient.TgUIPrimaryHUD_MissionInfo.eOwnerIcons
/*enum eOwnerIcons
{
	ATTACK_SWORDS                                      = 0,
	DEFEND_SHIELD                                      = 1,
	eOwnerIcons_MAX                                    = 2
};*/

// Enum TgClient.TgUIQuestDialog.QuestDialogTutorials
/*enum QuestDialogTutorials
{
	SQTut_ACCEPT                                       = 0,
	SQTut_TURNIN                                       = 1,
	SQTut_MAX                                          = 2
};*/

// Enum TgClient.TgUIQuestDialog.QuestTutorialStep
/*enum QuestTutorialStep
{
	QTS_FIRST_SELECT                                   = 0,
	QTS_NEXT                                           = 1,
	QTS_NEXT01                                         = 2,
	QTS_FINISH                                         = 3,
	QTS_MAX                                            = 4
};*/

// Enum TgClient.TgUIQuestDialog.eQuestDialogState
/*enum eQuestDialogState
{
	QDS_Intro                                          = 0,
	QDS_Offer                                          = 1,
	QDS_Turnin                                         = 2,
	QDS_Reward                                         = 3,
	QDS_MAX                                            = 4
};*/

// Enum TgClient.TgUISceneManager.eSceneDriverPrecacheType
/*enum eSceneDriverPrecacheType
{
	Precache_None                                      = 0,
	Precache_Driver                                    = 1,
	Precache_DriverAndScene                            = 2,
	Precache_MAX                                       = 3
};*/

// Enum TgClient.TgUIScoreboard.SCORE_LABELS
/*enum SCORE_LABELS
{
	SBLABEL_NAME                                       = 0,
	SBLABEL_CLASS                                      = 1,
	SBLABEL_AGENCY                                     = 2,
	SBLABEL_LEVEL                                      = 3,
	SBLABEL_SCORE                                      = 4,
	SBLABEL_MAX                                        = 5
};*/

// Enum TgClient.TgUIScoreboard.SCORE_STATUS
/*enum SCORE_STATUS
{
	STATUS_NONE                                        = 0,
	STATUS_TOP_SCORE                                   = 1,
	STATUS_LONG_LIFE                                   = 2,
	STATUS_DEAD                                        = 3,
	STATUS_MAX                                         = 4
};*/

// Enum TgClient.TgUIScoreDetails.SCORE_BOARD_FIELD
/*enum SCORE_BOARD_FIELD
{
	SBFIELD_LEVEL                                      = 0,
	SBFIELD_NAME                                       = 1,
	SBFIELD_AGENCY                                     = 2,
	SBFIELD_KILLS                                      = 3,
	SBFIELD_ASSISTS                                    = 4,
	SBFIELD_DMG_TAKEN                                  = 5,
	SBFIELD_DAMAGE                                     = 6,
	SBFIELD_BUFFS                                      = 7,
	SBFIELD_HEALING                                    = 8,
	SBFIELD_DEFENSE                                    = 9,
	SBFIELD_DEATHS                                     = 10,
	SBFIELD_OBJS                                       = 11,
	SBFIELD_KILLS_BOT                                  = 12,
	SBFIELD_MAX                                        = 13
};*/

// Enum TgClient.TgUISettingsMenu_General.eCheckboxGeneral
/*enum eCheckboxGeneral
{
	CBG_DEPLOYABLE_HEALTH                              = 0,
	CBG_RETICLE_LABELS                                 = 1,
	CBG_RETICLE_SHIFT                                  = 2,
	CBG_DISABLE_PLAYER_ICONS                           = 3,
	CBG_OFFSCREEN                                      = 4,
	CBG_MOUSE_SMOOTHING                                = 5,
	CBG_RETICLE_BAR                                    = 6,
	CBG_HIT_EFFECT_ZOOM                                = 7,
	CBG_HIT_EFFECT_FULL                                = 8,
	CBG_DYE_SELF                                       = 9,
	CBG_DYE_ALLIES                                     = 10,
	CBG_DYE_ENEMIES                                    = 11,
	CBG_FULL_HELP_ALERTS                               = 12,
	CBG_OVERHEAD_TEXT                                  = 13,
	CBG_OBJECTIVE_NAMES                                = 14,
	CBG_DEBUG_DAMAGE                                   = 15,
	CBG_MAX                                            = 16
};*/

// Enum TgClient.TgUISettingsMenu_General.eCheckboxHUD
/*enum eCheckboxHUD
{
	CBH_DEVICE                                         = 0,
	CBH_BUFF                                           = 1,
	CBH_PLAYER_INFO                                    = 2,
	CBH_TEAM_INFO                                      = 3,
	CBH_MINIMAP                                        = 4,
	CBH_BEACON                                         = 5,
	CBH_MISSION_INFO                                   = 6,
	CBH_RETICULE                                       = 7,
	CBH_CHATBOX                                        = 8,
	CBH_AGENT_INFO                                     = 9,
	CBH_COMBAT_ALERT                                   = 10,
	CBH_QUEST_TRACK                                    = 11,
	CBH_MAX                                            = 12
};*/

// Enum TgClient.TgUISettingsMenu_General.eCheckboxQueue
/*enum eCheckboxQueue
{
	CBQ_LATE_BACKFILLS_OK                              = 0,
	CBQ_QUICK_QUEUE                                    = 1,
	CBQ_FOR_FUTURE_USE                                 = 2,
	CBQ_MAX                                            = 3
};*/

// Enum TgClient.TgUISettingsMenu_General.eCheckboxTeams
/*enum eCheckboxTeams
{
	CBQ_AVA_INTEREST                                   = 0,
	CBQ_AVA_FUTURE_USE                                 = 1,
	CBQ_AVA_MAX                                        = 2
};*/

// Enum TgClient.TgUISettingsMenu_Sound.eGameSoundSliders
/*enum eGameSoundSliders
{
	GSS_MASTER_VOL                                     = 0,
	GSS_EFFECTS_VOL                                    = 1,
	GSS_MUSIC_VOL                                      = 2,
	GSS_MAX                                            = 3
};*/

// Enum TgClient.TgUISettingsMenu_Sound.eVoiceSoundSliders
/*enum eVoiceSoundSliders
{
	VSS_INCOMING_VOL                                   = 0,
	VSS_OUTGOING_VOL                                   = 1,
	VSS_ACTIVATE_VOL                                   = 2,
	VSS_MAX                                            = 3
};*/

// Enum TgClient.TgUISettingsMenu_Video.eVideoOptionCheckboxes
/*enum eVideoOptionCheckboxes
{
	BOX_VSYNC                                          = 0,
	BOX_SMOOTH_FPS                                     = 1,
	BOX_COMPOSITE_LIGHTS                               = 2,
	BOX_DIRECTIONAL_LIGHTMAPS                          = 3,
	BOX_DYNAMIC_SHADOWS                                = 4,
	BOX_COMPOSITE_SHADOWS                              = 5,
	BOX_HIGH_QUALITY_MATERIALS                         = 6,
	BOX_HIGH_CHARACTER_LOD                             = 7,
	BOX_SHADOWED_PLAYER_NAMES                          = 8,
	BOX_STATIC_DECALS                                  = 9,
	BOX_DYNAMIC_DECALS                                 = 10,
	BOX_FOLIAGE                                        = 11,
	BOX_BLOOM                                          = 12,
	BOX_DISTORTION                                     = 13,
	BOX_MOTION_BLUR                                    = 14,
	BOX_AMBIENT_OCCLUSION                              = 15,
	BOX_DEPTH_OF_FIELD                                 = 16,
	BOX_LENS_FLARE                                     = 17,
	BOX_D3D10                                          = 18,
	BOX_MAX                                            = 19
};*/

// Enum TgClient.TgUISettingsMenu_Video.eVideoOptionComboBoxes
/*enum eVideoOptionComboBoxes
{
	COMBO_PARTICLE_DETAIL                              = 0,
	COMBO_SHADOW_FILTERING                             = 1,
	COMBO_TEXTURE_FILTERING                            = 2,
	COMBO_ANISO_QUALITY                                = 3,
	COMBO_CHARACTER_TEXTURES                           = 4,
	COMBO_WORLD_TEXTURES                               = 5,
	COMBO_EFFECT_TEXTURES                              = 6,
	COMBO_MSAA_QUALITY                                 = 7,
	COMBO_MAX                                          = 8
};*/

// Enum TgClient.TgUISettingsMenu_Video.eVideoOptionWindowType
/*enum eVideoOptionWindowType
{
	VIDEOWINDOW_WINDOWED                               = 0,
	VIDEOWINDOW_FULLSCREEN                             = 1,
	VIDEOWINDOW_BORDERLESS                             = 2,
	VIDEOWINDOW_MAX                                    = 3
};*/

// Enum TgClient.TgUITutorialItem.ETutClassDevType
/*enum ETutClassDevType
{
	TutDev_Melee                                       = 0,
	TutDev_Ranged                                      = 1,
	TutDev_Offhand1                                    = 2,
	TutDev_Specialty                                   = 3,
	TutDev_Offhand2                                    = 4,
	TutDev_MAX                                         = 5
};*/

// Enum TgClient.TgUIUnlockScene.eUnlockTypes
/*enum eUnlockTypes
{
	UNLOCK_DEVICE                                      = 0,
	UNLOCK_ARMOR                                       = 1,
	UNLOCK_SKILL_POINT                                 = 2,
	UNLOCK_MAX                                         = 3
};*/


/*
# ========================================================================================= #
# Classes
# ========================================================================================= #
*/

// Class TgClient.TgAchievementData
// 0x0040 (0x007C - 0x003C)
class UTgAchievementData : public UObject
{
public:
	unsigned char                                      UnknownData00[ 0x3C ];                            		// 0x003C (0x003C) UNKNOWN PROPERTY: MapProperty TgClient.TgAchievementData.m_AchievementRecords
	unsigned long                                      m_bDataArrived : 1;                               		// 0x0078 (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1588 ];

		return pClassPointer;
	};

};


// Class TgClient.TgAgencyData
// 0x009C (0x00D8 - 0x003C)
class UTgAgencyData : public UObject
{
public:
	TArray< struct FsAgencyMemberInfo >                m_MemberListInfo;                                 		// 0x003C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsAgencyMemberInfo >                m_MemberListInfoSorted;                           		// 0x0048 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsAgencyRankData >                  m_RankListData;                                   		// 0x0054 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned char                                      m_CurrentSortType;                                		// 0x0060 (0x0001) [0x0000000000000000]              
	int                                                m_CurrentFilter;                                  		// 0x0064 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_SortedFilter;                                   		// 0x0068 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_HexMapData;                                     		// 0x006C (0x0004) [0x0000000000001000]              ( CPF_Native )
	struct FPointer                                    m_InventoryData;                                  		// 0x0070 (0x0004) [0x0000000000001000]              ( CPF_Native )
	struct FString                                     m_Name;                                           		// 0x0074 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_MOTD;                                           		// 0x0080 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_Information;                                    		// 0x008C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_Abbreviation;                                   		// 0x0098 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nLogoResId;                                     		// 0x00A4 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrency;                                      		// 0x00A8 (0x0004) [0x0000000000000000]              
	int                                                m_nSiteId;                                        		// 0x00AC (0x0004) [0x0000000000000000]              
	float                                              m_fTaxPercent;                                    		// 0x00B0 (0x0004) [0x0000000000000000]              
	int                                                m_nAgencyId;                                      		// 0x00B4 (0x0004) [0x0000000000000000]              
	int                                                m_nNumMembers;                                    		// 0x00B8 (0x0004) [0x0000000000000000]              
	int                                                m_nLocalPlayerId;                                 		// 0x00BC (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNoAgency : 1;                                  		// 0x00C0 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bRecruiting : 1;                                		// 0x00C0 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bRecruitingNonSub : 1;                          		// 0x00C0 (0x0004) [0x0000000000000000] [0x00000004] 
	struct FString                                     m_sRecruitingText;                                		// 0x00C4 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              m_fLastDataRequestTime;                           		// 0x00D0 (0x0004) [0x0000000000000000]              
	int                                                m_PendingUpdateFlags;                             		// 0x00D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1589 ];

		return pClassPointer;
	};

	struct FsAgencyRankData eventGetDefaultRankData ( );
	struct FsAgencyMemberInfo eventGetDefaultMemberInfo ( );
	void Tick ( float DeltaTime );
	void SetTax ( float fTax );
	void SetDescription ( struct FString sDescription );
	void SetOfficerComment ( int nPlayerId, struct FString sNote );
	void SetPublicComment ( int nPlayerId, struct FString sNote );
	void SetMOTD ( struct FString sMOTD );
	void TransferLeader ( struct FString sPlayerName );
	void Disband ( );
	void Create ( struct FString sAgencyName, struct FLinearColor AgencyColor );
	void Invite ( struct FString sPlayerName );
	void Remove ( int nPlayerId );
	void Demote ( int nPlayerId );
	void Promote ( int nPlayerId );
	void UpdateRecruiting ( struct FString sText, unsigned long bRecruiting, unsigned long bSubOnly );
	bool CheckFiltered ( struct FsAgencyMemberInfo Info, int nFilter );
	void SetFilter ( int nNewFilter );
	int GetNumMembersOfRank ( int nRankId );
	bool LocalPlayerHasPermission ( int flag );
	bool LocalPlayerIsLeader ( );
	struct FsAgencyMemberInfo GetLocalPlayerInfo ( );
	struct FsAgencyMemberInfo GetMemberInfo ( int nPlayerId );
	struct FsAgencyRankData GetLocalRankData ( );
	struct FsAgencyRankData GetRankData ( int nRank );
	void ClearData ( int rosterflags );
	bool RequestAgencyInfo ( int Flags );
	TArray< struct FsAgencyMemberInfo > GetSortedInfoArray ( );
	void InitAgencyData ( );
};


// Class TgClient.TgAllianceData
// 0x0068 (0x00A4 - 0x003C)
class UTgAllianceData : public UObject
{
public:
	struct FString                                     m_Name;                                           		// 0x003C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_MOTD;                                           		// 0x0048 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_Information;                                    		// 0x0054 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_Abbreviation;                                   		// 0x0060 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nAllianceId;                                    		// 0x006C (0x0004) [0x0000000000000000]              
	int                                                m_nOwnerAgencyId;                                 		// 0x0070 (0x0004) [0x0000000000000000]              
	struct FString                                     m_OwnerAgencyName;                                		// 0x0074 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nNumMembers;                                    		// 0x0080 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNoAlliance : 1;                                		// 0x0084 (0x0004) [0x0000000000000000] [0x00000001] 
	TArray< struct FsAllianceMemberInfo >              m_MemberListInfo;                                 		// 0x0088 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	float                                              m_fLastDataRequestTime;                           		// 0x0094 (0x0004) [0x0000000000000000]              
	int                                                m_PendingUpdateFlags;                             		// 0x0098 (0x0004) [0x0000000000000000]              
	struct FDouble                                     m_dtCreated;                                      		// 0x009C (0x0008) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1590 ];

		return pClassPointer;
	};

	struct FsAllianceMemberInfo eventGetDefaultMemberInfo ( );
	void Disband ( );
	void Remove ( int nAgencyId );
	void Invite ( struct FString sPlayerName );
	void Create ( struct FString sName );
	struct FsAllianceMemberInfo GetMemberInfo ( int nAgencyId );
	void Tick ( float DeltaSeconds );
	void ClearData ( unsigned long bGeneralData, unsigned long bMemberData );
	bool RequestAllianceInfo ( int Flags );
};


// Class TgClient.TgAttentionData
// 0x0004 (0x0040 - 0x003C)
class UTgAttentionData : public UObject
{
public:
	unsigned long                                      c_bPressedMissionKey : 1;                         		// 0x003C (0x0004) [0x0000000000044000] [0x00000001] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      c_bPressedEscKey : 1;                             		// 0x003C (0x0004) [0x0000000000044000] [0x00000002] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      c_bPressedMapKey : 1;                             		// 0x003C (0x0004) [0x0000000000044000] [0x00000004] ( CPF_Config | CPF_GlobalConfig )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1591 ];

		return pClassPointer;
	};

};


// Class TgClient.TgConsole
// 0x0004 (0x01F4 - 0x01F0)
class UTgConsole : public UConsole
{
public:
	unsigned long                                      m_bEnabled : 1;                                   		// 0x01F0 (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1592 ];

		return pClassPointer;
	};

	bool InputKey ( int ControllerId, struct FName Key, unsigned char Event, float AmountDepressed, unsigned long bGamepad );
	void SetEnabled ( unsigned long bEnabled );
};


// Class TgClient.TgDataInterface
// 0x0000 (0x003C - 0x003C)
class UTgDataInterface : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1593 ];

		return pClassPointer;
	};

	void GetLocalBotList ( class UTgDataSet* pDataSet );
	void GetInstanceList ( class UTgDataSet* pDataSet );
	void QueryPlayers ( class UTgDataSet* pDataSet );
	void GetCharacterStats ( class UTgDataSet* pDataSet );
	void GetLootTableItems ( int nLootTableId, int nProfileId, class UTgDataSet* pDataSet );
	void AddCharacter ( struct FCharacterInfoStruct CharInfo, class UTgDataSet* pDataSet );
};


// Class TgClient.TgDataSet
// 0x0014 (0x0050 - 0x003C)
class UTgDataSet : public UObject
{
public:
	int                                                m_nCallbackObject;                                		// 0x003C (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FName                                       m_nmCallbackFunc;                                 		// 0x0040 (0x0008) [0x0000000000000002]              ( CPF_Const )
	struct FPointer                                    m_pMarshal;                                       		// 0x0048 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FPointer                                    m_pRowSet;                                        		// 0x004C (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1594 ];

		return pClassPointer;
	};

	void MakeRequest ( struct FString sFuncName );
	void UpdateParam ( struct FString sName, struct FString sValue );
	void AddParam ( struct FString sName, struct FString sValue );
	void CallRegisteredForEvents ( unsigned char eEventType );
	void RegisterForEvents ( class UObject* pObject, struct FName nmFuncName );
	struct FString GetErrorText ( );
	struct FString GetErrorColumn ( );
	int GetErrorCode ( );
	void AddCurrentRow ( class UTgDataSet* pDataSet );
	void Sort ( struct FString sColName, unsigned long bAscending );
	void DeleteAll ( unsigned long bUpdateBound );
	void DeleteRow ( );
	void AddRow ( );
	bool GoToRowSet ( int nTokenId );
	bool MoveTo ( int nRowNumber );
	bool MoveLast ( );
	bool MoveNext ( );
	bool MoveFirst ( );
	void SetNbrColValue ( struct FString sColName, float fValue );
	void SetStrColValue ( struct FString sColName, struct FString sValue );
	float GetNbrColValue ( struct FString sColName );
	struct FString GetStrColValueByRow ( struct FString sColName, int iRowId );
	struct FString GetStrColValue ( struct FString sColName );
	struct FString GetStrColText ( struct FString sColName );
	int GetCurrentRowId ( );
	bool ColumnEmptyOrNull ( struct FString sColName );
	int RecordCount ( );
	void Reinitialize ( );
	void Clear ( );
};


// Class TgClient.TgGameViewportClient
// 0x000C (0x0108 - 0x00FC)
class UTgGameViewportClient : public UTgViewportClient
{
public:
	TArray< class UTgObjectReferencer* >               m_DeferredManifests;                              		// 0x00FC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1595 ];

		return pClassPointer;
	};

	void eventGameSessionEnded ( );
	void DrawTransition ( class UCanvas* Canvas );
	bool eventInit ( struct FString* OutError );
	void NativeGameSessionEnded ( );
	void ClearDeferredManifests ( );
	void AddDeferredManifest ( class UTgObjectReferencer* ObjReferencer );
	void DetachHUDSettings ( );
	void CenterMouseInViewport ( unsigned long CenterPreCaptureMousePos );
};


// Class TgClient.TgHUD_Game
// 0x0320 (0x08BC - 0x059C)
class ATgHUD_Game : public ATgHUD
{
public:
	class UTexture2D*                                  m_BarTexture;                                     		// 0x059C (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UTexture2D*                                  m_CutoutTexture;                                  		// 0x05A0 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UTexture2D*                                  m_ColorTexture;                                   		// 0x05A4 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UTexture2D*                                  m_TargetTexture;                                  		// 0x05A8 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FDigitData                                  m_DigitInfo[ 0xA ];                               		// 0x05AC (0x00A0) [0x0000000000000000]              
	struct FColor                                      m_HUDTeamColor;                                   		// 0x064C (0x0004) [0x0000000000000000]              
	int                                                m_nTextPanelWidth;                                		// 0x0650 (0x0004) [0x0000000000000000]              
	int                                                m_nTextPanelHeight;                               		// 0x0654 (0x0004) [0x0000000000000000]              
	int                                                m_nTeamPanelWidth;                                		// 0x0658 (0x0004) [0x0000000000000000]              
	int                                                m_nTeamPanelHeight;                               		// 0x065C (0x0004) [0x0000000000000000]              
	int                                                m_nTeamIconHeight;                                		// 0x0660 (0x0004) [0x0000000000000000]              
	class APlayerReplicationInfo*                      m_NamedPlayer;                                    		// 0x0664 (0x0004) [0x0000000000000000]              
	float                                              m_NameTime;                                       		// 0x0668 (0x0004) [0x0000000000000000]              
	class UMaterial*                                   m_LockAlertMat;                                   		// 0x066C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_LockAlertMIC;                                   		// 0x0670 (0x0004) [0x0000000000000000]              
	int                                                m_nEDLastUpdated;                                 		// 0x0674 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bDrawPawnHUD : 1;                               		// 0x0678 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bBlockPawnHUD : 1;                              		// 0x0678 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bDrawTaskForce : 1;                             		// 0x0678 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bShowHUDTargetExtraInfo : 1;                    		// 0x0678 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bRotatingRadarMap : 1;                          		// 0x0678 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_bFadeOutHitEffect : 1;                          		// 0x0678 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bEnableHitEffect : 1;                           		// 0x0678 (0x0004) [0x0000000000044000] [0x00000040] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bFadeOutZoomEffect : 1;                         		// 0x0678 (0x0004) [0x0000000000000000] [0x00000080] 
	class UDOFAndBloomEffect*                          m_HitBlurEffect;                                  		// 0x067C (0x0004) [0x0000000000000000]              
	class UMaterialEffect*                             m_HitMaterialEffect;                              		// 0x0680 (0x0004) [0x0000000000000000]              
	struct FName                                       c_CurrentScopeMaterialName;                       		// 0x0684 (0x0008) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_HitEffectMaterialInstance;                      		// 0x068C (0x0004) [0x0000000000002000]              ( CPF_Transient )
	float                                              m_HitEffectFadeTime;                              		// 0x0690 (0x0004) [0x0000000000000000]              
	float                                              m_MaxHitEffectIntensity;                          		// 0x0694 (0x0004) [0x0000000000000000]              
	struct FLinearColor                                m_MaxHitEffectColor;                              		// 0x0698 (0x0010) [0x0000000000000000]              
	class UMaterialEffect*                             m_SniperZoomEffect;                               		// 0x06A8 (0x0004) [0x0000000000000000]              
	class UDOFAndBloomEffect*                          m_ZoomBlurEffect;                                 		// 0x06AC (0x0004) [0x0000000000000000]              
	float                                              m_ZoomEffectFadeTime;                             		// 0x06B0 (0x0004) [0x0000000000000000]              
	float                                              m_CurrentZoomFOV;                                 		// 0x06B4 (0x0004) [0x0000000000000000]              
	float                                              m_EndZoomFOV;                                     		// 0x06B8 (0x0004) [0x0000000000000000]              
	float                                              m_StartZoomFOV;                                   		// 0x06BC (0x0004) [0x0000000000000000]              
	class UTgUISceneManager*                           m_UISceneManager;                                 		// 0x06C0 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD*                             m_PrimaryHUD;                                     		// 0x06C4 (0x0004) [0x0000000000000000]              
	class UTgUISettingsMenu_Controls*                  m_KeyBindMenu;                                    		// 0x06C8 (0x0004) [0x0000000000000000]              
	class UTgUIMainMenu*                               m_MainMenu;                                       		// 0x06CC (0x0004) [0x0000000000000000]              
	class UTgUIReceivedItemScreen*                     m_RewardScreen;                                   		// 0x06D0 (0x0004) [0x0000000000000000]              
	class UTgUICampaignMenu_HexMap*                    m_CampaignScreen;                                 		// 0x06D4 (0x0004) [0x0000000000000000]              
	class UTgUIAgencyMenu_Facilities*                  m_FacilitiesScreen;                               		// 0x06D8 (0x0004) [0x0000000000000000]              
	class UMaterialEffect*                             m_BinocularZoomMaterialEffect;                    		// 0x06DC (0x0004) [0x0000000000000000]              
	class UTgUIMissionScene*                           m_MissionScene;                                   		// 0x06E0 (0x0004) [0x0000000000000000]              
	class UTgUIMissionScene_Zone*                      m_ZoneMissionScene;                               		// 0x06E4 (0x0004) [0x0000000000000000]              
	class UTgUIAgentProfile_Equip*                     m_AgentEquipScene;                                		// 0x06E8 (0x0004) [0x0000000000000000]              
	class UTgAgencyData*                               m_AgencyData;                                     		// 0x06EC (0x0004) [0x0000000000000000]              
	class UTgAllianceData*                             m_AllianceData;                                   		// 0x06F0 (0x0004) [0x0000000000000000]              
	class UTgAchievementData*                          m_AchievementData;                                		// 0x06F4 (0x0004) [0x0000000000000000]              
	class UTgAttentionData*                            m_AttentionData;                                  		// 0x06F8 (0x0004) [0x0000000000000000]              
	TArray< int >                                      m_RecentAchievements;                             		// 0x06FC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UMaterialInstanceConstant*                   m_ZoomEffectMaterialInstance;                     		// 0x0708 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	TArray< struct FDamageDirectionInfo >              c_DamageDirectionData;                            		// 0x070C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                c_MaxNumOfDamgeDirectionIndicators;               		// 0x0718 (0x0004) [0x0000000000000000]              
	class UMaterial*                                   c_DamageDirectionParentMaterial;                  		// 0x071C (0x0004) [0x0000000000000000]              
	float                                              c_DamageDirectionFadeTime;                        		// 0x0720 (0x0004) [0x0000000000000000]              
	struct FName                                       c_DamageDirectionPositionalParamName;             		// 0x0724 (0x0008) [0x0000000000000000]              
	float                                              c_DamageDirectionDamageAmount;                    		// 0x072C (0x0004) [0x0000000000000000]              
	struct FName                                       c_DamageDirectionDamageAmountName;                		// 0x0730 (0x0008) [0x0000000000000000]              
	struct FName                                       c_DamageDirectionFadeParamName;                   		// 0x0738 (0x0008) [0x0000000000000000]              
	struct FName                                       c_DamageStateName;                                		// 0x0740 (0x0008) [0x0000000000000000]              
	float                                              c_ResolutionScale;                                		// 0x0748 (0x0004) [0x0000000000000000]              
	struct FDamageDirectionInfo                        c_BodyUnderAttackData;                            		// 0x074C (0x0018) [0x0000000000000000]              
	class UMaterial*                                   c_BodyUnderAttackParentMaterial;                  		// 0x0764 (0x0004) [0x0000000000000000]              
	float                                              c_BodyUnderAttackFadeTime;                        		// 0x0768 (0x0004) [0x0000000000000000]              
	struct FDamageDirectionInfo                        c_BeingLockedData;                                		// 0x076C (0x0018) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   c_BeingLockedParentMaterial;                      		// 0x0784 (0x0004) [0x0000000000000000]              
	float                                              c_BeingLockedFadeTime;                            		// 0x0788 (0x0004) [0x0000000000000000]              
	struct FDamageDirectionInfo                        c_BeingTargetedData;                              		// 0x078C (0x0018) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   c_BeingTargetedParentMaterial;                    		// 0x07A4 (0x0004) [0x0000000000000000]              
	float                                              c_BeingTargetedFadeTime;                          		// 0x07A8 (0x0004) [0x0000000000000000]              
	struct FDamageDirectionInfo                        c_BeingDetectedData;                              		// 0x07AC (0x0018) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   c_BeingDetectedParentMaterial;                    		// 0x07C4 (0x0004) [0x0000000000000000]              
	float                                              c_BeingDetectedFadeTime;                          		// 0x07C8 (0x0004) [0x0000000000000000]              
	struct FDamageDirectionInfo                        c_ActiveCombatData;                               		// 0x07CC (0x0018) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   c_ActiveCombatParentMaterial;                     		// 0x07E4 (0x0004) [0x0000000000000000]              
	float                                              c_ActiveCombatFadeTime;                           		// 0x07E8 (0x0004) [0x0000000000000000]              
	struct FDamageDirectionInfo                        c_InBombRangeData;                                		// 0x07EC (0x0018) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   c_InBombRangeParentMaterial;                      		// 0x0804 (0x0004) [0x0000000000000000]              
	float                                              c_InBombRangeFadeTime;                            		// 0x0808 (0x0004) [0x0000000000000000]              
	struct FVector2D                                   c_RemainingShiftingReticuleDistance;              		// 0x080C (0x0008) [0x0000000000000000]              
	struct FVector2D                                   c_CurrentReticulePos;                             		// 0x0814 (0x0008) [0x0000000000000000]              
	struct FVector2D                                   c_DestinationReticulePos;                         		// 0x081C (0x0008) [0x0000000000000000]              
	float                                              c_ShiftingReticuleSpeed;                          		// 0x0824 (0x0004) [0x0000000000000000]              
	class UTgStatsData*                                c_StatsData;                                      		// 0x0828 (0x0004) [0x0000000000000000]              
	class UTgVoiceMacroTree*                           c_VoiceMacroTree;                                 		// 0x082C (0x0004) [0x0000000000000000]              
	class AActor*                                      c_SceneTriggerActor;                              		// 0x0830 (0x0004) [0x0000000000000000]              
	struct FVector                                     m_vWorldMouseLocation;                            		// 0x0834 (0x000C) [0x0000000000000000]              
	struct FVector                                     m_vWorldMouseDirection;                           		// 0x0840 (0x000C) [0x0000000000000000]              
	struct FVector                                     m_vMouseHoverLocation;                            		// 0x084C (0x000C) [0x0000000000000000]              
	struct FVector                                     m_vMouseHitNormal;                                		// 0x0858 (0x000C) [0x0000000000000000]              
	TArray< struct FPointer >                          RewardQueue;                                      		// 0x0864 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UTgUIChat*                                   m_ChatUI;                                         		// 0x0870 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_AcceptQuestCue;                                 		// 0x0874 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_TurninQuestCue;                                 		// 0x0878 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_AbandonQuestCue;                                		// 0x087C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_QuestHUDTagMIC;                                 		// 0x0880 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_CompleteQuestHUDTagMIC;                         		// 0x0884 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_QuestOffscreenTagMIC;                           		// 0x0888 (0x0004) [0x0000000000000000]              
	float                                              m_fLastQuestHUDOpacity[ 0x2 ];                    		// 0x088C (0x0008) [0x0000000000000000]              
	struct FString                                     m_CachedMapnameString;                            		// 0x0894 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_endKismetEvents;                                		// 0x08A0 (0x000C) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
	TArray< int >                                      m_runOnceBilboards;                               		// 0x08AC (0x000C) [0x0000000000003002]              ( CPF_Const | CPF_Native | CPF_Transient )
	int                                                m_RequestingVendorScene;                          		// 0x08B8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1596 ];

		return pClassPointer;
	};

	void ShowPathToAttentionPOI ( );
	void StartVoiceMacroTree ( );
	void eventClearTriggeredSceneInfo ( );
	void eventSetTriggeredSceneInfo ( class AActor* triggerActor );
	void CycleTrackedAssignment ( );
	void ShowDevNotes ( );
	void BrowseToDevNotes ( );
	void UpdateReleaseTime ( float fReleaseTime );
	class UTgUIReleaseMenus* GetActiveReleaseMenu ( );
	void ShowReleaseDialog ( unsigned long bShow, struct FDeathZoomInfo Info, float fReleaseTime, unsigned long bCanManuallyRelease );
	class UTgUISceneDriver* ShowDevSpawnBotMenu ( );
	void ForceUpdateDeviceBar ( int nActiveDeviceId );
	void HideWorldInteractionText ( );
	void ShowWorldInteractionTextById ( int nMsgId );
	void ShowWorldInteractionText ( struct FString sMsg );
	void HideProgressBar ( unsigned long bWorldInteractionBar );
	void StartProgressBar ( float fDuration, struct FString sLabel, unsigned long bWorldInteractionBar );
	void DisplayBadConnectionAlert ( );
	void HideTaskForce ( );
	void ShowTaskForce ( );
	void SetReticuleVisibility ( unsigned long bCrossHairVisible, unsigned long bWeaponStatusVisible );
	void eventTick ( float DeltaTime );
	void NativeTick ( float DeltaTime );
	void PlayZoomBlur ( );
	void PlayZoomEffect ( unsigned long bEnable, unsigned char ZoomType );
	class UMaterialInstanceConstant* GetZoomEffectByAspectRatio ( );
	void SetZoomScope ( struct FName ScopeName );
	void DisplayDamage ( );
	void UpdateDamage ( );
	float CalcHitDirection ( struct FVector HitDir );
	void PlayHitEffect ( float Damage, struct FVector HitDir );
	void RadarMapRotates ( unsigned long RotatingMapStyle );
	void LogBreak ( int I );
	bool SelectPreviousDevice ( );
	bool SelectNextDevice ( );
	void UpdateSelectedDevice ( class ATgDevice* Dev );
	bool UsePickupDevice ( );
	bool SelectDeviceBarType ( unsigned char eType );
	bool SelectDeviceBarSlot ( int Slot );
	void ToggleDrawAllHUD ( );
	void ToggleDrawPawnHUD ( );
	bool ShouldDrawPawnHUD ( );
	void ToggleTargetExtraInfo ( );
	void eventShowRaidTab ( int nTab );
	void eventCheckLevelUnlocks ( unsigned long bIsTutorial );
	class UUIScene* eventOpenIntroScene ( float FadeTime, float TotalTime );
	void TgFxLightsOn ( unsigned long bOn );
	void OpenDevMenu ( );
	void LiveTeamPerfStats ( );
	bool IsForValidPlayer ( );
	void eventShowEndRaidScreen ( unsigned long bAttackerWon );
	void ShowQuestEndMissionScene ( unsigned long MissionVictor, unsigned long PlayerVictor, unsigned long bTie );
	void ShowEndMissionScreen ( unsigned long MissionVictor, unsigned long PlayerVictor, unsigned long bTie );
	void UpdateScoreBoard ( unsigned long bPlayerVictor, unsigned long bTie );
	void eventCloseConfirmPopup ( class UFunction* callbackPtr, unsigned long bTellGameClient );
	void eventCreateConfirmPopup ( struct FString Message, class UFunction* callbackPtr );
	class UTgUIPreviewScreen* eventViewItemPreview ( class UTgInventoryObject* Item );
	class UTgUISceneDriver* ToggleMainMenuScreen ( );
	void OpenSkillScreen ( );
	void OpenInventoryLog ( );
	void OpenItemModScreen ( );
	void OpenAgentScreen ( unsigned long bIsTutorial );
	void ShowGametypeInfo ( );
	void OpenLFGScreen ( );
	void OpenZoneMissions ( );
	void OpenAgencyDirectory ( );
	void OpenMailScreen ( );
	void OpenAuctionScreen ( );
	bool eventMainMenuDisplayed ( );
	void eventOpenPlayerSearchPrepopulatedById ( int nPlayerId );
	void eventOpenPlayerSearchPrepopulated ( struct FString sPlayerName, struct FString sAgencyName, struct FString sAllianceName );
	class UTgUISceneDriver* eventSelectAgentTab ( unsigned char Tab, class ATgPawn_Character* InspectedPawn );
	void eventSelectAgencyTab ( unsigned char Tab );
	void ZoneMap ( );
	void MissionLog ( );
	void OpenAgencyMenu ( );
	void OpenNavScreen ( );
	void OpenCharacterSkillScreen ( );
	void OpenCharacterStatsScreen ( );
	void OpenTeamDetailScreen ( );
	void OpenInventoryScreen ( );
	void OpenInstanceSelect ( );
	void OpenAchievementsNew ( );
	class UTgUIProductionPopup* eventOpenProductionPopup ( class UTgUISceneDriver* ParentScene );
	class UTgUIInventoryPopup* eventOpenFilteredInventoryPopup ( class UTgUISceneDriver* ParentScene );
	void OpenHexAuction ( int nHexId );
	void eventSelectCampaignTab ( unsigned char Tab );
	void OpenCombatAlerts ( );
	void OpenHexMap ( );
	void OpenHexLeaderboard ( );
	void TestSetHexBackground ( int nResourceId );
	void ShowDeviceBarHUD ( unsigned long Show );
	struct FName GetInputKeyForCommand ( class UPlayerInput* Input, struct FString CommandName );
	struct FString HUDLocalGetBind ( class UPlayerInput* Input, struct FName BindName );
	float eventGetAmountCurrentlyOffOfTargetAccuracy ( );
	float eventGetCurrentDeviceAccuracy ( );
	struct FString FormatTime ( );
	float ScaleY ( float Y );
	float ScaleX ( float X );
	bool InRange ( float X, float lower, float upper );
	void SizeInt ( int Value, float Scale, float* XL, float* YL );
	void DrawInt ( int Value, float X, float Y, float Scale );
	void DrawSpectatorHud ( );
	void DrawTargetText ( struct FString Str );
	void DrawHoverText ( struct FString Str );
	void DrawHealth ( );
	void DrawTextPanel ( );
	void AddNewDisplayMessage ( struct FString sMessage );
	void AddNewKilledTarget ( struct FString KilledName, struct FString KillerName, unsigned long KillerWasPlayer );
	void AddConsoleMessage ( struct FString M, class UClass* InMessageClass, class APlayerReplicationInfo* PRI, float Lifetime );
	void DisplayConsoleMessages ( );
	void eventCloseGUIItems ( );
	void eventOpenGUIItems ( );
	void DisplayDebug ( class AHUD* HUD, float* out_YL, float* out_YPos );
	void CancelActiveCombat ( );
	void DisplayActiveCombat ( int nStateParam );
	void CancelBeingDetected ( );
	void DisplayBeingDetected ( int nStateParam );
	void CancelBeingTargeted ( );
	void DisplayBeingTargeted ( );
	void CancelInBombRange ( );
	void DisplayInBombRange ( );
	void CancelBeingLocked ( );
	void DisplayBeingLocked ( );
	void CancelBodyUnderAttack ( );
	void DisplayBodyUnderAttack ( int Damage );
	void DisplayHit ( struct FVector HitDir, int Damage, class UClass* DamageType );
	void eventPostRender ( );
	void PostRenderScenes ( );
	void PostRenderSceneTooltips ( );
	class UObject* GetTgUISceneDriver ( class UUIScene* Scene );
	bool TriggerCloseTgUIScene ( class UUIScene* Scene );
	class UUIScene* TriggerOpenTgUIScene ( class UUIScene* Scene );
	void InitializeTgUIScenes ( );
	void PostBeginPlay ( );
	void DrawCommandInfo ( class ATgPlayerController* PC );
	void DrawHUD ( );
	class UTgGameViewportClient* GetTgGameViewportClient ( );
	struct FVector GetMouseHitNormal ( );
	struct FVector GetMouseHoverLocation ( );
	void TestAgencyRemoveItem ( int nInvId, int nInstanceCount, int nItemId );
	void TestAgencyAddItem ( int nInvId, int nInstanceCount );
	void TestAgencyUpdateCurrency ( int nCurrencyChange );
	void TestStrikeForceRemoveItem ( int nInvId, int nInstanceCount, int nTaskTeam );
	void TestStrikeForceAddItem ( int nInvId, int nItemId, int nInstanceCount, int nTaskTeam );
	void TestRemoveFactoryItem ( int nZoneObjectId, int nPosition, int nQueueId, int nCount );
	void TestAddFactoryItem ( int nZoneObjectId, int nPosition, int nInvId, int nCount );
	void TestAddFacility ( int nZoneObjectId, int nPosition, int nInvId );
	void CheckStorePrompt ( );
	void InitQuestVisuals ( );
	void ClientTestAgencyUpdateCurrency ( int nCurrencyChange );
	void ClientTestAgencyRemoveItem ( int nInvId, int nInstanceCount, int nItemId );
	void ClientTestAgencyAddItem ( int nInvId, int nInstanceCount );
	void ClientTestRemoveFactoryItem ( int nZoneObjectId, int nPosition, int nQueueId, int nCount );
	void ClientTestAddFactoryItem ( int nZoneObjectId, int nPosition, int nInvId, int nCount );
	void ClientTestAddFacility ( int nZoneObjectId, int nPosition, int nInvId );
	void ClientTestStrikeForceRemoveItem ( int nInvId, int nInstanceCount, int nTaskTeam );
	void ClientTestStrikeForceAddItem ( int nInvId, int nItemId, int nInstanceCount, int nTaskTeam );
	void UpdateRangedTargeterState ( unsigned char eNewState );
	void UpdateMouseVector ( );
	void RevealHiddenRewardScreen ( );
	void DismissRewardScreen ( );
	void SetRewardScreenReveal ( float fSeconds );
	void SetRewardScreenAutoDismiss ( float fSeconds );
	bool IsRewardScreenOpen ( );
	void OpenNextRewardScreen ( );
	bool GetControllerAndViewport ( class ATgPlayerController** pController, class UTgGameViewportClient** pViewport );
	class UTgUIMapTransition* GetMapTransition ( );
	void ToggleMapNameDisplay ( );
	void SetDepthTestingEnabled ( class UCanvas* pCanvas, unsigned long bEnable );
	void DrawObjectiveOverlays ( struct FVector CameraPosition, struct FVector CameraDir );
	void DrawActorOverlays ( );
	void ResubmitPopups ( );
	bool CheckShouldSkipEndMission ( );
	bool HideObjectiveNames ( );
	bool HideObjectives ( );
	bool HideEnemyTags ( );
	bool ShowCombatText ( );
	bool ShowDeployableHealthBars ( );
	bool CheckDisableWorldRendering ( );
	void OpenTeamScreen ( );
	class UTgUISceneDriver* OpenManagedSceneByID ( unsigned char funcID );
	class UTgUISceneDriver* ToggleManagedSceneByID ( unsigned char funcID, unsigned long bOpenedFromUIVolume );
	class UTgUISceneDriver* ToggleManagedScene ( struct FName ClassName, unsigned long bOpenedFromUIVolume );
	void ChatCycleTab ( unsigned long Increment );
	void ChatOpacity ( float ChatOpacityIn );
	void ChatEnd ( );
	void ChatPageDown ( unsigned long Active );
	void ChatPageUp ( unsigned long Active );
	void ChatToggleMode ( );
	void ChatToggleOnTop ( );
	void ChatStartReply ( );
	void ChatStartSlash ( );
	void ChatStartDefault ( );
	void FreemouseOnHUD ( unsigned long bFree );
	void GetStringDimensions ( class UFont* fontToUse, struct FString checkString, int* StringWidth, int* StringHeight );
	void UpdateUITextBox ( unsigned char TextBoxBitfield, float Duration, int messageID );
	void UpdateUITimer ( unsigned char ClockStateBitfield, float TimerDuration );
	void InitializeVoiceMacroTree ( );
	void InitializeCommonDataStores ( );
	void NativePostBeginPlay ( );
	void UseCrosshairForReticule ( struct FName CrosshairName );
	void ResetDeviceHudTimersUI ( int DeviceEqpPoint );
	void UpdateInHandDevicesUI ( class ATgDevice* pDevice );
	void UpdateHiddenDevicesUI ( );
	void UpdateDeviceBarUIItems ( );
	void UpdateModeEquipTimer ( int nIsActive, float fEquipTime );
	void UpdateDeviceSlotTimers ( int nDeviceInstanceId, int nDeviceMode, float fSetup, float fRecharge, float fElaspedCooldown );
	void TriggerDeviceRefresh ( );
};


// Class TgClient.TgLoginHUD
// 0x0028 (0x05C4 - 0x059C)
class ATgLoginHUD : public ATgHUD
{
public:
	class UTgUISceneManager*                           m_UISceneManager;                                 		// 0x059C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bCreateCharacterTriggered : 1;                  		// 0x05A0 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPlayTutorial : 1;                              		// 0x05A0 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bTutorialMoviePlayed : 1;                       		// 0x05A0 (0x0004) [0x0000000000000000] [0x00000004] 
	int                                                m_nClassToCreate;                                 		// 0x05A4 (0x0004) [0x0000000000000000]              
	struct FName                                       m_nmPreplayMovie;                                 		// 0x05A8 (0x0008) [0x0000000000000000]              
	class UTgDataSet*                                  c_DataSet;                                        		// 0x05B0 (0x0004) [0x0000000000000000]              
	class UTgDataInterface*                            c_DataInterface;                                  		// 0x05B4 (0x0004) [0x0000000000000000]              
	class AInterpActor*                                m_CachedEarthModel;                               		// 0x05B8 (0x0004) [0x0000000000000000]              
	class UTgUILoginMenu*                              m_LoginMenu;                                      		// 0x05BC (0x0004) [0x0000000000000000]              
	int                                                m_nNumCharacters;                                 		// 0x05C0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1597 ];

		return pClassPointer;
	};

	void ShowDevNotes ( );
	void BrowseToDevNotes ( );
	void eventBackToLogin ( );
	void eventSendCharCreateErrorCode ( int ErrorCode );
	void eventCreateCharacter ( );
	void ConditionalSetTutorialFlag ( );
	class UTgUIClassSelectScene* LoadClassSelect ( );
	class UTgUICustomHeadMenu* LoadCustomHeadMenu ( );
	class UTgUICreateCharA* LoadCharCreateMenu ( );
	void eventTick ( float DeltaTime );
	void NativeTick ( float DeltaTime );
	void InitializeDataInterface ( );
	void OpenMenu ( );
	void PostBeginPlay ( );
	void SetMatineeCamToLogin ( unsigned long bSetToLogin );
	void LoadCharacterSelectMenu ( unsigned long bError );
	void LoadLoginMenu ( );
	void InitializeTgUIScenes ( );
	class AInterpActor* GetEarthModel ( );
	class ATgSkeletalMeshActor_CharacterBuilder* GetCharacterBuilderSkelMeshActor ( );
	void GetCharMorphSettings ( struct FCharacterInfoStruct* CharInfo );
	class UTgUIMapTransition* GetMapTransition ( );
	void AddPlayerCharacterCallback ( int eType );
};


// Class TgClient.TgLoginInfo
// 0x0004 (0x03AC - 0x03A8)
class ATgLoginInfo : public AGameInfo
{
public:
	class UTgDebug*                                    m_DebugSettings;                                  		// 0x03A8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1598 ];

		return pClassPointer;
	};

	void PostBeginPlay ( );
	bool ShouldSkipLoginHUD ( );
	bool HasSelectedCharacter ( );
	bool IsInGame ( );
	bool IsLoggedIn ( );
	void CreateDebugStructure ( );
};


// Class TgClient.TgLoginPlayerController
// 0x0000 (0x0920 - 0x0920)
class ATgLoginPlayerController : public ATgPlayerController
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1599 ];

		return pClassPointer;
	};

	void StopAltFire ( unsigned char FireModeNum );
	void StartAltFire ( unsigned char FireModeNum );
	void StartFire ( );
	void NextWeapon ( );
	void PrevWeapon ( );
	void PostBeginPlay ( );
	bool CheckAutoLogin ( );
};


// Class TgClient.TgStatsData
// 0x0A78 (0x0AB4 - 0x003C)
class UTgStatsData : public UObject
{
public:
	unsigned long                                      m_bGameOver : 1;                                  		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	struct FScores                                     m_ScoreArray[ 0x18 ];                             		// 0x0040 (0x0A20) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_RankArray[ 0xA ];                               		// 0x0A60 (0x0028) [0x0000000000000000]              
	int                                                m_HighScoreArray[ 0x8 ];                          		// 0x0A88 (0x0020) [0x0000000000000000]              
	int                                                m_nPlayerCount;                                   		// 0x0AA8 (0x0004) [0x0000000000000000]              
	float                                              m_fLastCopy;                                      		// 0x0AAC (0x0004) [0x0000000000000000]              
	class ATgHUD_Game*                                 m_HUD;                                            		// 0x0AB0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1600 ];

		return pClassPointer;
	};

	void eventInit ( class ATgHUD_Game* HUD );
	int GetPlayerCount ( );
	class ATgRepInfo_Player* GetMyPRI ( );
	void PopulatePlayerRankArray ( );
	int GetBestPlayerRankIndex ( );
	void SortScores ( int eSortField );
	void CopyScores ( );
	void SetGameOver ( unsigned long bGameover, unsigned long bPlayerVictor );
};


// Class TgClient.TgUILevelMap
// 0x01C0 (0x01FC - 0x003C)
class UTgUILevelMap : public UObject
{
public:
	unsigned long                                      m_bInitialized : 1;                               		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bActive : 1;                                    		// 0x003C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bTransitioning : 1;                             		// 0x003C (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bRotatePoints : 1;                              		// 0x003C (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_DesiredOpacityChange : 1;                       		// 0x003C (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_DesiredOpacityIncrease : 1;                     		// 0x003C (0x0004) [0x0000000000000000] [0x00000020] 
	float                                              m_TimeOfTick;                                     		// 0x0040 (0x0004) [0x0000000000000000]              
	class ATgHUD_Game*                                 m_HUDParent;                                      		// 0x0044 (0x0004) [0x0000000000000000]              
	struct FVector                                     m_Location;                                       		// 0x0048 (0x000C) [0x0000000000000000]              
	struct FRotator                                    m_Rotation;                                       		// 0x0054 (0x000C) [0x0000000000000000]              
	float                                              m_fZoomBase;                                      		// 0x0060 (0x0004) [0x0000000000000000]              
	float                                              m_fZoomScale;                                     		// 0x0064 (0x0004) [0x0000000000000000]              
	float                                              m_fOriginalXResolution;                           		// 0x0068 (0x0004) [0x0000000000000000]              
	float                                              m_fOriginalYResolution;                           		// 0x006C (0x0004) [0x0000000000000000]              
	struct FVector                                     m_CurrentMapScale;                                		// 0x0070 (0x000C) [0x0000000000000000]              
	struct FVector                                     m_CurrentMapStart;                                		// 0x007C (0x000C) [0x0000000000000000]              
	int                                                m_LocalPlayerRotation;                            		// 0x0088 (0x0004) [0x0000000000000000]              
	struct FVector                                     m_LocalPlayerLocation;                            		// 0x008C (0x000C) [0x0000000000000000]              
	TArray< struct FsPlayerWidgetData >                m_PlayerWidgetData;                               		// 0x0098 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsDeployableWidgetData >            m_DeployableWidgetData;                           		// 0x00A4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsObjectiveWidgetData >             m_ObjectiveWidgetData;                            		// 0x00B0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsQuestVolumeWidgetData >           m_QuestWidgetData;                                		// 0x00BC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	float                                              m_DesiredOpacity;                                 		// 0x00C8 (0x0004) [0x0000000000000000]              
	float                                              m_CurrentOpacity;                                 		// 0x00CC (0x0004) [0x0000000000000000]              
	float                                              m_TimeOfRealStart;                                		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_BlankRadarTexture;                              		// 0x00D4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconLocalPlayer;                                		// 0x00D8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconTeamPlayer;                                 		// 0x00DC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconTeamPlayerBlink;                            		// 0x00E0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconTeamMedic;                                  		// 0x00E4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconTeamMedicBlink;                             		// 0x00E8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconFriendlyPlayer;                             		// 0x00EC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconFriendlyPlayerBlink;                        		// 0x00F0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconFriendlyMedic;                              		// 0x00F4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconFriendlyMedicBlink;                         		// 0x00F8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconEnemy;                                      		// 0x00FC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconFriendlyNPC;                                		// 0x0100 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconSound;                                      		// 0x0104 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconDeployableSensor;                           		// 0x0108 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconHealthDeploy;                               		// 0x010C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconSpawnPoint;                                 		// 0x0110 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconObjective;                                  		// 0x0114 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconQuestAvailable;                             		// 0x0118 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconQuestTurninUnavailable;                     		// 0x011C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconQuestTurnin;                                		// 0x0120 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconQuestRepeatableAvailable;                   		// 0x0124 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconQuestRepeatableTurnin;                      		// 0x0128 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_TeamIndicatorParent;                            		// 0x012C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_FriendlyIndicatorParent;                        		// 0x0130 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_ObjIndicatorParent;                             		// 0x0134 (0x0004) [0x0000000000000000]              
	TArray< class UMaterialInstanceConstant* >         m_TeamIndicatorArrows;                            		// 0x0138 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UMaterialInstanceConstant* >         m_ObjIndicatorArrows;                             		// 0x0144 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UMaterialInstanceConstant*                   m_TrackedQuestIndicatorArrow;                     		// 0x0150 (0x0004) [0x0000000000000000]              
	int                                                m_FoundOffscreenQuests;                           		// 0x0154 (0x0004) [0x0000000000000000]              
	int                                                m_bFoundTeamMember[ 0x14 ];                       		// 0x0158 (0x0050) [0x0000000000000000]              
	int                                                m_FoundTeamMembers;                               		// 0x01A8 (0x0004) [0x0000000000000000]              
	int                                                m_FoundObjectives;                                		// 0x01AC (0x0004) [0x0000000000000000]              
	float                                              m_IndicatorX;                                     		// 0x01B0 (0x0004) [0x0000000000000000]              
	float                                              m_IndicatorY;                                     		// 0x01B4 (0x0004) [0x0000000000000000]              
	float                                              m_IndicatorWid;                                   		// 0x01B8 (0x0004) [0x0000000000000000]              
	float                                              m_IndicatorHgt;                                   		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BaseImageWidget;                                		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_SizeGuidelineWidget;                            		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HelpLabel;                                      		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MapMIC;                                         		// 0x01CC (0x0004) [0x0000000000000000]              
	class UMaterialInstance*                           m_ActiveMapMaterialParent;                        		// 0x01D0 (0x0004) [0x0000000000000000]              
	struct FName                                       m_ActiveMapMaterialParentName;                    		// 0x01D4 (0x0008) [0x0000000000000000]              
	class UTexture*                                    m_MapImage;                                       		// 0x01DC (0x0004) [0x0000000000000000]              
	class UTexture*                                    m_MapImage2;                                      		// 0x01E0 (0x0004) [0x0000000000000000]              
	float                                              m_WidgetLeft;                                     		// 0x01E4 (0x0004) [0x0000000000000000]              
	float                                              m_WidgetTop;                                      		// 0x01E8 (0x0004) [0x0000000000000000]              
	float                                              m_WidgetWidth;                                    		// 0x01EC (0x0004) [0x0000000000000000]              
	float                                              m_WidgetHeight;                                   		// 0x01F0 (0x0004) [0x0000000000000000]              
	float                                              m_fDesiredScale;                                  		// 0x01F4 (0x0004) [0x0000000000000000]              
	float                                              m_DesiredOpacityChangeDelay;                      		// 0x01F8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1601 ];

		return pClassPointer;
	};

	void InitializeUniformScaleRadarMap ( );
	void ChangeDesiredMapOpacity ( unsigned long Activate, unsigned long Increase );
	void PostRender ( );
	void GenMapElements ( );
	void DrawMapElements ( class UCanvas* DrawCanvas );
	void UpdateAnimatingValues ( float TimeDelay );
	void SetOpacity ( float Opacity );
	void Hide ( unsigned long Immediate );
	void Show ( unsigned long Immediate );
	float GetWorldTime ( );
	void TickLevelMap ( float UpdateTime );
	void InitLevelMap ( class UMaterialInstanceConstant* LevelMapMIC, class ATgHUD* ParentTgHUD );
};


// Class TgClient.TgUIPrimaryHUD_ActiveItem
// 0x0030 (0x006C - 0x003C)
class UTgUIPrimaryHUD_ActiveItem : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bProgressBarActive : 1;                         		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bFinished : 1;                                  		// 0x0040 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bProgressBarTextNeedsUpdate : 1;                		// 0x0040 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bActiveTextActive : 1;                          		// 0x0040 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bProgressBarInSecondaryMode : 1;                		// 0x0040 (0x0004) [0x0000000000000000] [0x00000010] 
	float                                              m_fTimeStarted;                                   		// 0x0044 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeDuration;                                  		// 0x0048 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeDone;                                      		// 0x004C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ProgressBarBG;                                  		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUIProgressBar*                              m_ProgressBar;                                    		// 0x0054 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ProgressBarLabel;                               		// 0x0058 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ActiveTextLabel;                                		// 0x005C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ActiveTextImage;                                		// 0x0060 (0x0004) [0x0000000000000000]              
	float                                              m_fProgressBarSecondaryValue;                     		// 0x0064 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0068 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1602 ];

		return pClassPointer;
	};

	void SetSecondaryValue ( float fSecondaryValue );
	void SetSecondaryModeOn ( unsigned long bSecondaryModeOn );
	void ShowActiveText ( );
	void HideActiveText ( );
	void SetActiveTextById ( int nMsgId );
	void SetActiveText ( struct FString sMessage );
	void UpdateProgressBar ( float TimeNow );
	void ShowProgressBar ( );
	void HideProgressBar ( );
	void DeactivateProgressBar ( );
	void SetProgressBarText ( struct FString newProgressBarText );
	void StartProgressBar ( float TimeDuration, struct FString ProgressLabel );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void TickPrimaryHUDElement ( float TimeNow );
};


// Class TgClient.TgUIPrimaryHUD_AgentInfo
// 0x009C (0x00D8 - 0x003C)
class UTgUIPrimaryHUD_AgentInfo : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0040 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNeedsAgencyUpdate : 1;                         		// 0x0044 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bNeedsPlayerUpdate : 1;                         		// 0x0044 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bNeedsQueueUpdate : 1;                          		// 0x0044 (0x0004) [0x0000000000000000] [0x00000004] 
	int                                                m_nCachedPlayerXP;                                		// 0x0048 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedPlayerCurrency;                          		// 0x004C (0x0004) [0x0000000000000000]              
	int                                                m_nCachedPlayerToken;                             		// 0x0050 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedPlayerHZPoints;                          		// 0x0054 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedQueueId;                                 		// 0x0058 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedUnreadMail;                              		// 0x005C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BasePanel;                                      		// 0x0060 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TitlePanel;                                     		// 0x0064 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_MailIcon;                                       		// 0x0068 (0x0004) [0x0000000000000000]              
	float                                              m_MailIconRightPos;                               		// 0x006C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgencyNamePanel;                                		// 0x0070 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AgencyNameLabel;                                		// 0x0074 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceNamePanel;                              		// 0x0078 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceNameLabel;                              		// 0x007C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_PlayerTitlePanel;                               		// 0x0080 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PlayerTitleLabel;                               		// 0x0084 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_UpperLinePanel;                                 		// 0x0088 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_LocationPanel;                                  		// 0x008C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LocationLabel;                                  		// 0x0090 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_LocationLinePanel;                              		// 0x0094 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_PlayerStatsPanel;                               		// 0x0098 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Currency;                                       		// 0x009C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Level;                                          		// 0x00A0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Token;                                          		// 0x00A4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HZPoints;                                       		// 0x00A8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_XPPanel;                                        		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_XPProgressBar;                                  		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_XPProgressBarMICParent;                         		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_XPProgressBarMIC;                               		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_XPValue;                                        		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MiddleLinePanel;                                		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_QueuePanel;                                     		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QueueStatusLabel;                               		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QueueAbortLabel;                                		// 0x00CC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BottomPanel;                                    		// 0x00D0 (0x0004) [0x0000000000000000]              
	int                                                c_nNewMailSoundCueResId;                          		// 0x00D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1603 ];

		return pClassPointer;
	};

	bool CheckQueueUpdate ( );
	bool CheckPlayerUpdate ( );
	void UpdatePositioning ( );
	void UpdatePlayerValues ( );
	void UpdateQueueValues ( );
	void UpdateAgencyValues ( );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_BeaconStatus
// 0x0024 (0x0060 - 0x003C)
class UTgUIPrimaryHUD_BeaconStatus : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_BeaconStatusMIC;                                		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BeaconStatusImage;                              		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BeaconHolderLabel;                              		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ReviveLengthLabel;                              		// 0x004C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bInitialized : 1;                               		// 0x0050 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned char                                      m_CurrentStatus;                                  		// 0x0054 (0x0001) [0x0000000000000000]              
	float                                              m_CurrentDeployAmount;                            		// 0x0058 (0x0004) [0x0000000000000000]              
	float                                              m_CurrentFlashProgressionAmount;                  		// 0x005C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1604 ];

		return pClassPointer;
	};

	void UpdateBeaconDirection ( struct FVector NewLoc );
	class ATgRepInfo_TaskForce* GetLocalPlayerTaskForce ( );
	void UpdateBeaconHUDStatus ( unsigned char NewStatus );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_BuffBar
// 0x0174 (0x01B0 - 0x003C)
class UTgUIPrimaryHUD_BuffBar : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	struct FsBuffWidgetData                            m_BuffWidgets[ 0xA ];                             		// 0x0040 (0x0168) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x01A8 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bDebuffBar : 1;                                 		// 0x01AC (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1605 ];

		return pClassPointer;
	};

	void CheckBuffs ( );
	void Reset ( unsigned long bHide );
	bool AddBuff ( int EffectID, int nIndex, float TimeRemaining, int nNumStacks );
	void UpdateBuffWidget ( int WidgetIndex );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void TickPrimaryHUDElement ( float TimeNow );
};


// Class TgClient.TgUIPrimaryHUD_Clock
// 0x002C (0x0068 - 0x003C)
class UTgUIPrimaryHUD_Clock : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsAlarm : 1;                                   		// 0x0040 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bUsesBar : 1;                                   		// 0x0040 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bIsMissionTimer : 1;                            		// 0x0040 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bCountingDown : 1;                              		// 0x0040 (0x0004) [0x0000000000000000] [0x00000010] 
	class UUIPanel*                                    m_BaseClockPanel;                                 		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ImageClock;                                     		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ImageAlarm;                                     		// 0x004C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LabelText;                                      		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TimeDisplayText;                                		// 0x0054 (0x0004) [0x0000000000000000]              
	class UUIProgressBar*                              m_TimeDisplayBar;                                 		// 0x0058 (0x0004) [0x0000000000000000]              
	float                                              m_fClockTimeBase;                                 		// 0x005C (0x0004) [0x0000000000000000]              
	float                                              m_fClockTimeStarted;                              		// 0x0060 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0064 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1606 ];

		return pClassPointer;
	};

	void UpdateTimer ( );
	void UpdateMissionTimer ( );
	void Clear ( );
	void SetTime ( float fTimeBase, float fTimeStarted, unsigned long bCountingDown );
	void SetStyle ( unsigned long bUsingProgressBar, unsigned long bUsingAlarmImage );
	void UseAsMissionTimer ( unsigned long bIsMissionTimer );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_CombatAlert
// 0x0020 (0x005C - 0x003C)
class UTgUIPrimaryHUD_CombatAlert : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BasePanel;                                      		// 0x0040 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0044 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNeedsUpdate : 1;                               		// 0x0048 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUILabel*                                    m_HeaderLabel;                                    		// 0x004C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AlertNumberLabel;                               		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ActivateLabel;                                  		// 0x0054 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_CombatAlertSoundCue;                            		// 0x0058 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1607 ];

		return pClassPointer;
	};

	void UpdateLabels ( );
	void OnHexAssignmentUpdated ( );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_DangerBox
// 0x0088 (0x00C4 - 0x003C)
class UTgUIPrimaryHUD_DangerBox : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DangerBoxPanel;                                 		// 0x0040 (0x0004) [0x0000000000000000]              
	struct FAlertPanel                                 m_DangerBoxStruct[ 0x5 ];                         		// 0x0044 (0x0050) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0094 (0x0004) [0x0000000000000000]              
	class UAudioComponent*                             m_AudioComp;                                      		// 0x0098 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	unsigned long                                      m_bUseColorizedAlerts : 1;                        		// 0x009C (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_nDisplayedAlertNumbers[ 0x5 ];                  		// 0x00A0 (0x0014) [0x0000000000000000]              
	int                                                m_nAlertNumberCounter;                            		// 0x00B4 (0x0004) [0x0000000000000000]              
	TArray< struct FAlertInfo >                        m_AlertList;                                      		// 0x00B8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1608 ];

		return pClassPointer;
	};

	class ATgPawn* GetLocalPawn ( );
	void UnregisterForAlerts ( );
	void RegisterForAlerts ( );
	void InitAlertFade ( int nAlertIndex );
	void UpdateAlertFade ( );
	void UpdateDisplay ( );
	void RemoveTimedOutAlerts ( );
	void CancelAllAlerts ( );
	void RemoveAlert ( int nMsgId );
	void AddAlert ( unsigned char Priority, unsigned char Type, float fDuration, int nMsgId, struct FString sMessage );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_DeviceBar
// 0x0124 (0x0160 - 0x003C)
class UTgUIPrimaryHUD_DeviceBar : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	struct FsDeviceSlot                                m_DeviceSlots[ 0xB ];                             		// 0x0040 (0x0108) [0x0000000000000000]              
	int                                                m_nWhichSlotActive;                               		// 0x0148 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bWhichSlotActiveChanged : 1;                    		// 0x014C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bTriggerRefreshDeviceData : 1;                  		// 0x014C (0x0004) [0x0000000000000000] [0x00000002] 
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0150 (0x0004) [0x0000000000000000]              
	class UTgUIReticule*                               m_ReticuleLogic;                                  		// 0x0154 (0x0004) [0x0000000000000000]              
	int                                                m_nDefaultDeviceId;                               		// 0x0158 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_SelectDeviceSoundCue;                           		// 0x015C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1609 ];

		return pClassPointer;
	};

	int GetSlotByEquipPoint ( unsigned char eqpPoint );
	int GetSlotBySubType ( unsigned char subtype );
	void UpdateCooldown ( int nEquipPoint, float fStartTime, float fDuration );
	bool HasInhandDevice ( );
	bool SelectPreviousDevice ( );
	bool SelectNextDevice ( );
	bool SelectDeviceBarType ( unsigned char eType );
	bool SelectDeviceBarSlot ( int Slot );
	void ResetHUDCooldowns ( int DeviceEqpPt );
	void UpdateKeyBindings ( );
	void RefreshUICooldowns ( );
	bool GetActiveDeviceSlotString ( struct FString* DeviceString );
	void UpdateSelectedDevice ( class ATgDevice* Dev );
	void ClearAllDeviceBarSlotWidgets ( );
	void UserFlashOffhand ( int WhichSlot );
	void UserSelectedSlot ( int WhichSlot );
	void UpdateStatus ( );
	void PopulateDeviceBarSlots ( );
	class AReplicationInfo* FindSpawnedPet ( class ATgPawn* PlayerPawn, class ATgDevice* Dev );
	void TickTurretHealth ( );
	void TickVisuals ( float TimeNow );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene, class UTgUIReticule* HUDReticule );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene, class UTgUIReticule* HUDReticule );
};


// Class TgClient.TgUIPrimaryHUD_DialogQuery
// 0x0025 (0x0061 - 0x003C)
class UTgUIPrimaryHUD_DialogQuery : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bUseInstigator : 1;                             		// 0x0040 (0x0004) [0x0000000000000000] [0x00000002] 
	class UUILabel*                                    m_DialogText;                                     		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BackingImage;                                   		// 0x0048 (0x0004) [0x0000000000000000]              
	struct FString                                     m_sInstigator;                                    		// 0x004C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              m_fDialogTime;                                    		// 0x0058 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x005C (0x0004) [0x0000000000000000]              
	unsigned char                                      m_eDialogType;                                    		// 0x0060 (0x0001) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1610 ];

		return pClassPointer;
	};

	void eventSetDialogType ( unsigned char eType );
	void HideDialog ( );
	void ShowDialog ( struct FString sDialogText, float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void TickPrimaryHUDElement ( float TimeNow );
};


// Class TgClient.TgUIPrimaryHUD_KillDisplay
// 0x0014 (0x0050 - 0x003C)
class UTgUIPrimaryHUD_KillDisplay : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bKilledTargetDisplay : 1;                       		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUILabel*                                    m_KilledTargetLabel;                              		// 0x0044 (0x0004) [0x0000000000000000]              
	int                                                m_KilledTargetTime;                               		// 0x0048 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x004C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1611 ];

		return pClassPointer;
	};

	void AddNewDisplayMessage ( struct FString Message );
	void AddNewKilledTarget ( struct FString Target, struct FString KillerName, unsigned long KillerWasPlayer );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_MiniMap
// 0x0010 (0x004C - 0x003C)
class UTgUIPrimaryHUD_MiniMap : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UTgUILevelMap*                               m_LevelMap;                                       		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MapZoneName;                                    		// 0x0044 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0048 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1612 ];

		return pClassPointer;
	};

	void Show ( unsigned long Immediate );
	void Hide ( unsigned long Immediate );
	void InitPrimaryHUDElement ( );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_MissionInfo
// 0x0258 (0x0294 - 0x003C)
class UTgUIPrimaryHUD_MissionInfo : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIPanel*                                    m_ObjectivePanel;                                 		// 0x0044 (0x0004) [0x0000000000000000]              
	struct FObjectiveTeamStruct                        m_TeamElement;                                    		// 0x0048 (0x0010) [0x0000000000000000]              
	struct FObjectiveCTFRStruct                        m_CTFRElement;                                    		// 0x0058 (0x0014) [0x0000000000000000]              
	struct FObjectiveListStruct                        m_ObjectiveElement;                               		// 0x006C (0x00E0) [0x0000000000000000]              
	struct FObjectiveTicketStruct                      m_TicketElement;                                  		// 0x014C (0x0024) [0x0000000000000000]              
	struct FObjectiveBotPointsStruct                   m_BotPointsElement;                               		// 0x0170 (0x0034) [0x0000000000000000]              
	struct FObjectiveClockStruct                       m_ClockElement;                                   		// 0x01A4 (0x0008) [0x0000000000000000]              
	struct FHexStatusIndicatorStruct                   m_HexStatusElement;                               		// 0x01AC (0x0010) [0x0000000000000000]              
	struct FHexMultiStatusIndicatorHorzStuct           m_HexMultiStatusHorzElement;                      		// 0x01BC (0x007C) [0x0000000000000000]              
	struct FObjectiveVictoryBonusStruct                m_VictoryBonusElement;                            		// 0x0238 (0x0010) [0x0000000000000000]              
	int                                                m_nNumPrimaryObjectives;                          		// 0x0248 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_HexStatusMIC;                                   		// 0x024C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_HexMultiStatusHorzMICParent;                    		// 0x0250 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_HexMultiStatusHorzMIC[ 0x6 ];                   		// 0x0254 (0x0018) [0x0000000000000000]              
	class UMaterial*                                   m_ObjectiveBarParent;                             		// 0x026C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_VictoryDeathsBarMIC;                            		// 0x0270 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_BotHealthBarParent[ 0x2 ];                      		// 0x0274 (0x0008) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_TicketBarParent[ 0x2 ];                         		// 0x027C (0x0008) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0284 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BottomPanel;                                    		// 0x0288 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedRaidHexUpdateCount;                      		// 0x028C (0x0004) [0x0000000000000000]              
	int                                                m_nRobotIcon;                                     		// 0x0290 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1613 ];

		return pClassPointer;
	};

	void RepositionWidgets ( );
	void PopulateMissionInfo ( unsigned long bNoResetActive );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void InitPrimaryHUDElement ( );
};


// Class TgClient.TgUIPrimaryHUD_QuestTracking
// 0x0048 (0x0084 - 0x003C)
class UTgUIPrimaryHUD_QuestTracking : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIPanel*                                    m_QuestTrackerPanel;                              		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderLabel;                                    		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KeyPrompt;                                      		// 0x004C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestName;                                      		// 0x0050 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RequirementStrings[ 0x4 ];                      		// 0x0054 (0x0010) [0x0000000000000000]              
	class UUIPanel*                                    m_BottomDock;                                     		// 0x0064 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0068 (0x0004) [0x0000000000000000]              
	class ATgHUD_Game*                                 m_HUDParent;                                      		// 0x006C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BasePanel;                                      		// 0x0070 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedNameMsgId;                               		// 0x0074 (0x0004) [0x0000000000000000]              
	TArray< struct FQTrackIds >                        m_TrackIds;                                       		// 0x0078 (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1614 ];

		return pClassPointer;
	};

	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_RaidTimer
// 0x0054 (0x0090 - 0x003C)
class UTgUIPrimaryHUD_RaidTimer : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPrimary : 1;                                   		// 0x0040 (0x0004) [0x0000000000000000] [0x00000002] 
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0044 (0x0004) [0x0000000000000000]              
	struct FRaidTicketStruct                           m_RaidTicketBar;                                  		// 0x0048 (0x0018) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_TicketBarParent[ 0x2 ];                         		// 0x0060 (0x0008) [0x0000000000000000]              
	struct FRaidClockStruct                            m_RaidClock;                                      		// 0x0068 (0x0024) [0x0000000000000000]              
	int                                                m_nCachedRaidHexUpdateCount;                      		// 0x008C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1615 ];

		return pClassPointer;
	};

	void UpdateRaidTicketBar ( );
	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_Settings
// 0x0011 (0x004D - 0x003C)
class UTgUIPrimaryHUD_Settings : public UObject
{
public:
	unsigned long                                      m_bIgnore : 1;                                    		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bHidden : 1;                                    		// 0x003C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bHiddenConfig : 1;                              		// 0x003C (0x0004) [0x0000000000004000] [0x00000004] ( CPF_Config )
	unsigned long                                      m_bTickWhileHidden : 1;                           		// 0x003C (0x0004) [0x0000000000000000] [0x00000008] 
	TArray< struct FBasePanelStruct >                  m_BasePanels;                                     		// 0x0040 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned char                                      m_VisibilityConfigType;                           		// 0x004C (0x0001) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1616 ];

		return pClassPointer;
	};

	void Detach ( );
	void LoadConfigSettings ( );
	void SaveConfigSettings ( );
	bool ShouldTick ( );
	bool IsHidden ( );
	void CleanBasePanelList ( );
	void RemoveScene ( class UTgUISceneDriver* Scene );
	void RemoveBasePanel ( class UUIPanel* BasePanel );
	void AddBasePanel ( class UUIPanel* BasePanel, class UTgUISceneDriver* SceneDriver );
	void IgnoreSettings ( unsigned long bIgnore );
	void SetUIVisibility ( unsigned long Visible );
};


// Class TgClient.TgUIPrimaryHUD_SlotWidgets
// 0x0014 (0x0050 - 0x003C)
class UTgUIPrimaryHUD_SlotWidgets : public UObject
{
public:
	unsigned long                                      m_bSlotPopulated : 1;                             		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bSlotUnavailable : 1;                           		// 0x003C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bSlotTrainable : 1;                             		// 0x003C (0x0004) [0x0000000000000000] [0x00000004] 
	class UUIButton*                                   m_SlotButton;                                     		// 0x0040 (0x0004) [0x0000000000000000]              
	class UMaterial*                                   m_ItemMaterialParent;                             		// 0x0044 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_TexBlankIcon;                                   		// 0x0048 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_SlotMIC;                                        		// 0x004C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1617 ];

		return pClassPointer;
	};

	void Init ( class UUIButton* Button );
	void SetBlank ( );
	void SetUnavailable ( unsigned long bUnavailable );
	void Show ( struct FString IconTexture, int IconIndex );
	void Hide ( );
};


// Class TgClient.TgUIPrimaryHUD_TeamPanel
// 0x01F0 (0x022C - 0x003C)
class UTgUIPrimaryHUD_TeamPanel : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_SettingsPlayer;                                 		// 0x003C (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_Settings*                    m_SettingsTeam;                                   		// 0x0040 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0044 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bActive : 1;                                    		// 0x0048 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIPanel*                                    m_PlayerPanel;                                    		// 0x004C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TeamPanel;                                      		// 0x0050 (0x0004) [0x0000000000000000]              
	int                                                m_nTeamUpdateSeq;                                 		// 0x0054 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_XpProgPanel;                                    		// 0x0058 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_XpProgLabel;                                    		// 0x005C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_XpProgImage;                                    		// 0x0060 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_XpProgMIC;                                      		// 0x0064 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LevelLabel;                                     		// 0x0068 (0x0004) [0x0000000000000000]              
	struct FsTeamHealthStruct                          m_TeamHealthItem[ 0xC ];                          		// 0x006C (0x0150) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_PlayerHealthMICParent;                          		// 0x01BC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_TeamHealthMICParent;                            		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_PowerPoolImage;                                 		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_PowerPoolMIC;                                   		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PowerPoolLabel;                                 		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_MoraleBoostImage;                               		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MoraleBoostMIC;                                 		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MoraleBoostLabel;                               		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MoraleBoostAssaultMIC;                          		// 0x01DC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MoraleBoostMedicMIC;                            		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MoraleBoostReconMIC;                            		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MoraleBoostRoboticsMIC;                         		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_MoraleBoostDefaultMIC;                          		// 0x01EC (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_StatusDead;                                     		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_StatusOffline;                                  		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_SpeakerImage[ 0xC ];                            		// 0x01F8 (0x0030) [0x0000000000000000]              
	int                                                m_nLastXP;                                        		// 0x0228 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1618 ];

		return pClassPointer;
	};

	void TickPrimaryHUDElement ( float TimeNow );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
};


// Class TgClient.TgUIPrimaryHUD_VMTree
// 0x0038 (0x0074 - 0x003C)
class UTgUIPrimaryHUD_VMTree : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_PrimaryScene;                                   		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BasePanel;                                      		// 0x0044 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ActiveNodeLabel;                                		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NextNodeLabels[ 0x9 ];                          		// 0x004C (0x0024) [0x0000000000000000]              
	class UTgVoiceMacroTree*                           m_ActiveTree;                                     		// 0x0070 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1619 ];

		return pClassPointer;
	};

	bool HandleKeyInput ( struct FName KeyInput );
	void DeactivateTreeUI ( );
	void ActivateTreeAndUI ( class UTgVoiceMacroTree* tree );
	void PopulateTreeDisplay ( );
	void FixupWidgetsAsEmbedded ( class UUIScreenObject* BaseScreenObject, class UTgUISceneDriver* OwnerScene );
	void FixupWidgets ( class UUIPanel* BasePanel, class UTgUISceneDriver* OwnerScene );
	void TickPrimaryHUDElement ( float TimeNow );
};


// Class TgClient.TgUIReticule
// 0x0088 (0x00C4 - 0x003C)
class UTgUIReticule : public UObject
{
public:
	class UTgUIPrimaryHUD_Settings*                    m_Settings;                                       		// 0x003C (0x0004) [0x0000000000000000]              
	struct FLinearColor                                m_LocalPlayerHealth;                              		// 0x0040 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                m_LocalPlayerEnergy;                              		// 0x0050 (0x0010) [0x0000000000000000]              
	float                                              m_InCombatFlag;                                   		// 0x0060 (0x0004) [0x0000000000000000]              
	struct FLinearColor                                m_EquippingData;                                  		// 0x0064 (0x0010) [0x0000000000000000]              
	struct FLinearColor                                m_ProgressBarData;                                		// 0x0074 (0x0010) [0x0000000000000000]              
	unsigned long                                      m_bIsOnTarget : 1;                                		// 0x0084 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsTargetInRange : 1;                           		// 0x0084 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bIsTargetFriendly : 1;                          		// 0x0084 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bDoingCooldownFade : 1;                         		// 0x0084 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bCanInHandDeviceFireNow : 1;                    		// 0x0084 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_bInited : 1;                                    		// 0x0084 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bForceHide : 1;                                 		// 0x0084 (0x0004) [0x0000000000000000] [0x00000040] 
	unsigned long                                      m_bMinRetInitialized : 1;                         		// 0x0084 (0x0004) [0x0000000000000000] [0x00000080] 
	float                                              m_fCooldownFadeValue;                             		// 0x0088 (0x0004) [0x0000000000000000]              
	struct FColor                                      m_DefaultColor;                                   		// 0x008C (0x0004) [0x0000000000000000]              
	struct FColor                                      m_TargetInRangeColor;                             		// 0x0090 (0x0004) [0x0000000000000000]              
	struct FColor                                      m_TargetOutOfRangeColor;                          		// 0x0094 (0x0004) [0x0000000000000000]              
	struct FColor                                      m_TargetFriendlyColor;                            		// 0x0098 (0x0004) [0x0000000000000000]              
	struct FColor                                      m_InHandDeviceCantFireColor;                      		// 0x009C (0x0004) [0x0000000000000000]              
	float                                              m_LastKnownWorldTime;                             		// 0x00A0 (0x0004) [0x0000000000000000]              
	class ATgHUD_Game*                                 m_HUDParent;                                      		// 0x00A4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_CrosshairMatInstance;                           		// 0x00A8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_DefaultCrosshairMatInstance;                    		// 0x00AC (0x0004) [0x0000000000000000]              
	struct FName                                       m_ActiveCrosshairMaterialParentName;              		// 0x00B0 (0x0008) [0x0000000000000000]              
	class UMaterialInterface*                          m_ActiveCrosshairMaterialParent;                  		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_WeaponStatusMatInstance[ 0x2 ];                 		// 0x00BC (0x0008) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1620 ];

		return pClassPointer;
	};

};


// Class TgClient.TgUISceneDriver
// 0x0070 (0x00AC - 0x003C)
class UTgUISceneDriver : public UObject
{
public:
	unsigned long                                      m_bInitialized : 1;                               		// 0x003C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bLoaded : 1;                                    		// 0x003C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bEscapeKeyCloses : 1;                           		// 0x003C (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bCloseChildrenOnClose : 1;                      		// 0x003C (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bManagedScene : 1;                              		// 0x003C (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_bForcedOpen : 1;                                		// 0x003C (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bHookChat : 1;                                  		// 0x003C (0x0004) [0x0000000000000000] [0x00000040] 
	unsigned long                                      m_bIgnoreLoadCheck : 1;                           		// 0x003C (0x0004) [0x0000000000000000] [0x00000080] 
	unsigned long                                      m_bDataRequestPending : 1;                        		// 0x003C (0x0004) [0x0000000000000000] [0x00000100] 
	unsigned long                                      m_bEndOfDataset : 1;                              		// 0x003C (0x0004) [0x0000000000000000] [0x00000200] 
	class ATgHUD*                                      m_HUDParent;                                      		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUIInteraction*                              m_UIInteraction;                                  		// 0x0044 (0x0004) [0x0000000000000000]              
	class ULocalPlayer*                                m_OwnerPlayer;                                    		// 0x0048 (0x0004) [0x0000000000000000]              
	class UUIScene*                                    m_UIScene;                                        		// 0x004C (0x0004) [0x0000000000000000]              
	class UTgUISceneManager*                           m_Manager;                                        		// 0x0050 (0x0004) [0x0000000000000000]              
	struct FString                                     m_UISceneTemplatePath;                            		// 0x0054 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              m_TimeOfTick;                                     		// 0x0060 (0x0004) [0x0000000000000000]              
	class UUIScene*                                    m_SceneTemplate;                                  		// 0x0064 (0x0004) [0x0000000000000000]              
	class UTgUIDataStore_StringList*                   m_TgStringListDataStore;                          		// 0x0068 (0x0004) [0x0000000000002000]              ( CPF_Transient )
	struct FString                                     m_KeyCommand;                                     		// 0x006C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FPointer                                    m_pTgCallbackDevice;                              		// 0x0078 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                m_nPageNum;                                       		// 0x007C (0x0004) [0x0000000000000000]              
	int                                                m_nPageSize;                                      		// 0x0080 (0x0004) [0x0000000000000000]              
	class UMaterial*                                   m_ItemMaterialParent;                             		// 0x0084 (0x0004) [0x0000000000000000]              
	class UTgUISceneDriver*                            m_ParentScene;                                    		// 0x0088 (0x0004) [0x0000000000000000]              
	TArray< class UUIScreenObject* >                   m_FocusableEntities;                              		// 0x008C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_ActiveFocusIdx;                                 		// 0x0098 (0x0004) [0x0000000000000000]              
	TArray< struct FsTgUIRotator >                     m_ModelRotators;                                  		// 0x009C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nActiveModelRotator;                            		// 0x00A8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1621 ];

		return pClassPointer;
	};

	void eventUIListCallbackWrapper ( class UUIList* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnNotifyEditBoxStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	class UFont* eventGetFontFromUCByIndex ( int FontSize );
	void eventSetDefaultSceneDriverSceneDelegates ( );
	bool DefaultSceneDriverSceneDelegate ( struct FInputEventParameters* EventParms );
	void eventTickTgUISceneUC ( );
	void PostRender ( );
	void CheckSceneForUIModelDragRelease ( struct FInputEventParameters* EventParms );
	bool HandleTgUIModelFixedRotation ( struct FInputEventParameters* EventParms );
	bool HandleTgUIModelDragRotation ( struct FInputEventParameters* EventParms );
	bool SetFocusedWidget ( class UUIScreenObject* NewFocus );
	void SetNextWidgetFocused ( );
	void SetPreviousWidgetFocused ( );
	bool OnNextButton ( struct FInputEventParameters* EventParms );
	bool OnPrevButton ( struct FInputEventParameters* EventParms );
	bool OnFirstButton ( struct FInputEventParameters* EventParms );
	bool OnLastButton ( struct FInputEventParameters* EventParms );
	void OpenTabScene ( int nFuncId );
	bool DefaultTgShouldModulateDelegate ( float* AlphaModulationPercent );
	void SetParentUIScene ( class UTgUISceneDriver* Scene );
	void TriggerSceneFadeIn ( );
	struct FString CreateDataStoreBindingString ( class UUIDataStore* DataStore, struct FName SectionName );
	struct FName GetCurrentKeyBinding ( int* bAltModified );
	void DefaultSceneDriverCloseDelegate ( class UUIScene* DeactivatedScene );
	void SetVisible ( class UUIObject* pObject, unsigned long bVisible );
	void UnregisterCallbacks ( );
	void RegisterCallbacks ( );
	void Repaint ( );
	void SetButtonsEnabled ( unsigned long bEnabled );
	void HideChildControlsByTag ( class UUIScreenObject* Parent, struct FString sCompareTag, int nCompareTagLen );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void CloseTgUIScene ( );
	void OpenTgUIScene ( );
	class ATgPawn_Character* GetLocalTgPawn ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUICameraSettingsMenu
// 0x0020 (0x00CC - 0x00AC)
class UTgUICameraSettingsMenu : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_BackButton;                                     		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplyButton;                                    		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_CameraMode0Checkbox;                            		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_CameraMode1Checkbox;                            		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_CameraMode2Checkbox;                            		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_CameraMode3Checkbox;                            		// 0x00C0 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentCameraMode;                              		// 0x00C4 (0x0004) [0x0000000000000000]              
	int                                                m_PendingCameraMode;                              		// 0x00C8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1622 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void OnFakeRadioButtonChanged ( class UUIObject* Sender, int PlayerIndex );
	bool OnApplyButtonKey ( struct FInputEventParameters* EventParms );
	bool OnBackButtonKey ( struct FInputEventParameters* EventParms );
	void ApplyCameraSettings ( );
	void PopulateWidgets ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIChallengePopup
// 0x0024 (0x00D0 - 0x00AC)
class UTgUIChallengePopup : public UTgUISceneDriver
{
public:
	class UUILabel*                                    m_lblMapName;                                     		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_lblReady;                                       		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_lblTeamSize;                                    		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_lblRemainging;                                  		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_btnAccept;                                      		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_btnDecline;                                     		// 0x00C0 (0x0004) [0x0000000000000000]              
	int                                                m_nLastUpdate;                                    		// 0x00C4 (0x0004) [0x0000000000000000]              
	int                                                m_nExpiresAt;                                     		// 0x00C8 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_OpenSoundCue;                                   		// 0x00CC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1623 ];

		return pClassPointer;
	};

	bool OnDeclineButton ( struct FInputEventParameters* EventParms );
	bool OnAcceptButton ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIChat
// 0x0188 (0x0234 - 0x00AC)
class UTgUIChat : public UTgUISceneDriver
{
public:
	unsigned char                                      m_eChatState;                                     		// 0x00AC (0x0001) [0x0000000000000000]              
	unsigned char                                      m_byNumActiveTabs;                                		// 0x00AD (0x0001) [0x0000000000000000]              
	struct FName                                       m_nmSlashOpening;                                 		// 0x00B0 (0x0008) [0x0000000000000000]              
	struct FName                                       m_nmReplyOpening;                                 		// 0x00B8 (0x0008) [0x0000000000000000]              
	struct FName                                       m_nmDefaultOpening;                               		// 0x00C0 (0x0008) [0x0000000000000000]              
	struct FName                                       m_nmPageUp;                                       		// 0x00C8 (0x0008) [0x0000000000000000]              
	struct FName                                       m_nmPageDown;                                     		// 0x00D0 (0x0008) [0x0000000000000000]              
	int                                                m_nChatIndex;                                     		// 0x00D8 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bForceChatInFront : 1;                          		// 0x00DC (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bForceCombatLog : 1;                            		// 0x00DC (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bHideChatWhenIdle : 1;                          		// 0x00DC (0x0004) [0x0000000000044000] [0x00000004] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bFilterProfanity : 1;                           		// 0x00DC (0x0004) [0x0000000000044000] [0x00000008] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bShowAgencyMemberLogins : 1;                    		// 0x00DC (0x0004) [0x0000000000044000] [0x00000010] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bShowFriendLogins : 1;                          		// 0x00DC (0x0004) [0x0000000000044000] [0x00000020] ( CPF_Config | CPF_GlobalConfig )
	class UUILabel*                                    m_SendToLine;                                     		// 0x00E0 (0x0004) [0x0000000000000000]              
	struct FsOuterPanels                               m_OuterPanels;                                    		// 0x00E4 (0x0010) [0x0000000000000000]              
	class UUIImage*                                    m_ChatOpaqueBG;                                   		// 0x00F4 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ChatBG;                                         		// 0x00F8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_ChatEntryBox;                                   		// 0x00FC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ChannelCombo;                                   		// 0x0100 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LockedModeHelpLabel;                            		// 0x0104 (0x0004) [0x0000000000000000]              
	float                                              m_fIdleOpacity;                                   		// 0x0108 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nSelectedChannelGroupDisplay;                   		// 0x010C (0x0004) [0x0000000000000000]              
	struct FsChatChannelGroupLogic                     m_ChannelGroupLogic[ 0x5 ];                       		// 0x0110 (0x0078) [0x0000000000444000]              ( CPF_Config | CPF_GlobalConfig | CPF_NeedCtorLink )
	struct FsChatChannelGroupDisplay                   m_ChannelGroupDisplay[ 0x5 ];                     		// 0x0188 (0x003C) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	struct FsChatScrollButtons                         m_ScrollButtons;                                  		// 0x01C4 (0x0014) [0x0000000000000000]              
	class UUILabel*                                    m_ChatDisplay[ 0x9 ];                             		// 0x01D8 (0x0024) [0x0000000000000000]              
	class UUILabel*                                    m_TestSizingLabel;                                		// 0x01FC (0x0004) [0x0000000000000000]              
	struct FName                                       m_nmChannelDataStore;                             		// 0x0200 (0x0008) [0x0000000000000000]              
	TArray< unsigned char >                            m_ChatChannelList;                                		// 0x0208 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_CachedTellRecipient;                            		// 0x0214 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_CachedLastSender;                               		// 0x0220 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              m_fNextScrollPulse;                               		// 0x022C (0x0004) [0x0000000000000000]              
	int                                                m_nScrolling;                                     		// 0x0230 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1624 ];

		return pClassPointer;
	};

	void Tick ( float DeltaTime );
	bool OnChatScrollButton ( struct FInputEventParameters* EventParms );
	bool OnChatEntrySubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnChatTabClicked ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void ChatEntryLineChanged ( class UUIObject* Sender, int PlayerIndex );
	void UserSelectedChannel ( class UUIObject* Sender, int PlayerIndex );
	void FixupTgUIScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIClassSelectScene
// 0x0088 (0x0134 - 0x00AC)
class UTgUIClassSelectScene : public UTgUISceneDriver
{
public:
	float                                              m_LastSelectTime;                                 		// 0x00AC (0x0004) [0x0000000000000000]              
	struct FsClassSelectStruct                         m_ClassSelect[ 0x4 ];                             		// 0x00B0 (0x0060) [0x0000000000000000]              
	int                                                m_HighlightClass;                                 		// 0x0110 (0x0004) [0x0000000000000000]              
	int                                                m_HighlightGender;                                		// 0x0114 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedClass;                                  		// 0x0118 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedGender;                                 		// 0x011C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BackButton;                                     		// 0x0120 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NextButton;                                     		// 0x0124 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DescriptionPanel;                               		// 0x0128 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ClassGenderLabel;                               		// 0x012C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_DescriptionLabel;                               		// 0x0130 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1625 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void OnClassMouseOverDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnNextButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnBackButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectClassGenderDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void HighlightClassGender ( int ClassIndex, int genderIndex );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIConfirmPopup
// 0x001C (0x00C8 - 0x00AC)
class UTgUIConfirmPopup : public UTgUISceneDriver
{
public:
	class UUIPanel*                                    m_PopupPanel;                                     		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ConfirmText;                                    		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmYes;                                     		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmNo;                                      		// 0x00B8 (0x0004) [0x0000000000000000]              
	TArray< struct FsConfirmPopup >                    m_PopupList;                                      		// 0x00BC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1626 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void SetConfirmTextAndCallback ( struct FString Message, class UFunction* callbackPtr );
	void HandleConfirmButtonCallback ( unsigned long accepted );
	void RemoveConfirmPopupByCallback ( class UFunction* callbackPtr, unsigned long bTellGameClient );
	void SetConfirmDialogText ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	bool OnConfirmNo ( struct FInputEventParameters* EventParms );
	bool OnConfirmYes ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUICreateCharA
// 0x0048 (0x00F4 - 0x00AC)
class UTgUICreateCharA : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_BackButton;                                     		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NextButton;                                     		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NameLabel;                                      		// 0x00B4 (0x0004) [0x0000000000000000]              
	struct FsGenderButtonStruct                        m_MaleButton;                                     		// 0x00B8 (0x000C) [0x0000000000000000]              
	struct FsGenderButtonStruct                        m_FemaleButton;                                   		// 0x00C4 (0x000C) [0x0000000000000000]              
	class UTgUIScrollList*                             m_HeadScroll;                                     		// 0x00D0 (0x0004) [0x0000000000000000]              
	TArray< struct FsPresetHeadArchetype >             m_PresetHeadList;                                 		// 0x00D4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nCurrentGender;                                 		// 0x00E0 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrentProfile;                                		// 0x00E4 (0x0004) [0x0000000000000000]              
	int                                                m_nHeadSelection;                                 		// 0x00E8 (0x0004) [0x0000000000000000]              
	int                                                m_nHairSelection;                                 		// 0x00EC (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActor_CharacterBuilder*       m_CustomModel;                                    		// 0x00F0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1627 ];

		return pClassPointer;
	};

	void eventSendSections ( );
	void eventSetSelectHeadDelegate ( class UUIScreenObject* Obj );
	void eventFixupWidgetsUC ( );
	bool OnGenderButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectHeadDelegate ( struct FInputEventParameters* EventParms );
	bool OnNextButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnBackButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void SaveMorphSettings ( );
	void SelectHead ( int headIndex );
	void BuildHeadList ( );
	void SetupCustomModel ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
};


// Class TgClient.TgUICreatePlayerScene
// 0x000C (0x00B8 - 0x00AC)
class UTgUICreatePlayerScene : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_AcceptButton;                                   		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_EntryFeedback;                                  		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_EntryField;                                     		// 0x00B4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1628 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnAcceptInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUICreditsMenu
// 0x0020 (0x00CC - 0x00AC)
class UTgUICreditsMenu : public UTgUISceneDriver
{
public:
	class UUIImage*                                    m_GAImage;                                        		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_GAImageGuide;                                   		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIScrollFrame*                              m_ScrollFrame;                                    		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_HZImage;                                        		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditsLabel;                                   		// 0x00BC (0x0004) [0x0000000000000000]              
	float                                              m_TimeAfterScroll;                                		// 0x00C0 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bScrolling : 1;                                 		// 0x00C4 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bCloseOnInput : 1;                              		// 0x00C4 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bShowLogo : 1;                                  		// 0x00C4 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bContentReady : 1;                              		// 0x00C4 (0x0004) [0x0000000000000000] [0x00000008] 
	float                                              m_fLastTickTime;                                  		// 0x00C8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1629 ];

		return pClassPointer;
	};

	bool OnSceneInput ( struct FInputEventParameters* EventParms );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIDevInGame
// 0x0014 (0x00C0 - 0x00AC)
class UTgUIDevInGame : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SpawnBotButton;                                 		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EquipScreenButton;                              		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MissionScreenButton;                            		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LaunchScreenButton;                             		// 0x00BC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1630 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnSpawnBotButtonKey ( struct FInputEventParameters* EventParms );
	bool OnLaunchScreenButtonKey ( struct FInputEventParameters* EventParms );
	bool OnMissionScreenButtonKey ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonKey ( struct FInputEventParameters* EventParms );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIDevSpawnBot
// 0x007C (0x0128 - 0x00AC)
class UTgUIDevSpawnBot : public UTgUISceneDriver
{
public:
	class UUILabelButton*                              m_BotListWidgets[ 0x12 ];                         		// 0x00AC (0x0048) [0x0000000000000000]              
	class UUIButton*                                   m_SpawnBotButton;                                 		// 0x00F4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x00F8 (0x0004) [0x0000000000000000]              
	struct FsBotScrollButton                           m_ScrollSet;                                      		// 0x00FC (0x001C) [0x0000000000000000]              
	class UTgDataInterface*                            m_DataInterface;                                  		// 0x0118 (0x0004) [0x0000000000000000]              
	class UTgDataSet*                                  m_DataSet;                                        		// 0x011C (0x0004) [0x0000000000000000]              
	int                                                m_SlotSelected;                                   		// 0x0120 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNeedsRefresh : 1;                              		// 0x0124 (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1631 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnBotList18 ( struct FInputEventParameters* EventParms );
	bool OnBotList17 ( struct FInputEventParameters* EventParms );
	bool OnBotList16 ( struct FInputEventParameters* EventParms );
	bool OnBotList15 ( struct FInputEventParameters* EventParms );
	bool OnBotList14 ( struct FInputEventParameters* EventParms );
	bool OnBotList13 ( struct FInputEventParameters* EventParms );
	bool OnBotList12 ( struct FInputEventParameters* EventParms );
	bool OnBotList11 ( struct FInputEventParameters* EventParms );
	bool OnBotList10 ( struct FInputEventParameters* EventParms );
	bool OnBotList9 ( struct FInputEventParameters* EventParms );
	bool OnBotList8 ( struct FInputEventParameters* EventParms );
	bool OnBotList7 ( struct FInputEventParameters* EventParms );
	bool OnBotList6 ( struct FInputEventParameters* EventParms );
	bool OnBotList5 ( struct FInputEventParameters* EventParms );
	bool OnBotList4 ( struct FInputEventParameters* EventParms );
	bool OnBotList3 ( struct FInputEventParameters* EventParms );
	bool OnBotList2 ( struct FInputEventParameters* EventParms );
	bool OnBotList1 ( struct FInputEventParameters* EventParms );
	bool OnBotListBase ( struct FName Key, unsigned char Event, int Index );
	bool OnSpawnBotButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnSpawnBotSceneKey ( struct FInputEventParameters* EventParms );
	bool OnScrollLowerButton ( struct FInputEventParameters* EventParms );
	bool OnScrollHigherButton ( struct FInputEventParameters* EventParms );
	void StopScrolling ( );
	void StartScrolling ( unsigned long bHigher );
	void ScrollChange ( unsigned long bHigher );
	void eventSetupDataInterface ( );
	void UpdateScrolling ( );
	bool AttemptSpawn ( );
	void PopulateWidgets ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIEndRaidScreen
// 0x0044 (0x00F0 - 0x00AC)
class UTgUIEndRaidScreen : public UTgUISceneDriver
{
public:
	class UUISafeRegionPanel*                          m_AspectPanel;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ResultLabel;                                    		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidStats[ 0x3 ];                               		// 0x00B4 (0x000C) [0x0000000000000000]              
	class UUILabel*                                    m_RaiderStats[ 0x4 ];                             		// 0x00C0 (0x0010) [0x0000000000000000]              
	class UUILabel*                                    m_DefenderStats[ 0x4 ];                           		// 0x00D0 (0x0010) [0x0000000000000000]              
	int                                                m_eRaidPhase;                                     		// 0x00E0 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedRaidHexUpdateCount;                      		// 0x00E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x00E8 (0x0004) [0x0000000000000000]              
	class UTgUIHexWidget_Raid*                         m_pHexWidget;                                     		// 0x00EC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1632 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void ShowScene ( );
	void HideScene ( );
	bool OnCloseButtonPressed ( struct FInputEventParameters* EventParms );
	void SetLabelTextAndColor ( unsigned long bAttackersWon );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIEULAMenu
// 0x0018 (0x00C4 - 0x00AC)
class UTgUIEULAMenu : public UTgUISceneDriver
{
public:
	unsigned long                                      m_bHasViewed : 1;                                 		// 0x00AC (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bEULAMissing : 1;                               		// 0x00AC (0x0004) [0x0000000000000000] [0x00000002] 
	class UUILabelButton*                              m_EULAAcceptButton;                               		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_EULADeclineButton;                              		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_EULALabel;                                      		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIScrollFrame*                              m_ScrollFrame;                                    		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LinkToWebEULA;                                  		// 0x00C0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1633 ];

		return pClassPointer;
	};

	bool OnScroll ( class UUIScreenObject* Sender, float Nudges, unsigned long bMaxedOut );
	bool OnSceneInput ( struct FInputEventParameters* EventParms );
	bool OnButtonClick ( struct FInputEventParameters* EventParms );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIFriendsList
// 0x0080 (0x012C - 0x00AC)
class UTgUIFriendsList : public UTgUISceneDriver
{
public:
	unsigned char                                      m_eState;                                         		// 0x00AC (0x0001) [0x0000000000000000]              
	unsigned char                                      m_ePopupState;                                    		// 0x00AD (0x0001) [0x0000000000000000]              
	struct FsSelectedFriend                            m_SelectedPlayer;                                 		// 0x00B0 (0x0014) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FsFriendButtons                             m_FriendsButtons;                                 		// 0x00C4 (0x001C) [0x0000000000000000]              
	struct FsIgnoreButtons                             m_IgnoreButtons;                                  		// 0x00E0 (0x0014) [0x0000000000000000]              
	struct FsFriendNotes                               m_Notes;                                          		// 0x00F4 (0x0008) [0x0000000000000000]              
	struct FsFriendPopup                               m_FriendPopup;                                    		// 0x00FC (0x0018) [0x0000000000000000]              
	class UUICheckbox*                                 m_HideOfflineCheckbox;                            		// 0x0114 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0118 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_ScrollList;                                     		// 0x011C (0x0004) [0x0000000000000000]              
	TArray< struct FsFriendElement >                   m_FriendsDisplayList;                             		// 0x0120 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1634 ];

		return pClassPointer;
	};

	void RegisterCallbacks ( );
	bool OnPopupSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnPopupButtons ( struct FInputEventParameters* EventParms );
	bool OnSceneButtons ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKey ( struct FInputEventParameters* EventParms );
	bool OnIgnoreButtons ( struct FInputEventParameters* EventParms );
	bool OnListItemSelected ( struct FInputEventParameters* EventParms );
	bool OnFriendButtons ( struct FInputEventParameters* EventParms );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIHexAuction
// 0x005C (0x0108 - 0x00AC)
class UTgUIHexAuction : public UTgUISceneDriver
{
public:
	class UUIPanel*                                    m_PopupPanel;                                     		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HeaderPanel;                                    		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderLabel;                                    		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CoverButton;                                    		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MissilePanel;                                   		// 0x00BC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_SlotWidgets*                 m_MissileSlot;                                    		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MissileSelectButton;                            		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MissionTypePanel;                               		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_MissionCaptureCheckbox;                         		// 0x00CC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_MissionStealCheckbox;                           		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BiddingPanel;                                   		// 0x00D4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CurrentBidLabel;                                		// 0x00D8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RemainingTimeLabel;                             		// 0x00DC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_BidEntryBox;                                    		// 0x00E0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PlaceBidButton;                                 		// 0x00E4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AvailableCreditsLabel;                          		// 0x00E8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_InfoPanel;                                      		// 0x00EC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MessageLabel;                                   		// 0x00F0 (0x0004) [0x0000000000000000]              
	int                                                m_nHexPosition;                                   		// 0x00F4 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bTacticalStrikeMode : 1;                        		// 0x00F8 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_nMissileInvId;                                  		// 0x00FC (0x0004) [0x0000000000000000]              
	int                                                m_nAttackType;                                    		// 0x0100 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pHexAuctionData;                                		// 0x0104 (0x0004) [0x0000000000001000]              ( CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1635 ];

		return pClassPointer;
	};

	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void SetHexPosition ( int nId );
	void eventFixupWidgetsUC ( );
	void OnMissionTypeChanged ( class UUIObject* Sender, int PlayerIndex );
	bool OnSelectMissile ( struct FInputEventParameters* EventParms );
	bool OnPlaceBidClicked ( struct FInputEventParameters* EventParms );
	void UpdateButtonCovers ( );
	void RequestHexAuctionData ( );
	void PostOpenScene ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIInGameMainMenu
// 0x002C (0x00D8 - 0x00AC)
class UTgUIInGameMainMenu : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_EndMissionButton;                               		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SettingsButton;                                 		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ReturnToGameButton;                             		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ExitGameButton;                                 		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DisconnectButton;                               		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_NoMissionBG;                                    		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_MissionBG;                                      		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SpawnBotButton;                                 		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EquipScreenButton;                              		// 0x00CC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MissionScreenButton;                            		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LaunchScreenButton;                             		// 0x00D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1636 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnDisconnectButtonKey ( struct FInputEventParameters* EventParms );
	bool OnSpawnBotButtonKey ( struct FInputEventParameters* EventParms );
	bool OnLaunchScreenButtonKey ( struct FInputEventParameters* EventParms );
	bool OnSettingsButtonKey ( struct FInputEventParameters* EventParms );
	bool OnExitGameButtonKey ( struct FInputEventParameters* EventParms );
	bool OnReturnToGameButtonKey ( struct FInputEventParameters* EventParms );
	bool OnEndMissionButton ( struct FInputEventParameters* EventParms );
	void ProcessQuit ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIInstanceSelect
// 0x0028 (0x00D4 - 0x00AC)
class UTgUIInstanceSelect : public UTgUISceneDriver
{
public:
	class UTgUIScrollList*                             m_InstanceScrollFrame;                            		// 0x00AC (0x0004) [0x0000000000000000]              
	TArray< struct FsInstanceStruct >                  m_InstanceList;                                   		// 0x00B0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nMapGameId;                                     		// 0x00BC (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedItem;                                  		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ChangeButton;                                   		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UTgDataInterface*                            m_InstanceDataInterface;                          		// 0x00CC (0x0004) [0x0000000000000000]              
	class UTgDataSet*                                  m_InstanceDataSet;                                		// 0x00D0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1637 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetSelectInstanceDelegate ( class UUIScreenObject* Obj );
	void GetInstanceListCallback ( int eType );
	bool OnSelectInstanceDelegate ( struct FInputEventParameters* EventParms );
	bool OnChangeButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void SelectInstanceItem ( int ItemIndex );
	void ResizeInstanceWidgets ( int newSize );
	void PopulateInstanceData ( );
	void FixupTgUIScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIIntroScene
// 0x006C (0x0118 - 0x00AC)
class UTgUIIntroScene : public UTgUISceneDriver
{
public:
	class UUILabel*                                    m_IntroLabel[ 0x3 ];                              		// 0x00AC (0x000C) [0x0000000000000000]              
	class UUILabel*                                    m_IntroLabelOffset1[ 0x3 ];                       		// 0x00B8 (0x000C) [0x0000000000000000]              
	class UUILabel*                                    m_IntroLabelOffset2[ 0x3 ];                       		// 0x00C4 (0x000C) [0x0000000000000000]              
	struct FString                                     m_IntroString[ 0x3 ];                             		// 0x00D0 (0x0024) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              m_LabelOpacity[ 0x3 ];                            		// 0x00F4 (0x000C) [0x0000000000000000]              
	int                                                m_CurrentLabel;                                   		// 0x0100 (0x0004) [0x0000000000000000]              
	float                                              m_fIntroTimer;                                    		// 0x0104 (0x0004) [0x0000000000000000]              
	float                                              m_fLabelTimer;                                    		// 0x0108 (0x0004) [0x0000000000000000]              
	float                                              m_fFadeTimer;                                     		// 0x010C (0x0004) [0x0000000000000000]              
	float                                              m_fFadeTime;                                      		// 0x0110 (0x0004) [0x0000000000000000]              
	float                                              m_fTotalTime;                                     		// 0x0114 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1638 ];

		return pClassPointer;
	};

	void SetIntroParams ( float FadeTime, float TotalTime );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUILFGPopup
// 0x0174 (0x0220 - 0x00AC)
class UTgUILFGPopup : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
	struct FsLFGPlayerStruct                           m_LFGPlayer[ 0x6 ];                               		// 0x00B0 (0x0090) [0x0000000000000000]              
	struct FsLFGDataStruct                             m_LFGData[ 0x6 ];                                 		// 0x0140 (0x00C0) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_TeamListPanel;                                  		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AlertLabel;                                     		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TimerLabel;                                     		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AcceptButton;                                   		// 0x020C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelButton;                                   		// 0x0210 (0x0004) [0x0000000000000000]              
	float                                              m_AlertTimer;                                     		// 0x0214 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_PlayerResponded : 1;                            		// 0x0218 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_ResetMode : 1;                                  		// 0x0218 (0x0004) [0x0000000000000000] [0x00000002] 
	class USoundCue*                                   m_OpenSoundCue;                                   		// 0x021C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1639 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void UpdateLFGWidgets ( );
	bool OnCancelButton ( struct FInputEventParameters* EventParms );
	bool OnAcceptButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnLFGPopupKey ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUILFGCountdownPopup
// 0x0000 (0x0220 - 0x0220)
class UTgUILFGCountdownPopup : public UTgUILFGPopup
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1640 ];

		return pClassPointer;
	};

};


// Class TgClient.TgUILFGSpoilerPopup
// 0x0000 (0x0220 - 0x0220)
class UTgUILFGSpoilerPopup : public UTgUILFGPopup
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1641 ];

		return pClassPointer;
	};

};


// Class TgClient.TgUILoadItemProfile
// 0x000C (0x00B8 - 0x00AC)
class UTgUILoadItemProfile : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_LoadDefButton;                                  		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LoadOffButton;                                  		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LoadHybButton;                                  		// 0x00B4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1642 ];

		return pClassPointer;
	};

	bool OnDialogButtonKey ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIMapTransition
// 0x0054 (0x0100 - 0x00AC)
class UTgUIMapTransition : public UTgUISceneDriver
{
public:
	int                                                m_MapLoadStartTime;                               		// 0x00AC (0x0004) [0x0000000000000000]              
	unsigned char                                      m_MapTransitionState;                             		// 0x00B0 (0x0001) [0x0000000000000000]              
	unsigned long                                      m_bMinimalTransition : 1;                         		// 0x00B4 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bNoFadeIn : 1;                                  		// 0x00B4 (0x0004) [0x0000000000000000] [0x00000002] 
	class UUILabel*                                    m_MapFileName;                                    		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MapName;                                        		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Briefing;                                       		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BriefingPanel;                                  		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Loading;                                        		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LoadingTip;                                     		// 0x00CC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_Close;                                          		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_Background;                                     		// 0x00D4 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_MapRoleIcon;                                    		// 0x00D8 (0x0004) [0x0000000000000000]              
	struct FName                                       m_nmArriveMovie;                                  		// 0x00DC (0x0008) [0x0000000000000000]              
	struct FName                                       m_nmDepartMovie;                                  		// 0x00E4 (0x0008) [0x0000000000000000]              
	int                                                m_bIsInNewWorld;                                  		// 0x00EC (0x0004) [0x0000000000000000]              
	int                                                m_MapWaitStartTime;                               		// 0x00F0 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_DefenderIcon;                                   		// 0x00F4 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_AttackerIcon;                                   		// 0x00F8 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_DefaultLoadScreen;                              		// 0x00FC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1643 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool CanShowCloseButton ( );
	void PleaseShowCloseButton ( unsigned long bEnablePlease );
	bool OnCloseButtonClick ( struct FInputEventParameters* EventParms );
	void SetBackgroundImage ( struct FString fsImage );
	void ResetMapTransition ( );
	bool TickTgUIScene ( );
	void RegisterCallbacks ( );
	void PreCloseScene ( );
	void OpenTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUINavScene
// 0x001C (0x00C8 - 0x00AC)
class UTgUINavScene : public UTgUISceneDriver
{
public:
	class UTgUIScrollList*                             m_ScrollList;                                     		// 0x00AC (0x0004) [0x0000000000000000]              
	TArray< struct FsNavDisplay >                      m_ScrollListMembers;                              		// 0x00B0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class ATgPointOfInterest*                          m_SelectedPOI;                                    		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ShowPathButton;                                 		// 0x00C4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1644 ];

		return pClassPointer;
	};

	bool OnShowPath ( struct FInputEventParameters* EventParms );
	bool OnSelect ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventSetRawInputKeyForList ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	void ShowPathToSelected ( );
	void UpdateHighlight ( );
	bool Select ( int nIndex );
	bool SelectByButton ( class UUIButton* Button );
	void BuildPOIList ( );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUINewSettingsMenu
// 0x0000 (0x00AC - 0x00AC)
class UTgUINewSettingsMenu : public UTgUISceneDriver
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1645 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void PopulateWidgets ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIOmegaMenu
// 0x0044 (0x00F0 - 0x00AC)
class UTgUIOmegaMenu : public UTgUISceneDriver
{
public:
	class UUISafeRegionPanel*                          m_AspectPanel;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_OmegaLabel;                                     		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_OmegaPanel[ 0x2 ];                              		// 0x00B4 (0x0008) [0x0000000000000000]              
	struct FsOmegaAlertStruct                          m_OmegaAlert[ 0x2 ];                              		// 0x00BC (0x0020) [0x0000000000000000]              
	int                                                m_nOmegaMode;                                     		// 0x00DC (0x0004) [0x0000000000000000]              
	int                                                m_nOmegaId;                                       		// 0x00E0 (0x0004) [0x0000000000000000]              
	int                                                m_nOmegaParentId;                                 		// 0x00E4 (0x0004) [0x0000000000000000]              
	int                                                m_nUseMessageId;                                  		// 0x00E8 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bVisible : 1;                                   		// 0x00EC (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1646 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	struct FString GetUseMessage ( int msgId, struct FString vendorName );
	void SetUseMessage ( int msgId, struct FString vendorName );
	void SetOmegaVisibility ( unsigned long bVisible );
	void TriggerOmegaAlert ( unsigned long bForced );
	void PopulateOmegaWidgets ( int omegaId, int omegaMode );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIOpenTerritories
// 0x0014 (0x00C0 - 0x00AC)
class UTgUIOpenTerritories : public UTgUISceneDriver
{
public:
	class UUILabel*                                    m_MinOpenTimeLabel;                               		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MaxOpenTimeLabel;                               		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MessageLabel;                                   		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_MinutesEdit;                                    		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_OpenButton;                                     		// 0x00BC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1647 ];

		return pClassPointer;
	};

	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	bool OnOpenClicked ( struct FInputEventParameters* EventParms );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIPlayerSearchMenu
// 0x012C (0x01D8 - 0x00AC)
class UTgUIPlayerSearchMenu : public UTgUISceneDriver
{
public:
	class UTgDataSet*                                  m_PlayerSearchDataSet;                            		// 0x00AC (0x0004) [0x0000000000000000]              
	class UTgDataInterface*                            m_PlayerSearchDataInterface;                      		// 0x00B0 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_CurrentSortType;                                		// 0x00B4 (0x0001) [0x0000000000000000]              
	unsigned long                                      m_bAscendingSort : 1;                             		// 0x00B8 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIPanel*                                    m_ResultPanel;                                    		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ResultListPanel;                                		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_ResultScrollFrame;                              		// 0x00C4 (0x0004) [0x0000000000000000]              
	TArray< struct FsSearchResultDisplay >             m_ResultListMembers;                              		// 0x00C8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_SelectedPlayerId;                               		// 0x00D4 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrentPage;                                   		// 0x00D8 (0x0004) [0x0000000000000000]              
	struct FsSearchResultDetailDisplay                 m_DetailPanel;                                    		// 0x00DC (0x0088) [0x0000000000000000]              
	class UUIButton*                                   m_ClassHeaderLabelButton;                         		// 0x0164 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NameHeaderLabelButton;                          		// 0x0168 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AgencyHeaderLabelButton;                        		// 0x016C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceHeaderLabelButton;                      		// 0x0170 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FactionHeaderLabelButton;                       		// 0x0174 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LevelHeaderLabelButton;                         		// 0x0178 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LocationHeaderLabelButton;                      		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InTeamStatusHeaderLabelButton;                  		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SelectionButtonPanel;                           		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SendMessageSelectedButton;                      		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InviteSelectedButton;                           		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_PagingPanel;                                    		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LastPageButton;                                 		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FirstPageButton;                                		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PrevPageButton;                                 		// 0x019C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NextPageButton;                                 		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PageNumberLabel;                                		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SearchPanel;                                    		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_PlayerNameEditbox;                              		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyNameEditbox;                              		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AllianceNameEditbox;                            		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_MinimumLevelEditbox;                            		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_MaximumLevelEditbox;                            		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TeamStatusComboBox;                             		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ClassComboBox;                                  		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_FactionComboBox;                                		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SearchButton;                                   		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ResetButton;                                    		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x01D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1648 ];

		return pClassPointer;
	};

	bool OnSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnResetKey ( struct FInputEventParameters* EventParms );
	bool OnSearchKey ( struct FInputEventParameters* EventParms );
	void eventSetRawInputKeyForResultsList ( class UUIButton* Button );
	bool OnSendMessageSelected ( struct FInputEventParameters* EventParms );
	bool OnInviteSelected ( struct FInputEventParameters* EventParms );
	bool OnTeamStatusHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnLocationHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnLevelHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnFactionHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnAllianceHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnAgencyHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnNameHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnClassHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnNextPageKey ( struct FInputEventParameters* EventParms );
	bool OnPrevPageKey ( struct FInputEventParameters* EventParms );
	bool OnLastPageKey ( struct FInputEventParameters* EventParms );
	bool OnFirstPageKey ( struct FInputEventParameters* EventParms );
	bool OnSelectResult ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void SetParentUIScene ( class UTgUISceneDriver* Scene );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
	bool OnSearchMenuKey ( struct FInputEventParameters* EventParms );
	void NextPage ( );
	void PrevPage ( );
	void FirstPage ( );
	void LastPage ( );
	void SendMessageSelected ( );
	void PlayerSelected ( );
	void InviteSelected ( );
	bool SelectResult ( int nPlayerId );
	bool SelectResultByButton ( class UUIButton* Button );
	void UpdateHighlight ( );
	void UpdateDetailPanel ( int nPlayerId );
	void UpdateResultListDisplay ( );
	void GetPlayerSearchCallback ( int eType );
	void Search ( );
	void SearchFromFields ( );
	void PrepopulatedSearchById ( int nPlayerId );
	void PrepopulatedSearch ( struct FString sPlayerName, struct FString sAgencyName, struct FString sAllianceName );
	void ResetFields ( );
};


// Class TgClient.TgUIPostCreateLoad
// 0x0004 (0x00B0 - 0x00AC)
class UTgUIPostCreateLoad : public UTgUISceneDriver
{
public:
	class ATgLoginHUD*                                 m_pLoginHUD;                                      		// 0x00AC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1649 ];

		return pClassPointer;
	};

	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIPreviewScreen
// 0x0028 (0x00D4 - 0x00AC)
class UTgUIPreviewScreen : public UTgUISceneDriver
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     m_PaperDollMeshWidget;                            		// 0x00B0 (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActor_EquipScreen*            m_PaperDoll;                                      		// 0x00B4 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bRotatePaperDollCW : 1;                         		// 0x00B8 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bRotatePaperDollCCW : 1;                        		// 0x00B8 (0x0004) [0x0000000000000000] [0x00000002] 
	class UUIButton*                                   m_wRotateDudeCW;                                  		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRotateDudeCCW;                                 		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wDragButton;                                    		// 0x00C4 (0x0004) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_EquippedItemList;                               		// 0x00C8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1650 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void OnRotateDudeStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnRotateDudeCCWClicked ( struct FInputEventParameters* EventParms );
	bool OnRotateDudeCWClicked ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnPreviewScreenDelegate ( struct FInputEventParameters* EventParms );
	void SetPaperDollPreviewItem ( class UTgInventoryObject* Item );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
};


// Class TgClient.TgUIPrimaryHUD
// 0x0148 (0x01F4 - 0x00AC)
class UTgUIPrimaryHUD : public UTgUISceneDriver
{
public:
	class ATgHUD_Game*                                 m_TgHUDGame;                                      		// 0x00AC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_DeviceBar*                   m_DeviceBar;                                      		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_ActiveItem*                  m_ActiveItem;                                     		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_TeamPanel*                   m_TeamPanel;                                      		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_MiniMap*                     m_MiniMap;                                        		// 0x00BC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_KillDisplay*                 m_KillDisplay;                                    		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_BuffBar*                     m_BuffBar;                                        		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_BuffBar*                     m_DebuffBar;                                      		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_Clock*                       m_KismetClock;                                    		// 0x00CC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_MissionInfo*                 m_MissionInfo;                                    		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_DialogQuery*                 m_Dialog;                                         		// 0x00D4 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_BeaconStatus*                m_BeaconStatusIndicator;                          		// 0x00D8 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_DangerBox*                   m_DangerBox;                                      		// 0x00DC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_RaidTimer*                   m_RaidClock;                                      		// 0x00E0 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_VMTree*                      m_VMTree;                                         		// 0x00E4 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_AgentInfo*                   m_AgentInfo;                                      		// 0x00E8 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_CombatAlert*                 m_CombatAlert;                                    		// 0x00EC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_QuestTracking*               m_QuestTracking;                                  		// 0x00F0 (0x0004) [0x0000000000000000]              
	class UUISafeRegionPanel*                          m_AspectPanel;                                    		// 0x00F4 (0x0004) [0x0000000000000000]              
	class UTgUIReticule*                               m_ReticuleLogic;                                  		// 0x00F8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ReticuleBaseline;                               		// 0x00FC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_CrosshairBaseline;                              		// 0x0100 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ReticuleCrosshair;                              		// 0x0104 (0x0004) [0x0000000000000000]              
	float                                              m_fLastReticuleTargetTime;                        		// 0x0108 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MapNameLabel;                                   		// 0x010C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_SpeakingIcon;                                   		// 0x0110 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_FullscreenFadeIn;                               		// 0x0114 (0x0004) [0x0000000000000000]              
	struct FsAttentionPanel                            m_AttentionPanel;                                 		// 0x0118 (0x0024) [0x0000000000000000]              
	struct FWeaponStatusStruct                         m_ReticuleWeaponStatus[ 0x2 ];                    		// 0x013C (0x0048) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_ReticuleTargetHealthMIC;                        		// 0x0184 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_ReticuleTargetShieldMIC;                        		// 0x0188 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bHideReticuleLabels : 1;                        		// 0x018C (0x0004) [0x0000000000044000] [0x00000001] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bSuppressOverhealing : 1;                       		// 0x018C (0x0004) [0x0000000000044000] [0x00000002] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bDisplayDeployableHealth : 1;                   		// 0x018C (0x0004) [0x0000000000044000] [0x00000004] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bDisplayCombatText : 1;                         		// 0x018C (0x0004) [0x0000000000044000] [0x00000008] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bHideEnemyTags : 1;                             		// 0x018C (0x0004) [0x0000000000044000] [0x00000010] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bHideObjectives : 1;                            		// 0x018C (0x0004) [0x0000000000044000] [0x00000020] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bHideObjectiveNames : 1;                        		// 0x018C (0x0004) [0x0000000000044000] [0x00000040] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bHideTeamPanel : 1;                             		// 0x018C (0x0004) [0x0000000000044000] [0x00000080] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_nReticuleStatusBarIndex : 1;                    		// 0x018C (0x0004) [0x0000000000044000] [0x00000100] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bIsFading : 1;                                  		// 0x018C (0x0004) [0x0000000000000000] [0x00000200] 
	int                                                m_nEnableFullHitEffect;                           		// 0x0190 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	class UUIImage*                                    m_ItemProfileChangeAlert;                         		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BoostIcon;                                      		// 0x0198 (0x0004) [0x0000000000000000]              
	float                                              m_fFadeTimeStart;                                 		// 0x019C (0x0004) [0x0000000000000000]              
	float                                              m_fFadeTime;                                      		// 0x01A0 (0x0004) [0x0000000000000000]              
	struct FVector2D                                   m_FadeAlpha;                                      		// 0x01A4 (0x0008) [0x0000000000000000]              
	struct FLinearColor                                m_FadeColor;                                      		// 0x01AC (0x0010) [0x0000000000000000]              
	struct FString                                     m_PresetMapName;                                  		// 0x01BC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_PresetZoneName;                                 		// 0x01C8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     m_PresetSubzoneName;                              		// 0x01D4 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUILabel*                                    m_MapZoneNameAlert;                               		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MapNameAlert;                                   		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MapSubZoneNameAlert;                            		// 0x01E8 (0x0004) [0x0000000000000000]              
	float                                              m_fZoneNameAlertTime;                             		// 0x01EC (0x0004) [0x0000000000000000]              
	float                                              m_fMapNameAlertTime;                              		// 0x01F0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1651 ];

		return pClassPointer;
	};

	void eventFixupUIDeviceBarsUC ( );
	bool OnPrimaryHUDKeyDelegate ( struct FInputEventParameters* EventParms );
	void UpdateFade ( );
	void SetFade ( unsigned long bEnableFading, struct FColor FadeColor, struct FVector2D FadeAlpha, float FadeTime );
	void SetOmegaVisibility ( unsigned long bVisible );
	void TempToggleElement ( unsigned char Element, unsigned long bVisible );
	void UpdateEquipTimer ( unsigned long IsActive, float EquipTime );
	void TriggerUIBarsFullUpdate ( );
	void UpdateHUDCooldowns ( int DeviceEqpPt, int DeviceMode, float SetupTime, float RechargeTime, float StartupTime );
	void UpdateReticleOffset ( float OffsetDistanceFromCenterX, float OffsetDistanceFromCenterY );
	void UpdateTargetHealthBar ( );
	void UpdateReticuleLabelValues ( );
	void SwitchReticuleStatusBar ( int ReticuleToUse );
	void SetReticuleVisibility ( unsigned long CrossHairVisibility, unsigned long WeaponStatusVisibility );
	void SetUIClockTimer ( unsigned char ClockStateBitfield, float TimerDuration );
	void UpdateAlerts ( );
	void OnSpeaking ( unsigned long bSpeaking );
	void ToggleMapNameDisplay ( );
	void RegisterCallbacks ( );
	void PostOpenScene ( );
	void PreCloseScene ( );
	bool TickTgUIScene ( );
	void FixupWidgets ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIQueueScene
// 0x0AC0 (0x0B6C - 0x00AC)
class UTgUIQueueScene : public UTgUISceneDriver
{
public:
	class UUILabel*                                    m_StatusLabel;                                    		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUIScrollFrame*                              m_QueueScrollFrame;                               		// 0x00B0 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_QueueSized : 1;                                 		// 0x00B4 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bQuickRefresh : 1;                              		// 0x00B4 (0x0004) [0x0000000000000000] [0x00000002] 
	class UUIButton*                                   m_QueueButton;                                    		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_UnqueueButton;                                  		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RefreshButton;                                  		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DevButton;                                      		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x00C8 (0x0004) [0x0000000000000000]              
	struct FQueueFeedbackStruct                        m_QueueFeedback[ 0x6 ];                           		// 0x00CC (0x0060) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FQueueRowStruct                             m_Queues[ 0x32 ];                                 		// 0x012C (0x0A28) [0x0000000000000000]              
	int                                                m_SelectedQueue;                                  		// 0x0B54 (0x0004) [0x0000000000000000]              
	int                                                m_SavedQueueID;                                   		// 0x0B58 (0x0004) [0x0000000000000000]              
	float                                              m_fUpdateTimer;                                   		// 0x0B5C (0x0004) [0x0000000000000000]              
	float                                              m_fUpdateCap;                                     		// 0x0B60 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_pawn;                                           		// 0x0B64 (0x0004) [0x0000000000000000]              
	class ATgPlayerController*                         m_PC;                                             		// 0x0B68 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1652 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void AddFeedbackMessage ( struct FString newMessage );
	bool OnDevButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnRefreshQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnLeaveQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnJoinQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyDelegate ( struct FInputEventParameters* EventParms );
	void PopulateQueueList ( );
	void RegisterCallbacks ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIRaidHexMap
// 0x0110 (0x01BC - 0x00AC)
class UTgUIRaidHexMap : public UTgUISceneDriver
{
public:
	struct FsTabButtonStruct                           m_TabStruct[ 0x4 ];                               		// 0x00AC (0x0060) [0x0000000000000000]              
	class UTgUIHexWidget_Raid*                         m_pHexWidget;                                     		// 0x010C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RaidTimePanel;                                  		// 0x0110 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeLabel;                                  		// 0x0114 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeHeader;                                 		// 0x0118 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexInfoLabel;                                   		// 0x011C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexTypeLabel;                                   		// 0x0120 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexBonusLabel;                                  		// 0x0124 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TeamLeaders[ 0x6 ];                             		// 0x0128 (0x0018) [0x0000000000000000]              
	class UUILabel*                                    m_TeamCount[ 0x6 ];                               		// 0x0140 (0x0018) [0x0000000000000000]              
	class UUIPanel*                                    m_HexInfoPanel;                                   		// 0x0158 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BaseInfoPanel;                                  		// 0x015C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidLeaderLabel;                                		// 0x0160 (0x0004) [0x0000000000000000]              
	struct FMissionDelayStruct                         m_MissionDelay;                                   		// 0x0164 (0x0014) [0x0000000000000000]              
	unsigned long                                      m_bIsTeamLead : 1;                                		// 0x0178 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_eRaidPhase;                                     		// 0x017C (0x0004) [0x0000000000000000]              
	int                                                m_nCharId;                                        		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wPromoteButton;                                 		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wLeaveButton;                                   		// 0x0190 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedRaidHexUpdateCount;                      		// 0x0194 (0x0004) [0x0000000000000000]              
	struct FRaidControlPanelStruct                     m_ControlPanelStruct;                             		// 0x0198 (0x0020) [0x0000000000000000]              
	int                                                m_nTeamUpdateSeq;                                 		// 0x01B8 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1653 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnSceneRaidDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneCargoDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneDetailDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneLeaveDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseDelegate ( struct FInputEventParameters* EventParms );
	bool OnReturnDelegate ( struct FInputEventParameters* EventParms );
	bool OnLaunchClickDelegate ( struct FInputEventParameters* EventParms );
	void UpdateInfoPanel ( );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIRaidMenu
// 0x107C (0x1128 - 0x00AC)
class UTgUIRaidMenu : public UTgUISceneDriver
{
public:
	struct FsTabButtonStruct                           m_TabStruct[ 0x4 ];                               		// 0x00AC (0x0060) [0x0000000000000000]              
	struct FRaidTeamStruct                             m_RaidTeams[ 0x6 ];                               		// 0x010C (0x0F60) [0x0000000000000000]              
	struct FMemberStruct                               m_MovingMember;                                   		// 0x106C (0x0034) [0x0000000000000000]              
	struct FTotalAgentsStruct                          m_TotalAgentsStruct;                              		// 0x10A0 (0x000C) [0x0000000000000000]              
	struct FButtonStruct                               m_MoveToBtnStruct;                                		// 0x10AC (0x0008) [0x0000000000000000]              
	struct FButtonStruct                               m_PromoteBtnStruct;                               		// 0x10B4 (0x0008) [0x0000000000000000]              
	struct FSelectedSlot                               m_SelectedSlot;                                   		// 0x10BC (0x0008) [0x0000000000000000]              
	struct FSelectedSlot                               m_PickedUpSlot;                                   		// 0x10C4 (0x0008) [0x0000000000000000]              
	struct FSelectedSlot                               m_MySlot;                                         		// 0x10CC (0x0008) [0x0000000000000000]              
	struct FRaidControlPanelStruct                     m_ControlPanelStruct;                             		// 0x10D4 (0x0020) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x10F4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x10F8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wPromoteButton;                                 		// 0x10FC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wLeaveButton;                                   		// 0x1100 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidLeaderLabel;                                		// 0x1104 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RaidTimePanel;                                  		// 0x1108 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeLabel;                                  		// 0x110C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeHeader;                                 		// 0x1110 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_pawn;                                           		// 0x1114 (0x0004) [0x0000000000000000]              
	class ATgPlayerController*                         m_PC;                                             		// 0x1118 (0x0004) [0x0000000000000000]              
	int                                                m_nCharId;                                        		// 0x111C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bIsRaidLead : 1;                                		// 0x1120 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_nTeamUpdateSeq;                                 		// 0x1124 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1654 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
	bool OnSceneMapDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneCargoDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneDetailDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnReturnDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneLeaveDelegate ( struct FInputEventParameters* EventParms );
	bool OnScenePromoteDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool OnMoveToDelegate ( struct FInputEventParameters* EventParms );
	bool OnRaidTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnRaidMemberDelegate ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIReleaseMenus
// 0x0048 (0x00F4 - 0x00AC)
class UTgUIReleaseMenus : public UTgUISceneDriver
{
public:
	struct FsBasicReleaseDialog                        m_ReleaseDialog;                                  		// 0x00AC (0x001C) [0x0000000000000000]              
	class UUIImage*                                    m_DeathView;                                      		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KilledByLabel;                                  		// 0x00CC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KilledByWeapon;                                 		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KilledByHealth;                                 		// 0x00D4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_SpectateLabel;                                  		// 0x00D8 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_SpectatePawn;                                   		// 0x00DC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BasePanel;                                      		// 0x00E0 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bHideScene : 1;                                 		// 0x00E4 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bKillLabelFade : 1;                             		// 0x00E4 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bFadeDone : 1;                                  		// 0x00E4 (0x0004) [0x0000000000000000] [0x00000004] 
	float                                              m_fKillDisplayTime;                               		// 0x00E8 (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_MissionInfo*                 m_MissionInfo;                                    		// 0x00EC (0x0004) [0x0000000000000000]              
	class UTgUIPrimaryHUD_RaidTimer*                   m_RaidClock;                                      		// 0x00F0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1655 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnBasicReleaseDialogButtonKey ( struct FInputEventParameters* EventParms );
	void SetSpectateText ( );
	void ToggleHideScene ( unsigned long hideFlag );
	bool IsReleaseDialogActive ( );
	void SetDeathZoomInfo ( struct FDeathZoomInfo Info );
	void SetReleaseData ( unsigned long bActive, float fReleaseTime, unsigned long bCanManuallyRelease );
	void UpdateReleaseDialog ( );
	void UserManuallyReleased ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUISceneDriver_Tooltip
// 0x00D0 (0x017C - 0x00AC)
class UTgUISceneDriver_Tooltip : public UTgUISceneDriver
{
public:
	struct FsToolTip                                   m_sToolTip;                                       		// 0x00AC (0x002C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIPrefab*                                   m_wToolTipPrefab;                                 		// 0x00D8 (0x0004) [0x0000000000000000]              
	class UUIScreenObject*                             m_wToolTipTarget;                                 		// 0x00DC (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bTipDirty : 1;                                  		// 0x00E0 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bTipHidden : 1;                                 		// 0x00E0 (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                m_nTipTypeCount[ 0x6 ];                           		// 0x00E4 (0x0018) [0x0000000000000000]              
	struct FsConfirmPopupStruct                        m_wConfirmPopup;                                  		// 0x00FC (0x0060) [0x0000000000000000]              
	struct FsConfirmDataStruct                         m_ConfirmData;                                    		// 0x015C (0x001C) [0x0000000000000000]              
	class UUIPrefab*                                   m_wConfirmPrefab;                                 		// 0x0178 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1656 ];

		return pClassPointer;
	};

	void eventFixupConfirmWidgets ( );
	void OpenFeatureUpgradePopup ( unsigned long bSubscriberCheck );
	void OnConfirmItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnAmountChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnConfirmOkClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void OpenConfirmInfoPopup ( struct FString Text );
	void OpenConfirmDeletePopup ( class UTgInventoryObject* pItem, int maxItems );
	void CloseConfirmPopup ( );
	void ShowConfirmPopup ( );
	void SetUpConfirmEditBoxes ( );
	void SetUpConfirmItemPanel ( );
	void CreateConfirmPopup ( );
	bool TickTgUIScene ( );
	void OpenTgUIScene ( );
};


// Class TgClient.TgUIAgencyDirectory
// 0x00C8 (0x0244 - 0x017C)
class UTgUIAgencyDirectory : public UTgUISceneDriver_Tooltip
{
public:
	class ATgPlayerController*                         m_PC;                                             		// 0x017C (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedAgencyId;                              		// 0x0180 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bApplyMode : 1;                                 		// 0x0184 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_eApplicationState;                              		// 0x0188 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrentAgencyId;                               		// 0x018C (0x0004) [0x0000000000000000]              
	int                                                m_nPendingAgencyId;                               		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ResultsPanel;                                   		// 0x0194 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_AgencyScrollList;                               		// 0x0198 (0x0004) [0x0000000000000000]              
	TArray< struct FAgencyDirectoryStruct >            m_AgencyListMembers;                              		// 0x019C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   m_CloseButton;                                    		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HeaderPanel;                                    		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderLabel;                                    		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DirectoryPanel;                                 		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SearchPanel;                                    		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgencySearchPanel;                              		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceSearchPanel;                            		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgencyNamePanel;                                		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyNameEntryField;                           		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgencyMinPanel;                                 		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyMinEntryField;                            		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgencyMaxPanel;                                 		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyMaxEntryField;                            		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceNamePanel;                              		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AllianceNameEntryField;                         		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceMinPanel;                               		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AllianceMinEntryField;                          		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceMaxPanel;                               		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AllianceMaxEntryField;                          		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_RecruitingCheckbox;                             		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_RecruitingNonSubCheckbox;                       		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SearchButton;                                   		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ZoneComboBox;                                   		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MessagePanel;                                   		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MessageLabel;                                   		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_PendingPanel;                                   		// 0x020C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelButton;                                   		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplyButton;                                    		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ApplicationLabel;                               		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ApplicationPanel;                               		// 0x021C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TopMessageLabel;                                		// 0x0220 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_ApplicationEntryField;                          		// 0x0224 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SendCancelButton;                               		// 0x0228 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SendButton;                                     		// 0x022C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_PersonalApplicationPanel;                       		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PreviewLabel;                                   		// 0x0234 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_PreviewScrollList;                              		// 0x0238 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_PreviewPanel;                                   		// 0x023C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_PreviewImage;                                   		// 0x0240 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1657 ];

		return pClassPointer;
	};

	void eventSetRawInputKeyForAgencyButton ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	void OnEnterValueChanged ( class UUIObject* ebox, int PlayerIndex );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnCancelButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSendCancelButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSendButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnApplyButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectAgencyDelegate ( struct FInputEventParameters* EventParms );
	bool OnSearchButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyDelegate ( struct FInputEventParameters* EventParms );
	void PreCloseScene ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIAgencyMenu
// 0x002C (0x01A8 - 0x017C)
class UTgUIAgencyMenu : public UTgUISceneDriver_Tooltip
{
public:
	unsigned long                                      m_bNeedsAgencyDataUpdate : 1;                     		// 0x017C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bNeedsAllianceDataUpdate : 1;                   		// 0x017C (0x0004) [0x0000000000000000] [0x00000002] 
	class UUIButton*                                   m_GeneralTabButton;                               		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceTabButton;                              		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BaseTabButton;                                  		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplicantsTabButton;                            		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ManagementTabButton;                            		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FacilitiesTabButton;                            		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InventoryTabButton;                             		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RecruitingTabButton;                            		// 0x019C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AuditingTabButton;                              		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x01A4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1658 ];

		return pClassPointer;
	};

	void eventSelectTab ( unsigned char Tab );
	bool OnSelectInventoryTab ( struct FInputEventParameters* EventParms );
	bool OnSelectAuditingTab ( struct FInputEventParameters* EventParms );
	bool OnSelectRecruitingTab ( struct FInputEventParameters* EventParms );
	bool OnSelectFacilitiesTab ( struct FInputEventParameters* EventParms );
	bool OnSelectManagementTab ( struct FInputEventParameters* EventParms );
	bool OnSelectApplicantsTab ( struct FInputEventParameters* EventParms );
	bool OnSelectAllianceTab ( struct FInputEventParameters* EventParms );
	bool OnSelectBaseTab ( struct FInputEventParameters* EventParms );
	bool OnSelectGeneralTab ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	bool OnAgencyMenuKey ( struct FInputEventParameters* EventParms );
	class UTgAllianceData* GetAllianceData ( );
	class UTgAgencyData* GetAgencyData ( );
	void UpdateViewablePanels ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAgencyMenu_Alliance
// 0x0091 (0x0239 - 0x01A8)
class UTgUIAgencyMenu_Alliance : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_AlliancePagePanel;                              		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceInformationPanel;                       		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceNameLabel;                              		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceNumMembersLabel;                        		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceFormedLabel;                            		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceAtWarWithLabel;                         		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceAtWarLabel;                             		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceDescriptionLabel;                       		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceCreationButton;                         		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceCreatePanel;                            		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceNamePanel;                              		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AllianceNameEntryField;                         		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceTagPanel;                               		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceMiniumAgentsLabel;                      		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceMemberListPanel;                        		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_AllianceAgencyScrollList;                       		// 0x01E4 (0x0004) [0x0000000000000000]              
	TArray< struct FAllianceMemberDisplayStruct >      m_AllianceMemberListMembers;                      		// 0x01E8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_AllianceSelectedAgencyId;                       		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceMemberInfoPanel;                        		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceMemberName;                             		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceMemberLeader;                           		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceMemberMemberCount;                      		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceMemberHexCount;                         		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceMemberRemoveButton;                     		// 0x020C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceMemberLeaveButton;                      		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceMemberLeaveLabel;                       		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceApplicantPanel;                         		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AllianceApplicantNameEditBox;                   		// 0x021C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceApplicantInviteButton;                  		// 0x0220 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceApplicantErrorMessagePanel;             		// 0x0224 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AllianceConfirmPanel;                           		// 0x0228 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceConfirmAcceptButton;                    		// 0x022C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AllianceConfirmCancelButton;                    		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AllianceConfirmLabel;                           		// 0x0234 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_AllianceConfirmMode;                            		// 0x0238 (0x0001) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1659 ];

		return pClassPointer;
	};

	bool OnSelectAllianceMember ( struct FInputEventParameters* EventParms );
	void eventSetRawInputKeyForAllianceMemberList ( class UUIButton* Button );
	bool OnAllianceApplicantInviteSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnAllianceApplicantInviteButtonKey ( struct FInputEventParameters* EventParms );
	bool OnConfirmCancelKey ( struct FInputEventParameters* EventParms );
	bool OnConfirmAcceptKey ( struct FInputEventParameters* EventParms );
	bool OnMemberLeaveButton ( struct FInputEventParameters* EventParms );
	bool OnMemberRemoveButton ( struct FInputEventParameters* EventParms );
	bool OnCreationSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnCreationButtonKey ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	bool SelectAllianceMember ( int nAgencyId );
	bool SelectAllianceMemberByButton ( class UUIButton* Button );
	void UpdateAllianceMemberHighlight ( );
	void UpdateDetailPanel ( struct FsAllianceMemberInfo selectedAgency );
	void CloseConfirm ( );
	void DoConfirm ( unsigned char Mode );
	void ResizeAllianceMemberList ( int nSize );
	void SendAllianceCreate ( struct FString sName );
	void UpdateViewablePanels ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void UpdateAllianceDisplay ( );
	void SendAllianceLeave ( );
	void SendAllianceRemove ( int nAgencyId );
	void SendAllianceInvite ( struct FString sPlayerName );
	void FixupWidgets ( );
};


// Class TgClient.TgUIAgencyMenu_Applicants
// 0x0010 (0x01B8 - 0x01A8)
class UTgUIAgencyMenu_Applicants : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_ApplicantsPagePanel;                            		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_ApplicantNameEditBox;                           		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplicantInviteButton;                          		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ApplicantErrorMessagePanel;                     		// 0x01B4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1660 ];

		return pClassPointer;
	};

	bool OnApplicantInviteSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnApplicantInviteButtonKey ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void PostOpenScene ( );
	void FixupWidgets ( );
	void UpdateViewablePanels ( );
	void SendAgencyInvite ( struct FString sPlayerName );
};


// Class TgClient.TgUIAgencyMenu_Creation
// 0x0030 (0x01D8 - 0x01A8)
class UTgUIAgencyMenu_Creation : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_CreationPagePanel;                              		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_CreationNameEditBox;                            		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_CreationAbbreviationEditBox;                    		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_CreationColorImage;                             		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CreationColorButton;                            		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CreationButton;                                 		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreationErrorMessagePanel;                      		// 0x01C0 (0x0004) [0x0000000000000000]              
	struct FLinearColor                                m_GeneratedColor;                                 		// 0x01C4 (0x0010) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_GeneratedColorMIC;                              		// 0x01D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1661 ];

		return pClassPointer;
	};

	bool OnCreationSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnCreationButtonKey ( struct FInputEventParameters* EventParms );
	bool OnGenerateColorKey ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void GenerateNewColor ( );
	void FixupWidgets ( );
	void SendAgencyCreate ( struct FString sAgencyName );
};


// Class TgClient.TgUIAgencyMenu_Facilities
// 0x0108 (0x02B0 - 0x01A8)
class UTgUIAgencyMenu_Facilities : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_FacilitiesPagePanel;                            		// 0x01A8 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bCanEditHexMap : 1;                             		// 0x01AC (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bZoneChangeDisable : 1;                         		// 0x01AC (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bDragMode : 1;                                  		// 0x01AC (0x0004) [0x0000000000000000] [0x00000004] 
	int                                                m_nCallbackHexPosition;                           		// 0x01B0 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedHexUpdateCount;                          		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UTgUIHexWidget_Zone*                         m_pHexWidget;                                     		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ZoneListComboBox;                               		// 0x01BC (0x0004) [0x0000000000000000]              
	struct FHexInfoStruct                              m_HexInfo;                                        		// 0x01C0 (0x0040) [0x0000000000000000]              
	struct FFacilityQueueStruct                        m_QueueInfo;                                      		// 0x0200 (0x0044) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FDefensiveConfigStruct                      m_DefenseInfo;                                    		// 0x0244 (0x0024) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_TaskProgressBarParent;                          		// 0x0268 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_NavigationPanel;                                		// 0x026C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ZoomInButton;                                   		// 0x0270 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ZoomOutButton;                                  		// 0x0274 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PanButtons[ 0x4 ];                              		// 0x0278 (0x0010) [0x0000000000000000]              
	struct FName                                       m_HexButtonStyleResolverTag;                      		// 0x0288 (0x0008) [0x0000000000000000]              
	struct FName                                       m_InactiveShieldStyleName;                        		// 0x0290 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ActiveShieldStyleName;                          		// 0x0298 (0x0008) [0x0000000000000000]              
	struct FVector                                     m_DragMousePosition;                              		// 0x02A0 (0x000C) [0x0000000000000000]              
	float                                              m_fZoomDelta;                                     		// 0x02AC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1662 ];

		return pClassPointer;
	};

	void eventSetCancelTaskDelegate ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void UpdateProductionInformation ( );
	void UpdateHexInformation ( );
	void UpdateZoneList ( );
	bool OnNavButton ( struct FInputEventParameters* EventParms );
	bool OnDefenseApplied ( struct FInputEventParameters* EventParms );
	bool OnShieldTileClicked ( struct FInputEventParameters* EventParms );
	void UpdateDefenseButtons ( );
	void OnCurrentTaskStateChanged ( class UUIScreenObject* Sender, int nPlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnCancelCurrentTask ( struct FInputEventParameters* EventParms );
	bool OnCancelTask ( struct FInputEventParameters* EventParms );
	bool OnAddTask ( struct FInputEventParameters* EventParms );
	bool OnAbandonHex ( struct FInputEventParameters* EventParms );
	bool OnTransferHex ( struct FInputEventParameters* EventParms );
	bool OnChangeHQ ( struct FInputEventParameters* EventParms );
	bool OnChangeFacility ( struct FInputEventParameters* EventParms );
	void OnZoneChanged ( class UUIObject* Sender, int PlayerIndex );
	bool OnAgencyMenuKey ( struct FInputEventParameters* EventParms );
	int GetHoverHexCellPosition ( );
	int GetSelectedHexCellPosition ( );
	void PostRender ( );
	void UpdateHexMap ( );
	void UpdateToolTipData ( );
	void RegisterCallbacks ( );
	bool TickTgUIScene ( );
	void FixupWidgets ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIAgencyMenu_General
// 0x00B4 (0x025C - 0x01A8)
class UTgUIAgencyMenu_General : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_GeneralPagePanel;                               		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MemberListScrollFrame;                          		// 0x01AC (0x0004) [0x0000000000000000]              
	TArray< struct FsAgencyMemberDisplay >             m_MemberListMembers;                              		// 0x01B0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_SelectedPlayerId;                               		// 0x01BC (0x0004) [0x0000000000000000]              
	struct FsPopupTextEditPanel                        m_PopupEditField;                                 		// 0x01C0 (0x0014) [0x0000000000000000]              
	unsigned char                                      m_CurrentEditBoxMode;                             		// 0x01D4 (0x0001) [0x0000000000000000]              
	class UUILabel*                                    m_AgencyNameLabel;                                		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AgencyNumMembersLabel;                          		// 0x01DC (0x0004) [0x0000000000000000]              
	struct FsCommentPanel                              m_MOTDPanel;                                      		// 0x01E0 (0x000C) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowOfflineCheckbox;                            		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_NameHeaderLabelButton;                          		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_RankHeaderLabelButton;                          		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_StatusHeaderLabelButton;                        		// 0x01F8 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x01FC (0x0018) [0x0000000000000000]              
	struct FsMemberDetailPanel                         m_MemberDetailPanel;                              		// 0x0214 (0x0048) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1663 ];

		return pClassPointer;
	};

	void eventSetMemberListSortType ( unsigned char SortType );
	void eventSetRawInputKeyForMemberList ( class UUIButton* Button );
	bool OnChangeOfficerCommentButtonKey ( struct FInputEventParameters* EventParms );
	bool OnChangePublicCommentButtonKey ( struct FInputEventParameters* EventParms );
	bool OnChangeMOTDButtonKey ( struct FInputEventParameters* EventParms );
	void eventSubmitPopupEditBox ( );
	bool OnPopupSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnPopupPanelCancel ( struct FInputEventParameters* EventParms );
	bool OnPopupPanelAccept ( struct FInputEventParameters* EventParms );
	void eventSetupPopupTextEditPanel ( unsigned char NewMode );
	bool OnTeamInviteButtonKey ( struct FInputEventParameters* EventParms );
	bool OnRemoveButtonKey ( struct FInputEventParameters* EventParms );
	bool OnDemoteButtonKey ( struct FInputEventParameters* EventParms );
	bool OnPromoteButtonKey ( struct FInputEventParameters* EventParms );
	bool OnSearchHotlinkKey ( struct FInputEventParameters* EventParms );
	bool OnStatusHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnRankHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnNameHeaderKey ( struct FInputEventParameters* EventParms );
	bool OnSelectMember ( struct FInputEventParameters* EventParms );
	void OnShowOfflineCheckBoxChanged ( class UUIObject* Sender, int PlayerIndex );
	void eventFixupWidgetsUC ( );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void PostOpenScene ( );
	void PreCloseScene ( );
	void FixupWidgets ( );
	void TeamInviteSelected ( );
	void RemoveSelected ( );
	void DemoteSelected ( );
	void PromoteSelected ( );
	void PlayerSearchSelected ( );
	bool OnAgencyMenuKey ( struct FInputEventParameters* EventParms );
	bool SelectMember ( int nPlayerId );
	bool SelectMemberByButton ( class UUIButton* Button );
	void UpdateHighlight ( );
	void UpdateDetailPanel ( struct FsAgencyMemberInfo selectedPlayer, struct FsAgencyMemberInfo LocalPlayer );
	void UpdateMemberListDisplay ( unsigned long bNeedsResort );
};


// Class TgClient.TgUIAgencyMenu_Inventory
// 0x011C (0x02C4 - 0x01A8)
class UTgUIAgencyMenu_Inventory : public UTgUIAgencyMenu
{
public:
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x01A8 (0x0004) [0x0000000000000000]              
	TArray< struct FsInventoryListItem >               m_InventoryItems;                                 		// 0x01AC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsInventoryListItem >               m_AgencyItems;                                    		// 0x01B8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_LocalInventoryStore;                            		// 0x01C4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_AgencyList;                                     		// 0x01D0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ValidTypeList;                                  		// 0x01DC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_SelectedItem;                                   		// 0x01E8 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedAgencyItem;                             		// 0x01EC (0x0004) [0x0000000000000000]              
	int                                                m_CurrentCredits;                                 		// 0x01F0 (0x0004) [0x0000000000000000]              
	class ATgPawn*                                     m_PlayerPawn;                                     		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TypeComboBox;                                   		// 0x01F8 (0x0004) [0x0000000000000000]              
	int                                                m_TypeFilter;                                     		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x0200 (0x0004) [0x0000000000000000]              
	int                                                m_QualityFilter;                                  		// 0x0204 (0x0004) [0x0000000000000000]              
	struct FsAgencyInvPanel                            m_PersonalInventory;                              		// 0x0208 (0x0044) [0x0000000000000000]              
	struct FsAgencyInvPanel                            m_AgencyInventory;                                		// 0x024C (0x0044) [0x0000000000000000]              
	TArray< struct FString >                           m_ItemTypeStringList;                             		// 0x0290 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ItemTypeCodeList;                               		// 0x029C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x02A8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_QualityCodeList;                                		// 0x02B4 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned long                                      m_bRefresh : 1;                                   		// 0x02C0 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPopupMode : 1;                                 		// 0x02C0 (0x0004) [0x0000000000000000] [0x00000002] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1664 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetAgencyItemStateChange ( class UUIScreenObject* Obj );
	void eventSetSelectAgencyItemDelegate ( class UUIScreenObject* Obj );
	void eventSetInvItemStateChange ( class UUIScreenObject* Obj );
	void eventSetSelectInventoryItemDelegate ( class UUIScreenObject* Obj );
	bool OnPrevAgencyButton ( struct FInputEventParameters* EventParms );
	bool OnNextAgencyButton ( struct FInputEventParameters* EventParms );
	void OnAgencyItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnFilterChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnItemSendToDelegate ( struct FInputEventParameters* EventParms );
	bool OnDeleteItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnWithdrawCreditsDelegate ( struct FInputEventParameters* EventParms );
	bool OnDepositCreditsDelegate ( struct FInputEventParameters* EventParms );
	bool OnWithdrawItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnDepositItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnSortAgencyInventoryDelegate ( struct FInputEventParameters* EventParms );
	bool OnSortInventoryDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectAgencyItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnAgencyMenuKey ( struct FInputEventParameters* EventParms );
	void HideTransferItemPanel ( );
	void ShowTransferCreditsPanel ( int creditCount, unsigned long bDeposit );
	void ShowTransferItemPanel ( class UTgInventoryObject* pItem, int ItemCount, unsigned long bDeposit );
	void SelectAgencyItem ( int ItemIndex );
	void SelectInventoryItem ( int ItemIndex );
	void PopulateAgencyItems ( );
	void PopulateInventoryItems ( );
	void ResizeAgencyPanelWidgets ( int newSize );
	void ResizeInventoryPanelWidgets ( int newSize );
	void BuildLocalInventoryStore ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAgencyMenu_InventoryLog
// 0x0060 (0x0208 - 0x01A8)
class UTgUIAgencyMenu_InventoryLog : public UTgUIAgencyMenu
{
public:
	TArray< struct FInventoryLogStruct >               m_LogMembers;                                     		// 0x01A8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class ATgPlayerController*                         m_PC;                                             		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SearchPanel;                                    		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_DateStartField;                                 		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_DateEndField;                                   		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_EventComboBox;                                  		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_NameEditBox;                                    		// 0x01C8 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_EventStringList;                                		// 0x01CC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_EventCodeList;                                  		// 0x01D8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   m_SearchButton;                                   		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_LogScrollList;                                  		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ResultsPanel;                                   		// 0x01EC (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x01F0 (0x0018) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1665 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnSearchButtonDelegate ( struct FInputEventParameters* EventParms );
	void ResizeLogList ( int nSize );
	void ResetFilters ( );
	void FixupTgUIScene ( );
	void OpenTgUIScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAgencyMenu_Management
// 0x0098 (0x0240 - 0x01A8)
class UTgUIAgencyMenu_Management : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_ManagementPagePanel;                            		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DisbandPanel;                                   		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DisbandButton;                                  		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TransferLeaderPanel;                            		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_TransferLeaderEditBox;                          		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TransferLeaderButton;                           		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyDescriptionEditBox;                       		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AgencyDescriptionButton;                        		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AgencyDescriptionPreview;                       		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_AgencyRegionComboBox;                           		// 0x01CC (0x0004) [0x0000000000000000]              
	int                                                m_SelectedSiteIndex;                              		// 0x01D0 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedRankIndex;                              		// 0x01D4 (0x0004) [0x0000000000000000]              
	TArray< struct FsAgencyRankData >                  m_EditedRankData;                                 		// 0x01D8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	unsigned long                                      m_bRankDataDirty : 1;                             		// 0x01E4 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bSuppressRankValueUpdate : 1;                   		// 0x01E4 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bCanEditRanks : 1;                              		// 0x01E4 (0x0004) [0x0000000000000000] [0x00000004] 
	class UUIComboBox*                                 m_RankComboBox;                                   		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RankAddButton;                                  		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RankSendButton;                                 		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RankRevertButton;                               		// 0x01F4 (0x0004) [0x0000000000000000]              
	struct FsRankDetailPanel                           m_RankDetailPanel;                                		// 0x01F8 (0x0048) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1666 ];

		return pClassPointer;
	};

	bool OnAgencyDescriptionButton ( struct FInputEventParameters* EventParms );
	bool OnAgencyDescriptionSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnTransferLeaderButton ( struct FInputEventParameters* EventParms );
	bool OnTransferLeaderSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnRankDisabled ( struct FInputEventParameters* EventParms );
	void OnRankValueChanged ( class UUIObject* Sender, int PlayerIndex );
	bool OnRankDemote ( struct FInputEventParameters* EventParms );
	bool OnRankPromote ( struct FInputEventParameters* EventParms );
	bool OnRankRevert ( struct FInputEventParameters* EventParms );
	bool OnRankSend ( struct FInputEventParameters* EventParms );
	bool OnRankRemove ( struct FInputEventParameters* EventParms );
	bool OnRankAdd ( struct FInputEventParameters* EventParms );
	bool OnAgencyDisband ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void RegionComboBoxChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnAgencyDescriptionValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnRankChanged ( class UUIObject* Sender, int PlayerIndex );
	bool TickTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void FixupWidgets ( );
	void UpdateDetailPanel ( struct FsAgencyRankData rankData );
	void UpdateRankList ( );
	void RankValueUpdated ( class UUIObject* Sender );
	void DemoteRank ( );
	void PromoteRank ( );
	void RevertRankData ( );
	void SendRankData ( );
	void RemoveRank ( );
	void AddRank ( );
	void Disband ( );
	void TransferLeader ( );
	void SetAgencyRegion ( );
	void SetAgencyDescription ( );
	bool SelectRank ( int nRankId );
	void UpdateViewablePanels ( );
};


// Class TgClient.TgUIAgencyMenu_Recruiting
// 0x0060 (0x0208 - 0x01A8)
class UTgUIAgencyMenu_Recruiting : public UTgUIAgencyMenu
{
public:
	class UUIPanel*                                    m_RecruitingPanel;                                		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgentListingPanel;                              		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_MessageEntryField;                              		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MessageLabel;                                   		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_RecruitingCheckbox;                             		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_NonSubscriberCheckbox;                          		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MessageSaveButton;                              		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgentSubmissionsPanel;                          		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgentSubmissionsListPanel;                      		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_DateSortButton;                                 		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_ClassSortButton;                                		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_NameSortButton;                                 		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_LevelSortButton;                                		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_SkillSortButton;                                		// 0x01DC (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_AgentScrollList;                                		// 0x01E0 (0x0004) [0x0000000000000000]              
	TArray< struct FAgentSubmissionStruct >            m_AgentListMembers;                               		// 0x01E4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_SelectedAgentId;                                		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AgentMessagePanel;                              		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AgentMessageLabel;                              		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AgentNameLabel;                                 		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AgentAcceptButton;                              		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AgentDeclineButton;                             		// 0x0204 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1667 ];

		return pClassPointer;
	};

	bool OnSubmitButtonKey ( struct FInputEventParameters* EventParms );
	bool OnSelectAgent ( struct FInputEventParameters* EventParms );
	void eventSetRawInputKeyForAgentListMember ( class UUIButton* Button );
	bool OnAcceptKey ( struct FInputEventParameters* EventParms );
	bool OnDeclineKey ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void OnEnterValueChanged ( class UUIObject* ebox, int PlayerIndex );
	bool OnDeclineButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnAcceptButtonDelegate ( struct FInputEventParameters* EventParms );
	void SendRecruitingUpdate ( );
	bool SelectAgent ( int nCharacterId );
	bool SelectAgentByButton ( class UUIButton* Button );
	void UpdateViewablePanels ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void FixupWidgets ( );
};


// Class TgClient.TgUIAgentProfile
// 0x0078 (0x01F4 - 0x017C)
class UTgUIAgentProfile : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_EquipmentTabButton;                             		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SkillsTabButton;                                		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_StatsTabButton;                                 		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_InventoryTabButton;                             		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AchievementsTabButton;                          		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0190 (0x0004) [0x0000000000000000]              
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x0194 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_PlayerPawn;                                     		// 0x0198 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_InspectedPawn;                                  		// 0x019C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ProfileButtonArray[ 0x5 ];                      		// 0x01A0 (0x0014) [0x0000000000000000]              
	class UUILabel*                                    m_ProfileLabelArray[ 0x5 ];                       		// 0x01B4 (0x0014) [0x0000000000000000]              
	struct FName                                       m_ButtonStyleResolverTag;                         		// 0x01C8 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ButtonActiveStyleName;                          		// 0x01D0 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ButtonInactiveStyleName;                        		// 0x01D8 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ProfileActiveStyleName;                         		// 0x01E0 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ProfileInactiveStyleName;                       		// 0x01E8 (0x0008) [0x0000000000000000]              
	float                                              m_fLastProfileChange;                             		// 0x01F0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1668 ];

		return pClassPointer;
	};

	void eventSelectTab ( unsigned char Tab );
	bool OnSelectAchieveTab ( struct FInputEventParameters* EventParms );
	bool OnSelectInvTab ( struct FInputEventParameters* EventParms );
	bool OnSelectStatsTab ( struct FInputEventParameters* EventParms );
	bool OnSelectSkillsTab ( struct FInputEventParameters* EventParms );
	bool OnSelectEquipTab ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	bool OnSelectProfileButton ( struct FInputEventParameters* EventParms );
	class ATgPawn_Character* GetPawnToUse ( );
	bool CanChangeProfiles ( );
	bool IsReadOnlyMode ( );
	void SetInspectedPawn ( class ATgPawn_Character* Pawn );
	bool OnAgentMenuKey ( struct FInputEventParameters* EventParms );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAgentProfile_Achievement
// 0x04AC (0x06A0 - 0x01F4)
class UTgUIAgentProfile_Achievement : public UTgUIAgentProfile
{
public:
	TArray< struct FAchievementGroupData >             m_AchievementGroupData;                           		// 0x01F4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_CategoryPanel;                                  		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CountLabel;                                     		// 0x0204 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_CategoryPagingPanel;                            		// 0x0208 (0x0018) [0x0000000000000000]              
	struct FAchievementGroup                           m_AchievementGroups[ 0x16 ];                      		// 0x0220 (0x0108) [0x0000000000000000]              
	class UUIPanel*                                    m_ListPanel;                                      		// 0x0328 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_ListPagingPanel;                                		// 0x032C (0x0018) [0x0000000000000000]              
	struct FAchievementItem                            m_AchievementItems[ 0x9 ];                        		// 0x0344 (0x033C) [0x0000000000000000]              
	class UUILabel*                                    m_SceneHeader;                                    		// 0x0680 (0x0004) [0x0000000000000000]              
	class UMaterialInterface*                          m_ProgressBarMaterial;                            		// 0x0684 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pAchievementMarshal;                            		// 0x0688 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                m_CurrentCategoryPage;                            		// 0x068C (0x0004) [0x0000000000000000]              
	int                                                m_CurrentAchievementPage;                         		// 0x0690 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedAchievementGroupId;                     		// 0x0694 (0x0004) [0x0000000000000000]              
	int                                                m_nTotalCategoryPage;                             		// 0x0698 (0x0004) [0x0000000000000000]              
	int                                                m_nTotalCategoryAchievementPage;                  		// 0x069C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1669 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventClearRawInputKeyForAchievementsList ( class UUIButton* Button );
	void eventSetRawInputKeyForCategory ( class UUIButton* Button );
	void ActiveTooltipChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnProgressBarStateChangeDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnAchievementNextButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnAchievementPrevButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnCategoryNextButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnCategoryPrevButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnAchievementCategoryClicked ( struct FInputEventParameters* EventParms );
	void PreCloseScene ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIAgentProfile_Equip
// 0x00E4 (0x02D8 - 0x01F4)
class UTgUIAgentProfile_Equip : public UTgUIAgentProfile
{
public:
	TArray< struct FDeviceListItemStruct >             m_InventoryItems;                                 		// 0x01F4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FEquippedItemStruct >               m_EquippedDevices;                                		// 0x0200 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FEquippedItemStruct >               m_EquippedUpgrades;                               		// 0x020C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FEquippedItemStruct >               m_EquippedFlair;                                  		// 0x0218 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   m_DeviceTabButton;                                		// 0x0224 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ArmorTabButton;                                 		// 0x0228 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AppearanceTabButton;                            		// 0x022C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DevicePanel;                                    		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ArmorPanel;                                     		// 0x0234 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AppearancePanel;                                		// 0x0238 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PrevArrowButton;                                		// 0x023C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NextArrowButton;                                		// 0x0240 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PagingLabel;                                    		// 0x0244 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FeedbackLabel;                                  		// 0x0248 (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     m_PaperDollMeshWidget;                            		// 0x024C (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActor_EquipScreen*            m_PaperDoll;                                      		// 0x0250 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TutorialMainPanel;                              		// 0x0254 (0x0004) [0x0000000000000000]              
	struct FsTutTypePanels                             m_TutPanels[ 0x6 ];                               		// 0x0258 (0x0060) [0x0000000000000000]              
	int                                                m_CurrentPage;                                    		// 0x02B8 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentType;                                    		// 0x02BC (0x0004) [0x0000000000000000]              
	int                                                m_CurrentFilter;                                  		// 0x02C0 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentSort;                                    		// 0x02C4 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_IsDirty : 1;                                    		// 0x02C8 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bRefresh : 1;                                   		// 0x02C8 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bTutorialMode : 1;                              		// 0x02C8 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bShouldHandleChildScenePopup : 1;               		// 0x02C8 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned char                                      m_eRepairConfirmType;                             		// 0x02CC (0x0001) [0x0000000000000000]              
	unsigned char                                      m_eTutorialStep;                                  		// 0x02CD (0x0001) [0x0000000000000000]              
	unsigned char                                      m_eTutorialType;                                  		// 0x02CE (0x0001) [0x0000000000000000]              
	class UTgInventoryObject*                          m_SelectedInvObj;                                 		// 0x02D0 (0x0004) [0x0000000000000000]              
	int                                                m_nPendingModInventoryId;                         		// 0x02D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1670 ];

		return pClassPointer;
	};

	void eventSelectTab ( unsigned char Tab );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventSetEquippedItemRepairClick ( class UUIScreenObject* Obj );
	void eventSetRepairButtonMouseOver ( class UUIScreenObject* Obj );
	void eventSetEquippedItemStateChange ( class UUIScreenObject* Obj );
	void eventSetEquippedItemSelect ( class UUIScreenObject* Obj );
	void eventSetDeviceListStateChange ( class UUIScreenObject* Obj );
	void eventSetDeviceListSelect ( class UUIScreenObject* Obj );
	void eventSetInvItemSelect ( class UUIScreenObject* Obj );
	void eventSetInvItemStateChange ( class UUIScreenObject* Obj );
	void eventFixupWidgetsUC ( );
	void PostRender ( );
	bool CheckTutorialDisabled ( unsigned char Step );
	bool CheckTutorialStep ( unsigned char Step );
	void UpdateTutorialProgress ( unsigned char Step, int NewTutorialType );
	void BeginTutorial ( unsigned char TutType );
	void OnPaperDollUpdated ( );
	void UpdatePaperDoll ( unsigned long bInit );
	void DestroyPaperDoll ( );
	void SetupPaperDoll ( );
	bool OnConfirmOkClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void OnEquippedItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnEquippedItemSelect ( struct FInputEventParameters* EventParms );
	bool OnInvItemSelect ( struct FInputEventParameters* EventParms );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnDeviceListSelect ( struct FInputEventParameters* EventParms );
	void OnDeviceListStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnRepairButtonMouseOver ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnRepairButtonClick ( struct FInputEventParameters* EventParms );
	bool OnPrevDevicePageButton ( struct FInputEventParameters* EventParms );
	bool OnNextDevicePageButton ( struct FInputEventParameters* EventParms );
	bool OnSelectAppearanceTab ( struct FInputEventParameters* EventParms );
	bool OnSelectArmorTab ( struct FInputEventParameters* EventParms );
	bool OnSelectDeviceTab ( struct FInputEventParameters* EventParms );
	bool OnAgentMenuKey ( struct FInputEventParameters* EventParms );
	bool IsReadOnlyMode ( );
	void PreCloseScene ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAgentProfile_Inventory
// 0x03EC (0x05E0 - 0x01F4)
class UTgUIAgentProfile_Inventory : public UTgUIAgentProfile
{
public:
	class UUIPanel*                                    m_InvListPanel;                                   		// 0x01F4 (0x0004) [0x0000000000000000]              
	struct FsInvListItem                               m_InventoryItems[ 0x12 ];                         		// 0x01F8 (0x0318) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_LocalInventoryStore;                            		// 0x0510 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ValidTypeList;                                  		// 0x051C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_SelectedItem;                                   		// 0x0528 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentCredits;                                 		// 0x052C (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TypeComboBox;                                   		// 0x0530 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x0534 (0x0004) [0x0000000000000000]              
	int                                                m_TypeFilter;                                     		// 0x0538 (0x0004) [0x0000000000000000]              
	int                                                m_QualityFilter;                                  		// 0x053C (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_ItemTypeStringList;                             		// 0x0540 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ItemTypeCodeList;                               		// 0x054C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x0558 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_QualityCodeList;                                		// 0x0564 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   m_DeleteButton;                                   		// 0x0570 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SalvageButton;                                  		// 0x0574 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ActivateButton;                                 		// 0x0578 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ActivateLabel;                                  		// 0x057C (0x0004) [0x0000000000000000]              
	int                                                m_nSortType;                                      		// 0x0580 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bSortAscending : 1;                             		// 0x0584 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bRefresh : 1;                                   		// 0x0584 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bShowProgressBar : 1;                           		// 0x0584 (0x0004) [0x0000000000000000] [0x00000004] 
	class UUIButton*                                   m_HeaderName;                                     		// 0x0588 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderQuantity;                                 		// 0x058C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderType;                                     		// 0x0590 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderGeneration;                               		// 0x0594 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x0598 (0x0018) [0x0000000000000000]              
	class UUIPanel*                                    m_SalvagePanel;                                   		// 0x05B0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_SalvageProgBar;                                 		// 0x05B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SalvageCloseButton;                             		// 0x05B8 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_SalvagedItemList;                               		// 0x05BC (0x0004) [0x0000000000000000]              
	TArray< struct FsSalvagedItem >                    m_SalvagedItems;                                  		// 0x05C0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_SalvagedInvObject;                              		// 0x05CC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	float                                              m_fSalvageStartTime;                              		// 0x05D8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SalvageContinueButton;                          		// 0x05DC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1671 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetEquipIconStateChange ( class UUIScreenObject* Obj );
	void eventSetSalvagedItemStateChange ( class UUIScreenObject* Obj );
	void eventSetInvMainStateChange ( class UUIScreenObject* Obj );
	void eventSetInvItemStateChange ( class UUIScreenObject* Obj );
	void eventSetSelectInventoryItemDelegate ( class UUIScreenObject* Obj );
	void ResizeSalvagedDisplayList ( int nNewSize );
	void OnEquipIconStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnSalvagedItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnInvMainStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnSalvageCloseDelegate ( struct FInputEventParameters* EventParms );
	bool OnActivateInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnSortInventoryDelegate ( struct FInputEventParameters* EventParms );
	bool OnSalvageInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnDeleteInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	void OnFilterChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void SelectInventoryItem ( int ItemIndex );
	void PopulateInventoryItems ( );
	void BuildLocalInventoryStore ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
	void PreCloseScene ( );
};


// Class TgClient.TgUIAgentProfile_Skill
// 0x00A4 (0x0298 - 0x01F4)
class UTgUIAgentProfile_Skill : public UTgUIAgentProfile
{
public:
	struct FSSkillTreeInfo                             m_SkillTreeInfoData[ 0x3 ];                       		// 0x01F4 (0x006C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FSSkillTreePanel >                  m_SkillTreeControls;                              		// 0x0260 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   m_ResetButton;                                    		// 0x026C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ResetAllButton;                                 		// 0x0270 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SaveButton;                                     		// 0x0274 (0x0004) [0x0000000000000000]              
	class ATgPlayerController*                         m_PC;                                             		// 0x0278 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrentLevel;                                  		// 0x027C (0x0004) [0x0000000000000000]              
	int                                                m_nTotalPoints;                                   		// 0x0280 (0x0004) [0x0000000000000000]              
	int                                                m_nTotalPointsRemaining;                          		// 0x0284 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_IsDirty : 1;                                    		// 0x0288 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bReceivedData : 1;                              		// 0x0288 (0x0004) [0x0000000000000000] [0x00000002] 
	class UMaterial*                                   m_IconMaterialParent;                             		// 0x028C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PointsTotalLabel;                               		// 0x0290 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FeedbackLabel;                                  		// 0x0294 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1672 ];

		return pClassPointer;
	};

	void eventRequestPlayersSkillsFromServer ( );
	void eventFixupWidgetsUC ( );
	bool ValidateRespecSkillFeature ( );
	void OnSkillButtonStateChangeDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnSkillButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnResetAllDelegate ( struct FInputEventParameters* EventParms );
	bool OnResetDelegate ( struct FInputEventParameters* EventParms );
	bool OnSaveDelegate ( struct FInputEventParameters* EventParms );
	bool IsReadOnlyMode ( );
	void PostOpenScene ( );
	void PreCloseScene ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAgentProfile_Stat
// 0x0435 (0x0629 - 0x01F4)
class UTgUIAgentProfile_Stat : public UTgUIAgentProfile
{
public:
	class UUIImage*                                    m_ClassIcon;                                      		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DragButton;                                     		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DrawPanel;                                      		// 0x01FC (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     m_PaperDollMeshWidget;                            		// 0x0200 (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActor_CharacterBuilder*       m_PaperDoll;                                      		// 0x0204 (0x0004) [0x0000000000000000]              
	int                                                m_MaleShoulderMeshId;                             		// 0x0208 (0x0004) [0x0000000000000000]              
	int                                                m_FemaleShoulderMeshId;                           		// 0x020C (0x0004) [0x0000000000000000]              
	struct FsStatTab                                   m_SubTab[ 0x2 ];                                  		// 0x0210 (0x0018) [0x0000000000000000]              
	struct FsStatTab                                   m_IconTab[ 0x2 ];                                 		// 0x0228 (0x0018) [0x0000000000000000]              
	int                                                m_CurrentTab;                                     		// 0x0240 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentIconTab;                                 		// 0x0244 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LevelLabel;                                     		// 0x0248 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_XpLabel;                                        		// 0x024C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_XpProgImage;                                    		// 0x0250 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NextLevelLabel;                                 		// 0x0254 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AboutLabel;                                     		// 0x0258 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditLabel;                                    		// 0x025C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TokenLabel;                                     		// 0x0260 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HZPointLabel;                                   		// 0x0264 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AchievementBadgeLabel;                          		// 0x0268 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BoostPanel;                                     		// 0x026C (0x0004) [0x0000000000000000]              
	struct FsBoostInfo                                 m_BoostInfo[ 0x4 ];                               		// 0x0270 (0x0030) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_XpProgMIC;                                      		// 0x02A0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_SkillStarMICParent;                             		// 0x02A4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_SkillStarMIC[ 0x5 ];                            		// 0x02A8 (0x0014) [0x0000000000000000]              
	class UUIImage*                                    m_SkillStarImage[ 0x5 ];                          		// 0x02BC (0x0014) [0x0000000000000000]              
	class UUIImage*                                    m_SkillStarTexture;                               		// 0x02D0 (0x0004) [0x0000000000000000]              
	struct FCraftingSkillStruct                        m_CraftingControls[ 0x6 ];                        		// 0x02D4 (0x00A8) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_CraftingProgMICParent;                          		// 0x037C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PrevButton;                                     		// 0x0380 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NextButton;                                     		// 0x0384 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PagingLabel;                                    		// 0x0388 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pCraftingMarshal;                               		// 0x038C (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FPointer                                    m_pAchievementMarshal;                            		// 0x0390 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FFlairSetStruct                             m_FlairSets[ 0x2 ];                               		// 0x0394 (0x00B0) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UTgInventoryObject*                          m_SelectedHeadFlair;                              		// 0x0444 (0x0004) [0x0000000000000000]              
	struct FBadgeSetStruct                             m_BadgeSets[ 0x2 ];                               		// 0x0448 (0x01E0) [0x0000000000400000]              ( CPF_NeedCtorLink )
	unsigned char                                      m_eDollState;                                     		// 0x0628 (0x0001) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1673 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void OnBadgeIconStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnFlairIconStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnUnequipFlairClicked ( struct FInputEventParameters* EventParms );
	bool OnFlairIconClicked ( struct FInputEventParameters* EventParms );
	void OnPaperDollUpdated ( );
	void DestroyPaperDoll ( );
	void SetupPaperDoll ( );
	bool OnNextBadgeButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnPrevBadgeButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnNextButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnPrevButtonClicked ( struct FInputEventParameters* EventParms );
	bool OnSelectIconTab ( struct FInputEventParameters* EventParms );
	bool OnSelectSubTab ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void PostOpenScene ( );
	void PreCloseScene ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUIAuctionGeneral
// 0x0088 (0x0204 - 0x017C)
class UTgUIAuctionGeneral : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_TabBids;                                        		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TabBrowse;                                      		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TabAddAuctions;                                 		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TabAuctions;                                    		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BuyoutButton;                                   		// 0x0190 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x0194 (0x0018) [0x0000000000000000]              
	unsigned char                                      m_eCurrentMode;                                   		// 0x01AC (0x0001) [0x0000000000000000]              
	unsigned char                                      m_eAuctionMode;                                   		// 0x01AD (0x0001) [0x0000000000000000]              
	float                                              m_fUpdateTimer;                                   		// 0x01B0 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_PlayerPawn;                                     		// 0x01B4 (0x0004) [0x0000000000000000]              
	struct FsAuctionItemStruct                         m_ItemSelected;                                   		// 0x01B8 (0x004C) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1674 ];

		return pClassPointer;
	};

	bool ValidateAuctionFeature ( );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void OpenBuyoutConfirmPopup ( struct FsAuctionItemStruct* sItemSelected );
	bool OnAuctionItemSelect ( struct FInputEventParameters* EventParms );
	bool OnBuyoutButton ( struct FInputEventParameters* EventParms );
	bool OnTabButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIAuctionBidMenu
// 0x0058 (0x025C - 0x0204)
class UTgUIAuctionBidMenu : public UTgUIAuctionGeneral
{
public:
	class UTgUIScrollList*                             m_BidAndAuctionsList;                             		// 0x0204 (0x0004) [0x0000000000000000]              
	TArray< struct FsAuctionItemStruct >               m_BidAndAuctionMembers;                           		// 0x0208 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nSelectionIndex;                                		// 0x0214 (0x0004) [0x0000000000000000]              
	struct FsAuctionBidTab                             m_AuctionBidTab[ 0x2 ];                           		// 0x0218 (0x0018) [0x0000000000000000]              
	class UUIPanel*                                    m_BidPanel;                                       		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BidCreditsLabel;                                		// 0x0234 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_BidEditBox;                                     		// 0x0238 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RebidButton;                                    		// 0x023C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AuctionPanel;                                   		// 0x0240 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AuctionCreditsLabel;                            		// 0x0244 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelAuctionButton;                            		// 0x0248 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentBidCredits;                              		// 0x024C (0x0004) [0x0000000000000000]              
	int                                                m_CurrentAuctionCredits;                          		// 0x0250 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pMyBidsData;                                    		// 0x0254 (0x0004) [0x0000000000001000]              ( CPF_Native )
	struct FPointer                                    m_pMyAuctionsData;                                		// 0x0258 (0x0004) [0x0000000000001000]              ( CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1675 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetAuctionItemSelect ( class UUIScreenObject* Obj );
	void eventSetAuctionItemStateChange ( class UUIScreenObject* Obj );
	void OnAuctionTimeStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnAuctionItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnAuctionItemSelect ( struct FInputEventParameters* EventParms );
	bool OnCancelAuctionButton ( struct FInputEventParameters* EventParms );
	bool OnBidButton ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyPressed ( struct FInputEventParameters* EventParms );
	void UpdateHighlight ( );
	void UpdateBidListDisplay ( );
	void ResizeDisplayList ( int newSize );
	bool TickTgUIScene ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIAuctionBrowseMenu
// 0x00D0 (0x02D4 - 0x0204)
class UTgUIAuctionBrowseMenu : public UTgUIAuctionGeneral
{
public:
	class UUILabel*                                    m_CreditsLabel;                                   		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_NameEditBox;                                    		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x020C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SearchButton;                                   		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_LowestPriceOnly;                                		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BidOptionPanel;                                 		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_BidEntryBox;                                    		// 0x021C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BidButton;                                      		// 0x0220 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BidBGPanel;                                     		// 0x0224 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BidPagingLeft;                                  		// 0x0228 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BidPagingCenter;                                		// 0x022C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_BidPagingRight;                                 		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BidEntryLabel;                                  		// 0x0234 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedTypeCode;                              		// 0x0238 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedSubTypeCode;                           		// 0x023C (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_TypeScrollList;                                 		// 0x0240 (0x0004) [0x0000000000000000]              
	TArray< struct FTypeCategoryTemplate >             m_TypeCategoryDisplayList;                        		// 0x0244 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FTypeCategoryTreeNode >             m_TypeCategoryTree;                               		// 0x0250 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_CategoryArchetypeUsage;                         		// 0x025C (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	unsigned long                                      m_bCategoryExpanded : 1;                          		// 0x0268 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bUpdateCategoryList : 1;                        		// 0x0268 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bLowestPriceOnly : 1;                           		// 0x0268 (0x0004) [0x0000000000000000] [0x00000004] 
	class UTgUIScrollList*                             m_BrowseList;                                     		// 0x026C (0x0004) [0x0000000000000000]              
	TArray< struct FsAuctionItemStruct >               m_BrowseMembers;                                  		// 0x0270 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nSelectionIndex;                                		// 0x027C (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_GroupedBrowseList;                              		// 0x0280 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_BrowsePanel;                                    		// 0x0284 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_GroupedBrowsePanel;                             		// 0x0288 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentCredits;                                 		// 0x028C (0x0004) [0x0000000000000000]              
	struct FsAuctionItemFilter                         m_AuctionItemFilter;                              		// 0x0290 (0x0018) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FPointer                                    m_pAuctionBrowseData;                             		// 0x02A8 (0x0004) [0x0000000000001000]              ( CPF_Native )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x02AC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_QualityCodeList;                                		// 0x02B8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ValidTypeList;                                  		// 0x02C4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UTgUIPreviewScreen*                          m_PreviewScene;                                   		// 0x02D0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1676 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetAuctionTimeStateChange ( class UUIScreenObject* Obj );
	void eventSetAuctionItemSelect ( class UUIScreenObject* Obj );
	void eventSetAuctionItemStateChange ( class UUIScreenObject* Obj );
	void eventSetRawInputKeyForType ( class UUIButton* Button );
	void OnNotifyEditBoxStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnLowestPriceOnlyChanged ( class UUIObject* ebox, int PlayerIndex );
	bool OnTypeCategoryClicked ( struct FInputEventParameters* EventParms );
	void OnAuctionTimeStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnAuctionItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnAuctionItemSelect ( struct FInputEventParameters* EventParms );
	bool OnBidButton ( struct FInputEventParameters* EventParms );
	bool OnSearchButton ( struct FInputEventParameters* EventParms );
	bool OnResetButton ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyPressed ( struct FInputEventParameters* EventParms );
	void UpdateHighlight ( );
	void UpdateBrowseListDisplay ( );
	void ResizeDisplayList ( int newSize );
	bool TickTgUIScene ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIAuctionSellMenu
// 0x025C (0x0460 - 0x0204)
class UTgUIAuctionSellMenu : public UTgUIAuctionGeneral
{
public:
	TArray< class UTgInventoryObject* >                m_LocalInventoryList;                             		// 0x0204 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FsAuctionInvItem                            m_InventoryList[ 0x12 ];                          		// 0x0210 (0x01B0) [0x0000000000000000]              
	int                                                m_nInventoryIndex;                                		// 0x03C0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_YourCreditsLabel;                               		// 0x03C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeleteButton;                                   		// 0x03C8 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x03CC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TypeComboBox;                                   		// 0x03D0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditsLabel;                                   		// 0x03D4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ListPanel;                                      		// 0x03D8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SellOptionsPanel;                               		// 0x03DC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_QuantityEditBox;                                		// 0x03E0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PriceLabel;                                     		// 0x03E4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_PriceEditBox;                                   		// 0x03E8 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TimeComboBox;                                   		// 0x03EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PriceTooltip;                                   		// 0x03F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SellButton;                                     		// 0x03F4 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentCredits;                                 		// 0x03F8 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_DurationStringList;                             		// 0x03FC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_DurationCodeList;                               		// 0x0408 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x0414 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_QualityCodeList;                                		// 0x0420 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_ItemTypeStringList;                             		// 0x042C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ItemTypeCodeList;                               		// 0x0438 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ValidTypeList;                                  		// 0x0444 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nSortType;                                      		// 0x0450 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bSortAscending : 1;                             		// 0x0454 (0x0004) [0x0000000000000000] [0x00000001] 
	class UUIButton*                                   m_HeaderName;                                     		// 0x0458 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderQuantity;                                 		// 0x045C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1677 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void RegisterCallbacks ( );
	void OnQualityComboChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnTypeComboChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnSortInventoryDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnSellButton ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyPressed ( struct FInputEventParameters* EventParms );
	void UpdateInventoryHighlight ( );
	bool TickTgUIScene ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUICampaignMenu
// 0x001C (0x0198 - 0x017C)
class UTgUICampaignMenu : public UTgUISceneDriver_Tooltip
{
public:
	int                                                m_nCachedHexUpdateCount;                          		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0180 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_CurrentTab;                                     		// 0x0184 (0x0001) [0x0000000000000000]              
	class UUIButton*                                   m_HexMapTab;                                      		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CombatTab;                                      		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CombatLogTab;                                   		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_LeaderboardTab;                                 		// 0x0194 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1678 ];

		return pClassPointer;
	};

	void eventSelectTab ( unsigned char Tab );
	bool OnSelectLeaderboardTab ( struct FInputEventParameters* EventParms );
	bool OnSelectCombatLogTab ( struct FInputEventParameters* EventParms );
	bool OnSelectCombatTab ( struct FInputEventParameters* EventParms );
	bool OnSelectHexMapTab ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void OnHexAssignmentUpdated ( );
	void OnHexAssignmentChanged ( );
	class UTgAgencyData* GetAgencyData ( );
	bool OnHexDataUpdated ( );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	void RegisterCallbacks ( );
	bool TickTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUICampaignMenu_Combat
// 0x0068 (0x0200 - 0x0198)
class UTgUICampaignMenu_Combat : public UTgUICampaignMenu
{
public:
	class UTgUIScrollList*                             m_DefenderScrollList;                             		// 0x0198 (0x0004) [0x0000000000000000]              
	TArray< struct FsCombatAlertDefender >             m_DefenderListMembers;                            		// 0x019C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nDefenderSelectedIndex;                         		// 0x01A8 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_eDefenderSortType;                              		// 0x01AC (0x0001) [0x0000000000000000]              
	unsigned char                                      m_eAttackerSortType;                              		// 0x01AD (0x0001) [0x0000000000000000]              
	class UUICheckbox*                                 m_DefenderShowFull;                               		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderJoinButton;                             		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderKickTeamButton;                         		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderSortFacility;                           		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderSortAttacker;                           		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderSortZone;                               		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderSortPlayers;                            		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DefenderSortTime;                               		// 0x01CC (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_AttackerScrollList;                             		// 0x01D0 (0x0004) [0x0000000000000000]              
	TArray< struct FsCombatAlertAttacker >             m_AttackerListMembers;                            		// 0x01D4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nAttackerSelectedIndex;                         		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_AttackerShowFull;                               		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttackerJoinButton;                             		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttackerSortFacility;                           		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttackerSortDefender;                           		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttackerSortZone;                               		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttackerSortPlayers;                            		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttackerSortStatus;                             		// 0x01FC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1679 ];

		return pClassPointer;
	};

	void eventSetSelectAttackerDelegate ( class UUIScreenObject* Obj );
	void eventSetSelectDefenderDelegate ( class UUIScreenObject* Obj );
	void eventFixupWidgetsUC ( );
	bool OnAttackerSetSort ( struct FInputEventParameters* EventParms );
	bool OnAttackerJoin ( struct FInputEventParameters* EventParms );
	bool OnSelectAttacker ( struct FInputEventParameters* EventParms );
	void OnAttackerFilterChanged ( class UUIObject* Sender, int PlayerIndex );
	void AttackerUpdateHighlight ( );
	void SelectAttacker ( int nIndex );
	void ResizeAttackerList ( int nSize );
	void UpdateAttackerList ( );
	bool OnDefenderSetSort ( struct FInputEventParameters* EventParms );
	bool OnDefenderKickTeam ( struct FInputEventParameters* EventParms );
	bool OnDefenderJoin ( struct FInputEventParameters* EventParms );
	bool OnSelectDefender ( struct FInputEventParameters* EventParms );
	void OnDefenderFilterChanged ( class UUIObject* Sender, int PlayerIndex );
	void DefenderUpdateHighlight ( );
	void SelectDefender ( int nIndex );
	void ResizeDefenderList ( int nSize );
	void UpdateDefenderTimers ( );
	void UpdateDefenderList ( );
	void RegisterCallbacks ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
	bool TickTgUIScene ( );
	bool OnACADataUpdated ( );
};


// Class TgClient.TgUICampaignMenu_CombatLog
// 0x0050 (0x01E8 - 0x0198)
class UTgUICampaignMenu_CombatLog : public UTgUICampaignMenu
{
public:
	TArray< struct FCombatLogStruct >                  m_LogMembers;                                     		// 0x0198 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class ATgPlayerController*                         m_PC;                                             		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_LogScrollList;                                  		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ResultsPanel;                                   		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ZoneComboBox;                                   		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SearchPanel;                                    		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SearchButton;                                   		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ResetButton;                                    		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyEntryField;                               		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_OutcomeComboBox;                                		// 0x01C4 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_OutcomeStringList;                              		// 0x01C8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_OutcomeCodeList;                                		// 0x01D4 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UUIEditBox*                                  m_DateStartField;                                 		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_DateEndField;                                   		// 0x01E4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1680 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnResetButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSearchButtonDelegate ( struct FInputEventParameters* EventParms );
	void ResizeLogList ( int nSize );
	void ResetFilters ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUICampaignMenu_HexMap
// 0x010C (0x02A4 - 0x0198)
class UTgUICampaignMenu_HexMap : public UTgUICampaignMenu
{
public:
	unsigned long                                      m_bDragMode : 1;                                  		// 0x0198 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bZoneChangeDisable : 1;                         		// 0x0198 (0x0004) [0x0000000000000000] [0x00000002] 
	struct FVector                                     m_DragMousePosition;                              		// 0x019C (0x000C) [0x0000000000000000]              
	float                                              m_fZoomDelta;                                     		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_AllianceIconParentMIC;                          		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ZoneListComboBox;                               		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ZoneNameLabel;                                  		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ZoneMaintenanceTimeHeader;                      		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ZoneMaintenanceTimeLabel;                       		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ZoneEndTimeHeader;                              		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ZoneEndTimeLabel;                               		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ForceTimeHeader;                                		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ForceTimeLabel;                                 		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_OpenTerritories;                                		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_FilterPanel;                                    		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowCombatCheckBox;                             		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowStrategicCheckBox;                          		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowAllianceCheckBox;                           		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowClosedCheckBox;                             		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsPanel;                                		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsDefendPanel;                          		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HexOptionsDefendButton;                         		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsDefendTeammatesLabel;                 		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsDefendBidTimeLabel;                   		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsJoinPanel;                            		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HexOptionsJoinButton;                           		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HexOptionsJoinWithdrawButton;                   		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsBidPanel;                             		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HexOptionsBidButton;                            		// 0x020C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsBidFeedbackLabel;                     		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsCancelBidPanel;                       		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HexOptionsCancelBidButton;                      		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsCancelBidFeedbackLabel;               		// 0x021C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsMIPPanel;                             		// 0x0220 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsMIPAttackerLabel;                     		// 0x0224 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsMIPDefenderLabel;                     		// 0x0228 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsLockdownPanel;                        		// 0x022C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexOptionsLockdownTimeLabel;                    		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexOptionsShieldProgressPanel;                  		// 0x0234 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_HexOptionsShieldProgessImage;                   		// 0x0238 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_HexOptionsShieldProgressMIC;                    		// 0x023C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexTimePanel;                                   		// 0x0240 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexTimeHeaderLabel;                             		// 0x0244 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexTimeValueLabel;                              		// 0x0248 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexInfoPanel;                                   		// 0x024C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexInfoHeaderLabel;                             		// 0x0250 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexInfoAgencyLabel;                             		// 0x0254 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexInfoAllianceLabel;                           		// 0x0258 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexInfoRegionLabel;                             		// 0x025C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexInfoBonusPanel;                              		// 0x0260 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_HexInfoBonusLabel;                              		// 0x0264 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_FacilityInfoPanel;                              		// 0x0268 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FacilityNameLabel;                              		// 0x026C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FacilityIcon;                                   		// 0x0270 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_NavigationPanel;                                		// 0x0274 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ZoomInButton;                                   		// 0x0278 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ZoomOutButton;                                  		// 0x027C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PanButtons[ 0x4 ];                              		// 0x0280 (0x0010) [0x0000000000000000]              
	class UUIPanel*                                    m_PopupMessagePanel;                              		// 0x0290 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PopupMessageLabel;                              		// 0x0294 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PopupMessageButton;                             		// 0x0298 (0x0004) [0x0000000000000000]              
	class UTgUIHexWidget_Zone*                         m_pHexWidget;                                     		// 0x029C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_ProgressBarParent;                              		// 0x02A0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1681 ];

		return pClassPointer;
	};

	bool OnPopupButton ( struct FInputEventParameters* EventParms );
	bool OnOpenTerritories ( struct FInputEventParameters* EventParms );
	bool OnHexWithdraw ( struct FInputEventParameters* EventParms );
	bool OnHexRaid ( struct FInputEventParameters* EventParms );
	bool OnHexSteal ( struct FInputEventParameters* EventParms );
	bool OnHexCapture ( struct FInputEventParameters* EventParms );
	bool OnHexJoin ( struct FInputEventParameters* EventParms );
	bool OnHexDefend ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	bool OnNavButton ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnHexBidCancel ( struct FInputEventParameters* EventParms );
	bool OnHexBid ( struct FInputEventParameters* EventParms );
	void OnZoneChanged ( class UUIObject* Sender, int PlayerIndex );
	void UpdateHexOptionDisplay ( );
	void UpdateHexInformation ( );
	void UpdateZoneList ( );
	void UpdateToolTipData ( );
	bool UpdateHexMap ( );
	void OnHexAssignmentUpdated ( );
	bool OnHexDataUpdated ( );
	void OpenTerritories ( );
	void HexWithdraw ( );
	void HexRaid ( );
	void HexSteal ( );
	void HexCapture ( );
	void HexJoin ( );
	void HexDefend ( );
	void ClosePopupMessage ( );
	void OpenPopupMessage ( int nMsgId );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	void PostRender ( );
	void RegisterCallbacks ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUICampaignMenu_Leaderboard
// 0x02D0 (0x0468 - 0x0198)
class UTgUICampaignMenu_Leaderboard : public UTgUICampaignMenu
{
public:
	struct FPointer                                    m_pCurrentData;                                   		// 0x0198 (0x0004) [0x0000000000001000]              ( CPF_Native )
	class UUIPanel*                                    m_LeaderboardPanel;                               		// 0x019C (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x01A0 (0x0018) [0x0000000000000000]              
	struct FAgencyStatsStruct                          m_AgencyStatsMembers[ 0x13 ];                     		// 0x01B8 (0x02AC) [0x0000000000000000]              
	class UUIEditBox*                                  m_AgencyEntryField;                               		// 0x0464 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1682 ];

		return pClassPointer;
	};

	void eventSetRowMouseoverDelegate ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	void OnAgencyRowStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnAgencyValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void RegisterCallbacks ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUICargoMenu
// 0x016C (0x02E8 - 0x017C)
class UTgUICargoMenu : public UTgUISceneDriver_Tooltip
{
public:
	class UUIPanel*                                    m_SFSelectPanel;                                  		// 0x017C (0x0004) [0x0000000000000000]              
	struct FsSFSelectStruct                            m_SFSelect[ 0x6 ];                                		// 0x0180 (0x0048) [0x0000000000000000]              
	int                                                m_nSelectedSF;                                    		// 0x01C8 (0x0004) [0x0000000000000000]              
	struct FsTabButtonStruct                           m_TabStruct[ 0x4 ];                               		// 0x01CC (0x0060) [0x0000000000000000]              
	TArray< struct FsSFItem >                          m_SFInventoryList;                                		// 0x022C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsSFItem >                          m_RaidInventoryList;                              		// 0x0238 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIButton*                                   m_wCloseButton;                                   		// 0x0244 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x0248 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wPromoteButton;                                 		// 0x024C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wLeaveButton;                                   		// 0x0250 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wAddCargoButton;                                		// 0x0254 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveCargoButton;                             		// 0x0258 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRaidButton;                                    		// 0x025C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wHeaderLabel;                                   		// 0x0260 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RaidTimePanel;                                  		// 0x0264 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeLabel;                                  		// 0x0268 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeHeader;                                 		// 0x026C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AmountPanel;                                    		// 0x0270 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AmountEditBox;                                  		// 0x0274 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CargoHeaderLabel;                               		// 0x0278 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CargoTotalLabel;                                		// 0x027C (0x0004) [0x0000000000000000]              
	int                                                m_nTotalCargoSize;                                		// 0x0280 (0x0004) [0x0000000000000000]              
	struct FsDropshipItem                              m_DropshipItem;                                   		// 0x0284 (0x0030) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MemberListScrollFrame;                          		// 0x02B4 (0x0004) [0x0000000000000000]              
	TArray< struct FsTeamMemberStruct >                m_MemberList;                                     		// 0x02B8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UTgUIScrollList*                             m_CargoListScrollFrame;                           		// 0x02C4 (0x0004) [0x0000000000000000]              
	TArray< struct FsCargoItem >                       m_CargoList;                                      		// 0x02C8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nTeamUpdateSeq;                                 		// 0x02D4 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedMember;                                		// 0x02D8 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedCargo;                                 		// 0x02DC (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bOpenRefresh : 1;                               		// 0x02E0 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPlayerIsLeader : 1;                            		// 0x02E0 (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                m_nOwningAgencyId;                                		// 0x02E4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1683 ];

		return pClassPointer;
	};

	void eventSetCargoItemStateChange ( class UUIScreenObject* Obj );
	void eventSetDropshipItemStateChange ( class UUIScreenObject* Obj );
	void eventSetRawInputKeyForCargoList ( class UUIButton* Button );
	void eventSetRawInputKeyForMemberList ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	bool SelectCargoByButton ( class UUIButton* Button );
	bool SelectMemberByButton ( class UUIButton* Button );
	void OnAmountChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	void OnCargoItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnDropshipItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnCargoRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnCargoAddDelegate ( struct FInputEventParameters* EventParms );
	bool OnDropshipRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnDropshipAddDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectSFTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneMapDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneManageDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneDetailDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRaidDelegate ( struct FInputEventParameters* EventParms );
	bool OnCargoListDelegate ( struct FInputEventParameters* EventParms );
	bool OnTeamMemberDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneLeaveDelegate ( struct FInputEventParameters* EventParms );
	bool OnScenePromoteDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void RegisterCallbacks ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUICharacterSelectScene
// 0x08F0 (0x0A6C - 0x017C)
class UTgUICharacterSelectScene : public UTgUISceneDriver_Tooltip
{
public:
	class UTgDataSet*                                  c_DataSet;                                        		// 0x017C (0x0004) [0x0000000000000000]              
	class UTgDataInterface*                            c_DataInterface;                                  		// 0x0180 (0x0004) [0x0000000000000000]              
	int                                                c_nCurrentSelection;                              		// 0x0184 (0x0004) [0x0000000000000000]              
	int                                                c_nNumCharacters;                                 		// 0x0188 (0x0004) [0x0000000000000000]              
	struct FCustomCharacterAssembly                    c_CharacterList[ 0x8 ];                           		// 0x018C (0x0220) [0x0000000000000000]              
	struct FDeviceAttachmentAssembly                   c_CharacterDeviceList[ 0x8 ];                     		// 0x03AC (0x00E0) [0x0000000000000000]              
	int                                                c_CharacterIdList[ 0x8 ];                         		// 0x048C (0x0020) [0x0000000000000000]              
	int                                                c_CharacterProfileList[ 0x8 ];                    		// 0x04AC (0x0020) [0x0000000000000000]              
	int                                                c_CharacterLevelList[ 0x8 ];                      		// 0x04CC (0x0020) [0x0000000000000000]              
	struct FString                                     c_sCharacterNameList[ 0x8 ];                      		// 0x04EC (0x0060) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FCharacterSelectEntry                       c_CharacterListButton[ 0x8 ];                     		// 0x054C (0x00C0) [0x0000000000000000]              
	struct FCharacterDeleteBox                         c_CharacterDeleteDialogue;                        		// 0x060C (0x001C) [0x0000000000000000]              
	struct FCharacterErrorBox                          c_CharacterErrorDialogue;                         		// 0x0628 (0x0008) [0x0000000000000000]              
	class UUIButton*                                   c_SelectButton;                                   		// 0x0630 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   c_CreateButton;                                   		// 0x0634 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   c_DeleteButton;                                   		// 0x0638 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DragButton;                                     		// 0x063C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   c_BackButton;                                     		// 0x0640 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    c_ClassIconImage;                                 		// 0x0644 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RotateBox;                                      		// 0x0648 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RotateCCWButton;                                		// 0x064C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RotateCWButton;                                 		// 0x0650 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    c_StatusMessageBox;                               		// 0x0654 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    c_PlayerNameLabel;                                		// 0x0658 (0x0004) [0x0000000000000000]              
	unsigned char                                      c_SelectState;                                    		// 0x065C (0x0001) [0x0000000000000000]              
	unsigned char                                      c_SelectStatePrevious;                            		// 0x065D (0x0001) [0x0000000000000000]              
	class ATgSkeletalMeshActor_CharacterBuilder*       m_CustomModel;                                    		// 0x0660 (0x0004) [0x0000000000000000]              
	struct FPointer                                    c_pMorphSettings;                                 		// 0x0664 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                c_nCurrMorphSettings[ 0xFF ];                     		// 0x0668 (0x03FC) [0x0000000000000000]              
	int                                                c_nMaxMorphIndexSentFromServer;                   		// 0x0A64 (0x0004) [0x0000000000000000]              
	unsigned long                                      c_bErrorMode : 1;                                 		// 0x0A68 (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1684 ];

		return pClassPointer;
	};

	void eventClearModelData ( );
	bool eventUpdateCustomModel ( );
	bool eventSelectCharacter ( int Index );
	void OpenCreateCharacterScreen ( );
	bool OnDeleteCancel ( struct FInputEventParameters* EventParms );
	bool OnErrorOK ( struct FInputEventParameters* EventParms );
	bool OnDeleteOK ( struct FInputEventParameters* EventParms );
	bool CamZoomToggleButton ( struct FInputEventParameters* EventParms );
	bool OnDeleteSelection ( struct FInputEventParameters* EventParms );
	bool OnCreateSelection ( struct FInputEventParameters* EventParms );
	bool OnSelectSelection ( struct FInputEventParameters* EventParms );
	bool OnBackButton ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter7 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter6 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter5 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter4 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter3 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter2 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter1 ( struct FInputEventParameters* EventParms );
	bool OnSelectCharacter0 ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void eventInitializeDataInterface ( );
	bool OnSelectNext ( struct FInputEventParameters* EventParms );
	bool OnSelectPrevious ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool ValidateCharSlotFeature ( );
	void OnDeleteFieldChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	void PopulateCurrentMorphSettings ( );
	void ZoomOnChar ( unsigned long bZoomIn );
	void UpdateStatusPanel ( unsigned long bEnable, struct FString DisplayString );
	void UpdateCharacterListCallback ( int eType );
	void RegisterCallbacks ( );
	void TickDialogueBoxes ( );
	bool TickTgUIScene ( );
	void ClearCharacterData ( );
	void ConfirmDeleteCharacter ( );
	void ConfirmSelectCharacter ( );
	void UpdateShowHideWidgets ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUICraftScene
// 0x00E8 (0x0264 - 0x017C)
class UTgUICraftScene : public UTgUISceneDriver_Tooltip
{
public:
	class ATgPawn_Character*                           m_PlayerPawn;                                     		// 0x017C (0x0004) [0x0000000000000000]              
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x0180 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bRefresh : 1;                                   		// 0x0184 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bReadOnlyMode : 1;                              		// 0x0184 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bSortAscending : 1;                             		// 0x0184 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bShowProgress : 1;                              		// 0x0184 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bToggleOffProgressPanel : 1;                    		// 0x0184 (0x0004) [0x0000000000000000] [0x00000010] 
	class UTgUIScrollList*                             m_CraftScrollFrame;                               		// 0x0188 (0x0004) [0x0000000000000000]              
	TArray< struct FsCraftItemStruct >                 m_CraftItems;                                     		// 0x018C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_LocalCraftStore;                                		// 0x0198 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_ComponentPanel;                                 		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_ComponentScrollFrame;                           		// 0x01A8 (0x0004) [0x0000000000000000]              
	TArray< struct FsCraftComponentStruct >            m_ComponentItems;                                 		// 0x01AC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_LocalComponentStore;                            		// 0x01B8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_DetailPanel;                                    		// 0x01C4 (0x0004) [0x0000000000000000]              
	struct FsBPHeaderStruct                            m_DetailHeader;                                   		// 0x01C8 (0x0014) [0x0000000000000000]              
	class UTgUIScrollList*                             m_BPModScrollFrame;                               		// 0x01DC (0x0004) [0x0000000000000000]              
	TArray< struct FsBPModCategoryStruct >             m_BPModList;                                      		// 0x01E0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_BPModArchetypeUsage;                            		// 0x01EC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_CraftProgPanel;                                 		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_CraftItem;                                      		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemName;                                       		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_CraftProgBar;                                   		// 0x0204 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedItem;                                  		// 0x0208 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedCategory;                              		// 0x020C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CraftButton;                                    		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ReadOnlyPanel;                                  		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x021C (0x0004) [0x0000000000000000]              
	int                                                m_QualityFilter;                                  		// 0x0220 (0x0004) [0x0000000000000000]              
	TArray< int >                                      m_QualityCodeList;                                		// 0x0224 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x0230 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nSortType;                                      		// 0x023C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderName;                                     		// 0x0240 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderGeneration;                               		// 0x0244 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x0248 (0x0018) [0x0000000000000000]              
	float                                              m_fCraftStartTime;                                		// 0x0260 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1685 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetHoverCraftItemDelegate ( class UUIScreenObject* Obj );
	void eventSetSelectCraftItemDelegate ( class UUIScreenObject* Obj );
	bool ValidateCraftingFeature ( );
	bool OnPrevPage ( struct FInputEventParameters* EventParms );
	bool OnNextPage ( struct FInputEventParameters* EventParms );
	void OnCraftItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnFilterChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnCraftButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSortInventoryDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectCraftItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void ResizeCraftDetailsWidgets ( int newSize );
	void ResizeComponentPanelWidgets ( int newSize );
	void SelectCraftItem ( int ItemIndex );
	void PopulateCraftDetails ( );
	void PopulateCraftItems ( );
	void ResizeBlueprintPanelWidgets ( int newSize );
	void BuildLocalCraftStore ( );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUICustomHeadMenu
// 0x01AC (0x0328 - 0x017C)
class UTgUICustomHeadMenu : public UTgUISceneDriver_Tooltip
{
public:
	class ATgSkeletalMeshActor_CharacterBuilder*       m_CustomModel;                                    		// 0x017C (0x0004) [0x0000000000000000]              
	TArray< class UMorphNodeBase* >                    m_MorphNodes;                                     		// 0x0180 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsCustomHeadGroupStruct >           m_NodeData;                                       		// 0x018C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_BasicPanel;                                     		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DetailPanel;                                    		// 0x019C (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MorphScrollFrame;                               		// 0x01A0 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentMorphGroup;                              		// 0x01A4 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_DetailMode : 1;                                 		// 0x01A8 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPlayTutorial : 1;                              		// 0x01A8 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bCreateLock : 1;                                		// 0x01A8 (0x0004) [0x0000000000000000] [0x00000004] 
	class UUIButton*                                   m_RandomButton;                                   		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_NextButton;                                     		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BackButton;                                     		// 0x01B4 (0x0004) [0x0000000000000000]              
	struct FsCustomHeadPresetStruct                    m_GroupPreset[ 0x8 ];                             		// 0x01B8 (0x0140) [0x0000000000000000]              
	class UTgMorphNodeBlendList_ByHairStyle*           m_HairStyleMorphNode;                             		// 0x02F8 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_FacialHairMorphNode;                            		// 0x02FC (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_EyeColorMorphNode;                              		// 0x0300 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BlockerButton;                                  		// 0x0304 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrentGender;                                 		// 0x0308 (0x0004) [0x0000000000000000]              
	int                                                m_nCurrentProfile;                                		// 0x030C (0x0004) [0x0000000000000000]              
	int                                                m_nHairSelection;                                 		// 0x0310 (0x0004) [0x0000000000000000]              
	float                                              m_fCreateTimeout;                                 		// 0x0314 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_CreateHeadVerifyState;                          		// 0x0318 (0x0001) [0x0000000000000000]              
	TArray< unsigned char >                            m_nMorphSettings;                                 		// 0x031C (0x000C) [0x0000000000402000]              ( CPF_Transient | CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1686 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventZoomIn ( );
	void eventSetDetailButtonMouseOverDelegate ( class UUIButton* Button );
	void eventSetDetailButtonDelegate ( class UUIButton* Button );
	void eventSetLockButtonDelegate ( class UUIButton* Button );
	void eventSetGroupSliderValueChangeDelegate ( class UUISlider* slider );
	void eventSetSliderValueChangeDelegate ( class UUISlider* slider );
	void eventGoToCharacterSelect ( );
	void eventSendSections ( );
	void eventSetCreateHeadVerifyState ( unsigned char stateIn );
	void SaveMorphSettings ( );
	void OnDetailButtonMouseOverDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool CreateHeadNextButton ( struct FInputEventParameters* EventParms );
	bool OnDetailButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnLockButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnRandomAllButton ( struct FInputEventParameters* EventParms );
	bool OnBackButton ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void OnGroupSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OpenTutorialConfirmPopup ( );
	void PopulateMorphSliders ( );
	void BuildNodeData ( );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIDeviceModPopup
// 0x002C (0x01A8 - 0x017C)
class UTgUIDeviceModPopup : public UTgUISceneDriver_Tooltip
{
public:
	class UUIPanel*                                    m_PopupPanel;                                     		// 0x017C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ConfirmText;                                    		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ModLabel;                                       		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_DeviceLabel;                                    		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FeedbackLabel;                                  		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmYes;                                     		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ConfirmNo;                                      		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ModItemButton;                                  		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeviceItemButton;                               		// 0x019C (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_pModInv;                                        		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_pDeviceInv;                                     		// 0x01A4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1687 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnConfirmNo ( struct FInputEventParameters* EventParms );
	bool OnConfirmYes ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIDeviceStats
// 0x015C (0x02D8 - 0x017C)
class UTgUIDeviceStats : public UTgUISceneDriver_Tooltip
{
public:
	struct FDeviceFields                               m_DeviceRows[ 0x8 ];                              		// 0x017C (0x00E0) [0x0000000000000000]              
	struct FsDeviceItemStruct                          m_DeviceItems[ 0x8 ];                             		// 0x025C (0x0040) [0x0000000000000000]              
	class UUILabelButton*                              m_HeaderButtons[ 0x7 ];                           		// 0x029C (0x001C) [0x0000000000000000]              
	unsigned long                                      m_bOpenedByBaseMenu : 1;                          		// 0x02B8 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_GameOver : 1;                                   		// 0x02B8 (0x0004) [0x0000000000000000] [0x00000002] 
	class UTgStatsData*                                m_StatsData;                                      		// 0x02BC (0x0004) [0x0000000000000000]              
	float                                              m_fLastCopy;                                      		// 0x02C0 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_eCurrentSortField;                              		// 0x02C4 (0x0001) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x02C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SummaryButton;                                  		// 0x02CC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MatchStatsButton;                               		// 0x02D0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EndMissionButton;                               		// 0x02D4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1688 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnEndMissionButton ( struct FInputEventParameters* EventParms );
	bool OnMatchStatsButton ( struct FInputEventParameters* EventParms );
	bool OnSummaryButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void OnDeviceItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnFieldClicked ( struct FInputEventParameters* EventParms );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	void PostInitSceneDriver ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIEndMissionScreen
// 0x0144 (0x02C0 - 0x017C)
class UTgUIEndMissionScreen : public UTgUISceneDriver_Tooltip
{
public:
	class UUISafeRegionPanel*                          m_ScenePanel;                                     		// 0x017C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ResultLabel;                                    		// 0x0180 (0x0004) [0x0000000000000000]              
	struct FsBasicRewardPanel                          m_BasicRewards;                                   		// 0x0184 (0x0040) [0x0000000000000000]              
	struct FsAchievementPanel                          m_Achievements;                                   		// 0x01C4 (0x0024) [0x0000000000000000]              
	struct FsBonusRewardPanel                          m_BonusRewards[ 0x2 ];                            		// 0x01E8 (0x00A0) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_AvlMics[ 0x2 ];                                 		// 0x0288 (0x0008) [0x0000000000000000]              
	unsigned long                                      m_bHasCenteredMouse : 1;                          		// 0x0290 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bHintToExpectLoot : 1;                          		// 0x0290 (0x0004) [0x0000000000000000] [0x00000002] 
	class UMaterial*                                   m_RandomLootMat;                                  		// 0x0294 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfLastRandom;                              		// 0x0298 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeBetweenRandom;                             		// 0x029C (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfFirstLoad;                               		// 0x02A0 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_byCachedCompleteFlag;                           		// 0x02A4 (0x0001) [0x0000000000000000]              
	int                                                m_nImpendingWebsite;                              		// 0x02A8 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_LootRevealCue;                                  		// 0x02AC (0x0004) [0x0000000000000000]              
	class UTgSpecialFx*                                m_LootRingupFx;                                   		// 0x02B0 (0x0004) [0x0000000000000000]              
	int                                                m_nRolling;                                       		// 0x02B4 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pXpMarshal;                                     		// 0x02B8 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )
	struct FPointer                                    m_pDeferredMarshal;                               		// 0x02BC (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1689 ];

		return pClassPointer;
	};

	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnUpgradeButton ( struct FInputEventParameters* EventParms );
	bool OnEndMissionButton ( struct FInputEventParameters* EventParms );
	bool OnDeviceStatsButton ( struct FInputEventParameters* EventParms );
	bool OnMatchStatsButton ( struct FInputEventParameters* EventParms );
	void OnLootItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void BeginFadeIn ( );
	void SetLabelTextAndColor ( unsigned long MissionVictor, unsigned long PlayerVictor, unsigned long bTie );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIEndQuestMission
// 0x000C (0x0188 - 0x017C)
class UTgUIEndQuestMission : public UTgUISceneDriver_Tooltip
{
public:
	class UUISafeRegionPanel*                          m_ScenePanel;                                     		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EndMissionButton;                               		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ResultsLabel;                                   		// 0x0184 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1690 ];

		return pClassPointer;
	};

	bool OnEndMissionButton ( struct FInputEventParameters* EventParms );
	void BeginFadeIn ( );
	void SetLabelTextAndColor ( unsigned long MissionVictor, unsigned long PlayerVictor, unsigned long bTie );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIGameHelpItems
// 0x0038 (0x01B4 - 0x017C)
class UTgUIGameHelpItems : public UTgUISceneDriver_Tooltip
{
public:
	unsigned char                                      m_ActiveGameHelpItem;                             		// 0x017C (0x0001) [0x0000000000000000]              
	float                                              m_TimeOpened;                                     		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SmallFrame;                                     		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SmallFrameExit;                                 		// 0x0188 (0x0004) [0x0000000000000000]              
	struct FsTutItemInfo                               m_GameHelpItems[ 0x5 ];                           		// 0x018C (0x0028) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1691 ];

		return pClassPointer;
	};

	bool OnSceneDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIInventoryPopup
// 0x0270 (0x03EC - 0x017C)
class UTgUIInventoryPopup : public UTgUISceneDriver_Tooltip
{
public:
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x017C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bCloseOnAdd : 1;                                		// 0x0180 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIncProductionBPs : 1;                          		// 0x0180 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bRefresh : 1;                                   		// 0x0180 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bExcludeBoundInventory : 1;                     		// 0x0180 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bSingleMode : 1;                                		// 0x0180 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_bSortAscending : 1;                             		// 0x0180 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bBlockKeyInput : 1;                             		// 0x0180 (0x0004) [0x0000000000000000] [0x00000040] 
	struct FsInventoryPopupItem                        m_InventoryList[ 0x12 ];                          		// 0x0184 (0x01B0) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_InventoryItems;                                 		// 0x0334 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_TypeFilter;                                     		// 0x0340 (0x0004) [0x0000000000000000]              
	TArray< int >                                      m_SubTypeFilter;                                  		// 0x0344 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ValidTypeList;                                  		// 0x0350 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ExcludedTypeList;                               		// 0x035C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_SelectedItem;                                   		// 0x0368 (0x0004) [0x0000000000000000]              
	class ATgPawn*                                     m_PlayerPawn;                                     		// 0x036C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0370 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AddButton;                                      		// 0x0374 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeleteButton;                                   		// 0x0378 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BlockTypeButton;                                		// 0x037C (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TypeComboBox;                                   		// 0x0380 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x0384 (0x0004) [0x0000000000000000]              
	int                                                m_QualityFilter;                                  		// 0x0388 (0x0004) [0x0000000000000000]              
	int                                                m_SkillFilter;                                    		// 0x038C (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_ItemTypeStringList;                             		// 0x0390 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ItemTypeCodeList;                               		// 0x039C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x03A8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_QualityCodeList;                                		// 0x03B4 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FPointer                                    m_pExcludedInventory;                             		// 0x03C0 (0x0004) [0x0000000000001000]              ( CPF_Native )
	int                                                m_nExcludedSubtype;                               		// 0x03C4 (0x0004) [0x0000000000000000]              
	int                                                m_nSortType;                                      		// 0x03C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderName;                                     		// 0x03CC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderQuantity;                                 		// 0x03D0 (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x03D4 (0x0018) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1692 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnPrevPage ( struct FInputEventParameters* EventParms );
	bool OnNextPage ( struct FInputEventParameters* EventParms );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnSortInventoryDelegate ( struct FInputEventParameters* EventParms );
	void OnFilterChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnDeleteInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnBlockTypeDelegate ( struct FInputEventParameters* EventParms );
	bool OnAddButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void FixupTgUIScene ( );
	bool TickTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUIProductionPopup
// 0x000C (0x03F8 - 0x03EC)
class UTgUIProductionPopup : public UTgUIInventoryPopup
{
public:
	int                                                m_nMaximumTechLevelValueId;                       		// 0x03EC (0x0004) [0x0000000000000000]              
	int                                                m_nZoneObjectId;                                  		// 0x03F0 (0x0004) [0x0000000000000000]              
	int                                                m_nPosition;                                      		// 0x03F4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1693 ];

		return pClassPointer;
	};

	bool OnAddButtonDelegate ( struct FInputEventParameters* EventParms );
	void SetHex ( int nZoneObjectId, int nPosition );
	void SetMaximumTechLevel ( int nTechLevelValueId );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIItemModification
// 0x007C (0x01F8 - 0x017C)
class UTgUIItemModification : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x017C (0x0004) [0x0000000000000000]              
	struct FPagingStruct                               m_DevicePaging;                                   		// 0x0180 (0x000C) [0x0000000000000000]              
	struct FPagingStruct                               m_KitPaging;                                      		// 0x018C (0x000C) [0x0000000000000000]              
	class UUIButton*                                   m_CombineButton;                                  		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KitHeaderLabel;                                 		// 0x019C (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_FilterComboBox;                                 		// 0x01A0 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_ItemTypeStringList;                             		// 0x01A4 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ItemTypeCodeList;                               		// 0x01B0 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FExtendedItemModStruct >            m_DeviceItems;                                    		// 0x01BC (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FBaseItemModStruct >                m_KitItems;                                       		// 0x01C8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_CurrentPage;                                    		// 0x01D4 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentKitPage;                                 		// 0x01D8 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentType;                                    		// 0x01DC (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_SelectedDevice;                                 		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_SelectedKit;                                    		// 0x01E4 (0x0004) [0x0000000000000000]              
	int                                                m_nModPendingInventoryId;                         		// 0x01E8 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bRefresh : 1;                                   		// 0x01EC (0x0004) [0x0000000000000000] [0x00000001] 
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x01F0 (0x0004) [0x0000000000000000]              
	class ATgPawn_Character*                           m_PlayerPawn;                                     		// 0x01F4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1694 ];

		return pClassPointer;
	};

	void eventSetOnKitMouseOver ( class UUIScreenObject* Obj );
	void eventSetOnKitClick ( class UUIScreenObject* Obj );
	void eventSetOnKitItemClick ( class UUIScreenObject* Obj );
	void eventSetOnKitItemMouseOver ( class UUIScreenObject* Obj );
	void eventSetOnDeviceListMouseOver ( class UUIScreenObject* Obj );
	void eventSetOnDeviceListClick ( class UUIScreenObject* Obj );
	void eventSetOnDeviceItemClick ( class UUIScreenObject* Obj );
	void eventSetOnDeviceItemMouseOver ( class UUIScreenObject* Obj );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void OnFilterChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnKitClick ( struct FInputEventParameters* EventParms );
	void OnKitMouseOver ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnKitItemClick ( struct FInputEventParameters* EventParms );
	void OnKitItemMouseOver ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnDeviceListClick ( struct FInputEventParameters* EventParms );
	void OnDeviceListMouseOver ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnDeviceItemClick ( struct FInputEventParameters* EventParms );
	void OnDeviceItemMouseOver ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnPrevKitPageClick ( struct FInputEventParameters* EventParms );
	bool OnNextKitPageClick ( struct FInputEventParameters* EventParms );
	bool OnPrevDevicePageClick ( struct FInputEventParameters* EventParms );
	bool OnNextDevicePageClick ( struct FInputEventParameters* EventParms );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnCombineClick ( struct FInputEventParameters* EventParms );
	bool OnItemModMenuKey ( struct FInputEventParameters* EventParms );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PostOpenScene ( );
	bool TickTgUIScene ( );
};


// Class TgClient.TgUILFGMenu
// 0x00BC (0x0238 - 0x017C)
class UTgUILFGMenu : public UTgUISceneDriver_Tooltip
{
public:
	struct FsTabButtonStruct                           m_TabStruct[ 0x4 ];                               		// 0x017C (0x0060) [0x0000000000000000]              
	class UUIButton*                                   m_wCloseButton;                                   		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wPromoteButton;                                 		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wLeaveButton;                                   		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wStrikeForceButton;                             		// 0x01EC (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MemberListScrollFrame;                          		// 0x01F0 (0x0004) [0x0000000000000000]              
	TArray< struct FsTeamMemberStruct >                m_MemberList;                                     		// 0x01F4 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPanel*                                    m_RaidTimePanel;                                  		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeLabel;                                  		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeHeader;                                 		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wHeaderLabel;                                   		// 0x020C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_wAgencyCheckbox;                                		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_wAllianceCheckbox;                              		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_wMercenaryCheckbox;                             		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wFeedbackLabel;                                 		// 0x021C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wSubmitButton;                                  		// 0x0220 (0x0004) [0x0000000000000000]              
	int                                                m_nTeamUpdateSeq;                                 		// 0x0224 (0x0004) [0x0000000000000000]              
	float                                              m_InviteTimer;                                    		// 0x0228 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedMember;                                		// 0x022C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bOpenRefresh : 1;                               		// 0x0230 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPlayerIsLeader : 1;                            		// 0x0230 (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                m_nOwningAgencyId;                                		// 0x0234 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1695 ];

		return pClassPointer;
	};

	void eventSetRawInputKeyForMemberList ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	void OnLFGCriteriaChanged ( class UUIObject* Sender, int PlayerIndex );
	bool SelectMemberByButton ( class UUIButton* Button );
	bool OnSceneCargoDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneDetailDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneStrikeForceDelegate ( struct FInputEventParameters* EventParms );
	bool OnTeamMemberDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneLeaveDelegate ( struct FInputEventParameters* EventParms );
	bool OnScenePromoteDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUILoginMenu
// 0x00A8 (0x0224 - 0x017C)
class UTgUILoginMenu : public UTgUISceneDriver_Tooltip
{
public:
	unsigned char                                      m_ProcessingState;                                		// 0x017C (0x0001) [0x0000000000000000]              
	unsigned long                                      m_bCheckForAuto : 1;                              		// 0x0180 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bCreatingPlayerName : 1;                        		// 0x0180 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_SaveAccountInfo : 1;                            		// 0x0180 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bHasProcessedAnnouncement : 1;                  		// 0x0180 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bShowStatus : 1;                                		// 0x0180 (0x0004) [0x0000000000044000] [0x00000010] ( CPF_Config | CPF_GlobalConfig )
	unsigned long                                      m_bCrawlActive : 1;                               		// 0x0180 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bCrawlLinkActive : 1;                           		// 0x0180 (0x0004) [0x0000000000000000] [0x00000040] 
	struct FsLoginStatusPanel                          m_StatusPanel;                                    		// 0x0184 (0x001C) [0x0000000000000000]              
	class UUILabelButton*                              m_LoginButton;                                    		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_QuitButton;                                     		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_CreditsButton;                                  		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_SettingsButton;                                 		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_NameButton;                                     		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AccountEditBox;                                 		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_PasswordEditBox;                                		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_NameEditBox;                                    		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_SaveAcctCheckbox;                               		// 0x01C0 (0x0004) [0x0000000000000000]              
	int                                                m_TriggeredAccountLogin;                          		// 0x01C4 (0x0004) [0x0000000000000000]              
	int                                                m_TriggerCharacterCreate;                         		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Version;                                        		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_LoginPanel;                                     		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_NamePanel;                                      		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AnnouncementPanel;                              		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AnnouncementLabel;                              		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_Weblink;                                        		// 0x01E0 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfLogin;                                   		// 0x01E4 (0x0004) [0x0000000000000000]              
	int                                                m_nTimeBetweenUpdates;                            		// 0x01E8 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	class UUIPanel*                                    m_CrawlPanel;                                     		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CrawlLink;                                      		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CrawlText;                                      		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIScrollFrame*                              m_CrawlScrollFrame;                               		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TwitterButton;                                  		// 0x01FC (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfLastCrawlTextUpdate;                     		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_QueuePanel;                                     		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ServerFullLabel;                                		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PosLabel;                                       		// 0x020C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_TimeLabel;                                      		// 0x0210 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_DisconnectButton;                               		// 0x0214 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_QueueButton;                                    		// 0x0218 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BlockerButton;                                  		// 0x021C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0220 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1696 ];

		return pClassPointer;
	};

	bool OnWeblinkKey ( struct FInputEventParameters* EventParms );
	void OnNotifyPasswordEditBoxStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnNotifyAccountEditBoxStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnLoginSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnSetFocusToPasswordEditBox ( struct FInputEventParameters* EventParms );
	bool OnSetFocusToAccountEditBox ( struct FInputEventParameters* EventParms );
	bool OnEditBoxInputKey ( struct FInputEventParameters* EventParms );
	bool OnCheckboxKey ( struct FInputEventParameters* EventParms );
	bool OnSettingsCreditsButtonKey ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonKey ( struct FInputEventParameters* EventParms );
	bool OnDisconnectButtonKey ( struct FInputEventParameters* EventParms );
	bool OnQueueButtonKey ( struct FInputEventParameters* EventParms );
	bool OnQuitButtonKey ( struct FInputEventParameters* EventParms );
	bool OnLoginButtonKey ( struct FInputEventParameters* EventParms );
	bool OnStatusCancelButtonKey ( struct FInputEventParameters* EventParms );
	bool OnStatusOKButtonKey ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void TickVisuals ( float DeltaTime );
	bool OnCrawlScrollOverride ( class UUIScreenObject* Sender, float Nudges, unsigned long bMaxedOut );
	bool OnCrawlButtonClick ( struct FInputEventParameters* EventParms );
	void StartWebLink ( );
	void OnCreateCharacter ( );
	void HandleStatusCancel ( );
	void HandleStatusOK ( );
	void UpdateSavedLoginInfo ( unsigned long bLog );
	void UpdateStatusPanel ( unsigned long bEnable, struct FString DisplayString, unsigned long bFadeOut );
	void TickStatusPanel ( );
	void TriggerCharCreate ( );
	void TriggerLogin ( );
	bool IsProcessing ( );
	unsigned char GetProcessingState ( );
	void SetProcessingState ( unsigned char NewState );
	void CloseQueuePanel ( );
	void ProcessQueueButton ( );
	void ProcessDisconnect ( );
	void ToggleSaveAccountInfo ( );
	void ProcessQuit ( );
	void GetSavedAccountInfo ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
};


// Class TgClient.TgUIMailMenu
// 0x022C (0x03A8 - 0x017C)
class UTgUIMailMenu : public UTgUISceneDriver_Tooltip
{
public:
	TArray< struct FsAttachmentInfo >                  m_InvAttachments;                                 		// 0x017C (0x000C) [0x0000000000402000]              ( CPF_Transient | CPF_NeedCtorLink )
	int                                                m_ToPlayerId;                                     		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BlockerButton;                                  		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MessagesListPanel;                              		// 0x0194 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MessageList;                                    		// 0x0198 (0x0004) [0x0000000000000000]              
	TArray< struct FsMailItem >                        m_MailItems;                                      		// 0x019C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x01A8 (0x0018) [0x0000000000000000]              
	class UUIPanel*                                    m_MessagesDetailPanel;                            		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HeaderPanel;                                    		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_NameLabel;                                      		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_SubjectLabel;                                   		// 0x01CC (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_DetailList;                                     		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MessageBody;                                    		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DetailAttachmentsPanel;                         		// 0x01D8 (0x0004) [0x0000000000000000]              
	struct FsMailAttachment                            m_DetailAttachments[ 0x5 ];                       		// 0x01DC (0x00A0) [0x0000000000000000]              
	class UUIPanel*                                    m_MessageOptionsPanel;                            		// 0x027C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeleteButton;                                   		// 0x0280 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ReturnButton;                                   		// 0x0284 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ReplyButton;                                    		// 0x0288 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TakeButton;                                     		// 0x028C (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_ToEditBox;                                      		// 0x0290 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_SubjectEditBox;                                 		// 0x0294 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_EnterEditBox;                                   		// 0x0298 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_MessageLabel;                                   		// 0x029C (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MessageScrollList;                              		// 0x02A0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_FeedbackLabel;                                  		// 0x02A4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelButton;                                   		// 0x02A8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SendButton;                                     		// 0x02AC (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AttachmentsPanel;                               		// 0x02B0 (0x0004) [0x0000000000000000]              
	struct FsMailAttachment                            m_Attachments[ 0x4 ];                             		// 0x02B4 (0x0080) [0x0000000000000000]              
	class UUIPanel*                                    m_AttachmentOptionsPanel;                         		// 0x0334 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_AttachButton;                                   		// 0x0338 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RemoveButton;                                   		// 0x033C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_AmountToSendPanel;                              		// 0x0340 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_AmountEditBox;                                  		// 0x0344 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_CODCheckBox;                                    		// 0x0348 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_SendMoneyCheckBox;                              		// 0x034C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bToPlayerValidated : 1;                         		// 0x0350 (0x0004) [0x0000000000000000] [0x00000001] 
	struct FsMailTab                                   m_MailTab[ 0x2 ];                                 		// 0x0354 (0x0018) [0x0000000000000000]              
	int                                                m_CurrentTab;                                     		// 0x036C (0x0004) [0x0000000000000000]              
	int                                                m_nSelectionIndex;                                		// 0x0370 (0x0004) [0x0000000000000000]              
	int                                                m_nAttachmentSelectionIndex;                      		// 0x0374 (0x0004) [0x0000000000000000]              
	int                                                m_nDetailAttachmentSelectionIndex;                		// 0x0378 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pMailInboxData;                                 		// 0x037C (0x0004) [0x0000000000001000]              ( CPF_Native )
	class ATgPawn*                                     m_PlayerPawn;                                     		// 0x0380 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_texMsgMessage;                                  		// 0x0384 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_texMsgItem;                                     		// 0x0388 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_texMsgCredit;                                   		// 0x038C (0x0004) [0x0000000000000000]              
	struct FName                                       m_ButtonStyleResolverTag;                         		// 0x0390 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ButtonActiveStyleName;                          		// 0x0398 (0x0008) [0x0000000000000000]              
	struct FName                                       m_ButtonInactiveStyleName;                        		// 0x03A0 (0x0008) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1697 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetDetailAttachmentStateChange ( class UUIScreenObject* Obj );
	void eventSetAttachmentStateChange ( class UUIScreenObject* Obj );
	void eventSetDetailAttachmentSelect ( class UUIScreenObject* Obj );
	void eventSetAttachmentSelect ( class UUIScreenObject* Obj );
	void eventSetInboxItemSelect ( class UUIScreenObject* Obj );
	void eventSetMailItemSelect ( class UUIScreenObject* Obj );
	bool ValidateMailFeature ( );
	bool SetFocusedWidget ( class UUIScreenObject* NewFocus );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void OpenCODConfirmPopup ( int nCurrencyValue );
	void SelectMailTab ( int SelectTab );
	void UpdateAttachmentHighlight ( );
	void UpdateHighlight ( );
	void ResizeDisplayList ( int newSize );
	bool TickTgUIScene ( );
	void FixupWidgets ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	void PostInitSceneDriver ( );
	void OnSendMoneyCheckboxChanged ( class UUIObject* ebox, int PlayerIndex );
	void OnCODCheckboxChanged ( class UUIObject* ebox, int PlayerIndex );
	bool OnToSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	void OnToValueChanged ( class UUIObject* ebox, int PlayerIndex );
	void OnEnterValueChanged ( class UUIObject* ebox, int PlayerIndex );
	void OnDetailAttachmentStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnAttachmentStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnMailItemSelect ( struct FInputEventParameters* EventParms );
	bool OnDetailAttachmentSelect ( struct FInputEventParameters* EventParms );
	bool OnAttachmentSelect ( struct FInputEventParameters* EventParms );
	bool OnInboxItemSelect ( struct FInputEventParameters* EventParms );
	bool OnSelectMailTab ( struct FInputEventParameters* EventParms );
	bool OnSendButton ( struct FInputEventParameters* EventParms );
	bool OnCancelButton ( struct FInputEventParameters* EventParms );
	bool OnRemoveButton ( struct FInputEventParameters* EventParms );
	bool OnAttachButton ( struct FInputEventParameters* EventParms );
	bool OnReplyButton ( struct FInputEventParameters* EventParms );
	bool OnTakeButton ( struct FInputEventParameters* EventParms );
	bool OnReturnButton ( struct FInputEventParameters* EventParms );
	bool OnDeleteButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnMailMenuKey ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIMainMenu
// 0x0110 (0x028C - 0x017C)
class UTgUIMainMenu : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_EliteAgentButton[ 0x2 ];                        		// 0x017C (0x0008) [0x0000000000000000]              
	class UUIButton*                                   m_BoosterButton[ 0x2 ];                           		// 0x0184 (0x0008) [0x0000000000000000]              
	class UUIButton*                                   m_StoreButton[ 0x2 ];                             		// 0x018C (0x0008) [0x0000000000000000]              
	class UUIButton*                                   m_DisconnectButton;                               		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_QuitButton;                                     		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_MenuCloseButton;                                		// 0x019C (0x0004) [0x0000000000000000]              
	int                                                m_nNumButtons;                                    		// 0x01A0 (0x0004) [0x0000000000000000]              
	struct FsMainMenuButton                            m_VarButtons[ 0xA ];                              		// 0x01A4 (0x00A0) [0x0000000000000000]              
	unsigned long                                      m_bQuitMode : 1;                                  		// 0x0244 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bOpenedByVolume : 1;                            		// 0x0244 (0x0004) [0x0000000000000000] [0x00000002] 
	class UUIPanel*                                    m_QuitConfirmPanel;                               		// 0x0248 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_QuitConfirmYes;                                 		// 0x024C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_QuitConfirmNo;                                  		// 0x0250 (0x0004) [0x0000000000000000]              
	TArray< struct FName >                             m_ManagedSceneNames;                              		// 0x0254 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_ManagedSceneKeys;                               		// 0x0260 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FsContextItemData >                 m_ContextItems;                                   		// 0x026C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FName                                       m_OpenChildSceneName;                             		// 0x0278 (0x0008) [0x0000000000000000]              
	int                                                m_OpenChildSceneID;                               		// 0x0280 (0x0004) [0x0000000000000000]              
	float                                              m_MenuThrottleTimer;                              		// 0x0284 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeSinceBoostCheck;                           		// 0x0288 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1698 ];

		return pClassPointer;
	};

	void ExitQuitMode ( );
	void EnterQuitMode ( );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void SetUpMenuButtons ( );
	void AddContextItem ( int funcID );
	void ConfigureContextItems ( );
	int CountContextItems ( );
	int GetButtonFunctionID ( class UUIScreenObject* buttonObject );
	int GetSceneReferenceByClassName ( struct FName ClassName );
	unsigned char GetBaseFunctionID ( unsigned char funcID );
	int GetOverrideSceneReference ( unsigned char funcID );
	void CloseManagedScene ( unsigned long bPreserveVolumeFlag );
	class UTgUISceneDriver* GetOpenManagedScene ( );
	class UTgUISceneDriver* OpenManagedSceneByName ( struct FName ClassName, unsigned long bToggle, unsigned long bDevForce );
	class UTgUISceneDriver* OpenManagedScene ( int sceneReference, unsigned long bToggle, unsigned long bDevForce, unsigned long bIgnoreOverride );
	void BuildManagedSceneList ( );
	void OnButtonMouseOverDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnMainMenuExitMissionKey ( struct FInputEventParameters* EventParms );
	bool OnMainMenuQuitKey ( struct FInputEventParameters* EventParms );
	bool OnFixedPurposeButton ( struct FInputEventParameters* EventParms );
	bool OnMainMenuButtonKey ( struct FInputEventParameters* EventParms );
	bool OnMainMenuSceneKey ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
	void PreCloseScene ( );
};


// Class TgClient.TgUIMissionScene
// 0x08A0 (0x0A1C - 0x017C)
class UTgUIMissionScene : public UTgUISceneDriver_Tooltip
{
public:
	struct FGameTypeButtonStruct                       m_GameTypes[ 0x6 ];                               		// 0x017C (0x0150) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nSelectedGameType;                              		// 0x02CC (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedQueueIndex;                            		// 0x02D0 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bPlayerInQueue : 1;                             		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsLeaderOrSolo : 1;                            		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bPvEUseTeam : 1;                                		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bDisableJoin : 1;                               		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bPurchased : 1;                                 		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_bSubscribed : 1;                                		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bIsConquestTokenLevel : 1;                      		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000040] 
	unsigned long                                      m_bLastInTeam : 1;                                		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000080] 
	unsigned long                                      m_bClouds : 1;                                    		// 0x02D4 (0x0004) [0x0000000000000000] [0x00000100] 
	class UUIImage*                                    m_MapImage;                                       		// 0x02D8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_ScenarioPanel;                                  		// 0x02DC (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ScenarioLocked;                                 		// 0x02E0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioPlayerLabel[ 0x4 ];                     		// 0x02E4 (0x0010) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioPlayerCount[ 0x4 ];                     		// 0x02F4 (0x0010) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioActiveLabel;                            		// 0x0304 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioActiveCount;                            		// 0x0308 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioName;                                   		// 0x030C (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ScenarioImage;                                  		// 0x0310 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ScenarioFill;                                   		// 0x0314 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioDescription;                            		// 0x0318 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ScenarioJoin;                                   		// 0x031C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ScenarioJoinLabel;                              		// 0x0320 (0x0004) [0x0000000000000000]              
	struct FMissionRowStruct                           m_MissionRows[ 0xA ];                             		// 0x0324 (0x02A8) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FMissionRowStruct                           m_DifficultyRows[ 0x6 ];                          		// 0x05CC (0x0198) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FMissionRowStruct                           m_MissionRowsPvE[ 0x6 ];                          		// 0x0764 (0x0198) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                m_nCurrentPage;                                   		// 0x08FC (0x0004) [0x0000000000000000]              
	float                                              m_fLastSecondUpdate;                              		// 0x0900 (0x0004) [0x0000000000000000]              
	int                                                m_nHoverQueueIndex;                               		// 0x0904 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_IconTextures[ 0xA ];                            		// 0x0908 (0x0028) [0x0000000000000002]              ( CPF_Const )
	class UTexture2D*                                  m_CurrentQueueTexture;                            		// 0x0930 (0x0004) [0x0000000000000002]              ( CPF_Const )
	class UUIButton*                                   m_SoloButton;                                     		// 0x0934 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_TeamButton;                                     		// 0x0938 (0x0004) [0x0000000000000000]              
	int                                                m_nDifficulty;                                    		// 0x093C (0x0004) [0x0000000000000000]              
	int                                                m_nHoverDifficultyIndex;                          		// 0x0940 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_GlobalMapPanel;                                 		// 0x0944 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MissionListPanel;                               		// 0x0948 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_MissionListPanelPvE;                            		// 0x094C (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_MissionListPanelPvEPaging;                      		// 0x0950 (0x0018) [0x0000000000000000]              
	class UUIButton*                                   m_LeaveButton;                                    		// 0x0968 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x096C (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_JoinSoundCue;                                   		// 0x0970 (0x0004) [0x0000000000000000]              
	class USoundCue*                                   m_LeaveSoundCue;                                  		// 0x0974 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ZoneTab;                                        		// 0x0978 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_GlobeTab;                                       		// 0x097C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TabPanel;                                       		// 0x0980 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_TabBackground;                                  		// 0x0984 (0x0004) [0x0000000000000000]              
	struct FName                                       m_nmBriefingMovie;                                		// 0x0988 (0x0008) [0x0000000000000000]              
	class ATgPlayerController*                         m_PC;                                             		// 0x0990 (0x0004) [0x0000000000000000]              
	int                                                m_nMyLevel;                                       		// 0x0994 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ControlCheckBox;                                		// 0x0998 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_BreachCheckBox;                                 		// 0x099C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_DemolitionCheckBox;                             		// 0x09A0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_PayloadCheckBox;                                		// 0x09A4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ScrambleCheckBox;                               		// 0x09A8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_BalancedCheckBox;                               		// 0x09AC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_FullTeamCheckBox;                               		// 0x09B0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_NoTeamCheckBox;                                 		// 0x09B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BalancedButton;                                 		// 0x09B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_FullTeamButton;                                 		// 0x09BC (0x0004) [0x0000000000000000]              
	int                                                m_nControlSelected;                               		// 0x09C0 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nBreachSelected;                                		// 0x09C4 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nDemolitionSelected;                            		// 0x09C8 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nPayloadSelected;                               		// 0x09CC (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nScrambleSelected;                              		// 0x09D0 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nBalancedSelected;                              		// 0x09D4 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	int                                                m_nFullTeamSelected;                              		// 0x09D8 (0x0004) [0x0000000000044000]              ( CPF_Config | CPF_GlobalConfig )
	class USkeletalMeshComponent*                      GlobeMeshComp;                                    		// 0x09DC (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class USkeletalMeshComponent*                      PathsMeshComp;                                    		// 0x09E0 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UTgUISkeletalMeshWidget*                     m_MeshWidget;                                     		// 0x09E4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_QueueNamesPanel;                                		// 0x09E8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_GlobeMIC;                                       		// 0x09EC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_FlightMIC;                                      		// 0x09F0 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_PvEMIC;                                         		// 0x09F4 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_micDefenseActive;                               		// 0x09F8 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_micDefenseInActive;                             		// 0x09FC (0x0004) [0x0000000000000000]              
	float                                              m_fGlobeRotation;                                 		// 0x0A00 (0x0004) [0x0000000000000000]              
	float                                              m_fCloudRotation;                                 		// 0x0A04 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_HexPanel;                                       		// 0x0A08 (0x0004) [0x0000000000000000]              
	float                                              m_fInterpStartTick;                               		// 0x0A0C (0x0004) [0x0000000000000000]              
	float                                              m_fInterpStartRotation;                           		// 0x0A10 (0x0004) [0x0000000000000000]              
	float                                              m_fCloudStartRotation;                            		// 0x0A14 (0x0004) [0x0000000000000000]              
	int                                                m_nMapPosition;                                   		// 0x0A18 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1699 ];

		return pClassPointer;
	};

	void eventSetMissionReturn ( );
	void eventSetIconButtonDelegate ( class UUIButton* btn );
	void eventFixupWidgetsUC ( );
	bool CheckStopBriefing ( struct FInputEventParameters* EventParms );
	void GetQueueListCallback ( int eType );
	void ToolTipDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnButtonStateChangeDelegate ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnLocationPrevButton ( struct FInputEventParameters* EventParms );
	bool OnLocationNextButton ( struct FInputEventParameters* EventParms );
	bool OnSelectDifficultyDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectTeamTypeDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectGameTypeDelegate ( struct FInputEventParameters* EventParms );
	bool OnTabClickDelegate ( struct FInputEventParameters* EventParms );
	bool OnPreferenceChangeDelegate ( struct FInputEventParameters* EventParms );
	bool OnBriefingButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnLeaveQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnJoinQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectQueueDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyDelegate ( struct FInputEventParameters* EventParms );
	void SelectTeamType ( class UUIButton* btn );
	void ShowMissionPage ( int nPage );
	void UpdateInfoPanel ( );
	void TickVisual ( float DeltaTime );
	void PostRender ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void RegisterCallbacks ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIMissionScene_Zone
// 0x002C (0x01A8 - 0x017C)
class UTgUIMissionScene_Zone : public UTgUISceneDriver_Tooltip
{
public:
	class ATgPlayerController*                         m_PC;                                             		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_GlobeTab;                                       		// 0x0184 (0x0004) [0x0000000000000000]              
	class UTgUIMapWidget*                              m_MapWidget;                                      		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_MapImage;                                       		// 0x018C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BeaconHelp;                                     		// 0x0190 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bBeaconNetworkActive : 1;                       		// 0x0194 (0x0004) [0x0000000000000000] [0x00000001] 
	class UMaterialInstanceConstant*                   m_IconLocalPlayer;                                		// 0x0198 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconTeamPlayer;                                 		// 0x019C (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_IconObjective;                                  		// 0x01A0 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pPortalNetwork;                                 		// 0x01A4 (0x0004) [0x0000000000003000]              ( CPF_Native | CPF_Transient )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1700 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnTabClickDelegate ( struct FInputEventParameters* EventParms );
	bool OnMapClickDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyDelegate ( struct FInputEventParameters* EventParms );
	void PreCloseScene ( );
	void TickVisual ( float DeltaTime );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUINewPlayerHelp
// 0x0004 (0x0180 - 0x017C)
class UTgUINewPlayerHelp : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x017C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1701 ];

		return pClassPointer;
	};

	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void PostOpenScene ( );
};


// Class TgClient.TgUIQuestDialog
// 0x0500 (0x067C - 0x017C)
class UTgUIQuestDialog : public UTgUISceneDriver_Tooltip
{
public:
	unsigned char                                      m_eDialogState;                                   		// 0x017C (0x0001) [0x0000000000000000]              
	unsigned char                                      m_eTutorialStep;                                  		// 0x017D (0x0001) [0x0000000000000000]              
	unsigned char                                      m_eTutorialType;                                  		// 0x017E (0x0001) [0x0000000000000000]              
	int                                                m_nAvailableQuests;                               		// 0x0180 (0x0004) [0x0000000000000000]              
	int                                                m_nActiveQuests;                                  		// 0x0184 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedReward;                                		// 0x0188 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bHasSelectedReward : 1;                         		// 0x018C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bTurninReady : 1;                               		// 0x018C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bTutorialMode : 1;                              		// 0x018C (0x0004) [0x0000000000000000] [0x00000004] 
	struct FsQuestErrMsg                               m_QuestErr;                                       		// 0x0190 (0x0010) [0x0000000000000000]              
	struct FPointer                                    m_QuestGroup;                                     		// 0x01A0 (0x0004) [0x0000000000103000]              ( CPF_Native | CPF_Transient )
	struct FPointer                                    m_QuestVolume;                                    		// 0x01A4 (0x0004) [0x0000000000103000]              ( CPF_Native | CPF_Transient )
	struct FPointer                                    m_SelectedQuest;                                  		// 0x01A8 (0x0004) [0x0000000000103000]              ( CPF_Native | CPF_Transient )
	class UUIPanel*                                    m_PositionPanel;                                  		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_QuestNPCLabel;                                  		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_Feedback;                                       		// 0x01B4 (0x0004) [0x0000000000000000]              
	struct FsIntroWidgets                              m_IntroWidgets;                                   		// 0x01B8 (0x0104) [0x0000000000000000]              
	struct FsOfferWidgets                              m_OfferWidgets;                                   		// 0x02BC (0x0128) [0x0000000000000000]              
	struct FsTurninWidgets                             m_TurninWidgets;                                  		// 0x03E4 (0x00BC) [0x0000000000000000]              
	struct FsCompletionWidgets                         m_CompletionWidgets;                              		// 0x04A0 (0x0168) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_BoostingMic;                                    		// 0x0608 (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_EliteAgentMic;                                  		// 0x060C (0x0004) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_AutoLootImposters;                              		// 0x0610 (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_SelLootImposters;                               		// 0x061C (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_RequiredItemImposters;                          		// 0x0628 (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	class UTgInventoryObject*                          m_BoostItemImposter;                              		// 0x0634 (0x0004) [0x0000000000102000]              ( CPF_Transient )
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x0638 (0x0004) [0x0000000000000000]              
	class ATgSkeletalMeshActorNPC*                     m_NPC;                                            		// 0x063C (0x0004) [0x0000000000002000]              ( CPF_Transient )
	class UUIPanel*                                    m_TutorialMainPanel;                              		// 0x0640 (0x0004) [0x0000000000000000]              
	struct FsQTutPanels                                m_TutPanels[ 0x2 ];                               		// 0x0644 (0x0020) [0x0000000000000000]              
	TArray< int >                                      m_nAcceptTutorialVolumeId;                        		// 0x0664 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_nTurninTutorialVolumeId;                        		// 0x0670 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1702 ];

		return pClassPointer;
	};

	bool CheckTutorialTypeDisabled ( unsigned char eType );
	bool CheckTutorialDisabled ( unsigned char Step );
	bool CheckTutorialStep ( unsigned char Step );
	void UpdateTutorialProgress ( unsigned char Step, int NewTutorialType );
	void BeginTutorial ( unsigned char TutType );
	void ActiveHighlightChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void ActiveTooltipChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool DefaultSceneHandler ( struct FInputEventParameters* EventParms );
	bool OnBuyNowButton ( struct FInputEventParameters* EventParms );
	bool OnQuestButton ( struct FInputEventParameters* EventParms );
	bool OnItemButton ( struct FInputEventParameters* EventParms );
	bool OnDialogButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIQuestLog
// 0x0358 (0x04D4 - 0x017C)
class UTgUIQuestLog : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_NoActiveMissionPanel;                           		// 0x0180 (0x0004) [0x0000000000000000]              
	struct FsActiveWidgets                             m_ActiveWidgets;                                  		// 0x0184 (0x01DC) [0x0000000000000000]              
	int                                                m_SelectedQuestIndex;                             		// 0x0360 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bDoingSelection : 1;                            		// 0x0364 (0x0004) [0x0000000000000000] [0x00000001] 
	int                                                m_nQuestsDisplayed;                               		// 0x0368 (0x0004) [0x0000000000000000]              
	struct FsQuestLogElement                           m_QuestLogElements[ 0x14 ];                       		// 0x036C (0x00F0) [0x0000000000000000]              
	struct FPointer                                    m_pAmQuests[ 0x14 ];                              		// 0x045C (0x0050) [0x0000000000003000]              ( CPF_Native | CPF_Transient )
	TArray< class UTgInventoryObject* >                m_AutoLootImposters;                              		// 0x04AC (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_SelLootImposters;                               		// 0x04B8 (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	TArray< class UTgInventoryObject* >                m_RequiredItemImposters;                          		// 0x04C4 (0x000C) [0x0000000000502000]              ( CPF_Transient | CPF_NeedCtorLink )
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x04D0 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1703 ];

		return pClassPointer;
	};

	void ActiveTooltipChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool DefaultSceneHandler ( struct FInputEventParameters* EventParms );
	bool OnSceneButton ( struct FInputEventParameters* EventParms );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void PostOpenScene ( );
};


// Class TgClient.TgUIRewardScreen
// 0x0014 (0x0190 - 0x017C)
class UTgUIRewardScreen : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_CloseButton;                                    		// 0x017C (0x0004) [0x0000000000000000]              
	float                                              m_PreviousTickTime;                               		// 0x0180 (0x0004) [0x0000000000000000]              
	float                                              m_fAutoDismissTime;                               		// 0x0184 (0x0004) [0x0000000000000000]              
	float                                              m_fRevealTime;                                    		// 0x0188 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bStartAutoDismissOnOpen : 1;                    		// 0x018C (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1704 ];

		return pClassPointer;
	};

	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void RevealHiddenReward ( );
	void SetTimeToReveal ( );
	void BeginAutoDismiss ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	bool OnSceneKeyPressed ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIReceivedItemScreen
// 0x0044 (0x01D4 - 0x0190)
class UTgUIReceivedItemScreen : public UTgUIRewardScreen
{
public:
	class UUIPanel*                                    m_ItemPanel;                                      		// 0x0190 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_ItemData;                                       		// 0x0194 (0x0004) [0x0000000000001000]              ( CPF_Native )
	class UTgUIPrimaryHUD_SlotWidgets*                 m_wItem;                                          		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemNameLabel;                                  		// 0x019C (0x0004) [0x0000000000000000]              
	class UTgInventoryObject*                          m_InvItem;                                        		// 0x01A0 (0x0004) [0x0000000000000000]              
	int                                                m_nFromContainerType;                             		// 0x01A4 (0x0004) [0x0000000000000000]              
	int                                                m_nFromContainerId;                               		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UMaterial*                                   m_RandomLootMat;                                  		// 0x01AC (0x0004) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_RandomLootMIC;                                  		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_RandomLootImage;                                		// 0x01B4 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfLastRandom;                              		// 0x01B8 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeBetweenRandom;                             		// 0x01BC (0x0004) [0x0000000000000000]              
	float                                              m_fAutoDismissTimeAfterReveal;                    		// 0x01C0 (0x0004) [0x0000000000000000]              
	int                                                m_nDeferredPositionUpdate;                        		// 0x01C4 (0x0004) [0x0000000000000000]              
	struct FString                                     m_PromoContainerName;                             		// 0x01C8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1705 ];

		return pClassPointer;
	};

	void eventSetItemStateChange ( class UUIScreenObject* Obj );
	void OnItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void RevealHiddenReward ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUISettingsMenu_General
// 0x00F4 (0x0270 - 0x017C)
class UTgUISettingsMenu_General : public UTgUISceneDriver_Tooltip
{
public:
	class UUIButton*                                   m_VideoTabButton;                                 		// 0x017C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SoundTabButton;                                 		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ControlsTabButton;                              		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ChatTabButton;                                  		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplyButton;                                    		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_MouseSensitivitySlider;                         		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_MouseSensitivityEditbox;                        		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_InvertMouseCheckbox;                            		// 0x019C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_GeneralSettings[ 0x10 ];                        		// 0x01A0 (0x0040) [0x0000000000000000]              
	class UUICheckbox*                                 m_HUDVisibilitySettings[ 0xC ];                   		// 0x01E0 (0x0030) [0x0000000000000000]              
	class UUICheckbox*                                 m_QueueSettings[ 0x3 ];                           		// 0x0210 (0x000C) [0x0000000000000000]              
	struct FsTooltipMsg                                m_QueueSettingsTip[ 0x3 ];                        		// 0x021C (0x0024) [0x0000000000000000]              
	class UUIComboBox*                                 m_QueueSelectComboBox;                            		// 0x0240 (0x0004) [0x0000000000000000]              
	struct FsTooltipMsg                                m_QueueSelectTip;                                 		// 0x0244 (0x000C) [0x0000000000000000]              
	class UUICheckbox*                                 m_TeamSettings[ 0x2 ];                            		// 0x0250 (0x0008) [0x0000000000000000]              
	struct FsTooltipMsg                                m_TeamSettingsTip[ 0x2 ];                         		// 0x0258 (0x0018) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1706 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void OnHitEffectFullChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnHitEffectZoomChanged ( class UUIObject* Sender, int PlayerIndex );
	void ActiveStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnSiteChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnMouseEditBoxStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnMouseSliderChanged ( class UUIObject* Sender, int PlayerIndex );
	bool OnMouseSensitivitySubmit ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnTabClickedDelegate ( struct FInputEventParameters* EventParms );
	bool OnApplyButtonKey ( struct FInputEventParameters* EventParms );
	void PopulateSiteList ( );
	void PopulateQueueSettingWidgets ( );
	void PopulateHUDVisibilityWidgets ( );
	void PopulateWidgets ( );
	void ApplyHUDVisibilitySettings ( );
	void ApplyPrimaryHUDSettings ( );
	void ApplyControllerSettings ( );
	void ApplyMouseSettings ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUISettingsMenu_Sound
// 0x005C (0x01D8 - 0x017C)
class UTgUISettingsMenu_Sound : public UTgUISceneDriver_Tooltip
{
public:
	class UUISlider*                                   m_GameSlider[ 0x3 ];                              		// 0x017C (0x000C) [0x0000000000000000]              
	class UUISlider*                                   m_VoiceSlider[ 0x3 ];                             		// 0x0188 (0x000C) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplyButton;                                    		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_GeneralTabButton;                               		// 0x019C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_VideoTabButton;                                 		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ControlsTabButton;                              		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ChatTabButton;                                  		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_RenderDevComboBox;                              		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_CaptureDevComboBox;                             		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_EnableTeamCheckbox;                             		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_EnableInstanceCheckbox;                         		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_EnableVoipCheckbox;                             		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_EnableAudioCheckbox;                            		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_GamePanelBlocker;                               		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_VoicePanelBlocker;                              		// 0x01C8 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_RenderDevData;                                  		// 0x01CC (0x0004) [0x0000000000001000]              ( CPF_Native )
	struct FPointer                                    m_CaptureDevData;                                 		// 0x01D0 (0x0004) [0x0000000000001000]              ( CPF_Native )
	unsigned long                                      m_bNeedRestartConfirm : 1;                        		// 0x01D4 (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1707 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void OnEnableAudioChanged ( class UUIObject* Sender, int PlayerIndex );
	void ShowHideAudioPanel ( );
	void OnEnableVoipChanged ( class UUIObject* Sender, int PlayerIndex );
	void ShowHideVoipPanel ( );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void OpenRestartConfirmPopup ( );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnTabClickedDelegate ( struct FInputEventParameters* EventParms );
	bool OnApplyButtonKey ( struct FInputEventParameters* EventParms );
	void ApplyVoiceSliders ( );
	void ApplyGameSliders ( );
	void PopulateWidgets ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUISettingsMenu_Video
// 0x0350 (0x04CC - 0x017C)
class UTgUISettingsMenu_Video : public UTgUISceneDriver_Tooltip
{
public:
	TArray< struct FsTgAspectRatioInfo >               m_AspectRatioInfo;                                		// 0x017C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FName                                       m_AspectRatioDataName;                            		// 0x0188 (0x0008) [0x0000000000000000]              
	struct FName                                       m_StdComboRangeDataName;                          		// 0x0190 (0x0008) [0x0000000000000000]              
	struct FName                                       m_2StateComboRangeDataName;                       		// 0x0198 (0x0008) [0x0000000000000000]              
	struct FName                                       m_FilteringDataName;                              		// 0x01A0 (0x0008) [0x0000000000000000]              
	struct FName                                       m_AnisoQualityDataName;                           		// 0x01A8 (0x0008) [0x0000000000000000]              
	struct FName                                       m_WindowTypeDataName;                             		// 0x01B0 (0x0008) [0x0000000000000000]              
	struct FName                                       m_MSAAQualityDataName;                            		// 0x01B8 (0x0008) [0x0000000000000000]              
	struct FString                                     m_SelectedResolutionString;                       		// 0x01C0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIComboBox*                                 m_AspectRatioComboBox;                            		// 0x01CC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_ResolutionComboBox;                             		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_WindowTypeComboBox;                             		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CurrentResolutionLabel;                         		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_GeneralTabButton;                               		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SoundTabButton;                                 		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ControlsTabButton;                              		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ChatTabButton;                                  		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplyButton;                                    		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ResetButton;                                    		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_Checkboxes[ 0x13 ];                             		// 0x01F8 (0x004C) [0x0000000000000000]              
	class UUIComboBox*                                 m_ComboBoxes[ 0x8 ];                              		// 0x0244 (0x0020) [0x0000000000000000]              
	struct FsTooltipMsg                                m_CheckboxTips[ 0x13 ];                           		// 0x0264 (0x00E4) [0x0000000000000000]              
	struct FsTooltipMsg                                m_ComboBoxTips[ 0x13 ];                           		// 0x0348 (0x00E4) [0x0000000000000000]              
	class UUISlider*                                   m_GammaCorrectionSlider;                          		// 0x042C (0x0004) [0x0000000000000000]              
	struct FsVideoConfirm                              m_VideoConfirm;                                   		// 0x0430 (0x001C) [0x0000000000000000]              
	struct FsOptionValues                              m_GatheredSettings;                               		// 0x044C (0x0080) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1708 ];

		return pClassPointer;
	};

	void OnGammaCorrectionSliderChanged ( class UUIObject* Sender, int PlayerIndex );
	void ActiveStateChanged ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void CBoxChanged ( class UUIObject* Sender, int PlayerIndex );
	bool ConfirmHandler ( struct FInputEventParameters* EventParms );
	bool ButtonHandler ( struct FInputEventParameters* EventParms );
	bool TabHandler ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUITeamDetailMenu
// 0x00C0 (0x023C - 0x017C)
class UTgUITeamDetailMenu : public UTgUISceneDriver_Tooltip
{
public:
	struct FsTabButtonStruct                           m_TabStruct[ 0x4 ];                               		// 0x017C (0x0060) [0x0000000000000000]              
	class UUIButton*                                   m_wCloseButton;                                   		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wPromoteButton;                                 		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wLeaveButton;                                   		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wStrikeForceButton;                             		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRaidButton;                                    		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRaidButton2;                                   		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_wHeaderLabel;                                   		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RaidTimePanel;                                  		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeLabel;                                  		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeHeader;                                 		// 0x0204 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_DetailScrollFrame;                              		// 0x0208 (0x0004) [0x0000000000000000]              
	TArray< struct FsInvitePlayerStruct >              m_DetailList;                                     		// 0x020C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UTgUIScrollList*                             m_MemberListScrollFrame;                          		// 0x0218 (0x0004) [0x0000000000000000]              
	TArray< struct FsTeamMemberStruct >                m_MemberList;                                     		// 0x021C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nTeamUpdateSeq;                                 		// 0x0228 (0x0004) [0x0000000000000000]              
	float                                              m_InviteTimer;                                    		// 0x022C (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedMember;                                		// 0x0230 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bOpenRefresh : 1;                               		// 0x0234 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bPlayerIsLeader : 1;                            		// 0x0234 (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                m_nOwningAgencyId;                                		// 0x0238 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1709 ];

		return pClassPointer;
	};

	void eventSetRawInputKeyForMemberList ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	bool SelectMemberByButton ( class UUIButton* Button );
	bool OnSceneMapDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneManageDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneCargoDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneTeamDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRaidDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneStrikeForceDelegate ( struct FInputEventParameters* EventParms );
	bool OnTeamMemberDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneLeaveDelegate ( struct FInputEventParameters* EventParms );
	bool OnScenePromoteDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUITeamMenu
// 0x0118 (0x0294 - 0x017C)
class UTgUITeamMenu : public UTgUISceneDriver_Tooltip
{
public:
	struct FsTabButtonStruct                           m_TabStruct[ 0x4 ];                               		// 0x017C (0x0060) [0x0000000000000000]              
	class UUIButton*                                   m_wCloseButton;                                   		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wResetButton;                                   		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wSearchButton;                                  		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wInviteButton;                                  		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wMessageButton;                                 		// 0x01EC (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_wAvATeamSearchCheckbox;                         		// 0x01F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRemoveButton;                                  		// 0x01F4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wPromoteButton;                                 		// 0x01F8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wLeaveButton;                                   		// 0x01FC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wStrikeForceButton;                             		// 0x0200 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRaidButton;                                    		// 0x0204 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_wRaidButton2;                                   		// 0x0208 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_wSortHeaderButton[ 0x6 ];                       		// 0x020C (0x0018) [0x0000000000000000]              
	class UUILabel*                                    m_wHeaderLabel;                                   		// 0x0224 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_RaidTimePanel;                                  		// 0x0228 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeLabel;                                  		// 0x022C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_RaidTimeHeader;                                 		// 0x0230 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wNameSearchBox;                                 		// 0x0234 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wAllianceSearchBox;                             		// 0x0238 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wAgencySearchBox;                               		// 0x023C (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wLevelMinSearchBox;                             		// 0x0240 (0x0004) [0x0000000000000000]              
	class UUIEditBox*                                  m_wLevelMaxSearchBox;                             		// 0x0244 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_wClassComboBox;                                 		// 0x0248 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_wStatusComboBox;                                		// 0x024C (0x0004) [0x0000000000000000]              
	class UTgDataInterface*                            m_TeamDataInterface;                              		// 0x0250 (0x0004) [0x0000000000000000]              
	class UTgDataSet*                                  m_PlayerSearchDataSet;                            		// 0x0254 (0x0004) [0x0000000000000000]              
	unsigned char                                      m_CurrentSortType;                                		// 0x0258 (0x0001) [0x0000000000000000]              
	unsigned long                                      m_bAscendingSort : 1;                             		// 0x025C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bOpenRefresh : 1;                               		// 0x025C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bPlayerIsLeader : 1;                            		// 0x025C (0x0004) [0x0000000000000000] [0x00000004] 
	class UTgUIScrollList*                             m_InviteScrollFrame;                              		// 0x0260 (0x0004) [0x0000000000000000]              
	TArray< struct FsInvitePlayerStruct >              m_InviteList;                                     		// 0x0264 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UTgUIScrollList*                             m_MemberListScrollFrame;                          		// 0x0270 (0x0004) [0x0000000000000000]              
	TArray< struct FsTeamMemberStruct >                m_MemberList;                                     		// 0x0274 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nTeamUpdateSeq;                                 		// 0x0280 (0x0004) [0x0000000000000000]              
	float                                              m_InviteTimer;                                    		// 0x0284 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedItem;                                  		// 0x0288 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedMember;                                		// 0x028C (0x0004) [0x0000000000000000]              
	int                                                m_nOwningAgencyId;                                		// 0x0290 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1710 ];

		return pClassPointer;
	};

	void eventSetRawInputKeyForMemberList ( class UUIButton* Button );
	void eventSetRawInputKeyForPlayerList ( class UUIButton* Button );
	void eventFixupWidgetsUC ( );
	void GetPlayerSearchCallback ( int eType );
	bool SelectPlayerByButton ( class UUIButton* Button );
	bool SelectMemberByButton ( class UUIButton* Button );
	bool OnSceneSortDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneMapDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneManageDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneDetailDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneCargoDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRaidDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneStrikeForceDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectPlayer ( struct FInputEventParameters* EventParms );
	bool OnTeamMemberDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneLeaveDelegate ( struct FInputEventParameters* EventParms );
	bool OnScenePromoteDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneRemoveDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneMessageDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInviteDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneSearchDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneResetDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void SortSearchResults ( );
	void Search ( );
	void SearchFromFields ( );
	void ResetFields ( );
	void PrepopulatedSearchById ( int nPlayerId );
	void PrepopulatedSearch ( struct FString sPlayerName, struct FString sAgencyName, struct FString sAllianceName );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	void PreCloseScene ( );
	void PostInitSceneDriver ( );
};


// Class TgClient.TgUITutorialItem
// 0x00CC (0x0248 - 0x017C)
class UTgUITutorialItem : public UTgUISceneDriver_Tooltip
{
public:
	unsigned char                                      m_ActiveTutorial;                                 		// 0x017C (0x0001) [0x0000000000000000]              
	float                                              m_TimeOpened;                                     		// 0x0180 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_SingleFrame;                                    		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SingleFrameExit;                                		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DoubleFrame;                                    		// 0x018C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DoubleFrameExit[ 0x2 ];                         		// 0x0190 (0x0008) [0x0000000000000000]              
	struct FsTutItemInfo                               m_AssaultDevs[ 0x5 ];                             		// 0x0198 (0x0028) [0x0000000000000000]              
	struct FsTutItemInfo                               m_MedicDevs[ 0x5 ];                               		// 0x01C0 (0x0028) [0x0000000000000000]              
	struct FsTutItemInfo                               m_ReconDevs[ 0x5 ];                               		// 0x01E8 (0x0028) [0x0000000000000000]              
	struct FsTutItemInfo                               m_RoboticsDevs[ 0x5 ];                            		// 0x0210 (0x0028) [0x0000000000000000]              
	struct FsTutItemInfo                               m_Jetpack;                                        		// 0x0238 (0x0008) [0x0000000000000000]              
	class UMaterialInstanceConstant*                   m_BlingMIC;                                       		// 0x0240 (0x0004) [0x0000000000000000]              
	int                                                m_BlingMICId;                                     		// 0x0244 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1711 ];

		return pClassPointer;
	};

	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIVendorBase
// 0x0008 (0x0184 - 0x017C)
class UTgUIVendorBase : public UTgUISceneDriver_Tooltip
{
public:
	unsigned char                                      m_VendorType;                                     		// 0x017C (0x0001) [0x0000000000000000]              
	class ATgPawn_Character*                           m_PlayerPawn;                                     		// 0x0180 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1712 ];

		return pClassPointer;
	};

	void PostOpenScene ( );
};


// Class TgClient.TgUISteamTxnStatus
// 0x001C (0x01A0 - 0x0184)
class UTgUISteamTxnStatus : public UTgUIVendorBase
{
public:
	class UUILabel*                                    m_Status;                                         		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ButtonLabel;                                    		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_Button;                                         		// 0x018C (0x0004) [0x0000000000000000]              
	class UTgUIVendorBase*                             m_pCaller;                                        		// 0x0190 (0x0004) [0x0000000000100000]              
	unsigned long                                      m_bHasOverlay : 1;                                		// 0x0194 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bHasConfirmed : 1;                              		// 0x0194 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bNeedsExit : 1;                                 		// 0x0194 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bVariable : 1;                                  		// 0x0194 (0x0004) [0x0000000000000000] [0x00000008] 
	float                                              m_fTimeStarted;                                   		// 0x0198 (0x0004) [0x0000000000000000]              
	float                                              m_fTimeOfEvent;                                   		// 0x019C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1713 ];

		return pClassPointer;
	};

	bool TickTgUIScene ( );
	void PreCloseScene ( );
	void PostOpenScene ( );
	bool OnButtonDelegate ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIUpdateFaceMenu
// 0x0208 (0x038C - 0x0184)
class UTgUIUpdateFaceMenu : public UTgUIVendorBase
{
public:
	class ATgSkeletalMeshActor_CharacterBuilder*       m_PaperDoll;                                      		// 0x0184 (0x0004) [0x0000000000000000]              
	class UTgSkeletalMeshComponent*                    m_ShoulderMesh;                                   		// 0x0188 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	int                                                m_MaleShoulderMeshId;                             		// 0x018C (0x0004) [0x0000000000000000]              
	int                                                m_FemaleShoulderMeshId;                           		// 0x0190 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bCreateLock : 1;                                		// 0x0194 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_DetailMode : 1;                                 		// 0x0194 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bEnableBackgroundFading : 1;                    		// 0x0194 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bHasAPPrices : 1;                               		// 0x0194 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bHasNonAPPrices : 1;                            		// 0x0194 (0x0004) [0x0000000000000000] [0x00000010] 
	int                                                m_CurrentMorphGroup;                              		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSlider;                                      		// 0x019C (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSliderPrimaryColor;                          		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSliderSecondaryColor;                        		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSliderTertiaryColor;                         		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSliderPrimaryIntensity;                      		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSliderSecondaryIntensity;                    		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_InkSliderTertiaryIntensity;                     		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkMorphNode;                                   		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkPrimaryColorMorphNode;                       		// 0x01BC (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkSecondaryColorMorphNode;                     		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkTertiaryColorMorphNode;                      		// 0x01C4 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkPrimaryIntensityMorphNode;                   		// 0x01C8 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkSecondaryIntensityMorphNode;                 		// 0x01CC (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_InkTertiaryIntensityMorphNode;                  		// 0x01D0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SubmitButton;                                   		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelButton;                                   		// 0x01DC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_RandomButton;                                   		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     m_PaperDollMeshWidget;                            		// 0x01E4 (0x0004) [0x0000000000000000]              
	TArray< struct FsCustomHeadGroupStruct >           m_HeadGroups;                                     		// 0x01E8 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	struct FsCustomHeadPresetStruct                    m_GroupPreset[ 0x8 ];                             		// 0x01F4 (0x0140) [0x0000000000000000]              
	class UUIPanel*                                    m_BasicPanel;                                     		// 0x0334 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_DetailPanel;                                    		// 0x0338 (0x0004) [0x0000000000000000]              
	class UTgUIScrollList*                             m_MorphScrollFrame;                               		// 0x033C (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_BaseHeadSlider;                                 		// 0x0340 (0x0004) [0x0000000000000000]              
	struct FsUFPStruct                                 m_PlayerBalance;                                  		// 0x0344 (0x0014) [0x0000000000000000]              
	struct FsUFPStruct                                 m_ServicePrice;                                   		// 0x0358 (0x0014) [0x0000000000000000]              
	float                                              m_fFadeInTime;                                    		// 0x036C (0x0004) [0x0000000000000000]              
	float                                              m_fFadeOutTime;                                   		// 0x0370 (0x0004) [0x0000000000000000]              
	float                                              m_fFadeAmount;                                    		// 0x0374 (0x0004) [0x0000000000000000]              
	class UAnimTree*                                   m_HeadAnimTree;                                   		// 0x0378 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList_ByHairStyle*           m_HairStyleMorphNode;                             		// 0x037C (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_BaseHeadsMorphNode;                             		// 0x0380 (0x0004) [0x0000000000000000]              
	int                                                m_nCachedCurrency;                                		// 0x0384 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_LootTableItem;                                  		// 0x0388 (0x0004) [0x0000000000101000]              ( CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1714 ];

		return pClassPointer;
	};

	void OnInkSliderChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnGroupSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnDetailButtonMouseOver ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnRandomButton ( struct FInputEventParameters* EventParms );
	bool OnDetailButton ( struct FInputEventParameters* EventParms );
	bool OnLockButton ( struct FInputEventParameters* EventParms );
	bool OnSubmitButton ( struct FInputEventParameters* EventParms );
	bool OnCancelButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void UpdateCreditsInfo ( );
	void CompleteTransaction ( );
	void PreCloseScene ( );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIUpdateHairMenu
// 0x009C (0x0220 - 0x0184)
class UTgUIUpdateHairMenu : public UTgUIVendorBase
{
public:
	class ATgSkeletalMeshActor_CharacterBuilder*       m_PaperDoll;                                      		// 0x0184 (0x0004) [0x0000000000000000]              
	class UTgSkeletalMeshComponent*                    m_ShoulderMesh;                                   		// 0x0188 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )
	int                                                m_MaleShoulderMeshId;                             		// 0x018C (0x0004) [0x0000000000000000]              
	int                                                m_FemaleShoulderMeshId;                           		// 0x0190 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0194 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SubmitButton;                                   		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CancelButton;                                   		// 0x019C (0x0004) [0x0000000000000000]              
	class UTgUISkeletalMeshWidget*                     m_PaperDollMeshWidget;                            		// 0x01A0 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_HairStyleSlider;                                		// 0x01A4 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_HairColorSlider;                                		// 0x01A8 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_FacialHairSlider;                               		// 0x01AC (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_HairAccessoryColorSlider;                       		// 0x01B0 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_SecondaryHairColorSlider;                       		// 0x01B4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditsDataLabel;                               		// 0x01B8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CreditsCostLabel;                               		// 0x01BC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ExclusiveLabel;                                 		// 0x01C0 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_ShadeSlider3;                                   		// 0x01C4 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bEnableBackgroundFading : 1;                    		// 0x01C8 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bHasAPPrices : 1;                               		// 0x01C8 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bHasNonAPPrices : 1;                            		// 0x01C8 (0x0004) [0x0000000000000000] [0x00000004] 
	float                                              m_fFadeInTime;                                    		// 0x01CC (0x0004) [0x0000000000000000]              
	float                                              m_fFadeOutTime;                                   		// 0x01D0 (0x0004) [0x0000000000000000]              
	float                                              m_fFadeAmount;                                    		// 0x01D4 (0x0004) [0x0000000000000000]              
	class UAnimTree*                                   m_HeadAnimTree;                                   		// 0x01D8 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList_ByHairStyle*           m_HairStyleMorphNode;                             		// 0x01DC (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_HairColorMorphNode;                             		// 0x01E0 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_HairAccessoryMorphNode;                         		// 0x01E4 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_FacialHairMorphNode;                            		// 0x01E8 (0x0004) [0x0000000000000000]              
	class UTgMorphNodeBlendList*                       m_SecondaryHairColorMorphNode;                    		// 0x01EC (0x0004) [0x0000000000000000]              
	int                                                m_nCachedCurrency;                                		// 0x01F0 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_LootTableItem;                                  		// 0x01F4 (0x0004) [0x0000000000101000]              ( CPF_Native )
	struct FsUFPStruct                                 m_PlayerBalance;                                  		// 0x01F8 (0x0014) [0x0000000000000000]              
	struct FsUFPStruct                                 m_ServicePrice;                                   		// 0x020C (0x0014) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1715 ];

		return pClassPointer;
	};

	bool OnSubmitButton ( struct FInputEventParameters* EventParms );
	bool OnCancelButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	void eventInitializeUpdateHairMenu ( );
	void eventFixupWidgetsUC ( );
	bool OnConfirmNoClicked ( struct FInputEventParameters* EventParms );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	void CompleteTransaction ( );
	void UpdateCreditsInfo ( );
	void UpdateExclusiveLabelStatus ( );
	void UpdateSliderDisableStatus ( );
	void PreCloseScene ( );
	void OnSecondaryHairColorSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnHairAccessoryColorSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnFacialHairSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnHairColorSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void OnHairStyleSliderValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void SubmitButtonPressed ( );
	bool TickTgUIScene ( );
	void PostOpenScene ( );
};


// Class TgClient.TgUIVendorPaymentPopup
// 0x00B0 (0x0234 - 0x0184)
class UTgUIVendorPaymentPopup : public UTgUIVendorBase
{
public:
	class UUIPanel*                                    m_Outer;                                          		// 0x0184 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ItemName;                                       		// 0x0188 (0x0004) [0x0000000000000000]              
	class UTgUIInvWidget*                              m_ItemIcon;                                       		// 0x018C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PaymentLabel;                                   		// 0x0190 (0x0004) [0x0000000000000000]              
	struct FStdPaymentOptionPanel                      m_CostTypeStd;                                    		// 0x0194 (0x0058) [0x0000000000000000]              
	struct FAPPaymentOptionPanel                       m_CostTypeAP;                                     		// 0x01EC (0x0024) [0x0000000000000000]              
	int                                                m_nNumOptions;                                    		// 0x0210 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bItemUpdated : 1;                               		// 0x0214 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bIsSingleOptionAPOnly : 1;                      		// 0x0214 (0x0004) [0x0000000000000000] [0x00000002] 
	class UTgInventoryObject*                          m_pPurchasingItem;                                		// 0x0218 (0x0004) [0x0000000000100000]              
	class UTgUIVendorBase*                             m_pCaller;                                        		// 0x021C (0x0004) [0x0000000000100000]              
	struct FsCostDisplayItems                          m_Costs;                                          		// 0x0220 (0x0010) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0230 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1716 ];

		return pClassPointer;
	};

	void PreCloseScene ( );
	void PostOpenScene ( );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	bool OnButtonDelegate ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIVendorScene
// 0x0738 (0x08BC - 0x0184)
class UTgUIVendorScene : public UTgUIVendorBase
{
public:
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x0184 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentLootListID;                              		// 0x0188 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_VendorName;                                     		// 0x018C (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNoAPPrices : 1;                                		// 0x0190 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bBlockInvPanel : 1;                             		// 0x0190 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bSortAscending : 1;                             		// 0x0190 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bVendorSortAscending : 1;                       		// 0x0190 (0x0004) [0x0000000000000000] [0x00000008] 
	unsigned long                                      m_bRefresh : 1;                                   		// 0x0190 (0x0004) [0x0000000000000000] [0x00000010] 
	unsigned long                                      m_bClientSortUpdated : 1;                         		// 0x0190 (0x0004) [0x0000000000000000] [0x00000020] 
	unsigned long                                      m_bHasAPPrices : 1;                               		// 0x0190 (0x0004) [0x0000000000000000] [0x00000040] 
	unsigned long                                      m_bHasNonAPPrices : 1;                            		// 0x0190 (0x0004) [0x0000000000000000] [0x00000080] 
	unsigned long                                      m_bHasEliteEnticement : 1;                        		// 0x0190 (0x0004) [0x0000000000000000] [0x00000100] 
	unsigned long                                      m_bHasBoostEnticement : 1;                        		// 0x0190 (0x0004) [0x0000000000000000] [0x00000200] 
	unsigned char                                      m_eItemPreviewState;                              		// 0x0194 (0x0001) [0x0000000000000000]              
	class UUIButton*                                   m_ShowPreviewButton;                              		// 0x0198 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ShowPlayerInvButton;                            		// 0x019C (0x0004) [0x0000000000000000]              
	struct FsVendorInvItem                             m_InventoryList[ 0x12 ];                          		// 0x01A0 (0x01B0) [0x0000000000000000]              
	class UUIPanel*                                    m_PlayerInventoryPanel;                           		// 0x0350 (0x0004) [0x0000000000000000]              
	int                                                m_nSortType;                                      		// 0x0354 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderName;                                     		// 0x0358 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_HeaderQuantity;                                 		// 0x035C (0x0004) [0x0000000000000000]              
	struct FsPagingPanel                               m_PagingPanel;                                    		// 0x0360 (0x0018) [0x0000000000000000]              
	class UUIPanel*                                    m_PlayerInventoryCreditsPanel;                    		// 0x0378 (0x0004) [0x0000000000000000]              
	struct FsVendorItem                                m_VendorList[ 0x8 ];                              		// 0x037C (0x03A0) [0x0000000000000000]              
	TArray< class UTgInventoryObject* >                m_VendorInvObjs;                                  		// 0x071C (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	TArray< struct FPointer >                          m_VendorLootTableItems;                           		// 0x0728 (0x000C) [0x0000000000101000]              ( CPF_Native )
	struct FsPagingPanel                               m_VendorPagingPanel;                              		// 0x0734 (0x0018) [0x0000000000000000]              
	int                                                m_nVendorPageNum;                                 		// 0x074C (0x0004) [0x0000000000000000]              
	int                                                m_nVendorSortType;                                		// 0x0750 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_VendorHeaderName;                               		// 0x0754 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_VendorHeaderStdPrice;                           		// 0x0758 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_VendorHeaderStdPriceLabel;                      		// 0x075C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_VendorHeaderAPPrice;                            		// 0x0760 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_VendorHeaderAPPriceLabel;                       		// 0x0764 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedItem;                                   		// 0x0768 (0x0004) [0x0000000000000000]              
	int                                                m_SelectedInv;                                    		// 0x076C (0x0004) [0x0000000000000000]              
	int                                                m_CurrentCredits;                                 		// 0x0770 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentTokens;                                  		// 0x0774 (0x0004) [0x0000000000000000]              
	int                                                m_CurrentAP;                                      		// 0x0778 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_PurchaseButton;                                 		// 0x077C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeleteButton;                                   		// 0x0780 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SellButton;                                     		// 0x0784 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PrereqLabel;                                    		// 0x0788 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_PurchasedLabel;                                 		// 0x078C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ExpensiveLabel;                                 		// 0x0790 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ClassLabel;                                     		// 0x0794 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LevelLabel;                                     		// 0x0798 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_ComponentsLabel;                                		// 0x079C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BankLabel;                                      		// 0x07A0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_BankAmount;                                     		// 0x07A4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_BankCreditIcon;                                 		// 0x07A8 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_AgendaPointsAmount;                             		// 0x07AC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CurrencyLabel;                                  		// 0x07B0 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_CurrencyAmount;                                 		// 0x07B4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CurrencyIcon_Credits;                           		// 0x07B8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CurrencyIcon_Tokens;                            		// 0x07BC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CurrencyIcon_USD;                               		// 0x07C0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CurrencyIcon_EUR;                               		// 0x07C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CurrencyIcon_GBP;                               		// 0x07C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x07CC (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_TypeComboBox;                                   		// 0x07D0 (0x0004) [0x0000000000000000]              
	class UUIComboBox*                                 m_QualityComboBox;                                		// 0x07D4 (0x0004) [0x0000000000000000]              
	int                                                m_TypeFilter;                                     		// 0x07D8 (0x0004) [0x0000000000000000]              
	int                                                m_QualityFilter;                                  		// 0x07DC (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           m_ItemTypeStringList;                             		// 0x07E0 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ItemTypeCodeList;                               		// 0x07EC (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FString >                           m_QualityStringList;                              		// 0x07F8 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_QualityCodeList;                                		// 0x0804 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< int >                                      m_ValidTypeList;                                  		// 0x0810 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	int                                                m_nItemTypeofInterest;                            		// 0x081C (0x0004) [0x0000000000000000]              
	struct FItemPreview                                m_ItemPreview;                                    		// 0x0820 (0x008C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	float                                              m_fLastTime;                                      		// 0x08AC (0x0004) [0x0000000000000000]              
	int                                                m_nSpecifiedLootTable;                            		// 0x08B0 (0x0004) [0x0000000000000000]              
	int                                                m_nSpecifiedLootTableInternal;                    		// 0x08B4 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pLootItemsMarshal;                              		// 0x08B8 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1717 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnTogglePreviewInvButton ( struct FInputEventParameters* EventParms );
	bool OnResetPreviewButton ( struct FInputEventParameters* EventParms );
	bool OnVendorPrevButton ( struct FInputEventParameters* EventParms );
	bool OnVendorNextButton ( struct FInputEventParameters* EventParms );
	void OnInvItemStateChange ( class UUIScreenObject* Sender, int PlayerIndex, class UUIState* NewlyActiveState, class UUIState* PreviouslyActiveState );
	void OnResetDoubleClick ( class UUIScreenObject* Sender, int PlayerIndex );
	void OnFilterChangedDelegate ( class UUIObject* Sender, int PlayerIndex );
	bool OnConfirmYesClicked ( struct FInputEventParameters* EventParms );
	bool OnSortVendorDelegate ( struct FInputEventParameters* EventParms );
	bool OnSortInventoryDelegate ( struct FInputEventParameters* EventParms );
	bool OnDeleteInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSelectInventoryItemDelegate ( struct FInputEventParameters* EventParms );
	bool OnVendorItemSellDelegate ( struct FInputEventParameters* EventParms );
	bool OnVendorItemPurchaseDelegate ( struct FInputEventParameters* EventParms );
	bool OnVendorItemSelectDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneInputKeyDelegate ( struct FInputEventParameters* EventParms );
	void PostRender ( );
	void PostOpenScene ( );
	bool TickTgUIScene ( );
	void PreCloseScene ( );
};


// Class TgClient.TgUICashShop
// 0x0000 (0x08BC - 0x08BC)
class UTgUICashShop : public UTgUIVendorScene
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1718 ];

		return pClassPointer;
	};

	bool OnVendorItemPurchaseDelegate ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIScoreboard
// 0x020C (0x02B8 - 0x00AC)
class UTgUIScoreboard : public UTgUISceneDriver
{
public:
	int                                                m_LocalPlayerTaskForce;                           		// 0x00AC (0x0004) [0x0000000000000000]              
	struct FsScoreTeamStruct                           m_TeamPanel[ 0x2 ];                               		// 0x00B0 (0x01F0) [0x0000000000000000]              
	class UTgStatsData*                                m_StatsData;                                      		// 0x02A0 (0x0004) [0x0000000000000000]              
	float                                              m_fLastCopy;                                      		// 0x02A4 (0x0004) [0x0000000000000000]              
	class UTexture2D*                                  m_StatusTexture[ 0x4 ];                           		// 0x02A8 (0x0010) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1719 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void PopulateStructsWithUIPointers ( );
	void UpdateLabels ( );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIScoreDetails
// 0x04F4 (0x05A0 - 0x00AC)
class UTgUIScoreDetails : public UTgUISceneDriver
{
public:
	struct FPlayerField                                m_PlayerFields[ 0x14 ];                           		// 0x00AC (0x0460) [0x0000000000000000]              
	class UUIButton*                                   m_ScoreBoardHeaderButtons[ 0xD ];                 		// 0x050C (0x0034) [0x0000000000000000]              
	class UUILabel*                                    m_HeaderLabels[ 0xD ];                            		// 0x0540 (0x0034) [0x0000000000000000]              
	unsigned char                                      m_eCurrentSortField;                              		// 0x0574 (0x0001) [0x0000000000000000]              
	class UUILabel*                                    m_AttWinProbValueLabel;                           		// 0x0578 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_StatsPanel;                                     		// 0x057C (0x0004) [0x0000000000000000]              
	class UTgStatsData*                                m_StatsData;                                      		// 0x0580 (0x0004) [0x0000000000000000]              
	float                                              m_fLastCopy;                                      		// 0x0584 (0x0004) [0x0000000000000000]              
	int                                                m_LocalPlayerTaskForce;                           		// 0x0588 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x058C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SummaryButton;                                  		// 0x0590 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_DeviceStatsButton;                              		// 0x0594 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_EndMissionButton;                               		// 0x0598 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_GameOver : 1;                                   		// 0x059C (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1720 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	bool OnEndMissionButton ( struct FInputEventParameters* EventParms );
	bool OnDeviceStatsButton ( struct FInputEventParameters* EventParms );
	bool OnSummaryButton ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnFieldClicked ( struct FInputEventParameters* EventParms );
	bool OnSceneKey ( struct FInputEventParameters* EventParms );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUISelectionList
// 0x0074 (0x0120 - 0x00AC)
class UTgUISelectionList : public UTgUISceneDriver
{
public:
	class UUILabelButton*                              m_SelectionButtons[ 0xA ];                        		// 0x00AC (0x0028) [0x0000000000000000]              
	class UUIImage*                                    m_SelectionHilight[ 0xA ];                        		// 0x00D4 (0x0028) [0x0000000000000000]              
	class UTgDataInterface*                            c_DataInterface;                                  		// 0x00FC (0x0004) [0x0000000000000000]              
	class UTgDataSet*                                  c_DataSet;                                        		// 0x0100 (0x0004) [0x0000000000000000]              
	int                                                m_nSelectedIndex;                                 		// 0x0104 (0x0004) [0x0000000000000000]              
	int                                                c_nNbrOfRows;                                     		// 0x0108 (0x0004) [0x0000000000000000]              
	int                                                c_nScrollPos;                                     		// 0x010C (0x0004) [0x0000000000000000]              
	class UUIButton*                                   c_ScrollDownButton;                               		// 0x0110 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   c_ScrollUpButton;                                 		// 0x0114 (0x0004) [0x0000000000000000]              
	int                                                c_nScrollDir;                                     		// 0x0118 (0x0004) [0x0000000000000000]              
	class ATgTimerManager*                             c_ButtonTimers;                                   		// 0x011C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1721 ];

		return pClassPointer;
	};

	bool SelectSelectionRow9 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow8 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow7 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow6 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow5 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow4 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow3 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow2 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow1 ( struct FInputEventParameters* EventParms );
	bool SelectSelectionRow0 ( struct FInputEventParameters* EventParms );
	void ModifyScrollPos ( int nTimerId, unsigned char eEvent );
	bool ScrollListUp ( struct FInputEventParameters* EventParms );
	bool ScrollListDown ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void eventInitSelectionListScene ( );
	bool SelectSelectionRow ( struct FName Key, unsigned char Event, int nIndex );
	void PreCloseScene ( );
	void DrawGrid ( );
	void GetSelectionCallback ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUISettingsMenu_Chat
// 0x0268 (0x0314 - 0x00AC)
class UTgUISettingsMenu_Chat : public UTgUISceneDriver
{
public:
	struct FCC_Group_Configuration                     m_Groups[ 0x4 ];                                  		// 0x00AC (0x0240) [0x0000000000000000]              
	class UUIButton*                                   m_VideoTabButton;                                 		// 0x02EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SoundTabButton;                                 		// 0x02F0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ControlsTabButton;                              		// 0x02F4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_GeneralTabButton;                               		// 0x02F8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ApplyButton;                                    		// 0x02FC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x0300 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_OpacitySlider;                                  		// 0x0304 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ProfanityFilterCheckbox;                        		// 0x0308 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowAgencyLoginCheckbox;                        		// 0x030C (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_ShowFriendsLoginCheckbox;                       		// 0x0310 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1722 ];

		return pClassPointer;
	};

	bool OnSubmitText ( class UUIEditBox* ebox, int PlayerIndex );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnEboxClicked ( struct FInputEventParameters* EventParms );
	bool OnTabClickedDelegate ( struct FInputEventParameters* EventParms );
	bool OnApplyButtonKey ( struct FInputEventParameters* EventParms );
	void OnOpacityValueChanged ( class UUIObject* Sender, int PlayerIndex );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUISettingsMenu_Controls
// 0x0064 (0x0110 - 0x00AC)
class UTgUISettingsMenu_Controls : public UTgUISceneDriver
{
public:
	class UTgUIScrollList*                             m_KeyBindScrollFrame;                             		// 0x00AC (0x0004) [0x0000000000000000]              
	TArray< struct FsKeyBindCategory >                 m_KeyBinds;                                       		// 0x00B0 (0x000C) [0x0000000000500000]              ( CPF_NeedCtorLink )
	class UUIPrefabInstance*                           m_HeaderReference;                                		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUIPrefabInstance*                           m_KeyBindReference;                               		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUIPrefabInstance*                           m_LineReference;                                  		// 0x00C4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_KeyBindDefaults;                                		// 0x00C8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_CloseButton;                                    		// 0x00CC (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KeyBindInfo;                                    		// 0x00D0 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_KeyBindConfirmPanel;                            		// 0x00D4 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_KeyBindConfirmText;                             		// 0x00D8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_KeyBindConfirmYes;                              		// 0x00DC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_KeyBindConfirmNo;                               		// 0x00E0 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_VideoTabButton;                                 		// 0x00E4 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_SoundTabButton;                                 		// 0x00E8 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_GeneralTabButton;                               		// 0x00EC (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ChatTabButton;                                  		// 0x00F0 (0x0004) [0x0000000000000000]              
	int                                                m_nNumBindings;                                   		// 0x00F4 (0x0004) [0x0000000000000000]              
	int                                                m_CategorySelected;                               		// 0x00F8 (0x0004) [0x0000000000000000]              
	int                                                m_BindSelected;                                   		// 0x00FC (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bBindCapture : 1;                               		// 0x0100 (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bBindConfirm : 1;                               		// 0x0100 (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      m_bAltModifier : 1;                               		// 0x0100 (0x0004) [0x0000000000000000] [0x00000004] 
	unsigned long                                      m_bInfoVisible : 1;                               		// 0x0100 (0x0004) [0x0000000000000000] [0x00000008] 
	struct FName                                       m_ConfirmKeyName;                                 		// 0x0104 (0x0008) [0x0000000000000000]              
	class UPlayerInput*                                m_PlayerInput;                                    		// 0x010C (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1723 ];

		return pClassPointer;
	};

	void eventFixupWidgetsUC ( );
	void eventSetKeyBindSelectDelegate ( class UUIScreenObject* Obj );
	void eventSetBind ( struct FName BindName, struct FString Command, unsigned long altKey );
	void OnBindingsUpdated ( );
	struct FString GetBind ( struct FName Key, unsigned long bAlt );
	struct FString GetPreviousCommandName ( struct FString currentBinding );
	void UnbindCommand ( struct FString Command );
	void CaptureKeyToBind ( struct FName KeyName );
	void DisableBinding ( );
	void EnableBinding ( );
	void CreateWidgetsFromBindData ( );
	void PopulateBindDataArray ( );
	void PopulateWidgets ( );
	void FixupWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
	bool OnTabClickedDelegate ( struct FInputEventParameters* EventParms );
	bool OnCloseButton ( struct FInputEventParameters* EventParms );
	bool OnKeyBindConfirmNo ( struct FInputEventParameters* EventParms );
	bool OnKeyBindConfirmYes ( struct FInputEventParameters* EventParms );
	bool OnKeyBindBase ( struct FInputEventParameters* EventParms );
	bool OnKeyBindDefaults ( struct FInputEventParameters* EventParms );
	bool OnKeyBindMenuKey ( struct FInputEventParameters* EventParms );
};


// Class TgClient.TgUIUnlockScene
// 0x00B0 (0x015C - 0x00AC)
class UTgUIUnlockScene : public UTgUISceneDriver
{
public:
	struct FsUnlockItemPanel                           m_UnlockItem[ 0x5 ];                              		// 0x00AC (0x0078) [0x0000000000000000]              
	int                                                m_NumCurrentUnlocks;                              		// 0x0124 (0x0004) [0x0000000000000000]              
	class UUIPanel*                                    m_UnlockPanel;                                    		// 0x0128 (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_GratsLabel;                                     		// 0x012C (0x0004) [0x0000000000000000]              
	class UUILabel*                                    m_LevelLabel;                                     		// 0x0130 (0x0004) [0x0000000000000000]              
	class UUIButton*                                   m_ProgButton;                                     		// 0x0134 (0x0004) [0x0000000000000000]              
	class UUIImage*                                    m_FrameImage[ 0x3 ];                              		// 0x0138 (0x000C) [0x0000000000000000]              
	class UUIImage*                                    m_BGImage;                                        		// 0x0144 (0x0004) [0x0000000000000000]              
	class ATgPlayerController*                         m_PC;                                             		// 0x0148 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_bNewLevel : 1;                                  		// 0x014C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      m_bRequestSkills : 1;                             		// 0x014C (0x0004) [0x0000000000000000] [0x00000002] 
	int                                                m_nCurrentLevel;                                  		// 0x0150 (0x0004) [0x0000000000000000]              
	class ATgInventoryManager*                         m_InventoryManager;                               		// 0x0154 (0x0004) [0x0000000000000000]              
	struct FPointer                                    m_pSkillMarshal;                                  		// 0x0158 (0x0004) [0x0000000000001002]              ( CPF_Const | CPF_Native )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1724 ];

		return pClassPointer;
	};

	void eventRequestPlayersSkillsFromServer ( class ATgPawn_Character* MyPawn );
	void eventFixupWidgetsUC ( );
	bool OnNavButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnEquipButtonDelegate ( struct FInputEventParameters* EventParms );
	bool OnSceneKeyDelegate ( struct FInputEventParameters* EventParms );
	class ATgPointOfInterest* GetPOIByMsgId ( int msgId );
	void ProcessUnlockData ( );
	void CheckLevelUnlocks ( unsigned long bNewLevel );
	void PopulateUnlockWidgets ( );
	bool TickTgUIScene ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUIVoiceSetup
// 0x001C (0x00C8 - 0x00AC)
class UTgUIVoiceSetup : public UTgUISceneDriver
{
public:
	class UUISlider*                                   m_MicVolume;                                      		// 0x00AC (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_SpeakerVolume;                                  		// 0x00B0 (0x0004) [0x0000000000000000]              
	class UUISlider*                                   m_VADLevel;                                       		// 0x00B4 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_MicMute;                                        		// 0x00B8 (0x0004) [0x0000000000000000]              
	class UUICheckbox*                                 m_SpeakerMute;                                    		// 0x00BC (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_CloseButton;                                    		// 0x00C0 (0x0004) [0x0000000000000000]              
	class UUILabelButton*                              m_ApplyButton;                                    		// 0x00C4 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1725 ];

		return pClassPointer;
	};

	bool OnApplyButtonClick ( struct FInputEventParameters* EventParms );
	bool OnCloseButtonClick ( struct FInputEventParameters* EventParms );
	void eventFixupWidgetsUC ( );
	void FixupTgUIScene ( );
};


// Class TgClient.TgUISceneManager
// 0x0050 (0x008C - 0x003C)
class UTgUISceneManager : public UObject
{
public:
	class UTgGameViewportClient*                       m_ViewportClient;                                 		// 0x003C (0x0004) [0x0000000000000000]              
	class ATgHUD*                                      m_HUDParent;                                      		// 0x0040 (0x0004) [0x0000000000000000]              
	class UUIInteraction*                              m_UIInteraction;                                  		// 0x0044 (0x0004) [0x0000000000000000]              
	class ULocalPlayer*                                m_OwnerPlayer;                                    		// 0x0048 (0x0004) [0x0000000000000000]              
	unsigned long                                      m_SceneMouseState : 1;                            		// 0x004C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned char                                      UnknownData00[ 0x3C ];                            		// 0x0050 (0x003C) UNKNOWN PROPERTY: MapProperty TgClient.TgUISceneManager.m_SceneDriverMap

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1726 ];

		return pClassPointer;
	};

	void TickOpenScenes ( );
	bool ReleaseSceneDriverIfNotOpened ( class UTgUISceneDriver* SceneDriver );
	struct FName GetSceneDriverNameByResource ( struct FString ResourceName );
	class UTgUISceneDriver* GetSceneDriver ( struct FName DriverClass, unsigned long InitOnDemand );
	class UTgUISceneDriver* OpenSceneDriver ( struct FName DriverClass );
};


// Class TgClient.TgEntryHUD
// 0x0000 (0x0464 - 0x0464)
class ATgEntryHUD : public AHUD
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 51253 ];

		return pClassPointer;
	};

	void DrawHUD ( );
};


// Class TgClient.TgHUD_TeamGame
// 0x0004 (0x08C0 - 0x08BC)
class ATgHUD_TeamGame : public ATgHUD_Game
{
public:
	struct FColor                                      m_PlayerColor;                                    		// 0x08BC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 51994 ];

		return pClassPointer;
	};

	void DrawCol ( float X1, float X2, float Y, class ATgRepInfo_Player* prep );
	void DrawTeams ( );
	void DrawHUD ( );
};


// Class TgClient.TgLoginPawn
// 0x0004 (0x03D0 - 0x03CC)
class ATgLoginPawn : public APawn
{
public:
	class ATgLoginInfo*                                m_GameInfo;                                       		// 0x03CC (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 52097 ];

		return pClassPointer;
	};

	void StartUI ( );
	void eventOutsideWorldBounds ( );
	void eventFellOutOfWorld ( class UClass* dmgType );
	void PostBeginPlay ( );
	void eventTick ( float DeltaTime );
};


// Class TgClient.TgLoginPlayerCamera
// 0x0000 (0x0354 - 0x0354)
class ATgLoginPlayerCamera : public ACamera
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 52112 ];

		return pClassPointer;
	};

	void PostBeginPlay ( );
};



#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
