#pragma once

#ifdef _WIN32
#include <string>
#include <vector>
#endif

namespace mrbind
{
    // Outside of Windows does nothing. On Windows, discards the specified `argc`/`argv`, as they might be in a narrow encoding,
    //   and independently obtains replacements encoded in UTF-8.
    // Usage: `CommandLineAsUtf8 args(argc, argv);`, then use `args.argc` and `args.argv`.
    // Right now we're not actively using this, because SDL entry point handles the conversion for us.
    class CommandLineAsUtf8
    {
      public:
        int argc = 0;
        char **argv = nullptr;

      private:
        #ifdef _WIN32
        std::vector<std::string> argv_storage;
        std::vector<char *> argv_ptrs_storage;
        #endif

      public:
        CommandLineAsUtf8() {}

        CommandLineAsUtf8(int new_argc, char **new_argv);

        // The pointers are stable under moves, so this is fine.
        CommandLineAsUtf8(CommandLineAsUtf8 &&) = default;
        CommandLineAsUtf8 &operator=(CommandLineAsUtf8 &&) = default;
    };
}
