// Standalone, host-compiled (x86_64) sanity test for the -announce parse path.
// NOT part of the server build. Run manually from the repo root:
//
//   g++ -std=c++17 -I. -Ilib tests/announce_command_test.cpp \
//       src/ControlServer/ChatSession/ChatCommand.cpp \
//       -o /tmp/announce_test && /tmp/announce_test
//
// Expected output ends with: "ALL TESTS PASSED".
//
// Compile line is unverified (this environment does not build). ChatCommand.cpp
// also holds the Dispatch* helpers, so the link may pull in Logger /
// InstanceRegistry / TcpSession translation units — add them to the g++ line if
// it fails with undefined references. Only TryParseChatCommand is exercised.
//
// Covers parsing only — permission (ChatSession::IsAnnouncer) and delivery are
// not reachable without the asio session, and are exercised in-game.

#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include <cassert>
#include <cstdio>

int main() {
    using ChatCommand::TryParseChatCommand;

    // Ordinary chat is untouched.
    {
        auto r = TryParseChatCommand("hello world");
        assert(!r.recognized);
        assert(!r.suppress_broadcast);
        assert(!r.announce.has_value());
    }

    // A message that merely mentions the word is still ordinary chat.
    {
        auto r = TryParseChatCommand("what does -announce do?");
        assert(!r.recognized);
        assert(!r.announce.has_value());
    }

    // Happy path: text is captured verbatim after the command token.
    {
        auto r = TryParseChatCommand("-announce Server restart in 5 minutes");
        assert(r.recognized);
        assert(r.suppress_broadcast);
        assert(r.announce.has_value());
        assert(*r.announce == "Server restart in 5 minutes");
    }

    // Case-insensitive command token; leading/trailing space trimmed; inner
    // spacing of the announcement text preserved.
    {
        auto r = TryParseChatCommand("   -ANNOUNCE   keep  inner   spacing   ");
        assert(r.recognized);
        assert(r.announce.has_value());
        assert(*r.announce == "keep  inner   spacing");
    }

    // No text -> recognized and suppressed, but nothing to send. This must not
    // broadcast an empty line, and must not fall through to ordinary chat.
    {
        auto r = TryParseChatCommand("-announce");
        assert(r.recognized);
        assert(r.suppress_broadcast);
        assert(!r.announce.has_value());
    }
    {
        auto r = TryParseChatCommand("-announce    ");
        assert(r.recognized);
        assert(r.suppress_broadcast);
        assert(!r.announce.has_value());
    }

    // Neighbouring commands still parse as themselves.
    {
        auto r = TryParseChatCommand("-coords");
        assert(r.recognized);
        assert(r.coords);
        assert(!r.announce.has_value());
    }

    // Channel command tokens (PLAYER_COMMAND path).
    {
        using ChatCommand::ChannelForCommandToken;
        assert(*ChannelForCommandToken("l")        == 4);
        assert(*ChannelForCommandToken("local")    == 4);
        assert(*ChannelForCommandToken("T")        == 5);   // case-insensitive
        assert(*ChannelForCommandToken("c")        == 7);
        assert(*ChannelForCommandToken("city")     == 7);
        assert(*ChannelForCommandToken("i")        == 1);
        assert(*ChannelForCommandToken("tr")       == 12);
        assert(*ChannelForCommandToken("lfg")      == 13);
        // Not channel commands: unimplemented features and junk.
        assert(!ChannelForCommandToken("a").has_value());    // agency
        assert(!ChannelForCommandToken("al").has_value());   // alliance
        assert(!ChannelForCommandToken("rg").has_value());   // raid
        assert(!ChannelForCommandToken("").has_value());
        assert(!ChannelForCommandToken("nonsense").has_value());
    }

    printf("ALL TESTS PASSED\n");
    return 0;
}
