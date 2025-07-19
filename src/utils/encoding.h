#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <string>
#include <type_traits>

// Escaping/unescaping and converting strings between different encodings.

namespace em::Encoding
{
    // The [?] character that's used as a fallback on some errors.
    constexpr char32_t fallback_char = 0xfffd;

    template <typename T>
    concept CharType =
        std::same_as<T, char> || std::same_as<T, wchar_t> ||
        std::same_as<T, char8_t> || std::same_as<T, char16_t> || std::same_as<T, char32_t>;

    namespace low
    {
        // Returns true if `ch` is larger than allowed in Unicode.
        [[nodiscard]] constexpr bool CodepointIsTooLarge(char32_t ch) {return ch > 0x10ffff;}

        // Returns true if `ch` is a high surrogate (first element of a pair).
        [[nodiscard]] constexpr bool CodepointIsHighSurrotate(char32_t ch) {return ch >= 0xd800 && ch <= 0xdbff;}
        // Returns true if `ch` is a low surrogate (second element of a pair).
        [[nodiscard]] constexpr bool CodepointIsLowSurrotate(char32_t ch) {return ch >= 0xdc00 && ch <= 0xdfff;}
        // Returns true if `ch` is either element of a surrogate pair.
        [[nodiscard]] constexpr bool CodepointIsSurrotate(char32_t ch) {return CodepointIsHighSurrotate(ch) || CodepointIsLowSurrotate(ch);}

        // Returns true if `ch` is not a valid codepoint, either because it's too large or because it's reserved for surrogate pairs.
        [[nodiscard]] constexpr bool CodepointIsInvalid(char32_t ch) {return CodepointIsTooLarge(ch) || CodepointIsSurrotate(ch);}

        // Checks the codepoint as if by `CodepointIsInvalid(ch)`. Returns the error message on failure, or null on success.
        [[nodiscard]] constexpr const char *ValidateCodepoint(char32_t ch, bool check_surrogates)
        {
            if (CodepointIsTooLarge(ch))
                return "Invalid codepoint, larger than 0x10ffff.";
            if (check_surrogates && CodepointIsSurrotate(ch))
                return "Invalid codepoint, range 0xd800-0xdfff is reserved for surrogate pairs.";
            return nullptr;
        }

        // How many UTF-8 bytes are needed to encode this code point.
        // Returns 1..4 on success, 0 on failure. Allows surrogates, but doesn't allow values that are too large.
        [[nodiscard]] constexpr int CodepointUtf8ByteLength(char32_t ch)
        {
            if (CodepointIsTooLarge(ch))
                return 0;
            if (ch <= 0x7f)
                return 1;
            else if (ch <= 0x7ff)
                return 2;
            else if (ch <= 0xffff)
                return 3;
            else
                return 4;
        }

