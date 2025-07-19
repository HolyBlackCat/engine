#pragma once

#include "em/math/robust.h"
#include "em/zstring_view.h"

#include <fmt/format.h>
#include <SDL3/SDL_properties.h>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace em
{
    // Allows both integers and unscoped enums, since the SDL enums are all unscoped. Maybe we could allow scoped enums too later.
    template <typename T>
    concept SdlProperties_IntegerLike = std::integral<T> || (std::is_enum_v<T> && !std::is_scoped_enum_v<T>);
    template <typename T>
    concept SdlProperties_StringLike = Meta::same_as_any<std::string, std::string_view, zstring_view, const char *>;
    template <typename T>
    concept SdlProperties_Pointer = Meta::cv_unqualified<T> && std::is_pointer_v<T>;

    // All property types supported by `SdlProperties`.
    template <typename T>
    concept SdlProperties_SupportedType = Meta::same_as_any<T, bool, float> || std::integral<T> || SdlProperties_StringLike<T> || SdlProperties_Pointer<T>;

    // Wraps `SDL_PropertiesID`, which is basically a map with string keys and variant values.
    class SdlProperties
    {
        SDL_PropertiesID handle{};

        // This should be ignored when the handle is zero, but we still try to set it to false in that case.
        bool owning = false;

        // Converts an integer-like value to the integer type that SDL uses.
        template <SdlProperties_IntegerLike T>
        [[nodiscard]] static std::int64_t MakeInteger(T value)
        {
            if constexpr (std::is_enum_v<T>)
                return Math::Robust::cast<std::int64_t>(std::to_underlying(value));
            else
                return Math::Robust::cast<std::int64_t>(value);
        }

      public:
        // Creates a null instance.
        SdlProperties() {}

        // Creates a valid but empty instance.
        SdlProperties(std::nullptr_t);

        struct ViewExternalHandle {explicit ViewExternalHandle() = default;};
        // Creates a view to an existing handle.
        SdlProperties(ViewExternalHandle, SDL_PropertiesID new_handle) : handle(new_handle), owning(false) {}

        SdlProperties(const SdlProperties &other);
        SdlProperties(SdlProperties &&other) noexcept;
        SdlProperties &operator=(SdlProperties other) noexcept;
        ~SdlProperties();

        [[nodiscard]] explicit operator bool() const {return bool(handle);}
        [[nodiscard]] SDL_PropertiesID Handle() const {return handle;}

        // Throw if this is a null instance.
        void ThrowIfNull() const;
        // If this instance is null, initializes it to be empty. Otherwise no effect.
        void InitializeIfNull();

        // Returns true if we have a property with this name.
        [[nodiscard]] bool Has(zstring_view name) const;
        // Throws if `Has(name)` is false.
        void ThrowIfMissing(zstring_view name) const;


        // This version throws if the property is missing.
        template <SdlProperties_SupportedType T>
        [[nodiscard]] T Get(zstring_view name)
        {
            ThrowIfMissing(name);
            return Get<T>(name, {});
        }

        // Those versions return the default value if the property is missing (and also when the object is null, which SDL seems to handle automatically).
        // Note that here the dummy template arguments are NOT used as the type of `default_value`, to avoid unwanted deduction.
        // Also note that SDL seems to apply some implicit conversions on type mismatch.
        // Sync the list of supported types with `SdlProperties_SupportedType`.
        template <std::same_as<bool>         > [[nodiscard]] bool  Get(zstring_view name, bool                    default_value) const {return                       SDL_GetBooleanProperty(handle, name.c_str(), default_value);}
        template <std::same_as<float>        > [[nodiscard]] float Get(zstring_view name, float                   default_value) const {return                       SDL_GetFloatProperty  (handle, name.c_str(), default_value);}
        template <SdlProperties_IntegerLike T> [[nodiscard]] T     Get(zstring_view name, std::type_identity_t<T> default_value) const {return Math::Robust::cast<T>(SDL_GetNumberProperty (handle, name.c_str(), default_value));}
        template <SdlProperties_StringLike  T> [[nodiscard]] T     Get(zstring_view name, zstring_view            default_value) const {return T                    (SDL_GetStringProperty (handle, name.c_str(), default_value.c_str()));}
        template <SdlProperties_Pointer     T> [[nodiscard]] T     Get(zstring_view name, std::type_identity_t<T> default_value) const {return T                    (SDL_GetPointerProperty(handle, name.c_str(), default_value));}

        // The setters.
        // Note that those automatically initialize the instance if it's null.
                                               void Set(zstring_view name, bool         value) {InitializeIfNull(); SDL_SetBooleanProperty(handle, name.c_str(), value);}
                                               void Set(zstring_view name, float        value) {InitializeIfNull(); SDL_SetFloatProperty  (handle, name.c_str(), value);}
        template <SdlProperties_IntegerLike T> void Set(zstring_view name, T            value) {InitializeIfNull(); SDL_SetNumberProperty (handle, name.c_str(), value);}
        template <SdlProperties_StringLike  T> void Set(zstring_view name, zstring_view value) {InitializeIfNull(); SDL_SetStringProperty (handle, name.c_str(), value.c_str());}
        template <SdlProperties_Pointer     T> void Set(zstring_view name, T            value) {InitializeIfNull(); SDL_SetPointerProperty(handle, name.c_str(), (void *)value);} // Need the cast because the input pointer could be const.
    };
}
