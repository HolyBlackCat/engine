#pragma once

#include <SDL3/SDL_gpu.h>

namespace em::Gpu
{
    enum class MultisampleSamples
    {
        // The values here don't match the names, so you can't directly cast back and forth.
        _1 = SDL_GPU_SAMPLECOUNT_1,
        _2 = SDL_GPU_SAMPLECOUNT_2,
        _4 = SDL_GPU_SAMPLECOUNT_4,
        _8 = SDL_GPU_SAMPLECOUNT_8,
    };
}
