#pragma once

// CTR_Recursive_P ships unfinished: the attacker spawn-door matinee
// (SeqAct_Interp "Attackers", fired from TgSeqEvent_MissionTimer.MissionStarted)
// has no InterpActor bound to its kismet SeqVar_Object, so it never opens. We
// can't drive it from the server: the client runs its own copy of the map's
// kismet, where that binding ships null too, so a server-side bind opens the
// server collision but the client never animates the door.
//
// Instead we just clear the doorway — hide + disable collision on the door's
// two leaves. bHidden / bCollideActors / bBlockActors are all replicated
// (CPF_Net), so the client sees it open too.
//
// Two phases so nothing scans GObjObjects mid-match:
//   CacheSpawnDoors()       — at map init: resolve + cache the leaf actors.
//   OpenAttackerSpawnDoor() — at mission start (MissionStarted): hide the cached
//                             leaves, matching when the defender door opens.
// Leaf coordinates live in CtrRecursiveDoors.cpp. Channel: "recursive".
namespace CtrRecursiveDoors {
	void CacheSpawnDoors();
	void OpenAttackerSpawnDoor();
}
