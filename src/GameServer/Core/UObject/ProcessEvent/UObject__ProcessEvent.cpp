#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
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
	DyingBeginState,              // CallOriginal then BotDied
	DeviceFiringEndState,         // CallOriginal then jetpack-flag clear
	ServerStopFire,               // CallOriginal then RemoveEffectGroupsByCategory(stealth)
	TgEffectRemove,               // (guarded) ApplyBuffEffectFromHook in lieu of original
	CheckEffectBuffModifier,      // (guarded) scale parms->NewValue via GetBuffedProperty
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
		|| strcmp(name, "Function TgGame.TgPlayerController.GetPlayerViewPoint") == 0
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
	if (strcmp(name, "Function TgPawn.Dying.BeginState") == 0)                       return DispatchTag::DyingBeginState;
	if (strcmp(name, "Function TgDevice.DeviceFiring.EndState") == 0)                return DispatchTag::DeviceFiringEndState;
	if (strcmp(name, "Function TgGame.TgDevice.ServerStopFire") == 0)                return DispatchTag::ServerStopFire;
	if (strcmp(name, "Function TgGame.TgEffect.Remove") == 0)                        return DispatchTag::TgEffectRemove;
	if (strcmp(name, "Function TgGame.TgEffect.CheckEffectBuffModifier") == 0)       return DispatchTag::CheckEffectBuffModifier;

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
		}
		break;

	case DispatchTag::DyingBeginState: {
		CallOriginal(Object, edx, Function, Params, Result);
		// UScript TgPawn.Dying.BeginState only calls PawnDied for bIsPlayer==true.
		// For AI bots the Timer would normally call Controller.Destroy() → PawnDied,
		// but bots fall out of the world first (LifeSpan set by OutsideWorldBounds),
		// so the Timer never fires and BotDied is never called.
		// Fix: call TgBotFactory__BotDied @ 0x10a8cbf0 directly here,
		// mirroring what TgAIController.PawnDied() would do.
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
		// (`TgEffectManager__SetEffectRep.cpp`). Pulls the originally-
		// applied amount out of `effect->m_fCurrent` (which our apply
		// hook stored as PERCENT for calc 68/69) and reverses the buff
		// entry, mirroring `TgEffectBuff.uc:202`. We rely on our
		// reimplemented `TgEffectGroup__RemoveEffects` to dispatch
		// `eventRemove` via ProcessEvent — that path IS visible to this
		// hook (whereas the apply path runs invisibly, see
		// ApplyBuffEffect.hpp for the full diagnosis).
		//
		// Original outer guard was Params && IsBuff — when either fails we
		// fall through to the catch-all (log + CallOriginal) just as the
		// prior else-if chain did.
		if (Params && BuffEffectRegistry::IsBuff((UTgEffect*)Object)) {
			auto* parms = (UTgEffect_eventRemove_Parms*)Params;
			ApplyBuffEffectFromHook((UTgEffect*)Object, parms->Target, /*bRemove=*/true);
		} else {
			DoCatchAll();
		}
		break;

	case DispatchTag::CheckEffectBuffModifier:
		// CheckEffectBuffModifier is a UC native that the binary STRIPPED.
		// UC's TgEffect.ApplyEffect:115 and TgEffectDamage:131 call it to
		// scale fProratedAmount by the instigator's m_EffectBuffInfo
		// damage-modifier entries (prop 65). Without this, damage buffs
		// from Techro/Sensor stations and rolled mods that target prop 65
		// silently no-op even though the entry is correctly placed in
		// the buff registry.
		//
		// HOOK DOES NOT FIRE FOR UC CALLERS. UC bytecode dispatches
		// natives via inline VM (ProcessInternal → UFunction.Func), not
		// via UObject::ProcessEvent. This branch only catches SDK-wrapper
		// callers. The UC-call path is patched by the TgPawn.TakeDamage
		// hook below, which scales the integer Damage parm before UC's
		// TakeDamage body subtracts it from health — see that branch for
		// the working implementation. Kept here for the SDK-wrapper case.
		//
		// Original outer guard was Params != null. When Params is null we
		// fall through to the catch-all (log + CallOriginal), matching the
		// prior else-if chain.
		if (Params) {
			auto* parms = (UTgEffect_execCheckEffectBuffModifier_Parms*)Params;
			UTgEffect* effect = (UTgEffect*)Object;
			UTgEffectGroup* g = effect ? effect->m_EffectGroup : nullptr;
			AActor* instigator = g ? g->m_Instigator : nullptr;
			if (instigator && parms->NewValue != 0.0f) {
				const char* className = instigator->Class ? instigator->Class->GetFullName() : nullptr;
				if (className && strstr(className, "TgPawn") != nullptr) {
					ATgPawn* pawn = (ATgPawn*)instigator;
					typedef void(__fastcall* GetBuffedPropertyFn)(
						ATgPawn*, void*, unsigned char,
						int, int, int, int, int, float, float*, void*);
					static const GetBuffedPropertyFn GetBuffedPropertyNative =
						(GetBuffedPropertyFn)0x109d7ff0;
					float out = parms->NewValue;
					// Per-device buff scoping: the effect's m_EffectGroup
					// carries m_nSourceDeviceInstId from UC InitInstance
					// (the firing device's r_nDeviceInstanceId). Passing
					// it filters the buff registry to that device's
					// rolled mods + wildcards (skills) — preventing
					// cross-device damage-mod stacking.
					GetBuffedPropertyNative(
						pawn, /*edx=*/nullptr,
						/*eRequestContext=*/4,
						/*nPropId=*/65,                   // TGPID_DAMAGE_MODIFIER
						/*nReqCategoryCode=*/0,
						/*nReqSkillId=*/0,
						/*nReqDeviceInstId=*/g ? g->m_nSourceDeviceInstId : 0,
						/*bUsePotencyModifier=*/0,
						/*fBaseValue=*/parms->NewValue,
						/*fBuffedValue=*/&out,
						/*Effect=*/effect);
					if (out > 0.0f && out != parms->NewValue) {
						Logger::Log("effects",
							"[CHECK-BUFF-MOD] effect=%p propId=%d  base=%.3f -> buffed=%.3f  pawn=%p\n",
							effect, effect->m_nPropertyId, parms->NewValue, out, pawn);
						parms->NewValue = out;
					}
				}
			}
		} else {
			DoCatchAll();
		}
		break;

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


