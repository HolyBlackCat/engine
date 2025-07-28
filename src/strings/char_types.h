#pragma once

namespace em::Strings
{
    [[nodiscard]] constexpr bool IsWhitespace(char ch)
    {
        // Intentionally not adding vertical whitespace here. Who uses that?
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }
}
