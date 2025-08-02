#include "em/refl/macros/structs.h"
#include "gpu/buffer.h"
#include "gpu/command_buffer.h"
#include "gpu/copy_pass.h"
#include "gpu/device.h"
#include "gpu/pipeline.h"
#include "gpu/refl/vertex_layout.h"
#include "gpu/render_pass.h"
#include "gpu/shader.h"
#include "gpu/transfer_buffer.h"
#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "strings/trim.h"
#include "utils/filesystem.h"
#include "utils/process_queue.h"
#include "utils/reinterpret_span.h"
#include "sdl/sdl.h"
#include "sdl/window.h"

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
    )

    Gpu::Shader sh_v = Gpu::Shader(gpu, "main vert", Gpu::Shader::Stage::vertex, Filesystem::LoadedFile(fmt::format("{}assets/shaders/main.vert.spv", Filesystem::GetResourceDir())));
    Gpu::Shader sh_f = Gpu::Shader(gpu, "main frag", Gpu::Shader::Stage::fragment, Filesystem::LoadedFile(fmt::format("{}assets/shaders/main.frag.spv", Filesystem::GetResourceDir())));

    EM_STRUCT(Vertex)
    (
        (fvec3)(pos)
        (fvec3)(color)
    )

    Gpu::Pipeline pipeline = Gpu::Pipeline(gpu, Gpu::Pipeline::Params{
        .shaders = {
            .vert = &sh_v,
            .frag = &sh_f,
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

    Gpu::Buffer buffer = Gpu::Buffer(gpu, sizeof(fvec3) * 6);

    GameApp()
    {
        std::cout << "[" << R"(
            int main()
            {
                std::cout << "Sup\n";
            }
        )" << "]\n";

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

        buffer = Gpu::Buffer(gpu, pass, reinterpret_span<unsigned char>(verts));
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

std::unique_ptr<App::Module> em::Main()
{
    return std::make_unique<App::ReflectedApp<GameApp>>();
}
