#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <math.h>
#include <unordered_map>

// TgPawn__SetProperty @ 0x109bf420 — __thiscall(pawn, nPropertyId, fNewValue)
// Called as __fastcall with dummy EDX to avoid inline asm.
static void SetPawnProperty(ATgPawn* Pawn, int nPropertyId, float fNewValue) {
	((void(__fastcall*)(ATgPawn*, void*, int, float))0x109bf420)(Pawn, nullptr, nPropertyId, fNewValue);
}

// True if any of this device's fire modes carries a Stealth-category (621)
// effect group. Used by the per-refire HUD timer refresh and the
// ServerStopFire immediate-teardown path.
//
// Forces lazy population of s_EffectGroupList if empty — first activation
// has the list empty until UC's fire path triggers GetEffectGroup. Calling
// our hook with sentinel nType=-1 fills it as a side effect.
//
// Memoized per-ATgDevice* via `s_stealthDeviceCache`. RefireCheckTimer fires
// while the trigger is held (every 50-100 ms), and before memoization this
// walked m_FireMode + s_EffectGroupList on every tick. The fire-mode roster
// and per-mode effect-group list are immutable for a device's lifetime, so
// after the first call the answer is stable. Cache holds only `true` entries
// to keep the lazy-population branch live on first call (entries fall through
// to the full walk until s_EffectGroupList has been populated and a 621
// category is detected). False results aren't cached — they're cheap to
// recompute and a future fire mode add (we don't currently do this, but
// defensive) would still take effect.
static std::unordered_map<ATgDevice*, bool> s_stealthDeviceCache;

static bool HasStealthEffectGroup(ATgDevice* Device) {
	if (!Device) return false;
	auto it = s_stealthDeviceCache.find(Device);
	if (it != s_stealthDeviceCache.end()) return it->second;

	for (int m = 0; m < Device->m_FireMode.Count; m++) {
		UTgDeviceFire* fireMode = Device->m_FireMode.Data[m];
		if (!fireMode) continue;
		if (fireMode->s_EffectGroupList.Count == 0) {
			int idx = 0;
			TgDeviceFire__GetEffectGroup::Call(fireMode, nullptr, -1, &idx);
		}
		for (int i = 0; i < fireMode->s_EffectGroupList.Count; i++) {
			UTgEffectGroup* eg = fireMode->s_EffectGroupList.Data[i];
			if (eg && eg->m_nCategoryCode == 621) {
				s_stealthDeviceCache[Device] = true;
				return true;
			}
		}
	}
	return false;
}

// Native TgEffectManager::RefreshEffectRep @ 0x10a6efd0
// __thiscall(this, slot, fInitTimeRemaining). Writes the replicated HUD slot
// (r_ManagedEffectList[slot].fInitTimeRemaining), bumps the upper-16-bit
// generation counter in nExtraInfo so the client sees the refresh even if
// the float value hasn't changed, and sets bNetDirty.
typedef void (__fastcall *RefreshEffectRepFn)(ATgEffectManager*, void*, unsigned int, float);
static const RefreshEffectRepFn RefreshEffectRepNative =
	(RefreshEffectRepFn)0x10a6efd0;

// Refresh every active Stealth-category effect group on `Pawn` back to its
// full lifetime. Called from a continuous-fire signal so the short-lived
// (1s) managed effect keeps getting a fresh timer while the fire button is
// held. When the button releases, the refresh stops and the existing timer
// expires in ≤1s (the ServerStopFire hook tears it down immediately anyway).
//
// Refresh = zero the timer entry's Count (elapsed-time accumulator) so it
// re-arms for another full Rate. Uses SDK's FTimerData struct members
// directly rather than raw offsets.
static void RefreshStealthEffectTimers(ATgPawn* Pawn) {
	if (!Pawn) return;
	ATgEffectManager* Mgr = Pawn->r_EffectManager;
	if (!Mgr) return;

	for (int i = 0; i < Mgr->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* g = Mgr->s_AppliedEffectGroups.Data[i];
		if (!g || g->m_nCategoryCode != 621) continue;

		// Reset the LifeDone timer's elapsed accumulator (FTimerData::Count)
		// for this group.
		for (int t = 0; t < Mgr->Timers.Count; t++) {
			FTimerData& timer = Mgr->Timers.Data[t];
			if (timer.TimerObj == (UObject*)g) {
				timer.Count = 0.0f;
			}
		}

		// Push the refreshed countdown to the client HUD slot.
		int slot = g->s_ManagedEffectListIndex;
		if (slot >= 0 && slot < 0x10) {
			Mgr->m_fTimeRemaining[slot] = g->m_fLifeTime;
			RefreshEffectRepNative(Mgr, nullptr, (unsigned int)slot, g->m_fLifeTime);
		}
	}
}

// Carrier-loss cleanup. Called when a beacon-carrying pawn dies or its
// controlling PlayerController is destroyed (disconnect). For each team
// manager whose `r_BeaconHolder` matches this pawn's PRI, remove the
// pickup device (id 1918) from the carrier's slot 11 and re-trigger
// CheckBeacon — the native walks all PRIs, finds no IsCarryingBeacon
// holder, and re-spawns at the team's original-priority factory.
//
// Safe to invoke for any pawn — bails fast when no manager matches.
static void DropCarriedBeaconIfAny(ATgPawn* Pawn) {
	if (!Pawn || !Pawn->PlayerReplicationInfo) return;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	ATgTeamBeaconManager* managers[2] = {
		(GTeamsData.Attackers ? GTeamsData.Attackers->r_BeaconManager : nullptr),
		(GTeamsData.Defenders ? GTeamsData.Defenders->r_BeaconManager : nullptr),
	};
	for (ATgTeamBeaconManager* mgr : managers) {
		if (!mgr || mgr->r_BeaconHolder != pri) continue;

		Logger::Log("beacon",
			"DropCarriedBeaconIfAny: pawn=0x%p pri=0x%p was carrier for mgr=0x%p — clearing slot 11 + CheckBeacon\n",
			Pawn, pri, mgr);

		// Inventory device removal first — UC TgDevice.uc:677 invokes
		// CheckBeacon on inventory change, but we follow up with an explicit
		// call to guarantee the respawn fires even if the inventory hook
		// path doesn't trigger during the Dying / Destroyed teardown.
		ATgInventoryManager* invMgr = (ATgInventoryManager*)Pawn->InvManager;
		if (invMgr) {
			TgInventoryManager__NonPersistRemoveDevice::Call(invMgr, nullptr, 11);
		}

		// Whether or not the inventory remove succeeded, we still need
		// CheckBeacon to re-evaluate. bAttemptRespawn=true so it spawns
		// at the original-priority factory if nobody else is carrying.
		BeaconSdk::CheckBeacon(mgr, true);
	}
}

// (Former NativeApplyEffect / NativeRemoveEffect / ComputeEffectDelta helpers
// were removed — see the commented-out TgEffect.ApplyEffect/Remove branches
// in Call() below for the reasoning. UC owns the apply/remove math now; we
// just promote stealth-category groups to managed lifetime, refresh their
// timers on continuous fire, and tear down explicitly on fire-release.)
//
// Buff-routing for class-157 effects splits across two hook sites:
//  - Apply: `TgEffectManager__SetEffectRep::Call` (post-original) — the
//    apply chain doesn't surface in our ProcessEvent hook for reasons
//    documented in `ApplyBuffEffect.hpp`, but SetEffectRep does fire
//    reliably and arrives with `s_AppliedEffectGroups` populated.
//  - Remove: the `Function TgGame.TgEffect.Remove` branch below — our
//    reimplemented `TgEffectGroup__RemoveEffects` explicitly dispatches
//    `eventRemove` via ProcessEvent for each effect, so this branch fires
//    symmetrically with the SetEffectRep apply.

// One of these tags is computed for each UFunction the first time we see it
// and cached forever after. UFunction* is stable for the process lifetime,
// so a pointer-keyed cache lets us replace the historical strcmp cascade
// over Function->GetFullName() with an O(1) hash probe.
enum class DispatchTag : uint8_t {
	Unknown = 0,                  // catch-all (log + CallOriginal)
	EarlyOut,                     // pass-through: just CallOriginal
	RefireCheckTimer,             // independent stealth-refresh side effect, then catch-all
	TgGameLogin,                  // CallOriginal then clear PRI spectator flags
	TgGamePostLogin,              // Pre-seed PRI.r_TaskForce, then CallOriginal
	DyingBeginState,              // CallOriginal then BotDied
	DeviceFiringEndState,         // CallOriginal then jetpack-flag clear
	ServerStopFire,               // CallOriginal then RemoveEffectGroupsByCategory(stealth)
	TgEffectRemove,               // DoCatchAll — TgEffectBuff.Remove now reverses buffs canonically
	PostPawnSetup,                // CallOriginal (sets PHYS_Falling) then restore PHYS_Flying for flying bots
	ServerStartFire,              // Diagnostic: log every CanDeviceFireNow gate before CallOriginal
	GetPlayerViewPoint,           // Pre-sync c_nCameraYawOffset / c_nCameraPitchOffset from ctrl.Rotation
	BeaconPickUpDeployable,       // Diagnostic: log gate values before CallOriginal + return after
	PawnPickupNearestDeployable,  // Diagnostic: log TouchingActors walk + which deployable was picked
	BeaconEntranceHasExit,        // Post-call: override return to false when r_Beacon->m_bIsDeployed=0
	PlayerControllerDestroyed,    // Pre-call: drop carried beacon if any, then CheckBeacon respawn
	ServerPickupPutdownDeployableTag, // Diagnostic: log RPC entry to confirm key press reaches server
	// GameTimerDiagnostic,          // Diagnostic-only: before/after snapshots around original timer/match flow
};

