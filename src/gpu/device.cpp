#include "device.h"

#include "em/macros/meta/if_else.h"

#include <fmt/format.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <utility>

namespace em::Gpu
{
    Device::Device(std::nullptr_t)
        : Device() // Ensure cleanup on throw.
    {
        // If `EM_SDLGPU_DEBUG` is defined to 0 or 1, uses that value. Otherwise 1 if `NDEBUG` is not defined.
        bool debug_mode = EM_IS_TRUTHY(EM_TRUTHY_OR_FALLBACK(EM_SDLGPU_DEBUG)(EM_IS_FALSEY(EM_IS_EMPTY_OR_01(NDEBUG))));
        state.device = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), debug_mode, nullptr);
        if (!state.device)
            throw std::runtime_error(fmt::format("Unable to initialize the GPU device: {}", SDL_GetError()));
    }

    Device::Device(Device &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    Device &Device::operator=(Device other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    Device::~Device()
    {
        if (state.device)
            SDL_DestroyGPUDevice(state.device);
    }
}
