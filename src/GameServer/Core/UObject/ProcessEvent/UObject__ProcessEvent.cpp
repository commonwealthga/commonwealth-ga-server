#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
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
static bool HasStealthEffectGroup(ATgDevice* Device) {
	if (!Device) return false;
	for (int m = 0; m < Device->m_FireMode.Count; m++) {
		UTgDeviceFire* fireMode = Device->m_FireMode.Data[m];
		if (!fireMode) continue;
		if (fireMode->s_EffectGroupList.Count == 0) {
			int idx = 0;
			TgDeviceFire__GetEffectGroup::Call(fireMode, nullptr, -1, &idx);
		}
		for (int i = 0; i < fireMode->s_EffectGroupList.Count; i++) {
			UTgEffectGroup* eg = fireMode->s_EffectGroupList.Data[i];
			if (eg && eg->m_nCategoryCode == 621) return true;
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
	TgEffectRemove,               // (guarded) ApplyBuffEffectFromHook in lieu of original
	PostPawnSetup,                // CallOriginal (sets PHYS_Falling) then restore PHYS_Flying for flying bots
	ServerStartFire,              // Diagnostic: log every CanDeviceFireNow gate before CallOriginal
	GetPlayerViewPoint,           // Pre-sync c_nCameraYawOffset / c_nCameraPitchOffset from ctrl.Rotation
};

// First-sight classification: walks the strcmp ladder once per unique
// UFunction. Returns the matching DispatchTag (or Unknown for the catch-all
// path). Order kept matching the historical hand-written branches so the
// behavior of any function name not listed here is unchanged.
static DispatchTag ClassifyFunction(UFunction* fn) {
	const char* name = fn->GetFullName();
	if (!name) return DispatchTag::Unknown;

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

void __fastcall UObject__ProcessEvent::Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {
	if (!Object || !Function) return;

	const DispatchTag tag = GetDispatchTag(Function);

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
			Logger::ChannelIndents[GetLogChannel()]++;
			CallOriginal(Object, edx, Function, Params, Result);
			Logger::ChannelIndents[GetLogChannel()]--;
		} else {
			CallOriginal(Object, edx, Function, Params, Result);
		}
	};

	switch (tag) {
	case DispatchTag::EarlyOut:
		CallOriginal(Object, edx, Function, Params, Result);
		break;

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
		if (Pawn->Controller && !Pawn->Controller->bIsPlayer && !Pawn->r_bIsHenchman) {
			ATgAIController* AIC = (ATgAIController*)Pawn->Controller;
			if (AIC->m_pFactory) {
				if (Logger::IsChannelEnabled(GetLogChannel())) {
					Logger::Log(GetLogChannel(), "Dying.BeginState: calling BotDied on factory for %s\n", Pawn->GetFullName());
				}
				((void(__thiscall*)(ATgBotFactory*, ATgPawn*, ATgAIController*))0x10a8cbf0)(AIC->m_pFactory, Pawn, AIC);
				AIC->m_pFactory = nullptr;
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

		// Flying-class detection: pawn class strstr (catches all UC classes
		// whose `SetMovementPhysics` override would call `SetPhysics(4)`).
		const bool flyingClass = className &&
			(strstr(className, "TgPawn_Hover")           ||
			 strstr(className, "TgPawn_FlyingBoss")      ||
			 strstr(className, "TgPawn_AttackTransport") ||
			 strstr(className, "TgPawn_ColonyEye") ||
			 strstr(className, "TgPawn_NewWasp"));

		// Per-bot whitelist for ground-class pawns mounting a flying mesh.
		// See reference_bot_vehicle_possess_skips_setphysics.md for rationale.
		// Read bot_id off `m_pAmBot.Dummy + 0x1C` — set by SpawnBotById before
		// WaitForInventoryThenDoPostPawnSetup schedules this event.
		bool flyingBotId = false;
		void* BotConfig = Pawn->m_pAmBot.Dummy;
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
		if (Device->IsOffhandJetpack() && Device->Instigator) {
			ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
			SetPawnProperty(Pawn, 59, 0.0f);  // clear TGPID_FLIGHT_ACCELERATION
			Pawn->bNetDirty = 1;
			Pawn->bForceNetUpdate = 1;
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
		ATgDevice* Device = (ATgDevice*)Object;
		if (Device && Device->Instigator && HasStealthEffectGroup(Device)) {
			ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
			ATgEffectManager* Mgr = Pawn->r_EffectManager;
			if (Mgr) {
				TgEffectManager__RemoveEffectGroupsByCategory::Call(
					Mgr, nullptr, /*nCategoryCode=*/621, /*nQuantity=*/99);
			}
		}
		break;
	}

	case DispatchTag::TgEffectRemove:
		// Symmetric counterpart to the SetEffectRep-driven apply
		// (`TgEffectManager__SetEffectRep.cpp`): reverses the buff registry
		// entry for class-157 effects, mirroring `TgEffectBuff.uc:202`.
		//
		// The hook is ADDITIVE — buff-route Remove runs ALONGSIDE base UC
		// `TgEffect.Remove` (CallOriginal via DoCatchAll), not instead of it.
		// Apply is already additive: UC `TgEffect.ApplyEffect` runs via UC
		// dispatch, then `SetEffectRep`'s hook fires `ApplyBuffEffectFromHook`
		// on top. Remove must mirror that structure or we'd silently leak
		// any side-effect that base `TgEffect.Remove` is responsible for.
		//
		// Concrete leak that motivated the fix: iMINIGUN's prop-338 root
		// (egId 9350) is a class-80 effect whose UC Remove special-cases
		// `if (m_nPropertyId == 338) ApplyToProperty(Target, 49, 10000, true)`
		// to undo the prop-49 GroundSpeed clamp at TgEffect.uc:529-533. When
		// `BuffEffectRegistry::IsBuff()` false-positives on a pointer-reuse
		// collision (which it can — see ApplyBuffEffect.hpp), the prior
		// if/else here SKIPPED CallOriginal entirely and the player stayed
		// rooted forever. Additive removal makes the false positive benign:
		// UC Remove undoes the real prop modification, buff-route Remove
		// undoes the spurious registry entry.
		//
		// For LEGITIMATE class-157 effects, running UC `TgEffect.Remove`
		// additionally is also safe — class-157 buffs target modifier props
		// (113 Accuracy, 65 Effect Damage Modifier, …) the pawn doesn't carry
		// in `s_Properties`, so `ApplyToProperty(bRemove=true)` is a silent
		// no-op there. Same reasoning that motivated the buff-route hook in
		// the first place.
		if (Params && BuffEffectRegistry::IsBuff((UTgEffect*)Object)) {
			auto* parms = (UTgEffect_eventRemove_Parms*)Params;
			ApplyBuffEffectFromHook((UTgEffect*)Object, parms->Target, /*bRemove=*/true);
		}
		DoCatchAll();
		break;

	// Note: `Function TgGame.TgEffect.CheckEffectBuffModifier` previously had
	// a PE-time guard here that did SDK-caller damage-mod scaling. With the
	// native fully reimplemented at 0x10a6f270 (see
	// `TgEffect__CheckEffectBuffModifier`), both UC bytecode and SDK callers
	// reach the same native body — keeping a PE hook here would double-apply.
	// Removed; SDK callers now follow the default catch-all path which
	// CallOriginal's down to the native.

	// RefireCheckTimer's side effect ran above; it has no specific main
	// handler, so it falls into the catch-all (log + CallOriginal) — same
	// behavior as the prior else-if chain.
	case DispatchTag::RefireCheckTimer:
	case DispatchTag::Unknown:
	default:
		DoCatchAll();
		break;
	}
}


