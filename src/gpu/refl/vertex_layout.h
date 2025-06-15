#pragma once

#include "em/math/vector_traits.h"
#include "em/meta/common.h"
#include "em/refl/classify.h"
#include "em/refl/visit_members.h"
#include "gpu/pipeline.h"

#include <concepts>

namespace em::Gpu
{
    // Converts a scalar or vector type to a suitable SDL vertex element format enum constant.
    template <Meta::cvref_unqualified T, bool Norm = false>
    constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt = SDL_GPU_VERTEXELEMENTFORMAT_INVALID;

    // Same, but raises a SFINAE error if the type is unsupported, instead of returning `SDL_GPU_VERTEXELEMENTFORMAT_INVALID`.
    template <Meta::cvref_unqualified T, bool Norm = false> requires(vertex_elem_format_for_type_opt<T, Norm> != SDL_GPU_VERTEXELEMENTFORMAT_INVALID)
    constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type = vertex_elem_format_for_type_opt<T, Norm>;


    // This is a reflection attribute used by `ReflectedVertexLayout` below.
    // This makes an integral attribute as "normalized" (to 0..1 or to -1..1 for signed and unsigned respectively).
    struct Norm : Refl::BasicAttribute {};

    // This converts to `Pipeline::VertexBuffer`, initializing the layout from the vertex type `VertexType`.
    // This isn't SFINAE-friendly yet.
    template <Refl::ClassifiesAs<Refl::Category::structure> VertexType>
    requires std::default_initializable<VertexType> // Need this to construct a dummy instance to compute the offsets from.
    struct ReflectedVertexLayout
    {
        // Propagates to `Pipeline::VertexBuffer::per_instance`.
        bool per_instance = false;

        [[nodiscard]] operator Pipeline::VertexBuffer() const
        {
            static const Pipeline::VertexBuffer basic_ret = []{
                Pipeline::VertexBuffer ret;
                ret.pitch = sizeof(VertexType);

                VertexType dummy_vertex;

                auto AppendAttr = [&]<typename VisitDesc, Meta::Deduce..., typename T>(const T &dummy_attr)
                {
                    ret.attributes.push_back({
                        .format = vertex_elem_format_for_type<T, Refl::Structs::member_has_attribute<typename VisitDesc::type, VisitDesc::value, Norm>>,
                        .byte_offset_in_elem = std::uint32_t(reinterpret_cast<const char *>(&dummy_attr) - reinterpret_cast<const char *>(&dummy_vertex)),
                    });
                };

                auto Recurse = [&]<typename VisitDesc, Meta::Deduce..., typename T>(this auto &&self, const T &target) -> void
                {
                    if constexpr (Refl::ClassifiesAs<T, Refl::Category::structure>)
                        Refl::VisitMembers<Meta::LoopSimple, {}, VisitDesc::mode>(target, self);
                    else
                        AppendAttr.template operator()<VisitDesc>(target);
                };

                Recurse.template operator()<Refl::VisitingOther>(dummy_vertex);
                return ret;
            }();

            Pipeline::VertexBuffer ret = basic_ret;
            ret.per_instance = per_instance;
            return ret;
        }
    };



    // Some specializations:
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4    ) && Math::signed_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 1) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_INT;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4 * 2) && Math::signed_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_INT2;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4 * 3) && Math::signed_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 3) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_INT3;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4 * 4) && Math::signed_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_INT4;

    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4    ) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 1) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4 * 2) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_UINT2;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4 * 3) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 3) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_UINT3;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == 4 * 4) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 32> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_UINT4;

    template <Meta::cvref_unqualified T> requires (sizeof(T) == sizeof(float)    ) && std::same_as<Math::vec_base_t<T>, float> && (Math::vec_size<T> == 1) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == sizeof(float) * 2) && std::same_as<Math::vec_base_t<T>, float> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == sizeof(float) * 3) && std::same_as<Math::vec_base_t<T>, float> && (Math::vec_size<T> == 3) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    template <Meta::cvref_unqualified T> requires (sizeof(T) == sizeof(float) * 4) && std::same_as<Math::vec_base_t<T>, float> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T> = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;

    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 2) && Math::signed_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_BYTE2_NORM : SDL_GPU_VERTEXELEMENTFORMAT_BYTE2;
    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 4) && Math::signed_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_BYTE4_NORM : SDL_GPU_VERTEXELEMENTFORMAT_BYTE4;

    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 2) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2_NORM : SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2;
    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 4) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM : SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;

    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 2 * 2) && Math::signed_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM : SDL_GPU_VERTEXELEMENTFORMAT_SHORT2;
    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 2 * 4) && Math::signed_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM : SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;

    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 2 * 2) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 2) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM : SDL_GPU_VERTEXELEMENTFORMAT_USHORT2;
    template <Meta::cvref_unqualified T, bool Norm> requires (sizeof(T) == 2 * 4) && Math::unsigned_scalar_bits<Math::vec_base_t<T>, 8> && (Math::vec_size<T> == 4) constexpr SDL_GPUVertexElementFormat vertex_elem_format_for_type_opt<T, Norm> = Norm ? SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM : SDL_GPU_VERTEXELEMENTFORMAT_USHORT4;

    // Not implemented yet: SDL_GPU_VERTEXELEMENTFORMAT_HALF2, SDL_GPU_VERTEXELEMENTFORMAT_HALF4
    // Waiting for universal `std::float16_t` support...
}
