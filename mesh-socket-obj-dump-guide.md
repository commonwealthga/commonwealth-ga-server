# Mesh socket inspection via in-game `obj dump`

When we need to verify a SkeletalMesh asset path, its socket names, bone bindings,
or relative transforms, the **client-side `obj dump <name>` console command**
is the fastest source of truth.

Use this when:
- Adding a new bot/pawn to the socket-cycle / muzzle-offset system
- Verifying that DB import (`asm_data_set_resources`, `asm_data_set_asm_mesh_fxs`)
  matches what the engine actually has
- Debugging "wrong socket / wrong bone / wrong offset" for FX or projectile spawn

---

## Hard constraints we hit while building this pipeline

Read these before reaching for runtime mesh APIs.

1. **`Pawn->Mesh` is null on the dedicated server.** Confirmed via
   `[MeshState]` diagnostic 2026-05-14 (channel `socketdiag`). Result:
   - `Mesh->GetSocketWorldLocationAndRotation()` is unreachable server-side.
   - We cannot query bone transforms via the live `USkeletalMeshComponent`.
2. **`StaticLoadObject` / `StaticFindObject` are NOT available** in our SDK
   wrappers. We can only walk `GObjObjects`. And the `USkeletalMesh` asset
   is likely not loaded server-side at all (no component to reference it).
   Don't bother trying to read the asset at runtime — extract data offline
   via `obj dump` from the client and ship it as static C++ constants or DB
   rows.
3. **`obj dump` truncates large `TArray`s.** Specifically `RefSkeleton`
   (which has all the bind-pose bone data) only printed ~41 ints (≈ 2.3
   bones) before being cut off. The full skeleton is unobtainable this way.
   This kills the "decode the full skeleton offline" path unless you also
   have a way to dump GNames (we don't).
4. **`NameIndexMap` is empty in the dump** — TMaps in UE3 typically aren't
   populated until first runtime lookup. So we can't go bone-name → bone-index
   that way either.
5. **`names.txt` (UObject dump) does NOT contain bone FNames.** It only has
   classes/functions/properties. Bone FNames live only in the live engine's
   GNames table after the asset loads.

**Bottom line:** the only data we get reliably about bone positions on
dedi is **what we can derive from `BoxExtent` + socket `RelativeLocation`
offsets** (see derivation method below).

---

## The 3-step walk

Start in-game on the client (you need an instance of the pawn visible — spawn
or witness one first).

### 1. Dump the pawn

```
obj list class=TgPawn_Boss_Destroyer       # find an instance name
obj dump TgPawn_Boss_Destroyer_0           # use the suffix you found
```

**Grab:** the `Mesh=` line. Looks like:
```
Mesh=SkeletalMeshComponent'TgPawn_Boss_Destroyer_0.SkeletalMeshComponent_42'
```
The right-hand side is the component's full object path.

### 2. Dump the SkeletalMeshComponent

```
obj dump TgPawn_Boss_Destroyer_0.SkeletalMeshComponent_42
```

**Grab:** the `SkeletalMesh=` line. Looks like:
```
SkeletalMesh=SkeletalMesh'Robot_Antarctica_IceRaptor.models.Skl_Robot_ANT_IceShrike'
```

This is the asset path. Format: `Package.Group.Object` (3 levels). Group can
be `models`, `Mesh`, etc. — always read it from the dump, don't guess.

(We can't pass this to `StaticLoadObject` server-side — see constraint #2.
The path is still useful as a label for the data we ship.)

### 3. Dump the SkeletalMesh asset

```
obj dump Robot_Antarctica_IceRaptor.models.Skl_Robot_ANT_IceShrike
```

**Grab from the `SkeletalMesh properties` section:**
- **`Bounds=`** line — `Origin` + `BoxExtent` (used for the derivation method below)
- **`Sockets(N)=...`** lines — list of socket sub-object refs

Then for each relevant socket, dump it individually (note `:` separator,
not `.`):

```
obj dump Robot_Antarctica_IceRaptor.models.Skl_Robot_ANT_IceShrike:SkeletalMeshSocket_0
```

Yields:
```
SocketName=WSO_Origin_01
BoneName=sideGunRecoil_Left
RelativeLocation=(X=0.000000,Y=0.000000,Z=0.000000)
RelativeRotation=(Pitch=0,Yaw=-16384,Roll=16384)
RelativeScale=(X=1.000000,Y=1.000000,Z=1.000000)
```

---

## Naming conventions observed

| Prefix | Meaning |
|---|---|
| `WSO_Origin_NN` | Weapon trace origin — sits AT the bone (rel=(0,0,0)). The shoulder pivot. |
| `WSO_Emit_NN` | Muzzle FX emit point — offset from bone (e.g. (±51, 0, 0) for muzzle tip). **Critical for the derivation trick below.** |
| `CSO_Emit_NN` | Cosmetic / character socket emit — non-weapon (top guns, accessories). |

NN is 1-based and matches `display_order` in `asm_data_set_asm_mesh_fxs` —
which is what `m_nSocketIndex` cycles through (1, 2, 1, 2, …).

---

## The derivation method (skip the RefSkeleton walk)

Since we can't read bone bind-pose positions at runtime AND can't decode the
full skeleton offline (truncation), we derive shoulder bone positions from
**bounding box + WSO_Emit socket offsets**.

### The geometric assumption

For shoulder-mounted weapons:
- The cannon **muzzle tip** sits at or near the model's outer Y extent
  (cannons stick out the sides of the body)
- The **shoulder bone** is INBOARD from the muzzle tip by the cannon length
- The cannon length = `WSO_Emit.RelativeLocation.X` (bone-local)
- For the math to work, the bone's local +X axis must point outward (verify
  this from the WSO_Origin's `RelativeRotation` — Yaw=±16384 / ±90°
  swivels the gun-axis to point forward, which only makes sense if the bone
  X axis was pointing sideways to begin with)

