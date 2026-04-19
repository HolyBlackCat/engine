#pragma once

#include "em/math/vector.h"
#include "em/meta/reset_on_move.h"
#include "em/refl/macros/structs.h"
#include "gpu/buffer.h"
#include "gpu/command_buffer.h"
#include "gpu/copy_pass.h"
#include "gpu/device.h"
#include "gpu/render_pass.h"
#include "gpu/sampler.h"
#include "gpu/texture.h"
#include "gpu/transfer_buffer.h"
#include "graphics/shader_manager.h"

#include <span>

namespace em::Graphics
{
    // `Renderer2d` itself needs to be recreated every frame.
    // `Renderer2d::Resources` should persist across frames.
    class Renderer2d
    {
      public:
        struct Vertex
        {
            EM_REFL(
                (fvec2)(pos)
                (fvec4)(color)
                (fvec2)(texcoord)

                // X - color mixing: 0 = `color.rgb`, 1 = texture.
                // X - alpha mixing: 0 = `color.a`, 1 = texture.
                // X - blending mode: 1 = normal blending, 0 = additive blending.
                (fvec3)(factors)
            )

            constexpr Vertex() {}
            constexpr Vertex(fvec2 pos, fvec4 color,                                                                            float beta = 1) : pos(pos), color(color), factors(0, 0, beta) {}
            constexpr Vertex(fvec2 pos,              fvec2 texcoord, float alpha = 1,                                           float beta = 1) : pos(pos), texcoord(texcoord), factors(1, alpha, beta) {}
            constexpr Vertex(fvec2 pos, fvec4 color, fvec2 texcoord,                  float mix_color = 1, float mix_alpha = 1, float beta = 1) : pos(pos), color(color), texcoord(texcoord), factors(mix_color, mix_alpha, beta) {}
        };

        struct Params
        {
            std::size_t num_triangles = 1024;

            // Not optional. SDL doesn't let you just omit textures if the shader uses them.
            Gpu::Texture *texture = nullptr;
        };

        class Resources
        {
            friend Renderer2d;

            EM_REFL(
                (ShaderProgram)(static shader)
            )

            Params params;

            Gpu::Buffer buffer;
            Gpu::TransferBuffer transfer_buffer;
            Gpu::Sampler sampler;

            Gpu::DynamicPipeline pipeline;

          public:
            constexpr Resources() {}

            Resources(Gpu::Device &device, const Params &params);
        };

      private:
        struct State
        {
            Resources *resources = nullptr;
            Gpu::RenderPass *render_pass = nullptr;
            Gpu::CopyPass *copy_pass = nullptr;

            Gpu::TransferBuffer::Mapping mapping;

            std::size_t vertex_pos_in_buffer = 0;

            // Need this to make Clang happy in `ResetMovedFromStruct<...>` below.
            constexpr State() {}
        };
        Meta::ResetMovedFromStruct<State> state;

        void BeginRendering();
        void EndRendering();

      public:
        constexpr Renderer2d() {}

        // Before calling this, you must have a render command buffer and a copy command buffer, and start render and copy passes on them respectively.
        // After the destructor runs, you must submit `copy_pass` and then `render_pass`, in this order.
        // `viewport_size` only affects how the input coordinates are mapped to NDC.
        Renderer2d(Gpu::Device &device, Resources &resources, Gpu::CommandBuffer &render_cmdbuf, Gpu::RenderPass &render_pass, Gpu::CopyPass &copy_pass, SDL_GPUTextureFormat output_format, ivec2 viewport_size);

        Renderer2d(Renderer2d &&) = default;
        Renderer2d &operator=(Renderer2d &&) = default;

        ~Renderer2d();

        [[nodiscard]] std::size_t VertexCapacity() const {return state.resources->params.num_triangles * 3;}
        [[nodiscard]] std::size_t RemainingVertexCapacity() const {return VertexCapacity() - state.vertex_pos_in_buffer;}

        // Must send the vertices in multiples of three.
        void DrawVertices(std::span<const Vertex> vertices);
    };
}
