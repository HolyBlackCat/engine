#include "em/refl/macros/structs.h"
#include "gpu/buffer.h"
#include "gpu/command_buffer.h"
#include "gpu/device.h"
#include "gpu/copy_pass.h"
#include "gpu/pipeline.h"
#include "gpu/render_pass.h"
#include "gpu/shader.h"
#include "gpu/transfer_buffer.h"
#include "mainloop/main.h"
#include "mainloop/reflected_app.h"
#include "utils/filesystem.h"
#include "window/sdl.h"
#include "window/window.h"

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

    Gpu::Pipeline pipeline = Gpu::Pipeline(gpu, Gpu::Pipeline::Params{
        .shaders = {
            .vert = &sh_v,
            .frag = &sh_f,
        },
        .vertex_buffers = {
            {
                Gpu::Pipeline::VertexBuffer{
                    .pitch = sizeof(fvec3) * 2,
                    .attributes = {
                        Gpu::Pipeline::VertexAttribute{
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            .byte_offset_in_elem = 0,
                        },
                        Gpu::Pipeline::VertexAttribute{
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            .byte_offset_in_elem = sizeof(fvec3),
                        },
                    }
                }
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
        Gpu::TransferBuffer tr_buffer = Gpu::TransferBuffer(gpu, sizeof(fvec3) * 6);

        {
            Gpu::TransferBuffer::Mapping map = tr_buffer.Map();
            auto ptr = reinterpret_cast<fvec3 *>(map.Span().data());
            *ptr++ = fvec3(0, 0.5, 0);
            *ptr++ = fvec3(1, 0, 0);
            *ptr++ = fvec3(0.5, -0.5, 0);
            *ptr++ = fvec3(0, 1, 0);
            *ptr++ = fvec3(-0.5, -0.5, 0);
            *ptr++ = fvec3(0, 0, 1);
        }

        Gpu::CommandBuffer cmdbuf(gpu);
        Gpu::CopyPass pass(cmdbuf);
        tr_buffer.ApplyToBuffer(pass, buffer);
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
