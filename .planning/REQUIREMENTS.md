# Requirements: Commonwealth GA Server -- Multi-Instance Architecture

**Defined:** 2026-03-21
**Core Value:** Players connect to a control server, select a character, and seamlessly land in a shared home map instance

## v0.0.7 Requirements

Requirements for multi-instance architecture. Each maps to roadmap phases.

### IPC Infrastructure

- [x] **IPC-01**: Control server and game instances communicate via TCP IPC channel with length-prefix framing on localhost
- [x] **IPC-02**: Shared IpcProtocol.hpp defines canonical message types and GUID format for both DLL and control server
- [x] **IPC-03**: Game thread IPC sends are non-blocking (enqueue to mutex-protected queue, dedicated sender thread drains)
- [x] **IPC-04**: PlayerRegistry protected with std::mutex for concurrent IPC thread + game thread access

### Control Server

- [x] **CTRL-01**: Standalone C++ control server binary compiles for both Linux (native ELF) and Windows (native PE) via build system
- [x] **CTRL-02**: Control server accepts client TCP connections on ports 9000/9001 using Asio
- [x] **CTRL-03**: Full GA TCP protocol (auth, character select, inventory, marshaling) migrated from DLL TcpSession to control server
- [x] **CTRL-04**: Control server has its own SQLite database for player/character/session state

### Player Routing

- [x] **ROUT-01**: Control server sends PLAYER_REGISTER IPC to game instance and waits for ACK before sending go_play_response to client
- [x] **ROUT-02**: GSC_GO_PLAY uses dynamic instance address from registry instead of hardcoded Config::GetPort()
- [x] **ROUT-03**: Control server maintains instance registry: mapName -> {pid, udp_port, ready, playerCount}
- [x] **ROUT-04**: Profile ID correctly flows from control server through IPC to game instance, resolving the default-profile bug

### Instance Lifecycle

- [x] **INST-01**: Control server spawns UE3 game instances with DLL injection -- Linux via fork/exec + WINEDLLOVERRIDES, Windows via CreateProcess + built-in DLL injector
- [x] **INST-02**: Per-instance UDP port assigned from control server port pool, passed as CLI arg
- [x] **INST-03**: DLL sends INSTANCE_READY IPC message after map load; control server blocks player routing until received
- [x] **INST-04**: Home map instance pre-spawned at control server startup
- [x] **INST-05**: Shared home map instances reused (existing running instance used instead of spawning duplicate)

### Event Routing

- [ ] **EVNT-01**: All in-process global event maps (GTcpEvents, GBeaconPickupEvents, GQuestEvents) replaced with IPC GAME_EVENT messages
- [ ] **EVNT-02**: Control server routes received GAME_EVENT messages to the correct TcpSession for TCP delivery to client

### Cleanup

- [ ] **CLEN-01**: DLL TcpServer, ChatServer, and TcpServerInit removed; DLL no longer binds ports 9000/9001

## Future Requirements

### Mission Instances (v0.0.8+)

- **MISS-01**: On-demand mission instance spawning when player selects a mission
- **MISS-02**: Player transfer between instances (home -> mission -> home)
- **MISS-03**: Mission complete/abandon returns player to home map instance

### Reliability (v0.0.8+)

- **RELI-01**: Instance health monitoring via PID watchdog
- **RELI-02**: Graceful instance shutdown with player drain
- **RELI-03**: Crash recovery and resource cleanup

### Scale (v0.1.0+)

- **SCAL-01**: Multi-machine deployment
- **SCAL-02**: Load balancing across machines
- **SCAL-03**: Mission matchmaking and queuing

## Out of Scope

| Feature | Reason |
|---------|--------|
| Multi-machine deployment | Same machine for now; TCP IPC allows future extension |
| Mission instances and flow | Home map first; missions are next milestone |
| Dynamic port allocation (auto-discover) | Fixed port pool sufficient; avoids UE3 port constraints |
| Load balancing | Single machine; no need |
| Persistent process pool (pre-warmed) | UE3 instances too heavyweight; spawn on demand |
| HTTP/REST for IPC | Binary protocol matches existing patterns |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| IPC-01 | Phase 6 | Complete |
| IPC-02 | Phase 6 | Complete |
| IPC-03 | Phase 6 | Complete |
| IPC-04 | Phase 6 | Complete |
| CTRL-01 | Phase 7 | Complete |
| CTRL-02 | Phase 7 | Complete |
| CTRL-03 | Phase 7 | Complete |
| CTRL-04 | Phase 7 | Complete |
| ROUT-01 | Phase 8 | Complete |
| ROUT-02 | Phase 8 | Complete |
| ROUT-03 | Phase 8 | Complete |
| ROUT-04 | Phase 8 | Complete |
| INST-01 | Phase 9 | Complete |
| INST-02 | Phase 9 | Complete |
| INST-03 | Phase 9 | Complete |
| INST-04 | Phase 9 | Complete |
| INST-05 | Phase 9 | Complete |
| EVNT-01 | Phase 10 | Pending |
| EVNT-02 | Phase 10 | Pending |
| CLEN-01 | Phase 11 | Pending |

**Coverage:**
- v0.0.7 requirements: 20 total
- Mapped to phases: 20
- Unmapped: 0

---
*Requirements defined: 2026-03-21*
*Last updated: 2026-03-21 after roadmap creation (phases 6-11 assigned)*