        // Encodes a single character to UTF-8 or UTF-16 or UTF-32. Returns the error message or null on success.
        // On failure writes nothing to the output.
        // If `encode` is true, the input is potentially a multicharacter "code point". This is a good default.
        // If `encode` is false, the input a "code unit", which is directly cast to the target type.
        // `check_surrogates` only makes sense if `encode` is true. If `check_surrogates` is true, rejects the values reserved for surrogate pairs.
        template <CharType OutChar>
        constexpr const char *EncodeOne(char32_t ch, bool encode, bool check_surrogates, std::basic_string<OutChar> &output)
        {
            using OutCharUnsigned = std::make_unsigned_t<OutChar>;

            if (encode)
            {
                if (auto error = ValidateCodepoint(ch, check_surrogates))
                    return error;

                if constexpr (sizeof(OutChar) >= 4)
                {
                    // UTF-32

                    output += (OutChar)ch;
                }
                else if constexpr (sizeof(OutChar) >= 2)
                {
                    // UTF-16

                    if (ch > 0xffff)
                    {
                        // A surrogate pair.
                        ch -= 0x10000;
                        output += 0xd800 + ((ch >> 10) & 0x3ff);
                        output += 0xdc00 + (ch & 0x3ff);
                    }
                    else
                    {
                        // A single character.
                        output += char16_t(ch);
                    }
                }
                else // sizeof(OutChar) == 1
                {
                    // UTF-8

                    int bytes = CodepointUtf8ByteLength(ch);
                    assert(bytes != 0); // Shouldn't be possible, as we checked for errors above.

                    if (bytes == 1)
                    {
                        output += OutChar(ch);
                    }
                    else if (bytes == 2)
                    {
                        output += OutChar(0b11000000 | (ch >> 6));
                        output += OutChar(0b10000000 | (ch & 0b00111111));
                    }
                    else if (bytes == 3)
                    {
                        output += OutChar(0b11100000 |  (ch >> 12));
                        output += OutChar(0b10000000 | ((ch >>  6) & 0b00111111));
                        output += OutChar(0b10000000 | ( ch        & 0b00111111));
                    }
                    else if (bytes == 4)
                    {
                        output += OutChar(0b11110000 |  (ch >> 18));
                        output += OutChar(0b10000000 | ((ch >> 12) & 0b00111111));
                        output += OutChar(0b10000000 | ((ch >> 6 ) & 0b00111111));
                        output += OutChar(0b10000000 | ( ch        & 0b00111111));
                    }
                }
            }
            else
            {
                // Not using `em/math/robust.h` to avoid the dependency.
                if ((char32_t)(OutCharUnsigned)ch != ch)
                    return "This value is not representable in the target character type.";

                output += (OutChar)(OutCharUnsigned)ch;
            }

            return nullptr;
        }

        // Like `EncodeOne`, but also escapes the character. Never fails.
        // If `encode == false`, always escapes the character. It is assumed to be a code unit rather than a codepoint.
        // If `force_simple_escape == true`, prepends a backslash to this character. You should use this for quotes (pass e.g. `ch == '"'`).
        template <CharType OutChar>
        constexpr void EncodeAndEscapeOne(char32_t ch, bool encode, bool force_simple_escape, std::basic_string<OutChar> &output)
        {
            // When should we unconditionally escape?
            // Note that `force_simple_escape` is handled in this branch, if those conditions are all false.
            if (!(
                // If `encode == false`, it means we're printing an invalid symbol and should always escape it.
                // Since this could be a part of an invalid symbol, by itself it might not be out of range, so we can't do any range checks for this.
                !encode ||
                // Control characters.
                // Don't need to test `ch >= 0` here because `char32_t` is an unsigned type.
                (ch < ' ') || ch == 0x7f ||
                // Backslashes.
                ch == '\\' ||
                // Too large or a surrogate.
                CodepointIsInvalid(ch)
            ))
            {
                if (force_simple_escape)
                    output += '\\';

                // A normal character, try to write it.
                // It shouldn't be possible for this to fail. But it if it does, we fall back to escaping.
                [[maybe_unused]] const char *error = EncodeOne(ch, encode, true, output);
                assert(!error);
                if (!error)
                    return;
            }

            // Don't use nice escapes for invalid codepoints.
            if (CodepointIsInvalid(ch))
                encode = false;

            // Nice named escapes, only if `encode == true`.
            if (encode)
            {
                switch (ch)
                {
                    // We don't output `\0` here because that can merge with following digits. Instead we emit `\u{0}` lower in this function.
                    case '\'': output += '\\'; output += '\''; return; // Not sure if this can ever be hit, as quotes go through `force_simple_escape`.
                    case '\"': output += '\\'; output += '"'; return; // Same.
                    case '\\': output += '\\'; output += '\\'; return;
                    case '\a': output += '\\'; output += 'a'; return;
                    case '\b': output += '\\'; output += 'b'; return;
                    case '\f': output += '\\'; output += 'f'; return;
                    case '\n': output += '\\'; output += 'n'; return;
                    case '\r': output += '\\'; output += 'r'; return;
                    case '\t': output += '\\'; output += 't'; return;
                    case '\v': output += '\\'; output += 'v'; return;
                }
            }

            // The numeric escape:

            // The syntax with braces is from C++23. Without braces the escapes could consume extra characters on the right.
            // Octal escapes don't do that, but they're just inherently ugly.
            static constexpr std::size_t buffer_size = 13; // \u{12345678} \0
            OutChar buffer[buffer_size];
            OutChar *cur = buffer + buffer_size; // We'll be writing in reverse, this makes it easier to create the number.
            *--cur = '\0';
            *--cur = '}';
            do
            {
                OutChar digit = OutChar(ch % 16);
                if (digit >= 10)
                    digit += 'a';
                else
                    digit += '0';
                *--cur = digit;
            }
            while (ch > 0);

            *--cur = '{';
            *--cur = encode ? 'u' : 'x';
            *--cur = '\\';

            output += cur;
        }

