#pragma once

#include <gtl/vector.hpp>

#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace em
{
    // This tells `basic_mdarray` how to work with a custom backing container.
    template <typename T>
    struct mdarray_container_traits
    {
        using value_type = typename T::value_type;
        static constexpr T construct(std::size_t n) {return T(n);}
        static constexpr value_type *get_ptr(T &c) {return c.data();}
        static constexpr void clear(T &c) {c.clear();}
        static constexpr bool stores_size = true; // Then `.size()` is required.
    };
    template <typename E, typename D>
    struct mdarray_container_traits<std::unique_ptr<E[], D>>
    {
        using value_type = E;
        static constexpr std::unique_ptr<E, D> construct(std::size_t n) {return std::make_unique<E[]>(n);}
        static constexpr value_type *get_ptr(std::unique_ptr<E[], D> &c) {return c.get();}
        static constexpr void clear(std::unique_ptr<E[], D> &c) {c = nullptr;}
        static constexpr bool stores_size = false;
    };

    // This tag is used to construct `[basic_]mdarray` directly from the underlying container, which is unsafe if you get the size wrong.
    struct unsafe_mdarray_from_container {explicit unsafe_mdarray_from_container() = default;};

    // Describes an N-dimensional array. Prefer the higher-level typedef `mdarray<...>` declared below.
    // `C` is the underlying `std::vector`-like container. Can also be `std::unique_ptr<T[]>`. We deduce the element type from this template argument.
    // `I` is the index type, e.g. `ivecN` or `std::array`. Must be tuple-like, normally with all same element types.
    template <typename C, typename I>
    requires requires
    {
        std::tuple_size<I>::value; // Check that `I` is tuple-like. Note that `std::tuple_size_v` is not SFINAE-friendly.
    }
    class basic_mdarray
    {
      public:
        using underlying_container_type = C;
        using container_traits = mdarray_container_traits<C>;
        using index_type = I;
        using value_type = typename container_traits::value_type;

      private:
        index_type size_vec{};
        underlying_container_type container;

        static constexpr std::size_t SizeProd(const index_type &size)
        {
            std::size_t ret = 1;
            // Can't use `std::apply` because we want to support custom tuple-like types.
            [&]<std::size_t ...IndexI>(std::index_sequence<IndexI...>){
                using std::get; // Allow custom tuple-like types.
                ((void(ret *= std::size_t(get<IndexI>(size)))), ...);
            }(std::make_index_sequence<std::tuple_size_v<I>>{});
            return ret;
        }

        constexpr std::size_t ToLinearIndex(const index_type &index)
        {
            std::size_t ret = 0;
            std::size_t multiplier = 1;
            [&]<std::size_t ...IndexI>(std::index_sequence<IndexI...>){
                using std::get; // Allow custom tuple-like types.
                ((void(ret += std::size_t(get<IndexI>(index)) * multiplier), void(multiplier *= std::size_t(get<IndexI>(size_vec)))), ...);
            }(std::make_index_sequence<std::tuple_size_v<I>>{});
            return ret;
        }

      public:
        // Construct empty.
        [[nodiscard]] constexpr basic_mdarray() {}

        // Construct from a size.
        [[nodiscard]] constexpr basic_mdarray(index_type size) : size_vec(std::move(size)), container(container_traits::construct(SizeProd(size))) {}

        // Construct unsafely from the underlying container.
        // If this container type knows its size, throws if the size is wrong (doesn't match the multidimensonal size specified).
        [[nodiscard]] constexpr basic_mdarray(unsafe_mdarray_from_container, index_type size, underlying_container_type container)
            : size_vec(std::move(size)), container(std::move(container))
        {
            if constexpr (container_traits::stores_size)
            {
                if (flat_size() != SizeProd(size_vec))
                    throw std::logic_error("`basic_mdarray`: Constructed from a container of a wrong size.");
            }
        }

        basic_mdarray(const basic_mdarray &other) = default;
        basic_mdarray &operator=(const basic_mdarray &other) = default;

        // Need custom move operations to ensure that moved-from arrays are properly empty.

        constexpr basic_mdarray(basic_mdarray &&other) noexcept
            : size_vec(other.size_vec), container(std::move(other.container))
        {
            other.size_vec = {};
            container_traits::clear(other.container);
        }

        constexpr basic_mdarray &operator=(basic_mdarray &&other) noexcept
        {
            size_vec = std::move(other.size_vec);
            container = std::move(other.container);

            other.size_vec = {};
            container_traits::clear(other.container);
            return *this;
        }


        // Returns the size vector.
        [[nodiscard]] constexpr index_type size() const {return size_vec;}

        // Returns the flat size of the underlying container.
        [[nodiscard]] constexpr std::size_t flat_size() const
        {
            if constexpr (container_traits::stores_size)
                return container.size();
            else
                return SizeProd(size_vec);
        }

        // Returns the underlying elements.
        [[nodiscard]] constexpr std::span<value_type> as_flat_array()
        {
            if constexpr (container_traits::stores_size)
                return container;
            else
                return std::span<value_type>(container_traits::get_ptr(container), flat_size());
        }
        [[nodiscard]] constexpr const std::span<const value_type> as_flat_array() const
        {
            return const_cast<basic_mdarray &>(*this).as_flat_array();
        }

        [[nodiscard]] constexpr value_type &operator[](const index_type &index) &
        {
            std::size_t i = ToLinearIndex(index);
            assert(i < flat_size());
            return container[i];
        }

        [[nodiscard]] constexpr const value_type &operator[](const index_type &index) const &
        {
            return const_cast<basic_mdarray &>(*this)[index];
        }
        [[nodiscard]] constexpr value_type &&operator[](const index_type &index) &&
        {
            return std::move((*this)[index]);
        }
        [[nodiscard]] constexpr const value_type &&operator[](const index_type &index) const &&
        {
            return std::move((*this)[index]);
        }
    };

    template <typename T>
    struct SelectDefaultVectorTypeFormdarray {using type = std::vector<T>;};
    template <>
    struct SelectDefaultVectorTypeFormdarray<bool> {using type = gtl::vector<bool>;};

    // A multidimensional array backed by `std::vector<T>` (or for booleans `gtl::vector<bool>`).
    template <typename T, typename I>
    using mdarray = basic_mdarray<typename SelectDefaultVectorTypeFormdarray<T>::type, I>;
}
