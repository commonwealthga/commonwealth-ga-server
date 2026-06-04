# Replication

Notes on UE3 net property replication as it applies to this server, plus the recurring patterns of "client never got the update."

## GetOptimizedRepListV2 — stay as-generated

`src/GameServer/.../Actor__GetOptimizedRepListV2.cpp` (and per-class equivalents) split replicated properties per class into two blocks based on UC `replication{}` conditions:

- **`!bNetInitial`** — delta block. Properties replicated on every dirty tick.
- **`bNetInitial`** — initial bundle. Properties replicated once at channel open, then never again unless the actor is destroyed/respawned.

The split mirrors UC-generated rep conditions. **Do NOT reshuffle fields between the blocks.** The file is regenerated from UC declarations; divergence must be justified by a comment and ideally documented in a memory entry / inline note.

## Repnotify in the wrong block fires spuriously

A property declared with a `RepNotify` UC clause triggers `ReplicatedEvent('propName')` on the client whenever the field arrives in a rep bundle. If a RepNotify property lives in the `bNetInitial` block, opening a new channel for that actor (a client connecting late, a new map load, etc.) fires `ReplicatedEvent` with the baked default value — even though nothing actually changed server-side.

This caused the "defenders secured" announcer to fire spuriously on first sight 2026-05-16. Fixed by moving the relevant fields into the delta block where they belong.

**Look here FIRST for any "side effect fires on first sight of an actor" bug.**

## CPF_Net replication block — generator gaps

The V2 generator can miss "always replicate" fields if the UC declaration doesn't follow the standard form. `TgDeployable` had 8 always-replicate fields (`r_nFlashFireCount` et al.) that the generator skipped → stations heal but show no client FX. Added back manually 2026-04-18.

If a replicated field is silently not arriving on the client, suspect this. Check the per-class rep block has an entry for the field; if not, add it with the appropriate condition.

## NetGUID resolution race

Replicated object references serialized BEFORE the target's `NetGUID` is in the client's table resolve to `null` PERMANENTLY. The engine doesn't retry GUID resolution — once the ref slot is bound, subsequent replications of the same field hand the client the same null.

This breaks any "pre-spawned reference field" pattern: actor A is spawned before client connects, A's `r_TargetActor` field is serialized in the initial bundle, B (the target) hasn't been replicated yet, so the client gets `null` and stays stuck.

Fix: re-dirty the field after player connect (forces re-serialization at a time when both endpoints have the GUID).

Surfaced in the pre-spawned beacon DRI bug.

## TCP per-message ~1450-byte cap

The client silently drops anything past ~1450 bytes per framed TCP message. Bundle large logical state across multiple messages — the parser accumulates.

Where this bites: large inventory dumps, full skill saves, cosmetic catalogs.

## PackageMap filters TgClient out of USES

Forced-export GUIDs in different parent packages diverge — the client's `UPackage->Guid` for a given package mutates between sessions. If the server includes `TgClient` (UI-only, never replicated) in its package USES, the client kicks with "GA_Menu_FA version mismatch."

Filter at `PackageMap::Compute`, NOT at `SendPackageMap`. Filtering at the send site misaligns NetGUID indices on both ends. Fixed 2026-05-13.

## TCP/UDP ordering race on pawn-state-driven UI

The game/control-server split breaks the original server's in-process packet ordering guarantee. A TCP marshal that re-renders UI can beat a UDP property rep, leaving the widget reading stale state.

Mitigations applied:

- No `FlushNet` after the relevant Client RPC (lets the next UDP frame batch the property reps together).
- 120ms sleep in the control-server IPC handler before the TCP marshals (lets UDP property reps land first).

These are surgical mitigations, not a clean fix — the ordering guarantee is gone and there's no clean equivalent without re-merging the processes. Document any new ordering-sensitive path you discover.

## Inventory ID collision (bot ↔ player)

Player inventory IDs use `10000 + slot`. Bots use `Inventory::NextId()` which also started at `10000`. Identical IDs → client's ID-lookup randomly bound slots to the wrong pawn's device.

Fixed by bumping the bot pool to `1000000+`.

Lesson: if you add a new inventory-ID-producing path, allocate from a disjoint range.

## SEND_INVENTORY layout

`SEND_INVENTORY` (opcode `0x0182`) is the outer `DATA_SET` packet. Per-record InvData fields encode device+invId+slot. Mods need real `BLUEPRINT_ID` (not zero) for the suffix letters to render in the equip screen. Slots-per-profile is a separate table. Ask the human for the full wire-format breakdown when you need to extend the protocol.

## Equip-screen blank-slots gate

`InvManager.IsValid` compares `r_ItemCount` (replicated, `+0x1e8`, `CPF_Net`) to `m_InventoryMap` count. Mismatch → `FixupWidgets` skips the UI repopulate → all slots blank (device bar still works).

`Inventory::Equip`'s `r_ItemCount++` is a footgun. Set `r_ItemCount = DB pool count` after `Finalize` instead.

Full UI data-binding pipeline is non-trivial — ask the human if you need to extend it.

## Marshal subscriber TMap (CGameClient+0x330)

The client has a fallthrough dispatch table for opcodes not hardcoded in `MarshalReceived`. 6 dynamic opcodes identified:

- `0xD0` / `0x194` (queue)
- `0x1CD` (QUEST_ERROR — also has hardcoded bridge)
- `0x1D9` (agency directory)
- `0x1E9` (agent profile)
- `0x1EA` (auction bid result, fields `0x10C` + `0x61`)

All sendable from server with no client patches.

## Alert dispatcher / chat-message piggyback

Kill / assist / morale / autokick alerts all route to `PC vtable[0x734]` → `FUN_109649f0` → `CMarshal::send_marshal` → `FUN_10901f00` (the chat dispatcher at `CGameClient+0xf0`).

**The chat dispatcher IS the alert dispatcher.** Center-screen toast/sound is reachable from the server by sending opcode `0x73` (CHAT_MESSAGE) with `DISPLAY_AS_ALERT=1` + `MSG_ID`. Kill / death / assist / suicide MSG_IDs and the wire-field table — ask the human.

## Chat slash commands

The client filters `/`-prefixed chat messages; the server never sees them. Project commands use `-` prefix (e.g. `-changeteam`).