        // Decodes a single character from `source`. Returns the error message or null on success.
        // On failure `output_char` is still filled, but could be an invalid codepoint. `source` is advanced even on failure.
        // If `failed_because_of_surrogate` is specified, it's set to true on failure if the failure was caused by a lone surrogate in UTF-16 input.
        // When passing the result to `Encode{,AndEscape}One()`, set `encode = true` if this returned null, and to `false` if this returned an error.
        template <CharType InChar>
        [[nodiscard]] constexpr const char *DecodeOne(std::basic_string_view<InChar> &source, char32_t &output_char, bool *failed_because_of_surrogate = nullptr)
        {
            output_char = fallback_char;
            if (failed_because_of_surrogate)
                *failed_because_of_surrogate = false;

            if (source.empty())
                return "Unexpected end of string.";

            if constexpr (sizeof(InChar) >= 4)
            {
                output_char = (char32_t)source.front();
                source.remove_prefix(1);
            }
            else if constexpr (sizeof(InChar) >= 2)
            {
                if (CodepointIsLowSurrotate(source.front()))
                {
                    output_char = (char32_t)source.front();
                    source.remove_prefix(1);
                    if (failed_because_of_surrogate)
                        *failed_because_of_surrogate = true;
                    return "A lone low surrogate not preceded by a high surrogate.";
                }

                if (CodepointIsHighSurrotate(source.front()))
                {
                    if (source.size() >= 2 && CodepointIsLowSurrotate(source[1]))
                    {
                        output_char = char32_t((((char16_t)source[1] & 0x3ff) | ((char16_t)source[0] & 0x3ff) << 10) + char16_t(0x10000));
                        source.remove_prefix(2);
                        return nullptr;
                    }
                    else
                    {
                        output_char = (char32_t)source.front();
                        source.remove_prefix(1);
                        if (failed_because_of_surrogate)
                            *failed_because_of_surrogate = true;
                        return "A lone high surrogate not followed by a low surrogate.";
                    }
                }

                output_char = (char32_t)source.front();
                source.remove_prefix(1);
            }
            else // sizeof(InChar) == 1
            {
                int bytes = 0;
                if      ((source.front() & 0b10000000) == 0b00000000) bytes = 1; // Note the different bit pattern in this one.
                else if ((source.front() & 0b11100000) == 0b11000000) bytes = 2;
                else if ((source.front() & 0b11110000) == 0b11100000) bytes = 3;
                else if ((source.front() & 0b11111000) == 0b11110000) bytes = 4;

                if (bytes == 0)
                {
                    output_char = (char32_t)source.front();
                    source.remove_prefix(1);
                    return "This is not a valid first byte of a character for UTF-8.";
                }

                if (bytes == 1)
                {
                    output_char = (char32_t)source.front();
                    source.remove_prefix(1);
                    return nullptr;
                }

                // Extract bits from the first byte.
                output_char = (unsigned char)source.front() & (0xff >> bytes); // `bytes + 1` would have the same effect as `bytes`, but it's longer to type.

                // For each remaining byte...
                for (int i = 1; i < bytes; i++)
                {
                    // Stop if it's a first byte of some character, or if hitting the end of string.
                    if (i >= source.size() || (source[i] & 0b11000000) != 0b10000000)
                    {
                        output_char = (char32_t)source.front();
                        source.remove_prefix(1);
                        return "Incomplete multibyte UTF-8 character.";
                    }

                    // Extract bits and append them to the code.
                    output_char = output_char << 6 | ((unsigned char)source[i] & 0b00111111);
                }

                // Make sure the input uses the right amount of bytes for this character. If it doesn't, it's an error: https://en.wikipedia.org/wiki/UTF-8#Overlong_encodings
                if (bytes != CodepointUtf8ByteLength(output_char))
                {
                    output_char = (char32_t)source.front();
                    source.remove_prefix(1);
                    return "Overlong UTF-8 character encoding.";
                }

                source.remove_prefix(bytes);
            }

            // Not overwriting the output character if this fails, it could still be useful.
            return ValidateCodepoint(output_char, true);
        }

