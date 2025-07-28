#pragma once

#include "em/meta/void.h"

#include <functional>
#include <string_view>

namespace em::Strings
{
    // Splits `input` by separator `sep`, and calls `func` (which is `(std::string_view part) -> ??`) on every part.
    // Always calls `func` at least once, and calls it even on empty segments.
    // If `func` returns non-void, propagates the return value from it, stopping the loop when it returns truthy.
    [[nodiscard]] constexpr decltype(auto) Split(std::string_view input, std::string_view sep, auto &&func)
    {
        while (true)
        {
            auto pos = input.find(sep);

            if (pos == std::string_view::npos)
                return std::invoke(func, auto(input));

            if (decltype(auto) ret = Meta::InvokeWithVoidPlaceholderResult(func, input.substr(0, pos)); Meta::VoidTo<false>(ret))
                EM_RETURN_VARIABLE(ret);
            input.remove_prefix(pos + sep.size());
        }
    }
}
