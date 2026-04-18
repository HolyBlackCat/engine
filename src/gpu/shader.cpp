#include "shader.h"

#include "em/macros/utils/finally.h"
#include "gpu/command_buffer.h"
#include "gpu/device.h"
#include "gpu/render_pass.h"
#include "gpu/sampler.h"
#include "sdl/properties.h"

#include <fmt/format.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>

namespace em::Gpu
{
    Shader::Shader(Device &device, zstring_view name, Stage stage, const_byte_view spirv_binary)
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
          default:
            throw std::logic_error("Invalid shader stage enum.");
        }

        SdlProperties props;
        props.Set(SDL_SHADERCROSS_PROP_SHADER_DEBUG_ENABLE_BOOLEAN, device.DebugModeEnabled());
        props.Set(SDL_SHADERCROSS_PROP_SHADER_DEBUG_NAME_STRING, name);

        SDL_ShaderCross_SPIRV_Info input{
            .bytecode = reinterpret_cast<const unsigned char *>(spirv_binary.data()),
            .bytecode_size = spirv_binary.size(),
            // This seems to be fixed?
            // `glslc` complains if `main` is missing. They have the `-fentry-point=X` flag, but that seems to be only for HLSL (according to `--help`,
            //   and experimentally it seems to have no effect on GLSL).
            .entrypoint = "main",
            .shader_stage = shadercross_stage,
            // We could add a way to override this. Should we?
            // None for now?
            .props = props.Handle(),
        };

        // Produce the reflection metadata needed by `SDL_ShaderCross_CompileGraphicsShaderFromSPIRV()`.
        SDL_ShaderCross_GraphicsShaderMetadata *reflected_metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(input.bytecode, input.bytecode_size, 0);
        if (!reflected_metadata)
            throw std::runtime_error(fmt::format("Unable to reflect SPIRV shader: {}", SDL_GetError()));
        EM_FINALLY{ SDL_free(reflected_metadata); };

        // Must set before creating the shader to let the destructor do its job if we throw later in this function.
        state.device = device.Handle();

        state.shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device.Handle(), &input, &reflected_metadata->resource_info, 0);
        if (!state.shader)
            throw std::runtime_error(fmt::format("Unable to compile SPIRV shader: {}", SDL_GetError()));
    }

    Shader::Shader(Shader &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    Shader &Shader::operator=(Shader other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    Shader::~Shader()
    {
        if (state.shader)
            SDL_ReleaseGPUShader(state.device, state.shader);
    }

    void Shader::SetUniformBytes(CommandBuffer &cmdbuf, Stage stage, std::uint32_t slot, const_byte_view bytes)
    {
        // None of those functions can fail.
        switch (stage)
        {
          case Stage::vertex:
            SDL_PushGPUVertexUniformData(cmdbuf.Handle(), slot, bytes.data(), std::uint32_t(bytes.size()));
            return;
          case Stage::fragment:
            SDL_PushGPUFragmentUniformData(cmdbuf.Handle(), slot, bytes.data(), std::uint32_t(bytes.size()));
            return;
        }

        throw std::logic_error("Invalid shader stage enum.");
    }

    void Shader::BindTextures(RenderPass &render_pass, std::span<const TextureAndSampler> textures, Shader::Stage shader_stage, std::uint32_t first_slot)
    {
        std::vector<SDL_GPUTextureSamplerBinding> sdl_textures;
        sdl_textures.reserve(textures.size());

        for (const TextureAndSampler &texture : textures)
        {
            sdl_textures.push_back({
                .texture = texture.texture->Handle(),
                .sampler = texture.sampler->Handle(),
            });
        }

        // Those functions can't fail.
        switch (shader_stage)
        {
          case Shader::Stage::vertex:
            SDL_BindGPUVertexSamplers(render_pass.Handle(), first_slot, sdl_textures.data(), std::uint32_t(sdl_textures.size()));
            break;
          case Shader::Stage::fragment:
            SDL_BindGPUFragmentSamplers(render_pass.Handle(), first_slot, sdl_textures.data(), std::uint32_t(sdl_textures.size()));
            break;
          default:
            throw std::logic_error("Invalid shader stage enum.");
        }
    }
}
