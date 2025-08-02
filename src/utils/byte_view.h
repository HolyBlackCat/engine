#pragma once

#include "em/meta/common.h"
#include "em/macros/utils/implies.h"

#include <cassert>
#include <cstddef>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>

// `em::const_byte_view` and `em::mut_byte_view` are basically `std::span<[const] char>`, but can also bind to ranges of other types (trivially copyable ones),
//   and have helper functions to reinterpret them back into ranges of other types.
// It's constexpr as long as it's only converted to/from ranges of `char`.
// For that reason, we use `char` as the element type (instead of e.g. `unsigned char`) to allow it to be `constexpr` when bound to `char` ranges,
//   since `reinterpret_cast` isn't constexpr.

namespace em
{
    template <bool IsConst>
    class maybe_const_byte_view;

    using const_byte_view = maybe_const_byte_view<true>;
    using mut_byte_view = maybe_const_byte_view<false>;

    // Is this a range that `byte_view` can be constructed from?
    // Note that we explicitly ban string literals here (and const char arrays in general), since they have a trailing null,
    //   which we really don't want to count in the view's `.size()`. Cast your literals to `std::string_view` or `std::span`.
    // This should ignore cvref (apart from the string literal check, which only triggers on const, see below).
    template <typename T, bool IsConst>
    concept maybe_const_byte_viewable_range =
        !Meta::same_ignoring_cvref<T, maybe_const_byte_view<IsConst>> &&
        std::ranges::contiguous_range<T> &&
        std::ranges::sized_range<T> &&
        std::is_trivially_copyable_v<std::ranges::range_value_t<T>> &&
        // Explicitly opt out of string literals, see above.
        !Meta::possibly_string_literal_cvref<T> &&
        // If this is a non-const range, ensure the element
        (!IsConst EM_IMPLIES !std::ranges::constant_range<T>);

    template <typename T> concept const_byte_viewable_range = maybe_const_byte_viewable_range<T, true>;
    template <typename T> concept mut_byte_viewable_range = maybe_const_byte_viewable_range<T, false>;

    // Like `maybe_const_byte_viewable_range`, but checks an iterator instead of a range. Cvref is ignored.
    // You can pass the same type to both `Iter` and `Sentinel` if your end iterator (sentinel) has the same type as the first iterator.
    template <typename Iter, typename Sentinel, bool IsConst>
    concept maybe_const_byte_viewable_iter = maybe_const_byte_viewable_range<std::ranges::subrange<std::remove_cvref_t<Iter>, std::remove_cvref_t<Sentinel>>, IsConst>;

    template <typename Iter, typename Sentinel> concept const_byte_viewable_iter = maybe_const_byte_viewable_iter<Iter, Sentinel, true>;
    template <typename Iter, typename Sentinel> concept mut_byte_viewable_iter = maybe_const_byte_viewable_iter<Iter, Sentinel, false>;

    // `byte_view` can be reinterpreted as a span of those.
    template <typename T, bool IsConst>
    concept maybe_const_byte_view_reinterpretable_as_range_of =
        !Meta::reference<T> &&
        std::is_trivially_copyable_v<T> &&
        (IsConst EM_IMPLIES std::is_const_v<T>);

    template <typename T> concept const_byte_view_reinterpretable_as_range_of = maybe_const_byte_view_reinterpretable_as_range_of<T, true>;
    template <typename T> concept mut_byte_view_reinterpretable_as_range_of = maybe_const_byte_view_reinterpretable_as_range_of<T, false>;

    template <bool IsConst>
    class maybe_const_byte_view
    {
        using MaybeConstChar = Meta::maybe_const<IsConst, char>;

        // Using `char` instead of `unsigned char` to be able to use this at compile-time for `char` ranges (strings),
        //   since `reinterpret_cast` isn't constexpr.
        MaybeConstChar *data_ptr = nullptr;
        std::size_t data_size = 0;

      public:
        [[nodiscard]] constexpr maybe_const_byte_view() noexcept {}

        // From a range.
        template <maybe_const_byte_viewable_range<IsConst> T>
        [[nodiscard]] constexpr maybe_const_byte_view(T &&range) noexcept
            // Using a C-style cast instead of a `reinterpret_cast` to make this constexpr when the pointer type already matches.
            : data_ptr((MaybeConstChar *)std::ranges::data(range)), data_size(std::ranges::size(range) * sizeof(std::ranges::range_value_t<T>))
        {}

        // From two iterators.
        template <typename Iter, typename Sentinel = Iter>
        requires maybe_const_byte_viewable_iter<Iter, Sentinel, IsConst>
        [[nodiscard]] constexpr maybe_const_byte_view(Iter &&iter, Sentinel &&sentinel) noexcept
            : maybe_const_byte_view(std::ranges::subrange(EM_FWD(iter), EM_FWD(sentinel)))
        {}

        // From iterator and size.
        template <typename Iter>
        requires maybe_const_byte_viewable_iter<Iter, Iter, IsConst>
        [[nodiscard]] constexpr maybe_const_byte_view(Iter &&iter, std::ptrdiff_t size) noexcept
            : maybe_const_byte_view(std::ranges::subrange(iter, iter + size))
        {}

        // We don't propagate constness here.

        [[nodiscard]] constexpr MaybeConstChar *data() const noexcept {return data_ptr;}
        [[nodiscard]] constexpr std::size_t size() const noexcept {return data_size;}
        [[nodiscard]] constexpr bool empty() const noexcept {return data_size == 0;}

        [[nodiscard]] constexpr MaybeConstChar *begin() const noexcept {return data_ptr;}
        [[nodiscard]] constexpr MaybeConstChar *end() const noexcept {return data_ptr + data_size;}

        [[nodiscard]] constexpr MaybeConstChar &operator[](std::size_t i) const noexcept {assert(i < size()); return data_ptr[i];}
        [[nodiscard]] constexpr MaybeConstChar &front() const noexcept {assert(!empty()); return *data_ptr;}
        [[nodiscard]] constexpr MaybeConstChar &back() const noexcept {assert(!empty()); return data_ptr[data_size - 1];}

        [[nodiscard]] constexpr std::string_view AsStringView() const noexcept {return std::string_view(data_ptr, data_size);}

        template <maybe_const_byte_view_reinterpretable_as_range_of<IsConst> T>
        [[nodiscard]] constexpr std::span<T> AsRangeOf() const noexcept
        {
            assert(size() % sizeof(T) == 0); // Check that the current size is a multiple of the target type size.

            // Using a C-style cast instead of a `reinterpret_cast` to make this constexpr when the pointer type already matches.
            return std::span<T>((T *)data(), size() / sizeof(T));
        }
    };
}
