#include "em/refl/macros/structs.h"
#include "gpu/command_buffer.h"
#include "gpu/device.h"
#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "window/sdl.h"
#include "window/window.h"

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
        (em::Gpu::Device)(gpu, nullptr)
        (em::Window)(window, em::Window::Params{
            .gpu_device = &gpu,
        })
    )

    em::App::Action Tick() override
    {
        em::Gpu::CommandBuffer buffer(gpu);
        em::Gpu::Texture swapchain_tex = buffer.WaitAndAcquireSwapchainTexture(window);
        if (!swapchain_tex)
        {
            std::cout << "No swapchain texture, probably the window is minimized\n";
            buffer.CancelWhenDestroyed();
            return em::App::Action::cont; // No draw target.
        }

        fmt::println("Swapchain texture has size: [{},{},{}]", swapchain_tex.Size().x, swapchain_tex.Size().y, swapchain_tex.Size().z);

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
