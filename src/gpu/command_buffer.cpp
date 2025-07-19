#include "command_buffer.h"

#include "gpu/device.h"
#include "gpu/fence.h"
#include "sdl/window.h"

#include <fmt/format.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <utility>

namespace em::Gpu
{
    CommandBuffer::CommandBuffer(Device &device, Fence *output_fence)
        : CommandBuffer() // Ensure cleanup on throw.
    {
        state.output_fence = output_fence;
        state.num_active_exceptions = std::uncaught_exceptions();

        state.buffer = SDL_AcquireGPUCommandBuffer(device.Handle());
        if (!state.buffer)
            throw std::runtime_error(fmt::format("Unable to acquire a GPU command buffer: {}", SDL_GetError()));
    }

    CommandBuffer::CommandBuffer(CommandBuffer &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    CommandBuffer &CommandBuffer::operator=(CommandBuffer other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    CommandBuffer::~CommandBuffer() noexcept(false)
    {
        if (*this)
        {
            if (state.cancel_when_destroyed || state.num_active_exceptions < std::uncaught_exceptions())
            {
                if (!SDL_CancelGPUCommandBuffer(state.buffer))
                    throw std::runtime_error(fmt::format("Unable to cancel a GPU command buffer: {}", SDL_GetError()));
            }
            else
            {
                if (state.output_fence)
                {
                    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(state.buffer);
                    if (!fence)
                        throw std::runtime_error(fmt::format("Unable to submit a GPU command buffer and acquire a fence: {}", SDL_GetError()));
                    *state.output_fence = Fence(Fence::TakeOwnershipOfExistingFence{}, state.device, fence);
                }
                else
                {
                    if (!SDL_SubmitGPUCommandBuffer(state.buffer))
                        throw std::runtime_error(fmt::format("Unable to submit a GPU command buffer: {}", SDL_GetError()));
                }
            }
        }
    }

    void CommandBuffer::CancelWhenDestroyed()
    {
        if (*this)
            state.cancel_when_destroyed = true;
    }

    Texture CommandBuffer::WaitAndAcquireSwapchainTexture(Window &window)
    {
        // Note that this never needs freeing, so we don't really care what the value is on failure.
        SDL_GPUTexture *texture = nullptr;

        vec2<Uint32> size;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(state.buffer, window.Handle(), &texture, &size.x, &size.y))
            throw std::runtime_error(fmt::format("Unable to acquire a GPU swapchain texture: {}", SDL_GetError()));

        return Texture(Texture::ViewExternalHandle{}, state.device, texture, size.to<int>().to_vec3(1), window.GetSwapchainTextureFormat());
    }
}
