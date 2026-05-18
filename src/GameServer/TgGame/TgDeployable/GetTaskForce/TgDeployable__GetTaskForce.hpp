#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native TgDeployable::GetTaskForce is a stub on this binary — `return 0;`
// (see TgDeployable__GetTaskForce @ 0x10a195d0 in Ghidra). Every UC site that
// reads the deployable's task force via GetTaskForce() (notably
// TgDeploy_Beacon's state Deploy.BeginState/EndState, DestroyIt, Destroyed,
// OnSetTaskforce, PickUpDeployable) sees null and silently no-ops the
// downstream RegisterBeacon/UnRegisterBeacon call — leaving the UC beacon
// state machine permanently disarmed.
//
// We reimplement using the same DRI walk that the intact
// TgDeployable::GetTaskForceNumber native uses (@ 0x10a195e0):
//   1. r_DRI->r_InstigatorInfo->r_TaskForce  (player-deployed: set in
//      SpawnDeployableActor via the DRI ownership wiring)
//   2. r_DRI->r_TaskforceInfo when r_bOwnedByTaskforce is set
//      (factory-spawned: set by InitReplicationInfo using s_DeployFactory)
class TgDeployable__GetTaskForce : public HookBase<
	ATgRepInfo_TaskForce*(__fastcall*)(ATgDeployable*, void*),
	0x10a195d0,
	TgDeployable__GetTaskForce> {
public:
	static ATgRepInfo_TaskForce* __fastcall Call(ATgDeployable* Deployable, void* edx);
	static inline ATgRepInfo_TaskForce* __fastcall CallOriginal(ATgDeployable* Deployable, void* edx) {
		return m_original(Deployable, edx);
	}
};
