# CLAUDE.md

Entry point for AI assistants (Claude Code, Copilot, etc.) working on this repo. Read this every session before touching code.

## What this is

A custom server reimplementation for an old UE3 multiplayer game. The original server is gone; we reconstruct it by hooking the **client binary** with a DLL injected via Detours. Most "missing" behavior we add is server-side reimplementation of native UC functions that the shipped binary stripped or stubbed.

See `README.md` for project background, roadmap, and Discord/community links.

## ‼️ HARD RULES — non-negotiable

These have been learned the hard way over hundreds of sessions. Re-deriving why each one exists wastes the human's time, so just follow them.

### Build / commit / tooling

- **NEVER build the project.** Never run `make`, `make server`, or any compile/link command. The human builds locally. Static re-reading of the code is the only acceptable verification.
- **NEVER auto-commit.** The human controls all `git commit`s. Even if a plan you wrote and the human approved lists "git commit" as a step, do NOT commit. Stage at most. Plans should omit commit steps entirely.
- **Add every new `.cpp` to `Makefile`** in the same response that creates it. The Makefile uses an explicit source list, NOT wildcards. Forgetting → silent skip → linker `undefined reference` → wasted build cycle. Treat `.cpp` creation + `Makefile` edit as atomic.
- **No filesystem snooping.** Do not `find /`, do not read or list paths outside the project repo without asking first. If you think you need a file outside `src/`, ask.

### Project mindset

- **Everything is server-fixable.** The game client is unchanged from the original-server era. If a feature used to work and doesn't now, it is a SERVER gap — not a client bug. Never say "client-side, can't fix from server." Find what marshal / RPC / native / event the original server emitted, and emit it.
- **Do it right — never offer "correct vs. workaround" as a choice.** When a canonical/faithful implementation path is identifiable, just do it. Don't surface big-bang vs. stopgap as a user question. Big-bang risk is mitigated by static review of the correct impl, not by shipping a hack first.
- **Don't pile hacks on working code.** "This used to work, the rewrite broke it" → `git log`, revert the speculative manipulation, find the regression. Don't add `SetCollision` toggles, `SetLocation` cargo-cult fixes, `ForceUpdate` flushes etc. on top of a broken rewrite.
- **No UC modifications.** The client UnrealScript is unmodified retail. You cannot add classes, fields, or `defaultproperties`. Translate UC idioms to C++ vtable / ProcessEvent detours.
- **Apply, don't describe.** When the fix is concrete and low-risk, make the edit. Don't end the turn with a how-to paragraph.
- **No speculation.** Only state facts from logs, code, and test results. Theories about why something might work go in conversation, not in source comments and not as final answers.
- **Comment out, don't delete.** When disabling code, use `//` not deletion. Code you removed is easier to restore from `//` than from `git log`.
- **Short verified comments only.** No long comments, no theories in source. The "why" of a tricky line is one sentence. Speculation belongs in the conversation.

### Hooking & native reimplementation

- **Stub-native hooks must call the parent hook directly, NOT `CallOriginal`.** When the hooked native is a stub (no-op in the binary), `CallOriginal` is a no-op and does NOT chain to parent class overrides. You must explicitly `ParentHook::Call((AParentType*)derived, edx)`. When the parent native is intact, `CallOriginal` works. Decision is per-hook, based on stub-vs-intact.
- **MSVC struct-return convention in Detours hooks.** Hooks on natives returning `FVector` / `FRotator` / any struct must declare return type as `OutType*` and `return outLoc;` at every exit. Void return → EAX junk → caller dereferences null → crash.
- **Reimplement on the engine class, never build a parallel module.** When the game has a purpose-built class (e.g. `ATgTeamBeaconManager`, `UTgEffectManager`) with stripped natives, fill in the natives ON THAT CLASS. Don't invent a sibling C++ module that re-stores its replicated state.

### Class identity & object lookup

