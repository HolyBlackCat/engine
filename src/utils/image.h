#pragma once

#include "em/math/vector.h"
#include "utils/filesystem.h"
#include "utils/mdarray.h"

#include <string_view>

// This file is in `utils/` rather than `graphics/`, because `gpu/` depends on it.

namespace em
{
    class Image
    {
        struct StbiDeleter
        {
            void operator()(u8vec4 *ptr);
        };
        using StbiUniquePtr = std::unique_ptr<u8vec4[], StbiDeleter>;

      public:
        using pixels_type = basic_mdarray<StbiUniquePtr, ivec2>;

        pixels_type pixels;

        constexpr Image() {}

        // Loads the image from a blob (that can use the common file formats, such as PNG).
        // Throws on failure.
        Image(std::string_view name, const blob_or_file &data);
    };
}
