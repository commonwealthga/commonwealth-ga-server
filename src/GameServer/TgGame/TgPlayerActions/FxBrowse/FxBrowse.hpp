#pragma once

#include <string>

namespace TgPlayerActions::FxBrowseCmd {

// -fx: step through candidate special-FX so a suitable marker look can be
// picked by eye. The FX assets live in the client's packages and cannot be
// previewed any other way.
//
// WHAT IS ON THE MENU
// -------------------
// asm_data_set_effect_groups.target_fx_id reaches 428 distinct FX, of which
// 254 have NO row in asm_data_set_special_fx_sounds (i.e. are silent) and do
// have particles and/or materials. The table in the .cpp is a curated subset,
// filtered to the ones plausibly usable as a persistent player marker:
// long-lifetime auras, body loops, and the material-swap (team-coloured)
// entries. Sorted longest-lifetime first, because lifetime is what determines
// how often the effect must be re-pushed — and every re-push replays the
// effect's intro animation, which is the visible "pulse" on the current
// scanbot marker.
//
// DELIVERY MODE — THIS IS ALSO A TEST
// -----------------------------------
// Two routes, selectable at runtime so we can find out which works without a
// rebuild:
//
//   -fx pawn   Push SetEffectRep on the TARGET's own r_EffectManager. Known
//              to be the route the game itself uses, but the manager belongs
//              to the target pawn, so the FX is broadcast to everyone the
//              pawn is relevant to. Fine for browsing on a quiet instance.
//
//   -fx own    Push on a TgEffectManager WE spawn, with r_Owner = the target
//              pawn but network Owner = the browsing session's
//              PlayerController and bOnlyRelevantToOwner = 1. If the client's
//              native UpdateEffectForms resolves the form's owner from the
//              manager's r_Owner (TgEffectManager.uc:61 + TgEffectForm.c_Owner
//              suggest it does), this yields ARBITRARY FX, PER VIEWER — which
//              would supersede the scanbot/foreman route entirely: any of the
//              254 silent FX, seen only by the spectator, with none of the
//              1163 beep.
//
// UNPROVEN: the "own" route depends on native behaviour I could not confirm
// by static reading. If nothing renders in that mode, the route is dead and
// browsing should continue in "pawn" mode. The `markers` log channel records
// which mode each push used.

enum class Action {
    Show,     // no arg — report current entry
    Next,
    Prev,
    Jump,     // index in arg
    Off,
    ModePawn,
    ModeOwn,
};

// index is only read for Action::Jump.
void Execute(const std::string& session_guid, Action action, int index);

// Per-frame refresh (GameEngine__Tick). No-op unless a session is browsing.
void Tick();

void ForgetSession(const std::string& session_guid);

} // namespace TgPlayerActions::FxBrowseCmd
