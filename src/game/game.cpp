#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "sdl/basic_library.h"
#include "em/refl/macros/structs.h"

#include <iostream>
#include <memory>

struct GameApp : em::App::Module
{
    EM_REFL(
        (em::Sdl)(sdl, nullptr)
    )

    em::App::Action Tick() override
    {
        return em::App::Action::exit_success;
    }

    em::App::Action HandleEvent(SDL_Event &e) override
    {
        return em::App::Action::cont;
    }
};

std::unique_ptr<em::App::Module> em::Main()
{
    return std::make_unique<App::ReflectedApp<GameApp>>();
}
