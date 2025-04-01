#pragma once

#include <cstddef>

typedef struct SDL_GPUDevice SDL_GPUDevice;

namespace em::Gpu
{
    // This is attached to a window to render to it, or can be used for headless rendering. Presumably it can handle multiple windows at the same time.
    // In theory SDL lets you create this before or after the window, I believe? But our API requires this to be created first and then passed to the window,
    //   which makes sense, because multiple windows can share one GPU device.
    class Device
    {
        struct State
        {
            SDL_GPUDevice *device = nullptr;
        };
        State state;

      public:
        constexpr Device() {}

        Device(std::nullptr_t);

        Device(Device &&other) noexcept;
        Device &operator=(Device other) noexcept;
        ~Device();

        [[nodiscard]] explicit operator bool() const {return bool(state.device);}
        [[nodiscard]] SDL_GPUDevice *Handle() {return state.device;}
    };
}
