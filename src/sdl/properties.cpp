#include "properties.h"

#include <fmt/format.h>

#include <stdexcept>

namespace em
{
    SdlProperties::SdlProperties(std::nullptr_t)
        : SdlProperties() // Ensure cleanup on throw (isn't necessary at the moment).
    {
        handle = SDL_CreateProperties();
        if (!handle)
            throw std::runtime_error(fmt::format("Unable to create a SDL properties instance: {}", SDL_GetError()));
        owning = true;
    }

    SdlProperties::SdlProperties(const SdlProperties &other)
        : SdlProperties(nullptr) // Construct an instance.
    {
        if (!SDL_CopyProperties(other.handle, handle))
            throw std::runtime_error(fmt::format("Unable to copy SDL properties: {}", SDL_GetError()));
    }

    SdlProperties::SdlProperties(SdlProperties &&other) noexcept
        : handle(other.handle), owning(other.owning)
    {
        other.handle = {};
        other.owning = false;
    }

    SdlProperties &SdlProperties::operator=(SdlProperties other) noexcept
    {
        std::swap(handle, other.handle);
        std::swap(owning, other.owning);
        return *this;
    }

    SdlProperties::~SdlProperties()
    {
        if (owning && handle)
            SDL_DestroyProperties(handle);
    }

    void SdlProperties::ThrowIfNull() const
    {
        if (!*this)
            throw std::runtime_error("This `SdlProperties` instance is null.");
    }

    void SdlProperties::InitializeIfNull()
    {
        if (!*this)
            *this = nullptr;
    }

    bool SdlProperties::Has(zstring_view name) const
    {
        return SDL_HasProperty(handle, name.c_str());
    }

    void SdlProperties::ThrowIfMissing(zstring_view name) const
    {
        if (!Has(name))
            throw std::runtime_error(fmt::format("Missing property: `{}`.", name));
    }
}
