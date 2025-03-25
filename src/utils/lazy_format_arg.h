#pragma once

#include "em/macros/utils/forward.h"

#include <fmt/format.h>

#include <optional>
#include <utility>

namespace em
{
    // Takes a lambda. Formatting this with `fmt::format()` calls the lambda and formats the result.
    template <typename F>
    class LazyFormatArg
    {
        // Libfmt appears to want const values to be formattable too (maybe because we inherit from another formatter, not sure),
        //   so we need to make the conversion operator const, so `result` has to be mutable, and this should be mutable too so we can forward
        //   the functor when calling it (which only happens once).
        mutable F functor;

      public:
        using type = decltype(std::declval<F>()()); // The value type.

      private:
        // Caches the value in case it's somehow formatted multiple times.
        // Also this lets us not worry about dangling.
        mutable std::optional<type> result;

      public:
        constexpr LazyFormatArg(auto &&functor) : functor(EM_FWD(functor)) {}

        // Lazy evaluation.
        [[nodiscard]] constexpr operator const type &() const
        {
            if (!result)
                result = EM_FWD_EX(functor)();

            return *result;
        }
    };

    template <typename T>
    LazyFormatArg(T) -> LazyFormatArg<T>;
}

template <typename F, typename C>
struct fmt::formatter<em::LazyFormatArg<F>, C> : fmt::formatter<std::remove_cvref_t<typename em::LazyFormatArg<F>::type>, C> {};
