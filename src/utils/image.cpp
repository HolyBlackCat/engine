#include "image.h"

#include <fmt/format.h>
#include <stb_image.h>

namespace em
{
    void Image::StbiDeleter::operator()(u8vec4 *ptr)
    {
        stbi_image_free(ptr);
    }

    Image::Image(std::string_view name, blob data)
    {
        ivec2 size;
        StbiUniquePtr u(reinterpret_cast<u8vec4 *>(stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(data.data()), int(data.size()), &size.x, &size.y, nullptr, 4)));
        if (!u)
            throw std::runtime_error(fmt::format("Failed to parse image: `{}`.", name));
        pixels = pixels_type(unsafe_mdarray_from_container{}, size, std::move(u));
    }
}
