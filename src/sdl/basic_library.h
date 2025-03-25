#pragma once

#include "em/zstring_view.h"
#include "errors/critical_error.h"

#include <fmt/format.h>

namespace em
{
    struct AppMetadata
    {
        // All of those can be empty to use the defaults.
        // Adding initializers to signal to Clang that partial designated init shouldn't cause a warning.

        zstring_view name{};
        zstring_view version{};
        zstring_view identifier{};
        zstring_view author{};
        zstring_view copyright{};
        zstring_view url{};
        zstring_view type = "game"; // SDL says this should be one of: `game`, `application` (default), `mediaplayer`. See https://wiki.libsdl.org/SDL3/SDL_SetAppMetadataProperty
    };

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
        Sdl(const AppMetadata &metadata); // Actually initializes the library.

        Sdl(Sdl &&other) noexcept : state(std::move(other.state)) {state = {};}
        Sdl &operator=(Sdl other) noexcept {std::swap(state, other.state); return *this;}
        ~Sdl();
    };
}
