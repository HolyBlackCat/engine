#include "fence.h"

#include <fmt/format.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <utility>

namespace em::Gpu
{
    Fence::Fence(TakeOwnershipOfExistingFence, SDL_GPUDevice *device, SDL_GPUFence *fence)
        : Fence() // Ensure cleanup on throw.
    {
        state.device = device;
        state.fence = fence;
    }

    Fence::Fence(Fence &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    Fence &Fence::operator=(Fence other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    Fence::~Fence()
    {
        if (state.fence)
            SDL_ReleaseGPUFence(state.device, state.fence);
    }

    bool Fence::IsReady()
    {
        return SDL_QueryGPUFence(state.device, state.fence);
    }

    void Fence::Wait()
    {
        if (!SDL_WaitForGPUFences(state.device, false, &state.fence, 1))
            throw std::runtime_error(fmt::format("Unable to wait for a GPU fence: {}", SDL_GetError()));
    }
}
