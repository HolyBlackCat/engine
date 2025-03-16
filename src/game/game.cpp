#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "sdl/basic_library.h"
#include "em/refl/macros/structs.h"

#include <iostream>
#include <memory>

struct GameApp : em::App::Module
{
    EM_REFL(
        (em::Modules::Sdl)(sdl)
    )

    em::App::Action Init() override
    {
        std::cout << "Hello!\n";
        return em::App::Action::cont;
    }

    em::App::Action Tick() override
    {
        return em::App::Action::exit_success;
    }

    em::App::Action HandleEvent(SDL_Event &e) override
    {
        return em::App::Action::cont;
    }

    void Deinit(bool failure) override
    {
        (void)failure;
    }
};

std::unique_ptr<em::App::Module> em::Main()
{
    return std::make_unique<App::ReflectedApp<GameApp>>();
}
