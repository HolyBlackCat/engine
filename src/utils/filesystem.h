#pragma once

#include "em/meta/zero_moved_from.h"
#include "em/zstring_view.h"
#include "utils/blob.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#ifdef _WIN32
#include <string>
#endif

namespace em::Filesystem
{
    // This is normally same as the directory where the executable is located.
    // Except on some weird platforms.
    // NOTE: This always ends with a directory separator.
    [[nodiscard]] zstring_view GetResourceDir();


    #ifdef _WIN32
    // Widen UTF-8 string to a `wchar_t` string.
    [[nodiscard]] std::wstring WindowsUtf8ToWide(zstring_view input);
    #endif

    using offset_t = std::int64_t;

    namespace Raw
    {
        // Like `std::fopen`, but handles unicode on Windows correctly.
        [[nodiscard]] FILE *fopen(zstring_view name, zstring_view mode);

        // Long fire aware seeking: [

        // File position or -1 on failure.
        [[nodiscard]] offset_t ftell(FILE *file);
        // Returns on 0 success. `origin` is one of `SEEK_CUR`, `SEEK_END`, `SEEK_SET`.
        [[nodiscard]] int fseek(FILE *file, offset_t offset, int origin);

        // ]
    }

    enum class SeekMode
    {
        absolute = SEEK_SET,
        relative = SEEK_CUR,
        end = SEEK_END,
    };

    // A C file wrapper.
    class File
    {
        FILE *handle = nullptr;

      public:
        constexpr File() {}

        // Opens a file, throws on failure.
        // If `size` isn't null, computes the size and writes it there.
        File(zstring_view name, zstring_view mode, offset_t *size = nullptr);

        File(File &&other) noexcept : handle(other.handle) {other.handle = nullptr;}
        File &operator=(File other) noexcept {std::swap(handle, other.handle); return *this;}

        ~File();

        [[nodiscard]] explicit operator bool() const {return handle;}
        [[nodiscard]] FILE *Handle() {return handle;}

        // File position or throws on failure.
        [[nodiscard]] offset_t CurrentPos() const;
        // Sets the current position or throws on failure.
        void SetCurrentPos(offset_t pos, SeekMode mode);
    };


    // The contents of a file loaded to memory.
    class LoadedFile : public zblob
    {
        // A file name for the user.
        Meta::ZeroMovedFrom<std::string> name;

      public:
        constexpr LoadedFile() {}

        // Load from a file.
        LoadedFile(zstring_view file_path);

        [[nodiscard]] const std::string &GetName() const {return name.value;}
    };
}
