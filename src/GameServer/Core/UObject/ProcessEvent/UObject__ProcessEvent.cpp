#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
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

void __fastcall UObject__ProcessEvent::Call(UObject* Object, void* edx, UFunction* Function, void* Params, void* Result) {

	if (Object && Function) {
		std::string name = std::string(Function->GetFullName());
		std::string objname = std::string(Object->GetFullName());

		// Diagnostic: log any damage-dispatch related UC event. Temporary.
		if (name.find("TakeDamage") != std::string::npos
			|| name.find("SendCombatMessage") != std::string::npos
			|| name.find("ApplyDamage") != std::string::npos
			|| name.find("ApplyHealth") != std::string::npos) {
			Logger::Log("combat-trace", "PE: %s  obj=%s\n", name.c_str(), objname.c_str());
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

