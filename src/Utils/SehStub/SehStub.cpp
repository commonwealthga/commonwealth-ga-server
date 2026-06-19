// Workaround for an upstream mingw-w64-i686-crt toolchain bug (2026-06-19):
// the installed build (14.0.0.r98.g19f5121a2-1, a git snapshot, paired with
// gcc 16.1.0) ships libmsvcrt.a's i386__beginthreadex.o referencing
// __mingw_SEH_error_handler, but no library in the toolchain defines it —
// confirmed via `pacman -S --needed mingw-w64-i686-{gcc,crt,headers}`
// reporting all three already up to date. Without this stub, every DLL
// using -static (required — see CFLAGS comment in Makefile, dropping
// -static instead broke DLL injection) fails to link.
//
// This handler is only invoked, per mingw-w64-crt's i386__beginthreadex.c,
// as the SEH filter installed around a freshly-spawned thread's SSE
// floating-point state setup — an edge case none of this project's threads
// trigger. Returning ExceptionContinueSearch (1) is what letting the
// exception propagate unhandled would do anyway, i.e. a correct no-op for
// our usage.
extern "C" int __mingw_SEH_error_handler(void*, void*, void*, void*) {
    return 1; // EXCEPTION_CONTINUE_SEARCH
}
