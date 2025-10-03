#ifndef EM_ENABLE_TESTS

#include "main.h"

#include "mainloop/module.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    auto app = em::Main(argc, argv);
    if (!app)
        return SDL_APP_SUCCESS;

    *appstate = app.release();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    return SDL_AppResult(static_cast<em::App::Module *>(appstate)->Tick());
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e)
{
    return SDL_AppResult(static_cast<em::App::Module *>(appstate)->HandleEvent(*e));
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;

    // This is the only callback that can be reached with a null `appstate`.
    delete static_cast<em::App::Module *>(appstate);
}

#endif // EM_ENABLE_TESTS
