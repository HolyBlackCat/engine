#pragma once

#include "command_line/parser_refl.h"
#include "command_line/parser.h"
#include "em/refl/recursively_visit_elems_static.h"
#include "em/refl/macros/structs.h"
#include "em/refl/static_virtual.h"
#include "graphics/shader_manager.h"

#include <concepts>
#include <memory>

namespace em::App
{
    // This is a CRTP base used to create your own state base, which then doesn't need to be a template.
    // Inherit your states from that without CRTP.
    template <typename Derived>
    struct BasicState
    {
        EM_REFL(
            EM_STATIC_VIRTUAL(Interface, std::derived_from<_em_Derived, Derived>)
            (
                // Collect provided command line flags, if any.
                // Recursively calls `static void ProvidedCommandLineFlags(CommandLine::Parser &parser)` on every subobject.
                // This interface requires that function to be static.
                (AddProvidedCommandLineFlagsStatic, (CommandLine::Parser &parser) -> void)
                (
                    CommandLine::Refl::AddProvidedCommandLineFlagsStatic<_em_Derived>(parser);
                )

                // Collect provided command line flags, if any.
                // Recursively handles static variables of type `Graphics::Shader` in every subobject.
                // This interface requires those variables to be static.
                (NeededShadersStatic, (Graphics::BasicShaderManager &shaders) -> void)
                (
                    Refl::RecursivelyVisitStaticElemsOfTypeCvref<_em_Derived, Graphics::Shader &>([&](Graphics::Shader &sh){shaders.AddShader(sh);});
                )

                (Make, () -> std::unique_ptr<Derived>)
                (
                    return std::make_unique<_em_Derived>();
                )
            )
        )
    };
}
