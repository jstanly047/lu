#pragma once
#include <cstdint>
#include <concepts>
#include <limits>
#include <type_traits>

namespace lu::common
{
    template <unsigned ElementsPerCacheLine>
    struct GetCacheLineIndexBitsSize 
    {
        static constexpr unsigned value = ElementsPerCacheLine == 256u ? 8 :
                                    ElementsPerCacheLine == 128u ? 7 :
                                    ElementsPerCacheLine == 64u ? 6 :
                                    ElementsPerCacheLine == 32u ? 5 :
                                    ElementsPerCacheLine == 16u ? 4 :
                                    ElementsPerCacheLine == 8u ? 3 :
                                    ElementsPerCacheLine == 4u ? 2 :
                                    ElementsPerCacheLine == 2u ? 1 : 0;
    };

    template<bool Minimize_Contention, unsigned ArraySize, unsigned ElementsPerCacheLine>
    struct GetIndexShuffleBits 
    {
        static int constexpr indexBitsSize = GetCacheLineIndexBitsSize<ElementsPerCacheLine>::value;
        static unsigned constexpr minSize = 1u << (indexBitsSize * 2);
        static constexpr unsigned value = Minimize_Contention && ArraySize >= minSize ? indexBitsSize : 0;
    };

    template<int Bits>
    constexpr unsigned remapIndex(unsigned index) noexcept 
    {
        unsigned constexpr mixMask{(1u << Bits) - 1};
        unsigned const mix{(index ^ (index >> Bits)) & mixMask};
        return index ^ mix ^ (mix << Bits);
    }

    template<>
    constexpr unsigned remapIndex<0>(unsigned index) noexcept 
    {
        return index;
    }

    template<int Bits, class T>
    constexpr T& map(T* elements, unsigned index) noexcept 
    {
        return elements[remapIndex<Bits>(index)];
    }

    template <typename T>
    constexpr T nextPowerOfTwo(T n) 
    {
        //static constexpr T maxSupportedSize = std::numeric_limits<T>::max();
        //static_assert(maxSupportedSize > 0, "Integer type size is too large to handle.");

        //static_assert(n > 0 && n <= maxSupportedSize, "Input size is out of range.");

        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        if constexpr (sizeof(T) > 2) 
        {
            n |= n >> 16;

            if constexpr (sizeof(T) > 4)
            {
                n |= n >> 32;
            }
        }

        return ++n;
    }
}
