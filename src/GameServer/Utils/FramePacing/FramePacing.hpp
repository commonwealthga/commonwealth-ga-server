#pragma once

// Game-thread frame pacing.
//
// Between frames the engine waits for the next tick (NetServerMaxTickRate):
// it sleeps most of the gap, then busy-waits the last few milliseconds with
// Sleep(0). Under Wine every Sleep(0) is a scheduler yield plus bookkeeping
// syscalls, so an otherwise idle instance burns a noticeable share of a core
// in that loop.
//
// Install() hooks Sleep: a Sleep(0) on the game thread becomes a short real
// delay (kYieldDelayUs) instead of a yield. The frame still ends on time (the
// engine re-checks the clock after every call); it just checks far less often.
// Other threads and non-zero sleeps are untouched.
//
// Wine only (no-op on Windows). Off with the -noframepacing switch.
namespace FramePacing {

void Install();

// Called at the top of GameEngine::Tick; records the game thread once.
void NoteGameThread();

}
