#pragma once

#include "em/math/vector.h"

#include <string_view>

typedef struct SDL_Window SDL_Window;

namespace em
{
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
            std::string_view name = "{name}"; // Use `{name}` and `{version}` to query the application properties passed to `em::Sdl`.
            ivec2 size = ivec2(1920, 1080) / 3;
            bool resizable = true;
        };

        Window(const Params &params);

        Window(Window &&other) noexcept;
        Window &operator=(Window other) noexcept;
        ~Window();
    };
}
