#pragma once

// Curated id whitelists for the spectator overlay feed. Both lists were
// supplied directly (FOR_CLAUDE_EFFECT_IDS.txt / FOR_CLAUDE_SKILL_IDS.txt) --
// UTgEffectGroup.m_nEffectGroupId covers a much larger space than either
// list, including plenty of internal/implementation groups nobody wants
// showing up on a broadcast overlay.
namespace OverlayIdCatalog {

// True if id is a whitelisted "effect" (buff/debuff) -- the only ids ever
// rendered as an icon tile on a player's overlay card.
bool IsEffectId(int id);

// True if id is a whitelisted "skill" group. Pushed through the same
// polling/relay path as effect ids (control server -> overlay HTTP JSON),
// but deliberately excluded from the player-card render -- skill ids are
// tracked for other consumers, not the tile itself.
bool IsSkillId(int id);

}  // namespace OverlayIdCatalog