        // Decodes and unescapes a single character or escape sequence. Returns the error message or null on success.
        // If `failed_because_of_bad_encoding` is specified, it is set to true on error if the error is caused by a failure to decode a character,
        //   which is recoverable (one offending code unit is consumed from `source`). Otherwise the failure is caused by an invalid escape sequence,
        //   then `source` points to the offending character, and the error is not recoverable.
        // If `output_encode` is false, the `output_char` is a code unit rather than a code point,
        //   i.e. should be casted directly to the target type without encoding. See `EncodeOne()` for details.
        template <CharType InChar>
        [[nodiscard]] constexpr const char *DecodeAndUnescapeOne(std::basic_string_view<InChar> &source, char32_t &output_char, bool &output_encode, bool *failed_because_of_bad_encoding = nullptr)
        {
            // I know the output is unspecified on failure, but just in case.
            output_char = fallback_char;
            output_encode = true;
            if (failed_because_of_bad_encoding)
                *failed_because_of_bad_encoding = false;

            if (source.empty())
                return "Unexpected end of string.";

            if (source.front() != '\\')
            {
                // Not escaped.
                const char *error = DecodeOne(source, output_char);
                output_encode = !error; // User should ignore the output parameters on failure, but we're still doing the meaningful thing here.
                if (failed_because_of_bad_encoding && error)
                    *failed_because_of_bad_encoding = true;
                return error;
            }
            source.remove_prefix(1); // Remove the backslash.

            if (source.empty())
                return "Incomplete escape sequence at the end of string.";

            // Consumes digits for an escape sequence.
            // If `hex == true` those are hex digits, otherwise octal.
            // `max_digits` is how many digits we can consume, or `-1` to consume as many as possible,
            //   or `-2` to wait for a `}` (either way must consume at least one).
            // If `allow_less_digits == true` will allow less digits than expected, but at least one (only makes sense for a positive `max_digits`).
            // Writes the resulting number to `result`. Returns the error on failure, or an empty string on success.
            auto ConsumeDigits = [&] [[nodiscard]] (char32_t &result, bool hex, int max_digits, bool allow_less_digits = false) -> const char *
            {
                result = 0;

                int i = 0;
                while (true)
                {
                    bool is_digit = false;
                    bool is_decimal = false;
                    bool is_hex_lowercase = false;
                    bool is_hex_uppercase = false;

                    if (hex)
                    {
                        is_digit = !source.empty() && (
                            (is_decimal = source.front() >= '0' && source.front() <= '9') ||
                            (is_hex_lowercase = source.front() >= 'a' && source.front() <= 'f') ||
                            (is_hex_uppercase = source.front() >= 'A' && source.front() <= 'F')
                        );
                    }
                    else
                    {
                        is_decimal = true;
                        is_digit = !source.empty() && source.front() >= '0' && source.front() <= '7';
                    }

                    if (!is_digit)
                    {
                        if ((max_digits < 0 || allow_less_digits) && i > 0)
                            break;
                        else
                            return hex ? "Expected hexadecimal digit in escape sequence." : "Expected octal digit in escape sequence.";
                    }

                    result <<= hex ? 4 : 3;
                    if ((result >> (hex ? 4 : 3)) != result)
                        return "Overflow in escape sequence.";

                    result += char32_t(is_decimal ? source.front() - '0' : is_hex_lowercase ? source.front() - 'a' + 10 : source.front() - 'A' + 10);

                    source.remove_prefix(1);
                    i++;
                    if (i == max_digits)
                        break;
                }

                if (max_digits == -2)
                {
                    if (!source.starts_with('}'))
                        return "Expected closing `}` in the escape sequence.";
                    source.remove_prefix(1);
                }

                return nullptr;
            };

            output_encode = false;

            switch (source.front())
            {
                case 'N': source.remove_prefix(1); return "Named character escapes are not supported.";

                case '\'': output_char = '\''; source.remove_prefix(1); return nullptr;
                case '"':  output_char = '"';  source.remove_prefix(1); return nullptr;
                case '\\': output_char = '\\'; source.remove_prefix(1); return nullptr;
                case 'a':  output_char = '\a'; source.remove_prefix(1); return nullptr;
                case 'b':  output_char = '\b'; source.remove_prefix(1); return nullptr;
                case 'f':  output_char = '\f'; source.remove_prefix(1); return nullptr;
                case 'n':  output_char = '\n'; source.remove_prefix(1); return nullptr;
                case 'r':  output_char = '\r'; source.remove_prefix(1); return nullptr;
                case 't':  output_char = '\t'; source.remove_prefix(1); return nullptr;
                case 'v':  output_char = '\v'; source.remove_prefix(1); return nullptr;

              case 'o':
                {
                    source.remove_prefix(1);
                    if (!source.starts_with('{'))
                        return "Expected opening `{` in the escape sequence.";
                    source.remove_prefix(1);
                    if (auto error = ConsumeDigits(output_char, false, -2))
                        return error;
                }
                break;

              case 'x':
                {
                    source.remove_prefix(1);
                    if (source.starts_with('{'))
                    {
                        source.remove_prefix(1);
                        if (auto error = ConsumeDigits(output_char, true, -2))
                            return error;
                    }
                    else
                    {
                        if (auto error = ConsumeDigits(output_char, true, -1))
                            return error;
                    }
                }
                break;

              case 'u':
              case 'U':
                {
                    const bool braced = source.starts_with("u{");
                    output_encode = true;
                    if (braced)
                    {
                        source.remove_prefix(2);
                        if (auto error = ConsumeDigits(output_char, true, -2))
                            return error;
                    }
                    else
                    {
                        const int num_digits = source.starts_with('u') ? 4 : 8;
                        source.remove_prefix(1);
                        if (auto error = ConsumeDigits(output_char, true, num_digits))
                            return error;
                    }
                }
                break;

              default:
                source.remove_prefix(1);
                if (!source.empty() && source.front() >= '0' && source.front() <= '7')
                {
                    if (auto error = ConsumeDigits(output_char, false, 3, true))
                        return error;
                    break;
                }
                return "Invalid escape sequence.";
            }

            return nullptr;
        }
    }

