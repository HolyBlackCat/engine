#include "basic_library.h"

#include <SDL3/SDL_messagebox.h>

namespace em::Modules
{
    void Sdl::InitLib()
    {
        if (initialized)
            return;

        error_handler = CriticalErrorHandler([](zstring_view message)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
        });

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
            throw std::runtime_error(fmt::format("SDL init failed: {}", SDL_GetError()));

        initialized = true;
    }

    void Sdl::ResetLib()
    {
        if (!initialized)
            return;

        SDL_Quit();

        error_handler = nullptr;
        initialized = false;
    }

    App::Action Sdl::Init()
    {
        InitLib();
        return App::Action::exit_success;
    }

    void Sdl::Deinit(bool failure)
    {
        (void)failure;
        ResetLib();
    }

    Sdl::~Sdl()
    {
        ResetLib();
    }
}
