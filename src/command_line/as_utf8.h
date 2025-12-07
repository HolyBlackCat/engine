#pragma once

#ifdef _WIN32
#include <string>
#include <vector>
#endif

namespace em::CommandLine
{
    // Outside of Windows does nothing. On Windows, discards the specified `argc`/`argv`, as they might be in a narrow encoding,
    //   and independently obtains replacements encoded in UTF-8.
    // Usage: `ArgsAsUtf8 args(argc, argv);`, then use `args.argc` and `args.argv`.
    // Right now we're not actively using this, because SDL entry point handles the conversion for us.
    class ArgsAsUtf8
    {
      public:
        int argc = 0;
        char **argv = nullptr;

        ArgsAsUtf8() {}

        ArgsAsUtf8(int new_argc, char **new_argv);

        // No pointers get invalidated, so this is fine.
        ArgsAsUtf8(ArgsAsUtf8 &&) = default;
        ArgsAsUtf8 &operator=(ArgsAsUtf8 &&) = default;

        #ifdef _WIN32
      private:
        std::vector<std::string> argv_storage;
        std::vector<char *> argv_ptrs_storage;
        #endif
    };
}
