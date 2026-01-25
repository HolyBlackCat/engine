#pragma once

#include "em/refl/recursively_visit_elems.h"
#include "em/refl/recursively_visit_types.h"
#include "command_line/parser.h"

#include <type_traits>

// This is an interface to parse command line arguments using reflection.
// Usage:
// * Add `[static] void ProvidedCommandLineFlags(CommandLine::Parser &parser) {parser.AddFlag(...);}` to some of your member objects.
// * Call `CommandLine::Refl::AddProvidedCommandLineFlags[Static]()` defined below to collect the flags into a parser.
// * Call `parser.Parse(...);`.
// If you use the `[Static]` overloads, the callbacks must be static, and you'll get hard errors otherwise.
// If you don't, then both static and non-static versions work.


namespace em::CommandLine::Refl
{
    // This intentionally accepts any signature. We want a hard error on a wrong signature.
    // This intentionally accepts both static and non-static functions. Some usages require them to be static, see below.
    template <typename T>
    concept ProvidesCommandLineFlags = requires{&std::remove_cvref_t<T>::ProvidedCommandLineFlags;};

    struct ProvidesCommandLineFlagsPred
    {
        template <typename T>
        struct type : std::bool_constant<ProvidesCommandLineFlags<T>> {};
    };

    // Calls `ProvidedCommandLineFlags(parser)` on every subobject of `object` that has it, if any.
    // The expected signature is `void ProvidedCommandLineFlags(CommandLine::Parser &parser)`.
    // This version allows the function to either be static or non-static.
    void AddProvidedCommandLineFlags(Parser &parser, auto &object)
    {
        em::Refl::RecursivelyVisitElemsMatchingPred<ProvidesCommandLineFlagsPred>(object, [&parser](auto &&member) -> void
        {
            // Catch non-void return types this way.
            return member.ProvidedCommandLineFlags(parser);
        });
    }

    // Calls a static `ProvidedCommandLineFlags(parser)` on every sub-type of `object` that has it, if any.
    // The expected signature is `void ProvidedCommandLineFlags(CommandLine::Parser &parser)`.
    // This version requires the function to be static, and triggers a hard error otherwise, which is intended.
    template <typename T>
    void AddProvidedCommandLineFlagsStatic(Parser &parser)
    {
        em::Refl::RecursivelyVisitTypesMatchingPred<T, ProvidesCommandLineFlagsPred>([&parser]<typename SubT> -> void
        {
            // Catch non-void return types this way.
            return std::remove_cvref_t<SubT>::ProvidedCommandLineFlags(parser);
        });
    }
}
