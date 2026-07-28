# Chat channels — investigation + implementation plan

Branch: `implement-chats` (from `master`).
Status: **9 of 12 channels live and tested** — Personal, Announcements (20),
City, Local, Instance, Team, Trade, LFG, Agency, Alliance. All five
shared-infrastructure items except `PLAYER_COMMAND` (INF-2) are done.
Agency (2) and Alliance (3) landed once `enable-agencies-alliances` merged
upstream — they route on the agency-management DB backend. **Task checklist is §5b.**

Today the server has exactly two working channels: a server-wide broadcast sent
on channel 4 ("Local") and whispers on channel 8. This document records what the
client actually supports, verified against the binary, and the plan to build the
real channel set on top of it.

---

## 1. The channel enum (confirmed)

`FUN_10902990` in the client (`GlobalAgenda.exe`, image base `0x10900000`)
initialises two parallel 22-entry tables — channel display name (as a message id
resolved through `asm_data_set_msg_translations`) and channel colour
(`DAT_1199eca0`). The array index **is** the value we put in the `CHAT_CH_TYPE`
(0x00BE) wire field.

| id | name | msg id | colour | visible in a default tab? |
|----|------|--------|--------|---------------------------|
| 0  | Global Agenda GM | 42221 | `fff400` yellow | yes — All |
| 1  | Instance | 30348 | `277dff` royal blue | yes — All |
| 2  | Agency | 30349 | `32cd32` lime green | yes — All, Agencies |
| 3  | Alliance | 30361 | `ffa500` orange | yes — All, Agencies |
| 4  | **Local** | 30350 | `f0e68c` khaki | yes — All |
| 5  | Team | 30351 | `6495ed` cornflower blue | yes — All, Teams |
| 6  | Raid | 30352 | `f94aff` magenta | yes — All, Teams |
| 7  | City | 30353 | `f0e68c` khaki | yes — All |
| 8  | **Personal** (whisper/tell) | 30354 | `ba55d3` medium orchid | yes — All, Agencies, Teams, Personal |
| 9  | Private 1 | 30355 | `00ffaa` spring green | **no** |
| 10 | GM | 30356 | `ffa58f` salmon | **no** |
| 11 | System | 30357 | `fff400` yellow | yes — all four tabs |
| 12 | **Trade** | 52905 | `c8c8c8` grey | yes — All |
| 13 | **LFG ("Looking For Group")** | 52906 | `c8c8c8` grey | yes — All |
| 14 | Help | 30358 | `ffffff` white | no |
| 15 | VOIP | 30359 | `ffffff` white | no |
| 16 | Unknown | 30360 | `ffffff` white | no |
| 17 | VOIP | 30359 | `ffffff` white | Combat Log tab (mask bit 17) |
| 18 | VOIP | 30359 | `ffffff` white | no |
| 19 | Combat | 36537 | `ffffff` white | no |
| 20 | Unknown | 30360 | `fff400` **yellow** | **always** (see below) |
| 21 | Unknown | 30360 | `ffffff` white | no |

Colours are ARGB with alpha `0xfa`, read straight out of the initialiser
(`0x1199eca0 + id*4`). The name/colour loop bound is `iVar6 < 0x58` = 88 bytes =
22 entries, which independently confirms the enum has exactly 22 members.

> **Correction (2026-07-21).** Ids 12–21 in an earlier revision of this document
> were guessed from msg-id ordering and were wrong. The table above is now read
> directly out of the name-table initialiser: `FUN_10902990` stores each resolved
> message string to `[0x1199ec28 + channelId*4]`, so the destination address *is*
> the id. Ids 0–11 were unchanged by the correction. Colour table is the parallel
> array at `0x1199eca0` (same indexing, `0x1199eca0 + channelId*4`).

### How this was verified

Three independent cross-checks, all agreeing:

1. **Known-good values.** 4 = Local and 8 = Personal are what we already ship and
   what real clients render correctly.
2. **Tab filter bits.** `TgUIChat.uc` `defaultproperties` (and the shipped
   `TgGame/Config/DefaultUI.ini`) define five tabs by bitmask. `FUN_113be150`
   filters each incoming line with `filterBits & (1 << channelId)`. Decoding
   each mask against the table above gives a coherent grouping:

   | tab | mask | decodes to |
   |-----|------|-----------|
   | All (msg 59873) | 14847 | 0–8, System, Trade, LFG |
   | Agencies (59874) | 2317 | GlobalGM, Agency, Alliance, Personal, System |
   | Teams (59875) | 2401 | GlobalGM, Team, Raid, Personal, System |
   | Personal (59876) | 2305 | GlobalGM, Personal, System |
   | Combat Log (59877) | 131072 | Combat (bit 17) |

