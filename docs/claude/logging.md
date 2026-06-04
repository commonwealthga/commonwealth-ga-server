# Logging

Channels, conventions, and "how to ask for logs without making the human angry."

## Channels are everything

Logger output is routed by **named channel**, not by severity. Channels are toggled at runtime via `control-server.json`. Disabled channels short-circuit cheaply.

`Logger::Log("channel-name", "fmt", args...)` is the only entry point. There is no `LogInfo` / `LogWarn` / `LogError` hierarchy — pick a channel that names what you're diagnosing.

## NEVER use `GetLogChannel()`

This is one of the most-violated rules.

`GetLogChannel()` returns a channel reserved for the **call-tree visualizer**. The visualizer parses output from that channel to render hook-call trees. Polluting it with diagnostic logs corrupts the visualizer's input.

For ANY diagnostic print, pick a named channel:

```cpp
Logger::Log("beacon",   "Spawning beacon %d at %s", id, locStr.c_str());
Logger::Log("pending",  "Tick: %d pending equip-updates queued",     count);
Logger::Log("config",   "Loaded %d entries from %s",                 n, path);
```

If you don't have a natural channel name, name it after the **symptom you're chasing**. "blank-slots", "stealth-stuck-on", "jetpack-no-lift" — whatever describes what the human reports.

## ONE channel per investigation

When you're about to ask the human to capture logs, consolidate first. Walk every `Logger::Log(...)` site that's relevant to the bug and switch them all to a single dedicated channel (typically the symptom name). THEN tell the human:

> Please enable channel `<symptom-name>` in `control-server.json`, repro the bug, and share the log.

Never ask the human to "enable channels A, B, and C." Multi-channel captures produce fragmented timelines that are hard to correlate. The human has explicitly said: "save it as a HARD RULE."

## Enabling channels — config only

Channels are toggled in `control-server.json` under the logging section. NEVER edit `dllmain` (or any source file) to "turn on a channel for debugging." Ask the human to flip the config.

## Log file locations

Each enabled channel writes to its own file. Locations vary by environment, but the pattern is `<wineprefix>/drive_c/<channel>.txt`. The human knows where their prefix is — when you need to refer to a log location in a question, say "the `<channel>` log" and let the human paste from wherever they keep it.

For listen-server / dedicated-server: only the server-side log is reliable. The human typically does NOT run with a client-side DLL injected, so client log files are stale leftovers from prior sessions. Don't trust client logs unless the human explicitly confirms the client DLL is loaded for the repro.

## Useful conventional channel names

(These already exist or are commonly used — match an existing channel rather than inventing a near-duplicate.)

- `pending` — deferred work / lazy bootstrap
- `beacon` — beacon system
- `config` — config & DB loads
- `effect` / `buff` — effect/buff/property apply/reverse
- `inventory` — equip / unequip / inventory map
- `bot` — bot spawn, AI init, factory dispatch
- `replication` — net property dirty / repnotify / NetGUID
- `objective` — mission objective state, kismet pin dispatch
- `damage` — damage application, damage numbers
- `marshal` — TCP marshal send/receive

When in doubt, grep `src/` for `Logger::Log("` to find what's already in use.

## Don't log inside hot paths without a gate

Some hooks fire every frame per actor (e.g. `Tick`, `CalcCameraView`, projectile `Update`). `Logger::Log` is itself cheap when the channel is disabled (pointer-keyed cache hit), but C++ evaluates variadic args BEFORE the callee can check the gate — so a call like `Logger::Log(ch, "%s", BuildBigString())` still runs `BuildBigString()` even when the channel is off.

Wrap expensive-arg sites with `Logger::IsChannelEnabled("channel")`:

```cpp
if (Logger::IsChannelEnabled("foo")) {
    const std::string s = BuildExpensiveString(obj);
    Logger::Log("foo", "tick: %s", s.c_str());
}
```

Cheap-arg sites can call `Logger::Log` directly — it gates internally.

## Crash-only channels

For very high-volume diagnostic flows where even file I/O is too slow, use `Logger::EnableCrashChannel(name)`. Output is recorded into an in-memory ring buffer and only flushed by the crash handler — zero disk I/O during normal operation. A channel can be in both the file-enabled and crash-enabled lists.

## Log directory

Per-channel files land at `<Logger::LogDir>/<channel>.txt`. `LogDir` defaults to `"C:"` (which resolves to `<wineprefix>/drive_c/<channel>.txt` under Wine). The human can override via `Config::GetLogDir()` and per-instance subdirs, but for "where do I find the log file" purposes, ask the human and let them paste the path.
