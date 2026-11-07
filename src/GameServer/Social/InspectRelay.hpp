#pragma once
#include <cstdint>

// X inspect: the client's Agent Profile asks for an inspected agent's data with
// SEND_CHARACTER_INVENTORY 0x01C5 (Equipment tab, sent on every inspect open) and
// SEND_CHARACTER_CRAFTING_SKILLS 0x017F (Stats tab init), both carrying
// SOURCE_CHARACTER_ID = the target. Their packet-table flag is 0x3000, so the client
// sends them over the UE marshal channel to the GAME server, not to the control server.
// Handle() forwards such a request to the control server (IPC GAME_EVENT
// "inspect_request"), which answers the viewer over its TCP link.
// Returns true when the packet was an inspect request (consumed).
namespace InspectRelay {
    bool Handle(short opcode, void* InBunch, void* edx);
}
