# Agencies & Alliances

Read this before touching the agency/alliance subsystem. It carries the reversed
wire protocol, the permission bit layout, and the findings that are NOT fixable
so nobody re-derives them.

## What this is

The persistent **agency** + **alliance** subsystem, reimplemented server-side.
The retail client sends these over the **ControlServer TCP connection** (same
transport as login/team/whisper), handled in
`src/ControlServer/TcpSession/TcpSession.cpp` `handle_packet()`. The client
natives are NOT stubbed — the retail client genuinely emits the opcodes; we just
had no server handlers or DB schema. All work is server-side (no client mods).

Code: `Database.{cpp,hpp}` + `TcpSession.{cpp,hpp}`. No new `.cpp`, so the
Makefile is untouched.

## Testing notes

- Log channels: `agency` (actions) and `tcp` (raw packets), enabled in
  `out/control-server.json` → `enabled_channels`. `dbperf` reports SQLite
  statements/sec if you're checking query load.
- DB is `out/server.db`. Reset test data with:
  `DELETE FROM ga_agency_members, ga_agency_ranks, ga_agencies,
  ga_alliance_members, ga_alliances;`
- Reversing note: the invite/accept natives are PlayerController vtable slots —
  `execAgencyInvite` → slot **0x4cc**, `execAgencyAccept` → **0x4d0**. The PC
  vtable base is NOT `0x11875fd8` (that table's 0x4cc is a mouse-hover trace).

## Ghidra reversing = self-serve via curl

The Ghidra MCP bridge is broken (-32000) but the **HTTP plugin is reachable at
`http://127.0.0.1:8090`** — curl it directly. Client binary base `0x10900000`
(matches hooks). Endpoints: `/decompile_function?address=0x..`,
`/xrefs_to?address=`, `/xrefs_from?address=`, `/strings?filter=&limit=`,
`/searchFunctions?query=`, `/segments`. Binary is stripped (FUN_xxxx).
When Ghidra hasn't defined a function (vtable-only targets), **capstone** on
`out/client/Binaries/GlobalAgenda.exe` works (pefile + capstone installed).

**Key techniques used:**
- Native exec: `/strings?filter=intUClassexecFn` → `/xrefs_to` gives the
  registration name-slot; func ptr is at `name_slot+4` (`/xrefs_from`).
- UI/data class vtable: find the class registration fn (asserts on
  `.\Src\Class.cpp` OR `FUN_112f6230(...,L"ClassName",...,ctor,...)`), the ctor
  sets `*this = &vtable`; read `*(vtable+slot)` for any virtual. Exec thunks do
  `mov eax,[this]; call [eax+SLOT]` — disassemble to get SLOT.
- Marshal primitives (client receive): `FUN_10938090`=GetInt32,
  `FUN_10937ed0`=GetWStr, `FUN_10938050`=GetFlag(str: first char Y/y/T/t=true),
  `FUN_10938110`=GetDataSet, `FUN_10938160`=GetDateTime(double).
- Find a receiver by scanning `.text` for a function writing the object's field
  offsets together (capstone regex on disp32 bytes).

## Wire protocol — FULLY REVERSED

### Transport / framing
Packet = `[u16 len][u16 opcode][u16 item_count][TLV fields...]`. TLV = `[u16
tag][value]`. Value encoding by tag TYPE (see `PacketView.hpp kTypeEnc` +
`TcpTypes.h` "Type:" comments): 0=WCHAR_STR (`WriteString`: u8 len + 0x00 +
narrow bytes — NOT wide), 1=FLOAT, 2=DOUBLE, 3=UINT32, 6=DATA_SET, 7=DATETIME
(8-byte double = **unix epoch seconds**). DATA_SET = `[tag][u16 rowcount]` then
per row `[u16 fieldcount][TLV...]`. Helpers: `Write1B/2B/4B/WriteString/
WriteDouble/WriteFloat`, `append(...)`, `WriteNBytesRaw`. Read via `PacketView`.

### Opcodes (GA_U, `TcpFunctions.h`)
Agency: CREATE 0xB4, INVITE 0xB5, INVITATION 0xB6, ACCEPT_INVITE 0xB7,
REJECT_INVITE 0xB8, PROMOTE 0xBB, DEMOTE 0xBC, REMOVE 0xBD (solo-leader remove =
disband), PLAYER_REMOVE 0xBF, PLAYER_ADD 0xC0, LEAVE 0xC1, GET_ROSTER 0xC2,
SET_SITE_ID 0xC3, SET_NOTE 0xC4 (MOTD/notes edit), UPDATE 0xC5,
UPDATE_RECRUITING 0xCA, UPDATE_RANKS 0xCB.
Alliance: CREATE 0xD1, DISBAND 0xD2, REMOVE 0xD3 (kick+leave), INVITE 0xD4,
INVITATION 0xD5, ACCEPT_INVITE 0xD6, REJECT_INVITE 0xD7, GET_MEMBER_LIST 0xDA.

### AGENCY_GET_ROSTER (0xC2) response — `append_agency_payload()` (12 fields)
The client's receiver = `FUN_113cd3d0` (dispatcher `FUN_113a6590`, gated on
PlayerController+0x6EC = m_AgencyData). Fields:
- `ROSTER_FLAGS 0x44C` (u32) = **bit0** → clears m_bNoAgency [THE GATE].
- `NAME 0x370` (str) = agency name **(NOT AGENCY_NAME 0x24)**.
- `AGENCY_MOTD 0x23`, `AGENCY_INFORMATION 0x20` (str).
- `RECRUITING_LISTING_TEXT 0x41F` (str); `RECRUITING_FLAG 0x41E` +
  `RECRUITING_SUB_ONLY_FLAG 0x420` = **"y"/"n" strings** (WCHAR_STR, not bytes —
  a raw byte desyncs the stream).