    // Converts `source` to a different encoding, appends to `output`. Silently ignores encoding errors.
    template <CharType InChar, CharType OutChar>
    void ConvertRelaxed(std::basic_string_view<InChar> source, std::basic_string<OutChar> &output)
    {
        const InChar *cur = source.data();
        const InChar *const end = source.data() + source.size();

        while (cur != end)
        {
            char32_t ch = 0;
            if (low::DecodeOne(cur, end, ch))
                ch = fallback_char;
            (void)low::EncodeOne(ch, true, output); // No need to handle errors here.
        }
    }


    #if 0
    // Parses a double-quoted escaped string. Returns the error on failure or null on success.
    // Can write out-of-range characters to `output` due to escapes.
    // If `allow_prefix == true`, will silently ignore the literal prefix for this character type.
    template <CharType OutChar>
    [[nodiscard]] const char *ParseQuotedString(std::string_view &source, std::basic_string<OutChar> &output)
    {
        if (!source.starts_with('"'))
            return "Expected opening `\"`.";
        source.remove_prefix(1);

        while (!source.empty() && !source.starts_with('"'))
        {
            char32_t ch = 0;
            bool encode = true;
            const std::string_view old_source = source;
            if (const char *error = low::DecodeAndUnescapeOne(source, ch, encode))
                return error;

            if (const char *error = low::EncodeOne(ch, encode, output))
            {
                // Only roll back the pointer on encoding errors. This should point to the offending escape sequence.
                source = old_source;
                return error;
            }
        }

        if (!source.starts_with('"'))
            return "Expected closing `\"`.";
        source.remove_prefix(1);

        return nullptr;
    }

