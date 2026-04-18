#pragma once

#include "em/math/vector.h"
#include "em/meta/reset_on_move.h"
#include "em/refl/macros/structs.h"
#include "gpu/buffer.h"
#include "gpu/device.h"
#include "gpu/render_pass.h"
#include "gpu/sampler.h"
#include "gpu/texture.h"
#include "graphics/shader_manager.h"

namespace em::Graphics
{
    class PixelUpscaler
    {
      public:
        class Resources
        {
            friend PixelUpscaler;

            Gpu::Buffer fullscreen_triangle;

            // The small original texture.
            Gpu::Texture tex1;
            // The nearest sampler for this texture.
            Gpu::Sampler sampler1;
            // The pipeline to render from `tex1` to `tex2`.
            Gpu::Pipeline pipeline1;

            // The second texture, upscaled by an integral amount.
            Gpu::Texture tex2;
            // The linear sampler for this texture.
            Gpu::Sampler sampler2;
            // The pipeline to render from `tex2` to the user output. Here the output format can vary.
            Gpu::DynamicPipeline pipeline2;


            EM_REFL(
                (Graphics::ShaderProgram)(static shader)
            )

          public:
            constexpr Resources() {}

            // `copy_pass` is only used for initialization. Finish it before using the upscaler.
            Resources(Gpu::Device &device, Gpu::CopyPass &copy_pass, ivec2 size, SDL_GPUTextureFormat internal_texture_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);

            // Draw your stuff to this texture.
            [[nodiscard]] Gpu::Texture &InputTexture() {return tex1;}
        };

      private:
        struct State
        {
            Gpu::Device *device = nullptr;
            Resources *resources = nullptr;
            Gpu::CommandBuffer *cmdbuf = nullptr;
            Gpu::Texture *output_texture = nullptr;

            Gpu::RenderPass user_render_pass;

            State() {} // Make Clang happy.
        };
        Meta::ResetMovedFromStruct<State> state;

      public:
        constexpr PixelUpscaler() {}

        // This starts its own render pass.
        PixelUpscaler(Gpu::Device &device, Resources &resources, Gpu::CommandBuffer &cmdbuf, Gpu::Texture &output_texture);

        ~PixelUpscaler();

        // Draw your stuff in this render pass.
        [[nodiscard]] Gpu::RenderPass &RenderPass() {return state.user_render_pass;}

        // Draw your stuff to this texture.
        [[nodiscard]] Gpu::Texture &InputTexture() {return state.resources->tex1;}
    };
}
