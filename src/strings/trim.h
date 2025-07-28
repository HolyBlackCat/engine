#pragma once

#include "em/meta/const_string.h"
#include "em/zstring_view.h"
#include "strings/char_types.h"
#include "strings/split.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <string>

namespace em::Strings
{
    constexpr void TrimLeadingWhitespace(std::string_view &input)
    {
        while (!input.empty() && IsWhitespace(input.front()))
            input.remove_prefix(1);
    }

    constexpr void TrimTrailingWhitespace(std::string_view &input)
    {
        while (!input.empty() && IsWhitespace(input.back()))
            input.remove_suffix(1);
    }

    constexpr void TrimLeadingEmptyLines(std::string_view &input)
    {
        const char *pos = nullptr;

        for (const char &ch : input)
        {
            if (ch == '\n')
                pos = &ch;
            else if (!IsWhitespace(ch))
                break;
        }

        if (pos)
            input.remove_prefix(std::size_t(pos - input.data() + 1));
    }

    // The output will contain a trailing newline if the input did as well, or if we removed at least one empty line.
    constexpr void TrimTrailingEmptyLines(std::string_view &input)
    {
        const char *pos = nullptr;

        const char *const begin = input.data();
        for (const char *cur = begin + input.size(); cur != begin;)
        {
            cur--;

            if (*cur == '\n')
                pos = cur;
            else if (!IsWhitespace(*cur))
                break;
        }

        if (pos)
            input.remove_suffix(input.size() - std::size_t(pos - input.data()) - 1);
    }

    // Removes all whitespace around the string, great for trimming up indented raw strings.
    // Prefer to use `""_compact` below for literals instead of calling this directly.
    // Trims leading and trailing whitespace-only lines, trims trailing whitespace on every line,
    //   and also trims the indentation common on all lines (ignoring the final empty line, if any).
    // Like with `TrimTrailingEmptyLines()`, the output will contain a trailing newline if the input did,
    //   or if we removed at least one whitespace-only line from the end.
    // By default throws on mixed space/tab indentation, but if you pass `mixed_indentation`, will write `true` to that instead of throwing,
    //   and still process the entire string.
    [[nodiscard]] constexpr std::string Compact(std::string_view input, bool *mixed_indentation = nullptr)
    {
        if (mixed_indentation)
            *mixed_indentation = false;

        TrimLeadingEmptyLines(input);
        TrimTrailingEmptyLines(input);

        char indent_ch = '\0';
        std::size_t min_indent = std::size_t(-1);

        // This ignores the trailing empty line, if any.
        std::size_t num_lines_ignoring_trailing_empty_line = 0;
        std::size_t num_chars = 0;

        // Figure out the longest indentation.
        Split(input, "\n", [&](std::string_view part)
        {
            // Remove the trailing whitespace to compute `num_chars` properly.
            TrimTrailingWhitespace(part);

            // This runs even for the trailing empty line, if any.
            num_chars += part.size() + 1; // Plus the line break.

            // Ignore the trailing empty line, if any.
            if (part.data() == input.data() + input.size())
                return;

            num_lines_ignoring_trailing_empty_line++;

            std::size_t this_indent = 0;

            while (!part.empty() && (part.front() == ' ' || part.front() == '\t'))
            {
                if (indent_ch == '\0')
                {
                    // Remember the first indentation character we see.
                    indent_ch = part.front();
                }
                else if (indent_ch != part.front())
                {
                    // Report mixed indentation, but if we're not throwing, still continue as usual.
                    if (mixed_indentation)
                        *mixed_indentation = true;
                    else
                        throw std::runtime_error("Mixed tabs and spaces in a string passed to `em::Strings::Compact()`.");
                }

                this_indent++;
                part.remove_prefix(1);
            }

            if (this_indent < min_indent)
                min_indent = this_indent;
        });

        // Compute the resulting string length.
        num_chars--; // Minus one newline.
        num_chars -= num_lines_ignoring_trailing_empty_line * min_indent;

        std::string ret;
        ret.reserve(num_chars);

        // Write the characters.
        bool first = true;
        Split(input, "\n", [&](std::string_view part)
        {
            if (first)
                first = false;
            else
                ret += '\n';

            // This part runs for all lines except the last empty one, if any.
            if (part.data() != input.data() + input.size())
            {
                TrimTrailingWhitespace(part);
                ret += part.substr(min_indent);
            }
        });

        // Did we compute the final length correctly?
        assert(ret.size() == num_chars);

        return ret;
    }
}

namespace em
{
    namespace Strings::detail
    {
        // This should be a static variable in `operator""_compact`, but that's bugged in Clang: https://github.com/llvm/llvm-project/issues/150851
        template <Meta::ConstString S>
        constexpr auto compact_storage = []{
            Meta::ConstString<Strings::Compact(S.view()).size() + 1> ret;
            std::string result = Strings::Compact(S.view());
            std::copy_n(result.data(), result.size(), ret.str);
            return ret;
        }();
    }

    template <Meta::ConstString S>
    [[nodiscard]] consteval zstring_view operator""_compact()
    {
        return Strings::detail::compact_storage<S>.str;
    }
}
