#pragma once

#include "SDL3/SDL_stdinc.h"
#include "em/meta/common.h"
#include "em/meta/zero_moved_from.h"
#include "em/zstring_view.h"
#include "utils/byte_view.h"

#include <concepts>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace em
{
    template <typename T>
    concept Blob_ByteLike = sizeof(T) == 1 && Meta::cv_unqualified<T> && std::signed_integral<T> && !std::is_same_v<T, bool>;

    // An arbitrary array of bytes. Read-only.
    // Either owning and ref-counted, or non-owning.
    // Either null-terminated or not.
    template <bool IsNullTerminated> class basic_blob;

    using blob = basic_blob<false>; // Not null-terminated.
    using zblob = basic_blob<true>; // Null-terminated.

    template <bool IsNullTerminated>
    class basic_blob
    {
        // This nicely gives us owning and non-owning pointers, and we actually want the refcounted semantics.
        std::shared_ptr<const unsigned char[]> ptr;

        // If null-terminated, this size does NOT include the null terminator.
        Meta::ZeroMovedFrom<std::size_t> data_size;

      public:
        static constexpr bool is_null_terminated = IsNullTerminated;

        constexpr basic_blob() {}

        struct Owning {explicit Owning() = default;};

        // Fully custom.
        basic_blob(std::shared_ptr<const unsigned char[]> ptr, std::size_t size) noexcept
            : ptr(std::move(ptr)), data_size(size)
        {}

        // Some constructors below work always, some only for non-null-terminated blobs, because they don't guarantee null-termianted-ness.

        // Owning byte vector. Not null-terminated.
        template <Blob_ByteLike T>
        basic_blob(Owning, std::vector<T> vector) requires(!IsNullTerminated)
        {
            data_size = vector.size();
            // Take ownership of a pointer, and then...
            ptr = decltype(ptr)(
                reinterpret_cast<const unsigned char *>(vector.data()),
                [](const unsigned char *ptr){delete[] reinterpret_cast<const T *>(ptr);} // Don't forget the right deleter.
            );
            ::new((void *)&vector) decltype(vector); // Destroy the vector in a way that leaks the pointer.
        }

        // Owning string. Null-terminated.
        basic_blob(Owning, std::string str)
        {
            data_size = str.size();

            // Move the string into a shared pointer.
            auto sh_str = std::make_shared<std::string>(std::move(str));
            // Move the ownership into `ptr`, but make it point to the contents.
            ptr = decltype(ptr)(std::move(sh_str), reinterpret_cast<const unsigned char *>(sh_str->c_str()));
        }

        struct NonOwning {explicit NonOwning() = default;};

        // Non-owning string view. Not null-terminated.
        basic_blob(NonOwning, std::string_view str) noexcept requires(!IsNullTerminated)
        {
            data_size = str.size();
            ptr = decltype(ptr)(std::shared_ptr<void>{}, reinterpret_cast<const unsigned char *>(str.data()));
        }

        // Non-owning string view. Null-terminated.
        basic_blob(NonOwning, byte_view view) noexcept
        {
            data_size = view.size();
            ptr = decltype(ptr)(std::shared_ptr<void>{}, reinterpret_cast<const unsigned char *>(view.data()));
        }


        struct OwningSdl {explicit OwningSdl() = default;};

        // Owning, will call `SDL_free` to clean up.
        basic_blob(OwningSdl, const void *ptr, std::size_t size)
            : ptr(reinterpret_cast<const unsigned char *>(ptr), [](const unsigned char *data){SDL_free(const_cast<unsigned char *>(data));}), data_size(size)
        {}


        // Non-null?
        [[nodiscard]] explicit operator bool() const noexcept {return bool(ptr);}

        // Copies the data into a null-terminated view.
        [[nodiscard]] zblob MakeNullTerminated() const requires(!IsNullTerminated)
        {
            // Not going through `std::string`, as this is slightly more optimal.
            auto ptr = std::make_shared_for_overwrite<unsigned char[]>(size() + 1);
            std::memcpy(ptr.get(), data(), size());
            ptr[size()] = 0;
            return zblob(std::move(ptr), size());
        }

        [[nodiscard]] const unsigned char *data() const {return ptr.get();}
        [[nodiscard]] const unsigned char *c_str() const requires IsNullTerminated {return ptr.get();}

        [[nodiscard]] std::size_t size() const {return data_size.value;} // Doesn't include the null-terminator if any.

        [[nodiscard]] const unsigned char *begin() const {return data();}
        [[nodiscard]] const unsigned char *end() const {return data() + size();}

        [[nodiscard]] operator std::span<const unsigned char>() const {return {ptr.get(), data_size.value};}
        [[nodiscard]] operator zstring_view() const requires IsNullTerminated {return zstring_view(zstring_view::TrustSpecifiedSize{}, operator std::string_view());}
        // This doesn't work automatically (through `operator zstring_view`) in some cases,
        //   because the compiler refuses to consider more than one user-defined conversion at the same time.
        [[nodiscard]] operator std::string_view() const {return {reinterpret_cast<const char *>(ptr.get()), data_size.value};}
    };
}
