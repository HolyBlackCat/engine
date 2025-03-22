#include "basic_library.h"

#include <SDL3/SDL_messagebox.h>

namespace em
{
    Sdl::Sdl(std::nullptr_t)
        : Sdl() // Ensure cleanup on throw.
    {
        state.error_handler = CriticalErrorHandler([](zstring_view message)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
        });

        state.initialized = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (!state.initialized)
            throw std::runtime_error(fmt::format("SDL init failed: {}", SDL_GetError()));
    }

    Sdl::~Sdl()
    {
        if (!state.initialized)
            return;

        SDL_Quit();
    }
}
