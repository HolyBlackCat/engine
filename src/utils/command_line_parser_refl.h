#pragma once

#include "em/refl/for_each_matching_elem.h"
#include "utils/command_line_parser.h"

#include <type_traits>

namespace em::CommandLine
{
    template <typename T>
    concept ProvidesCommandLineArgs = std::is_member_function_pointer_v<decltype(&std::remove_cvref_t<T>::ProvidedCommandLineArgs)>;

    struct ProvidesCommandLineArgsPred
    {
        template <typename T>
        struct type : std::bool_constant<ProvidesCommandLineArgs<T>> {};
    };

    // Calls `ProvidedCommandLineArgs(parser)` on every subobject of `object` that has it, if any.
    // The expected signature is `void ProvidedCommandLineArgs(CommandLine::Parser &parser)`.
    void ReflectFlags(Parser &parser, auto &object)
    {
        Refl::ForEachElemMatchingPred<ProvidesCommandLineArgsPred>(object, [&](auto &&member) -> void
        {
            // Catch non-void return types this way.
            return member.ProvidedCommandLineArgs(parser);
        });
    }
}