3. **The Chat settings screen** (`TgUISettingsMenu_Chat`, see §1b) exposes exactly
   11 channel checkboxes, and the All tab's mask 14847 is exactly
   `{0..8, 11, 12, 13}` — 12 bits, of which id 0 (Global Agenda GM) is not
   user-configurable. 14 − 1 mask members ... 11 checkboxes + GM + the two ids
   the player cannot toggle. Under the *old* table the All tab would have had
   Help and VOIP checkboxes and no Trade or LFG; the live screen shows Trade and
   LFG and neither Help nor VOIP. This pins 12 = Trade, 13 = LFG.

   (The Combat Log tab's mask is `1 << 17`, which lands on a VOIP name-table
   entry rather than the Combat entry at 19. Combat-log lines are client-local
   and irrelevant to us; not chased further.)

### Two quirks in the filter code, both exploitable

- `FUN_113be150` remaps **channel 20 → 11 before the filter test**
  (`0x113be235: cmp ecx,0x14` / `mov ecx,0xb`, then `1 << cl` against the mask).
  Channel 20 is an unnamed id (msg 30360), *not* Trade. Because bit 11 (System)
  is set in all four default tab masks, **a message sent on channel 20 renders in
  every tab regardless of the player's settings** — this is the unfilterable
  channel, and it is free (nothing else uses id 20). It draws yellow (`fff400`,
  same as System), and has no channel-name prefix of its own (msg 30360).
  Trade (12) is a normal, filterable, checkbox-controlled channel.
- The `CHAT_ANNOUNCEMENT_FLAG` wire field, when set on a channel 4 or 7 message,
  recolours it as channel 9 (spring green). Free "announcement" styling on Local
  and City without changing channel.

### Consequence for the "GM channel as global chat" idea

Channel **10 (GM) is in no default tab** — messages sent there are invisible
unless the player hand-edits `TgUI.ini`. Do not use it.

Use instead:
- **channel 0 "Global Agenda GM"** — yellow, present in the All tab. Right choice
  for staff announcements.
- **channel 20** (unnamed, remapped to the System filter bit) — the only
  unfilterable channel. Right choice if the goal is "reaches someone no matter
  how they configured their tabs". Renders white.

---

## 1b. The Chat settings screen — one filter layer, not two

`TgClient/Classes/TgUISettingsMenu_Chat.uc` (native impl, no UC logic):

```
const NUM_SHOWN_CONFIGURABLE_CHANNELS = 11;
const NUM_CONFIGURABLE_CHANNELGROUPS  = 4;
var CC_Group_Configuration m_Groups[4];   // each: m_Panel, m_GroupNameEditBox,
                                          //       CC_ConfigElement m_Channels[11]
```

- The dropdown at the top of the screen is the **tab** selector — the four
  configurable groups are `TgUIChat.m_ChannelGroupLogic[0..3]` (All, Agencies,
  Teams, Personal). The Combat Log tab (index 4) is not configurable. Tabs are
  also **renameable** (`m_GroupNameEditBox` → `m_TabName`).
- The 11 checkboxes are the 11 user-configurable channels: **ids 1–8, 11, 12,
  13** (Instance, Agency, Alliance, Local, Team, Raid, City, Personal, System,
  Trade, LFG).
- The checkboxes write `m_ChannelGroupLogic[tab].m_FilterBits` — **the same mask
  `FUN_113be150` tests.** There is no second filter layer. `m_FilterBits` is
  `globalconfig`, so it persists to `TgUI.ini` per player.

Consequences for us:

- Every channel we ship on ids 1–8/11/12/13 is individually mutable by the
  player, and defaults to *on* in the All tab.
- A player can also delete a channel from every tab, so no channel on that list
  is guaranteed delivery. Only **channel 20** is (via the → 11 remap).
- Nothing here is server-driven. We cannot change a player's filter bits; we can
  only choose which channel id to send on.

## 2. What the client lets us control

### Channel list — server-*gated*, not server-driven (corrected)