static bool IsGameTimerDiagnosticFunction(const char* name) {
	if (!name) return false;

	if (strcmp(name, "Function TgGame.TgGame.StartGameTimer") == 0 ||
	    strcmp(name, "Function TgGame.TgGame_Arena.StartGameTimer") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.MissionTimer") == 0 ||
	    strcmp(name, "Function TgGame.TgGame_Arena.MissionTimer") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.ChangeTimerState") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.SetMissionTime") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.MissionTimerStart") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.MissionTimerStop") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.SendMissionTimerEvent") == 0 ||
	    strcmp(name, "Function TgGame.TgGame.SendMissionTimerNotify") == 0 ||
	    strcmp(name, "Function TgGame.TgGame_Arena.GameStarted") == 0 ||
	    strcmp(name, "Function TgGame.TgGame_Arena.PreRoundFinished") == 0 ||
	    strcmp(name, "Function TgGame.TgGame_Arena.NextRoundStart") == 0 ||
	    strcmp(name, "Function TgGame.TgGame_Defense.RoundTimer") == 0) {
		return true;
	}

	if (strstr(name, ".PreRound.BeginState") ||
	    strstr(name, ".RoundInProgress.BeginState") ||
	    strstr(name, ".PostRound.BeginState") ||
	    strstr(name, ".GameRunning.BeginState")) {
		return true;
	}

	return false;
}

static void LogGameTimerSnapshot(const char* phase, UObject* Object, UFunction* Function) {
	if (!Logger::IsChannelEnabled("gametimer")) return;

	std::string functionName = Function ? Function->GetFullName() : "<null-function>";
	std::string objectName = Object ? Object->GetFullName() : "<null-object>";
	std::string objectClass = (Object && Object->Class) ? Object->Class->GetFullName() : "<null-class>";

	ATgGame* Game = nullptr;
	if (Object && objectClass.find("TgGame") != std::string::npos) {
		Game = (ATgGame*)Object;
	} else if (Globals::Get().GGameInfo) {
		Game = (ATgGame*)Globals::Get().GGameInfo;
	}

	if (!Game) {
		Logger::Log("gametimer",
			"[%s] %s obj=%s class=%s game=<null>\n",
			phase, functionName.c_str(), objectName.c_str(), objectClass.c_str());
		return;
	}

	std::string gameName = ((UObject*)Game)->GetFullName();
	std::string gameClass = Game->Class ? Game->Class->GetFullName() : "<null-game-class>";
	std::string stateName = Game->GetStateName().GetName();

	ATgRepInfo_Game* GRI = Game->GameReplicationInfo
		? (ATgRepInfo_Game*)Game->GameReplicationInfo
		: nullptr;

	Logger::Log("gametimer",
		"[%s] %s obj=%s objClass=%s game=%s gameClass=%s state=%s "
		"wait=%d delayed=%d ended=%d timerState=%d pausedState=%d shouldWait=%d allowOT=%d winState=%d gameType=%d "
		"mission=%.2f gameMission=%.2f overtime=%.2f startedAt=%.2f worldTime=%.2f "
		"GRI{ptr=%p matchOver=%d round=%d/%d mtState=%d mtChange=%d rem=%.2f cTime=%.2f cSecs=%.2f remaining=%d limit=%d flags raid=%d mission=%d arena=%d match=%d overtime=%d}\n",
		phase,
		functionName.c_str(),
		objectName.c_str(),
		objectClass.c_str(),
		gameName.c_str(),
		gameClass.c_str(),
		stateName.c_str(),
		(int)Game->bWaitingToStartMatch,
		(int)Game->bDelayedStart,
		(int)Game->bGameEnded,
		(int)Game->m_eTimerState,
		(int)Game->m_eTimerStatePaused,
		(int)Game->m_bShouldWait,
		(int)Game->m_bAllowOvertime,
		(int)Game->m_GameWinState,
		(int)Game->m_GameType,
		Game->m_fMissionTime,
		Game->m_fGameMissionTime,
		Game->m_fGameOvertimeTime,
		Game->s_fMissionTimerStartedAt,
		Game->WorldInfo ? Game->WorldInfo->TimeSeconds : -1.0f,
		GRI,
		GRI ? (int)GRI->bMatchIsOver : -1,
		GRI ? GRI->r_nRoundNumber : -1,
		GRI ? GRI->r_nMaxRoundNumber : -1,
		GRI ? (int)GRI->r_nMissionTimerState : -1,
		GRI ? GRI->r_nMissionTimerStateChange : -1,
		GRI ? GRI->r_fMissionRemainingTime : -1.0f,
		GRI ? GRI->c_fMissionTime : -1.0f,
		GRI ? GRI->c_fMissionTimeSeconds : -1.0f,
		GRI ? GRI->RemainingTime : -1,
		GRI ? GRI->TimeLimit : -1,
		GRI ? (int)GRI->r_bIsRaid : -1,
		GRI ? (int)GRI->r_bIsMission : -1,
		GRI ? (int)GRI->r_bIsArena : -1,
		GRI ? (int)GRI->r_bIsMatch : -1,
		GRI ? (int)GRI->r_bInOverTime : -1);

	if (gameClass.find("TgGame_Arena") != std::string::npos ||
	    gameClass.find("TgGame_Defense") != std::string::npos ||
	    gameClass.find("TgGame_CTF") != std::string::npos ||
	    gameClass.find("TgGame_PointRotation") != std::string::npos) {
		ATgGame_Arena* Arena = (ATgGame_Arena*)Game;
		Logger::Log("gametimer",
			"[%s]   ArenaFields round=%d betweenDelay=%d setup=%d objectiveUnlock=%d resetPlayfield=%d resetPlayers=%d displayEnd=%d\n",
			phase,
			Arena->s_nRoundNumber,
			Arena->s_nBetweenRoundDelay,
			Arena->s_nRoundSetupTime,
			Arena->s_nObjectiveUnlockDelay,
			(int)Arena->s_bResetPlayfieldBetweenRounds,
			(int)Arena->s_bResetPlayersBetweenRounds,
			(int)Arena->s_bDisplayEndRoundScreen);
	}

	if (gameClass.find("TgGame_Defense") != std::string::npos) {
		ATgGame_Defense* Defense = (ATgGame_Defense*)Game;
		Logger::Log("gametimer",
			"[%s]   DefenseFields maxRound=%d roundDuration=%.2f waveSpawners=%d\n",
			phase,
			Defense->s_nMaxRoundNumber,
			Defense->s_fRoundDuration,
			Defense->s_WaveSpawnerList.Count);
	}
}