- `AGENCY_ID 0x1F`; `PLAYER_ID 0x3C3` = local player member id (→
  m_nLocalPlayerId, drives LocalPlayerIsLeader); `COUNT 0xE4` = #members.
- `DATA_SET_RANKS 0x193` rows{`RANK_ID 0x417`, `RANK_LEVEL 0x418`,
  `PERMISSION_FLAGS 0x3B0`, `NAME 0x370`} — rank name is 0x370, NOT RANK_NAME.
- `DATA_SET_MEMBERS 0x179` rows{`PLAYER_NAME 0x3C5`, `PLAYER_ID 0x3C3`,
  `RANK_ID 0x417`, `LEVEL 0x303`, `PROFILE_ID 0x3DF`} — member LEVEL/PROFILE are
  currently **50/0 placeholders** (see TODO "show class/level/online").
- "No agency" reset (after disband): send `MSG_ID 0x36E = 0x37B9` (1 field) —
  sets m_bNoAgency + ClearData → creation menu returns.

### ‼️ LEADER = rank_LEVEL 0
`LocalPlayerIsLeader` (`FUN_113cc580`) returns
`GetRankData(myRankId).nRankLevel == 0`. Seeded ranks: `{id0/level0=Leader
perms 0xFFFFFFFF, id1/level1=Officer, id2/level2=Member}`, creator member
rank_id 0. This gates the alliance Create panel (via `UpdateAllianceDisplay` →
LocalPlayerIsLeader). Getting this wrong = blank/locked panels.

### ALLIANCE_GET_MEMBER_LIST (0xDA) response — `append_alliance_payload()` (8)
Receiver = `FUN_113ce850` (dispatcher `FUN_113a6840`, gated on PC+0x6F0 =
m_AllianceData; row parser `FUN_113ce350`). Fields:
- `ROSTER_FLAGS 0x44C` = **0x5** (bit0 core + **bit2 = DATA_SET_AGENCIES gate**;
  flags=1 shows count but empty affiliated list — the member rows need bit2).
- `NAME 0x370` = alliance name; `AGENCY_NAME 0x24` = owner agency name.
- `ALLIANCE_ID 0x29`; `OWNER_AGENCY_ID 0x390`; `COUNT 0xE4` = #member-agencies.
- `CREATED_DATETIME 0xE9` (DATETIME/double) → "Formed <date>". ‼️ The alliance
  formatter (`FUN_10955ab0`, `FILETIME = K - input*(-864e9)`) reads this double
  as **DAYS SINCE 1900-01-01** (input 0 → 01/01/1900). Send
  `days = (unix_seconds + 2208988800) / 86400`. (NB: other DATETIME fields like
  ACQUIRE_DATETIME use raw unix-seconds doubles — the representation depends on
  which client formatter consumes it, so reverse the formatter per field.)
