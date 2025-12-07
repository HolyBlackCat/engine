#pragma once

#include "em/refl/for_each_matching_elem.h"
#include "em/refl/for_each_matching_type.h"
#include "graphics/shader_manager.h"

#include <type_traits>

namespace em::Graphics::ShaderManagerRefl
{
    // This intentionally accepts any signature. We want a hard error on a wrong signature.
    // The function needs to be `static`, but we intentionally accept non-static ones here, and then hard error on them.
    template <typename T>
    concept NeedsShaders = requires{&std::remove_cvref_t<T>::NeededShaders;};

    struct NeedsShadersPred
    {
        template <typename T>
        struct type : std::bool_constant<NeedsShaders<T>> {};
    };

    // Calls `NeededShaders(shaders)` on every subobject of `object` that has it, if any.
    // The expected signature is `void NeededShaders(Graphics::BasicShaderManager &shaders)`.
    // This version allows the function to either be static or non-static.
    void AddNeededShaders(BasicShaderManager &shaders, auto &object)
    {
        em::Refl::ForEachElemMatchingPred<NeedsShadersPred>(object, [&shaders](auto &&member) -> void
        {
            // Catch non-void return types this way.
            return member.NeededShaders(shaders);
        });
    }

    // Calls `NeededShaders(shaders)` on every subobject of `object` that has it, if any.
    // The expected signature is `static void NeededShaders(Graphics::BasicShaderManager &shaders)`.
    // This version requires the function to be static.
    template <typename T>
    void AddNeededShadersStatic(BasicShaderManager &shaders)
    {
        em::Refl::ForEachTypeMatchingPred<T, NeedsShadersPred>([&shaders]<typename SubT> -> void
        {
            // Catch non-void return types this way.
            return std::remove_cvref_t<SubT>::NeededShaders(shaders);
        });
    }
}
