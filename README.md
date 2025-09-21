# Global Agenda server reimplementation

This is a work-in-progress attempt to provide an open source implementation of a Global Agenda server.

Global Agenda was a game developed by Hi-Rez Studios, the official server has been shut down and the game is no longer playable.

We are not affiliated with Hi-Rez Studios in any way and we do not claim any rights to the game.

## Roadmap

**Phase 1 "start with a dream"** (february - august 2025)
- [x] set up an SDK and compile a DLL
- [x] fix crashes when running game with "server" argument
- [x] reimplement missing low-level server-side networking code
- [x] implement a proof-of-concept TCP login server and establish a UDP gameplay connection

**Phase 2 "make it work"** (august 2025 - est. end of year 2025)
- [ ] *(in progress)* release the source code
- [ ] *(in progress)* refactor code, fix bugs
- [ ] *(in progress)* spawn a player-controlled character with hardcoded settings (gear, skills, level etc.)
- [ ] *(in progress)* start a match and play it to the end, with bots and enemy AI working
- [ ] *(in progress)* figure out game modes/difficulty modifiers (same map can be played on High/Max/Ultra-max/Double-agent)
- [ ] *(in progress)* allow multiple clients on one server

**Phase 3 "make it right"** (2026)
- [ ] get a public server up and running 24/7
- [ ] implement important parts of TCP server and database (enable changing gear, skills, cosmetics etc.)
- [ ] move hardcoded stuff to a database
- [ ] implement server orchestration (matchmaking, home map etc.)

**Phase 4 "make it fast"** (???)
- [ ] continue fixing bugs and performance issues
- [ ] continue implementing less important gameplay features
- [ ] modding

