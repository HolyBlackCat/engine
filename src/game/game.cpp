#include "command_line/parser_refl.h"
#include "command_line/parser.h"
#include "em/refl/macros/structs.h"
#include "em/refl/static_virtual.h"
#include "gpu/buffer.h"
#include "gpu/command_buffer.h"
#include "gpu/copy_pass.h"
#include "gpu/device.h"
#include "gpu/pipeline.h"
#include "gpu/render_pass.h"
#include "gpu/shader.h"
#include "gpu/transfer_buffer.h"
#include "graphics/pixel_upscaler.h"
#include "graphics/renderer_2d.h"
#include "graphics/shader_manager.h"
#include "mainloop/game_state.h"
#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "sdl/sdl.h"
#include "sdl/window.h"

#include <iostream>
#include <memory>

using namespace em;

static constexpr ivec2 screen_size = ivec2(1920, 1080) / 4;

struct GameApp : App::Module
{
    EM_REFL(
        (Sdl)(sdl, AppMetadata{
            .name = "Hello world",
            .version = "0.0.1",
            // .identifier = "",
            // .author = "",
            // .copyright = "",
            // .url = "",
        })
        (Gpu::Device)(gpu, Gpu::Device::Params{})
        (Window)(window, Window::Params{
            .gpu_device = &gpu,
            .size = screen_size * 2,
            .min_size = screen_size,
        })
        (Graphics::ShaderManager)(shader_manager, gpu)
        (Gpu::Texture)(texture)
        (Graphics::Renderer2d::Resources)(renderer_resources)
        (Graphics::PixelUpscaler::Resources)(upscaler_resources)
    )

    GameApp(int argc, char **argv)
    {
        { // Collect needed shaders, before parsing the flags.
            Refl::RecursivelyVisitStaticElemsOfTypeCvref<Graphics::Shader &>(*this, [&](Graphics::Shader &sh){shader_manager.AddShader(sh);});
        }

        { // Parse the command line args.
            CommandLine::Parser parser;
            parser.AddDefaultHelpFlag();
            // Here we allow non-static callbacks in `this`, and only static callbacks in the game states.
            CommandLine::Refl::AddProvidedCommandLineFlags(parser, *this);
            for (const auto &in : Refl::StaticVirtual::GetMap<App::BasicState::Interface>())
                in.second->AddProvidedCommandLineFlagsStatic(parser);
            parser.Parse(argc, argv);
        }

        // Init graphics:

        { // Load the texture.
            Gpu::CommandBuffer cmdbuf(gpu);
            Gpu::CopyPass copy_pass(cmdbuf);
            texture = Gpu::Texture(gpu, copy_pass, Image("dummy", Filesystem::LoadedFile(fmt::format("{}assets/images/dummy.png", Filesystem::GetResourceDir()))));
            upscaler_resources = Graphics::PixelUpscaler::Resources(gpu, copy_pass, screen_size);
        }

        renderer_resources = Graphics::Renderer2d::Resources(gpu, Graphics::Renderer2d::Params{.num_triangles = 1, .texture = &texture});
    }

    App::Action Tick() override
    {
        Gpu::SwapchainAcquireResult swapchain = WaitAndAcquireSwapchainTextureAndCmdBuf(window, gpu);
        if (!swapchain)
        {
            swapchain.cmdbuf.CancelWhenDestroyed();
            return App::Action::cont; // No draw target.
        }

        { // Draw the graphics that's going to be upscaled.
            Graphics::PixelUpscaler upscaler(gpu, upscaler_resources, swapchain.cmdbuf, swapchain.texture);

            Gpu::CommandBuffer copy_cmdbuf(gpu);
            Gpu::CopyPass copy_pass(copy_cmdbuf);

            Graphics::Renderer2d r(gpu, renderer_resources, swapchain.cmdbuf, upscaler.RenderPass(), copy_pass, upscaler.InputTexture().GetFormat(), upscaler.InputTexture().GetSize().to_vec2());

            Graphics::Renderer2d::Vertex verts[] {
                Graphics::Renderer2d::Vertex(fvec2(100, 100), fvec4(1,0,0,1)),
                Graphics::Renderer2d::Vertex(fvec2(164, 100), fvec4(0,1,0,1)),
                Graphics::Renderer2d::Vertex(fvec2(100, 164), fvec4(0,0,1,1)),
                Graphics::Renderer2d::Vertex(fvec2(100 + 32, 100), fvec2(0,0)),
                Graphics::Renderer2d::Vertex(fvec2(164 + 32, 100), fvec2(64,0)),
                Graphics::Renderer2d::Vertex(fvec2(100 + 32, 164), fvec2(0,64)),
            };

            r.DrawVertices(verts);
        }

        return App::Action::cont;
    }

    App::Action HandleEvent(SDL_Event &e) override
    {
        if (e.type == SDL_EVENT_QUIT)
            return App::Action::exit_success;
        return App::Action::cont;
    }
};

std::unique_ptr<App::Module> em::Main(int argc, char **argv)
{
    return std::make_unique<App::ReflectedApp<GameApp>>(argc, argv);
}