### Y derivation (lateral, mirrored L/R)

```
shoulder_Y_pawn_local = ±(BoxExtent.Y - |WSO_Emit_NN.RelativeLocation.X|)
```
Negative for left, positive for right (UE3: Y=right).

### Z derivation (vertical)

```
mesh_Z_to_pawn_local = mesh_Z - cylinder_half_height
```

(Per memory note: `Pawn->Location` = cylinder center; `cylinder_half_height`
comes from `asm_data_set_assembly_meshes.collision_height` — Shrike=117.)

For "shoulders are upper-third" (visually verifiable for most rigs):
- `mesh_Z_shoulder ≈ Bounds.Origin.Z + 0.4 * BoxExtent.Z`
- `pawn_local_Z = that - cylinder_half_height`

This is the wobblier number. Iterate visually if needed.

### X derivation (forward/back)

Default to **0** (shoulders centered front-to-back). Adjust empirically only
if the muzzle clearly visually leads/trails the body.

### Worked example: Boss Shrike

From dumps:
- `Bounds.Origin = (0, -1.3, 117.22), BoxExtent = (131.37, 110.68, 125.92)`
- `WSO_Emit_01.RelativeLocation = (51, 0, 0)` on bone `sideGunRecoil_Left`
- `WSO_Emit_02.RelativeLocation = (-51, 0, 0)` on bone `sideGunRecoil_Right`
- `cylinder_half_height = 117` (from DB, per memory)

Computed pawn-local shoulder positions:
- Y: ±(110.68 - 51) = **±59.68**
- Z: 117.22 + 0.4 × 125.92 - 117 ≈ +50.6 (rounded to **+63** in actual code
  after first visual iteration — the upper-third estimate skewed slightly low)
- X: **0**

Shipped values in `TgDeviceFire__InitializeProjectile.cpp`:
```cpp
constexpr float kShrikeShoulderY = 59.68f;
constexpr float kShrikeShoulderZ = 63.0f;
```

---

## Implementation pattern (per bot)

End-to-end pipeline for a multi-cannon bot:

