#pragma once

#include <SDL3/SDL_init.h>

namespace em::App
{
    enum class Action
    {
        cont = SDL_APP_CONTINUE, // This is always zero, it seems.
        exit_success = SDL_APP_SUCCESS,
        exit_failure = SDL_APP_FAILURE,
    };

    // The base class for the whole application and for the individual modules in it.
    struct Module
    {
        virtual ~Module() = default;

        virtual Action Tick() {return Action::cont;}
        virtual Action HandleEvent(SDL_Event &e) {(void)e; return Action::cont;}
    };
}
