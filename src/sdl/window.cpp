#include "window.h"

#include "em/macros/utils/lift.h"
#include "utils/lazy_format_arg.h"

#include <fmt/format.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <utility>

namespace em
{
    Window::Window(const Params &params)
        : Window() // Ensure cleanup on throw.
    {
        // Strings returned by `SDL_GetAppMetadataProperty()` don't need freeing.
        std::string name = fmt::format(
            fmt::runtime(params.name),
            fmt::arg("name", LazyFormatArg(EM_EXPR(SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING)))),
            fmt::arg("version", LazyFormatArg(EM_EXPR(SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING))))
        );
        state.window = SDL_CreateWindow(name.c_str(), params.size.x, params.size.y, params.resizable * SDL_WINDOW_RESIZABLE);
    }

    Window::Window(Window &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    Window &Window::operator=(Window other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    Window::~Window()
    {
        if (state.window)
            SDL_DestroyWindow(state.window);
    }
}
