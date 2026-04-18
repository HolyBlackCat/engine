#pragma once

#include "em/math/vector.h"

#include <SDL3/SDL_gpu.h>

#include <optional>

typedef struct SDL_GPUSampler SDL_GPUSampler;

namespace em::Gpu
{
    class Device;

    // Stores texture access settings, but doesn't store the actual texture.
    class Sampler
    {
      public:
        enum class Filter
        {
            nearest = SDL_GPU_FILTER_NEAREST,
            linear  = SDL_GPU_FILTER_LINEAR,
        };

        enum class Wrap
        {
            repeat          = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            mirrored_repeat = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
            clamp           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        };

      private:
        struct State
        {
            // Not a `Device *` to keep the address stable.
            SDL_GPUDevice *device = nullptr;
            SDL_GPUSampler *sampler = nullptr;
        };
        State state;

      public:
        constexpr Sampler() {}

        struct Mipmap
        {
            // Mipmap filter.
            Filter filter = Filter::nearest;

            // Some other magic.
            float lod_bias = 0;
            float lod_min = 0;
            float lod_max = 0;
        };

        struct Params
        {
            // Downscale filter.
            Filter filter_min = Filter::linear;
            // Upscale filter.
            Filter filter_mag = Filter::linear;

            // How to wrap the coordinates.
            vec3<Wrap> wrap = vec3<Wrap>(Wrap::repeat);

            // Mipmaps.
            // Note that SDL doesn't have a bool to completely disable them, so when this is null, we just set the mipmap parameters to zeroes.
            std::optional<Mipmap> mipmap{};

            // This is apparently sometimes useful for implementing shadow mapping.
            // If the sampled texture is a depth texture, you can enable this, and then you must also change the shader to use `sampler2DShadow` and `shadow2D()`
            //   instead of `sampler2D` and `texture2D()` respectively. `sampler2D()` takes an extra third texture coordinate,
            //   which is compared against the depth value in the texture (the coordinate is the lhs, and the texture contents are the rhs),
            //   and then returns either `1.0` or `0.0` to indicate the result (possibly something in between, when using linear filtering).
            // But you can also sample depth textures normally, instead of doing this.
            std::optional<SDL_GPUCompareOp> compare_mode{};

            // Setting this enables anisotropic filtering.
            std::optional<float> anisotropy_max{};
        };

        Sampler(Device &device, const Params &params);

        Sampler(Sampler &&other) noexcept;
        Sampler &operator=(Sampler other) noexcept;
        ~Sampler();

        [[nodiscard]] explicit operator bool() const {return bool(state.sampler);}
        [[nodiscard]] SDL_GPUSampler *Handle() {return state.sampler;}
    };
}
