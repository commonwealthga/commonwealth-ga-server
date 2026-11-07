#pragma once

#include "src/pch.hpp"

// Skylark suit-up. Tutorial chapter 3 (Skylark_P) starts in
// the Lab Suit. Its Kismet "Suit Up!" room (SeqEvent_Touch_13 -> suit-up
// matinee SeqAct_Interp_6 -> TgSeqAct_SetPlayerMesh "Default Suit", mesh 0)
// is where the original handed over the agent's class armour; the Lockhart
// "Offhand" lines + Offhand Popup follow. TgPawn.OnSetPlayerMesh ->
// native ServerOnSetPlayerMesh(0) lands in our hooks, which call OnSetPlayerMesh.
namespace TutorialSuitUp {

// SpawnPlayerCharacter, Skylark only: the agent's own look (before the Lab
// Suit) and loadout profile.
void SaveOwnLook(int64_t charId, const FCustomCharacterAssembly& own, int itemProfileId);

// True once this agent has suited up in this instance (respawns keep it).
bool IsSuitedUp(int64_t charId);

// From the ServerOnSetPlayerMesh hooks.
void OnSetPlayerMesh(ATgPawn* Pawn, int MeshAsmId);

// Lab Suit rule: the equip
// screen (Adrenaline rifle, Skylark "Off Hand - press U" popup) saves with
// suit/helmet/dye slots empty -> CosmeticEquip::ClearSlot(6) -> class armour
// fallback (and the agent's cosmetic rows deleted). Until the Skylark suit-up
// (Inception / Adrenaline / Skylark before Suit Up) the Lab Suit stays.
bool InLabSuitPhase(ATgPawn* Pawn);
void ApplyLabSuit(ATgPawn* Pawn, const char* why);

}  // namespace TutorialSuitUp
