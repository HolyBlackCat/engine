#pragma once

#include <string_view>
#include <string>

namespace em::Strings
{
    [[nodiscard]] constexpr bool IsWhitespace(char ch)
    {
        // Intentionally not adding vertical whitespace here. Who uses that?
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }


    [[nodiscard]] constexpr bool AsciiIsUpper(char ch)
    {
        return ch >= 'A' && ch <= 'Z';
    }

    [[nodiscard]] constexpr bool AsciiIsLower(char ch)
    {
        return ch >= 'a' && ch <= 'z';
    }

    [[nodiscard]] constexpr bool AsciiIsAlpha(char ch)
    {
        return AsciiIsUpper(ch) || AsciiIsLower(ch);
    }

    [[nodiscard]] constexpr bool AsciiIsDigit(char ch)
    {
        return ch >= '0' && ch <= '9';
    }

    [[nodiscard]] constexpr bool AsciiIsAlnum(char ch)
    {
        return AsciiIsDigit(ch) || AsciiIsAlpha(ch);
    }

    // `...Strict` means that this rejects `$`.
    [[nodiscard]] constexpr bool IsNonDigitIdentifierCharStrict(char ch)
    {
        return AsciiIsAlpha(ch) || ch == '_';
    }

    // `...Strict` means that this rejects `$`.
    [[nodiscard]] constexpr bool IsIdentifierCharStrict(char ch)
    {
        return AsciiIsDigit(ch) || IsNonDigitIdentifierCharStrict(ch);
    }


    [[nodiscard]] constexpr char AsciiToUpper(char ch)
    {
        return AsciiIsLower(ch) ? ch - 'a' + 'A' : ch;
    }

    [[nodiscard]] constexpr char AsciiToLower(char ch)
    {
        return AsciiIsUpper(ch) ? ch - 'A' + 'a' : ch;
    }


    [[nodiscard]] inline std::string AsciiToUpper(std::string str)
    {
        for (char &ch : str)
            ch = AsciiToUpper(ch);
        return str;
    }

    [[nodiscard]] inline std::string AsciiToLower(std::string str)
    {
        for (char &ch : str)
            ch = AsciiToLower(ch);
        return str;
    }
}
