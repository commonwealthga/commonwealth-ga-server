#pragma once

#include "src/pch.hpp"

// Tutorial chapters 2-4 (Adrenaline_P / Skylark_P /
// AgencyZero_P) end through their own Kismet: TgSeqEvent_ObjectiveCaptured
// (Guardian / Elite Assassin death, Agency Zero proximity objective) ->
// cutscene/elevator -> TgSeqAct_RequestMission (next chapter / Dome). The
// generic mission end (BeginEndMission on the objective win) froze the map
// and sent the player home 30 s later, before RequestMission ran. On these
// maps an attackers-win end is held back; if no RequestMission arrives within
// kFallbackSeconds the normal end runs after all.
namespace TutorialChapterEnd {

// BeginEndMissionImpl: true = do not end now (chapter map, attackers win).
bool DeferEnd(ATgGame* Game, unsigned char winState);

// ServerOnRequestMission hook: the chapter hand-off is under way.
void OnRequestMission();

// Kismet TgSeqAct_RequestMission activation (ProcessEvent SeqOpActivated):
// RequestCount() before the original dispatch, AfterKismetRequestMission()
// after it. On chapter maps whose action target is not the player pawn the
// dispatch never reaches TgPawn.ServerOnRequestMission - run our hook for
// every player pawn then, so the chapter hands off.
int RequestCount();
void AfterKismetRequestMission(UObject* Action, int countBefore);

// GameEngine tick: fallback end when RequestMission never came.
void Tick();

}  // namespace TutorialChapterEnd
