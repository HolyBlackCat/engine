#include "pipeline.h"

#include "gpu/device.h"
#include "gpu/shader.h"

#include <fmt/format.h>

#include <stdexcept>
#include <utility>

namespace em::Gpu
{
    Pipeline::Pipeline(Device &device, const Params &params)
        : Pipeline() // Ensure cleanup on throw.
    {
        SDL_GPUGraphicsPipelineCreateInfo sdl_params{};


        { // Shaders.
            sdl_params.vertex_shader = params.shaders.vert->Handle();
            sdl_params.fragment_shader = params.shaders.frag->Handle();
        }


        // Vertex attributes:

        // Storage for attribute config.
        std::vector<SDL_GPUVertexBufferDescription> sdl_vertex_buffers;
        std::vector<SDL_GPUVertexAttribute> sdl_vertex_attributes;

        { // Fill the storage vectors and refer to them in `sdl_params`.
            bool first_attr = true;
            bool custom_locations = false;


            // Reserve memory.
            sdl_vertex_buffers.reserve(params.vertex_buffers.size());

            std::size_t total_num_attrs = 0;
            for (const VertexBuffer &vertex_buffer : params.vertex_buffers)
                total_num_attrs += vertex_buffer.attributes.size();

            sdl_vertex_attributes.reserve(total_num_attrs);


            for (const VertexBuffer &vertex_buffer : params.vertex_buffers)
            {
                for (const VertexAttribute &vertex_attribute : vertex_buffer.attributes)
                {
                    if (first_attr)
                    {
                        custom_locations = bool(vertex_attribute.custom_location_in_shader);
                    }
                    else
                    {
                        if (custom_locations != bool(vertex_attribute.custom_location_in_shader))
                            throw std::runtime_error("When creating a graphics pipeline, all vertex attribute locations must be either specified manually or left unspecified, in which case we use incremental ones.");
                    }

                    sdl_vertex_attributes.push_back({
                        .location = vertex_attribute.custom_location_in_shader ? *vertex_attribute.custom_location_in_shader : std::uint32_t(sdl_vertex_attributes.size()),
                        .buffer_slot = std::uint32_t(sdl_vertex_buffers.size()),
                        .format = vertex_attribute.format,
                        .offset = vertex_attribute.byte_offset_in_elem,
                    });
                }

                sdl_vertex_buffers.push_back({
                    .slot = std::uint32_t(sdl_vertex_buffers.size()),
                    .pitch = vertex_buffer.pitch,
                    .input_rate = vertex_buffer.per_instance ? SDL_GPU_VERTEXINPUTRATE_INSTANCE : SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .instance_step_rate = 0, // Must be set to `0` for now, SDL doesn't implement it yet.
                });
            }

            sdl_params.vertex_input_state.vertex_attributes = sdl_vertex_attributes.data();
            sdl_params.vertex_input_state.num_vertex_attributes = std::uint32_t(sdl_vertex_attributes.size());
            sdl_params.vertex_input_state.vertex_buffer_descriptions = sdl_vertex_buffers.data();
            sdl_params.vertex_input_state.num_vertex_buffers = std::uint32_t(sdl_vertex_buffers.size());
        }


        // Color targets.

        // The storage for the color targets.
        std::vector<SDL_GPUColorTargetDescription> sdl_color_targets;

        { // Fill the color targets and write them to `sdl_params`.
            sdl_color_targets.reserve(params.targets.color.size());

            for (const ColorTarget &target : params.targets.color)
            {
                sdl_color_targets.push_back({
                    .format = target.texture_format,
                    .blend_state = {},
                });

                SDL_GPUColorTargetBlendState &sdl_blend = sdl_color_targets.back().blend_state;

                if (target.blending)
                {
                    sdl_blend.enable_blend = true;

                    sdl_blend.src_color_blendfactor = target.blending->color.source;
                    sdl_blend.dst_color_blendfactor = target.blending->color.target;
                    sdl_blend.color_blend_op        = target.blending->color.operation;
                    sdl_blend.src_alpha_blendfactor = target.blending->alpha.source;
                    sdl_blend.dst_alpha_blendfactor = target.blending->alpha.target;
                    sdl_blend.alpha_blend_op        = target.blending->alpha.operation;
                }

                if (target.color_write_mask != bvec4(1))
                {
                    sdl_blend.enable_color_write_mask = true;
                    sdl_blend.color_write_mask =
                        target.color_write_mask.r() * SDL_GPU_COLORCOMPONENT_R |
                        target.color_write_mask.g() * SDL_GPU_COLORCOMPONENT_G |
                        target.color_write_mask.b() * SDL_GPU_COLORCOMPONENT_B |
                        target.color_write_mask.a() * SDL_GPU_COLORCOMPONENT_A;
                }
            }

            sdl_params.target_info.color_target_descriptions = sdl_color_targets.data();
            sdl_params.target_info.num_color_targets = std::uint32_t(sdl_color_targets.size());


            if (params.targets.depth_stencil_format)
            {
                sdl_params.target_info.has_depth_stencil_target = true;
                sdl_params.target_info.depth_stencil_format = *params.targets.depth_stencil_format;
            }
        }


        // Primitve type.
        sdl_params.primitive_type = SDL_GPUPrimitiveType(params.primitive);


        { // Rasterizer.
            sdl_params.rasterizer_state.fill_mode = params.rasterizer.wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
            sdl_params.rasterizer_state.cull_mode = SDL_GPUCullMode(params.rasterizer.culling);
            sdl_params.rasterizer_state.front_face = params.rasterizer.front_faces_are_clockwise ? SDL_GPU_FRONTFACE_CLOCKWISE : SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

            if (params.rasterizer.depth_bias)
            {
                sdl_params.rasterizer_state.enable_depth_bias = true;

                sdl_params.rasterizer_state.depth_bias_constant_factor = params.rasterizer.depth_bias->constant_factor;
                sdl_params.rasterizer_state.depth_bias_slope_factor = params.rasterizer.depth_bias->slope_factor;
                sdl_params.rasterizer_state.depth_bias_clamp = params.rasterizer.depth_bias->clamp;
            }

            sdl_params.rasterizer_state.enable_depth_clip = params.rasterizer.clip_by_depth;
        }


        // Multisampling.
        // The rest of this structure isn't implemented in SDL yet. Only this one member.
        sdl_params.multisample_state.sample_count = SDL_GPUSampleCount(params.multisample.samples);


        { // Depth and stencil.
            if (params.depth)
            {
                // Note that SDL says that depth testing must be enabled for the depth writes to work, that's why we code it like this.
                // If you want only the writes but no testing, set the comparison operator to `ALWAYS`.
                sdl_params.depth_stencil_state.enable_depth_test = true;
                sdl_params.depth_stencil_state.enable_depth_write = params.depth->write_depth;
                sdl_params.depth_stencil_state.compare_op = params.depth->depth_pass_condition;
            }

            if (params.stencil)
            {
                sdl_params.depth_stencil_state.enable_stencil_test = true;

                sdl_params.depth_stencil_state.front_stencil_state.fail_op       = params.stencil->front_faces.on_fail_stencil;
                sdl_params.depth_stencil_state.front_stencil_state.pass_op       = params.stencil->front_faces.on_pass_stencil_and_depth;
                sdl_params.depth_stencil_state.front_stencil_state.depth_fail_op = params.stencil->front_faces.on_pass_stencil_but_fail_depth;
                sdl_params.depth_stencil_state.front_stencil_state.compare_op    = params.stencil->front_faces.stencil_pass_condition;

                sdl_params.depth_stencil_state.back_stencil_state.fail_op       = params.stencil->back_faces.on_fail_stencil;
                sdl_params.depth_stencil_state.back_stencil_state.pass_op       = params.stencil->back_faces.on_pass_stencil_and_depth;
                sdl_params.depth_stencil_state.back_stencil_state.depth_fail_op = params.stencil->back_faces.on_pass_stencil_but_fail_depth;
                sdl_params.depth_stencil_state.back_stencil_state.compare_op    = params.stencil->back_faces.stencil_pass_condition;

                sdl_params.depth_stencil_state.compare_mask = params.stencil->compare_mask;
                sdl_params.depth_stencil_state.write_mask = params.stencil->write_mask;
            }
        }


        // Must set before creating the pipeline to let the destructor do its job if we throw later in this function.
        state.device = device.Handle();

        state.pipeline = SDL_CreateGPUGraphicsPipeline(device.Handle(), &sdl_params);
        if (!state.pipeline)
            throw std::runtime_error(fmt::format("Unable to create a GPU pipeline: {}", SDL_GetError()));
    }

    Pipeline::Pipeline(Pipeline &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }
    Pipeline &Pipeline::operator=(Pipeline other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    Pipeline::~Pipeline()
    {
        if (state.pipeline)
            SDL_ReleaseGPUGraphicsPipeline(state.device, state.pipeline);
    }
}
