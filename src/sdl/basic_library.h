#pragma once

#include "errors/critical_error.h"
#include "mainloop/module.h"

#include <fmt/format.h>
#include <SDL3/SDL_init.h>

namespace em::Modules
{
    class Sdl : public App::Module
    {
        bool initialized = false;

        CriticalErrorHandler error_handler;

        void InitLib();
        void ResetLib();

      public:
        Sdl() {}
        Sdl(const Sdl &) = delete;
        Sdl &operator=(const Sdl &) = delete;
        ~Sdl();

        App::Action Init() override;
        void Deinit(bool failure) override;
    };
}
