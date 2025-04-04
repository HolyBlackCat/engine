#pragma once

typedef struct SDL_GPUDevice SDL_GPUDevice;
typedef struct SDL_GPUFence SDL_GPUFence;

namespace em::Gpu
{
    // This is like a `std::future`, it indicates when certain asynchronous things are done.
    // But it doesn't return any value by itself.
    class Fence
    {
        struct State
        {
            // Not a `Device *` to keep the address stable.
            SDL_GPUDevice *device = nullptr;
            SDL_GPUFence *fence = nullptr;
        };
        State state;

      public:
        constexpr Fence() {}

        struct TakeOwnershipOfExistingFence {explicit TakeOwnershipOfExistingFence() = default;};
        Fence(TakeOwnershipOfExistingFence, SDL_GPUDevice *device, SDL_GPUFence *fence);

        Fence(Fence &&other) noexcept;
        Fence &operator=(Fence other) noexcept;
        ~Fence();

        [[nodiscard]] explicit operator bool() const {return bool(state.fence);}
        [[nodiscard]] SDL_GPUFence *Handle() {return state.fence;}

        // Doesn't block, returns true if the fence is ready.
        [[nodiscard]] bool IsReady();

        // Blocks until the fence is ready. Throws on failure.
        // SDL also provides a way to wait for more than once fence (any or all of them), but we don't expose that yet.
        void Wait();
    };
}
