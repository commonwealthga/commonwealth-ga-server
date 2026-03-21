# Roadmap: Commonwealth GA Server

## Milestones

- ✅ **v0.0.5 Clean Inventory API** -- Phases 1-3 (shipped 2026-03-21)
- ✅ **v0.0.6 Class-Aware Spawning** -- Phases 4-5 (shipped 2026-03-21)
- 🚧 **v0.0.7 Multi-Instance Architecture** -- Phases 6-11 (in progress)

## Phases

<details>
<summary>✅ v0.0.5 Clean Inventory API (Phases 1-3) -- SHIPPED 2026-03-21</summary>

- [x] Phase 1: Enums & Constants (1/1 plans) -- completed 2026-03-21
- [x] Phase 2: Inventory::Equip API (1/1 plans) -- completed 2026-03-21
- [x] Phase 3: Refactor Existing Code (2/2 plans) -- completed 2026-03-21

</details>

<details>
<summary>✅ v0.0.6 Class-Aware Spawning (Phases 4-5) -- SHIPPED 2026-03-21</summary>

- [x] Phase 4: Class Identity (1/1 plans) -- completed 2026-03-21
- [x] Phase 5: Class Equipment (1/1 plans) -- completed 2026-03-21

</details>

### v0.0.7 Multi-Instance Architecture (In Progress)

**Milestone Goal:** Players connect to a control server, select a character, and seamlessly land in a shared home map instance -- the foundation for multi-map gameplay.

- [x] **Phase 6: Thread Safety + IPC Plumbing** - Mutex PlayerRegistry; build IPC channel and shared protocol header (completed 2026-03-21)
- [ ] **Phase 7: Control Server + Protocol Migration** - Standalone control server binary; full GA TCP protocol migrated from DLL
- [ ] **Phase 8: Player Registration + UDP Redirect** - PLAYER_REGISTER flow with ACK; dynamic GSC_GO_PLAY; profile ID fixed
- [ ] **Phase 9: Instance Lifecycle Management** - Spawn/track UE3 instances; readiness signaling; home map pre-spawn
- [ ] **Phase 10: In-Game Event Routing** - Replace in-process event maps with IPC GAME_EVENT; control server routes to client
- [ ] **Phase 11: Cleanup and Validation** - Remove dead DLL TCP code; validate end-to-end with two players

## Phase Details

### Phase 6: Thread Safety + IPC Plumbing
**Goal**: IPC transport and shared protocol exist; game thread is safe for concurrent IPC access
**Depends on**: Phase 5
**Requirements**: IPC-01, IPC-02, IPC-03, IPC-04
**Success Criteria** (what must be TRUE):
  1. A DLL IpcClient connects to a control-server IpcServer over loopback TCP and exchanges a PING/PONG round-trip without error
  2. A message sent from the game thread is enqueued and delivered by the sender thread without blocking the UE3 tick
  3. PlayerRegistry operations from the IPC thread and game thread do not race (mutex in place, no data corruption under concurrent access)
  4. IpcProtocol.hpp compiles cleanly in both the i686 DLL and the x86_64 control server with identical message type constants and GUID format
**Plans:** 2/2 plans complete
Plans:
- [ ] 06-01-PLAN.md -- Shared protocol headers + vendor nlohmann/json + PlayerRegistry mutex
- [ ] 06-02-PLAN.md -- IpcClient implementation + Actor__Tick wiring + Config + Makefile

### Phase 7: Control Server + Protocol Migration
**Goal**: Standalone control server binary handles all client TCP connections using the full GA protocol
**Depends on**: Phase 6
**Requirements**: CTRL-01, CTRL-02, CTRL-03, CTRL-04
**Success Criteria** (what must be TRUE):
  1. `make control-server` produces a native Linux ELF binary (and Windows PE on Windows) without errors
  2. A GA client can connect to the control server on ports 9000/9001, complete auth, and reach character select without connecting to the DLL
  3. The DLL no longer starts a TCP listener or handles any GA client TCP protocol messages
  4. Control server reads and writes its own SQLite database for player/character/session state
