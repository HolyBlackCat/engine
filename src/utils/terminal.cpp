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
    bool IsTerminalAttached(FILE *stream)
    {
        auto lambda = []<bool IsStderr>
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
            return lambda.operator()<false>();
        if (stream == stderr)
            return lambda.operator()<true>();

        return false;
    }

    void InitAnsiOnce(FILE *stream)
    {
        #ifdef _WIN32
        auto lambda = []<bool IsStderr>
        {
            // We do this at most once.
            [[maybe_unused]] static const auto once = []{
                auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
                DWORD current_mode{};
                GetConsoleMode(handle, &current_mode);
                SetConsoleMode(handle, current_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                return nullptr;
            }();
        };

        if (stream == stdout)
            lambda.operator()<false>();
        if (stream == stderr)
            lambda.operator()<true>();
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
}
