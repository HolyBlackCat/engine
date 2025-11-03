#include "terminal.h"


// For terminal detection:
#if defined(_WIN32)
#  ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX 1
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#endif


namespace em::Terminal
{
    #ifdef _WIN32
    static void InitAnsiImmediately(bool is_stderr)
    {
        auto handle = GetStdHandle(is_stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        DWORD current_mode{};
        GetConsoleMode(handle, &current_mode);
        auto new_flags = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if ((current_mode & new_flags) != new_flags)
            SetConsoleMode(handle, current_mode | new_flags);
    }
    #endif

    bool IsTerminalAttached(FILE *stream)
    {
        static constexpr auto Lambda = []<bool IsStderr>
        {
            // We cache the return value.
            static bool ret = []{
                #ifdef _WIN32
                return GetFileType(GetStdHandle(IsStderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
                #else
                return isatty(IsStderr ? STDERR_FILENO : STDOUT_FILENO) == 1;
                #endif
            }();
            return ret;
        };

        if (stream == stdout)
            return Lambda.operator()<false>();
        if (stream == stderr)
            return Lambda.operator()<true>();

        return false;
    }

    void InitAnsiOnce(FILE *stream)
    {
        #ifdef _WIN32
        static constexpr auto Lambda = []<bool IsStderr>
        {
            // We do this at most once.
            [[maybe_unused]] static const auto once = []{
                InitAnsiImmediately(IsStderr);
                return nullptr;
            }();
        };

        if (stream == stdout)
            Lambda.operator()<false>();
        if (stream == stderr)
            Lambda.operator()<true>();
        #else
        (void)stream;
        #endif
    }

    void SendAnsiResetSequence(FILE *stream)
    {
        if (IsTerminalAttached(stream))
        {
            InitAnsiOnce(stream);
            std::fputs("\033[0m", stream); // `fputs`, unlike `puts`, doesn't append a newline.
        }
    }

    void DefaultToConsole(FILE *stream)
    {
        #if _WIN32
        static constexpr auto StreamIsNull = [](bool is_stderr) -> bool
        {
            return GetFileType(GetStdHandle(is_stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE)) == 0;
        };

        static constexpr auto Lambda = []<bool IsStderr>
        {
            // We do this at most once.
            [[maybe_unused]] static const auto once = []{
                FILE *stream = IsStderr ? stderr : stdout;

                if (StreamIsNull(IsStderr))
                {
                    // Do nothing even if this fails, I guess?
                    // As I understand it, this can fail if the console is already attached to the process (but not to this stream, or the condition above wouldn't pass).
                    AllocConsole();

                    // Here we unconditionally redirect the chosen stream and `stdin`, and ALSO redirect the remaining output stream if not already redirected.

                    if (!IsStderr || StreamIsNull(false))
                        freopen("CONOUT$", "w", stdout);
                    if (IsStderr || StreamIsNull(true))
                        freopen("CONOUT$", "w", stderr);
                    freopen("CONIN$", "r", stdin);

                    // It's easier to set up ANSI sequences now than to communicate to `InitAnsiOnce()`.
                    InitAnsiImmediately(stream);
                }
                else
                {
                    // Do this for symmetry with the other branch.
                    InitAnsiOnce(stream);
                }
                return nullptr;
            }();
        };

        if (stream == stdout)
            Lambda.operator()<false>();
        if (stream == stderr)
            Lambda.operator()<true>();

        #else
        (void)stream;
        #endif
    }
}
