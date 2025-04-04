#include "copy_pass.h"

#include "gpu/command_buffer.h"

#include <fmt/format.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <utility>

namespace em::Gpu
{
    CopyPass::CopyPass(CommandBuffer &command_buffer)
        : CopyPass() // Ensure cleanup on throw.
    {
        state.pass = SDL_BeginGPUCopyPass(command_buffer.Handle());
        if (!state.pass)
            throw std::runtime_error(fmt::format("Unable to begin a GPU copy pass: {}", SDL_GetError()));
    }

    CopyPass::CopyPass(CopyPass &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    CopyPass &CopyPass::operator=(CopyPass other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    CopyPass::~CopyPass()
    {
        if (state.pass)
            SDL_EndGPUCopyPass(state.pass);
    }
}
