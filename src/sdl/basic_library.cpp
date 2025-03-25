#include "basic_library.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>

namespace em
{
    Sdl::Sdl(const AppMetadata &metadata)
        : Sdl() // Ensure cleanup on throw.
    {
        state.error_handler = CriticalErrorHandler([](zstring_view message)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
        });

        { // Set metadata.
            struct Property
            {
                zstring_view AppMetadata::*member = nullptr;
                const char *name = nullptr;
            };

            static constexpr Property props[] = {
                {&AppMetadata::name,       SDL_PROP_APP_METADATA_NAME_STRING},
                {&AppMetadata::version,    SDL_PROP_APP_METADATA_VERSION_STRING},
                {&AppMetadata::identifier, SDL_PROP_APP_METADATA_IDENTIFIER_STRING},
                {&AppMetadata::author,     SDL_PROP_APP_METADATA_CREATOR_STRING},
                {&AppMetadata::copyright,  SDL_PROP_APP_METADATA_COPYRIGHT_STRING},
                {&AppMetadata::url,        SDL_PROP_APP_METADATA_URL_STRING},
                {&AppMetadata::type,       SDL_PROP_APP_METADATA_TYPE_STRING},
            };

            for (const auto &[member, name] : props)
            {
                if (!(metadata.*member).empty())
                    SDL_SetAppMetadataProperty(name, (metadata.*member).c_str());
            }
        }

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
