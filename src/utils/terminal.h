#pragma once

#include <cstdio>

namespace em::Terminal
{
    // Is the terminal attacked? The only valid parameters are `stdout` and `stderr`, for everything else will always return false.
    [[nodiscard]] bool IsTerminalAttached(FILE *stream);

    // On Windows, initializes ANSI escape sequence support. Only acts one per stream, even if called repeatedly.
    // Does nothing if `IsTerminalAttached(stream)` is false.
    // The only valid parameters are `stdout` and `stderr`.
    void InitAnsiOnce(FILE *stream);

    // Sends the "reset format" sequence to `stream` if it's attached to a terminal. Does nothing otherwise.
    void SendAnsiResetSequence(FILE *stream);
}
