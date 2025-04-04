#pragma once

#include "em/zstring_view.h"

#include <span>


typedef struct SDL_GPUDevice SDL_GPUDevice;
typedef struct SDL_GPUShader SDL_GPUShader;

namespace em::Gpu
{
    class Device;

    // A texture.
    class Shader
    {
        struct State
        {
            // Need this to delete shaders.
            // This can't be `Device *` to keep the address stable.
            SDL_GPUDevice *device = nullptr;
            SDL_GPUShader *shader = nullptr;
        };
        State state;

      public:
        constexpr Shader() {}

        enum class Stage
        {
            vertex,
            fragment,
            compute,
        };

        // The name is optional.
        Shader(Device &device, zstring_view name, Stage stage, std::span<const unsigned char> spirv_binary);

        Shader(Shader &&other) noexcept;
        Shader &operator=(Shader other) noexcept;
        ~Shader();

        [[nodiscard]] explicit operator bool() const {return bool(state.shader);}
        [[nodiscard]] SDL_GPUShader *Handle() {return state.shader;}
    };
}
