#pragma once


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
            bool debug_mode_enabled = false;
        };
        State state;

      public:
        constexpr Device() {}

        struct Params
        {
            // If true, on Windows fall back to a software Vulkan implementation that is shipped with Edge (and all other chrome-based browsers).
            bool fallback_to_software_rendering = true;
        };

        Device(const Params &params);

        Device(Device &&other) noexcept;
        Device &operator=(Device other) noexcept;
        ~Device();

        [[nodiscard]] explicit operator bool() const {return bool(state.device);}
        [[nodiscard]] SDL_GPUDevice *Handle() {return state.device;}

        [[nodiscard]] bool DebugModeEnabled() const {return state.debug_mode_enabled;}
    };
}
