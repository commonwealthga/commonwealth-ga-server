#pragma once

#include <string>

// Everyday player commands, typed in chat as "-name" (the game's
// own /bug /who /loc ... are client-local or go to dead Hi-Rez services) or
// arriving as an unknown slash command on PLAYER_COMMAND. Replies go to the
// player's System channel. Never dev-gated; nothing here changes game state
// except -stuck (cooldown, PvP refused DLL-side).
namespace PlayerCommands {

// Names we answer (lowercase, no leading '-' or '/'): working commands plus
// original commands that get a "not available on this server" reply.
inline bool Known(const std::string& n) {
    static const char* kNames[] = {
        "help", "commands", "who", "played", "loc", "stuck", "unstuck",
        "afk", "search", "claimpromotion", "support", "petition", "roll", "random", "time",
        "invites",
        "raid",
        "cargo",
        "gag", "ungag",   // staff only
    };
    for (const char* k : kNames) if (n == k) return true;
    return false;
}

// Execute `name` for the player owning session_guid. Returns false when the
// name is not one of ours (caller keeps its own unknown-command handling).
bool Run(const std::string& session_guid, const std::string& name, const std::string& rest);

} // namespace PlayerCommands
