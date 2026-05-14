#pragma once

#include <string>

class ATgPawn;

namespace TgPlayerActions::PossessCmd {

// Possess the AI-controlled pawn the player is currently aiming at. Refuses
// to possess other PlayerControllers' pawns. State is keyed on session_guid;
// running -possess twice in a row without -unpossess is a no-op (the first
// possession remains active).
void ExecutePossess(const std::string& session_guid);

// Reverse the last -possess: return the player to their original pawn and
// re-attach the AI controller to the previously-possessed bot. No-op if the
// session isn't currently possessing anything.
void ExecuteUnpossess(const std::string& session_guid);

// If `bot` is currently being possessed by some player session, push a fresh
// SEND_INVENTORY built from the bot's r_EquipDeviceInfo to that player's
// client. Used by the SetInventoryDirty hook to handle the bot-side refresh
// (player-side refresh goes through the normal DB-backed path). Returns true
// if a send was dispatched. Bots not under active possession are skipped.
bool TrySendBotLoadoutRefresh(ATgPawn* bot);

} // namespace TgPlayerActions::PossessCmd
