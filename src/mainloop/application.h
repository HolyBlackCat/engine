#pragma once

#include <SDL3/SDL_init.h>

namespace em
{
    // The base class for the whole application and for the individual modules in it.
    struct AppModule
    {
        virtual ~AppModule() = default;

        enum class AppAction
        {
            cont = SDL_APP_CONTINUE, // This is always zero, it seems.
            exit_success = SDL_APP_SUCCESS,
            exit_failure = SDL_APP_FAILURE,
        };

        virtual AppAction Init() {return AppAction::exit_success;}
        virtual AppAction Tick() {return AppAction::cont;}
        virtual AppAction HandleEvent(SDL_Event &e) {(void)e; return AppAction::cont;}
        virtual void Deinit(bool failure) {(void)failure;}
    };
}