**Plans**: TBD

### Phase 8: Player Registration + UDP Redirect
**Goal**: Control server routes an authenticated player to the correct game instance via IPC ACK-before-redirect
**Depends on**: Phase 7
**Requirements**: ROUT-01, ROUT-02, ROUT-03, ROUT-04
**Success Criteria** (what must be TRUE):
  1. When a player selects go_play, the control server sends PLAYER_REGISTER IPC and waits for ACK before sending go_play_response to the client
  2. The go_play_response contains the dynamic instance UDP address from the instance registry, not a hardcoded port
  3. The game instance PlayerRegistry contains the correct profile ID for the player when their UDP HELLO arrives
  4. The instance registry holds mapName -> {pid, udp_port, ready, playerCount} and is queryable before routing
**Plans**: TBD

### Phase 9: Instance Lifecycle Management
**Goal**: Control server spawns, tracks, and reuses UE3 game instances; clients are never routed until an instance is ready
**Depends on**: Phase 8
**Requirements**: INST-01, INST-02, INST-03, INST-04, INST-05
**Success Criteria** (what must be TRUE):
  1. Control server spawns a UE3 process via fork/exec with Wine, passing the assigned UDP port as a CLI arg; the instance starts and binds that port
  2. Control server does not route any player to an instance until INSTANCE_READY IPC is received from the DLL
  3. Home map instance is running and marked ready before the first client connection attempt
  4. A second player connecting to the same home map joins the already-running instance rather than spawning a new process
**Plans**: TBD

### Phase 10: In-Game Event Routing
**Goal**: In-game events (beacon pickups, quest progress, inventory updates) reach the client through IPC rather than in-process maps
**Depends on**: Phase 9
**Requirements**: EVNT-01, EVNT-02
**Success Criteria** (what must be TRUE):
  1. Beacon pickup, quest event, and inventory event hooks send GAME_EVENT IPC messages instead of writing to GTcpEvents/GBeaconPickupEvents/GQuestEvents
  2. Control server receives GAME_EVENT, looks up the correct TcpSession by session GUID, and delivers the event to the client over TCP
**Plans**: TBD

### Phase 11: Cleanup and Validation
**Goal**: Dead DLL TCP code is removed and two players can connect, spawn, and receive events through the full architecture
**Depends on**: Phase 10
**Requirements**: CLEN-01
**Success Criteria** (what must be TRUE):
  1. DLL TcpServer, ChatServer, and TcpServerInit are removed; the DLL does not attempt to bind ports 9000/9001 at any point
  2. Two players can connect to the control server, complete auth and character select, land in the same home map instance with correct class identity, and receive in-game events
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Enums & Constants | v0.0.5 | 1/1 | Complete | 2026-03-21 |
| 2. Inventory::Equip API | v0.0.5 | 1/1 | Complete | 2026-03-21 |
| 3. Refactor Existing Code | v0.0.5 | 2/2 | Complete | 2026-03-21 |
| 4. Class Identity | v0.0.6 | 1/1 | Complete | 2026-03-21 |
| 5. Class Equipment | v0.0.6 | 1/1 | Complete | 2026-03-21 |
| 6. Thread Safety + IPC Plumbing | 2/2 | Complete   | 2026-03-21 | - |
| 7. Control Server + Protocol Migration | v0.0.7 | 0/? | Not started | - |
| 8. Player Registration + UDP Redirect | v0.0.7 | 0/? | Not started | - |
| 9. Instance Lifecycle Management | v0.0.7 | 0/? | Not started | - |
| 10. In-Game Event Routing | v0.0.7 | 0/? | Not started | - |
| 11. Cleanup and Validation | v0.0.7 | 0/? | Not started | - |

---
*Roadmap created: 2026-03-20*
*Last updated: 2026-03-21 after Phase 6 planning (2 plans created)*
