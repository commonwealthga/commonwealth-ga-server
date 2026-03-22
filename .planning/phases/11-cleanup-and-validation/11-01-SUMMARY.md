---
phase: 11
plan: 01
subsystem: DLL cleanup
tags: [cleanup, tcpserver-removal, gpawnsessions, makefile]
dependency_graph:
  requires: [10-02]
  provides: [CLEN-01]
  affects: [DLL build, GPawnSessions consumers]
tech_stack:
  added: []
  patterns: [GPawnSessions relocated to Storage layer]
key_files:
  created:
    - src/GameServer/Storage/PawnSessions/PawnSessions.hpp
    - src/GameServer/Storage/PawnSessions/PawnSessions.cpp
  modified:
    - src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp
    - src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.cpp
    - src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.cpp
    - src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.cpp
    - src/GameServer/Engine/GameEngine/Init/GameEngine__Init.cpp
    - src/GameServer/Engine/GameEngine/Init/GameEngine__Init.hpp
    - src/dllmain.cpp
    - Makefile
  deleted:
    - src/TcpServer/ (11 files: TcpServerInit, TcpEvents, TcpSession, ChatSession, ChatServer, PacketView)
decisions:
  - GPawnSessions moved to src/GameServer/Storage/PawnSessions/ -- permanent home alongside ClientConnectionsData/TeamsData/PlayerRegistry
  - QuestAction enum defined locally in CGameClient__MarshalReceived.cpp -- sole consumer after TcpEvents.hpp removal
metrics:
  duration: ~12 min
  completed: 2026-03-22
  tasks: 2
  files_changed: 15
---

# Phase 11 Plan 01: TcpServer Removal and GPawnSessions Relocation Summary

Remove dead DLL TcpServer code (ports 9000/9001) and relocate GPawnSessions to its permanent home in the Storage layer.

## What Was Built

**Task 1 (6836f0c):** GPawnSessions relocated to PawnSessions.hpp/cpp in `src/GameServer/Storage/PawnSessions/`. All 3 consumers (NotifyControlMessage write, NonPersistAddDevice read, NonPersistRemoveDevice read) updated to include the new header. TcpEvents.hpp removed from CGameClient__MarshalReceived.cpp. 4 Phase 10 commented-out event map write blocks deleted. GameEngine__Init stripped of TcpServerInit include, bInitTcpServer static member, and port-binding if-block. dllmain.cpp stripped of bInitTcpServer = true.

**Task 2 (c548b92):** src/TcpServer/ directory (11 files) deleted from disk and git. Stale Emacs swap file deleted. Makefile updated: 4 TcpServer SOURCE_FILES entries removed, 1 PawnSessions.cpp entry added. DLL builds clean with zero errors.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] QuestAction enum missing after TcpEvents.hpp removal**
- **Found during:** Task 2 (first build attempt)
- **Issue:** `QuestAction` enum and its usage in `CGameClient__MarshalReceived.cpp` were defined in `TcpEvents.hpp`. After removing TcpEvents.hpp, the build failed with "QuestAction not declared in this scope".
- **Fix:** Added `enum class QuestAction { Request, Abandon };` locally in CGameClient__MarshalReceived.cpp. This file is the sole consumer of the enum.
- **Files modified:** `src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.cpp`
- **Commit:** c548b92

## Verification Results

1. `ls src/TcpServer/ 2>&1` returns "No such file or directory" -- PASS
2. `grep -rn "TcpServer" src/ Makefile | grep -v ".gch" | grep -v "obj/"` returns nothing -- PASS
3. `grep -n "GPawnSessions" src/GameServer/Storage/PawnSessions/PawnSessions.hpp` shows extern declaration -- PASS
4. GPawnSessions hits in src/GameServer/: PawnSessions.hpp (extern), PawnSessions.cpp (definition), NotifyControlMessage (write), NonPersistAddDevice (read x2), NonPersistRemoveDevice (read x2) -- PASS
5. `make cleanserver && make out/version.dll` exits 0 -- PASS

## Self-Check: PASSED

- PawnSessions.hpp: FOUND
- PawnSessions.cpp: FOUND
- Commit 6836f0c (Task 1): FOUND
- Commit c548b92 (Task 2): FOUND
