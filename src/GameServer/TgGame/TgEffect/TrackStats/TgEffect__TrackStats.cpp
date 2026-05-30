#include "src/GameServer/TgGame/TgEffect/TrackStats/TgEffect__TrackStats.hpp"
#include "src/GameServer/Combat/SendKillAlert/SendKillAlert.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackBotHealing/TgPawn__TrackBotHealing.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDamagedBot/TgPawn__TrackDamagedBot.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDamagedPlayer/TgPawn__TrackDamagedPlayer.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackDefense/TgPawn__TrackDefense.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackHealing/TgPawn__TrackHealing.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.hpp"
#include "src/GameServer/TgGame/TgPawn/TrackKilledBot/TgPawn__TrackKilledBot.hpp"
#include "src/GameServer/TgGame/Morale/MoraleCredit.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string>

namespace {

// FString lifecycle pattern (used by FireAdd*Alert below): construct fresh
// copies via FString(wchar_t*) → new[] allocates Data, pass-by-memcpy into
// Parms struct (CPF_NeedCtorLink), call ProcessEvent, then null out the
// local FString fields. ProcessEvent's NeedCtorLink cleanup appFree's Data;
// nulling skips the C++ destructor's redundant `delete[]`. Same pattern as
// SpawnPlayerCharacter.cpp:277-285.

// UE3 UFunction flags we toggle to force RPC marshaling.
constexpr uint32_t FUNC_Net         = 0x00000040;
constexpr uint32_t FUNC_NetReliable = 0x00000080;
constexpr uint32_t FUNC_NetClient   = 0x01000000;

// Fire TgPlayerController::AddKillAlert / AddAssistAlert on the target PC,
// forcing it to run as a reliable-client RPC even though UC declares the
// function as a plain `native function` (flags 0x00020400 = Public|Native,
// no Net flags).
//
// Why the dance: these alert natives' bodies build a CMarshal row and call
// PC vtable[0x734] — the local HUD alert dispatcher that renders the same
// toast as AddMoraleReadyAlert. ProcessEvent on a non-Net function runs
// the native body LOCALLY; on the server that's a no-op (no HUD). To get
// the toast on the client, we add FUNC_Net|FUNC_NetClient|FUNC_NetReliable
// to the function's flags just for this call, so the engine's send-side
// RPC machinery picks it up and marshals to the client connection. The
// client receives it, looks up the same UFunction* locally (its flags are
// untouched on that side), and dispatches the native — vtable[0x734]
// fires on the client side and the toast renders.
//
// We restore the original flags after the call so we don't permanently
// mutate the UFunction (unlike the SDK header's buggy `|= ~0x400` which
// ends up setting nearly every flag forever).
void DispatchAsClientRPC(UFunction* pFn, ATgPlayerController* PC,
                         void* parms) {
	if (!pFn || !PC || !parms) return;
	const uint32_t origFlags = pFn->FunctionFlags;
	pFn->FunctionFlags = origFlags | FUNC_Net | FUNC_NetReliable | FUNC_NetClient;
	PC->ProcessEvent(pFn, parms, nullptr);
	pFn->FunctionFlags = origFlags;
}

void FireAddKillAlert(ATgPlayerController* KillerPC, wchar_t* killedSrc,
                      wchar_t* killerSrc, bool killerWasPlayer) {
	if (!KillerPC || !killedSrc) {
		if (Logger::IsChannelEnabled("kills")) {
			Logger::Log("kills", "[FireAddKillAlert] skip — KillerPC=%p killedSrc=%p\n",
				(void*)KillerPC, (void*)killedSrc);
		}
		return;
	}
	static UFunction* pFn = nullptr;
	if (!pFn) {
		pFn = (UFunction*)ObjectCache::Find(
			"Function TgGame.TgPlayerController.AddKillAlert");
	}
	if (!pFn) {
		if (Logger::IsChannelEnabled("kills")) {
			Logger::Log("kills", "[FireAddKillAlert] skip — UFunction not found\n");
		}
		return;
	}

	FString killedNameCopy(killedSrc);
	FString killerNameCopy(killerSrc && killerSrc[0] ? killerSrc : (wchar_t*)L"");

	ATgPlayerController_execAddKillAlert_Parms parms = {};
	memcpy(&parms.KilledName, &killedNameCopy, sizeof(FString));
	memcpy(&parms.KillerName, &killerNameCopy, sizeof(FString));
	parms.KillerWasPlayer = killerWasPlayer ? 1 : 0;

	if (Logger::IsChannelEnabled("kills")) {
		Logger::Log("kills",
			"[FireAddKillAlert] PC=%s killed='%ls' killer='%ls' wasPlayer=%d origFlags=0x%08X PC->Player=%p\n",
			KillerPC->GetName(),
			killedSrc, killerSrc && killerSrc[0] ? killerSrc : L"",
			(int)killerWasPlayer,
			pFn->FunctionFlags,
			(void*)KillerPC->Player);
	}

	DispatchAsClientRPC(pFn, KillerPC, &parms);

	killedNameCopy.Data = nullptr;
	killedNameCopy.Count = killedNameCopy.Max = 0;
	killerNameCopy.Data = nullptr;
	killerNameCopy.Count = killerNameCopy.Max = 0;
}

// Fire `Function TgGame.TgPlayerController.ClientAddKilled` — a real
// reliable-client UC event (FUNC_NetClient set natively) whose UC body
// routes to `TgHUD.AddNewKilledTarget` → `m_PrimaryHUD.m_KillDisplay`
// (the kill-feed widget). Standard RPC marshaling: ProcessEvent on the
// server-side PC and the engine handles client delivery.
void FireClientAddKilled(ATgPlayerController* KillerPC, wchar_t* killedSrc,
                         wchar_t* killerSrc, bool killerWasPlayer) {
	if (!KillerPC || !killedSrc) return;
	static UFunction* pFn = nullptr;
	if (!pFn) {
		pFn = (UFunction*)ObjectCache::Find(
			"Function TgGame.TgPlayerController.ClientAddKilled");
	}
	if (!pFn) return;

	FString killedNameCopy(killedSrc);
	FString killerNameCopy(killerSrc && killerSrc[0] ? killerSrc : (wchar_t*)L"");

	ATgPlayerController_eventClientAddKilled_Parms parms = {};
	memcpy(&parms.KilledName, &killedNameCopy, sizeof(FString));
	memcpy(&parms.KillerName, &killerNameCopy, sizeof(FString));
	parms.KillerWasPlayer = killerWasPlayer ? 1 : 0;

	if (Logger::IsChannelEnabled("kills")) {
		Logger::Log("kills",
			"[FireClientAddKilled] PC=%s killed='%ls' killer='%ls' wasPlayer=%d funcFlags=0x%08X\n",
			KillerPC->GetName(),
			killedSrc, killerSrc && killerSrc[0] ? killerSrc : L"",
			(int)killerWasPlayer,
			pFn->FunctionFlags);
	}

	KillerPC->ProcessEvent(pFn, &parms, nullptr);

	killedNameCopy.Data = nullptr;
	killedNameCopy.Count = killedNameCopy.Max = 0;
	killerNameCopy.Data = nullptr;
	killerNameCopy.Count = killerNameCopy.Max = 0;
}

// AssistAlert takes only KilledName + KillerName (no KillerWasPlayer).
// Fired ON THE ASSISTER'S PC, so the assister sees "you assisted in killing X".
void FireAddAssistAlert(ATgPlayerController* AssisterPC, wchar_t* killedSrc,
                        wchar_t* killerSrc) {
	if (!AssisterPC || !killedSrc) return;
	static UFunction* pFn = nullptr;
	if (!pFn) {
		pFn = (UFunction*)ObjectCache::Find(
			"Function TgGame.TgPlayerController.AddAssistAlert");
	}
	if (!pFn) return;

	FString killedNameCopy(killedSrc);
	FString killerNameCopy(killerSrc && killerSrc[0] ? killerSrc : (wchar_t*)L"");

	ATgPlayerController_execAddAssistAlert_Parms parms = {};
	memcpy(&parms.KilledName, &killedNameCopy, sizeof(FString));
	memcpy(&parms.KillerName, &killerNameCopy, sizeof(FString));

	if (Logger::IsChannelEnabled("kills")) {
		Logger::Log("kills",
			"[FireAddAssistAlert] PC=%s killed='%ls' killer='%ls' PC->Player=%p\n",
			AssisterPC->GetName(),
			killedSrc, killerSrc && killerSrc[0] ? killerSrc : L"",
			(void*)AssisterPC->Player);
	}

	DispatchAsClientRPC(pFn, AssisterPC, &parms);

	killedNameCopy.Data = nullptr;
	killedNameCopy.Count = killedNameCopy.Max = 0;
	killerNameCopy.Data = nullptr;
	killerNameCopy.Count = killerNameCopy.Max = 0;
}

// Resolve a fire-mode reference to the killer's device id. Mirrors
// `Class'TgGame.TgEffect'.static.GetDeviceIdFromMode` (a stripped native);
// the call chain is fireMode → m_Owner (ATgDevice) → r_nDeviceId. Drives the
// m_KilledByWeapon UILabel on the release dialog.
int ResolveDeviceIdFromFireMode(UObject* deviceModeRef) {
	if (!deviceModeRef) return 0;
	UTgDeviceFire* fireMode = (UTgDeviceFire*)deviceModeRef;
	AActor* owner = fireMode->m_Owner;
	if (!owner) return 0;
	// Class-name substring per reference_sdk_staticclass_misalignment — IsA is
	// unreliable on this server build. Cached lookup, no per-call alloc.
	if (!ObjectClassCache::ClassNameContains(owner->Class, "TgDevice")) return 0;
	return ((ATgDevice*)owner)->r_nDeviceId;
}

// Populate Victim->m_DeathZoomInfo via the UC event SaveDeathInfoForZoomCam.
// The UC body (TgPawn.uc:9345-9382) handles Detonator unwrap and OwnerPRI
// resolution. Early-outs unless Controller is a TgPlayerController, so safe
// to call on any victim — bots no-op.
void FireSaveDeathInfoForZoomCam(ATgPawn* Victim, ATgPawn* KillerForUC,
                                  ATgPawn* KillerOwner, int deviceId, bool bPetKill) {
	if (!Victim || !KillerForUC) return;
	static UFunction* pFnSaveDeath = nullptr;
	if (!pFnSaveDeath) {
		pFnSaveDeath = (UFunction*)ObjectCache::Find(
			"Function TgGame.TgPawn.SaveDeathInfoForZoomCam");
	}
	if (!pFnSaveDeath) return;

	ATgPawn_eventSaveDeathInfoForZoomCam_Parms parms = {};
	parms.KillerOwner = KillerOwner;
	parms.Killer      = KillerForUC;
	parms.DeviceID    = deviceId;
	parms.bPetKill    = bPetKill ? 1 : 0;
	Victim->ProcessEvent(pFnSaveDeath, &parms, nullptr);
}

}  // namespace

