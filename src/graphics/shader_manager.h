#pragma once

#include "gpu/shader.h"
#include "utils/filesystem.h"

#include <fmt/format.h>

#include <set>
#include <string>
#include <vector>

namespace em::CommandLine
{
    class Parser;
}

namespace em::Gpu
{
    class Device;
}

namespace em::Graphics
{
    // A single shader managed by the shader manager.
    // Those are intended to be static variables.
    // They need to be constructed early, so our current example code only allows non-static shaders in the main game class, as opposed to the game state class that are constructed at runtime.
    // If this is non-static, it must normally be constructed before `ShaderManager` to ensure the correct destruction order (or manually destroy the `ShaderManager` early for the equivalent result).
    struct Shader
    {
        std::string name;
        Gpu::Shader::Stage stage{};
        std::string source;
        Gpu::Shader shader; // This one field is set lazily by `ShaderManager` when it loads the shaders.

        constexpr Shader() {}
        constexpr Shader(std::string name, Gpu::Shader::Stage stage, std::string source) : name(std::move(name)), stage(stage), source(std::move(source)) {}

        Shader(Shader &&) = default;
        Shader &operator=(Shader &&) = default;

        ~Shader();
    };

    // This is a base of `ShaderManager` that can't be constructed directly.
    // It restricts the interface to only be able to add new shaders, but not to touch any other state of the shader manager.
    class BasicShaderManager
    {
      protected:
        struct ShaderNameLess
        {
            static bool operator()(const Shader *a, const Shader *b)
            {
                return a->name < b->name;
            }
        };

        std::set<Shader *, ShaderNameLess> shaders;
        bool finalized = false;

        BasicShaderManager() {}

        // Move-only for simplicity.
        BasicShaderManager(BasicShaderManager &&) = default;
        BasicShaderManager &operator=(BasicShaderManager &&) = default;

      public:
        void AddShader(Shader &new_shader);
    };

    class ShaderManager : public BasicShaderManager
    {
        Gpu::Device *device = nullptr;

      public:
        // The directory where we look for shaders, and possibly place compiled ones if that's enabled.
        std::string dir = fmt::format("{}{}", Filesystem::GetResourceDir(), "assets/shaders");

        // The extra flags to pass to the shader compiler, if `CompileWhenFinalized()` is used.
        std::vector<std::string> glslc_flags = {"-O"};

        // Set to true to compile missing shaders when calling `Finalize()`, and delete unneeded ones.
        bool compile_when_finalized = false;

        constexpr ShaderManager() {}
        ShaderManager(Gpu::Device &device);

        // Move-only for simplicity.
        ShaderManager(ShaderManager &&) = default;
        ShaderManager &operator=(ShaderManager &&) = default;

        ~ShaderManager();

        void Finalize();

        void ProvidedCommandLineFlags(CommandLine::Parser &parser);
    };
}
