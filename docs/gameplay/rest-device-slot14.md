# RestDevice Slot 14

This documents the hidden RestDevice/class-device behavior needed for R self-heal.

## What It Is

- Device `864` is `HUMAN BASE ATTRIBUTES`.
- It lives in equipment slot `14`.
- Its game device type is `TGDT_BASE_HUMAN_ATTRIB` / value `392`.
- The client uses this hidden slot for the R self-heal action.

Relevant decompiled script behavior:

- `TgPlayerController.SetDeviceSlotAndMode(14, 0)` calls `TgPawn.GetDeviceByEqPoint(14)`.
- If a `TgDevice` is returned, `TgPawn.StartAction()` calls `Dev.StartFire()`.
- `TgDevice.ReplicatedEvent('r_nDeviceId')` treats device type `392` as the RestDevice and calls `TgPawn.SetRestDevice(self)`.
- `TgPawn.InterruptRestDevice()` later stops `GetDeviceByEqPoint(r_nRestDeviceSlot)`.

## Required Server Shape

Both halves are required:

- Game hook must create a live `ATgDevice` in slot `14`.
- Control TCP inventory must send `DEVICE_ID=<inventory_id>` for that equipped row.

If the live actor exists but the TCP row sends `DEVICE_ID=0`, the client does not bind R to the RestDevice and R does nothing.

If the TCP row sends a nonzero `DEVICE_ID` but no live actor exists, the client expects an actor that cannot be found. That was the bad client-state/FPS recovery-loop failure.

## Current Implementation

Game hook:

- File: `src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp`
- Slot `14` / device `864` calls `Inventory::Equip()`.
- `Inventory::Equip()` sets:
  - `m_EquippedDevices[14]`
  - `m_bIsRestDevice`
  - `m_RestDevice`
  - `r_nRestDeviceSlot`
  - PRI `r_EquipDeviceInfo[14]`
- Spawn code also copies PRI `r_EquipDeviceInfo[14]` to pawn `r_EquipDeviceInfo[14]`.
- Spawn code marks pawn, PRI, and RestDevice dirty.
- Spawn code does not include this slot in the startup `UpdateClientDevices()` refresh counter.

Control TCP:

- File: `src/ControlServer/TcpSession/TcpSession.cpp`
- Equipped device rows send `ITEM_ID=<device_id>`, `INVENTORY_ID=<inventory_id>`, and `DEVICE_ID=<inventory_id>`.
- Cosmetic and armor item rows still send `DEVICE_ID=0` because they have no backing `ATgDevice`.

## Do Not Regress

- Do not make slot `14` DB-only if R self-heal must work.
- Do not force RestDevice slot `14` `DEVICE_ID=0` while a live actor exists.
- Do not send nonzero `DEVICE_ID` for slot `14` unless the game hook creates the live actor.
- Do not call startup `UpdateClientDevices()` just to publish hidden RestDevice state; that path previously crashed loading after `GSC_GO_PLAY`.

## Observed Failures

- DB-only RestDevice plus `DEVICE_ID=0`: login works, R self-heal no-ops.
- Live RestDevice plus `DEVICE_ID=0`: actor exists in logs, R self-heal still no-ops.
- Nonzero `DEVICE_ID` without a live actor: client enters bad inventory recovery state and FPS drops.
- Live RestDevice plus direct pawn/PRI dirty replication plus nonzero `DEVICE_ID`: R self-heal works.
