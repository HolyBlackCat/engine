#pragma once

#include "em/macros/utils/returns.h"
#include "em/meta/common.h"

#include <cassert>
#include <cstddef>
#include <ranges>
#include <span>
#include <type_traits>

namespace em
{
    namespace detail::ReinterpretSpan
    {
        template <typename T> struct IsStdSpan : std::false_type {};
        template <typename T, std::size_t N> struct IsStdSpan<std::span<T, N>> : std::true_type {};
    }

    // Takes a `std::span` and `reinterpret_cast`s it to have the element type `T`. The original constness is preserved.
    template <
        Meta::cvref_unqualified T,
        Meta::Deduce...,
        typename U, std::size_t N,
        typename Ret = std::span<
            Meta::copy_cv<U, T>,
            N == std::dynamic_extent
                ? std::dynamic_extent
                : N * sizeof(U) % sizeof(T) == 0
                    ? N * sizeof(U) / sizeof(T)
                    : throw "Unable to `reinterpret_span()` a fixed-size span because the input byte size is not a multiple of the target type size."
        >
    >
    [[nodiscard]] auto reinterpret_span(std::span<U, N> input) noexcept -> Ret
    {
        assert(input.size_bytes() % sizeof(T) == 0);
        return Ret(reinterpret_cast<typename Ret::value_type *>(input.data()), input.size_bytes() / sizeof(T));
    }

    // Same, but takes an arbitrary contiguous range and converts it to a span first.
    template <Meta::cvref_unqualified T, Meta::Deduce..., typename U>
    requires (!detail::ReinterpretSpan::IsStdSpan<std::remove_cvref_t<U>>::value && std::ranges::contiguous_range<U>)
    [[nodiscard]] auto reinterpret_span(U &&input) EM_RETURNS((reinterpret_span<T>)(std::span(input)))
}