- `DATA_SET_AGENCIES 0x113` rows{`AGENCY_NAME 0x24`, `AGENCY_ID 0x1F`,
  `PLAYER_COUNT 0x3C2`, `TERRITORY_COUNT 0x4F5`}.
- "No alliance" reset: send `MSG_ID 0x36E = 0x624F` (1 field) — sets
  m_bNoAlliance + unlocks the in-tab Create panel. Empty response = blank tab.
- **"Leader: Unknown" in the affiliated-agency detail is UNFIXABLE** — RE-VERIFIED
  2026-07-19 at the binary. `sAllianceMemberInfo` (TgAllianceData.uc) does have
  an `sLeaderName` field, but NOTHING on the wire ever fills it: the row parser
  `FUN_113ce350` reads exactly 4 tags — 0x24 AGENCY_NAME → sMemberName, 0x1F,
  0x3C2, 0x4F5 — and the whole receiver `FUN_113ce850` reads only 0x36E, 0x44C,
  0x528, 0x370, 0x24, 0x29, 0x390, 0xE4, 0xE9, 0x113. No name tag anywhere, so
  sLeaderName keeps its "" default and the label falls back to "Unknown". The
  only lever would be smuggling the leader into AGENCY_NAME (e.g. "MESA
  (Angel)"), which corrupts the list + header text — not worth it. Leave it.

### Invitation popups = MSG_ID template + @@token@@
Popup = `MSG_ID` (localized template) + substitution fields keyed by token name
(@@leader_name@@ ← LEADER_NAME, @@alliance_name@@ ← ALLIANCE_NAME, etc). Msg
templates live in DB table `asm_data_set_msg_translations(msg_id, message)`.
Alliance invite = **MSG_ID 25111** "Invitation from @@leader_name@@ to join
alliance @@alliance_name@@". Agency-invite equivalent = **14266**
"@@leader_name@@ has invited you to join agency \"@@agency_name@@\".". Others:
14261 sent-confirm, 25116 alliance sent-confirm, 25143 rank-no-permission,
25112/25140 "respond to invite first", 22370/25148 self-invite.

### Cross-session invitation delivery pattern (implemented for alliance)
`static std::map<agency_id,alliance_id> pending_alliance_invites_` (+ mutex).
INVITE: resolve target agency by leader name (`GetAgencyIdByLeaderName`, rank 0),
store pending, `DeliverAllianceInvitation(target_user)` iterates `g_sessions_`
by `user_id_` and sends the popup. ACCEPT (0xD6, 0 fields): pop pending for MY
agency → `AddAgencyToAlliance`. REJECT (0xD7): erase pending. This design means
accept/reject payloads don't matter — server pending state drives it. Mirror
this for **agency** invites.

## Implemented (all tested in the live client)

### Agency + alliance foundations
- **Create agency** (AGENCY_CREATE 0xB4, `handle_agency_create`). Request =
  AGENCY_NAME 0x24 + LINEAR_COLOR_R/G/B (0x030D/0x030C/0x030B floats; alpha
  0x030A absent, no abbreviation). `Database::CreateAgency` is one transaction:
  insert agency + seed the 3 ranks + add the creator as a member. Reply is a
  normal AGENCY_GET_ROSTER push (no separate create-ack) so the receiver clears
  m_bNoAgency and the UI leaves the creation panel.
  - Schema (idempotent CREATE block at the end of `Database::Init`):
    `ga_agencies`, `ga_agency_members` (PK user_id ⇒ ONE AGENCY PER ACCOUNT),
    `ga_agency_ranks`.
  - ‼️ Two wire mistakes cost a full test cycle here, both recorded above: the
    ROSTER_FLAGS 0x44C bit0 gate is mandatory (without it the client ignores the
    entire block), and agency/rank NAME is **0x370**, not AGENCY_NAME/RANK_NAME.
  - ‼️ LEADER = rank_**level** 0, not rank_id 0 (see the section above). Seeding
    the leader at rank_id 1 hid the alliance Create panel.
- **Disband agency** (0xBE DISBAND / 0xC1 LEAVE / 0xBD REMOVE all route to
  `handle_agency_disband`; a solo leader's "remove last member" confirms as a
  disband). `Database::DisbandAgency` deletes the agency + members + ranks + any
  alliance it owns + its own alliance membership. Reply = the "no agency" reset
  MSG_ID 0x37B9 so the creation menu comes back.
  - Original symptom: solo leader → remove last member → confirm → nothing
    happened, because no handler existed for any of the three opcodes.
- **Create alliance** (ALLIANCE_CREATE 0xD1, name = NAME 0x370). Requires the
  player be their agency's LEADER. `Database::CreateAlliance` inserts the
  alliance and its owner agency as first member, atomically. Schema:
  `ga_alliances` + `ga_alliance_members` (PK agency_id ⇒ one alliance per
  agency).
- **Disband alliance** (ALLIANCE_DISBAND 0xD2, 0 fields, owner agency only).
  Reply = the "no alliance" reset MSG_ID 0x624F — an empty response is ignored
  and leaves the tab blank.
- **Invite an agency to an alliance** (ALLIANCE_INVITE 0xD4 = PLAYER_NAME 0x3C5
  of the target agency's leader) → ALLIANCE_INVITATION 0xD5 popup → ACCEPT 0xD6
  / REJECT 0xD7, both 0 fields (server pending state drives it — see the
  cross-session pattern above). Popup text = MSG_ID 25111 + LEADER_NAME +
  ALLIANCE_NAME; without the tokens it renders "Unknown".
- **Kick / leave an alliance** (ALLIANCE_REMOVE 0xD3 = AGENCY_ID 0x1F of the
  agency to drop; one opcode for both directions). Owner may kick anyone,
  a member may drop itself, the owner agency can't be removed from its own
  alliance (that's a disband). The owner having no Remove button is intended.
- **Affiliated-agencies list** — was arriving EMPTY despite COUNT being right:
  DATA_SET_AGENCIES 0x113 is gated by **ROSTER_FLAGS bit2**, so alliance flags
  must be 0x5 (bit0 core + bit2 members). The agency member set 0x179 is NOT
  flag-gated, which is why it worked with flags=1 — the asymmetry is the trap.
- **"Formed <date>"** — CREATED_DATETIME 0xE9. v1 sent raw unix seconds and
  rendered "00/4245/0000"; the alliance formatter reads DAYS SINCE 1900 (see
  the ALLIANCE_GET_MEMBER_LIST section for the conversion).

### Agency management

- **Invite player to agency** (0xB5 → 0xB6 popup → 0xB7/0xB8). TESTED WORKING.
  - Request 0xB5 carries PLAYER_NAME; accept = 0xB7, decline = 0xB8 (UC has a
    single native `ATgPlayerController::AgencyAccept(bool bAccepted)`, but it
    does split into the two opcodes).
  - The popup is NOT a data receiver like the roster/alliance ones — it's the
    HUD dialog `TgUIPrimaryHUD_DialogQuery` type `DialogInviteToAgency`, same
    widget the team invite (0x4B) uses. Payload = 3 fields: MSG_ID **14266** +
    LEADER_NAME + AGENCY_NAME (mirrors `send_team_invitation_popup`).
  - Server state: `pending_agency_invites_` keyed by **user_id** (the alliance
    map keys by agency_id). Target must be ONLINE — resolved by scanning
    `g_sessions_` for `player_name`, no DB name lookup. Accept →
    `Database::AddAgencyMember(..., rank_id 2 = Member)` → push roster.

- **Promote / demote / kick / leave**. TESTED WORKING.
  - Target arrives as `PLAYER_ID` = the member's **character_id** (that's what we
    put in the DATA_SET_MEMBERS rows; UC: `TgAgencyData::Promote/Demote/Remove(
    int nPlayerId)`). DB: `SetAgencyMemberRank` / `RemoveAgencyMember`, both
    keyed by character_id.
  - Promote/demote = one step along the `rank_level`-ASC rank list. Promote stops
    one below rank_level 0 — moving INTO Leader is `TransferLeader`, a separate
    native. Guard: actor must outrank both the target and the destination rank.
  - Kick = 0xBF, or 0xBD carrying a PLAYER_ID that isn't mine. Self/no target =
    the leave-or-disband path. Non-leader LEAVE used to be rejected outright
    (pre-invite there was never a second member) — now drops just that row.
- **Live push fan-out** (fixes "stale until relog / until you press O").
  - `TcpSession::DeliverAgencyRefresh(user_id)` finds the live session and pushes
    BOTH `send_agency_get_roster_response()` + `send_alliance_member_list_response()`.
  - Why it's needed: the agency roster poll always runs, so the AgentInfo/agency
    panel self-heals — but the **Alliance tab only refreshes while it's open**.
    Any change a player didn't initiate must be pushed to them explicitly.
  - Wired into: agency kick, agency disband (all other members), self-leave,
    agency invite-accept, alliance create / accept / disband / remove. For the
    delete paths, collect the affected user_ids BEFORE the DELETE (`Database::
    GetAllianceMemberUserIds` / `GetAgencyMemberUserIds`) — afterwards the
    membership rows are gone and the players are unreachable.

- **MOTD / description / public+officer notes** (AGENCY_SET_NOTE 0xC4). TESTED.
  - One opcode, four edits; the tag present says which: AGENCY_MOTD 0x23,
    AGENCY_INFORMATION 0x20 (Management tab "description"), PUBLIC_COMMENT 0x3F3
    / OFFICER_COMMENT 0x386 + PLAYER_ID for per-member notes.
  - ‼️ The notes also have to be ECHOED BACK: DATA_SET_MEMBERS rows now carry 7
    fields (added PUBLIC_COMMENT + OFFICER_COMMENT). The writes worked from day
    one; the boxes were empty because the roster response never returned them.
  - Officer text is withheld server-side from ranks without VIEW_OFFICER_MSG.
- **Recruiting tab** (AGENCY_UPDATE_RECRUITING 0xCA). TESTED. Fields:
  RECRUITING_LISTING_TEXT 0x41F + RECRUITING_FLAG 0x41E +
  RECRUITING_SUB_ONLY_FLAG 0x420, the two flags as "y"/"n" strings.
- **Transfer Leader** = **AGENCY_SET_OWNER 0xC6** (not a name — the client
  resolves it and sends the target's PLAYER_ID/character_id). TESTED. Target
  takes rank 0, old leader drops to the next rank, ga_agencies.leader_user_id
  follows. NOTE: there is NO confirmation dialog anywhere — TgUIAgencyMenu_
  Management.uc wires the button straight to TransferLeader(), and that class
  has no OnConfirmYesClicked (General/Facilities/Inventory do). Not addable
  server-side; the only push channels are the HUD invite dialog and MSG_ID
  status lines, neither of which can gate an action on a Yes.
- **Rank editor** (AGENCY_UPDATE_RANKS 0xCB). TESTED (checkbox edits, add rank,
  delete rank all persist). Leader only.
  - Request = DATA_SET_RANKS 0x193 with EVERY rank row {RANK_ID 0x417, NAME
    0x370, PERMISSION_FLAGS 0x3B0, RANK_LEVEL 0x418} — a full replacement set,
    so add / rename / delete / permission edits all arrive together.
    `Database::ReplaceAgencyRanks` rewrites the table in one transaction,
    refuses any set with no rank_level 0 row, and moves members off deleted
    ranks to the bottom rank.
  - **PERMISSION_FLAGS bits** (read off single-checkbox submits; baseline 0x0F
    rides along on every submit and maps to no checkbox):
    `0x40` Promote · `0x80` Demote · `0x100` Invite · `0x200` Remove/Kick ·
    `0x400` Edit Description · `0x800` Edit Public Msg · `0x1000` Edit Officer
    Msg · `0x2000` View Officer Msg · `0x8000` Edit MOTD · `0x20000` Facility
    Mgmt · `0x40000` Inventory Removal. Enum `Database::AgencyPerm`.
  - Enforced server-side on invite / promote / demote / kick / MOTD /
    description / public+officer message. rank_level 0 always passes.
    Verified in test: an Officer with Invite could invite, and was correctly
    refused on Edit Description.
- **Status/error lines** (`send_agency_message`): 14258 no permission, 14259
  character not found, 14260 already a member, 14261 invite sent, 14262 member
  of another agency, 14265 you have no agency, 14267 has a pending invite.
  CARRIER CONFIRMED: echo on **AGENCY_INVITE (0xB5)** — the request opcode —
  with MSG_ID + PLAYER_NAME. One send renders in all three places: the Invite
  tab's label, a modal OK dialog, and the chat log. Never 0xB6 (HUD dialog).

- **Member online status / class / location**. TESTED.
  - Member row parser `FUN_113ccbe0` reads, in order: PLAYER_NAME 0x3C5,
    PLAYER_ID 0x3C3, RANK_ID 0x417, LEVEL 0x303, PROFILE_ID 0x3DF,
    ACTIVE_FLAG 0x16, MAP_NAME_MSG_ID 0x327, PUBLIC_COMMENT 0x3F3,
    OFFICER_COMMENT 0x386. Nothing else exists on this row.
  - ACTIVE_FLAG is stored INVERTED: `eMemberStatus = (flag == 0)` and UC has
    `AGENCY_MEMBER_ONLINE = 0`, so **online must send "y"**.
  - MAP_NAME_MSG_ID → the Location column (client localizes it into
    sMemberLocationString). **Omit the field entirely when offline** — a 0 id
    localizes to "Unknown"; an absent field leaves the string "" (blank). Field
    count is per row, so offline rows send 8 fields and online ones 9.
  - LEVEL stays **50** — no level is tracked anywhere on this server
    (GSC_CHARACTER_LIST and QUERY_PLAYERS both hardcode 50). Not a placeholder
    to fix here; fix it globally or not at all.
- **Joined / Win-Loss / Last Online are UNFIXABLE**, same class as "Leader:
  Unknown". `TgUIAgencyMenu_General.uc` has the widgets (`joinedValueLabel`,
  `wlValueLabel`, `lastOnlineValueLabel` in `sMemberDetailPanel`), but the only
  per-member data the client holds is `sAgencyMemberInfo`, whose complete field
  list is: sMemberName, nMemberRankId, nMemberRankLevel, eMemberStatus,
  nMemberLocationId, sMemberLocationString, nMemberPlayerId, nMemberLevel,
  nMemberProfileId, sPublicComment, sOfficerComment. No date, no W/L field
  exists, so no tag we could send would ever reach those labels.
  `ga_agency_members.joined_at` is populated and has nowhere to go.

## DB query rate (checked 2026-07-19)
The roster is polled every **2s per online client** (alliance list every 5s), so
anything per-member in that path multiplies fast. Two problems were found and
fixed while wiring online status:
- `GetAgencyInfo` looked up each member's class with a separate
  `GetCharacterById` → **O(N) queries per poll** (a 40-member agency = 40 extra
  queries every 2s, per online member). Now a `LEFT JOIN ga_characters` inside
  the existing members query.
- Online detection first used `InstanceRegistry::GetActiveSearchablePlayers()`,
  a 5-table join that also pulls user/player names the roster already has. Now
  `Database::GetOnlineAgencyMemberMaps(agency_id)` — one query, scoped to that
  agency, returning character_id → map_name.

Result: a **fixed 5 queries per roster poll regardless of member count** (agency
id, meta, ranks, members, online map). No cache layer was added on purpose — a
cache buys nothing measured here and reintroduces the stale-data class of bug
this branch spent most of its time fixing. Measure first: enable log channel
**`dbperf`** in `out/control-server.json` for a statements-per-second count
(sqlite3_trace_v2 hook in `Database::GetConnection`, counts EVERY statement
server-wide, including code we didn't write).

## Still open
- **Agency + alliance chat channels** — deliberately NOT here; they belong with
  the wider chat rework (whisper / friends / ignore). Membership routing will
  need agency_id/alliance_id checks against the ga_* tables; see
  `src/ControlServer/ChatSession/`.
- **Member detail panel**: nothing left that's server-reachable (see the
  UNFIXABLE note above).
- The same DB-rate check is worth running on the friends/ignore path — the
  `dbperf` channel works server-wide.

## Gotchas recap
- Agency/alliance NAME renders ALL-CAPS in headers = client display font; DB
  stores correct case ("Rez"). Not a bug.
- One agency per ACCOUNT (ga_agency_members PK user_id); one alliance per AGENCY
  (ga_alliance_members PK agency_id).
- Real DB is `out/server.db` (repo-root `server.db` is 0 bytes / stale).