The send combo box is rebuilt by `FUN_113bdd20`. It walks a **hardcoded 10-entry
candidate table** and adds an entry only when that channel is also enabled in the
client's 16-slot table (which our `CHAT_CONFIG` 0x0072 packet fills):

```
for (i = 0; i < 10; i++)                       // loop bound 0x28 / 4
    if (cand[i] < 0x10 && enabledTable[cand[i]] != 0)
        addComboEntry(L"  /%s:  %s", prefix[i], channelName(cand[i]));
```

`cand[]` is at `0x1199c770`, the parallel prefix strings at `0x1199c748`:

| slot | prefix | channel | slot | prefix | channel |
|------|--------|---------|------|--------|---------|
| 0 | `L`  | 4 Local    | 5 | `RG`  | 6 Raid |
| 1 | `1`  | 7 City     | 6 | `TR`  | 12 Trade |
| 2 | `A`  | 2 Agency   | 7 | `LFG` | 13 LFG |
| 3 | `AL` | 3 Alliance | 8 | `P1`  | 9 Private 1 |
| 4 | `T`  | 5 Team     | 9 | `P2`  | 10 GM |

Consequences — these are hard client limits, not things a packet can change:

- **Instance (1) and Personal (8) are not on the list and can never appear in
  the combo box.** Instance is reachable only through the client's own `/i`;
  Personal through the tell flow (`UserSelectedChannel` special-cases 8 before
  the list is consulted). Confirmed in game: sending `CHAT_CONFIG {7,4,1,5,8}`
  produced a three-entry combo — Local, City, Team.
- **The prefixes are hardcoded.** City is `/1`, not `/C`. Nothing server-side
  changes that.
- **Private 1 (9) and GM (10) are selectable but sit in no default tab** — a
  player who picks them is shouting into a void. Do not enable them.
- Slot 6 = channel 12 labelled `TR` and slot 7 = channel 13 labelled `LFG` is a
  third independent confirmation that 12 = Trade and 13 = LFG.
- `UserSelectedChannel` (`0x113c17b0`) then reads `m_ChatChannelList[comboIndex]`
  (`this+0x208`) to recover the channel byte for the entry the player picked.

### Slash commands — the channel commands are real, and are server-side

The client's own `/help chat` (`FUN_10912700`) prints this list, built from
hardcoded (command string, message id) pairs:

| command | help text (msg id) |
|---------|--------------------|
| `/w [player name] <message>` | Personal Channel (25909) |
| `/t` | Team Channel (25903) |
| `/l` | **Attacker/Defender Channel** (25904) |
| `/a` | Agency Channel (25905) |
| `/al` | Alliance Channel (25906) |
| `/s` | Strike force (30362) |
| `/rg` | Raid Channel (26906) |

Note `/l` is literally named "Attacker/Defender Channel" — the client's own
documentation says Local is side-scoped. That settles the Local design question.

There are **two dispatch paths**, and this is the part that matters:

**1. Handled inside the client** (`FUN_10917300` builds a 114-entry name→id map;
`FUN_10915fc0` switches on the id):

- `tell` (9), `warn` (10), `w` (11) — parse `<name> <message>`; `warn` sets the
  extra GM-warn flag.
- **`gl` (17) → `FUN_109018d0(0, text)` — sends on channel 0 (Global).**
- **`i` (18) → `FUN_109018d0(1, text)` — sends on channel 1 (Instance).**
- `rgw` (19), `raidw` (20), `*raid` (108) — raid / raid-group chat.
- `who` (13), `chatwho` (33) — `CHAT_LIST_CHANNEL_MEMBERS` queries.
- `friend` / `unfriend` / `ignore` / `unignore` (24–27).
- `agencyinvite` (6), `agencyleave` / `aquit` (7), `agencydisband` (8),
  `invite` (2), `kick` (3), `leave` (4), `teamsetleader` (41),
  `setraidleader` (45).
- ids 50–107 are emotes, consumed by `FUN_109140e0` before the switch.
- `say` (37) and friends are UE console passthroughs.

`FUN_109018d0(channel, text)` is the *same* function the normal (non-slash) send
path calls with the combo box's current channel. It builds packet 0x0073 with
`CHAT_CH_TYPE` (0xBE) = channel and `MESSAGE` (0x355) = text — and it **guards
`channel < 0x10`**. So the client cannot send on channels 16–21. With the
corrected enum, Trade (12) and LFG (13) are *below* the guard: the client can
send on them, so they can be real player channels via the combo box. Channel 20
(the unfilterable one) is above the guard — server-send only, which is exactly
right for announcements.

