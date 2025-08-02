#pragma once

#include "em/meta/common.h"

#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>

// `em::byte_view` is basically a `std::span<const char>`, but can also bind to ranges of other types (trivially copyable ones).
// We use `char` as the element type instead of `unsigned char` to allow it to be `constexpr` when bound to `char` ranges.
// Binding it to a range of any other type makes it non-constexpr.

namespace em
{
    class byte_view;

    // Is this a range that `byte_view` can be constructed from?
    // Note that we explicitly ban string literals here (and const char arrays in general), since they have a trailing null,
    //   which we really don't want to count in the view's `.size()`. Cast your literals to `std::string_view` or `std::span`.
    // This should ignore cvref (apart from the string literal check, which only triggers on const, see below).
    template <typename T>
    concept byte_viewable_range =
        !Meta::same_ignoring_cvref<T, byte_view> &&
        std::ranges::contiguous_range<T> &&
        std::ranges::sized_range<T> &&
        std::is_trivially_copyable_v<std::ranges::range_value_t<T>> &&
        !Meta::possibly_string_literal_cvref<T>; // Explicitly opt out of string literals, see above.

    class byte_view
    {
        std::string_view underlying_view;

      public:
        [[nodiscard]] constexpr byte_view() noexcept {}

        template <byte_viewable_range T>
        [[nodiscard]] constexpr byte_view(T &&range) noexcept
            // Using a C-style cast instead of a `reinterpret_cast` to make this constexpr when the pointer type already matches.
            : underlying_view((const char *)std::ranges::data(range), std::ranges::size(range) * sizeof(std::ranges::range_value_t<T>))
        {}

        [[nodiscard]] constexpr const char *data() const noexcept {return underlying_view.data();}
        [[nodiscard]] constexpr std::size_t size() const noexcept {return underlying_view.size();}
        [[nodiscard]] constexpr bool empty() const noexcept {return underlying_view.empty();}

        [[nodiscard]] constexpr const char *begin() const noexcept {return data();}
        [[nodiscard]] constexpr const char *end() const noexcept {return data() + size();}

        [[nodiscard]] constexpr operator std::span<const char>() const noexcept {return underlying_view;}
        [[nodiscard]] constexpr operator std::string_view() const noexcept {return underlying_view;}

        [[nodiscard]] constexpr const char &operator[](std::size_t i) const noexcept {return underlying_view[i];}
        [[nodiscard]] constexpr const char &front() const noexcept {return underlying_view.front();}
        [[nodiscard]] constexpr const char &back() const noexcept {return underlying_view.back();}
    };
}