// First-sight classification: walks the strcmp ladder once per unique
// UFunction. Returns the matching DispatchTag (or Unknown for the catch-all
// path). Order kept matching the historical hand-written branches so the
// behavior of any function name not listed here is unchanged.
static DispatchTag ClassifyFunction(UFunction* fn) {
	const char* rawName = fn->GetFullName();
	if (!rawName) return DispatchTag::Unknown;
	const std::string nameString = rawName;
	const char* name = nameString.c_str();

	// Hottest set first: per-tick / per-move noise that we want to pass straight through.
	if (   strcmp(name, "Function Engine.Actor.Tick") == 0
		|| strcmp(name, "Function Engine.GameInfoDataProvider.ProviderInstanceBound") == 0
		|| strcmp(name, "Function TgGame.TgPawn.ShouldRechargePowerPool") == 0
		|| strcmp(name, "Function TgGame.TgPawn_Character.Tick") == 0
		|| strcmp(name, "Function TgGame.TgDeployable.Tick") == 0
		|| strcmp(name, "Function Engine.PlayerController.SendClientAdjustment") == 0
		|| strcmp(name, "Function TgGame.TgMissionObjective_Proximity.Tick") == 0
		|| strcmp(name, "Function Engine.PlayerController.ServerMove") == 0
		|| strcmp(name, "Function Engine.PlayerController.OldServerMove") == 0
		|| strcmp(name, "Function TgGame.TgMissionObjective.IsLocalPlayerAttacker") == 0
		|| strcmp(name, "Function TgGame.TgProperty.Copy") == 0
		|| strcmp(name, "Function TgGame.TgDeploy_BeaconEntrance.Touch") == 0
		|| strcmp(name, "Function TgGame.TgMissionObjective_Bot.Tick") == 0
		|| strcmp(name, "Function Engine.Actor.PreBeginPlay") == 0
		|| strcmp(name, "Function Engine.Actor.SetInitialState") == 0
		|| strcmp(name, "Function Engine.Actor.PostBeginPlay") == 0
		|| strcmp(name, "Function Engine.Emitter.PostBeginPlay") == 0
		|| strcmp(name, "Function Engine.LadderVolume.PostBeginPlay") == 0
		|| strcmp(name, "Function Engine.KAsset.PostBeginPlay") == 0
		|| strcmp(name, "Function TgGame.TgRepInfo_Player.Timer") == 0
		|| strcmp(name, "Function TgGame.GameRunning.Timer") == 0
		|| strcmp(name, "Function Engine.GameReplicationInfo.Timer") == 0
		|| strcmp(name, "Function TgGame.TgPawn.GetCameraValues") == 0
		|| strcmp(name, "Function TgGame_Defense.RoundInProgress.Tick") == 0
		|| strcmp(name, "Function Engine.Actor.Touch") == 0
		|| strcmp(name, "Function TgGame.TgDeploy_BeaconEntrance.RecheckActiveTimer") == 0
		|| strcmp(name, "Function TgGame_Arena.RoundInProgress.Tick") == 0
		|| strcmp(name, "Function TgPawn.Dying.Tick") == 0
		|| strcmp(name, "Function Engine.GameInfo.Timer") == 0
		|| strcmp(name, "Function TgGame.TgRepInfo_Game.ServerUpdateTimer") == 0
		|| strcmp(name, "Function TgGame.TgPawn.IsCrewed") == 0
		|| strcmp(name, "Function TgGame.TgDevice.IsOffhand") == 0
		|| strcmp(name, "Function TgGame.TgAIController.SeePlayer") == 0
		|| strcmp(name, "Function TgGame.TgSkeletalMeshActorNPC.Tick") == 0
		|| strcmp(name, "Function TgGame.TgSkeletalMeshActor_CharacterBuilder.Tick") == 0
		|| strcmp(name, "Function TgGame.TgInterpolatingCameraActor.Tick") == 0)
	{
		return DispatchTag::EarlyOut;
	}

	// Specific handlers.
	if (strcmp(name, "Function TgDevice.DeviceFiring.RefireCheckTimer") == 0)        return DispatchTag::RefireCheckTimer;
	if (strcmp(name, "Function TgGame.TgGame.Login") == 0)                           return DispatchTag::TgGameLogin;
	if (strcmp(name, "Function TgGame.TgGame.PostLogin") == 0)                       return DispatchTag::TgGamePostLogin;
	if (strcmp(name, "Function TgPawn.Dying.BeginState") == 0)                       return DispatchTag::DyingBeginState;
	if (strcmp(name, "Function TgDevice.DeviceFiring.EndState") == 0)                return DispatchTag::DeviceFiringEndState;
	if (strcmp(name, "Function TgGame.TgDevice.ServerStopFire") == 0)                return DispatchTag::ServerStopFire;
	if (strcmp(name, "Function TgGame.TgDevice.ServerStartFire") == 0)               return DispatchTag::ServerStartFire;
	if (strcmp(name, "Function TgGame.TgEffect.Remove") == 0)                        return DispatchTag::TgEffectRemove;
	if (strcmp(name, "Function TgGame.TgPawn.WaitForInventoryThenDoPostPawnSetup") == 0) return DispatchTag::PostPawnSetup;
	if (strcmp(name, "Function TgGame.TgPlayerController.GetPlayerViewPoint") == 0)  return DispatchTag::GetPlayerViewPoint;
	if (strcmp(name, "Function TgGame.TgDeploy_Beacon.PickUpDeployable") == 0)        return DispatchTag::BeaconPickUpDeployable;
	if (strcmp(name, "Function TgGame.TgPawn.PickupNearestDeployable") == 0)         return DispatchTag::PawnPickupNearestDeployable;
	if (strcmp(name, "Function TgGame.TgDeploy_BeaconEntrance.HasExit") == 0)        return DispatchTag::BeaconEntranceHasExit;
	if (strcmp(name, "Function Engine.PlayerController.Destroyed") == 0)             return DispatchTag::PlayerControllerDestroyed;
	if (strcmp(name, "Function TgGame.TgPlayerController.ServerPickupPutdownDeployable") == 0) return DispatchTag::ServerPickupPutdownDeployableTag;
	// if (IsGameTimerDiagnosticFunction(name)) return DispatchTag::GameTimerDiagnostic;

	return DispatchTag::Unknown;
}

// Pointer-keyed cache: every (UFunction*) maps to a tag forever. UC functions
// are loaded once and don't move, so this is safe. First call for a given fn
// pays the strcmp ladder above + one hash insert; every subsequent call is a
// single hash lookup. After the first ~second of warm-up the cache is
// populated for everything the game tick touches.
//
// Single-threaded by assumption — same as the rest of this hook (the prior
// implementation's `Logger::ChannelIndents[..]++` would already race on
// concurrent ProcessEvent invocations). UE3 ProcessEvent fires from the game
// thread.
static std::unordered_map<UFunction*, DispatchTag> s_DispatchCache;

static DispatchTag GetDispatchTag(UFunction* fn) {
	auto it = s_DispatchCache.find(fn);
	if (it != s_DispatchCache.end()) return it->second;
	const DispatchTag tag = ClassifyFunction(fn);
	s_DispatchCache.emplace(fn, tag);
	return tag;
}

// "scope" channel investigation — smooth scope-zoom transition broke into an
// instant snap somewhere around 2026-05-29/30. The smooth-zoom path is
// entirely client-side (Pawn.Tick → ManageZoomingClientSide → ClientZoomIn →
// TickZoom lerp), but the SERVER drives r_bAimingMode + applies type-266 aim
// effect group + may write replicated fields the client camera reads. This
// instrumentation captures every server-side UC call that touches the aim
// path AND every direct write to scope-relevant replicated fields, all on
// ONE dedicated "scope" channel so the user can hand back a single
// continuous timeline.
//
// Cache: first call for any UFunction pays a name lookup + substring check;
// subsequent calls are O(1) hash lookup. UFunction pointers are stable.
static std::unordered_map<UFunction*, bool> s_ScopeRelatedCache;

static bool IsScopeRelated(UFunction* fn) {
	auto it = s_ScopeRelatedCache.find(fn);
	if (it != s_ScopeRelatedCache.end()) return it->second;
	const char* raw = fn->GetFullName();
	const std::string name = raw ? raw : "";
	const bool match =
		name.find("ServerUpdateAimingMode")     != std::string::npos ||
		name.find("ServerUpdateSnipeScopeMode") != std::string::npos ||
		name.find("ApplyAimEffects")            != std::string::npos ||
		name.find("RemoveAimEffects")           != std::string::npos ||
		name.find("EnterAimingMode")            != std::string::npos ||
		name.find("ExitAimingMode")             != std::string::npos ||
		name.find("OnAimingModeChange")         != std::string::npos ||
		name.find("ServerSetBinoculars")        != std::string::npos ||
		name.find("ManageZoomingClientSide")    != std::string::npos ||
		name.find("ClientZoomIn")               != std::string::npos ||
		name.find("ClientZoomOut")              != std::string::npos ||
		name.find("TickZoom")                   != std::string::npos ||
		name.find("AdjustFOVAngle")             != std::string::npos ||
		name.find("ReplicatedEvent")            != std::string::npos ||
		name.find("PostNetReceive")             != std::string::npos ||
		name.find("ApplyPawnSetup")             != std::string::npos ||
		name.find("WaitForInventoryThenDoPostPawnSetup") != std::string::npos;
	s_ScopeRelatedCache.emplace(fn, match);
	return match;
}

// Resolve a TgPawn pointer from the ProcessEvent Object for state logging.
// Object could be the pawn itself, a TgDevice (Instigator is the pawn), or
// a TgPlayerController (Pawn member). Returns null if we can't find one.
static ATgPawn* ResolvePawnForScopeLog(UObject* Object) {
	if (!Object) return nullptr;
	const char* clsRaw = Object->Class ? Object->Class->GetFullName() : nullptr;
	if (!clsRaw) return nullptr;
	const std::string clsName = clsRaw;
	if (clsName.find("TgPawn") != std::string::npos) {
		return (ATgPawn*)Object;
	}
	if (clsName.find("TgDevice") != std::string::npos) {
		ATgDevice* dev = (ATgDevice*)Object;
		return (ATgPawn*)dev->Instigator;
	}
	if (clsName.find("PlayerController") != std::string::npos) {
		APlayerController* pc = (APlayerController*)Object;
		return (ATgPawn*)pc->Pawn;
	}
	return nullptr;
}

// Per-call snapshot for the scope channel. Captures the field state of
// interest BEFORE the UC body runs; the caller emits a paired AFTER log.
static void LogScopeCall(const char* phase, UObject* Object, UFunction* Function) {
	const char* fnRaw  = Function->GetFullName();
	const std::string fnName = fnRaw ? fnRaw : "<null-fn>";
	const char* objRaw = Object->GetFullName();
	const std::string objName = objRaw ? objRaw : "<null-obj>";

	ATgPawn* pawn = ResolvePawnForScopeLog(Object);
	if (pawn) {
		ATgDevice* wpn = (ATgDevice*)pawn->Weapon;
		const char* wpnRaw = wpn && wpn->Class ? wpn->Class->GetFullName() : nullptr;
		const std::string wpnName = wpnRaw ? wpnRaw : "<no-weapon>";
		Logger::Log("scope",
			"[%s] %s on %s  r_bAimingMode=%d r_bIsInSnipeScope=%d r_bUsingBinoculars=%d "
			"weapon=%s r_nBodyMeshAsmId=%d\n",
			phase, fnName.c_str(), objName.c_str(),
			(int)pawn->r_bAimingMode, (int)pawn->r_bIsInSnipeScope,
			(int)pawn->r_bUsingBinoculars,
			wpnName.c_str(), pawn->r_nBodyMeshAsmId);
	} else {
		Logger::Log("scope", "[%s] %s on %s  (no resolvable pawn)\n",
			phase, fnName.c_str(), objName.c_str());
	}
}

