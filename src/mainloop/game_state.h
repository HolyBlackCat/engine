#pragma once

#include "command_line/parser_refl.h"
#include "command_line/parser.h"
#include "em/refl/macros/structs.h"
#include "em/refl/static_virtual.h"
#include "graphics/shader_manager_refl.h"
#include "graphics/shader_manager.h"

#include <concepts>

namespace em::App
{
    struct BasicState
    {
        EM_REFL(
            EM_STATIC_VIRTUAL(Interface, std::derived_from<_em_Derived, _em_Self>)
            (
                // Collect provided command line flags, if any.
                // Recursively calls `static void ProvidedCommandLineFlags(CommandLine::Parser &parser)` on every subobject.
                // This interface requires that function to be static.
                (AddProvidedCommandLineFlagsStatic, (CommandLine::Parser &parser) -> void)
                (
                    CommandLine::Refl::AddProvidedCommandLineFlagsStatic<_em_Derived>(parser);
                )

                // Collect provided command line flags, if any.
                // Recursively calls `static void ProvidedCommandLineFlags(Graphics::BasicShaderManager &parser)` on every subobject.
                // This interface requires that function to be static.
                (NeededShadersStatic, (Graphics::BasicShaderManager &shaders) -> void)
                (
                    Graphics::ShaderManagerRefl::AddNeededShadersStatic<_em_Derived>(shaders);
                )
            )
        )
    };
}
