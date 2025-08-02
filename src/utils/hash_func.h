#pragma once

#include "utils/byte_view.h"

#include <cstddef>
#include <cstdint>

namespace em
{
    using hash_t = std::uint32_t;

    // A constexpr implementation of MurmurHash3, adapted from `https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp`.
    [[nodiscard]] constexpr hash_t Hash(const_byte_view bytes, hash_t seed = 0)
    {
        const std::size_t nblocks = bytes.size() / 4;

        std::uint32_t h1 = seed;

        constexpr std::uint32_t c1 = 0xcc9e2d51;
        constexpr std::uint32_t c2 = 0x1b873593;

        for (std::size_t i = 0; i < nblocks; i++)
        {
            std::uint32_t k1 = (std::uint32_t)(std::uint8_t)bytes[i*4+0] << 0
                             | (std::uint32_t)(std::uint8_t)bytes[i*4+1] << 8
                             | (std::uint32_t)(std::uint8_t)bytes[i*4+2] << 16
                             | (std::uint32_t)(std::uint8_t)bytes[i*4+3] << 24;

            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> (32 - 15));
            k1 *= c2;

            h1 ^= k1;
            h1 = (h1 << 13) | (h1 >> (32 - 13));
            h1 = h1 * 5 + 0xe6546b64;
        }

        const char *tail = bytes.data() + nblocks * 4;

        std::uint32_t k1 = 0;

        switch (bytes.size() & 3)
        {
          case 3:
            k1 ^= (std::uint32_t)(std::uint8_t)tail[2] << 16;
            [[fallthrough]];
          case 2:
            k1 ^= (std::uint32_t)(std::uint8_t)tail[1] << 8;
            [[fallthrough]];
          case 1:
            k1 ^= (std::uint32_t)(std::uint8_t)tail[0];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> (32 - 15));
            k1 *= c2; h1 ^= k1;
        };

        h1 ^= bytes.size();

        h1 ^= h1 >> 16;
        h1 *= 0x85ebca6b;
        h1 ^= h1 >> 13;
        h1 *= 0xc2b2ae35;
        h1 ^= h1 >> 16;
        return h1;
    }
}
