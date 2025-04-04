#include "shader.h"

#include "gpu/device.h"

#include <fmt/format.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>

namespace em::Gpu
{
    Shader::Shader(Device &device, zstring_view name, Stage stage, std::span<const unsigned char> spirv_binary)
        : Shader() // Ensure cleanup on throw.
    {
        SDL_ShaderCross_ShaderStage shadercross_stage{};
        switch (stage)
        {
          case Stage::vertex:
            shadercross_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
            break;
          case Stage::fragment:
            shadercross_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
            break;
          case Stage::compute:
            shadercross_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
            break;
          default:
            throw std::runtime_error("Invalid shader stage enum.");
        }

        SDL_ShaderCross_SPIRV_Info input{
            .bytecode = spirv_binary.data(),
            .bytecode_size = spirv_binary.size(),
            // This seems to be fixed?
            // `glslc` complains if `main` is missing. They have the `-fentry-point=X` flag, but that seems to be only for HLSL (according to `--help`,
            //   and experimentally it seems to have no effect on GLSL).
            .entrypoint = "main",
            .shader_stage = shadercross_stage,
            // We could add a way to override this. Should we?
            .enable_debug = device.DebugModeEnabled(),
            .name = name.empty() ? nullptr : name.c_str(),
            // None for now?
            .props = 0,
        };

        // Must set before creating the shader to let the destructor do its job if we throw later in this function.
        state.device = device.Handle();

        // The output parameter for this is not optional, so we must have this even if we're not going to use it.
        SDL_ShaderCross_GraphicsShaderMetadata output_metadata{};

        // This also outputs some reflection metadata. Do we need it?
        state.shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device.Handle(), &input, &output_metadata);
        if (!state.shader)
            throw std::runtime_error(fmt::format("Unable to compile SPIRV shader: {}", SDL_GetError()));
    }

    Shader::~Shader()
    {
        if (state.shader)
            SDL_ReleaseGPUShader(state.device, state.shader);
    }
}