### Step 1 — DB cycle data (`SocketCycle`)
Already DB-driven via `asm_data_set_asm_mesh_fxs JOIN asm_data_set_resources`
filtered by `display_order > 0 AND name LIKE '%Origin%'`. If the new bot's
asm rows are correctly imported, no work needed here. (Verify by enabling
`socketdiag` channel — `[MeshState]` line will print on first fire if the
SocketCycle override kicked in.)

This drives:
- `m_nSocketMax` set in `TgDevice__CalcFireSocketIndexMax` hook
- `m_nSocketIndex` cycled by engine
- `GetFireSocketName` returning `WSO_Origin_NN` per slot

### Step 2 — Static shoulder offsets (`InitializeProjectile`)
Per body_asm_id, ship a `(L_offset, R_offset)` pair derived above. In
`TgDeviceFire__InitializeProjectile.cpp`:

```cpp
const int asmId = SocketCycle::GetBodyAsmId(reinterpret_cast<ATgPawn*>(deviceInstigator));
if (asmId != kBodyAsmId) return;

// NOTE the INVERTED mapping — see "Slot mapping is inverted" below.
const float yOff = (slot == 2) ? -kShoulderY : +kShoulderY;
const FVector local(0.0f, yOff, kShoulderZ);
const FVector world = YawRotate(local, deviceInstigator->Rotation.Yaw);
Projectile->Location.X = deviceInstigator->Location.X + world.X;
Projectile->Location.Y = deviceInstigator->Location.Y + world.Y;
Projectile->Location.Z = deviceInstigator->Location.Z + world.Z;
```

#### Slot→side mapping is RIGHT-first

Empirically verified on Shrike 2026-05-14 (InitializeProjectile layer) AND
2026-05-15 (GetWeaponStartTraceLocationFromSocketOffsetInfo trace layer).
The same mapping holds at both layers, ruling out the
"engine pre-increments between layers" theory — the cause is purely the DB.

**Cause:** `asm_data_set_asm_mesh_fxs.display_order` for Shrike lists the
right-bone socket FIRST, despite the `WSO_Origin_01` / `_02` suffixes
suggesting Left=1 / Right=2. SocketCycle stores them in display_order, so
`vec[0]` (= `m_nSocketIndex == 1`) is the right cannon, `vec[1]` is the left.

**Use this mapping for every bot until a counterexample appears:**

| `m_nSocketIndex` | Bot's side | Apply offset |
|---|---|---|
| 1 | Right | +Y |
| 2 | Left  | -Y |

