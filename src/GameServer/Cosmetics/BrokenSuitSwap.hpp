#pragma once

struct FCustomCharacterAssembly;

// Per-viewer cosmetic replacement for "broken" suits/helmets — cosmetics whose
// client-side attach stutters some machines. Players opt out of seeing the
// originals with "-togglebrokensuits 0" (ga_user_preferences key below); the
// ActorChannel::ReplicateActor hook then presents the mapped replacement
// cosmetic to that connection only. Everyone else (and the wearer) keeps the
// original. The swap table lives in BrokenSuitSwap.cpp (kSwapPairs).
namespace BrokenSuitSwap {

// ga_user_preferences key + default. Default ON — broken suits are cool;
// players who stutter opt out via -togglebrokensuits 0.
constexpr const char* kPrefKey = "show_broken_suits";
constexpr bool kPrefDefault = true;

// Nuclear option while the stutter culprits are being pinpointed: hide EVERY
// suit/helmet/flair from this viewer ("-toggleallsuits 0"). Everyone EXCEPT
// the viewer's own character renders the class-default body + bare head (own
// cosmetics stay; own broken suits still go through the swap map). Takes
// precedence over the swap map for other players.
constexpr const char* kPrefKeyAll = "show_all_suits";
constexpr bool kPrefAllDefault = true;

// Original flair/mesh values stashed by ApplyReplacements for RestoreAssembly.
struct SavedFields {
	int SuitFlairId;
	int SuitMeshId;
	int HeadFlairId;
	int HelmetMeshId;
};

// True when the assembly wears at least one mapped cosmetic. Pure in-memory
// check after the first call (which lazily resolves the swap table's mesh
// asm ids from the DB, once per process).
bool HasMappedCosmetic(const FCustomCharacterAssembly& a);

// True when the assembly wears any suit or helmet/flair at all (the cheap
// pre-gate for the hide-all path). Pure field reads, no table involved.
bool HasAnyCosmetic(const FCustomCharacterAssembly& a);

// True when this viewer connection has broken suits disabled. Memory-only:
// GClientConnectionsData → user_id → cached UserPreferences (preloaded at
// PLAYER_REGISTER). Unknown connections see originals.
bool ViewerHidesBrokenSuits(void* Connection);

// True when this viewer disabled ALL suits (kPrefKeyAll). Same memory-only
// lookup chain as ViewerHidesBrokenSuits.
bool ViewerHidesAllSuits(void* Connection);

// Rewrite mapped SuitFlairId/HeadFlairId to their replacement item id AND the
// paired mesh field (SuitMeshId/HelmetMeshId, picked by the pawn's gender) —
// flair and mesh must move together or animations play on the wrong skeleton
// (see the SuitMeshId comment in CosmeticEquip.cpp ApplyOnly). A map entry
// with `to = 0` means "show as wearing nothing": suit → flair -1 + the
// class-default body for (profileId, gender); helmet → flair -1 + mesh -1.
// `profileId` is the WEARER's r_nProfileId (pawn or PRI — both replicate it).
// Returns true if anything changed; originals go to `saved` for RestoreAssembly.
bool ApplyReplacements(FCustomCharacterAssembly& a, int profileId, SavedFields& saved);

// Hide EVERYTHING: suit → flair -1 + class-default body for (profileId,
// gender); helmet → flair -1 + mesh -1. Same "wearing nothing" state as the
// swap map's `to = 0` sentinel, applied unconditionally. Pure memory — never
// touches the DB or the swap table. Restore via RestoreAssembly.
bool HideAll(FCustomCharacterAssembly& a, int profileId, SavedFields& saved);

void RestoreAssembly(FCustomCharacterAssembly& a, const SavedFields& saved);

} // namespace BrokenSuitSwap
