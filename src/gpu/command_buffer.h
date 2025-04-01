#pragma once

#include "gpu/texture.h"

typedef struct SDL_GPUDevice SDL_GPUDevice;
typedef struct SDL_GPUCommandBuffer SDL_GPUCommandBuffer;

namespace em
{
    class Window;
}

namespace em::Gpu
{
    class Device;

    // All rendering starts here. You need a new one every frame.
    // Each thread needs its own buffer, and you can have more than one buffer per thread.
    // The commands are executed when you call `.Finish()`.
    class CommandBuffer
    {
        struct State
        {
            // Storing this instead of `Device *` for pointer stability.
            // Need this at least for acquiring the swapchain texture.
            SDL_GPUDevice *device = nullptr;

            SDL_GPUCommandBuffer *buffer = nullptr;

            bool cancel_when_destroyed = false;
            int num_active_exceptions = 0;
        };
        State state;

      public:
        constexpr CommandBuffer() {}

        CommandBuffer(Device &device);

        CommandBuffer(CommandBuffer &&other) noexcept;
        CommandBuffer &operator=(CommandBuffer other) noexcept;

        // Normally executes the buffer. But if the user has called `CancelWhenDestroyed()` before,
        //   OR if this is being called due to an exception, cancels it instead.
        // Both cancelling and submitting can throw (apparently cancelling might throw if you already acquired the swapchain texture on this).
        // Does NOT block to wait for the buffer to finish executing.
        ~CommandBuffer() noexcept(false);

        [[nodiscard]] explicit operator bool() const {return bool(state.buffer);}
        [[nodiscard]] SDL_GPUCommandBuffer *Handle() {return state.buffer;}


        // Sets a flat to cancel on destruction instead of executing.
        // SDL docs say that it's an error to do this after acquiring the swapchain texture.
        void CancelWhenDestroyed();


        // Get a temporary texture that represents the window.
        // Blocks if there are too many frames in flight. There's a version that doesn't block and returns null, but we don't expose that at the moment.
        // CAN RETURN NULL if the window is minimized. Don't render anything in that case.
        // If this return null, you should probably call `CancelWhenDestroyed()` and then do nothing else.
        // The docs say you can't cancel after acquiring the texture, but so far my understanding is that it applies only to SUCCESSFULLY acquiring it.
        // This seems to never return null for me on Linux on XFCE, but it does return null in Wine, and there cancelling works fine.
        [[nodiscard]] Texture WaitAndAcquireSwapchainTexture(Window &window);
    };
}