Same mapping at both the **trace-layer** hook (`GetWeaponStartTraceLocationFromSocketOffsetInfo`)
and the **InitProj-layer** hook (currently retired, but if revived for
non-Shrike bots that don't go through the trace path).

If a future bot's projectiles come out swapped after applying this mapping,
the bot's specific `display_order` in `asm_data_set_asm_mesh_fxs` is
left-first instead — flip the single ternary and add a comment.

### Step 3 — Yaw-rotate pawn-local → world

UE3: X=forward, Y=right, Z=up. Yaw rotates around Z. Rotator units:
65536 = 360° → multiply by `π / 32768` for radians.

```cpp
const float a = (float)yawUnits * (3.14159265f / 32768.0f);
const float c = cosf(a), s = sinf(a);
return FVector(local.X*c - local.Y*s,
               local.X*s + local.Y*c,
               local.Z);
```

### Step 4 — Verify visually
Enable channel (`shrike_aim` for Shrike; pick a name per bot). Log shows
slot, yaw, pawn pos, projectile pos. In game, fire and check that the
projectile trail aligns with the muzzle flash. Iterate `Z` if vertical
offset is off; `Y` should be tight from the derivation.

---

## Why not hook GetWeaponStartTraceLocation directly

UC `TgPawn_Boss.GetWeaponStartTraceLocation` is what the fire chain calls,
and it returns `Pawn->Location` when `m_TgSocketOffsetInfo` is null (always,
on dedi). Hooking it via ProcessEvent intercept WILL NOT FIRE because the
caller chain is UC-to-UC bytecode (inline dispatch — `ProcessEvent` is
bypassed for UC-to-UC, even if marked `event`). See memory note
`reference_takedamage_buff_hook.md` for the same gotcha pattern.

`InitializeProjectile` is the practical injection point because it's
reimplemented in our codebase already (we own `m_Owner` access there) and
runs after the projectile actor exists with a valid `Location` field.

For **hitscan** weapons (no projectile actor) the trace start fix is harder
— there's no equivalent post-spawn hook. Visually the muzzle flash already
alternates correctly (via the existing socket-cycle hooks); the hit
detection trace still starts at body center but for hitscan that rarely
matters because the target is large relative to the offset. If hitscan
trace position becomes important, the next step is to find the native that
Initiates the trace and rewrite its start parameter.

---

## What to record per new asset (checklist)

| Field | How to obtain | Where it ends up |
|---|---|---|
| `body_asm_id` | DB lookup from class name | C++ constant per pawn type |
| Full asset path (`Package.Group.Object`) | step 2 above | comment in C++ |
| `Bounds.BoxExtent.Y` | step 3 (asset dump) | derivation input |
| `Bounds.BoxExtent.Z` + `Bounds.Origin.Z` | step 3 | derivation input (Z) |
| WSO_Emit_NN `RelativeLocation.X` for L and R | step 3 (socket dumps) | derivation input |
| `cylinder_half_height` | already in `asm_data_set_assembly_meshes.collision_height` | indirect via DB |
| Computed `(±shoulderY, shoulderZ)` | derivation method above | C++ constants |

---

## Gotchas

- **`obj dump` truncates large `TArray`s** (RefSkeleton was clipped at ~41
  ints). Don't expect to extract full skeleton this way.
- **Sub-object separator is `:` not `.`** when addressing things inside a
  SkeletalMesh (`Skl_X:SkeletalMeshSocket_0`). Asset-level addressing uses
  `.`.
- Some sockets are filler / `toes` / pose helpers — filter by `BoneName`
  or name prefix when reading.
- `Yaw=16384` in UE3 rotator units = 90° (full circle = 65536). Watch
  signs when interpreting rotations.
- Bones like `sideGunRecoil_*` are animation-driven; the bind-pose offset
  is what you get on dedi (no AnimTree). Usually close enough for visual
  alignment.
- `m_TgSocketOffsetInfo` (TgPawn+0x1268) is declared but never written by
  any binary populator — it's a dead path. Don't try to construct one;
  the derivation method bypasses it entirely.
- The `bone X axis points outward` assumption holds for shoulder-mounted
  cannons in this game's rigs but isn't guaranteed for all bones.
  WSO_Emit's mirrored ±X across L/R sockets is the giveaway. If a future
  bot's WSO_Emits don't mirror cleanly, the derivation needs different
  geometric assumptions.

---

## Recorded data

### Boss Shrike (TgPawn_Boss_Destroyer, body_asm_id 794)
Asset: `Robot_Antarctica_IceRaptor.models.Skl_Robot_ANT_IceShrike`

| Socket | Bone | Rel.Location | Rel.Rotation |
|---|---|---|---|
| WSO_Origin_01 | sideGunRecoil_Left | (0, 0, 0) | (0, -16384, 16384) |
| WSO_Origin_02 | sideGunRecoil_Right | (0, 0, 0) | (0, +16384, 16384) |
| WSO_Emit_01 | sideGunRecoil_Left | (51, 0, 0) | — |
| WSO_Emit_02 | sideGunRecoil_Right | (-51, 0, 0) | — |
| WSO_Emit_03 | sideGunRecoil_Left | (46.21, 0.06, 18) | — |
| WSO_Emit_04 | sideGunRecoil_Right | (-46, 0, 18.25) | — |
| CSO_Emit_01 | topGunRecoil_Right | (-36.87, 0, -13.48) | — |
| CSO_Emit_03 | topGunMount_Left | (36.92, -10.67, -31.11) | — |

Bounds: Origin=(0, -1.3, 117.22), BoxExtent=(131.37, 110.68, 125.92).

Shipped offsets: Y=±59.68, Z=+63.