**2. Forwarded to the server as `PLAYER_COMMAND` (0x019F).** `/t`, `/l`, `/a`,
`/al`, `/s`, `/rg` are *not* in the client's name map. A lookup miss falls to
`default:` → `FUN_10920990`, which packs the raw text into a `PLAYER_COMMAND`
(0x019F) packet with `TEXT_VALUE` (0x04FF) and sends it on the **main TCP
connection** (not the chat socket).

That is how these commands worked originally: the client is a dumb forwarder and
the *server* parsed `/a hello` and fanned it out. The control server handles no
`PLAYER_COMMAND` today, which is exactly why they appear broken now.

**This is therefore not optional and not a separate branch — implementing the
channel commands means implementing `PLAYER_COMMAND`.** It also gives us emote
handling and a protocol-native replacement for our `-command` prefix hack for
free.

---

## 3. Server-side state already available

Everything needed for scoping is one lookup from `ChatSession`, which already
holds `session_guid_`.

- `InstanceRegistry::GetInstancePlayerTaskForce(guid)` → `(instance_id,
  task_force)` for the player's current non-stopped instance.
- `InstanceRegistry::GetActivePlayersForInstance(instance_id)` → roster rows with
  guid, profile id and task force.
- `ga_instance_players` is populated for the home map as well as missions
  (`TcpSession.cpp` registers on PLAYER_REGISTER ACK for home, successor and
  mission instances), and `left_at` is stamped on exit — the data is live, not
  stale.
- `TeamService` is keyed by session guid but exposes no public "member guids"
  accessor. `FindTeamByMemberLocked` exists privately; needs a thin wrapper.
- **Agency and alliance chat are live** (channels 2 and 3). The management
  backend from `enable-agencies-alliances` merged upstream, so
  `ga_agencies` / `ga_agency_members` / `ga_alliances` / `ga_alliance_members`
  are populated and readable. `RecipientsFor` resolves guid → `user_id` →
  agency/alliance membership via `Database::GetAgencyIdForUser` /
  `GetAgencyMemberUserIds` / `GetAllianceIdForAgency` / `GetAllianceMemberUserIds`,
  then delivers to online members by walking connected sessions.

---

## 4. Proposed channel mapping

| Channel | Command | Scope | Effort |
|---------|---------|-------|--------|
| 7 City | combo box | every connected chat session — today's behaviour, renamed | trivial |
| 4 Local | `/l` | same `instance_id` **and** same `task_force` ("Attacker/Defender"); on home map / VR, side ignored | small |
| 1 Instance | `/i` (client-side) | same `instance_id`, both sides | small |
| 5 Team | `/t` | `TeamService` party members | small + one accessor |
| 8 Personal | `/w`, `/tell` | whisper — already working | none |
| 0 Global | `/gl` (client-side) | everyone; staff-gated or open | small |
| 2 Agency | `/a` | agency membership | done |
| 3 Alliance | `/al` | alliance membership | done |
| 6 Raid | `/rg`, `*raid` | not implemented | — |
| 0 GM | staff broadcast to everyone | small, needs an admin flag |
| 12 Trade, 13 LFG, 9 Private 1, 14 Help | not implemented | — |

This matches both the community expectation and the client's own design: Local is
instance + side, Instance is the both-sides instance-wide channel, City is the
cross-instance social channel.

### The one open decision: does City reach players inside missions?

- **Simple (recommended to ship first):** yes. City is exactly today's global
  broadcast with a different channel id — a one-constant change.
- **Insulated:** scope City to non-mission instances (home map + VR) and use
  channel 0 for cross-server pings to people mid-mission.

Ship the simple version, gather feedback, revisit. The insulated variant is a
predicate change in one function, not a redesign.

---

## 5. Implementation plan

1. **Recipient filtering in `ChatSession::broadcast`.** Add one
   `RecipientsFor(channel)` helper returning the peer set. The existing ignore
   gate, frame construction and whisper path stay untouched.
