#include "renderer_2d.h"

#include "gpu/command_buffer.h"
#include "gpu/refl/vertex_layout.h"
#include "gpu/render_pass.h"

#include "strings/trim.h"

namespace em::Graphics
{
    ShaderProgram Renderer2d::Resources::shader(
        "Renderer2d",
        (std::string)R"(
            #version 460

            layout(set = 1, binding = 0) uniform Uni
            {
                vec2 u_scr_size;
            };

            layout(location = 0) in vec2 a_pos;
            layout(location = 1) in vec4 a_color;
            layout(location = 2) in vec2 a_texcoord;
            layout(location = 3) in vec3 a_factors;

            layout(location = 0) out vec4 v_color;
            layout(location = 1) out vec2 v_texcoord;
            layout(location = 2) out vec3 v_factors;

            void main()
            {
                gl_Position = vec4(a_pos * 2 / u_scr_size, 0, 1);
                v_color = a_color;
                v_texcoord = a_texcoord;
                v_factors = a_factors;
            }
        )"_compact,
        (std::string)R"(
            #version 460

            layout(set = 2, binding = 0) uniform sampler2D u_texture;

            layout(set = 3, binding = 0) uniform Uni
            {
                vec2 u_tex_size;
            };


            layout(location = 0) in vec4 v_color;
            layout(location = 1) in vec2 v_texcoord;
            layout(location = 2) in vec3 v_factors;

            layout(location = 0) out vec4 out_color;

            void main()
            {
                vec4 tex_color = texture(u_texture, v_texcoord / u_tex_size);
                out_color = vec4(mix(v_color.rgb, tex_color.rgb, v_factors.x),
                                 mix(v_color.a  , tex_color.a  , v_factors.y));

                out_color.rgb *= out_color.a;
                out_color.a *= v_factors.z;
            }
        )"_compact
    );

    Renderer2d::Resources::Resources(Gpu::Device &device, const Params &params)
        : params(params)
    {
        std::uint32_t byte_size = std::uint32_t(params.num_triangles * 3 * sizeof(Vertex));
        buffer = Gpu::Buffer(device, byte_size);
        transfer_buffer = Gpu::TransferBuffer(device, byte_size);
        sampler = Gpu::Sampler(device, Gpu::Sampler::Params{.filter_min = Gpu::Sampler::Filter::nearest, .filter_mag = Gpu::Sampler::Filter::nearest});

        pipeline = Gpu::Pipeline::Params{
            .shaders = shader,
            .vertex_buffers = {Gpu::ReflectedVertexLayout<Vertex>{}},
            .targets = {.color = {{
                .blending = Gpu::Pipeline::Blending::Premultiplied(),
            }}},
            .rasterizer = {},
        };
    }

    void Renderer2d::BeginRendering()
    {
        state.mapping = state.resources->transfer_buffer.Map();
        state.vertex_pos_in_buffer = 0;
    }

    void Renderer2d::EndRendering()
    {
        state.mapping = {};
        state.resources->transfer_buffer.ApplyToBuffer(*state.copy_pass, 0, state.resources->buffer, 0, std::uint32_t(state.vertex_pos_in_buffer * sizeof(Vertex)));

        // We have to call this again when `ApplyToBuffer()` cycles the buffer.
        state.render_pass->BindVertexBuffers({{
            {.buffer = &state.resources->buffer},
        }});

        state.render_pass->DrawPrimitives(std::uint32_t(state.vertex_pos_in_buffer));
    }

    Renderer2d::Renderer2d(Gpu::Device &device, Resources &resources, Gpu::CommandBuffer &render_cmdbuf, Gpu::RenderPass &render_pass, Gpu::CopyPass &copy_pass, SDL_GPUTextureFormat output_format, ivec2 viewport_size)
    {
        resources.pipeline.RequestOutputFormat(device, output_format);

        state.resources = &resources;
        state.render_pass = &render_pass;
        state.copy_pass = &copy_pass;

        // Try to support having no texture, because why not.
        if (state.resources->params.texture)
        {
            Gpu::Shader::BindTextures(render_pass, {{.texture = state.resources->params.texture, .sampler = &state.resources->sampler}});
            Gpu::Shader::SetUniform(render_cmdbuf, Gpu::Shader::Stage::fragment, 0, state.resources->params.texture->GetSize().to_vec2().to<float>());
        }

        Gpu::Shader::SetUniform(render_cmdbuf, Gpu::Shader::Stage::vertex, 0, fvec2(viewport_size.x, -viewport_size.y)); // Flip the Y component to make the Y axis go down.
        Gpu::Shader::BindTextures(render_pass, {{
            {.texture = state.resources->params.texture, .sampler = &state.resources->sampler},
        }});

        state.render_pass->BindPipeline(state.resources->pipeline);

        BeginRendering();
    }

    Renderer2d::~Renderer2d()
    {
        EndRendering();
    }

    void Renderer2d::DrawVertices(std::span<const Vertex> vertices)
    {
        assert(vertices.size() % 3 == 0);

        while (!vertices.empty())
        {
            std::size_t num_vertices_in_chunk = std::min(vertices.size(), RemainingVertexCapacity());

            std::copy_n(vertices.data(), num_vertices_in_chunk, state.mapping.AsRangeOf<Vertex>().data() + state.vertex_pos_in_buffer);
            state.vertex_pos_in_buffer += num_vertices_in_chunk;
            vertices = vertices.subspan(num_vertices_in_chunk);

            if (RemainingVertexCapacity() == 0)
            {
                EndRendering();
                BeginRendering();
            }
        }
    }
}
