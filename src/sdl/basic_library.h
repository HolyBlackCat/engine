#pragma once

#include "errors/critical_error.h"

#include <fmt/format.h>
#include <SDL3/SDL_init.h>

namespace em
{
    // A RAII wrapper for initializing SDL.
    // Also attaches an error handler to show errors as message boxes.
    class Sdl
    {
        struct State
        {
            bool initialized = false;

            CriticalErrorHandler error_handler;
        };
        State state;

      public:
        Sdl() {} // Constructs a null instance.
        Sdl(std::nullptr_t); // Actually initializes the library.

        Sdl(Sdl &&other) noexcept : state(std::move(other.state)) {state = {};}
        Sdl &operator=(Sdl other) noexcept {std::swap(state, other.state); return *this;}
        ~Sdl();
    };
}