void __fastcall UObject__ProcessEvent::Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {
	if (!Object || !Function) return;

	const DispatchTag tag = GetDispatchTag(Function);

	// Scope-zoom investigation: log BEFORE every scope-related UC call.
	// AFTER is logged after the switch dispatch completes (see end of fn).
	// Gated on channel-enabled so production cost is one hash lookup + one
	// branch. See the IsScopeRelated comment block for context.
	const bool scopeLog = Logger::IsChannelEnabled("scope") && IsScopeRelated(Function);
	if (scopeLog) LogScopeCall("BEFORE", Object, Function);

	// Per-fire-tick refresh for hold-to-sustain stealth. Independent of the
	// main dispatch — fires alongside whatever the catch-all does for this
	// function (RefireCheckTimer is not in EarlyOut and has no specific
	// handler beyond this side effect, so it lands in the catch-all path).
	//
	// The UC state machine's DeviceFiring state arms RefireCheckTimer on a
	// recurring timer while the fire button is held; it stops firing the
	// moment the button releases. That's exactly the "while firing" signal
	// we need, and it fires at the device's refire rate (faster than our 1s
	// effect lifetime for any sane stealth device).
	//
	// Previous attempt hooked TgPawn.ApplyStealth but that UC event only
	// fires when SetProperty(124) is called, which happens only on fire-
	// start for application_value=156 stealth effect groups (UC's
	// "Newest Wins" re-submit path discards duplicates instead of
	// re-applying) — so the refresh never triggered and the effect
	// expired at 1s regardless of hold duration.
	if (tag == DispatchTag::RefireCheckTimer) {
		ATgDevice* Device = (ATgDevice*)Object;
		if (Device && Device->Instigator && HasStealthEffectGroup(Device)) {
			RefreshStealthEffectTimers((ATgPawn*)Device->Instigator);
		}
	}

	// Catch-all behavior shared by Unknown, RefireCheckTimer (after the side
	// effect above), and the inner-guard-fail paths in TgEffectRemove /
	// CheckEffectBuffModifier. The only consumer of Function->GetFullName()
	// and Object->GetFullName() left in this file lives here, so we gate
	// the GetFullName + std::string copies behind a channel-enabled check —
	// in production "hook_calltree" is off and this whole block degrades to
	// a single CallOriginal with no allocations.
	auto DoCatchAll = [&]() {
		if (Logger::IsChannelEnabled(GetLogChannel())) {
			// Two GetFullName() calls in a single Log line would clobber each
			// other (shared per-thread buffer in the engine's name-assembly
			// code), so copy each into its own std::string first.
			std::string n  = Function->GetFullName();
			std::string on = Object->GetFullName();
			Logger::Log(GetLogChannel(), "├─ %s [%s]\n", n.c_str(), on.c_str());
			Logger::IndentChannel(GetLogChannel(), +1);
			CallOriginal(Object, edx, Function, Params, Result);
			Logger::IndentChannel(GetLogChannel(), -1);
		} else {
			CallOriginal(Object, edx, Function, Params, Result);
		}
	};

	switch (tag) {
	case DispatchTag::EarlyOut:
		CallOriginal(Object, edx, Function, Params, Result);
		break;

	// case DispatchTag::GameTimerDiagnostic:
	// 	LogGameTimerSnapshot("before", Object, Function);
	// 	CallOriginal(Object, edx, Function, Params, Result);
	// 	LogGameTimerSnapshot("after", Object, Function);
	// 	break;

	case DispatchTag::GetPlayerViewPoint: {
		// Sync c_nCameraYawOffset / c_nCameraPitchOffset from Controller.Rotation
		// before the UC body runs.
		//
		// Why: TgPlayerController.GetPlayerViewPoint dispatches to native
		// CalcCameraView (0x109692c0 -> FUN_10967d30). The pawn-view-target
		// branch outputs POVRotation = Controller.Rotation ONLY when the PC's
		// state is PlayerWalking or PlayerHackingBot. For any other state
		// (PlayerJetting, PlayerFlying, PlayerHanging, ...) it falls through
		// to a branch that writes:
		//   POVRotation.Yaw   = this->c_nCameraYawOffset    (offset 0x7B0)
		//   POVRotation.Pitch = this->c_nCameraPitchOffset  (offset 0x7B4)
		//
		// Those offsets are normally refreshed by `state PlayerJetting.PlayerMove`
		// and `.UpdateRotation` via `PlayerInput.aTurn`/`aLookUp` — but
		// PlayerInput is client-side; on the dedicated server they stay
		// frozen at whatever AcknowledgePossession seeded (Pawn.Rotation.Yaw
		// at first possession, ~0 for pitch). Result: while the jetpack is
		// firing, GetBaseAimRotation reads a stale POVRotation, the
		// reticle trace points the wrong way, and projectiles spawn with
		// the wrong rotation regardless of where the player is aiming.
		//
		// Fix: keep these offsets sync'd with the controller's actual
		// rotation right before any code path that consumes them. We do it
		// here (rather than per-Tick) so the cost is only paid when somebody
		// actually reads the view point, and the sync is fresh against the
		// most-recent ServerMove update.
		ATgPlayerController* PC = (ATgPlayerController*)Object;
		if (PC) {
			PC->c_nCameraYawOffset   = PC->Rotation.Yaw;
			PC->c_nCameraPitchOffset = PC->Rotation.Pitch;
		}
		CallOriginal(Object, edx, Function, Params, Result);
		break;
	}

	case DispatchTag::TgGameLogin:
		// GameInfo.Login (called via super.Login from TgGame.Login) takes the
		// spectator branch when ChangeTeam returns false (TgGame.ChangeTeam
		// hardcoded false). It sets PRI.bOnlySpectator/bIsSpectator/bOutOfLives
		// = true on the new PC.
		//
		// SpawnPlayActor immediately calls eventPostLogin → TgGame.RestartPlayer,
		// which gates on bOnlySpectator and returns without spawning. Without
		// intervention the player is permanently stuck as spectator.
		//
		// Fix: clear those flags right after Login returns, BEFORE
		// SpawnPlayActor's subsequent eventPostLogin runs. The first PostLogin
		// then takes the non-spectator branch and spawns naturally — no need
		// for a redundant second eventPostLogin call from HandlePlayerConnected,
		// and no replication race that lands the client in SpectatingMatch (which
		// fires ServerSetViewTarget(none) on its BeginState and locks server-side
		// PC.ViewTarget = self, breaking aim until respawn).
		CallOriginal(Object, edx, Function, Params, Result);
		if (Params) {
			// Parms layout (ATgGame_eventLogin_Parms / AGameInfo_eventLogin_Parms):
			//   0x00 Portal (FString)
			//   0x0C Options (FString)
			//   0x18 ErrorMessage (FString)
			//   0x24 ReturnValue (APlayerController*)
			APlayerController* PC = *(APlayerController**)((char*)Params + 0x24);
			if (PC && PC->PlayerReplicationInfo) {
				PC->PlayerReplicationInfo->bOnlySpectator = 0;
				PC->PlayerReplicationInfo->bIsSpectator   = 0;
				PC->PlayerReplicationInfo->bOutOfLives    = 0;
			}
			// r_TaskForce seeding lives in the TgGamePostLogin case below
			// rather than here. At Login-return time the engine has not yet
			// wired NewPC->Player to the InPlayer connection (verified
			// empirically — PC->Player reads as 0x00000000 in this intercept),
			// so we cannot resolve the player's task_force from
			// GClientConnectionsData via PC->Player. The engine assigns
			// PC->Player between Login returning and PostLogin being called,
			// so PostLogin (which then runs super.PostLogin → RestartPlayer
			// → FindPlayerStart) is the right window for the seed.
		}
		break;

	case DispatchTag::TgGamePostLogin: {
		// Parms layout (AGameInfo_eventPostLogin_Parms / ATgGame_eventPostLogin_Parms):
		//   0x00 NewPlayer (APlayerController*)
		//
		// Seed PRI.r_TaskForce BEFORE super.PostLogin (called from the UC
		// body of TgGame.PostLogin) triggers RestartPlayer → FindPlayerStart.
		// Without this seed the picker reads playerTaskForce=0, fails every
		// team filter, and lands the player in the wrong spawn room.
		// SpawnPlayerCharacter writes the same field later
		// (TgGame__SpawnPlayerCharacter.cpp:295) but by then the spawn point
		// has already been chosen, so the wrong-room result persists until
		// the player dies and respawns.
		if (Params) {
			APlayerController* NewPlayer = *(APlayerController**)Params;
			if (NewPlayer && NewPlayer->PlayerReplicationInfo && NewPlayer->Player) {
				ATgRepInfo_Player* repInfo = (ATgRepInfo_Player*)NewPlayer->PlayerReplicationInfo;
				int32_t connectionIndex = (int32_t)((UNetConnection*)NewPlayer->Player);
				int tf = GClientConnectionsData[connectionIndex].PlayerInfo.task_force;
				ATgRepInfo_TaskForce* taskforce = (tf == 1) ? GTeamsData.Attackers
				                                  : (tf == 2 ? GTeamsData.Defenders : nullptr);
				Logger::Log("spawn",
					"TgGame.PostLogin intercept: conn=%d task_force=%d prevTF=%p newTF=%p\n",
					connectionIndex, tf, (void*)repInfo->r_TaskForce, (void*)taskforce);
				if (taskforce != nullptr) {
					repInfo->r_TaskForce = taskforce;
					repInfo->Team = taskforce;
					repInfo->bNetDirty = 1;
					repInfo->bForceNetUpdate = 1;
				}
			}
		}
		CallOriginal(Object, edx, Function, Params, Result);
		break;
	}

	case DispatchTag::DyingBeginState: {
		// Beacon carrier-loss BEFORE CallOriginal so the inventory remove
		// runs while the pawn still owns its InvManager. The original UC
		// chain doesn't touch the beacon-carry slot directly; without this
		// the pawn dies, IsCarryingBeacon stays true until the corpse is
		// fully destroyed, and the team's beacon never respawns.
		DropCarriedBeaconIfAny((ATgPawn*)Object);

		CallOriginal(Object, edx, Function, Params, Result);
		// UScript TgPawn.Dying.BeginState only calls PawnDied for bIsPlayer==true.
		// For AI bots the Timer would normally call Controller.Destroy() → PawnDied,
		// but bots fall out of the world first (LifeSpan set by OutsideWorldBounds),
		// so the Timer never fires and BotDied is never called.
		// Fix: call TgBotFactory__BotDied @ 0x10a8cbf0 directly here,
		// mirroring what TgAIController.PawnDied() would do.
		//
		// Kill attribution (m_DeathZoomInfo population + ClientAddKilled RPC)
		// lives in TgEffect__TrackStats — it fires inside the damage callstack
		// with direct access to InstigatorPawn, before the state's `Begin:`
		// latent block ships m_DeathZoomInfo to the client on the next Tick.
		// We can't do it here because m_LastDamager is never populated:
		// UpdateDamagers has no UC callers and the binary native that
		// originally drove it was stripped.
		ATgPawn* Pawn = (ATgPawn*)Object;
		// bIsPlayer is unreliable in this build — AI bots default bIsPlayer=true
		// (it's a "valid combatant" gate, not "has client connection"). Use a
		// class-name check, per feedback_bIsPlayer_unreliable.md. The intent
		// here is: this pawn is an AI bot (not a player, not a henchman).
		if (Pawn->Controller && Pawn->Controller->Class && !Pawn->r_bIsHenchman) {
			const char* ctrlRaw = Pawn->Controller->Class->GetFullName();
			const std::string ctrlClass = ctrlRaw ? ctrlRaw : "";
			const bool isPlayerCtrl = ctrlClass.find("PlayerController") != std::string::npos;
			if (!isPlayerCtrl) {
				ATgAIController* AIC = (ATgAIController*)Pawn->Controller;
				if (AIC->m_pFactory) {
					if (Logger::IsChannelEnabled(GetLogChannel())) {
						const char* pnRaw = Pawn->GetFullName();
						const std::string pnName = pnRaw ? pnRaw : "<null>";
						Logger::Log(GetLogChannel(), "Dying.BeginState: calling BotDied on factory for %s\n", pnName.c_str());
					}
					((void(__thiscall*)(ATgBotFactory*, ATgPawn*, ATgAIController*))0x10a8cbf0)(AIC->m_pFactory, Pawn, AIC);
					AIC->m_pFactory = nullptr;
				}
			}
		}
		break;
	}

	case DispatchTag::PostPawnSetup: {
		Logger::Log("flying", "WaitForInventoryThenDoPostPawnSetup called on %s\n",
			Object ? Object->GetFullName() : "<null>");
		// Hook target: `Function TgGame.TgPawn.WaitForInventoryThenDoPostPawnSetup`.
		//
		// We do NOT hook `PostPawnSetup` directly — UC subclass overrides call
		// `super.PostPawnSetup()` which compiles to `EX_FinalFunction` (direct
		// dispatch, bypasses ProcessEvent). The base-class hook never fires
		// for flying-class subclasses that override PostPawnSetup. The outer
		// WaitForInventoryThenDoPostPawnSetup, by contrast, is called from C++
		// via the SDK ProcessEvent wrapper at SpawnBotById time and re-entered
		// from the engine's SetTimer dispatch — both paths route through
		// ProcessEvent, so this hook fires reliably.
		//
		// UC body (TgPawn.uc:6690):
		//   simulated function WaitForInventoryThenDoPostPawnSetup() {
		//       if (inventory_ready)  PostPawnSetup();   // sets PHYS_Falling
		//       else                  SetTimer(0.1, 'WaitFor...');
		//   }
		//
		// The function polls recursively until inventory is ready. We apply
		// the physics fix after every iteration — idempotent for non-firing
		// iterations, corrects the SetPhysics(2) clobber on the firing one.
		// The fix has to live here because the recovery paths the original
		// game relied on (subclass SetMovementPhysics override, the stripped
		// InitializeHoverBot native, the Pawn::PossessedBy non-vehicle branch)
		// are all gone in this build.
		CallOriginal(Object, edx, Function, Params, Result);

		ATgPawn* Pawn = (ATgPawn*)Object;
		const char* className = Pawn->Class ? Pawn->Class->GetFullName() : nullptr;

		// Diagnostic snapshot: dump everything we know about the bot's
		// transform/visual state. Uses typed SDK accessors — earlier raw
		// offsets were wrong (bot_id off m_pAmBot was bogus 567 for every
		// bot, cylinder offsets 0x158/0x154 are pre-UE3 layout, mesh ptr
		// read worked but returned default). Read directly via the typed
		// field names so we get the real values.
		{
			int botIdLog = Pawn->r_nProfileId;  // set by SpawnBotById to bot_id
			float cylH = -1.0f, cylR = -1.0f;
			if (Pawn->CylinderComponent) {
				cylH = Pawn->CylinderComponent->CollisionHeight;
				cylR = Pawn->CylinderComponent->CollisionRadius;
			}
			float meshTransX = 999.0f, meshTransY = 999.0f, meshTransZ = 999.0f;
			float meshScale = -1.0f;
			if (Pawn->Mesh) {
				meshTransX = Pawn->Mesh->Translation.X;
				meshTransY = Pawn->Mesh->Translation.Y;
				meshTransZ = Pawn->Mesh->Translation.Z;
				meshScale  = Pawn->Mesh->Scale;
			}
			Logger::Log("flying",
				"BotPose: botId=%d class=%s Location=(%.1f,%.1f,%.1f) DrawScale=%.3f "
				"CylinderR=%.1f CylinderH=%.1f Mesh.Translation=(%.1f,%.1f,%.1f) MeshComp.Scale=%.3f "
				"m_fMeshScale=%.3f m_fBaseTranslationOffset=%.1f m_fCrouchTranslationOffset=%.1f "
				"Physics=%d\n",
				botIdLog,
				className ? className : "<no-class>",
				Pawn->Location.X, Pawn->Location.Y, Pawn->Location.Z,
				Pawn->DrawScale,
				cylR, cylH,
				meshTransX, meshTransY, meshTransZ,
				meshScale,
				Pawn->m_fMeshScale,
				Pawn->m_fBaseTranslationOffset,
				Pawn->m_fCrouchTranslationOffset,
				(int)Pawn->Physics);
		}

		// Flying-class detection: pawn class strstr — only classes whose UC
		// `SetMovementPhysics` override actually calls `SetPhysics(4)`. Scanner
		// is NOT one of these: it extends `TgPawn_Robot`, not `TgPawn_Hover`,
		// inherits the default PHYS_Walking, and uses a tall primary cylinder
		// (35×70 → 140uu) to LOOK like it's hovering while the cylinder bottom
		// rests on the ground. Adding it here put the chassis into PHYS_Flying
		// and broke that ground-anchored illusion.
		const bool flyingClass = className &&
			(strstr(className, "TgPawn_Hover")           ||
			 strstr(className, "TgPawn_FlyingBoss")      ||
			 strstr(className, "TgPawn_AttackTransport") ||
			 strstr(className, "TgPawn_ColonyEye")       ||
			 strstr(className, "TgPawn_NewWasp"));

		// Per-bot whitelist for ground-class pawns mounting a flying mesh.
		// See reference_bot_vehicle_possess_skips_setphysics.md for rationale.
		// Read bot_id off `m_pAmBot.Dummy + 0x1C` — set by SpawnBotById before
		// WaitForInventoryThenDoPostPawnSetup schedules this event.
		bool flyingBotId = false;
		void* BotConfig = (void*)Pawn->m_pAmBot.Dummy;
		if (BotConfig) {
			const int botId = *(int*)((char*)BotConfig + 0x1C);
			flyingBotId = (botId == 1107 || botId == 1657);
		}

		// Clear `bDriving` (APawn +0x3D0 bit 0x01, CPF_Net) on every bot
		// regardless of class. `AIController->Possess(Bot, 0, /*vehicleTransition=*/1)`
		// in SpawnBotById sets it true via the engine's vehicle-mode possession
		// path — that puts the pawn into "driver pose" anim blending (sitting/
		// hunched in a cockpit), which renders as a crouched mesh while the
		// collision cylinder stays full size. UC never references bDriving, so
		// it's pure engine territory; clearing it directly lets the bot's
		// normal stand/walk anim play. Matches the user's empirical
		// observation that bots appear crouched until possessed (crewing
		// toggles driver state via r_ControlPawn back-refs).
		//
		// Also clear bDriverIsVisible (bit 0x02) for sanity — the standalone
		// bot isn't a passenger of anything.
		unsigned int& driverBits = *(unsigned int*)((char*)Pawn + 0x3D0);
		driverBits &= ~0x00000003u;
		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;

		if (flyingClass || flyingBotId) {
			// Stack 1 — Engine physics mode: PHYS_Flying.
			Pawn->Physics = 4;

			// Stack 2 — Engine gating bits at APawn +0x1EC.
			// In UE3 physFlying, if `bCanFly` is false the engine immediately
			// transitions back to physFalling. CDO defaults set it true on
			// every flying class, but force it here in case spawn path or
			// PostPawnSetup ever loses it. Clear `bSimulateGravity` so the
			// engine's gravity integrator doesn't accelerate the pawn down.
			//   bit 0x00002000 = bCanFly
			//   bit 0x00040000 = bSimulateGravity (clear)
			//   bit 0x00000800 = bCanWalk          (leave alone — most flying
			//                    classes can still walk if they touch ground)
			unsigned int& engineBits = *(unsigned int*)((char*)Pawn + 0x1EC);
			engineBits |= 0x00002000u;
			engineBits &= ~0x00040000u;

			// Stack 3 — game-custom SoftZ gravity (TgPawn +0x3D8 bit 0x40000000).
			// All three flying classes (Hover, FlyingBoss, ColonyEye) override
			// `m_bAffectedBySoftZ=false` in their UC defaultproperties — that's
			// the game's reinvented vertical-pull system. If the CDO override
			// isn't honored at instance time (we've seen this with other
			// CDO-based defaults), the flying bot inherits TgPawn's parent
			// default of `true` and SoftZ pulls it down regardless of Physics
			// mode. Force-clear here.
			unsigned int& tgBits = *(unsigned int*)((char*)Pawn + 0x3D8);
			tgBits &= ~0x40000000u;

			Pawn->bNetDirty = 1;
			Pawn->bForceNetUpdate = 1;
			if (Logger::IsChannelEnabled("flying")) {
				Logger::Log("flying",
					"PostPawnSetup: applied PHYS_Flying + bCanFly + clear SoftZ for %s (class=%s)\n",
					Pawn->GetFullName(), className ? className : "<no-class>");
			}
		}
		break;
	}

	case DispatchTag::DeviceFiringEndState: {
		CallOriginal(Object, edx, Function, Params, Result);
		ATgDevice* Device = (ATgDevice*)Object;

		// Beacon deploy cleanup. UC TgDevice.uc:2877 fires ConsumeDevice when
		// r_bConsumedOnUse is set, but ConsumeDevice's RemoveConsumableFromOwnerInventory
		// native only does HUD sync — it doesn't actually remove the device from
		// the pawn's inventory.  The original Deploy native presumably did that
		// work; ours just spawns the deployable.  Without inventory cleanup the
		// beacon would linger in m_EquippedDevices[11] and the control-server's
		// device-bar slot would never clear.
		//
		// Calling NonPersistRemoveDevice HERE — after CallOriginal — is crucial:
		// CallOriginal runs the client-side `if (m_bUsesDeployMode &&
		// Instigator.Weapon == self) ChangeToPreviousWeapon()` (TgDevice.uc:2871)
		// while Pawn->Weapon is still the beacon.  Only after that do we clear
		// the slot, so the race that drove the weapon to melee is gone.
		//
		// Gate ONLY on m_bIsBeaconPlacing — r_bConsumedOnUse is a consumables
		// concept (different device category we don't have implemented yet) and
		// isn't reliably set on beacon devices.  TgInventoryManager.NonPersistAddDevice
		// sets m_bIsBeaconPlacing=1 explicitly when nEquipPoint==11, so any
		// device firing DeviceFiring.EndState with that bit set is a
		// carry-beacon and should be removed from inventory after the deploy.
		{
			uint32_t devFlags = *(uint32_t*)((char*)Device + 0x22C);
			bool bIsBeaconPlacing = (devFlags & 0x10000u) != 0;
			int nEquipPoint = (int)Device->r_eEquippedAt;
			Logger::Log("beacon",
				"DeviceFiring.EndState: device=0x%p devId=%d invId=%d slot=%d "
				"devFlags=0x%08x m_bIsBeaconPlacing=%d instigator=0x%p\n",
				Device, Device->r_nDeviceId, Device->r_nInventoryId, nEquipPoint,
				devFlags, (int)bIsBeaconPlacing, Device->Instigator);
			if (bIsBeaconPlacing && Device->Instigator) {
				ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
				ATgInventoryManager* InvMgr = (ATgInventoryManager*)Pawn->InvManager;
				if (InvMgr && nEquipPoint >= 1 && nEquipPoint <= 24) {
					// Two cases:
					//   A. Device is the CURRENT slot occupant (1st deploy): normal
					//      cleanup — NonPersistRemoveDevice clears the slot + IPC.
					//   B. Device is an ORPHAN from a prior cleanup whose state
					//      machine fired EndState late (2nd+ deploy): slot 11 now
					//      holds a different (NEW) device. We can't just clean the
					//      slot because we'd kill the NEW beacon. Instead, drop
					//      the orphan's m_bIsBeaconPlacing bit + sever its
					//      Instigator so any further state-machine ticks no-op,
					//      AND re-issue cleanup for whatever beacon currently
					//      occupies the slot (NEW), since that's the one the
					//      player just deployed.
					ATgDevice* slotDev = Pawn->m_EquippedDevices[nEquipPoint];
					if (slotDev == Device) {
						Logger::Log("beacon",
							"DeviceFiring.EndState: post-deploy cleanup pawn=0x%p device=0x%p slot=%d (current occupant)\n",
							Pawn, Device, nEquipPoint);
						TgInventoryManager__NonPersistRemoveDevice::Call(InvMgr, nullptr, nEquipPoint);
					} else {
						uint32_t slotFlags = slotDev ? *(uint32_t*)((char*)slotDev + 0x22C) : 0u;
						bool slotIsBeacon = slotDev && (slotFlags & 0x10000u) != 0;
						Logger::Log("beacon",
							"DeviceFiring.EndState: orphan EndState fired — slot=%d holds 0x%p (beacon=%d). "
							"Detaching orphan + cleaning slot occupant.\n",
							nEquipPoint, slotDev, (int)slotIsBeacon);
						// Detach orphan so its state machine can't side-effect
						// further: clear m_bIsBeaconPlacing (so this gate stops
						// firing for it), null its Instigator (so any future
						// state-scoped UC code that touches Instigator no-ops),
						// and reset r_eEquippedAt so any future lookups don't
						// route back to slot 11.
						*(uint32_t*)((char*)Device + 0x22C) &= ~0x10000u;
						Device->Instigator   = nullptr;
						Device->r_eEquippedAt = 0;
						// Clean the NEW beacon currently in the slot — that's
						// the one the player actually just deployed.
						if (slotIsBeacon) {
							TgInventoryManager__NonPersistRemoveDevice::Call(InvMgr, nullptr, nEquipPoint);
						}
					}
				}
			}
		}
		break;
	}

	case DispatchTag::ServerStartFire: {
		// Diagnostic: capture every gate `TgDevice.CanDeviceFireNow` reads,
		// then run CallOriginal and check whether the device's state actually
		// transitioned to DeviceBuildup (=gate passed) or stayed in Active
		// (=gate failed). Goal: pinpoint why server rejects in-hand-weapon
		// fire while client accepted it (firing-while-jetpacking → no
		// projectile / no hitscan damage on server, client renders the shot).
		//
		// AVOID SDK `eventXxx` wrappers that return `bool` — the generated
		// SDK code returns `Parms.ReturnValue` where ReturnValue is a
		// 1-bit bitfield in a stack-allocated Parms struct. UC's UnrealVM
		// writes the bit, but reads sometimes pick up stack garbage instead
		// (observed: `knockedDown=1 dying=1` simultaneously impossible,
		// `IsFiring=0` on a device whose state is DeviceFiring). Use direct
		// memory reads + state-name strcmp instead.
		//
		// Channel: "fire_gate" (in control-server.json).
		bool wantLog = Logger::IsChannelEnabled("fire_gate");
		const char* stateBefore = nullptr;
		if (wantLog) {
			ATgDevice* Device = (ATgDevice*)Object;
			if (Device) {
				ATgPawn*  Pawn = (ATgPawn*)Device->Instigator;
				AWeapon*  Wpn  = Pawn ? Pawn->Weapon : nullptr;
				int       fm   = (int)Device->CurrentFireMode;
				UTgDeviceFire* FireMode =
					(fm >= 0 && fm < Device->m_FireMode.Count)
						? Device->m_FireMode.Data[fm] : nullptr;
				stateBefore = Device->GetStateName().GetName();

				// Read GIsNonCombat directly (the SDK wrapper for IsNonCombat
				// is unreliable). We force it to 0 in combat at engine init,
				// but still want to surface it for diagnosis.
				int isNonCombat = *(int*)0x119A01C5;

				Logger::Log("fire_gate",
					"[ServerStartFire] device=%p devId=%d type=%d offHand=%d handDev=%d "
					"usesDeploy=%d stealthDev=%d fm=%d/%d stateBefore=%s\n",
					Device,
					Device->r_nDeviceId, Device->m_nDeviceType,
					(int)Device->m_bIsOffHand, (int)Device->m_bHandDevice,
					(int)Device->m_bUsesDeployMode, (int)Device->r_bIsStealthDevice,
					fm, Device->m_FireMode.Count,
					stateBefore ? stateBefore : "<null>");

				if (Pawn) {
					const char* pawnState = Pawn->GetStateName().GetName();

					// Aim-rotation source-of-truth dump. The projectile spawn
					// rotation = `Rotator(Vector(GetAdjustedAim(StartTrace)))`,
					// which routes through `TgPawn.GetBaseAimRotation` →
					// `PC.GetPlayerViewPoint`. So Pawn.Rotation, Controller
					// Rotation, AND GetPlayerViewPoint output are the three
					// candidate sources of the wrong yaw observed during
					// flight (projectile spawned with rot=(0, 32760, 0) =
					// fixed -X direction, vs ground rot=(-460, -13936, 0)).
					AController* Ctl = Pawn->Controller;
					FVector  pvLoc  = {0,0,0};
					FRotator pvRot  = {0,0,0};
					if (Ctl) {
						Ctl->eventGetPlayerViewPoint(&pvLoc, &pvRot);
					}

					Logger::Log("fire_gate",
						"  pawn=%p physics=%d weapon=%p (self==weapon? %d) controller=%p\n"
						"  flags: GIsNonCombat=%d isHacking=%d isDecoy=%d "
						"binoculars=%d offhandCooldownNet=%d offhandCooldownLocal=%d\n"
						"  flags: disableAllDevices=%d disableAction=%d aimingMode=%d\n"
						"  state=%s  power=%.2f\n"
						"  pawn.Rotation     =(p=%d, y=%d, r=%d)\n"
						"  ctrl.Rotation     =(p=%d, y=%d, r=%d)\n"
						"  PlayerViewPoint   =(p=%d, y=%d, r=%d) at loc=(%.1f, %.1f, %.1f)\n",
						Pawn, (int)Pawn->Physics, Wpn, (int)(Wpn == (AWeapon*)Device), Ctl,
						isNonCombat, (int)Pawn->r_bIsHacking, (int)Pawn->r_bIsDecoy,
						(int)Pawn->r_bUsingBinoculars, (int)Pawn->r_bInGlobalOffhandCooldown,
						(int)Pawn->bInGlobalOffhandCooldownClient,
						(int)Pawn->r_bDisableAllDevices, (int)Pawn->c_bDisableAction,
						(int)Pawn->r_bAimingMode,
						pawnState ? pawnState : "<null>",
						Pawn->r_fCurrentPowerPool,
						Pawn->Rotation.Pitch, Pawn->Rotation.Yaw, Pawn->Rotation.Roll,
						Ctl ? Ctl->Rotation.Pitch : 0,
						Ctl ? Ctl->Rotation.Yaw   : 0,
						Ctl ? Ctl->Rotation.Roll  : 0,
						pvRot.Pitch, pvRot.Yaw, pvRot.Roll,
						pvLoc.X, pvLoc.Y, pvLoc.Z);
				}

				if (FireMode) {
					// `eventAmountCurrentlyOffOfTargetAccuracy` returns float
					// (not bool) so its SDK wrapper IS reliable. The gate at
					// TgDevice.uc:968 fails if this is > 0.0001 while
					// m_bRequireAimMode is set.
					float accOff = Device->eventAmountCurrentlyOffOfTargetAccuracy(Device->CurrentFireMode);
					Logger::Log("fire_gate",
						"  firemode: fireType=%d cont=%d restrictInCombat=%d "
						"requireAimMode=%d restrictFireFlags=0x%x restrictPhysFlags=0x%x "
						"attackRate=%.3f accuracyOff=%.4f\n",
						(int)FireMode->m_nFireType,
						(int)FireMode->m_bContinuousFire,
						(int)FireMode->m_bRestrictInCombat,
						(int)FireMode->m_bRequireAimMode,
						FireMode->m_nRestrictFiringFlags,
						FireMode->m_nRestrictPhysicsFlags,
						FireMode->GetAttackRate(),
						accOff);
				}

				if (Pawn) {
					// Jetpack-slot dump. Compare state via strcmp instead of
					// the broken `eventIsFiring()` SDK call. UC's DeviceFiring
					// state override returns IsFiring=true, but the SDK
					// wrapper returns garbage.
					ATgDevice* JP = nullptr;
					for (int i = 0; i < 15; ++i) {
						ATgDevice* d = Pawn->m_EquippedDevices[i];
						if (d && d->m_nDeviceType == 806) { JP = d; break; }
					}
					if (JP) {
						const char* jpState = JP->GetStateName().GetName();
						bool jpFiring = jpState && (strcmp(jpState, "DeviceFiring") == 0 ||
						                            strcmp(jpState, "DeviceBuildup") == 0);
						Logger::Log("fire_gate",
							"  jetpack: dev=%p state=%s offHand=%d handDev=%d devType=%d "
							"isFiring(stateName)=%d isPawnWeapon=%d\n",
							JP, jpState ? jpState : "<null>",
							(int)JP->m_bIsOffHand, (int)JP->m_bHandDevice, JP->m_nDeviceType,
							(int)jpFiring,
							(int)(Pawn->Weapon == (AWeapon*)JP));
					} else {
						Logger::Log("fire_gate", "  jetpack: <no TGDT_TRAVEL device equipped>\n");
					}
				}
			}
		}

		CallOriginal(Object, edx, Function, Params, Result);

		// Did the gate pass? StartFire's success path is `GotoState('DeviceBuildup')`
		// (TgDevice.uc:1411). If the device's state is now DeviceBuildup or
		// DeviceFiring, the gate passed; if it's still in its pre-call state
		// (typically 'Active'), CanDeviceFireNow returned false. This is the
		// ground-truth verdict — beats any SDK wrapper call.
		if (wantLog) {
			ATgDevice* Device = (ATgDevice*)Object;
			if (Device) {
				const char* stateAfter = Device->GetStateName().GetName();
				bool passed = stateAfter && (strcmp(stateAfter, "DeviceBuildup") == 0 ||
				                             strcmp(stateAfter, "DeviceFiring") == 0);
				Logger::Log("fire_gate",
					"  verdict: stateBefore=%s -> stateAfter=%s  GATE %s\n",
					stateBefore ? stateBefore : "<null>",
					stateAfter ? stateAfter : "<null>",
					passed ? "PASSED (fire dispatched)" : "FAILED (no fire)");

				// Aim-rotation cache dump — POST-CallOriginal so we see the
				// value computed during THIS fire's ProjectileFire chain.
				// `TgPawn.GetBaseAimRotation` (TgPawn.uc:7892) caches its
				// result into `m_CachedBaseAimRotation` at offset 0x15D4 of
				// TgPawn (per SDK header). The cache is per-tick, so reading
				// it now gives this shot's computed aim. Compare against
				// ctrl.Rotation / PlayerViewPoint logged earlier — if they
				// differ, the shooting-reticle branch at TgPawn.uc:7889
				// (OutRotation = Rotator(HitLocation - WeaponLoc)) is what
				// overrode the rotation. The c_bUsesShootingReticle flag
				// from the cached camera-values struct tells us whether
				// that branch is even being taken; it lives at offset 0x1574
				// of TgPawn, with c_bUsesShootingReticle being bit 0x01 of
				// the first dword.
				ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
				if (Pawn) {
					FRotator& cachedAim = *(FRotator*)((char*)Pawn + 0x15D4);
					unsigned int camFlags = *(unsigned int*)((char*)Pawn + 0x1574);
					bool usesReticle = (camFlags & 0x1) != 0;
					Logger::Log("fire_gate",
						"  cached aim @ TgPawn+0x15D4 = (p=%d, y=%d, r=%d)  "
						"c_bUsesShootingReticle=%d\n\n",
						cachedAim.Pitch, cachedAim.Yaw, cachedAim.Roll,
						(int)usesReticle);
				} else {
					Logger::Log("fire_gate", "\n");
				}
			}
		}
		break;
	}

	case DispatchTag::ServerStopFire: {
		// Stealth is a "hold-to-sustain" buff and the 1s-promoted lifetime
		// would leave the HUD icon visible for up to a second after the
		// user releases fire. Tear it down immediately on fire-release
		// for snappier UX. RemoveEffectGroupsByCategory matches clones by
		// m_nCategoryCode (not pointer), runs our RemoveEffects on each,
		// and hits the symmetric UC Remove path — m_fRaw restores
		// correctly.
		//
		// Other device categories rely on UC's native
		// remove path (LifeOver timer or explicit RemoveEffectType). Our
		// RemoveEffectGroup now matches clones by egId when the caller
		// passes a template, so scope-out, rest-end, etc. all work
		// natively without any bracket.
		CallOriginal(Object, edx, Function, Params, Result);
		// ATgDevice* Device = (ATgDevice*)Object;
		// if (Device && Device->Instigator && HasStealthEffectGroup(Device)) {
		// 	ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
		// 	ATgEffectManager* Mgr = Pawn->r_EffectManager;
		// 	if (Mgr) {
		// 		TgEffectManager__RemoveEffectGroupsByCategory::Call(
		// 			Mgr, nullptr, /*nCategoryCode=*/621, /*nQuantity=*/99);
		// 	}
		// }
		break;
	}

	case DispatchTag::TgEffectRemove:
		// Passthrough. Correct own-class dispatch is done by RemoveEffects /
		// DispatchEffectRemove (the SDK eventRemove wrapper resolves only the
		// base TgEffect.Remove, so buffs are dispatched explicitly there).
		DoCatchAll();
		break;

	// Note: `Function TgGame.TgEffect.CheckEffectBuffModifier` previously had
	// a PE-time guard here that did SDK-caller damage-mod scaling. With the
	// native fully reimplemented at 0x10a6f270 (see
	// `TgEffect__CheckEffectBuffModifier`), both UC bytecode and SDK callers
	// reach the same native body — keeping a PE hook here would double-apply.
	// Removed; SDK callers now follow the default catch-all path which
	// CallOriginal's down to the native.

	case DispatchTag::PawnPickupNearestDeployable: {
		// Diagnostic: dump pawn->Touching contents so we can see whether the
		// beacon is even in the touching set. If it's not, the pickup chain
		// stops here (UC iterates `TouchingActors(TgDeployable, deployable)`).
		ATgPawn* Pawn = (ATgPawn*)Object;
		Logger::Log("beacon", "PickupNearestDeployable: ENTER pawn=0x%p\n", Pawn);
		if (Pawn) {
			// Touching is an AActor TArray at offset 0x100 on AActor (per SDK).
			struct ArrPtr { AActor** Data; int Count; int Max; };
			ArrPtr* touching = reinterpret_cast<ArrPtr*>((char*)Pawn + 0x100);
			Logger::Log("beacon",
				"PickupNearestDeployable: pawn=0x%p Touching.Count=%d Touching.Data=%p\n",
				Pawn, touching->Count, touching->Data);
			for (int i = 0; i < touching->Count; ++i) {
				AActor* other = touching->Data[i];
				if (!other) continue;
				const char* clsName = (other->Class ? other->Class->GetFullName() : "<null>");
				bool isDep = clsName && strstr(clsName, "TgDeploy") != nullptr;
				Logger::Log("beacon",
					"  Touching[%d] = 0x%p class=%s%s\n",
					i, other, clsName ? clsName : "<null>",
					isDep ? " <- candidate" : "");
				if (isDep) {
					ATgDeployable* dep = (ATgDeployable*)other;
					Logger::Log("beacon",
						"    deployableId=%d m_nPickupDeviceId=%d s_bWasPickedUp=%d "
						"m_bPickupOnlyOnce=%d m_bInDestroyedState=%d r_DRI=0x%p\n",
						dep->r_nDeployableId, dep->m_nPickupDeviceId,
						(int)dep->s_bWasPickedUp, (int)dep->m_bPickupOnlyOnce,
						(int)dep->m_bInDestroyedState, dep->r_DRI);
				}
			}
		}
		CallOriginal(Object, edx, Function, Params, Result);
		// Log the return value (Parms layout: ATgPawn_eventPickupNearestDeployable_Parms
		// has just a bool ReturnValue at offset 0x0)
		if (Params) {
			bool ret = (*(uint32_t*)Params & 1u) != 0;
			Logger::Log("beacon", "PickupNearestDeployable: EXIT returned %s\n", ret ? "true" : "false");
		}
		break;
	}

	case DispatchTag::BeaconPickUpDeployable: {
		// Diagnostic: log every gate value before the UC body runs so we
		// know exactly which check is blocking pickup.
		ATgDeploy_Beacon* beacon = (ATgDeploy_Beacon*)Object;
		ATgPawn* pReceiver = nullptr;
		if (Params) {
			pReceiver = *(ATgPawn**)Params;
		}
		Logger::Log("beacon", "PickUpDeployable: ENTER beacon=0x%p pReceiver=0x%p\n", beacon, pReceiver);
		if (beacon) {
			ATgRepInfo_Deployable* dri = beacon->r_DRI;
			ATgRepInfo_Player* recvPri = pReceiver ? (ATgRepInfo_Player*)pReceiver->PlayerReplicationInfo : nullptr;
			ATgRepInfo_TaskForce* recvTf = recvPri ? recvPri->r_TaskForce : nullptr;

			bool canBePickedUp = (beacon->m_nPickupDeviceId > 0)
				&& !beacon->s_bWasPickedUp
				&& beacon->m_bPickupOnlyOnce;

			Logger::Log("beacon",
				"PickUpDeployable[entry]: beacon=0x%p pReceiver=0x%p\n"
				"  m_nPickupDeviceId=%d s_bWasPickedUp=%d m_bPickupOnlyOnce=%d -> CanBePickedUp=%s\n"
				"  m_bInDestroyedState=%d\n"
				"  beacon.r_DRI=0x%p instigatorInfo=0x%p tfInfo=0x%p bOwnedByTf=%d\n"
				"  receiver.PRI=0x%p receiver.PRI.r_TaskForce=0x%p\n",
				beacon, pReceiver,
				beacon->m_nPickupDeviceId, (int)beacon->s_bWasPickedUp, (int)beacon->m_bPickupOnlyOnce,
				canBePickedUp ? "TRUE" : "FALSE",
				(int)beacon->m_bInDestroyedState,
				dri, dri ? dri->r_InstigatorInfo : nullptr, dri ? dri->r_TaskforceInfo : nullptr,
				dri ? (int)dri->r_bOwnedByTaskforce : -1,
				recvPri, recvTf);
		}
		CallOriginal(Object, edx, Function, Params, Result);
		if (Params) {
			bool ret = (*(uint32_t*)((char*)Params + 4) & 1u) != 0;
			Logger::Log("beacon", "PickUpDeployable: EXIT returned %s\n", ret ? "true" : "false");
		}
		break;
	}

	case DispatchTag::ServerPickupPutdownDeployableTag: {
		// Pickup-key RPC. Run the UC body first — its TouchingActors walk in
		// `PickupNearestDeployable` handles factory beacons (which were in
		// the world long enough for the player to walk into their cylinder)
		// and any other deployable the player has physically entered.
		CallOriginal(Object, edx, Function, Params, Result);

		// Spawn-while-overlapping fallback. UE3 fires `Touch` on collision
		// ENTRY — a beacon spawned around a stationary pawn never registers
		// in the pawn's Touching list, so UC's PickupNearestDeployable can't
		// find it even with the pawn standing on top of it. Check the team's
		// registered beacon by distance and pick it up manually if eligible.
		//
		// Skip when UC already did the pickup (it set s_bWasPickedUp on the
		// world beacon or cleared mgr->r_Beacon via DestroyIt/UnRegister) —
		// the gates below catch both cases.
		APlayerController* PC = (APlayerController*)Object;
		ATgPawn* Pawn = PC ? (ATgPawn*)PC->Pawn : nullptr;
		if (!Pawn || !Pawn->PlayerReplicationInfo) break;
		ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (!pri->r_TaskForce || !pri->r_TaskForce->r_BeaconManager) break;
		ATgTeamBeaconManager* mgr = pri->r_TaskForce->r_BeaconManager;
		ATgDeploy_Beacon* beacon = mgr->r_Beacon;
		if (!beacon) break;
		if (beacon->m_bInDestroyedState) break;
		if (beacon->s_bWasPickedUp) break;
		if (beacon->m_nPickupDeviceId <= 0) break;
		if (!beacon->m_bPickupOnlyOnce) break;
		// Pickup during the Deploy phase is allowed — the entrance teleport
		// still gates on m_bIsDeployed via the BeaconEntranceHasExit hook,
		// so picking up mid-deploy correctly cancels both the deploy and the
		// (not-yet-active) entrance link.

		// Proximity: 64uu XY radius covers a player standing on or next to a
		// beacon (beacon CollisionCylinder radius is ~34uu in DB, player
		// cylinder ~18uu, so combined ~52uu — round up to 64 for headroom).
		// Z difference up to 80uu allowed so the player can be jumping over
		// it and still trigger pickup.
		float dx = Pawn->Location.X - beacon->Location.X;
		float dy = Pawn->Location.Y - beacon->Location.Y;
		float dz = Pawn->Location.Z - beacon->Location.Z;
		float xyDistSq = dx*dx + dy*dy;
		if (xyDistSq > 64.f * 64.f) break;
		if (dz < -80.f || dz > 80.f) break;

		// Manual dispatch of UC PickUpDeployable. ProcessEvent re-enters our
		// dispatcher → BeaconPickUpDeployable case logs entry/exit. UC body
		// handles inventory add + DestroyIt + UnRegister.
		Logger::Log("beacon",
			"ServerPickupPutdown: fallback firing for beacon=0x%p dist=%.0fuu (UC TouchingActors missed it)\n",
			beacon, sqrtf(xyDistSq));
		const bool picked = beacon->PickUpDeployable(Pawn);
		Logger::Log("beacon",
			"  fallback PickUpDeployable returned %s\n", picked ? "true" : "false");
		break;
	}

	case DispatchTag::PlayerControllerDestroyed: {
		// Disconnect cleanup: if this controller's pawn was carrying a beacon
		// for one of the team managers, drop it and respawn at the team's
		// original-priority factory. Pre-CallOriginal so the pawn / InvManager
		// chain is still wired when we issue the inventory remove.
		APlayerController* PC = (APlayerController*)Object;
		if (PC && PC->Pawn) {
			DropCarriedBeaconIfAny((ATgPawn*)PC->Pawn);
		}
		CallOriginal(Object, edx, Function, Params, Result);
		break;
	}

	case DispatchTag::BeaconEntranceHasExit: {
		// UC HasExit (TgDeploy_BeaconEntrance.uc:122) returns
		// `beaconManager.GetBeacon() != none` — i.e. true the moment
		// `r_Beacon` is non-null, regardless of deploy phase. UC's
		// `Deploy.BeginState` calls `RegisterBeacon(self, false)` which sets
		// `r_Beacon` before the deploy animation completes, so without this
		// gate the entrance teleport unlocks the instant a player throws a
		// beacon — well before its visible deploy timer runs out.
		//
		// Gate the result on `r_Beacon->m_bIsDeployed` (flipped to 1 by UC
		// `TgDeployable::DeployComplete` after `r_fTimeToDeploySecs` elapses,
		// AND for factory-spawned beacons by `WireDeployableOwnership` at
		// spawn time — those keep their immediate-active behavior).
		//
		// Parms layout: ATgDeploy_BeaconEntrance_eventHasExit_Parms has just
		// `bool ReturnValue` at offset 0 (bitfield :1).
		CallOriginal(Object, edx, Function, Params, Result);
		if (!Params) break;
		uint32_t* retPtr = (uint32_t*)Params;
		const bool ret = (*retPtr & 1u) != 0;
		if (!ret) break;  // already false, nothing to gate
		ATgDeploy_BeaconEntrance* entrance = (ATgDeploy_BeaconEntrance*)Object;
		if (!entrance || !entrance->r_DRI) break;
		// Resolve the entrance's team manager via its DRI. Factory-spawned
		// entrances get r_TaskforceInfo set by WireDeployableOwnership in
		// TgBeaconFactory::SpawnObject; that points at the team's tfri whose
		// r_BeaconManager owns the matching exit beacon.
		ATgRepInfo_TaskForce* tfri = entrance->r_DRI->r_TaskforceInfo;
		ATgTeamBeaconManager* mgr = tfri ? tfri->r_BeaconManager : nullptr;
		if (!mgr || !mgr->r_Beacon) break;
		if (!mgr->r_Beacon->m_bIsDeployed) {
			*retPtr = 0;  // clear the bool — entrance stays inactive
		}
		break;
	}

	// RefireCheckTimer's side effect ran above; it has no specific main
	// handler, so it falls into the catch-all (log + CallOriginal) — same
	// behavior as the prior else-if chain.
	case DispatchTag::RefireCheckTimer:
	case DispatchTag::Unknown:
	default:
		DoCatchAll();
		break;
	}

	if (scopeLog) LogScopeCall("AFTER ", Object, Function);
}