// Stats dispatcher only. Morale credit (rate, Output Mod scaling, anti-feedback
// filter, accumulator math, replicated mirror) lives in MoraleCredit::Award —
// invoked at the end of this function. Per the binary's AddMoralePoints native
// decompile + the data audit (2026-05-19), morale gain has no DB-driven rate
// property; constants are calibrated from old-GA playtest evidence and live
// in the MoraleCredit module.
void __fastcall TgEffect__TrackStats::Call(UTgEffect* /*Effect*/, void* /*edx*/,
                                           ATgPawn* Instigator, AActor* Target, FImpactInfoBytes Impact,
                                           float fDamage, int iTargetDeviceModeId,
                                           unsigned long bIsEnemy, float fMissingHealth) {
	if (Instigator == nullptr) return;
	if (Target     == nullptr) return;

	// UC dispatches both damage AND heal through here:
	//   damage path: fDamage = +fHealthChange (post-clamp HP drop)
	//   heal path:   fDamage = -fProratedAmount (negated by caller)
	// Self-effects don't credit (a player healing themselves with their own
	// adrenaline gun, or eating splash from their own grenade). Two shapes:
	//   1. Direct: Instigator == Target.
	//   2. Indirect: Instigator is an owned deployable/pet (turret, medical
	//      station, sensor self-buff, etc.) acting on its deployer — i.e.
	//      Instigator->r_Owner == Target. Without this gate, parking on your
	//      own medical station would farm STYPE_HEALING + morale points.
	if (Target == reinterpret_cast<AActor*>(Instigator)) return;
	if (Instigator->r_Owner != nullptr &&
	    Target == reinterpret_cast<AActor*>(Instigator->r_Owner)) return;

	const bool isHeal = (fDamage < 0.0f);
	const float magnitude = isHeal ? -fDamage : fDamage;
	if (magnitude <= 0.0f) return;

	// ===== Target classification (used by credit decisions below) =====
	// Class-name substring per `feedback_bIsPlayer_unreliable` / `reference_sdk_staticclass_misalignment`.
	// Forcefield first so the "TgDeploy" generic check doesn't shadow it.
	// Single cache lookup, three string::find calls on the cached name.
	const std::string& targetClassName = ObjectClassCache::GetClassName(Target);
	const bool     targetIsForceField = (targetClassName.find("TgDeploy_ForceField") != std::string::npos);
	const bool     targetIsDeployable = (targetClassName.find("TgDeploy")            != std::string::npos);
	const bool     targetIsPawn       = (targetClassName.find("TgPawn")              != std::string::npos);
	ATgPawn*       targetPawn         = targetIsPawn       ? (ATgPawn*)Target       : nullptr;
	ATgDeployable* targetDeployable   = targetIsDeployable ? (ATgDeployable*)Target : nullptr;

	// Ownership: heal/morale credit exclusion for self-stewardship.
	//   Pet pawn: r_Owner points to the deploying pawn.
	//   Deployable: engine field Instigator points to the deploying pawn.
	bool targetIsOwnedByInstigator = false;
	if (targetIsPawn && targetPawn->r_Owner == Instigator) {
		targetIsOwnedByInstigator = true;
	} else if (targetIsDeployable &&
	           (AActor*)targetDeployable->Instigator == (AActor*)Instigator) {
		targetIsOwnedByInstigator = true;
	}

	// "Has client connection" predicate — reused below.
	auto IsRealPlayer = [](AController* ctrl) -> bool {
		if (!ctrl) return false;
		return ObjectClassCache::ClassNameContains(ctrl->Class, "PlayerController");
	};

	// Resolve pet → owner for scoreboard credit (kills/damage/heal credit a
	// player, never a pet bot's own PRI). TrackKill does its own resolution
	// internally; we only need this for the non-TrackKill paths.
	ATgPawn* damageCreditPawn = Instigator;
	if (Instigator->r_Owner != nullptr) damageCreditPawn = Instigator->r_Owner;

	// ===== Rolling damager history for assist credit =====
	// UC's UpdateDamagers native is stripped (no UC caller). Without
	// rotation, m_SecondToLastDamager stays null forever and TrackKill can
	// only credit healing assists — never damage assists. Shift the
	// history on every damage event so previous damagers are still in the
	// list when the killing blow lands.
	//
	// Resolve to the player (pet → owner, detonator → controller) so the
	// assist credits a real human, not the pet bot.
	if (!isHeal && targetIsPawn && magnitude > 0.0f) {
		ATgPawn* damagerForHistory = Instigator;
		if (ObjectClassCache::ClassNameContains(damagerForHistory->Class, "TgPawn_Detonator") &&
		    damagerForHistory->r_ControlPawn != nullptr) {
			damagerForHistory = damagerForHistory->r_ControlPawn;
		}
		if (damagerForHistory->r_Owner != nullptr) {
			damagerForHistory = damagerForHistory->r_Owner;
		}
		// Self-damage and consecutive same-damager hits don't shift —
		// otherwise a single sustained hose would clobber m_SecondToLastDamager
		// with its own pawn over and over.
		if (damagerForHistory != targetPawn &&
		    damagerForHistory != targetPawn->m_LastDamager) {
			targetPawn->m_SecondToLastDamager = targetPawn->m_LastDamager;
			targetPawn->m_LastDamager        = damagerForHistory;
		}
	}

	// ===== STYPE_DAMAGETAKEN — credit before the team-damage early-return =====
	// User spec: "absorbed = total damage taken post-mitigation / (1+death
	// count)". Total includes team-damage; client derives the absorbed
	// column from STYPE_DAMAGETAKEN. Deployables don't carry their own
	// scoreboard so we only credit pawn victims.
	if (!isHeal && targetIsPawn) {
		TgPawn__TrackDamageTaken::Call(targetPawn, nullptr, (int)magnitude);
	}

	// ===== STYPE_DEFENSE (forcefield branch) — credit before the team-damage early-return =====
	// User spec: defense = damage absorbed by owned forcefield-type
	// deployables. Forcefield takes the hit so its owner gets credit for
	// the damage that didn't reach nearby teammates. Personal-shield branch
	// lives in TgEffectManager::SubmitMitigationDamage (per-shield-tick
	// callback). Enemy damage only — we don't credit team damage to your
	// own forcefield.
	if (!isHeal && bIsEnemy && targetIsForceField && targetDeployable->Instigator) {
		ATgPawn* deployer = (ATgPawn*)targetDeployable->Instigator;
		TgPawn__TrackDefense::Call(deployer, nullptr, 0, (int)magnitude);
	}

	// ===== Kill attribution =====
	//
	// Drives "Killed by X" release-dialog label + "You killed X !" kill-feed
	// toast on the killer's HUD. UC code in TgPawn.Died already sends the
	// chat-line ClientMessage; the on-screen widgets need their own pipes:
	//
	//   - m_DeathZoomInfo: server-side struct on the victim TgPawn, shipped
	//     to the client by the Dying state's `Begin:` latent block calling
	//     DoDeathZoom → ClientDoDeathZoom RPC. UC SaveDeathInfoForZoomCam
	//     (TgPawn.uc:9345) is the populator, originally fired by the
	//     stripped damage-attribution native.
	//   - ClientAddKilled: reliable client RPC on the killer's PC. UC
	//     TgPlayerController.ClientAddKilled forwards to TgHUD.AddNewKilledTarget
	//     → m_PrimaryHUD.m_KillDisplay (the toast widget).
	//
	// TrackStats fires inside the same callstack as the damage event, after
	// Target.TakeDamage has returned (which includes the Died → GotoState
	// state transition). Dying state's `Begin:` latent code runs from the
	// next actor Tick, so we have time to populate m_DeathZoomInfo here
	// before it ships to the client.
	//
	// Only consider damage events targeting a pawn that just dropped to
	// Health <= 0. Heals and damage to deployables don't trigger a kill
	// pipeline; non-fatal damage doesn't fire a death zoom.
	if (!isHeal) {
		if (targetPawn != nullptr && targetPawn->Health <= 0) {
			ATgPawn* Victim = targetPawn;
			// Detonator unwrap: a TgPawn_Detonator (timed bomb, etc.) carries
			// the firing pawn in r_ControlPawn. Mirrors the UC body of
			// SaveDeathInfoForZoomCam.
			ATgPawn* DamagerPawn = Instigator;
			if (ObjectClassCache::ClassNameContains(DamagerPawn->Class, "TgPawn_Detonator") &&
			    DamagerPawn->r_ControlPawn != nullptr) {
				DamagerPawn = DamagerPawn->r_ControlPawn;
			}

			// Pet kill: the actual hit came from a pet bot owned by a player.
			// Both the kill-feed toast and the death-zoom UI need to show
			// "Your <pet> killed X !" — UC differentiates via bPetKill +
			// passing the pet as Killer (with r_Owner resolved internally).
			ATgPawn* KillerOwner = nullptr;
			bool     isPetKill   = false;
			if (DamagerPawn->r_Owner != nullptr) {
				KillerOwner = DamagerPawn->r_Owner;
				isPetKill   = true;
			}

			// m_LastDamager / m_SecondToLastDamager were already updated by
			// the damager-history rotation above (runs on every damage
			// event), so they hold the killing damager + the previous one
			// at this point. No additional write here.

			// The killer's weapon id drives the "Killed by [X] with [Y]"
			// m_KilledByWeapon label on the release dialog. Source it from
			// the fire-mode reference embedded in Impact (FImpactInfo @ 0x54),
			// NOT from iTargetDeviceModeId — that one is the *target's*
			// active fire mode at the time of death (passed to TrackStats
			// for morale-credit gating, not kill attribution).
			const int killerDeviceId = ResolveDeviceIdFromFireMode(
				(UObject*)Impact.dw[0x54 / 4]);

			// Populate Victim->m_DeathZoomInfo. UC handler no-ops for
			// non-player victims (bots), so call unconditionally.
			FireSaveDeathInfoForZoomCam(Victim, DamagerPawn, KillerOwner,
			                            killerDeviceId, isPetKill);

			if (Logger::IsChannelEnabled("kills")) {
				ATgPawn* logCreditPawn = isPetKill ? KillerOwner : DamagerPawn;
				// GetName/GetFullName share a static buffer — copy each name
				// before format-printing or all %s args alias the last call.
				const char* vRaw = Victim->GetName();
				std::string vName = vRaw ? vRaw : "<null>";
				const char* iRaw = Instigator->GetName();
				std::string iName = iRaw ? iRaw : "<null>";
				const char* dRaw = DamagerPawn->GetName();
				std::string dName = dRaw ? dRaw : "<null>";
				Logger::Log("kills",
					"[TrackStats] kill: victim=%s instigator=%s damager=%s isPet=%d killerDevId=%d "
					"victim.Controller.bIsPlayer=%d creditPawn.Controller.bIsPlayer=%d\n",
					vName.c_str(),
					iName.c_str(),
					dName.c_str(),
					(int)isPetKill,
					killerDeviceId,
					Victim->Controller ? (int)Victim->Controller->bIsPlayer : -1,
					logCreditPawn && logCreditPawn->Controller
						? (int)logCreditPawn->Controller->bIsPlayer : -1);
			}

			// Fire kill-feed notifications on the killer's HUD. Only player
			// killers have a PC to receive these; AI killing a player → no
			// notification (bot has no client connection).
			//
			// Two parallel paths drive two different HUD elements:
			//   1. `ClientAddKilled` reliable-client RPC → kill-feed widget
			//      `m_PrimaryHUD.m_KillDisplay` (corner kill-feed list).
			//   2. `SendKillAlert` → CHAT_MESSAGE marshal carrying
			//      DISPLAY_AS_ALERT — same dispatcher (FUN_10901f00 @
			//      CGameClient+0xf0) the local AddKillAlert uses via
			//      vtable[0x734], just reached over the wire. Drives the
			//      center-screen toast + sound effect.
			ATgPawn* CreditPawn = isPetKill ? KillerOwner : DamagerPawn;

			// STYPE_KILLS / STYPE_KILLS_BOT / STYPE_ASSISTS — only enemy
			// kills count toward the scoreboard. TrackKill on the victim
			// handles pet→owner resolution, picks the right kill counter
			// (player vs bot) by victim Controller class, and credits any
			// assisters from m_LastDamager / m_SecondToLastDamager /
			// killer.m_LastHealer with dedup. Calling it here is the only
			// path: TrackKill's native has no UC caller in our build.
			if (bIsEnemy) {
				TgPawn__TrackKill::Call(Victim, nullptr, DamagerPawn);
			}

			if (CreditPawn != nullptr && CreditPawn != Victim &&
			    IsRealPlayer(CreditPawn->Controller) &&
			    Victim->PlayerReplicationInfo != nullptr) {
				ATgPlayerController* KillerPC = (ATgPlayerController*)CreditPawn->Controller;
				wchar_t* killedSrc = Victim->PlayerReplicationInfo->PlayerName.Data;
				wchar_t* killerSrc = (isPetKill && DamagerPawn->PlayerReplicationInfo)
					? DamagerPawn->PlayerReplicationInfo->PlayerName.Data
					: (wchar_t*)L"";

				// Path 1: ClientAddKilled (kill-feed widget).
				FireClientAddKilled(KillerPC, killedSrc, killerSrc, /*killerWasPlayer=*/!isPetKill);

				// Path 2: Center-screen kill toast via CHAT_MESSAGE piggyback.
				// The client's AddKillAlert (vtable[0x734] @ 0x10964810) builds
				// a CMarshal with DISPLAY_AS_ALERT/ALERT_PRIORITY/etc and routes
				// it through CGameClient+0xf0's chat dispatcher. Our network
				// path (CHAT_MESSAGE opcode 0x73 → FUN_109027e0) lands at the
				// same dispatcher when MSG_ID is present in the marshal. So a
				// server-sent chat marshal with the alert fields lights up the
				// same toast. See SendKillAlert.hpp for the full rationale.
				//
				// For pet kills, pass the pet (DamagerPawn) name so the
				// template "Your <pet> defeated <victim>!" can substitute it
				// into @@player_name@@. Direct kills pass nullptr and the
				// "You defeated <victim>!" template is used instead.
				UNetConnection* KillerConn = (UNetConnection*)KillerPC->Player;
				if (KillerConn && (uintptr_t)KillerConn >= 0x10000) {
					wchar_t* petSrc = (isPetKill && DamagerPawn->PlayerReplicationInfo)
						? DamagerPawn->PlayerReplicationInfo->PlayerName.Data
						: nullptr;
					SendKillAlert::Call(KillerConn, killedSrc, petSrc);
				}

				// Assist alerts — per user spec, ONLY fire on player victims
				// (assist tracking doesn't apply to bot kills). Two
				// independent categories that can stack on the same assister:
				//   - Damage: m_LastDamager / m_SecondToLastDamager on victim
				//     (other players who hit the victim before the killing blow)
				//   - Healing: m_LastHealer on the killer (player who topped
				//     the killer off before they secured the kill)
				// A player who BOTH damaged the victim AND healed the killer
				// gets TWO separate alerts — no dedup between categories,
				// only within damage.
				if (IsRealPlayer(Victim->Controller)) {
					wchar_t* killerNameForAssist = CreditPawn->PlayerReplicationInfo
						? CreditPawn->PlayerReplicationInfo->PlayerName.Data
						: (wchar_t*)L"";

					ATgPawn* damageAssists[2] = {
						Victim->m_LastDamager,
						Victim->m_SecondToLastDamager,
					};
					for (int i = 0; i < 2; i++) {
						ATgPawn* assister = damageAssists[i];
						if (!assister || assister == Victim || assister == CreditPawn) continue;
						// Dedup within damage category only.
						bool dup = false;
						for (int j = 0; j < i; j++) {
							if (damageAssists[j] == assister) { dup = true; break; }
						}
						if (dup) continue;
						if (!IsRealPlayer(assister->Controller)) continue;
						ATgPlayerController* AssistPC = (ATgPlayerController*)assister->Controller;
						FireAddAssistAlert(AssistPC, killedSrc, killerNameForAssist);
					}

					// Healing assist — independent of damage. m_LastHealer is
					// populated by our heal path below (TrackStats fires for
					// both damage and heal).
					ATgPawn* healer = CreditPawn->m_LastHealer;
					if (healer && healer != Victim && healer != CreditPawn &&
					    IsRealPlayer(healer->Controller)) {
						ATgPlayerController* HealerPC = (ATgPlayerController*)healer->Controller;
						FireAddAssistAlert(HealerPC, killedSrc, killerNameForAssist);
					}
				}
			}
		}

		// Deployable-destroyed credit. Mirrors the pawn-kill path: enemy
		// deployable down → owner's STYPE_KILLS_BOT++ (per user spec,
		// destroying enemy deployables counts the same as a bot kill). No
		// assists.
		//
		// Edge case (accepted): a DOT can land another tick on the
		// already-destroyed deployable before the engine reaps it. We do
		// not track a "credited" flag — would-be double-credit is rare.
		if (bIsEnemy && targetIsDeployable && targetDeployable->r_nHealth <= 0 &&
		    magnitude > 0.0f) {
			TgPawn__TrackKilledBot::Call(damageCreditPawn, nullptr, 0);

			// Beacon destroy carries an additional bonus on top of the
			// bot-kill credit: +3000 STYPE_OBJS for the destroyer and a
			// center-screen "@@player_name@@ destroyed the enemy beacon!"
			// alert broadcast to every player on the destroyer's task
			// force (including the destroyer themselves). Numbers and
			// 5s alert duration per user spec.
			if (targetClassName.find("TgDeploy_Beacon") != std::string::npos &&
			    damageCreditPawn->PlayerReplicationInfo != nullptr) {
				ATgRepInfo_Player* attackerPRI =
					(ATgRepInfo_Player*)damageCreditPawn->PlayerReplicationInfo;
				attackerPRI->r_Scores[9] += 3000;  // STYPE_OBJS
				attackerPRI->bNetDirty = 1;
				attackerPRI->bForceNetUpdate = 1;

				ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
				if (Game != nullptr) {
					Game->eventSendCombatMessage(0x59F4, nullptr, damageCreditPawn, 3000, 0);
				// SendCombatMessage::Call(damageCreditPawn, /*Source=*/nullptr,
				//                         /*Target=*/damageCreditPawn, 3000,
				//                         SendCombatMessage::Type::OBJ_POINTS);
				}

				// Attacker-team alert. Defender-team "your beacon was
				// destroyed!" lives separately on the client side
				// (TgTeamBeaconManager::BeaconDestroyedClient fires when
				// r_BeaconDestroyed replicates) and is not in scope here.
				// MSG_ID 0x653D template: "@@player_name@@ destroyed the
				// enemy beacon!" — fills PLAYER_NAME slot only.
				if (attackerPRI->r_TaskForce != nullptr) {
					SendAlert::BroadcastToTaskforce(
						attackerPRI->r_TaskForce,
						/*msgId*/                0x653D,
						/*priority=APT_High*/    2,
						/*type=ATT_Beneficial*/  1,
						/*duration*/             5.0f,
						/*name*/                 nullptr,
						/*playerName=destroyer*/ attackerPRI->PlayerName.Data);
				}

				if (Logger::IsChannelEnabled("stats")) {
					Logger::Log("stats",
						"[ObjPoints] beacon destroyed by %s (+3000 obj, team alert)\n",
						damageCreditPawn->GetName());
				}
			}
		}
	} else {
		// Heal path — also runs through TrackStats. Populate m_LastHealer on
		// the heal target so future kill events can credit a healing assist.
		// UpdateHealer native is stripped + has no UC callers, so this is
		// the only thing that keeps the field fresh.
		if (targetPawn != nullptr) {
			targetPawn->m_LastHealer = Instigator;
		}

		// STYPE_HEALING credit — credit Instigator (or owner if pet) for
		// heal/repair done to others. Skip self-heal (already filtered at
		// the top) and own pets/deployables (per user spec: repairing your
		// own gear doesn't count). TrackHealing for player heal targets,
		// TrackBotHealing for everything else.
		if (!targetIsOwnedByInstigator) {
			if (targetIsPawn) {
				const bool victimIsBot = !IsRealPlayer(targetPawn->Controller);
				if (victimIsBot) {
					TgPawn__TrackBotHealing::Call(damageCreditPawn, nullptr,
						0, magnitude, fMissingHealth, 0);
				} else {
					TgPawn__TrackHealing::Call(damageCreditPawn, nullptr,
						0, magnitude, fMissingHealth, 0);
				}
			} else if (targetIsDeployable) {
				TgPawn__TrackBotHealing::Call(damageCreditPawn, nullptr,
					0, magnitude, fMissingHealth, 0);
			}
		}
	}
	// ===== end kill attribution =====

	// Damage on a friendly target = team damage. UC also calls TrackTeamDamage
	// in that case (a separate stub) which originally awarded zero morale.
	// Don't credit.
	if (!isHeal && !bIsEnemy) {
		if (Logger::IsChannelEnabled("morale")) {
			Logger::Log("morale",
				"[TrackStats] team-damage no credit: instigator=0x%p target=0x%p fDamage=%.2f\n",
				Instigator, Target, fDamage);
		}
		return;
	}

	// STYPE_DAMAGE credit — only enemy damage counts (team damage already
	// returned above). Covers damage to players, AI bots, and deployables.
	// TrackDamagedPlayer for human-controlled victims, TrackDamagedBot for
	// AI bots and deployables (TrackDamagedBot's body ignores its
	// TargetPawn arg, so passing nullptr for the deployable case is safe).
	if (!isHeal) {
		if (targetIsPawn) {
			const bool victimIsBot = !IsRealPlayer(targetPawn->Controller);
			if (victimIsBot) {
				TgPawn__TrackDamagedBot::Call(damageCreditPawn, nullptr,
					targetPawn, 0, (int)magnitude);
			} else {
				TgPawn__TrackDamagedPlayer::Call(damageCreditPawn, nullptr,
					targetPawn, 0, (int)magnitude);
			}
		} else if (targetIsDeployable) {
			TgPawn__TrackDamagedBot::Call(damageCreditPawn, nullptr,
				nullptr, 0, (int)magnitude);
		}
	}

	// ===== STYPE_OBJS damage-near-owned-point =====
	// Per user spec: 1 dmg = 1 obj pt when EITHER the attacker OR the victim
	// is inside a proximity-objective's collision proxy that is currently
	// owned by the attacker's task force, AND the victim is a human player.
	// Bot/deployable/shield/forcefield victims don't count. Attacker may be
	// anything (turret, pet, player) — credit goes to damageCreditPawn,
	// which is pet→owner resolved.
	//
	// The single TgCollisionProxy per objective is reused as both "on point"
	// and "near point" — there's no separate wider fight-zone array in UC
	// (the engineered "near point" was native-side and stripped). The proxy
	// radius set per-level is generous enough that this is acceptable in
	// practice; if it turns out too restrictive we can layer a distance
	// check on Obj->Location.
	//
	// Break on first match — don't double-credit if both attacker and victim
	// happen to stand inside two different owned proxies simultaneously.
	if (!isHeal && targetIsPawn && IsRealPlayer(targetPawn->Controller) &&
	    magnitude > 0.0f && damageCreditPawn->PlayerReplicationInfo != nullptr) {
		ATgRepInfo_Player* attackerPRI =
			(ATgRepInfo_Player*)damageCreditPawn->PlayerReplicationInfo;
		if (attackerPRI->r_TaskForce != nullptr) {
			const int attackerTeam = (int)attackerPRI->r_TaskForce->r_nTaskForce;

			// Ticket mode (TgGame_Ticket): map-default ownership is bogus —
			// points start "owned" by defender but are functionally neutral
			// until first real capture. Other modes (defense / control etc.):
			// the defender genuinely owns from t=0 and deserves credit for
			// fighting on their initially-owned point. So the
			// `r_bHasBeenCapturedOnce` gate only applies in Ticket mode.
			ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
			const bool ticketMode = Game &&
				ObjectClassCache::ClassNameContains(Game->Class, "TgGame_Ticket");

			for (ATgMissionObjective* Obj : ActorCache::MissionObjectives) {
				if (!Obj || !Obj->r_bIsActive) continue;
				// Ticket-only: skip pre-capture neutral state. In other modes,
				// trust the map-seeded ownership immediately.
				if (ticketMode && !Obj->r_bHasBeenCapturedOnce) continue;
				// Owned by attacker's team only (r_nOwnerTaskForce is the
				// current owner; flips when captured).
				if (Obj->r_nOwnerTaskForce != attackerTeam) continue;
				// Must have a collision proxy — covers TgMissionObjective_Proximity
				// and its subclasses (e.g. TgMissionObjective_Escort). Check by
				// class name; the proxy field offset is the same across the
				// Proximity hierarchy but undefined on plain TgMissionObjective.
				const std::string& ocname = ObjectClassCache::GetClassName(Obj->Class);
				if (ocname.find("TgMissionObjective_Proximity") == std::string::npos &&
				    ocname.find("TgMissionObjective_Escort")    == std::string::npos) continue;
				ATgMissionObjective_Proximity* PObj =
					(ATgMissionObjective_Proximity*)Obj;
				if (!PObj->s_CollisionProxy) continue;

				bool inProxy = false;
				for (int i = 0; i < PObj->s_CollisionProxy->m_NearByPlayers.Count; i++) {
					ATgPawn* P = PObj->s_CollisionProxy->m_NearByPlayers.Data[i];
					if (P == damageCreditPawn || P == Instigator || P == targetPawn) {
						inProxy = true;
						break;
					}
				}
				if (!inProxy) continue;

				attackerPRI->r_Scores[9] += (int)magnitude;  // STYPE_OBJS
				attackerPRI->bNetDirty = 1;

				ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
				if (Game != nullptr) {
					Game->eventSendCombatMessage(0x59F4, nullptr, damageCreditPawn, (int)magnitude, 0);
				// SendCombatMessage::Call(damageCreditPawn, /*Source=*/nullptr,
				//                         /*Target=*/damageCreditPawn, (int)magnitude,
				//                         SendCombatMessage::Type::OBJ_POINTS);
				}

				if (Logger::IsChannelEnabled("stats")) {
					const char* cRaw = damageCreditPawn->GetName();
					std::string cName = cRaw ? cRaw : "<null>";
					const char* tRaw = targetPawn->GetName();
					std::string tName = tRaw ? tRaw : "<null>";
					Logger::Log("stats",
						"[ObjPoints] dmg-near-pt: %s +%d obj (objId=%d, victim=%s)\n",
						cName.c_str(), (int)magnitude,
						Obj->nObjectiveId, tName.c_str());
				}
				break;
			}
		}
	}

	// Skip morale credit when healing your own pets/deployables. A tech parked
	// on their own Heal Beacon would otherwise rack up infinite morale via
	// repair ticks against their own gear. Self-heal (Target == Instigator) is
	// already filtered at the top of this function.
	if (isHeal && targetIsOwnedByInstigator) return;

	// All other morale-system concerns (anti-feedback for morale-device
	// sources, recipient-side gates, Output Mod scaling, accumulator math,
	// replicated mirror write) live in MoraleCredit::Award. The damage
	// source's morale-device classification is DB-driven via the source's
	// `iTargetDeviceModeId` — the parent device's `slot_used_value_id == 476`
	// flag ("Morale Device" category) is what the binary's intact
	// AddMoralePoints native at 0x109d2f00 checks. Catches both direct
	// morale-fire damage AND bomb-spawned-by-morale (Shatter Bomb etc.)
	// because retail registers both under the same category.
	MoraleCredit::Award(damageCreditPawn, magnitude, isHeal,
	                    fMissingHealth, iTargetDeviceModeId);
}
