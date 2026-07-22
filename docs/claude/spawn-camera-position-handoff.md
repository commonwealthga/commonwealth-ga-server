# Handoff: camera/ViewTarget position on spawn (normal players)

Investigation notes assembled from a spectator-mode debugging session. Written
for whoever picks up the "camera position on spawn for normal players" issue.
Everything below is either (a) a direct quote of pre-existing code comments,
or (b) real log lines from a live 2-player match. Anything not directly
supported by one of those is marked as an open question, not a conclusion.

## Confirmed: two-phase `FindPlayerStart` on every join

Every connecting player (spectator or not) triggers `TgFindPlayerStart`
**twice** before their pawn exists:

1. **Phase 1** — runs before `PostLogin` seeds `r_TaskForce`. At this point
   `playerTaskForce=0` (unseeded default). This picks a `TgTeamPlayerStart`
   and the engine issues `ClientSetViewTarget` pointing the camera at some
   actor — while `Pawn` is still null for everyone at this point, real player
   or not.
2. **Phase 2** — runs after our `PostLogin` hook seeds `r_TaskForce` (for
   non-spectators). This second pick is what `SpawnPlayerCharacter` actually
   uses to place the real pawn.

This is documented in the existing code comment on the `TgGamePostLogin` case
in [`UObject__ProcessEvent.cpp`](../../src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.cpp):

> Seed PRI.r_TaskForce BEFORE super.PostLogin (called from the UC body of
> TgGame.PostLogin) triggers RestartPlayer → FindPlayerStart. Without this
> seed the picker reads playerTaskForce=0, fails every team filter, and lands
> the player in the wrong spawn room. Do not pre-write PRI.Team here: native
> SetTeam compares that field later, and a pre-write can skip AddPRI or crash
> duplicate VR joins. SpawnPlayerCharacter writes the same field later
> (TgGame__SpawnPlayerCharacter.cpp:295) but by then the spawn point has
> already been chosen, so the wrong-room result persists until the player
> dies and respawns.

**Open question:** the map used in the log below (`1P_SDColony06_P`) only had
one `TgTeamPlayerStart` candidate registered, so phase 1's `tf=0` mismatch
never actually got a chance to pick the *wrong* room — it's the only room.
Whether the "wrong room" failure mode in the comment above still reproduces
today, on a map with multiple team-specific starts, joining a **live**
(already-running) instance rather than a fresh map start, is untested in this
session.

## Real log evidence (real player joining a live instance)

From `out/logs/<instance_id>/spawn.txt`, one real player joining an
already-running match (channel `spawn`, produced by the `SpectateVisualState`
ProcessEvent dispatch tag — see below):

```
TgFindPlayerStart: 1 candidate(s), playerTaskForce=0, s_nCurrentPriority=1
  TgTeamPlayerStart mapObjId=13996 enabled=1 tf=0 prio=-1 rating=190.0 primary=1 eligible=1 loc=(-9680,5312,331)
TgFindPlayerStart: chose TgTeamPlayerStart mapObjId=13996 tf=0 prio=-1 rating=190.0
SpectateVisualState: fn=Function Engine.PlayerController.ClientSetViewTarget obj=TgPlayerController ... pawn=00000000 viewTarget=1f435a00 paramActor=1f435a00 onlySpec=0 isSpec=0 outOfLives=0 ...
TgGame.PostLogin intercept: conn=490950656 task_force=1 prevTF=00000000 newTF=159ae100
TgFindPlayerStart: 1 candidate(s), playerTaskForce=1, s_nCurrentPriority=1
  TgTeamPlayerStart mapObjId=13996 enabled=1 tf=0 prio=-1 rating=190.0 primary=1 eligible=1 loc=(-9680,5312,331)
TgFindPlayerStart: chose TgTeamPlayerStart mapObjId=13996 tf=0 prio=-1 rating=190.0
SpawnPlayerCharacter: removed duplicate taskforce roster entry attackers=1 defenders=0 pri=205c6900
SpawnPlayerCharacter: volume state refreshed pawn=23248000 disableAllDevices=0 enableEquip=1 enableSkills=0 currentOmega=00000000
SpawnPlayerCharacter: volume scan pawn=23248000 loc=(-9680.0,5312.0,334.2) modify=0/2 omega=1/3 recalcModify=0 recalcOmega=1 repairedModify=0 repairedOmega=0 disableAllDevices=0 enableSkills=0
SpectateVisualState: fn=Function Engine.PlayerController.ClientSetViewTarget ... pawn=23248000 viewTarget=23248000 paramActor=23248000 onlySpec=0 isSpec=0 outOfLives=0 ...
SpectateVisualState: fn=Function Engine.PlayerController.ClientSetViewTarget ... pawn=23248000 viewTarget=1f435a00 paramActor=1f435a00 onlySpec=0 isSpec=0 outOfLives=0 ...
SpectateVisualState: fn=Function Engine.PlayerController.ClientSetViewTarget ... pawn=23248000 viewTarget=23248000 paramActor=23248000 onlySpec=0 isSpec=0 outOfLives=0 ...
SpectateVisualState: fn=Function TgGame.TgPlayerController.ClientSetCameraFade ... pawn=23248000 viewTarget=23248000 paramActor=00000000 fade=0 ...
SpectateVisualState: fn=Function TgGame.TgPlayerController.ClientSetCinematicMode ... pawn=23248000 viewTarget=23248000 paramActor=00000000 cinematic=0 affectsHud=1
HandlePlayerConnected: cleared client fade/cinematic hud fadeFn=160baf80 cinematicFn=15f3a680 pawn=23248000 viewTarget=23248000
```

