#pragma once

#include <string>

namespace TgPlayerActions::SetItemCountCmd {

// Set ATgInventoryManager::r_ItemCount on one player's pawn to `total`.
//
// r_ItemCount must equal the number of entries in the CLIENT's m_InventoryMap or
// ATgInventoryManager::IsValid() (0x10a163f0) returns false and
// UTgUIAgentProfile_Equip::FixupWidgets skips the whole slot data-binding pass —
// every equip slot renders blank (reference_equip_screen_is_valid_gate.md).
//
// The control server owns component stock and sends the SEND_INVENTORY records
// that create those map entries, but r_ItemCount is a replicated engine field it
// cannot write. So it recomputes the authoritative total and pushes it here.
//
// ABSOLUTE, not a delta: a delta only has to be missed or applied twice once
// (a grant with no pawn, a dropped IPC, two grants in a frame) for the count to
// drift permanently. An absolute value is self-correcting on every send.
//
// MUST be called on the game thread (IpcClient::DrainInbound runs from
// Actor::Tick, which satisfies this).
void Execute(const std::string& session_guid, int total);

} // namespace TgPlayerActions::SetItemCountCmd
