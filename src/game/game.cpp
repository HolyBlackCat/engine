#include "command_line/parser_refl.h"
#include "command_line/parser.h"
#include "em/refl/macros/structs.h"
#include "em/refl/recursively_visit_elems_maybe_static.h"
#include "em/refl/static_virtual.h"
#include "gpu/buffer.h"
#include "gpu/command_buffer.h"
#include "gpu/copy_pass.h"
#include "gpu/device.h"
#include "gpu/pipeline.h"
#include "gpu/refl/vertex_layout.h"
#include "gpu/render_pass.h"
#include "gpu/shader.h"
#include "gpu/transfer_buffer.h"
#include "graphics/shader_manager.h"
#include "mainloop/game_state.h"
#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "sdl/sdl.h"
#include "sdl/window.h"
#include "strings/trim.h"

#include <iostream>
#include <memory>

using namespace em;

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
        })
        (Graphics::Shader)(sh_v, Graphics::Shader("main vert", Gpu::Shader::Stage::vertex, (std::string)R"(
            #version 460

            layout(location = 0) in vec3 a_pos;
            layout(location = 1) in vec4 a_color;

            layout(location = 0) out vec4 v_color;

            void main()
            {
                v_color = a_color;
                gl_Position = vec4(a_pos, 1);
            }
        )"_compact))
        (Graphics::Shader)(sh_f, Graphics::Shader("main frag", Gpu::Shader::Stage::fragment, (std::string)R"(
            #version 460

            layout(location = 0) in vec4 v_color;

            layout(location = 0) out vec4 o_color;

            void main()
            {
                o_color = v_color;
            }
        )"_compact))
        (Graphics::ShaderManager)(shader_manager, gpu) // Must be after non-static shaders, or manually destructed before them.
    )



    EM_STRUCT(Vertex)
    (
        (fvec3)(pos)
        (fvec3)(color)
    )

    Gpu::Pipeline pipeline;

    Gpu::Buffer buffer = Gpu::Buffer(gpu, sizeof(fvec3) * 6);

    GameApp(int argc, char **argv)
    {
        { // Collect needed shaders, before parsing the flags.
            Refl::RecursivelyVisitMaybeStaticElemsOfTypeCvref<Graphics::Shader &>(*this, [&](Graphics::Shader &sh){shader_manager.AddShader(sh);});
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

        pipeline = Gpu::Pipeline(gpu, Gpu::Pipeline::Params{
            .shaders = {
                .vert = &sh_v.shader,
                .frag = &sh_f.shader,
            },
            .vertex_buffers = {
                {
                    Gpu::ReflectedVertexLayout<Vertex>{},
                }
            },
            .targets = {
                .color = {
                    Gpu::Pipeline::ColorTarget{
                        .texture_format = window.GetSwapchainTextureFormat()
                    }
                },
            },
        });

        Vertex verts[3] = {
            {
                .pos = fvec3(0, 0.5, 0),
                .color = fvec3(1, 0, 0),
            },
            {
                .pos = fvec3(0.5, -0.5, 0),
                .color = fvec3(0, 1, 0),
            },
            {
                .pos = fvec3(-0.5, -0.5, 0),
                .color = fvec3(0, 0, 1),
            },
        };


        Gpu::CommandBuffer cmdbuf(gpu);
        Gpu::CopyPass pass(cmdbuf);

        buffer = Gpu::Buffer(gpu, pass, verts);
    }

    App::Action Tick() override
    {
        Gpu::CommandBuffer cmdbuf(gpu);
        Gpu::Texture swapchain_tex = cmdbuf.WaitAndAcquireSwapchainTexture(window);
        if (!swapchain_tex)
        {
            cmdbuf.CancelWhenDestroyed();
            return App::Action::cont; // No draw target.
        }

        Gpu::RenderPass rp(cmdbuf, Gpu::RenderPass::Params{
            .color_targets = {
                Gpu::RenderPass::ColorTarget{
                    .texture = {
                        .texture = &swapchain_tex,
                    },
                },
            },
        });

        rp.BindPipeline(pipeline);
        rp.BindVertexBuffers({{Gpu::RenderPass::VertexBuffer{
            .buffer = &buffer,
        }}});
        rp.DrawPrimitives(3);

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