**Notable, unexplained pattern:** after the real pawn (`23248000`) spawns and
`ViewTarget` is set to it, `ViewTarget` **bounces back** to the same
placeholder actor from phase 1 (`1f435a00`) before returning to the pawn a
second time — three `ClientSetViewTarget` calls in quick succession:
pawn → placeholder → pawn. This happens for every real player in the capture,
not just once. Whether this is:
- harmless (e.g. a fade/cinematic transition intentionally routing through a
  camera actor), or
- the actual visible symptom being chased (a camera jump/flicker on spawn),

is an **open question** — not established in this session. This bounce is the
most concrete lead for whoever continues this.

## Diagnostic tooling already in place

The `DispatchTag::SpectateVisualState` case in
[`UObject__ProcessEvent.cpp`](../../src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.cpp)
is what produced the `SpectateVisualState:` log lines above. It's a pure
diagnostic logger (no gameplay effect) that fires whenever any of these four
functions run on a `TgPlayerController`, logging pawn/ViewTarget/paramActor/
`bOnlySpectator`/`bIsSpectator`/`bOutOfLives`/fade/cinematic state on the
`"spawn"` log channel:
- `Engine.PlayerController.ServerSetViewTarget`
- `Engine.PlayerController.ClientSetViewTarget`
- `TgGame.TgPlayerController.ClientSetCameraFade`
- `TgGame.TgPlayerController.ClientSetCinematicMode`

This is the tool to keep using — it already gave clean before/after visibility
into the bounce above without adding any new hooks.

**Not yet located in this session:** the actual hook/native emitting the
`TgFindPlayerStart:` log lines themselves (candidate list, chosen start,
`tf`/`prio`/`rating`/`primary`/`eligible` fields). Grep for
`"TgFindPlayerStart:"` to find it — useful if the two-phase call pattern
itself (not just its ViewTarget side-effects) needs to be traced further.

## Related historical bugs already fixed (context, not open items)

These are already resolved in the codebase, but explain *why* the
PostLogin/HandlePlayerConnected code is shaped the way it is — relevant
background if the new investigation touches the same functions.

1. **`ViewTarget` stuck on the PC itself.** If `bOnlySpectator` stays true too
   long after a normal (non-spectator) login, the client enters
   `SpectatingMatch` state, whose `BeginState` fires
   `ServerSetViewTarget(Pawn=null)` — which the engine's `SetViewTarget`
   converts from null to *self*, permanently pinning `ViewTarget = PC` instead
   of the pawn. Comment in `MarshalChannel__NotifyControlMessage.cpp`:

   > Removing both eliminates the UnPossess+re-Possess transient ViewTarget
   > flip caused by the second Possess and the SpectatingMatch-state race
   > (client's replicated bOnlySpectator landing as TRUE when
   > ClientSetReadyState fires, causing client to GotoState('SpectatingMatch')
   > → BeginState fires ServerSetViewTarget(Pawn=null on client) → server
   > SetViewTarget(none) → null→self conversion in public SetViewTarget →
   > ViewTarget = PC permanently).

   Fixed by clearing `bOnlySpectator`/`bIsSpectator`/`bOutOfLives` inside the
   `PostLogin` hook (for non-spectator joins), before `RestartPlayer` runs.

2. **Defensive fallback for the same class of bug** —
   [`TgPlayerController__GetViewTarget.cpp`](../../src/GameServer/TgGame/TgPlayerController/GetViewTarget/TgPlayerController__GetViewTarget.cpp):
   if the engine's `GetViewTarget` ever resolves to the PC itself while a
   healthy possessed pawn exists, this hook forces it back to the pawn.
   Belt-and-suspenders on top of fix #1, not the primary fix.

3. **`HandlePlayerConnected` explicitly resets and re-clears view state** after
   spawn, logging on the `"aim-trace"` channel:
   ```cpp
   Logger::Log("aim-trace", "RESETFORCEVIEWTARGET pawn = 0x%p\n", newcontroller->Pawn);
   newcontroller->ResetForceViewTarget();
   Logger::Log("aim-trace", "NEW VIEWTARGET = 0x%p\n", newcontroller->ViewTarget);
   ```
   followed by clearing camera fade/cinematic mode "after real TgHUD_Game
   exists" (its own comment notes the client's `TgPlayerController` overrides
   update HUD fade and `m_bDrawPawnHUD` at this point). `"aim-trace"` is a
   second log channel worth enabling alongside `"spawn"` if reproducing this.

## Suggested next steps

1. Reproduce with both `"spawn"` and `"aim-trace"` channels enabled, on a map
   with more than one `TgTeamPlayerStart` candidate, to actually exercise the
   phase-1 `tf=0` mismatch the existing code comment warns about.
2. Grep for `"TgFindPlayerStart:"` to find and read the hook/native producing
   that log line — not located in this session.
3. Determine whether the pawn→placeholder→pawn `ViewTarget` bounce seen above
   is visible to the player (camera jump) or fully masked by loading/fade —
   correlate against the fade/cinematic `SpectateVisualState` lines
   immediately following it in the same capture.