2. **One roster query per message.** Resolve the sender's `(instance_id,
   task_force)` once, fetch the instance roster once, then match peers by
   `session_guid_`. Do not query per peer. Add per-session caching against an
   epoch only if profiling shows it matters.
3. **Widen `CHAT_CONFIG`.** `AnnounceJoinIfNeeded` currently sends `{4, 8}`;
   send `{7, 4, 1, 5, 8}` instead. Channels the player cannot currently use
   (Team with no party, Local outside an instance) reply with a system message
   rather than being dynamically added and removed — far less state to keep in
   sync than re-sending `CHAT_CONFIG` on every instance or party transition.
3b. **Handle `PLAYER_COMMAND` (0x019F) in `TcpSession`.** Read `TEXT_VALUE`
   (0x04FF), split the leading token, and map `t`/`l`/`a`/`al`/`s`/`rg` to the
   channel ids above, routing the remainder through the same
   `RecipientsFor(channel)` path as step 1. Unknown tokens get a system reply.
   Without this, `/t` and `/l` stay dead no matter what `CHAT_CONFIG` says —
   only `/gl`, `/i`, `/w` and the combo box reach the chat socket on their own.
   The command arrives on the main TCP session, so delivery needs a
   send-by-guid entry point alongside the existing
   `ChatSession::SystemMessageTo`.
4. **Move `kSystemChannelId` from 4 to 11 (System).** Channel 11 is present in
   all four default tabs and is correctly yellow-tinted. Join/leave notices and
   error replies stop masquerading as Local chat.
5. **Add `TeamService::GetTeamMemberGuids(guid)`** — thin public wrapper over the
   existing locked member lookup.
6. **Agency and Alliance** — DONE. Landed after `enable-agencies-alliances`
   merged upstream; route on its DB backend.
7. **Logging and test.** Everything on the existing `chat` channel. Test matrix:
   two accounts across two instances and two sides, walking every channel, plus a
   whisper regression pass.

---

## 5b. Checklist — one item per channel

Status key: **DONE** = shipped and tested in game. **WIP** = someone owns it.
**TODO** = unclaimed. **BLOCKED** = waiting on something named.

### Shared infrastructure (do these first — most channels depend on them)

- [x] **INF-1 · `RecipientsFor(channel)` in `ChatSession::broadcast`** — one
      helper returning the peer set for a channel; the existing ignore gate,
      frame construction and whisper path stay untouched. Resolve the sender's
      `(instance_id, task_force)` once and fetch the instance roster once per
      message, then match peers by `session_guid_` — do not query per peer.
      *Blocks: Local, Instance, Team, City-insulated.*
- [x] **INF-2 · `PLAYER_COMMAND` (0x019F) handler in `TcpSession`** —
      **DONE + tested** (`/c`, `/city`, `/l`, `/local`, `/t`, `/i` all verified
      in a live merc mission; unknown-token reply still unconfirmed).
      `handle_player_command` reads `TEXT_VALUE`,
      strips an optional leading `/`, maps the token via
      `ChatCommand::ChannelForCommandToken`, and routes through
      `ChatSession::SendChannelMessageFrom` (guid-addressed, shares
      `SendOnChannel` with the chat socket). Unknown token → system reply.
      Aliases we define ourselves: `/c` and `/city` (the combo box hardcodes
      City's label as `/1`), `/i`, `/tr`, `/lfg`, plus long forms.
      Original description below.
      *Unblocks `/t` and `/l`; `/a`, `/al`, `/rg` stay unmapped until the
      features behind them exist (unknown-token reply instead).*
- [x] **INF-3 · Widen `CHAT_CONFIG`** — `AnnounceJoinIfNeeded` currently sends
      `{4, 8}`; send `{7, 4, 1, 5, 8}`. Populates the send combo box. Channels
      the player can't currently use (Team with no party, Local outside an
      instance) reply with a system message rather than being added/removed
      dynamically — far less state to keep in sync.
- [x] **INF-4 · Move `kSystemChannelId` 4 → 11** — channel 11 is in all four
      default tabs and is correctly yellow. Join/leave notices and error replies
      stop masquerading as Local chat. One-constant change, no dependencies.
- [x] **INF-5 · `TeamService::GetTeamMemberGuids(guid)`** — thin public wrapper
      over the existing private `FindTeamByMemberLocked`. *Blocks: Team.*

### Channels

- [x] **8 · Personal** (whisper/tell) — `/w`, `/tell`, reply keybind. **DONE**,
      shipped before this workstream.
- [x] **20 · Announcements** — `-announce <text>`, gated on the `announcers`
      list in `control-server.json`. Unfilterable (remaps to the System filter
      bit), yellow, no name prefix. **DONE + tested**: permitted sender sees
      yellow line, non-permitted sender gets the refusal.
- [x] **7 · City** — entry: combo box. Scope: every connected chat session, i.e.
      exactly today's global broadcast with the channel id changed.
      **DONE + tested.** Shipped the simple version per §4 (reaches
      players inside missions); the insulated variant is a predicate change in
      `RecipientsFor`, not a redesign.
- [x] **4 · Local** — combo box + `/l` / `/local`. Scope: same `instance_id`
      **and** same `task_force`; on home map / VR the registry reports no task
      force, so it falls back to instance-wide. **DONE + tested** in a live merc
      mission, including across an autobalance that moved all three players.
      Note for testers: autobalance can leave a player alone on a side, where
      "only I can see my Local message" is correct, not a bug. The `[Scope]`
      log line reports `recipients=` for exactly this reason.
- [x] **1 · Instance** — entry: **`/i` only** — the combo box cannot offer it
      (not in the client's candidate table, see §2). Scope: same `instance_id`,
      both sides. **DONE + tested.**
- [x] **5 · Team** — combo box + `/t` / `/team`. Scope: `TeamService` party
      members. **DONE + tested**, including the no-party system reply.
- [ ] **0 · Global Agenda GM** — entry: `/gl` (client-side) or a staff command.
      Scope: everyone. Yellow, in the All tab, not user-toggleable. Decide
      whether it's open to all players or staff-gated — if staff-gated, reuse
      the `announcers` config list rather than adding a second mechanism.
      **TODO.**
- [x] **2 · Agency** — entry: `/a` / `/agency`. Scope: online agency members
      (guid → `user_id` → `GetAgencyMemberUserIds`). No-agency reply:
      "*** You are not in an agency ***". **DONE.**
- [x] **3 · Alliance** — entry: `/al` / `/alliance`. Scope: online members of
      every agency in the alliance (`GetAllianceIdForAgency` →
      `GetAllianceMemberUserIds`). No-alliance reply:
      "*** Your agency is not in an alliance ***". **DONE.**
- [ ] **6 · Raid** — entry: `/rg`, `*raid`, `rgw`/`raidw` for raid-whisper.
      Scope: raid group. **TODO**, no raid-group concept exists server-side yet —
      scope this before starting.
- [x] **12 · Trade** and **13 · LFG** — combo box (`/TR`, `/LFG`), global scope,
      same recipient set as City. **DONE + tested** — free once City worked,
      both are on the client's combo candidate table.
- [ ] *9 Private 1, 10 GM, 14 Help, 15–19 VOIP/Combat* — **not planned.** 9, 10
      and 14+ are in no default tab, so messages sent there are invisible unless
      the player hand-edits `TgUI.ini`. Do not use them.

### Test matrix (run once the instance-scoped channels are in)

Two accounts, two instances, two sides. Walk every channel and confirm both
that it reaches who it should and that it does **not** reach who it shouldn't —
the second half is the one that catches a broken `RecipientsFor`. Plus a whisper
regression pass, since `broadcast` is shared. Everything logs to the existing
`chat` channel.

## 6. Reference — how to re-derive this

- Client binary: `D:\SteamLibrary\steamapps\common\Global Agenda Live\Binaries\GlobalAgenda.exe`,
  image base `0x10900000` (matches Ghidra and our hook addresses).
- Channel name/colour table init: `FUN_10902990` — decompiles cleanly, and its
  store addresses give the id mapping directly. Read this, don't infer ids from
  msg-id ordering.
- Chat settings screen: `TgUISettingsMenu_Chat` (UC declares the layout; natives
  registered at `0x1199c4c0..0x1199c4f0`, name/fn pairs).
- Per-channel colour fetch: `FUN_10901b00`, table at `DAT_1199eca0` (BSS, filled
  at runtime — reading the file gives zeroes, read the initialiser instead).
- Incoming message render + tab filter: `FUN_113be150`.
- Combo box selection: `FUN_113c17b0` (`UserSelectedChannel`).
- Chat entry submit: TgUIChat vtable `0x11877690`, slot `0x1e0` → `0x113c0010`.
- Slash command name→id table: `FUN_10917300`; dispatch switch: `FUN_10915fc0`.
- Message id → text: `asm_data_set_msg_translations` in `out/server.db`
  (the repo-root `server.db` is 0 bytes; use `out/server.db` via python sqlite3).


## 7. If you got here you are my hero.