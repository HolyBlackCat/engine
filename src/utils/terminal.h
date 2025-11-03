#pragma once

#include <cstdio>

namespace em::Terminal
{
    // Is the terminal attacked? The only valid parameters are `stdout` and `stderr`, for everything else will always return false.
    [[nodiscard]] bool IsTerminalAttached(FILE *stream);

    // On Windows, initializes ANSI escape sequence support. Only acts one per stream, even if called repeatedly.
    // Does nothing if `IsTerminalAttached(stream)` is false.
    // The only valid parameters are `stdout` and `stderr`, for everything else does nothing.
    void InitAnsiOnce(FILE *stream);

    // Sends the "reset format" sequence to `stream` if it's attached to a terminal. Does nothing otherwise.
    void SendAnsiResetSequence(FILE *stream);

    // On Windows, if the stream is discarded because the application was built without a console (with `-mwindows` or MSVC equivalent),
    //   attaches a new console to the process and redirect the stream there.
    // The only valid parameters are `stdout` and `stderr`, for everything else does nothing.
    // Calling this repeatedly on the same stream has no effect.
    // This also does `InitAnsiOnce(stream)` (because it's easier to do it unconditionally than to communicate to that function that it needs to reset its flag).
    // It seems that C++ streams can sometimes not realize that they've been retargeted, e.g. if you `std::cout << std::flush` before calling this, at least on libc++,
    //   and then they won't print anything even when the console opens. I guess just be sure to call this early enough if you need the console.
    void DefaultToConsole(FILE *stream);
}