    // Parses a single-quoted escaped character. Returns the error on failure or null on success.
    // Can write an out-of-range character to `output` due to escapes.
    // If `allow_prefix == true`, will silently ignore the literal prefix for this character type.
    template <CharType OutChar>
    [[nodiscard]] std::string ParseQuotedChar(const char *&source, OutChar &output)
    {
        if (*source != '\'')
            return "Expected opening `'`.";
        source++;

        if (*source == '\'')
            return "Expected a character before the closing `'`.";

        const char *old_source = source;

        char32_t ch = 0;
        bool encode = true;
        if (const char *error = low::DecodeAndUnescapeOne(source, nullptr, ch, encode))
            return error;

        if (*source != '\'')
            return "Expected closing `'`.";

        std::basic_string<OutChar> buffer;
        if (const char *error = low::EncodeOne(ch, encode, buffer))
        {
            source = old_source;
            return error;
        }
        if (buffer.size() != 1)
        {
            source = old_source;
            return "This codepoint doesn't fit into a single character.";
        }

        output = buffer.front();

        source++;

        return "";
    }

    // Appends a quoted escaped string to `output`.
    // Silently ignores encoding errors in input, and tries to escape them.
    // If `add_prefix == true`, adds the proper literal prefix for this character type.
    template <CharType InChar>
    void MakeQuotedString(std::basic_string_view<InChar> source, char quote, bool add_prefix, std::string &output)
    {
        if (add_prefix)
            output += low::type_prefix<InChar>;

        output += quote;

        const InChar *cur = source.data();
        const InChar *const end = source.data() + source.size();

        while (cur != end)
        {
            char32_t ch = 0;
            bool fail = bool(low::DecodeOne(cur, end, ch));
            low::EncodeAndEscapeOne(ch, !fail, quote, output);
        }

        output += quote;
    }


    input:
        strict
        relaxed
        escaped strict
        escaped relaxed

    output:
        strict
        relaxed (can only be produced if the input is escaped?)
        escaped strict



    #error 1. replace char pointer refs with string_view refs
    #error 2. make everything constexpr (will need to get rid of std::snprintf for that)
    #error 3. port the tests over
    #error 4. does this deserve a separate library?

    #error test that we can produce invalid surrogates if needed? a separate reencode func?

    possible errors:
    A non-continuation byte (or the string ending) before the end of a character
    An overlong encoding (0xE0 followed by less than 0xA0, or 0xF0 followed by less than 0x90)
    A 4-byte sequence that decodes to a value greater than U+10FFFF (0xF4 followed by 0x90 or greater)
    #endif
}
