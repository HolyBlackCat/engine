#pragma once

#include "em/math/vector.h"

#include <string_view>

typedef struct SDL_Window SDL_Window;

namespace em
{
    namespace Gpu
    {
        class Device;
    }

    // One window. Maybe attached to a SDL GPU device, maybe not.
    class Window
    {
        struct State
        {
            SDL_Window *window = nullptr;
        };
        State state;

      public:
        constexpr Window() {}

        struct Params
        {
            Gpu::Device *gpu_device = nullptr; // GPU device, if this window uses the SDL GPU API. Otherwise leave null.
            std::string_view name = "{name}"; // Use `{name}` and `{version}` to query the application properties passed to `em::Sdl`.
            ivec2 size = ivec2(1920, 1080) / 3;
            bool resizable = true;
        };

        Window(const Params &params);

        Window(Window &&other) noexcept;
        Window &operator=(Window other) noexcept;
        ~Window();

        [[nodiscard]] explicit operator bool() const {return bool(state.window);}
        [[nodiscard]] SDL_Window *Handle() {return state.window;}
    };
}
