#include "src/GameServer/Engine/GameEngine/Tick/GameEngine__Tick.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/GameServer/Combat/MissionAlerts/MissionAlerts.hpp"
#include "src/GameServer/GameModes/Acquisition/Acquisition.hpp"
#include "src/GameServer/AvaCargo/AvaCargoDll.hpp"
#include "src/GameServer/Combat/CombatMessageFlusher/CombatMessageFlusher.hpp"
#include "src/GameServer/Matchmaking/SetupRebalanceTrigger/SetupRebalanceTrigger.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Engine/KismetWebDump/KismetWebDump.hpp"
#include "src/GameServer/TgGame/MissionVODirector/MissionVODirector.hpp"
#include "src/GameServer/Cosmetics/SuitRebuildKick.hpp"
#include "src/GameServer/TgGame/TgDeviceVolume/setupDevice/TgDeviceVolume__setupDevice.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/Markers/Markers.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/FxBrowse/FxBrowse.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconCarryReaper/BeaconCarryReaper.hpp"
#include "src/GameServer/Inventory/ConsumableReaper/ConsumableReaper.hpp"
#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"
#include "src/GameServer/Utils/FramePacing/FramePacing.hpp"
#include "src/GameServer/Utils/ActorTickLod/ActorTickLod.hpp"
#include "src/GameServer/Utils/BotSight/BotSight.hpp"
#include "src/GameServer/Tutorial/TutorialBotRoster.hpp"
#include "src/GameServer/Tutorial/TutorialChapterEnd.hpp"
#include "src/GameServer/Tutorial/TutorialBoss.hpp"
#include "src/GameServer/WorldGroups/WorldGroups.hpp"
#include "src/GameServer/Maps/MapAdditions/MapAdditions.hpp"
#include "src/GameServer/TgGame/ObjectiveGuide/ObjectiveGuide.hpp"
#include "src/GameServer/TgGame/NewsStand/NewsStand.hpp"
#include "src/GameServer/PvpBots/PvpBots.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnRequestMission/TgPawn__ServerOnRequestMission.hpp"

void __fastcall GameEngine__Tick::Call(void* Engine, void* edx, float DeltaSeconds) {
	// Frame-time probe. No-op unless the "perf" channel is enabled.
	PerfProbe::BeginFrame();
	FramePacing::NoteGameThread();
	ActorTickLod::BeginFrame(DeltaSeconds);
	// Opted-in bot factories: hostile bots notice each other (2 Hz).
	BotSight::Tick(DeltaSeconds);
	// Bot roster log (no-op unless the "tutbots" channel is on).
	TutorialBotRoster::Tick(DeltaSeconds);
	// Chapter maps - fallback mission end if RequestMission never came.
	TutorialChapterEnd::Tick();
	// Skylark boss identify + post-cine safety net.
	TutorialBoss::Tick(DeltaSeconds);
	// Dev-spawned PvP bots: deferred sight kick on their foe.
	PvpBots::Tick(DeltaSeconds);
	// Spawned world groups in a passive return (group-control), ~4 Hz.
	WorldGroups::Tick();
	{ PerfProbe::Scope _p(PerfProbe::SLOT_IPC_DRAIN); IpcClient::DrainInbound(); }
	// MissionAlerts self-throttles to ~1Hz off WorldInfo.TimeSeconds.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_MISSION_ALERTS); MissionAlerts::Tick(); }
	Acquisition::Tick();  // DualCTF robots (1 Hz, no-op elsewhere)
	AvaCargoDll::Tick();  // AvA vehicles (1 Hz, no-op without vehicles)
	// CombatMessageFlusher self-throttles to ~10Hz; drains per-pawn accumulators
	// that the engine's 21-record / 101-index threshold (FUN_109f9bc0) would
	// otherwise hold indefinitely for sparse damage events.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_COMBAT_FLUSH); CombatMessageFlusher::Tick(); }
	// Self-gates on mission timer SETUP state; fires one rebalance request 5s
	// before setup ends.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_REBALANCE_TRIGGER); SetupRebalanceTrigger::Tick(); }
	// Match stats: pending-death flush + objective capture/contest time.
	// Self-throttles to 4Hz; no-op when stats disabled (home map).
	{ PerfProbe::Scope _p(PerfProbe::SLOT_MATCH_STATS); MatchStats::Tick(); }
	// One-shot delayed kismet dump (no-op unless -dumpkismet armed it at
	// BeginPlay). Fires after the sublevel-streaming settle window so the dump
	// includes streamed sublevels (e.g. the *_Sound announcer-VO kismet).
	{ PerfProbe::Scope _p(PerfProbe::SLOT_KISMET_DUMP); KismetWebDump::TickDelayedDump(DeltaSeconds); }
	// Raid_DomeCityDefense_P: fire Ava's per-round satellite-countdown VO
	// (Bancroft_HalfwayPoint/_30sRemaining/_10sRemaining) off the round timer.
	// Self-gates on map name + Defense round state; no-op elsewhere.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_MISSION_VO); MissionVODirector::Tick(DeltaSeconds); }
	// 1P_ missions: server-pushed objective arrow trail (~1Hz poll, 30 s per player).
	ObjectiveGuide::Tick(DeltaSeconds);
	// Dome City Global News stand: news feed in chat when a player walks up (~1Hz poll).
	NewsStand::Tick(DeltaSeconds);
	TgPawn__ServerOnRequestMission::Tick(DeltaSeconds);
	// Deferred post-profile-switch rebuild kick (suit-mesh load-race recovery).
	{ PerfProbe::Scope _p(PerfProbe::SLOT_SUIT_REBUILD); SuitRebuildKick::Tick(DeltaSeconds); }
	// One-shot: verify (and if needed redo) the VR heal pad's PostBeginPlay
	// arming ~5s after setupDevice registered it. No-op on other maps.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_VR_HEAL_PAD); DomeVrHealPad::TickArmCheck(); }
	// -markers highlight refresh. Returns immediately unless some session has
	// markers enabled; otherwise self-throttles to ~0.45Hz.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_MARKERS); TgPlayerActions::MarkersCmd::Tick(); }
	// -fx browser refresh; no-op unless someone is stepping through candidates.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_FX_BROWSE); TgPlayerActions::FxBrowseCmd::Tick(); }
	// Verifies that a deployed beacon's carry device actually left slot 11.
	// Returns immediately unless a deploy armed a record in the last few
	// seconds; the normal DeviceFiring.EndState cleanup disarms it first, so
	// this only ever acts when that path was missed.
	{ PerfProbe::Scope _p(PerfProbe::SLOT_BEACON_REAPER); BeaconCarryReaper::Tick(DeltaSeconds); }
	// Takes used consumables out of their slot; returns immediately when idle.
	ConsumableReaper::Tick(DeltaSeconds);
	// Hand-authored boss alarms; returns immediately unless the map registered one.
	MapAdditions::Tick();
	{ PerfProbe::Scope _p(PerfProbe::SLOT_ENGINE_TICK); CallOriginal(Engine, edx, DeltaSeconds); }
	PerfProbe::EndFrame();
}
