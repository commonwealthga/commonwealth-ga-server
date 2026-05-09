#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/Utils/Logger/Logger.hpp"

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

void __fastcall UObject__ProcessEvent::Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {

	if (Object && Function) {
		std::string name = std::string(Function->GetFullName());
		std::string objname = std::string(Object->GetFullName());

		// Diagnostic: log any damage-dispatch related UC event. Temporary.
		if (name.find("TakeDamage") != std::string::npos
			|| name.find("SendCombatMessage") != std::string::npos
			|| name.find("ApplyDamage") != std::string::npos
			|| name.find("ApplyHealth") != std::string::npos
			|| name.find("ApplyHit") != std::string::npos
			|| name.find("ProcessInstantHit") != std::string::npos
			|| name.find("CalcInstantFire") != std::string::npos
			|| name.find("AdjustDamage") != std::string::npos) {
			Logger::Log("combat-trace", "PE: %s  obj=%s\n", name.c_str(), objname.c_str());
		}

		// Diagnostic: log UC dispatch on TgDeployable, EXCLUDING per-tick
		// noise (Tick, RefireCheckTimer, RecheckActiveTimer, Touch, UnTouch,
		// SetProperty, Deploy.Tick — none of these are part of the damage
		// flow). Want to see ProcessEffect, TakeDamage, eventXxx etc.
		if ((objname.find("TgDeployable") != std::string::npos
				|| objname.find("TgDeploy_") != std::string::npos)
			&& name.find(".Tick") == std::string::npos
			&& name.find("RefireCheckTimer") == std::string::npos
			&& name.find("RecheckActiveTimer") == std::string::npos
			&& name.find(".Touch") == std::string::npos
			&& name.find(".UnTouch") == std::string::npos
			&& name.find(".SetProperty") == std::string::npos) {
			Logger::Log("combat-trace", "PE-DEP: %s  obj=%s\n",
				name.c_str(), objname.c_str());
		}

		// Diagnostic: hitscan-chain dispatches — exclude ServerStopFire
		// (~350x per ServerStartFire, pure noise) and the Get* helpers.
		if ((name.find("Fire") != std::string::npos
				|| name.find("Trace") != std::string::npos
				|| name.find("Impact") != std::string::npos
				|| name.find("Hit") != std::string::npos)
			&& name.find("ServerStopFire") == std::string::npos
			&& name.find("GetCone") == std::string::npos
			&& name.find("GetInterruption") == std::string::npos) {
			if (objname.find("TgDeviceFire") != std::string::npos
				|| objname.find("TgDevice_") != std::string::npos
				|| objname.find("TgProj") != std::string::npos
				|| objname.find("TgPawn_Character") != std::string::npos) {
				Logger::Log("combat-trace", "PE-FIRE: %s  obj=%s\n",
					name.c_str(), objname.c_str());
			}
		}

		// Diagnostic: log every Actor.ReplicatedEvent (and subclass overrides)
		// with the VarName that just replicated. Used to determine whether
		// r_bIsStealthed / r_nApplyStealth are actually reaching the owning
		// client, since the client-side stealth UC branches never fire.
		// The first param (offset 0) is FName VarName — see
		// AActor_eventReplicatedEvent_Parms. Channel enabled in dllmainclient.cpp.
		if (Params && name.size() >= 16
			&& name.compare(name.size() - 16, 16, ".ReplicatedEvent") == 0) {
			FName* VarName = (FName*)Params;
			char* varStr = VarName->GetName();
			Logger::Log("replicated_event", "RE: var=%s  obj=%s  fn=%s\n",
				varStr ? varStr : "<null>", objname.c_str(), name.c_str());
		}

		// Stealth-submission trace: every step of the Fire → SubmitEffect →
		// ProcessEffect → ApplyEffects chain. Excludes ApplyStealth (firehose
		// from AI assassins) and TgEffectForm (client-visual not server-state).
		if ((name.find("ApplyEffect") != std::string::npos
				|| name.find("ProcessEffect") != std::string::npos
				|| name.find("SubmitEffect") != std::string::npos
				|| name.find("ApplyEffectType") != std::string::npos
				|| name.find("RemoveEffectType") != std::string::npos
				|| name.find("TgEffect.Remove") != std::string::npos
				|| name.find("TgEffectGroup.Remove") != std::string::npos
				|| name.find("ApplyToProperty") != std::string::npos
				|| name.find("ApplyAimEffects") != std::string::npos
				|| name.find("RemoveAimEffects") != std::string::npos)
			&& name.find("ApplyStealth") == std::string::npos
			&& name.find("TgEffectForm") == std::string::npos) {
			Logger::Log("effects", "[PE] %s  obj=%s\n", name.c_str(), objname.c_str());
		}

		// Skill-UI trace: log every UFunction dispatch on a TgUIAgentProfile_Skill
		// instance, plus the key server-side RPC (SendCharacterSkillMarshal) and
		// CGameClient handlers. Lets us answer: does OnSkillButtonDelegate fire on
		// click? does OnSaveDelegate fire on Save? Does the send path actually
		// reach CGameClient::SendSkillsToServer? Channel "skills-trace".
		if (objname.find("TgUIAgentProfile_Skill") != std::string::npos
			|| name.find("UIAgentProfile_Skill") != std::string::npos
			|| name.find("SendCharacterSkillMarshal") != std::string::npos
			|| name.find("SendSkillsToServer") != std::string::npos
			|| name.find("ReapplyCharacterSkillTree") != std::string::npos
			|| name.find("RemoveCharacterSkillTree") != std::string::npos
			|| name.find("ProcessSkillPromptResponse") != std::string::npos) {
			Logger::Log("skills-trace", "[PE] %s  obj=%s\n", name.c_str(), objname.c_str());
		}

		// Stealth state snapshot: log pawn's stealth-related fields when
		// ApplyStealth fires — tells us whether ApplyProperty case 0x7C actually
		// flipped the bit, vs. ApplyStealth being dispatched via some path that
		// didn't touch prop 124.
		if (strcmp("Function TgGame.TgPawn.ApplyStealth", name.c_str()) == 0) {
			ATgPawn* Pawn = (ATgPawn*)Object;
			if (Pawn) {
				Logger::Log("effects",
					"[STEALTH] ApplyStealth state: pawn=%s r_bIsStealthed=%d m_fStealthTransTime=%.3f r_nApplyStealth=%d r_nStealthDisabled=%d\n",
					Pawn->GetFullName(),
					(int)(Pawn->r_bIsStealthed ? 1 : 0),
					Pawn->r_fStealthTransitionTime,
					Pawn->r_nApplyStealth,
					Pawn->r_nStealthDisabled);
			}
		}

		// Per-fire-tick refresh for hold-to-sustain stealth. The UC state
		// machine's DeviceFiring state arms `RefireCheckTimer` on a recurring
		// timer while the fire button is held; it stops firing the moment the
		// button releases (the timer is canceled as part of the state exit).
		// That's exactly the "while firing" signal we need, and it fires at
		// the device's refire rate (faster than our 1s effect lifetime for any
		// sane stealth device).
		// Previous attempt hooked TgPawn.ApplyStealth but that UC event only
		// fires when SetProperty(124) is called, which happens only on fire-
		// start for application_value=156 stealth effect groups (UC's
		// "Newest Wins" re-submit path discards duplicates instead of
		// re-applying) — so the refresh never triggered and the effect
		// expired at 1s regardless of hold duration.
		if (strcmp("Function TgDevice.DeviceFiring.RefireCheckTimer", name.c_str()) == 0) {
			ATgDevice* Device = (ATgDevice*)Object;
			if (Device && Device->Instigator && HasStealthEffectGroup(Device)) {
				RefreshStealthEffectTimers((ATgPawn*)Device->Instigator);
			}
		}

		// Deployable state-machine diagnostic — traces Deploy → Active →
		// DeviceBuildup → DeviceFiring transitions and the DeviceFiring
		// refire timer.  If the medstation (or any persistent-tick deployable)
		// never reaches DeviceFiring.BeginState, whichever preceding step is
		// missing tells us where the UC state machine stalls.
		//
		// State-method UFunction names drop the package prefix (see
		// reference_state_ufunction_names.md): Function <Class>.<State>.<Method>
		// — NOT Function TgGame.<Class>.<State>.<Method>.
		//
		// Also taps class-level StartFire on both the deployable and the device
		// (prefix kept — non-state methods keep `TgGame.`).  A station's
		// Active.BeginState typically calls StartFire to transition into
		// DeviceBuildup; if the log shows Active.BeginState but no StartFire,
		// Active itself is the dead end.
		if (strncmp("Function TgDeployable.", name.c_str(), 22) == 0) {
			const char* suffix = name.c_str() + 22;
			if (strcmp(suffix, "Deploy.BeginState") == 0
			 || strcmp(suffix, "Deploy.EndState") == 0
			 || strcmp(suffix, "Deploy.StartFire") == 0
			 || strcmp(suffix, "Active.BeginState") == 0
			 || strcmp(suffix, "DeviceBuildup.BeginState") == 0
			 || strcmp(suffix, "DeviceBuildup.EndState") == 0
			 || strcmp(suffix, "DeviceBuildup.DeployableTimer") == 0
			 || strcmp(suffix, "DeviceFiring.BeginState") == 0
			 || strcmp(suffix, "DeviceFiring.EndState") == 0
			 || strcmp(suffix, "DeviceFiring.RefireCheckTimer") == 0) {
				ATgDeployable* D = (ATgDeployable*)Object;
				// Diagnostic only — observe the actor flag dword + cylinder
				// flag dword at each state transition to verify the
				// post-ApplyDeployableSetup CollisionType revert held.
				// Field offsets per Engine SDK header:
				//   AActor +0xB0:      bCanBeDamaged=0x80000, bCollideActors=0x8000000,
				//                      bCollideWorld=0x10000000, bBlockActors=0x40000000,
				//                      bProjTarget=0x80000000.
				//   UPrimitive +0x128: CollideActors=0x10, BlockActors=0x40,
				//                      BlockZeroExtent=0x80, BlockNonZeroExtent=0x100,
				//                      BlockRigidBody=0x400.
				uint32_t actorFlags = D ? *(uint32_t*)((char*)D + 0xB0) : 0;
				uint32_t cylFlags = (D && D->CollisionComponent)
					? *(uint32_t*)((char*)D->CollisionComponent + 0x128) : 0;
				Logger::Log("heal_tick",
					"[DEPLOYABLE STATE] %s  deployable=0x%p id=%d mFireMode=%p props=%d role=%d bIsDeployed=%d bInDestroyed=%d\n"
					"                   actorFlags=0x%08x (bCollide=%d bBlock=%d bProj=%d bDmg=%d bWorld=%d)\n"
					"                   cylComp=%p cylFlags=0x%08x (CollideActors=%d BlockActors=%d BlockZero=%d BlockNonZero=%d)\n",
					suffix, D, D ? D->r_nDeployableId : -1,
					D ? (void*)D->m_FireMode : nullptr,
					D ? D->s_Properties.Count : -1,
					D ? (int)D->Role : -1,
					D ? (int)D->m_bIsDeployed : -1,
					D ? (int)D->m_bInDestroyedState : -1,
					actorFlags,
					(int)((actorFlags >> 27) & 1),
					(int)((actorFlags >> 30) & 1),
					(int)((actorFlags >> 31) & 1),
					(int)((actorFlags >> 19) & 1),
					(int)((actorFlags >> 28) & 1),
					D ? (void*)D->CollisionComponent : nullptr,
					cylFlags,
					(int)((cylFlags >> 4) & 1),
					(int)((cylFlags >> 6) & 1),
					(int)((cylFlags >> 7) & 1),
					(int)((cylFlags >> 8) & 1));
			}
		}
		if (strcmp("Function TgGame.TgDeployable.StartFire", name.c_str()) == 0) {
			ATgDeployable* D = (ATgDeployable*)Object;
			Logger::Log("heal_tick",
				"[DEPLOYABLE] StartFire() called  deployable=0x%p id=%d\n",
				D, D ? D->r_nDeployableId : -1);
		}


		// Diagnostic: log every TgDeployable.UpdateHealth call. UC's
		// TgEffectHeal.ApplyEffect (prop 260 / Repair) dispatches here for
		// deployable targets, so a repair-arm pulse on a station should
		// produce one of these per pulse — and the resulting r_nHealth /
		// DRI.r_nHealthCurrent change is the "did the heal land?" answer.
		// Channel: heal_tick (already enabled when investigating station
		// repair). Prints the new HP, current HP before, and the address of
		// the calling Function so we can tell whether it came from the
		// effect-system path or somewhere else (e.g. internal damage).
		if (Params && strcmp("Function TgGame.TgDeployable.UpdateHealth", name.c_str()) == 0) {
			ATgDeployable* D = (ATgDeployable*)Object;
			int newHp = *(int*)Params;
			Logger::Log("heal_tick",
				"[DEPLOYABLE] UpdateHealth: deployable=0x%p id=%d  r_nHealth %d -> %d  "
				"r_DRI=%p DRI.r_nHealthCurrent=%d\n",
				D, D ? D->r_nDeployableId : -1,
				D ? D->r_nHealth : -1, newHp,
				D ? (void*)D->r_DRI : nullptr,
				(D && D->r_DRI) ? D->r_DRI->r_nHealthCurrent : -1);
		}
		// Device-level state machine (independent of the deployable's).  Only
		// fire modes on deployables run — noise from player weapons is filtered
		// via the Owner check: a UTgDeviceFire owned by an ATgDeployable is
		// what we want.
		if (strncmp("Function TgDevice.", name.c_str(), 18) == 0) {
			const char* suffix = name.c_str() + 18;
			if (strcmp(suffix, "DeviceBuildup.BeginState") == 0
			 || strcmp(suffix, "DeviceBuildup.EndState") == 0
			 || strcmp(suffix, "DeviceFiring.BeginState") == 0) {
				// Object is a UTgDeviceFire or an ATgDevice — log class name
				// so player-weapon noise is visually separable from station
				// fire-mode transitions when scanning the log.
				UObject* obj = (UObject*)Object;
				if (obj) {
					const char* cn = obj->Class ? obj->Class->GetFullName() : nullptr;
					Logger::Log("heal_tick",
						"[DEVICE STATE] %s  obj=0x%p class=%s\n",
						suffix, obj, cn ? cn : "<null>");
				}
			}
		}

		if (strcmp("Function Engine.Actor.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.GameInfoDataProvider.ProviderInstanceBound", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPlayerController.GetPlayerViewPoint", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn.ShouldRechargePowerPool", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn_Character.Tick", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDeployable.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.PlayerController.SendClientAdjustment", name.c_str()) == 0
		|| strcmp("Function TgGame.TgMissionObjective_Proximity.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.PlayerController.ServerMove", name.c_str()) == 0
		|| strcmp("Function Engine.PlayerController.OldServerMove", name.c_str()) == 0
		|| strcmp("Function TgGame.TgMissionObjective.IsLocalPlayerAttacker", name.c_str()) == 0
		|| strcmp("Function TgGame.TgProperty.Copy", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDeploy_BeaconEntrance.Touch", name.c_str()) == 0
		|| strcmp("Function TgGame.TgMissionObjective_Bot.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.PreBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.SetInitialState", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.Emitter.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.LadderVolume.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function Engine.KAsset.PostBeginPlay", name.c_str()) == 0
		|| strcmp("Function TgGame.TgRepInfo_Player.Timer", name.c_str()) == 0
		|| strcmp("Function TgGame.GameRunning.Timer", name.c_str()) == 0
		|| strcmp("Function Engine.GameReplicationInfo.Timer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn.GetCameraValues", name.c_str()) == 0
		|| strcmp("Function TgGame_Defense.RoundInProgress.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.Actor.Touch", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDeploy_BeaconEntrance.RecheckActiveTimer", name.c_str()) == 0
		|| strcmp("Function TgGame_Arena.RoundInProgress.Tick", name.c_str()) == 0
		|| strcmp("Function TgPawn.Dying.Tick", name.c_str()) == 0
		|| strcmp("Function Engine.GameInfo.Timer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgRepInfo_Game.ServerUpdateTimer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgPawn.IsCrewed", name.c_str()) == 0
		|| strcmp("Function TgGame.TgDevice.IsOffhand", name.c_str()) == 0
		|| strcmp("Function TgGame.TgAIController.SeePlayer", name.c_str()) == 0
		|| strcmp("Function TgGame.TgSkeletalMeshActorNPC.Tick", name.c_str()) == 0
		|| strcmp("Function TgGame.TgSkeletalMeshActor_CharacterBuilder.Tick", name.c_str()) == 0
		|| strcmp("Function TgGame.TgInterpolatingCameraActor.Tick", name.c_str()) == 0
		// || strcmp("Function Engine.SequenceOp.Activated", name.c_str()) == 0
		) {
			CallOriginal(Object, edx, Function, Params, Result);
		// } else if (strcmp("Function TgGame.TgDevice.CanDeviceStartFiringNow", name.c_str()) == 0) {
		// 	ATgDevice_eventCanDeviceStartFiringNow_Parms* CanDeviceStartFiringNowParams = (ATgDevice_eventCanDeviceStartFiringNow_Parms*)Params;
		// 	CanDeviceStartFiringNowParams->ReturnValue = true;
		// } else if (strcmp("Function TgGame.TgDevice.CanDeviceFireNow", name.c_str()) == 0) {
		// 	ATgDevice_eventCanDeviceFireNow_Parms* CanDeviceFireNowParams = (ATgDevice_eventCanDeviceFireNow_Parms*)Params;
		// 	CanDeviceFireNowParams->ReturnValue = true;


		} else if (strcmp("Function TgPawn.Dying.BeginState", name.c_str()) == 0) {
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
					Logger::Log(GetLogChannel(), "Dying.BeginState: calling BotDied on factory for %s\n", Pawn->GetFullName());
					((void(__thiscall*)(ATgBotFactory*, ATgPawn*, ATgAIController*))0x10a8cbf0)(AIC->m_pFactory, Pawn, AIC);
					AIC->m_pFactory = nullptr;
				}
			}
		} else if (strcmp("Function TgDevice.DeviceFiring.EndState", name.c_str()) == 0) {
			CallOriginal(Object, edx, Function, Params, Result);
			ATgDevice* Device = (ATgDevice*)Object;
			if (Device->IsOffhandJetpack() && Device->Instigator) {
				ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
				SetPawnProperty(Pawn, 59, 0.0f);  // clear TGPID_FLIGHT_ACCELERATION
				Pawn->bNetDirty = 1;
				Pawn->bForceNetUpdate = 1;
			}

		} else if (strcmp("Function TgGame.TgDevice.ServerStopFire", name.c_str()) == 0) {
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

		} else if (Params
				&& strcmp("Function TgGame.TgEffect.Remove", name.c_str()) == 0
				&& BuffEffectRegistry::IsBuff((UTgEffect*)Object)) {
			// Symmetric counterpart to the SetEffectRep-driven apply
			// (`TgEffectManager__SetEffectRep.cpp`). Pulls the originally-
			// applied amount out of `effect->m_fCurrent` (which our apply
			// hook stored as PERCENT for calc 68/69) and reverses the buff
			// entry, mirroring `TgEffectBuff.uc:202`. We rely on our
			// reimplemented `TgEffectGroup__RemoveEffects` to dispatch
			// `eventRemove` via ProcessEvent — that path IS visible to this
			// hook (whereas the apply path runs invisibly, see
			// ApplyBuffEffect.hpp for the full diagnosis).
			auto* parms = (UTgEffect_eventRemove_Parms*)Params;
			ApplyBuffEffectFromHook((UTgEffect*)Object, parms->Target, /*bRemove=*/true);

		} else if (Params
				&& strcmp("Function TgGame.TgEffect.CheckEffectBuffModifier", name.c_str()) == 0) {
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
			auto* parms = (UTgEffect_execCheckEffectBuffModifier_Parms*)Params;
			UTgEffect* effect = (UTgEffect*)Object;
			UTgEffectGroup* g = effect ? effect->m_EffectGroup : nullptr;
			AActor* instigator = g ? g->m_Instigator : nullptr;
			if (instigator && parms && parms->NewValue != 0.0f) {
				const char* className = instigator->Class ? instigator->Class->GetFullName() : nullptr;
				if (className && strstr(className, "TgPawn") != nullptr) {
					ATgPawn* pawn = (ATgPawn*)instigator;
					typedef void(__fastcall* GetBuffedPropertyFn)(
						ATgPawn*, void*, unsigned char,
						int, int, int, int, int, float, float*, void*);
					static const GetBuffedPropertyFn GetBuffedPropertyNative =
						(GetBuffedPropertyFn)0x109d7ff0;
					float out = parms->NewValue;
					GetBuffedPropertyNative(
						pawn, /*edx=*/nullptr,
						/*eRequestContext=*/4,
						/*nPropId=*/65,                   // TGPID_DAMAGE_MODIFIER
						/*nReqCategoryCode=*/0,
						/*nReqSkillId=*/0,
						/*nReqDeviceInstId=*/0,
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

		// DEAD CODE — left commented for archaeology. Hypothesized that
		// FUNC_Event UC functions called from UC bytecode dispatch through
		// ProcessEvent, so a TakeDamage hook here would catch the damage
		// path. Verified WRONG via combat-trace: zero `PE: …TakeDamage…`
		// lines across full damage cycles. UC-to-UC dispatches via inline
		// VM (CallFunction → ProcessInternal → run bytecode) regardless of
		// FUNC_Event. Damage-buff scaling now lives in CloneEffectGroup
		// (m_fBase pre-scale on cloned damage effects) — see the [DAMAGE-
		// BUFF] log line emitted there.
		// } else if (Params
		// 		&& name.size() >= 11
		// 		&& name.compare(name.size() - 11, 11, ".TakeDamage") == 0
		// 		&& name.compare(0, sizeof("Function TgGame.")-1, "Function TgGame.") == 0) {
		} else if (false) {
			// Damage-buff scaling happens HERE — *.TakeDamage are FUNC_Event
			// (FunctionFlags 0x00024802 includes 0x20000 = FUNC_Event), so
			// UC's `Target.TakeDamage(int(fProratedAmount), …)` from
			// TgEffectDamage dispatches via ProcessEvent and lands in this
			// hook. The CheckEffectBuffModifier hook above can NOT see UC-
			// side calls (inline VM native-dispatch bypasses ProcessEvent),
			// which is why fixed-base damage was leaving the player's
			// prop-65 buff unapplied.
			//
			// Param layout: every *.TakeDamage variant in TgGame_f_structs.h
			// (TgPawn, TgDeployable, TgPawn_Scanner, TgDeploy_ForceField,
			// TgDynamicSMActor, TgFracturedStaticMeshActor,
			// TgObjectiveAttachActor) has the same first 0x44 bytes:
			//   +0x00 int Damage     +0x04 AController* InstigatedBy
			//   +0x40 AActor* DamageCauser
			//
			// Resolution priority for the buff-source pawn:
			//   1. InstigatedBy.Pawn — the controller drives the firing pawn,
			//      which is what's looked up for buffs (player/AI alike).
			//   2. DamageCauser — fallback when no controller (e.g. trigger
			//      damage). Only used if it's itself a pawn; projectiles and
			//      deployables-as-causer don't carry buffs.
			//
			// Order-of-operations vs. UC: UC's TgEffectDamage::ApplyEffect
			// computes fProratedAmount × buff THEN applies protection. We
			// scale at TakeDamage which is post-protection. For prop-65 calc
			// 68 (PERC_INCREASE) this is mathematically identical because
			// (1+buff)*(1-armor) is commutative under multiplication. An
			// absolute (calc-67 ADD) prop-65 would differ by a protection
			// factor — none in the current DB; acceptable until concrete.
			int* pDamage = (int*)Params;
			if (pDamage && *pDamage > 0) {
				AController* instigatedBy = *(AController**)((char*)Params + 0x4);
				AActor*      causer       = *(AActor**)((char*)Params + 0x40);

				ATgPawn* causerPawn = nullptr;
				if (instigatedBy && instigatedBy->Pawn) {
					ATgPawn* p = (ATgPawn*)instigatedBy->Pawn;
					const char* cn = p->Class ? p->Class->GetFullName() : nullptr;
					if (cn && strstr(cn, "TgPawn") != nullptr) causerPawn = p;
				}
				if (!causerPawn && causer) {
					const char* cn = causer->Class ? causer->Class->GetFullName() : nullptr;
					if (cn && strstr(cn, "TgPawn") != nullptr) causerPawn = (ATgPawn*)causer;
				}

				if (causerPawn) {
					typedef void(__fastcall* GetBuffedPropertyFn)(
						ATgPawn*, void*, unsigned char,
						int, int, int, int, int, float, float*, void*);
					static const GetBuffedPropertyFn GetBuffedPropertyNative =
						(GetBuffedPropertyFn)0x109d7ff0;
					float baseValue = (float)*pDamage;
					float out = baseValue;
					GetBuffedPropertyNative(
						causerPawn, /*edx=*/nullptr,
						/*eRequestContext=*/4,            // BUFF_OTHER (identity for prop 65)
						/*nPropId=*/65,                   // TGPID_DAMAGE_MODIFIER
						/*nReqCategoryCode=*/0,
						/*nReqSkillId=*/0,
						/*nReqDeviceInstId=*/0,
						/*bUsePotencyModifier=*/0,
						/*fBaseValue=*/baseValue,
						/*fBuffedValue=*/&out,
						/*Effect=*/nullptr);
					if (out > baseValue) {
						int newDamage = (int)(out + 0.5f);
						Logger::Log("effects",
							"[DAMAGE-BUFF] %s  %d -> %d  (+%.1f%%)  causer=%p target=%s\n",
							name.c_str(), *pDamage, newDamage,
							(out / baseValue - 1.0f) * 100.0f,
							(void*)causerPawn, objname.c_str());
						*pDamage = newDamage;
					}
				}
			}
			CallOriginal(Object, edx, Function, Params, Result);

		} else if (Params
				&& (strcmp("Function TgGame.TgPawn_Character.ReplicatedEvent", name.c_str()) == 0
					|| strcmp("Function TgGame.TgPawn.ReplicatedEvent", name.c_str()) == 0)) {
			// UC ApplyStealthClient only calls SwapMeshToStealthed(false) — the UC path for
			// stealth-ON is missing, so player pawns never become visually stealthed when
			// r_bIsStealthed flips to true. UC body runs first (super dispatches inline,
			// not via ProcessEvent), then we invoke the native TgPawn_Character override
			// directly to bypass recursion through this hook.
			// Guarded to player pawns on client (not bots, not server-authority) so we
			// don't perturb AI assassins' existing visual mechanism.
			CallOriginal(Object, edx, Function, Params, Result);

			FName* VarName = (FName*)Params;
			char* varStr = VarName ? VarName->GetName() : nullptr;
			if (varStr
					&& (strcmp(varStr, "r_bIsStealthed") == 0
						|| strcmp(varStr, "r_nApplyStealth") == 0)) {
				ATgPawn* Pawn = (ATgPawn*)Object;
				if (Pawn && !Pawn->r_bIsBot && Pawn->Role != 3 /* ROLE_Authority */ && Pawn->Mesh) {
					int stealthed = Pawn->r_bIsStealthed ? 1 : 0;
					// Dispatch to the correct override. Character's version swaps main mesh
					// + 4 attachments + weapon mesh; base handles only the main mesh.
					// Internally idempotent via c_SavedMICForStealth (pawn+0xF28).
					bool isCharacter = objname.compare(0, 17, "TgPawn_Character ") == 0;
					uintptr_t swapAddr = isCharacter ? 0x109d8c70 : 0x109d8310;
					((void(__thiscall*)(ATgPawn*, int))swapAddr)(Pawn, stealthed);
					Logger::Log("effects", "[STEALTH] SwapMeshToStealthed(%d) on %s via %s  m_fMakeVisibleCurrent=%.3f c_SavedMICForStealth=%p\n",
						stealthed, objname.c_str(), varStr,
						Pawn->m_fMakeVisibleCurrent,
						(void*)Pawn->c_SavedMICForStealth);
				}
			}

		} else {
			Logger::Log(GetLogChannel(), "├─ %s [%s]\n", name.c_str(), objname.c_str());
			Logger::ChannelIndents[GetLogChannel()]++;

			CallOriginal(Object, edx, Function, Params, Result);

			Logger::ChannelIndents[GetLogChannel()]--;
			// Logger::Log(GetLogChannel(), "}\n");
		}
	}
}

