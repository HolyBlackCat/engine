#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "sdl/basic_library.h"
#include "em/refl/macros/structs.h"
#include "sdl/window.h"

#include <iostream>
#include <memory>

struct GameApp : em::App::Module
{
    EM_REFL(
        (em::Sdl)(sdl, em::AppMetadata{
            .name = "Hello world",
            .version = "0.0.1",
            // .identifier = "",
            // .author = "",
            // .copyright = "",
            // .url = "",
        })
        (em::Window)(window, em::Window::Params{

        })
    )

    em::App::Action Tick() override
    {
        return em::App::Action::cont;
    }

    em::App::Action HandleEvent(SDL_Event &e) override
    {
        if (e.type == SDL_EVENT_QUIT)
            return em::App::Action::exit_success;
        return em::App::Action::cont;
    }
};

std::unique_ptr<em::App::Module> em::Main()
{
    return std::make_unique<App::ReflectedApp<GameApp>>();
}
