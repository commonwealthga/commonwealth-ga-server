#pragma once

#include "src/pch.hpp"

class ATgPawn;

// Pitch-shifted bot VO.
//
// The normal bark path (writing the replicated int ATgPawn::r_nBotSoundCueId,
// see `reference_bot_vo_bark_system.md`) has no pitch channel — it's a bare id
// and the client plays the cue as authored. This module is the alternative:
// resolve the cue to a real USoundCue* server-side and push it to each client
// through Engine.PlayerController.Kismet_ClientPlaySound, which takes an
// explicit PitchMultiplier.
//
// The sound stays POSITIONAL. Kismet_ClientPlaySound does
// `SourceActor.CreateAudioComponent(ASound, false, true)`, and
// Actor.uc:886 declares bAttachToSelf defaulting to TRUE, so the
// AudioComponent is attached to the pawn — it tracks the bot as it moves and
// attenuates/pans normally. bSuppressSpatialization is left 0.
namespace BotVoice {

// Play VO cue slot `cueId` on `pawn` for every connected client at `pitch`
// (1.0 = unmodified, >1 higher, <1 lower).
//
// `cueId` is the same semantic slot the r_nBotSoundCueId path uses; it's
// resolved against the pawn's OWN audio group (via r_nBodyMeshAsmId), so the
// bot still speaks in its own voice.
//
// Returns false if the cue couldn't be resolved or loaded for this pawn's
// audio group, or if no client took the RPC — callers should fall back to the
// plain r_nBotSoundCueId write rather than going silent.
bool PlayPitched(ATgPawn* pawn, int cueId, float pitch);

}  // namespace BotVoice
