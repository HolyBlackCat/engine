#include "command_line_as_utf8.h"

#ifdef _WIN32
#include <cstddef>
#include <stdexcept>

#define MOMINMAX
// I'm not adding `WIN32_LEAN_AND_MEAN` because that seems to exclude `CommandLineToArgvW()`.
#include <windows.h>
#endif

namespace em::CommandLine
{
    ArgsAsUtf8::ArgsAsUtf8(int new_argc, char **new_argv)
    {
        #ifndef _WIN32
        argc = new_argc;
        argv = new_argv;
        #else
        // Windows encodes `argv` in a narrow codepage by default, which can make us lose some characters.
        // Because of that, we need to independently obtain the arguments in UTF-16, and then convert them to UTF-8.
        // This is inspired by: https://github.com/libsdl-org/SDL/blob/main/src/main/windows/SDL_sysmain_runapp.c
        // And by my own code frmo another project: https://github.com/MeshInspector/mrbind/blob/2077ba0762d5a84cf1ef34800ca47df8438d95f6/src/generators/common/command_line_args_as_utf8.cpp
        (void)new_argc;
        (void)new_argv;

        struct ArgvwGuard
        {
            LPWSTR *value = nullptr;
            ~ArgvwGuard()
            {
                // `LocalFree()` is a no-op on null pointers.
                // Only one call to `LocalFree()` is needed, as `CommandLineToArgvW()` allocates one contiguous block of memory with everything.
                LocalFree(value);
            }
        };
        ArgvwGuard argvw_guard{CommandLineToArgvW(GetCommandLineW(), &argc)};
        if (!argvw_guard.value)
            throw std::runtime_error("ArgsAsUtf8: `CommandLineToArgvW` returned null.");

        argv_storage.resize(std::size_t(argc));
        argv_ptrs_storage.resize(std::size_t(argc) + 1); // Need an extra null pointer at the end.
        argv = argv_ptrs_storage.data();

        for (std::size_t i = 0; i < std::size_t(argc); i++)
        {
            // Compute the buffer size needed for the convesion, including the null-terminator.
            const int out_size = WideCharToMultiByte(CP_UTF8, 0, argvw_guard.value[i], -1, nullptr, 0, nullptr, nullptr);
            if (!out_size)
                throw std::runtime_error("ArgsAsUtf8: `WideCharToMultiByte` failed to calculate the resulting string size.");

            // Subtracting 1 because `std::string` adds a null-terminator automatically.
            argv_storage[i].resize(std::size_t(out_size) - 1);

            if (WideCharToMultiByte(CP_UTF8, 0, argvw_guard.value[i], -1, argv_storage[i].data(), out_size, nullptr, nullptr) == 0)
                throw std::runtime_error("ArgsAsUtf8: `WideCharToMultiByte` failed to convert the string.");

            argv_ptrs_storage[i] = argv_storage[i].data();
        }
        #endif
    }
}