- **NEVER `obj->IsA()`.** Unreliable on this build (SDK `StaticClass()` indices misalign with the binary's `GObjObjects`).
- **NEVER iterate `GObjObjects()` directly.** It's slow, scales linearly with world object count. Use one of the caches below.
- **Class identity → `ObjectClassCache::ClassNameContains(obj, "TgPawn")`** (or `GetClassName(obj)` for the underlying `const std::string&`). Resolves once per `UClass*`, hands out a stable string thereafter. Solves the GetFullName shared-buffer hazard automatically.
- **Need a `UClass*` to spawn / compare → `ClassPreloader::GetClass("Class Pkg.ClassName")`.** Do NOT add new per-class helpers (`GetTgFooClass()`); the existing ones are legacy.
- **Find an object by full name → `ObjectCache::Find(fullName)`.** Lazy progressive scan, caches as it goes.
- **Map-baked actors (player starts, mission objectives, volumes, …) → `ActorCache`.** Typed vectors, populated once at map load.
- **Always copy `GetFullName()` to `std::string` IMMEDIATELY** if you must call it directly: `const std::string name(raw ? raw : "<null>");`. The engine returns into a shared static buffer that gets clobbered on the next `GetFullName()` from ANY object.

### SDK conventions

- **Always use UE3 A/U engine prefixes on class names:** `ATgPawn` (actor), `UTgDeviceForm` (uobject). The prefix carries the actor-vs-uobject signal. Cast helper symbols are exempt (`Cast_TgPawn` stays).
- **Resolve pointer-arithmetic offsets to SDK field names.** Use `pawn->r_fDeployRate`, never `*(float*)((char*)pawn + 0x1034)`. Look up every offset in `src/SDK/SDK_HEADERS/*.h` before considering a doc or code change done.
- **Prefer SDK members over raw offsets** in general — the SDK headers expose typed fields for known offsets.
- **`bIsPlayer` is unreliable.** AI bots default `bIsPlayer=true` in this build (it's a "valid combatant" gate, not "has client connection"). Clearing it breaks turret targeting. Use `ObjectClassCache::ClassNameContains(Controller, "PlayerController")` instead.

### Logging

- **Debug logs go to a dedicated channel, NEVER `GetLogChannel()`.** `GetLogChannel()` drives the call-tree visualizer. Use `Logger::Log("<symptom-name>", ...)` for diagnostic prints — pick a named channel like `"pending"`, `"beacon"`, `"config"`, etc.
- **Channels are enabled via `control-server.json`, never `dllmain`.** Don't edit `dllmain` to toggle channels; ask the human to flip the config.
- **ONE channel when asking the human for logs.** Before asking the human to capture logs, consolidate every relevant `Logger::Log(...)` site onto a single dedicated channel (typically the symptom name), THEN tell the human which one channel to enable. Never ask for multi-channel captures — fragmented timelines.

### Data & content

- **Include dev / unreleased content in default pools.** For cosmetics / items / bots / etc., INCLUDE `(Test)`, `Troll`, `zDev Only -`, `zzFlair -` entries. Exposing unshipped original-dev work is a core appeal of the server, not noise to filter.

## Coding conventions

- **Field naming:** `r_` = replicated, `s_` = server-only, `c_` = client-only, `m_` = member. Match the convention on existing fields when adding new ones.
- **Hook layout:** one hook per file under `src/GameServer/<Subsystem>/<Class>/<Native>/<Native>.{hpp,cpp}`. The `.hpp` declares `HookBase<...>`; the `.cpp` implements `Call()`.
- **DB lives at `<repo-root>/server.db`** (a SQLite file). NOT at `data/server.db` — that's a different file. Use the sqlite3 CLI to read schema/data; never trust stale dumps.
- **Don't trust SDK `StaticClass()`** — the generated indices misalign with the binary's `GObjObjects`. Use `ClassPreloader::GetClass("Class Pkg.Name")`.
- **SDK `eventXxx()` wrappers resolve the BASE class.** Calling `effect->eventRemove()` from C++ runs `TgEffect.Remove`, not the `TgEffectBuff` override. Dispatch by actual class when polymorphism matters.
- **TArray:** use the SDK `TArray<T>` methods directly. `Add()`, `Clear()`, and the default ctor all route through `GAllocator`, and `Add()` handles an uninitialized (`Data==NULL`, `Count=Max=0`) array correctly. You don't need an init macro — just call `Add` on the field. There is no `Empty()`; use `Clear()`. Never `libc free/realloc` on `.Data` — UE3's allocator owns it.
- **FString:** do NOT modify the class. At handoff sites, allocate `fs.Data` via `GAllocator::Malloc` + manual field write, then null `Data` before scope exit.

## Repo orientation

- `src/` — server source
  - `src/SDK/SDK_HEADERS/` — full SDK with class layouts and offsets. `TgGame_classes.h` is the big one.
  - `src/GameServer/` — engine hooks (organized by subsystem/class/native)
  - `src/ControlServer/` — orchestration & matchmaking
  - `src/Database/` — SQLite access
  - `src/IpcClient/` — control ↔ DLL IPC
  - `src/Utils/` — caches (`ObjectClassCache`, `ObjectCache`, `ActorCache`, `ClassPreloader`), `Logger`, `Macros.hpp`
- `Makefile` — explicit source list. Add new `.cpp` here.
- `server.db` — SQLite at repo root. Asset / blueprint / effect data.
- `control-server.json` — runtime knobs, log channel toggles.
- Existing technical docs in the repo: `bots.md`, `deployables.md`, `device_volumes.md`, `equip-slots.md`, `equippable-devices.md`, `medsec_spawn_tables.md`, `umax_spawn_tables.md`, `docs/gameplay/rest-device-slot14.md`. Read the relevant one before working in an area.

## Supporting docs (read on demand)

Bigger detail lives in `docs/claude/`. Don't load eagerly; pull when relevant.

- [`docs/claude/hooking-and-sdk.md`](docs/claude/hooking-and-sdk.md) — HookBase pattern, ParentHook vs. CallOriginal stub rule, MSVC struct-return, FName ProcessEvent dispatch, SDK pitfalls (IsA, StaticClass, GetFullName, FString, TArray, SDK bitfield params bug)
- [`docs/claude/logging.md`](docs/claude/logging.md) — channel conventions, log file locations, single-channel-when-asking, common channels
- [`docs/claude/effect-system.md`](docs/claude/effect-system.md) — template/clone model, m_bIsManaged, m_bRemovable, displacement+revert, BUFF_PAWN/PROP_LIST, death cleanup, RCST on revive
- [`docs/claude/replication.md`](docs/claude/replication.md) — bNetInitial vs. delta gating, GetOptimizedRepListV2 conventions, CPF_Net rep gaps, NetGUID race, TCP per-message ~1450-byte cap

## Working agreement (style)

- Ask questions in chat as plain text. Don't gate replies behind `AskUserQuestion` with pre-canned options unless the choice is genuinely closed.
- Don't re-verify things that have been proven working (SDK wrappers, HookBase mechanics, etc.). Trust the existing infrastructure.
- When digging through Ghidra output, paginate searches (`limit=200` truncates large classes). Don't trust agent-style `(class level)` tags — verify the cited line is actually inside `defaultproperties{}`, not a function body.
- Logs at `~/Games/gawineprefix/drive_c/<channel>.txt` (server-side) and `~/<wineprefix>/drive_c/<channel>.txt` (client-side, when the human runs with a client DLL — they often don't). Don't trust client logs by default; the human may not be running the client DLL, in which case client log files are stale leftovers.
