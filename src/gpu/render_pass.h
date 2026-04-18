#pragma once

#include "em/math/vector.h"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <variant>

namespace em::Gpu
{
    class Buffer;
    class CommandBuffer;
    class Pipeline;
    class Sampler;
    class Shader;
    class Texture;

    class RenderPass
    {
        struct State
        {
            SDL_GPURenderPass *pass = nullptr;
        };
        State state;

      public:
        constexpr RenderPass() {}

        // Clear the contents.
        struct ColorClear
        {
            // I'd like to set this to `fvec4(0,0,0,1)` right here,
            //   but then `ColorInitialContents initial_contents;` doesn't find the default constructor of this type unless we make it user-declared,
            //   which makes us lose aggregate-ness, which isn't what I want. So we keep this default-constructed, and provide a default color below.
            fvec4 color;
        };
        // Load the existing texture contents and don't clear them.
        struct ColorLoad {};
        // Neither load nor clear. The target contents will start in an undefined state.
        struct ColorDontCare {};

        using ColorInitialContents = std::variant<ColorClear, ColorLoad, ColorDontCare>;

        struct TextureTarget
        {
            Texture *texture = nullptr;

            // A layer for texture arrays or 3D textures.
            std::uint32_t layer = 0;
            // A mipmap level to write to.
            std::uint32_t mipmap_level = 0;

            // Set to false if you need to load the existing image first.
            bool cycle = true;
        };

        struct ColorTarget
        {
            // The target texture.
            TextureTarget texture;

            // The initial contents of this texture.
            ColorInitialContents initial_contents = ColorClear{fvec4(0,0,0,1)};

            // If false the output will be undefined. If true it will be written to the texture.
            // Even if this is true, the comments in SDL headers make it look like we still need to specify a texture.
            bool store_output = true;

            // You can optionally set this if `texture` is a multisample texture.
            // Then we'll also produce NON-multisample results and write them here.
            // Then you can also set `store_output = false` if you don't need the multisample results, and only need the non-multisample ones.
            //
            // Adding `{}` to prevent Clang from warning when omitting an initializer for this in designated initialization.
            std::optional<TextureTarget> multisample_resolved_texture{};
        };

        struct DepthClear
        {
            // I'd like to set this to `1` right here,
            //   but then `DepthInitialContents initial_contents;` doesn't find the default constructor of this type unless we make it user-declared,
            //   which makes us lose aggregate-ness, which isn't what I want. So we keep this default-constructed, and provide a default color below.
            float value;
        };
        struct DepthLoad {};
        struct DepthDontCare {};

        using DepthInitialContents = std::variant<DepthClear, DepthLoad, DepthDontCare>;

        struct DepthTarget
        {
            // Should we store the output?
            // Docs recommend false for oneshot depth textures.
            bool store_output = false;

            DepthInitialContents initial_contents = DepthClear{1};
        };

        struct StencilClear
        {
            // I'd like to set this to some value right here,
            //   but then `StencilInitialContents initial_contents;` doesn't find the default constructor of this type unless we make it user-declared,
            //   which makes us lose aggregate-ness, which isn't what I want. So we keep this default-constructed, and provide a default color below.
            std::uint8_t value;
        };
        struct StencilLoad {};
        struct StencilDontCare {};

        using StencilInitialContents = std::variant<StencilClear, StencilLoad, StencilDontCare>;

        struct StencilTarget
        {
            // Should we store the output?
            // Docs recommend false for oneshot stencil textures.
            bool store_output = false;

            StencilInitialContents initial_contents = StencilClear{0};
        };

        struct DepthStencil
        {
            Texture *texture = nullptr;

            DepthTarget depth;
            StencilTarget stencil;

            // Set to false if you need to load the existing image first.
            bool cycle = true;

            // SDL docs say that depth/stencil don't support multisample resolving, see: https://wiki.libsdl.org/SDL3/SDL_GPUDepthStencilTargetInfo
            // So there's no `multisample_resolved_texture` here.
        };

        struct Params
        {
            std::vector<ColorTarget> color_targets;

            // Adding `{}` to prevent Clang from warning when omitting an initializer for this in designated initialization.
            std::optional<DepthStencil> depth_stencil_target{};
        };

        RenderPass(CommandBuffer &command_buffer, const Params &params);

        RenderPass(RenderPass &&other) noexcept;
        RenderPass &operator=(RenderPass other) noexcept;
        ~RenderPass();

        [[nodiscard]] explicit operator bool() const {return bool(state.pass);}
        [[nodiscard]] SDL_GPURenderPass *Handle() {return state.pass;}


        struct Viewport
        {
            // Those are measured in PIXELS, with top-left corner being zero.
            // I assume those are framebuffer pixels, not window pixels.
            fvec2 pos;
            fvec2 size;

            // Those are separate. Firstly for convenience, and secondly in the SDL they are min/max unlike pos/size above, and we do it this way too.
            float min_depth = 0;
            float max_depth = 1;
        };

        // Set the active viewport coordinates.
        // This seems to be optional, it defaults to the entire framebuffer.
        void SetViewport(const Viewport &viewport);


        // Select the active pipeline.
        void BindPipeline(Pipeline &pipeline);


        struct VertexBuffer
        {
            Buffer *buffer = nullptr;

            std::uint32_t byte_offset = 0;
        };

        // Select what vertex buffer to use.
        void BindVertexBuffers(std::span<const VertexBuffer> buffers, std::uint32_t first_slot = 0);


        // Drawing:

        void DrawPrimitives(std::uint32_t num_vertices, std::uint32_t first_vertex = 0) {DrawPrimitivesInstanced(num_vertices, 1, first_vertex, 0);}
        void DrawPrimitivesInstanced(std::uint32_t num_vertices, std::uint32_t num_instances, std::uint32_t first_vertex = 0, std::uint32_t first_instance = 0);
    };
}
