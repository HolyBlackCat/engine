#include "pixel_upscaler.h"

#include "em/macros/utils/lift.h"
#include "gpu/refl/vertex_layout.h"
#include "strings/trim.h"

namespace em::Graphics
{
    namespace
    {
        EM_STRUCT( Vertex )
        (
            (fvec2)(pos)
        )
    }

    ShaderProgram PixelUpscaler::Resources::shader(
        "PixelUpscaler",
        (std::string)R"(
            #version 460

            layout(location = 0) in vec2 a_pos;

            layout(location = 0) out vec2 v_texcoord;

            void main()
            {
                gl_Position = vec4(a_pos, 0, 1);

                // The negation of Y is optional. Removing the negation cancels itself out over the two render passes.
                // But negating it here helps when looking at the intermediate texture in renderdoc.
                // Without the negation it would be mirrored vertically.
                v_texcoord = a_pos * vec2(0.5, -0.5) + 0.5;
            }
        )"_compact,
        (std::string)R"(
            #version 460

            layout(set = 2, binding = 0) uniform sampler2D u_texture;

            layout(location = 0) in vec2 v_texcoord;

            layout(location = 0) out vec4 out_color;

            void main()
            {
                out_color = texture(u_texture, v_texcoord);
            }
        )"_compact
    );

    PixelUpscaler::Resources::Resources(Gpu::Device &device, Gpu::CopyPass &copy_pass, ivec2 size, SDL_GPUTextureFormat internal_texture_format)
        : fullscreen_triangle(device, copy_pass, std::array{
            Vertex{fvec2(-1, -1)},
            Vertex{fvec2( 3, -1)},
            Vertex{fvec2(-1,  3)},
        }),
        tex1(device, {
            .format = internal_texture_format,
            .usage = Gpu::Texture::UsageFlags::sampler | Gpu::Texture::UsageFlags::color_target,
            .size = size.to_vec3(1),
        }),
        sampler1(device, {.filter_min = Gpu::Sampler::Filter::nearest, .filter_mag = Gpu::Sampler::Filter::nearest}),
        pipeline1(device, Gpu::Pipeline::Params{
            .shaders = shader,
            .vertex_buffers = {Gpu::ReflectedVertexLayout<Vertex>{}},
            .targets = {{Gpu::Pipeline::ColorTarget{}}},
        }),
        // `tex2` is constructed lazily, because its size can change.
        sampler2(device, {.filter_min = Gpu::Sampler::Filter::linear, .filter_mag = Gpu::Sampler::Filter::linear}),
        pipeline2(Gpu::Pipeline::Params{
            // Here parameters appear to be the same as for `pipeline1`, but since this one is actually a `DynamicPipeline`, it will have the output format updated on the fly.
            .shaders = shader,
            .vertex_buffers = {Gpu::ReflectedVertexLayout<Vertex>{}},
            .targets = {{Gpu::Pipeline::ColorTarget{}}},
        })
    {}

    PixelUpscaler::PixelUpscaler(Gpu::Device &device, Resources &resources, Gpu::CommandBuffer &cmdbuf, Gpu::Texture &output_texture)
    {
        state.device = &device;
        state.resources = &resources;
        state.cmdbuf = &cmdbuf;
        state.output_texture = &output_texture;

        int int_scale = (output_texture.GetSize().to_vec2() / resources.tex1.GetSize().to_vec2()).reduce(EM_FUNC(std::min));
        ivec2 desired_tex2_size = resources.tex1.GetSize().to_vec2() * int_scale;

        if (!resources.tex2 || resources.tex2.GetSize().to_vec2() != desired_tex2_size)
        {
            resources.tex2 = Gpu::Texture(device, {
                .format = state.resources->tex1.GetFormat(), // Same as in the other texture.
                .usage = Gpu::Texture::UsageFlags::sampler | Gpu::Texture::UsageFlags::color_target,
                .size = desired_tex2_size.to_vec3(1),
            });
        }

        // Begin the user render pass.
        state.user_render_pass = Gpu::RenderPass(cmdbuf, {.color_targets = {{
            .texture = {&resources.tex1},
        }}});
    }

    PixelUpscaler::~PixelUpscaler()
    {
        if (!state.resources)
            return; // A null instance, do nothing.

        // Finish the user render pass.
        state.user_render_pass = {};

        { // Upscale by an integral amount.
            Gpu::RenderPass rp(*state.cmdbuf, {.color_targets = {Gpu::RenderPass::ColorTarget{
                .texture = {&state.resources->tex2},
                .initial_contents = Gpu::RenderPass::DontCare{},
            }}});

            rp.BindPipeline(state.resources->pipeline1);
            rp.BindVertexBuffers({{.buffer = &state.resources->fullscreen_triangle}});
            Gpu::Shader::BindTextures(rp, {{.texture = &state.resources->tex1, .sampler = &state.resources->sampler1}});
            rp.DrawPrimitives(3);
        }

        { // Upscale by the remaining fractional amount.
            state.resources->pipeline2.RequestOutputFormat(*state.device, state.output_texture->GetFormat());

            Gpu::RenderPass rp(*state.cmdbuf, {.color_targets = {Gpu::RenderPass::ColorTarget{
                .texture = {state.output_texture},
                .initial_contents = Gpu::RenderPass::DontCare{},
            }}});

            rp.BindPipeline(state.resources->pipeline2);
            rp.BindVertexBuffers({{.buffer = &state.resources->fullscreen_triangle}});
            Gpu::Shader::BindTextures(rp, {{.texture = &state.resources->tex2, .sampler = &state.resources->sampler2}});

            float float_scale = (state.output_texture->GetSize().to_vec2().to<float>() / state.resources->tex1.GetSize().to_vec2()).reduce(EM_FUNC(std::min));

            ivec2 output_size = (state.resources->tex1.GetSize().to_vec2() * float_scale).map(EM_FUNC(std::round)).to<int>();
            ivec2 output_pos = (state.output_texture->GetSize().to_vec2() - output_size) / 2;

            rp.SetViewport({.pos = output_pos, .size = output_size});
            rp.DrawPrimitives(3);
        }
    }
}
