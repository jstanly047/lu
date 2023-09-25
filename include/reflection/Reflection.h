#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

#ifndef LU_REFLECTION_AUTODETECT_MEMBERS_MODE
#define LU_REFLECTION_AUTODETECT_MEMBERS_MODE (0)
#endif

#ifndef LU_REFLECTION_INLINE
#if defined __clang__ || defined __GNUC__
#define LU_REFLECTION_INLINE __attribute__((always_inline))
#if defined __clang__
#define LU_REFLECTION_CONSTEXPR_INLINE_LAMBDA __attribute__((always_inline)) constexpr
#else
#define LU_REFLECTION_CONSTEXPR_INLINE_LAMBDA constexpr __attribute__((always_inline))
#endif
#elif defined _MSC_VER
#define LU_REFLECTION_INLINE [[msvc::forceinline]]
#define LU_REFLECTION_CONSTEXPR_INLINE_LAMBDA /*constexpr*/ [[msvc::forceinline]]
#endif
#else // LU_REFLECTION_INLINE
#define LU_REFLECTION_CONSTEXPR_INLINE_LAMBDA constexpr
#endif // LU_REFLECTION_INLINE

#if defined LU_REFLECTION_INLINE_MODE && !LU_REFLECTION_INLINE_MODE
#undef LU_REFLECTION_INLINE
#define LU_REFLECTION_INLINE
#undef LU_REFLECTION_CONSTEXPR_INLINE_LAMBDA
#define LU_REFLECTION_CONSTEXPR_INLINE_LAMBDA constexpr
#endif

#ifndef LU_REFLECTION_INLINE_DECODE_VARINT
#define LU_REFLECTION_INLINE_DECODE_VARINT (0)
#endif

#define PARENS () 

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define CONV_TO_STRING(str)  #str
#define CREAT_LIST_HELPER(str, ...) \
        CONV_TO_STRING(str) \
        __VA_OPT__(, CREAT_LIST_AGAIN PARENS (__VA_ARGS__))
#define CREAT_LIST_AGAIN() CREAT_LIST_HELPER
#define CREAT_LIST(...) \
    __VA_OPT__(EXPAND(CREAT_LIST_HELPER(__VA_ARGS__)))

#define CREAT_STRING_LIST_HELPER(sep, prefix, postfix, str, ...) \
    prefix CONV_TO_STRING(str) postfix \
    __VA_OPT__(sep CREAT_STRING_LIST_AGAIN PARENS (sep, prefix, postfix, __VA_ARGS__))
#define CREAT_STRING_LIST_AGAIN() CREAT_STRING_LIST_HELPER
#define CREAT_STRING_LIST(sep, prefix, postfix, ...) \
    __VA_OPT__(EXPAND(CREAT_STRING_LIST_HELPER(sep, prefix, postfix, __VA_ARGS__)))

#define CONCAT_REC_HELPER(str, ...) \
    str \
    __VA_OPT__(CONCAT_REC_AGAIN PARENS (__VA_ARGS__))
#define CONCAT_REC_AGAIN() CONCAT_REC_HELPER
#define CONCAT_REC(...) \
    __VA_OPT__(EXPAND(CONCAT_REC_HELPER(__VA_ARGS__)))


#define MACRO_CONCAT(A, B) A##_##B

#define MAKE_ARRAY_IMPL(STRUCT_NAME, N, ...) \
    constexpr std::array<const char*, N> arr_##STRUCT_NAME = { CREAT_LIST(...)}

#define MAKE_ARRAY(STRUCT_NAME, ...)\
    MAKE_ARRAY_IMPL(STRUCT_NAME, NUMBER_OF_MEMBERS<STRUCT_NAME>, __VA_ARGS__) 

namespace lu::reflection
{
    using default_size_type = std::uint32_t;

    enum class kind
    {
        in,
        out
    };

    template <std::size_t Count = std::numeric_limits<std::size_t>::max()>
    struct members
    {
        constexpr static std::size_t value = Count;
    };

    template <auto Protocol, std::size_t Members = std::numeric_limits<std::size_t>::max()>
    struct protocol
    {
        constexpr static auto value = Protocol;
        constexpr static auto members = Members;
    };

    template <auto Id>
    struct serialization_id
    {
        constexpr static auto value = Id;
    };

    constexpr auto success(std::errc code)
    {
        return std::errc{} == code;
    }

    constexpr auto failure(std::errc code)
    {
        return std::errc{} != code;
    }

    struct [[nodiscard]] errc
    {
        constexpr errc(std::errc code = {}) : code(code)
        {
        }

        constexpr operator std::errc() const
        {
            return code;
        }

        constexpr void or_throw() const
        {
            if (failure(code)) [[unlikely]] {
    #ifdef __cpp_exceptions
                throw std::system_error(std::make_error_code(code));
    #else
                std::abort();
    #endif
            }
        }

        std::errc code;
    };

    constexpr auto success(errc code)
    {
        return std::errc{} == code;
    }

    constexpr auto failure(errc code)
    {
        return std::errc{} != code;
    }

    struct access
    {
        struct any
        {
            template <typename Type>
            operator Type();
        };

        template <typename Item>
        constexpr static auto make(auto &&... arguments)
        {
            return Item{std::forward<decltype(arguments)>(arguments)...};
        }

        template <typename Item>
        constexpr static auto placement_new(void * address,
                                            auto &&... arguments)
        {
            return ::new (address)
                Item(std::forward<decltype(arguments)>(arguments)...);
        }

        template <typename Item>
        constexpr static auto make_unique(auto &&... arguments)
        {
            return std::unique_ptr<Item>(
                new Item(std::forward<decltype(arguments)>(arguments)...));
        }

        template <typename Item>
        constexpr static void destruct(Item & item)
        {
            item.~Item();
        }

        template <typename Type>
        constexpr static auto number_of_members();

        constexpr static auto max_visit_members = 50;

        LU_REFLECTION_INLINE constexpr static decltype(auto) visit_members(
            auto && object,
            auto && visitor) requires(0 <=
                                    number_of_members<decltype(object)>()) &&
            (number_of_members<decltype(object)>() <= max_visit_members)
        {
            constexpr auto count = number_of_members<decltype(object)>();

            // clang-format off
            if constexpr (count == 0) { return visitor(); } else if constexpr (count == 1) { auto && [a1] = object; return visitor(a1); } else if constexpr (count == 2) { auto && [a1, a2] = object; return visitor(a1, a2); /*......................................................................................................................................................................................................................................................................*/ } else if constexpr (count == 3) { auto && [a1, a2, a3] = object; return visitor(a1, a2, a3); } else if constexpr (count == 4) { auto && [a1, a2, a3, a4] = object; return visitor(a1, a2, a3, a4); } else if constexpr (count == 5) { auto && [a1, a2, a3, a4, a5] = object; return visitor(a1, a2, a3, a4, a5); } else if constexpr (count == 6) { auto && [a1, a2, a3, a4, a5, a6] = object; return visitor(a1, a2, a3, a4, a5, a6); } else if constexpr (count == 7) { auto && [a1, a2, a3, a4, a5, a6, a7] = object; return visitor(a1, a2, a3, a4, a5, a6, a7); } else if constexpr (count == 8) { auto && [a1, a2, a3, a4, a5, a6, a7, a8] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8); } else if constexpr (count == 9) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9); } else if constexpr (count == 10) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); } else if constexpr (count == 11) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); } else if constexpr (count == 12) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); } else if constexpr (count == 13) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13); } else if constexpr (count == 14) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14); } else if constexpr (count == 15) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15); } else if constexpr (count == 16) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16); } else if constexpr (count == 17) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17); } else if constexpr (count == 18) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18); } else if constexpr (count == 19) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19); } else if constexpr (count == 20) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20); } else if constexpr (count == 21) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21); } else if constexpr (count == 22) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22); } else if constexpr (count == 23) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23); } else if constexpr (count == 24) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24); } else if constexpr (count == 25) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25); } else if constexpr (count == 26) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26); } else if constexpr (count == 27) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27); } else if constexpr (count == 28) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28); } else if constexpr (count == 29) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29); } else if constexpr (count == 30) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30); } else if constexpr (count == 31) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31); } else if constexpr (count == 32) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32); } else if constexpr (count == 33) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33); } else if constexpr (count == 34) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34); } else if constexpr (count == 35) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35); } else if constexpr (count == 36) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36); } else if constexpr (count == 37) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37); } else if constexpr (count == 38) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38); } else if constexpr (count == 39) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39); } else if constexpr (count == 40) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40); } else if constexpr (count == 41) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41); } else if constexpr (count == 42) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42); } else if constexpr (count == 43) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43); } else if constexpr (count == 44) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44); } else if constexpr (count == 45) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45); } else if constexpr (count == 46) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46); } else if constexpr (count == 47) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47); } else if constexpr (count == 48) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48); } else if constexpr (count == 49) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49); } else if constexpr (count == 50) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50] = object; return visitor(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50);
                // Calls the visitor above with all data members of object.
            }
            // clang-format on
        }

        template <typename Type>
            constexpr static decltype(auto)
            visit_members_types(auto && visitor) requires(0 <= number_of_members<Type>()) &&
            (number_of_members<Type>() <= max_visit_members)

        {
            using type = std::remove_cvref_t<Type>;
            constexpr auto count = number_of_members<Type>();

            // clang-format off
            if constexpr (count == 0) { return visitor.template operator()<>(); } else if constexpr (count == 1) { auto f = [&](auto && object) { auto && [a1] = object; return visitor.template operator()<decltype(a1)>(); }; /*......................................................................................................................................................................................................................................................................*/ return decltype(f(std::declval<type>()))(); } else if constexpr (count == 2) { auto f = [&](auto && object) { auto && [a1, a2] = object; return visitor.template operator()<decltype(a1), decltype(a2)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 3) { auto f = [&](auto && object) { auto && [a1, a2, a3] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 4) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 5) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 6) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 7) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 8) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 9) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 10) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 11) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 12) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 13) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 14) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 15) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 16) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 17) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 18) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 19) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 20) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 21) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 22) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 23) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 24) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 25) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 26) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 27) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 28) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 29) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 30) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 31) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 32) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 33) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 34) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 35) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 36) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 37) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 38) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 39) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 40) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 41) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 42) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 43) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 44) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 45) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 46) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 47) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 48) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47), decltype(a48)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 49) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47), decltype(a48), decltype(a49)>(); }; return decltype(f(std::declval<type>()))(); } else if constexpr (count == 50) { auto f = [&](auto && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50] = object; return visitor.template operator()<decltype(a1), decltype(a2), decltype(a3), decltype(a4), decltype(a5), decltype(a6), decltype(a7), decltype(a8), decltype(a9), decltype(a10), decltype(a11), decltype(a12), decltype(a13), decltype(a14), decltype(a15), decltype(a16), decltype(a17), decltype(a18), decltype(a19), decltype(a20), decltype(a21), decltype(a22), decltype(a23), decltype(a24), decltype(a25), decltype(a26), decltype(a27), decltype(a28), decltype(a29), decltype(a30), decltype(a31), decltype(a32), decltype(a33), decltype(a34), decltype(a35), decltype(a36), decltype(a37), decltype(a38), decltype(a39), decltype(a40), decltype(a41), decltype(a42), decltype(a43), decltype(a44), decltype(a45), decltype(a46), decltype(a47), decltype(a48), decltype(a49), decltype(a50)>(); }; return decltype(f(std::declval<type>()))();
                // Returns visitor.template operator()<member-types...>();
            }
            // clang-format on
        }

        constexpr static auto try_serialize(auto && item)
        {
            if constexpr (requires { serialize(item); }) {
                return serialize(item);
            }
        }

        template <typename Type, typename Archive>
        constexpr static auto has_serialize()
        {
            return requires {
                    requires std::same_as<
                        typename std::remove_cvref_t<Type>::serialize,
                        members<
                            std::remove_cvref_t<Type>::serialize::value>>;
                } ||
                requires(Type && item) {
                    requires std::same_as<
                        std::remove_cvref_t<decltype(try_serialize(
                            item))>,
                        members<std::remove_cvref_t<
                            decltype(try_serialize(item))>::value>>;
                } ||
                requires {
                    requires std::same_as<
                        typename std::remove_cvref_t<Type>::serialize,
                        protocol<
                            std::remove_cvref_t<Type>::serialize::value,
                            std::remove_cvref_t<Type>::serialize::members>>;
                } ||
                requires(Type && item) {
                    requires std::same_as<
                        std::remove_cvref_t<decltype(try_serialize(
                            item))>,
                        protocol<
                            std::remove_cvref_t<decltype(try_serialize(
                                item))>::value,
                            std::remove_cvref_t<decltype(try_serialize(
                                item))>::members>>;
                } ||
                requires(Type && item, Archive && archive) {
                    std::remove_cvref_t<Type>::serialize(archive, item);
                } || requires(Type && item, Archive && archive) {
                            serialize(archive, item);
                        };
        }

        template <typename Type, typename Archive>
        constexpr static auto has_explicit_serialize()
        {
            return requires(Type && item, Archive && archive)
            {
                std::remove_cvref_t<Type>::serialize(archive, item);
            }
            || requires(Type && item, Archive && archive)
            {
                serialize(archive, item);
            };
        }

        template <typename Type>
        struct byte_serializable_visitor;

        template <typename Type>
        constexpr static auto byte_serializable();

        template <typename Type>
        struct endian_independent_byte_serializable_visitor;

        template <typename Type>
        constexpr static auto endian_independent_byte_serializable();

        template <typename Type, typename Self, typename... Visited>
        struct self_referencing_visitor;

        template <typename Type, typename Self = Type, typename... Visited>
        constexpr static auto self_referencing();

        template <typename Type>
        constexpr static auto has_protocol()
        {
            return requires
            {
                requires std::same_as<
                    typename std::remove_cvref_t<Type>::serialize,
                    protocol<std::remove_cvref_t<Type>::serialize::value,
                            std::remove_cvref_t<Type>::serialize::members>>;
            }
            || requires(Type && item)
            {
                requires std::same_as<
                    std::remove_cvref_t<decltype(try_serialize(item))>,
                    protocol<
                        std::remove_cvref_t<decltype(try_serialize(item))>::value,
                        std::remove_cvref_t<decltype(try_serialize(
                            item))>::members>>;
            };
        }

        template <typename Type>
        constexpr static auto get_protocol()
        {
            if constexpr (
                requires {
                    requires std::same_as<
                        typename std::remove_cvref_t<Type>::serialize,
                        protocol<
                            std::remove_cvref_t<Type>::serialize::value,
                            std::remove_cvref_t<Type>::serialize::members>>;
                }) {
                return std::remove_cvref_t<Type>::serialize::value;
            } else if constexpr (
                requires(Type && item) {
                    requires std::same_as<
                        std::remove_cvref_t<decltype(try_serialize(item))>,
                        protocol<std::remove_cvref_t<decltype(try_serialize(
                                    item))>::value,
                                std::remove_cvref_t<decltype(try_serialize(
                                    item))>::members>>;
                }) {
                return std::remove_cvref_t<decltype(try_serialize(
                    std::declval<Type>()))>::value;
            } else {
                static_assert(!sizeof(Type));
            }
        }
    };

    template <typename Type>
    struct destructor_guard
    {
        LU_REFLECTION_INLINE constexpr ~destructor_guard()
        {
            access::destruct(object);
        }

        Type & object;
    };

    template <typename Type>
    destructor_guard(Type) -> destructor_guard<Type>;

    namespace traits
    {
        template <typename Type>
        struct is_unique_ptr : std::false_type
        {
        };

        template <typename Type>
        struct is_unique_ptr<std::unique_ptr<Type, std::default_delete<Type>>>
            : std::true_type
        {
        };

        template <typename Type>
        struct is_shared_ptr : std::false_type
        {
        };

        template <typename Type>
        struct is_shared_ptr<std::shared_ptr<Type>> : std::true_type
        {
        };

        template <typename Variant>
        struct variant_impl;

        template <typename... Types, template <typename...> typename Variant>
        struct variant_impl<Variant<Types...>>
        {
            using variant_type = Variant<Types...>;

            template <std::size_t Index,
                    std::size_t CurrentIndex,
                    typename FirstType,
                    typename... OtherTypes>
            constexpr static auto get_id()
            {
                if constexpr (Index == CurrentIndex) {
                    if constexpr (requires {
                                    requires std::same_as<
                                        serialization_id<
                                            FirstType::serialize_id::value>,
                                    typename FirstType::serialize_id>;
                                }) {
                        return FirstType::serialize_id::value;
                    } else if constexpr (
                        requires {
                            requires std::same_as<
                                serialization_id<decltype(serialize_id(
                                    std::declval<FirstType>()))::value>,
                            decltype(serialize_id(std::declval<FirstType>()))>;
                        }) {
                        return decltype(serialize_id(
                            std::declval<FirstType>()))::value;
                    } else {
                        return std::byte{Index};
                    }
                } else {
                    return get_id<Index, CurrentIndex + 1, OtherTypes...>();
                }
            }

            template <std::size_t Index>
            constexpr static auto id()
            {
                return get_id<Index, 0, Types...>();
            }

            template <std::size_t CurrentIndex = 0>
            LU_REFLECTION_INLINE constexpr static auto id(auto index)
            {
                if constexpr (CurrentIndex == (sizeof...(Types) - 1)) {
                    return id<CurrentIndex>();
                } else {
                    if (index == CurrentIndex) {
                        return id<CurrentIndex>();
                    } else {
                        return id<CurrentIndex + 1>(index);
                    }
                }
            }

            template <auto Id, std::size_t CurrentIndex = 0>
            constexpr static std::size_t index()
            {
                static_assert(CurrentIndex < sizeof...(Types));

                if constexpr (variant_impl::id<CurrentIndex>() == Id) {
                    return CurrentIndex;
                } else {
                    return index<Id, CurrentIndex + 1>();
                }
            }

            template <std::size_t CurrentIndex = 0>
            LU_REFLECTION_INLINE constexpr static std::size_t index(auto && id)
            {
                if constexpr (CurrentIndex == sizeof...(Types)) {
                    return std::numeric_limits<std::size_t>::max();
                } else {
                    if (variant_impl::id<CurrentIndex>() == id) {
                        return CurrentIndex;
                    } else {
                        return index<CurrentIndex + 1>(id);
                    }
                }
                return std::numeric_limits<std::size_t>::max();
            }

            template <std::size_t... LeftIndices, std::size_t... RightIndices>
            constexpr static auto unique_ids(std::index_sequence<LeftIndices...>,
                                            std::index_sequence<RightIndices...>)
            {
                auto unique_among_rest = []<auto LeftIndex, auto LeftId>()
                {
                    return (... && ((LeftIndex == RightIndices) ||
                                    (LeftId != id<RightIndices>())));
                };
                return (... && unique_among_rest.template
                            operator()<LeftIndices, id<LeftIndices>()>());
            }

            template <std::size_t... LeftIndices, std::size_t... RightIndices>
            constexpr static auto
            same_id_types(std::index_sequence<LeftIndices...>,
                        std::index_sequence<RightIndices...>)
            {
                auto same_among_rest = []<auto LeftIndex, auto LeftId>()
                {
                    return (... &&
                            (std::same_as<
                                std::remove_cv_t<decltype(LeftId)>,
                                std::remove_cv_t<decltype(id<RightIndices>())>>));
                };
                return (... && same_among_rest.template
                            operator()<LeftIndices, id<LeftIndices>()>());
            }

            template <typename Type, std::size_t... Indices>
            constexpr static std::size_t index_by_type(std::index_sequence<Indices...>)
            {
                return ((std::same_as<
                            Type,
                            std::variant_alternative_t<Indices, variant_type>> *
                        Indices) +
                        ...);
            }

            template <typename Type>
            constexpr static std::size_t index_by_type()
            {
                return index_by_type<Type>(
                    std::make_index_sequence<std::variant_size_v<variant_type>>{});
            }

            using id_type = decltype(id<0>());
        };

        template <typename Variant>
        struct variant_checker;

        template <typename... Types, template <typename...> typename Variant>
        struct variant_checker<Variant<Types...>>
        {
            using type = variant_impl<Variant<Types...>>;
            static_assert(
                type::unique_ids(std::make_index_sequence<sizeof...(Types)>(),
                        std::make_index_sequence<sizeof...(Types)>()));
            static_assert(
                type::same_id_types(std::make_index_sequence<sizeof...(Types)>(),
                                    std::make_index_sequence<sizeof...(Types)>()));
        };

        template <typename Variant>
        using variant = typename variant_checker<Variant>::type;

        template <typename Tuple>
        struct tuple;

        template <typename... Types, template <typename...> typename Tuple>
        struct tuple<Tuple<Types...>>
        {
            template <std::size_t Index = 0>
            LU_REFLECTION_INLINE constexpr static auto visit(auto && tuple, auto && index, auto && visitor)
            {
                if constexpr (Index + 1 == sizeof...(Types)) {
                    return visitor(std::get<Index>(tuple));
                } else {
                    if (Index == index) {
                        return visitor(std::get<Index>(tuple));
                    }
                    return visit<Index + 1>(tuple, index, visitor);
                }
            }
        };

        template <typename Type, typename Visitor = std::monostate>
        struct visitor
        {
            using byte_type = std::byte;
            using view_type = std::span<std::byte>;

            static constexpr bool resizable = false;

            constexpr auto operator()(auto && ... arguments) const
            {
                if constexpr (requires {
                                visitor(std::forward<decltype(arguments)>(
                                    arguments)...);
                            }) {
                    return visitor(
                        std::forward<decltype(arguments)>(arguments)...);
                } else {
                    return sizeof...(arguments);
                }
            }

            template <typename...>
            constexpr auto serialize_one(auto && ... arguments) const
            {
                return (*this)(std::forward<decltype(arguments)>(arguments)...);
            }

            template <typename...>
            constexpr auto serialize_many(auto && ... arguments) const
            {
                return (*this)(std::forward<decltype(arguments)>(arguments)...);
            }

            constexpr static auto kind()
            {
                return kind::out;
            }

            std::span<std::byte> data();
            std::span<std::byte> remaining_data();
            std::span<std::byte> processed_data();
            std::size_t position() const;
            std::size_t & position();
            errc enlarge_for(std::size_t);
            void reset(std::size_t = 0);

            [[no_unique_address]] Visitor visitor;
        };

        constexpr auto get_default_size_type()
        {
            return default_size_type{};
        }

        constexpr auto get_default_size_type(auto option, auto... options)
        {
            if constexpr (requires {
                            typename decltype(option)::default_size_type;
                        }) {
                if constexpr (std::is_void_v<typename decltype(option)::default_size_type>) {
                    return std::monostate{};
                } else {
                    return typename decltype(option)::default_size_type{};
                }
            } else {
                return get_default_size_type(options...);
            }
        }

        template <typename... Options>
        using default_size_type_t =
            std::conditional_t<std::same_as<std::monostate,
                                            decltype(get_default_size_type(
                                                std::declval<Options>()...))>,
                            void,
                            decltype(get_default_size_type(
                                std::declval<Options>()...))>;

        template <typename Option, typename... Options>
        constexpr auto get_alloc_limit()
        {
            if constexpr (requires {
                            std::remove_cvref_t<
                                Option>::alloc_limit_value;
                        }) {
                return std::remove_cvref_t<Option>::alloc_limit_value;
            } else if constexpr (sizeof...(Options) != 0) {
                return get_alloc_limit<Options...>();
            } else {
                return std::numeric_limits<std::size_t>::max();
            }
        }

        template <typename... Options>
        constexpr auto alloc_limit()
        {
            if constexpr (sizeof...(Options) != 0) {
                return get_alloc_limit<Options...>();
            } else {
                return std::numeric_limits<std::size_t>::max();
            }
        }

        template <typename Option, typename... Options>
        constexpr auto get_enlarger()
        {
            if constexpr (requires {
                            std::remove_cvref_t<
                                Option>::enlarger_value;
                        }) {
                return std::remove_cvref_t<Option>::enlarger_value;
            } else if constexpr (sizeof...(Options) != 0) {
                return get_enlarger<Options...>();
            } else {
                return std::tuple{3, 2};
            }
        }

        template <typename... Options>
        constexpr auto enlarger()
        {
            if constexpr (sizeof...(Options) != 0) {
                return get_enlarger<Options...>();
            } else {
                return std::tuple{3, 2};
            }
        }

        template <typename Type>
        constexpr auto underlying_type_generic()
        {
            if constexpr (std::is_enum_v<Type>) {
                return std::underlying_type_t<Type>{};
            } else {
                return Type{};
            }
        }

        template <typename Type>
        using underlying_type_t = decltype(underlying_type_generic<Type>());

        template <typename Id>
        struct id_serializable
        {
            using serialize_id = Id;
        };

        constexpr auto unique(auto && ... values)
        {
            auto unique_among_rest = [](auto && value, auto && ... values)
            {
                return (... && ((&value == &values) ||
                                (value != values)));
            };
            return (... && unique_among_rest(values, values...));
        }
    } // namespace traits

    namespace concepts
    {
        template <typename Type>
        concept byte_type = std::same_as<std::remove_cv_t<Type>, char> ||
                            std::same_as<std::remove_cv_t<Type>, unsigned char> ||
                            std::same_as<std::remove_cv_t<Type>, std::byte>;

        template <typename Type>
        concept byte_view = byte_type<typename std::remove_cvref_t<Type>::value_type> &&
            requires(Type value)
        {
            value.data();
            value.size();
        };

        template <typename Type>
        concept has_serialize =
            access::has_serialize<Type,
                                traits::visitor<std::remove_cvref_t<Type>>>();

        template <typename Type>
        concept has_explicit_serialize = access::has_explicit_serialize<
            Type,
            traits::visitor<std::remove_cvref_t<Type>>>();

        template <typename Type>
        concept variant = !has_serialize<Type> && requires (Type variant) {
            variant.index();
            std::get_if<0>(&variant);
            std::variant_size_v<std::remove_cvref_t<Type>>;
        };

        template <typename Type>
        concept optional = !has_serialize<Type> && requires (Type optional) {
            optional.value();
            optional.has_value();
            optional.operator bool();
            optional.operator*();
        };

        template <typename Type>
        concept container =
            !has_serialize<Type> && !optional<Type> && requires(Type container)
        {
            typename std::remove_cvref_t<Type>::value_type;
            container.size();
            container.begin();
            container.end();
        };

        template <typename Type>
        concept associative_container = container<Type> && requires(Type container)
        {
            typename std::remove_cvref_t<Type>::key_type;
        };

        template <typename Type>
        concept tuple = !has_serialize<Type> && !container<Type> && requires(Type tuple)
        {
            sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
        }
        &&!requires(Type tuple)
        {
            tuple.index();
        };

        template <typename Type>
        concept owning_pointer = !optional<Type> &&
            (traits::is_unique_ptr<std::remove_cvref_t<Type>>::value ||
            traits::is_shared_ptr<std::remove_cvref_t<Type>>::value);

        template <typename Type>
        concept bitset =
            !has_serialize<Type> && requires(std::remove_cvref_t<Type> bitset)
        {
            bitset.flip();
            bitset.set();
            bitset.test(0);
            bitset.to_ullong();
        };

        template <typename Type>
        concept has_protocol = access::has_protocol<Type>();

        template <typename Type>
        concept by_protocol = has_protocol<Type> && !has_explicit_serialize<Type>;

        template <typename Type>
        concept basic_array = std::is_array_v<std::remove_cvref_t<Type>>;

        template <typename Type>
        concept unspecialized =
            !container<Type> && !owning_pointer<Type> && !tuple<Type> &&
            !variant<Type> && !optional<Type> && !bitset<Type> &&
            !std::is_array_v<std::remove_cvref_t<Type>> && !by_protocol<Type>;

        template <typename Type>
        concept empty = requires
        {
            std::integral_constant<std::size_t, sizeof(Type)>::value;
            requires std::is_empty_v<std::remove_cvref_t<Type>>;
        };

        template <typename Type>
        concept byte_serializable = access::byte_serializable<Type>();

        template <typename Type>
        concept endian_independent_byte_serializable =
            access::endian_independent_byte_serializable<Type>();

        template <typename Archive>
        concept endian_aware_archive = requires
        {
            requires std::remove_cvref_t<Archive>::endian_aware;
        };

        template <typename Archive, typename Type>
        concept serialize_as_bytes = endian_independent_byte_serializable<Type> ||
            (!endian_aware_archive<Archive> && byte_serializable<Type>);

        template <typename Type, typename Reference>
        concept type_references = requires
        {
            requires container<Type>;
            requires std::same_as<typename std::remove_cvref_t<Type>::value_type,
                                std::remove_cvref_t<Reference>>;
        }
        || requires
        {
            requires associative_container<Type>;
            requires std::same_as<typename std::remove_cvref_t<Type>::key_type,
                                std::remove_cvref_t<Reference>>;
        }
        || requires
        {
            requires associative_container<Type>;
            requires std::same_as<typename std::remove_cvref_t<Type>::mapped_type,
                                std::remove_cvref_t<Reference>>;
        }
        || requires (Type && value)
        {
            requires owning_pointer<Type>;
            requires std::same_as<std::remove_cvref_t<decltype(*value)>,
                                std::remove_cvref_t<Reference>>;
        }
        || requires (Type && value)
        {
            requires optional<Type>;
            requires std::same_as<std::remove_cvref_t<decltype(*value)>,
                                std::remove_cvref_t<Reference>>;
        };

        template <typename Type>
        concept self_referencing = access::self_referencing<Type>();

        template <typename Type>
        concept has_fixed_nonzero_size = requires
        {
            requires std::integral_constant<std::size_t,
                std::remove_cvref_t<Type>{}.size()>::value != 0;
        };

        template <typename Type>
        concept array =
            basic_array<Type> ||
            (container<Type> && has_fixed_nonzero_size<Type> && requires {
                requires Type {
                }
                .size() * sizeof(typename Type::value_type) == sizeof(Type);
                Type{}.data();
            });
    } // namespace concepts

    template <typename CharType, std::size_t Size>
    struct string_literal : public std::array<CharType, Size + 1>
    {
        using base = std::array<CharType, Size + 1>;
        using value_type = typename base::value_type;
        using pointer = typename base::pointer;
        using const_pointer = typename base::const_pointer;
        using iterator = typename base::iterator;
        using const_iterator = typename base::const_iterator;
        using reference = typename base::const_pointer;
        using const_reference = typename base::const_pointer;
        using size_type = default_size_type;

        constexpr string_literal() = default;
        constexpr string_literal(const CharType (&value)[Size + 1])
        {
            std::copy_n(std::begin(value), Size + 1, std::begin(*this));
        }

        constexpr auto operator<=>(const string_literal &) const = default;

        constexpr default_size_type size() const
        {
            return Size;
        }

        constexpr bool empty() const
        {
            return !Size;
        }

        using base::begin;

        constexpr auto end()
        {
            return base::end() - 1;
        }

        constexpr auto end() const
        {
            return base::end() - 1;
        }

        using base::data;
        using base::operator[];
        using base::at;

    private:
        using base::cbegin;
        using base::cend;
        using base::rbegin;
        using base::rend;
    };

    template <typename CharType, std::size_t Size>
    string_literal(const CharType (&value)[Size])
        -> string_literal<CharType, Size - 1>;

    template <typename Item>
    class bytes
    {
    public:
        using value_type = Item;

        constexpr explicit bytes(std::span<Item> items) :
            m_items(items.data()), m_size(items.size())
        {
        }

        constexpr explicit bytes(std::span<Item> items, auto size) :
            m_items(items.data()), m_size(std::size_t(size))
        {
        }

        constexpr auto data() const
        {
            return m_items;
        }

        constexpr std::size_t size_in_bytes() const
        {
            return m_size * sizeof(Item);
        }

        constexpr std::size_t count() const
        {
            return m_size;
        }

    private:
        static_assert(std::is_trivially_copyable_v<Item>);

        Item * m_items;
        std::size_t m_size;
    };

    template <typename Item>
    bytes(std::span<Item>) -> bytes<Item>;

    template <typename Item>
    bytes(std::span<Item>, std::size_t) -> bytes<Item>;

    template <typename Item, std::size_t Count>
    bytes(Item(&)[Count]) -> bytes<Item>;

    template <concepts::container Container>
    bytes(Container && container)
        -> bytes<std::remove_reference_t<decltype(container[0])>>;

    template <concepts::container Container>
    bytes(Container && container, std::size_t)
        -> bytes<std::remove_reference_t<decltype(container[0])>>;

    constexpr auto as_bytes(auto && object)
    {
        return bytes(std::span{&object, 1});
    }

    template <typename Option>
    struct option
    {
        using lu_bits_option = void;
        constexpr auto operator()(auto && archive)
        {
            if constexpr (requires {
                            archive.option(static_cast<Option &>(*this));
                        }) {
                archive.option(static_cast<Option &>(*this));
            }
        }
    };

    inline namespace options
    {
        struct append : option<append>
        {
        };

        struct reserve : option<reserve>
        {
            constexpr explicit reserve(std::size_t size) : size(size)
            {
            }
            std::size_t size{};
        };

        struct resize : option<resize>
        {
            constexpr explicit resize(std::size_t size) : size(size)
            {
            }
            std::size_t size{};
        };

        template <std::size_t Size>
        struct alloc_limit : option<alloc_limit<Size>>
        {
            constexpr static auto alloc_limit_value = Size;
        };

        template <std::size_t Multiplier, std::size_t Divisor = 1>
        struct enlarger : option<enlarger<Multiplier, Divisor>>
        {
            constexpr static auto enlarger_value =
                std::tuple{Multiplier, Divisor};
        };

        using exact_enlarger = enlarger<1, 1>;

        namespace endian
        {
            struct big : option<big>
            {
                constexpr static auto value = std::endian::big;
            };

            struct little : option<little>
            {
                constexpr static auto value = std::endian::little;
            };

            using network = big;

            using native = std::
                conditional_t<std::endian::native == std::endian::little, little, big>;

            using swapped = std::
                conditional_t<std::endian::native == std::endian::little, big, little>;
        } // namespace endian

        struct no_fit_size : option<no_fit_size>
        {
        };

        struct no_enlarge_overflow : option<no_enlarge_overflow>
        {
        };

        struct enlarge_overflow : option<enlarge_overflow>
        {
        };

        struct no_size : option<no_size>
        {
            using default_size_type = void;
        };

        struct size1b : option<size1b>
        {
            using default_size_type = unsigned char;
        };

        struct size2b : option<size2b>
        {
            using default_size_type = std::uint16_t;
        };

        struct size4b : option<size4b>
        {
            using default_size_type = std::uint32_t;
        };

        struct size8b : option<size8b>
        {
            using default_size_type = std::uint64_t;
        };

        struct size_native : option<size_native>
        {
            using default_size_type = std::size_t;
        };
    } // namespace options

    template <typename Type>
    constexpr auto access::number_of_members()
    {
        using type = std::remove_cvref_t<Type>;
        if constexpr (std::is_array_v<type>) {
            return std::extent_v<type>;
        } else if constexpr (!std::is_class_v<type>) {
            return 0;
        } else if constexpr (concepts::container<type> &&
                            concepts::has_fixed_nonzero_size<type>) {
            return type{}.size();
        } else if constexpr (concepts::tuple<type>) {
            return std::tuple_size_v<type>;
        } else if constexpr (requires {
                                requires std::same_as<
                                    typename type::serialize,
                                    members<type::serialize::value>>;
                                requires type::serialize::value !=
                                    std::numeric_limits<
                                        std::size_t>::max();
                            }) {
            return type::serialize::value;
        } else if constexpr (requires(Type && item) {
                                requires std::same_as<
                                    decltype(try_serialize(item)),
                                    members<decltype(try_serialize(
                                        item))::value>>;
                                requires decltype(try_serialize(
                                    item))::value !=
                                    std::numeric_limits<
                                        std::size_t>::max();
                            }) {
            return decltype(serialize(std::declval<type>()))::value;
        } else if constexpr (requires {
                                requires std::same_as<
                                    typename type::serialize,
                                    protocol<type::serialize::value,
                                            type::serialize::members>>;
                                requires type::serialize::members !=
                                    std::numeric_limits<
                                        std::size_t>::max();
                            }) {
            return type::serialize::members;
        } else if constexpr (requires(Type && item) {
                                requires std::same_as<
                                    decltype(try_serialize(item)),
                                    protocol<decltype(try_serialize(item))::value,
                                            decltype(try_serialize(
                                                item))::members>>;
                                requires decltype(try_serialize(
                                    item))::members !=
                                    std::numeric_limits<
                                        std::size_t>::max();
                            }) {
            return decltype(serialize(std::declval<type>()))::members;
    #if LU_REFLECTION_AUTODETECT_MEMBERS_MODE == 0
        } else if constexpr (std::is_aggregate_v<type>) {
            // clang-format off
            if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},  /*.................................................................................................................*/ any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 50; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 49; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 48; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 47; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 46; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 45; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 44; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 43; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 42; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 41; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 40; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 39; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 38; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 37; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 36; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 35; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 34; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 33; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 32; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 31; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 30; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 29; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 28; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 27; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 26; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 25; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 24; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 23; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 22; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 21; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 20; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 19; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 18; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 17; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 16; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 15; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 14; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 13; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 12; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 11; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 10; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 9; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 8; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 7; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 6; } else if constexpr (requires { type{any{}, any{}, any{}, any{}, any{}}; }) { return 5; } else if constexpr (requires { type{any{}, any{}, any{}, any{}}; }) { return 4; } else if constexpr (requires { type{any{}, any{}, any{}}; }) { return 3; } else if constexpr (requires { type{any{}, any{}}; }) { return 2; } else if constexpr (requires { type{any{}}; }) { return 1;
                // Returns the number of members
                // clang-format on
            } else if constexpr (concepts::empty<type> && requires {
                                    typename std::void_t<decltype(type{})>;
                                }) {
                return 0;
            } else {
                return -1;
            }
    #elif LU_REFLECTION_AUTODETECT_MEMBERS_MODE > 0
    #if LU_REFLECTION_AUTODETECT_MEMBERS_MODE == 1
            // clang-format off
        } else if constexpr (requires { [](Type && object) { auto && [a1] = object; }; }) { return 1; } else if constexpr (requires { [](Type && object) { auto && [a1, a2] = object; }; }) { return 2; /*.................................................................................................................*/ } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3] = object; }; }) { return 3; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4] = object; }; }) { return 4; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5] = object; }; }) { return 5; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6] = object; }; }) { return 6; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7] = object; }; }) { return 7; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8] = object; }; }) { return 8; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object; }; }) { return 9; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object; }; }) { return 10; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object; }; }) { return 11; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object; }; }) { return 12; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object; }; }) { return 13; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] = object; }; }) { return 14; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = object; }; }) { return 15; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16] = object; }; }) { return 16; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = object; }; }) { return 17; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18] = object; }; }) { return 18; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19] = object; }; }) { return 19; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20] = object; }; }) { return 20; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = object; }; }) { return 21; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22] = object; }; }) { return 22; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23] = object; }; }) { return 23; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24] = object; }; }) { return 24; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25] = object; }; }) { return 25; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26] = object; }; }) { return 26; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27] = object; }; }) { return 27; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28] = object; }; }) { return 28; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29] = object; }; }) { return 29; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30] = object; }; }) { return 30; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31] = object; }; }) { return 31; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32] = object; }; }) { return 32; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33] = object; }; }) { return 33; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34] = object; }; }) { return 34; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35] = object; }; }) { return 35; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36] = object; }; }) { return 36; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37] = object; }; }) { return 37; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38] = object; }; }) { return 38; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39] = object; }; }) { return 39; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40] = object; }; }) { return 40; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41] = object; }; }) { return 41; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42] = object; }; }) { return 42; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43] = object; }; }) { return 43; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44] = object; }; }) { return 44; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45] = object; }; }) { return 45; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46] = object; }; }) { return 46; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47] = object; }; }) { return 47; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48] = object; }; }) { return 48; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49] = object; }; }) { return 49; } else if constexpr (requires { [](Type && object) { auto && [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50] = object; }; }) { return 50;
            // Returns the number of members
            // clang-format on
    #else // LU_REFLECTION_AUTODETECT_MEMBERS_MODE == 1
    #error "Invalid value for LU_REFLECTION_AUTODETECT_MEMBERS_MODE"
    #endif
    #endif
        } else {
            return -1;
        }
    }

    template <typename Type>
    struct access::byte_serializable_visitor
    {
        template <typename... Types>
        constexpr auto operator()() {
            using type = std::remove_cvref_t<Type>;

            if constexpr (concepts::empty<type>) {
                return std::false_type{};
            } else if constexpr ((... || has_explicit_serialize<
                                            Types,
                                            traits::visitor<Types>>())) {
                return std::false_type{};
            } else if constexpr ((... || !byte_serializable<Types>())) {
                return std::false_type{};
            } else if constexpr ((0 + ... + sizeof(Types)) != sizeof(type)) {
                return std::false_type{};
            } else if constexpr ((... || concepts::empty<Types>)) {
                return std::false_type{};
            } else {
                return std::true_type{};
            }
        }
    };

    template <typename Type>
    constexpr auto access::byte_serializable()
    {
        constexpr auto members_count = number_of_members<Type>();
        using type = std::remove_cvref_t<Type>;

        if constexpr (members_count < 0) {
            return false;
        } else if constexpr (!std::is_trivially_copyable_v<type>) {
            return false;
        } else if constexpr (has_explicit_serialize<type,
                                                    traits::visitor<type>>()) {
            return false;
        } else if constexpr (
            !requires {
                requires std::integral_constant<
                    int,
                    (std::bit_cast<std::remove_all_extents_t<type>>(
                        std::array<
                            std::byte,
                            sizeof(std::remove_all_extents_t<type>)>()),
                    0)>::value == 0;
            }) {
            return false;
        } else if constexpr (concepts::array<type>) {
            return byte_serializable<
                std::remove_cvref_t<decltype(std::declval<type>()[0])>>();
        } else if constexpr (members_count > 0) {
            return visit_members_types<type>(
                byte_serializable_visitor<type>{})();
        } else {
            return true;
        }
    }

    template <typename Type>
    struct access::endian_independent_byte_serializable_visitor
    {
        template <typename... Types>
        constexpr auto operator()() {
            using type = std::remove_cvref_t<Type>;

            if constexpr (concepts::empty<type>) {
                return std::false_type{};
            } else if constexpr ((... || has_explicit_serialize<
                                            Types,
                                            traits::visitor<Types>>())) {
                return std::false_type{};
            } else if constexpr ((... || !endian_independent_byte_serializable<Types>())) {
                return std::false_type{};
            } else if constexpr ((0 + ... + sizeof(Types)) != sizeof(type)) {
                return std::false_type{};
            } else if constexpr ((... || concepts::empty<Types>)) {
                return std::false_type{};
            } else if constexpr (!concepts::byte_type<type>) {
                return std::false_type{};
            } else {
                return std::true_type{};
            }
        }
    };

    template <typename Type>
    constexpr auto access::endian_independent_byte_serializable()
    {
        constexpr auto members_count = number_of_members<Type>();
        using type = std::remove_cvref_t<Type>;

        if constexpr (members_count < 0) {
            return false;
        } else if constexpr (!std::is_trivially_copyable_v<type>) {
            return false;
        } else if constexpr (has_explicit_serialize<type,
                                                    traits::visitor<type>>()) {
            return false;
        } else if constexpr (
            !requires {
                requires std::integral_constant<
                    int,
                    (std::bit_cast<std::remove_all_extents_t<type>>(
                        std::array<
                            std::byte,
                            sizeof(std::remove_all_extents_t<type>)>()),
                    0)>::value == 0;
            }) {
            return false;
        } else if constexpr (concepts::array<type>) {
            return endian_independent_byte_serializable<
                std::remove_cvref_t<decltype(std::declval<type>()[0])>>();
        } else if constexpr (members_count > 0) {
            return visit_members_types<type>(
                endian_independent_byte_serializable_visitor<type>{})();
        } else {
            return concepts::byte_type<type>;
        }
    }

    template <typename Type, typename Self, typename... Visited>
    struct access::self_referencing_visitor
    {
        template <typename... Types>
        constexpr auto operator()() {
            using type = std::remove_cvref_t<Type>;
            using self = std::remove_cvref_t<Self>;

            if constexpr (concepts::empty<type>) {
                return std::false_type{};
            } else if constexpr ((... || concepts::type_references<
                                            std::remove_cvref_t<Types>,
                                            self>)) {
                return std::true_type{};
            } else if constexpr ((sizeof...(Visited) != 0) &&
                                (... || std::same_as<type, Visited>)) {
                return std::false_type{};
            } else if constexpr ((... ||
                                self_referencing<std::remove_cvref_t<Types>,
                                                self,
                                                type,
                                                Visited...>())) {
                return std::true_type{};
            } else {
                return std::false_type{};
            }
        }
    };

    template <typename Type, typename Self/* = Type*/, typename... Visited>
    constexpr auto access::self_referencing()
        {
        constexpr auto members_count = number_of_members<Type>();
        using type = std::remove_cvref_t<Type>;
        using self = std::remove_cvref_t<Self>;

        if constexpr (members_count < 0) {
            return false;
        } else if constexpr (has_explicit_serialize<type,
                                                    traits::visitor<type>>()) {
            return false;
        } else if constexpr (members_count == 0) {
            return false;
        } else if constexpr (concepts::array<type>) {
            return self_referencing<
                std::remove_cvref_t<decltype(std::declval<type>()[0])>,
                self,
                Visited...>();
        } else {
            return visit_members_types<type>(
                self_referencing_visitor<type, self, Visited...>{})();
        }
    }

    template <typename Type>
    constexpr auto number_of_members()
    {
        return access::number_of_members<Type>();
    }

    LU_REFLECTION_INLINE constexpr decltype(auto) visit_members(auto && object,
                                                        auto && visitor)
    {
        return access::visit_members(object, visitor);
    }

    template <typename Type>
    constexpr decltype(auto) visit_members_types(auto && visitor)
    {
        return access::visit_members_types<Type>(visitor);
    }

    template <typename Type>
    struct optional_ptr : std::unique_ptr<Type>
    {
        using base = std::unique_ptr<Type>;
        using base::base;
        using base::operator=;

        constexpr optional_ptr(base && other) noexcept :
            base(std::move(other))
        {
        }
    };

    template <typename Type, typename...>
    optional_ptr(Type *) -> optional_ptr<Type>;

    template <typename Archive, typename Type>
    LU_REFLECTION_INLINE constexpr static auto serialize(
        Archive & archive,
        const optional_ptr<Type> & self) requires(Archive::kind() == kind::out)
    {
        if (!self) [[unlikely]] {
            return archive(std::byte(false));
        } else {
            return archive(std::byte(true), *self);
        }
    }

    template <typename Archive, typename Type>
    LU_REFLECTION_INLINE constexpr static auto
    serialize(Archive & archive,
            optional_ptr<Type> & self) requires(Archive::kind() == kind::in)
    {
        std::byte has_value{};
        if (auto result = archive(has_value); failure(result))
            [[unlikely]] {
            return result;
        }

        if (!bool(has_value)) [[unlikely]] {
            self = {};
            return errc{};
        }

        if (auto result =
                archive(static_cast<std::unique_ptr<Type> &>(self));
            failure(result)) [[unlikely]] {
            return result;
        }

        return errc{};
    }

    template <typename Type, typename SizeType>
    struct sized_item : public Type
    {
        using Type::Type;
        using Type::operator=;

        constexpr sized_item(Type && other) noexcept(
            std::is_nothrow_move_constructible_v<Type>) :
            Type(std::move(other))
        {
        }

        constexpr sized_item(const Type & other) :
            Type(other)
        {
        }

        LU_REFLECTION_INLINE constexpr static auto serialize(auto & archive,
                                                        auto & self)
        {
            if constexpr (std::remove_cvref_t<decltype(archive)>::kind() == kind::out) {
                return archive.template serialize_one<SizeType>(
                    static_cast<const Type &>(self));
            } else {
                return archive.template serialize_one<SizeType>(
                    static_cast<Type &>(self));
            }
        }
    };

    template <typename Type, typename SizeType>
    auto serialize(const sized_item<Type, SizeType> &)
        -> members<number_of_members<Type>()>;

    template <typename Type, typename SizeType>
    using sized_t = sized_item<Type, SizeType>;

    template <typename Type>
    using unsized_t = sized_t<Type, void>;

    template <typename Type, typename SizeType>
    struct sized_item_ref
    {
        constexpr explicit sized_item_ref(Type && value) :
            value(std::forward<Type>(value))
        {
        }

        LU_REFLECTION_INLINE constexpr static auto serialize(auto & serializer,
                                                        auto & self)
        {
            return serializer.template serialize_one<SizeType>(self.value);
        }

        Type && value;
    };

    template <typename SizeType, typename Type>
    constexpr auto sized(Type && value)
    {
        return sized_item_ref<Type &, SizeType>(value);
    }

    template <typename Type>
    constexpr auto unsized(Type && value)
    {
        return sized_item_ref<Type &, void>(value);
    }

    enum class varint_encoding
    {
        normal,
        zig_zag,
    };

    template <typename Type, varint_encoding Encoding = varint_encoding::normal>
    struct varint
    {
        varint() = default;

        using value_type = Type;
        static constexpr auto encoding = Encoding;

        constexpr varint(Type value) : value(value)
        {
        }

        constexpr operator Type &() &
        {
            return value;
        }

        constexpr operator Type() const
        {
            return value;
        }

        constexpr decltype(auto) operator*() &
        {
            return (value);
        }

        constexpr auto operator*() const &
        {
            return value;
        }

        Type value{};
    };

    namespace concepts
    {
        template <typename Type>
        concept varint = requires
        {
            requires std::same_as<
                Type,
                lu::reflection::varint<typename Type::value_type, Type::encoding>>;
        };
    } // namespace concepts

    template <typename Type>
    constexpr auto varint_max_size = sizeof(Type) * CHAR_BIT / (CHAR_BIT - 1) +
                                    1;

    template <varint_encoding Encoding = varint_encoding::normal>
    LU_REFLECTION_INLINE constexpr auto varint_size(auto value)
    {
        if constexpr (Encoding == varint_encoding::zig_zag) {
            return varint_size(std::make_unsigned_t<decltype(value)>((value << 1) ^
                            (value >> (sizeof(value) * CHAR_BIT - 1))));
        } else {
            return ((sizeof(value) * CHAR_BIT) -
                    std::countl_zero(
                        std::make_unsigned_t<decltype(value)>(value | 0x1)) +
                    (CHAR_BIT - 2)) /
                (CHAR_BIT - 1);
        }
    }

    template <typename Archive, typename Type, varint_encoding Encoding>
    LU_REFLECTION_INLINE constexpr auto serialize(
        Archive & archive,
        varint<Type, Encoding> self) requires(Archive::kind() == kind::out)
    {
        auto orig_value = std::conditional_t<std::is_enum_v<Type>,
                                            traits::underlying_type_t<Type>,
                                            Type>(self.value);
        auto value = std::make_unsigned_t<Type>(orig_value);
        if constexpr (varint_encoding::zig_zag == Encoding) {
            value =
                (value << 1) ^ (orig_value >> (sizeof(Type) * CHAR_BIT - 1));
        }

        constexpr auto max_size = varint_max_size<Type>;
        if constexpr (Archive::resizable) {
            if (auto result = archive.enlarge_for(max_size); failure(result))
                [[unlikely]] {
                return result;
            }
        }

        auto data = archive.remaining_data();
        if constexpr (!Archive::resizable) {
            auto data_size = data.size();
            if (data_size < max_size) [[unlikely]] {
                if (data_size < varint_size(value)) [[unlikely]] {
                    return errc{std::errc::result_out_of_range};
                }
            }
        }

        using byte_type = std::remove_cvref_t<decltype(data[0])>;
        std::size_t position = {};
        while (value >= 0x80) {
            data[position++] = byte_type((value & 0x7f) | 0x80);
            value >>= (CHAR_BIT - 1);
        }
        data[position++] = byte_type(value);

        archive.position() += position;
        return errc{};
    }

    constexpr auto decode_varint(auto data, auto & value, auto & position)
    {
        using value_type = std::remove_cvref_t<decltype(value)>;
        if (data.size() < varint_max_size<value_type>) [[unlikely]] {
            std::size_t shift = 0;
            for (auto & byte_value : data) {
                auto next_byte = value_type(byte_value);
                value |= (next_byte & 0x7f) << shift;
                if (next_byte >= 0x80) [[unlikely]] {
                    shift += CHAR_BIT - 1;
                    continue;
                }
                position += 1 + std::distance(data.data(), &byte_value);
                return errc{};
            }
            return errc{std::errc::result_out_of_range};
        } else {
            auto p = data.data();
            do {
                // clang-format off
                value_type next_byte;
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 0)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 1)); if (next_byte < 0x80) [[likely]] { break; }
                if constexpr (varint_max_size<value_type> > 2) {
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 2)); if (next_byte < 0x80) [[likely]] { break; }
                if constexpr (varint_max_size<value_type> > 3) {
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 3)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 4)); if (next_byte < 0x80) [[likely]] { break; }
                if constexpr (varint_max_size<value_type> > 5) {
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 5)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 6)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 7)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 8)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x01) << ((CHAR_BIT - 1) * 9)); if (next_byte < 0x80) [[likely]] { break; } }}}
                return errc{std::errc::value_too_large};
                // clang-format on
            } while (false);
            position += std::distance(data.data(), p);
            return errc{};
        }
    }

    template <typename Archive, typename Type, varint_encoding Encoding>
    LU_REFLECTION_INLINE constexpr auto serialize(
        Archive & archive,
        varint<Type, Encoding> & self) requires(Archive::kind() == kind::in)
    {
        using value_type = std::conditional_t<
            std::is_enum_v<Type>,
            std::make_unsigned_t<traits::underlying_type_t<Type>>,
            std::make_unsigned_t<Type>>;
        value_type value{};
        auto data = archive.remaining_data();

        if constexpr (!LU_REFLECTION_INLINE_DECODE_VARINT) {
            auto & position = archive.position();
            if (!data.empty() && !(value_type(data[0]) & 0x80)) [[likely]] {
                value = value_type(data[0]);
                position += 1;
            } else if (auto result =
                        std::is_constant_evaluated()
                            ? decode_varint(data, value, position)
                            : decode_varint(
                                    std::span{
                                        reinterpret_cast<const std::byte *>(
                                            data.data()),
                                        data.size()},
                                    value,
                                    position);
                    failure(result)) [[unlikely]] {
                return result;
            }

            if constexpr (varint_encoding::zig_zag == Encoding) {
                self.value =
                    decltype(self.value)((value >> 1) ^ -(value & 0x1));
            } else {
                self.value = decltype(self.value)(value);
            }
            return errc{};
        } else if (data.size() < varint_max_size<value_type>) [[unlikely]] {
            std::size_t shift = 0;
            for (auto & byte_value : data) {
                auto next_byte = decltype(value)(byte_value);
                value |= (next_byte & 0x7f) << shift;
                if (next_byte >= 0x80) [[unlikely]] {
                    shift += CHAR_BIT - 1;
                    continue;
                }
                if constexpr (varint_encoding::zig_zag == Encoding) {
                    self.value =
                        decltype(self.value)((value >> 1) ^ -(value & 0x1));
                } else {
                    self.value = decltype(self.value)(value);
                }
                archive.position() +=
                    1 + std::distance(data.data(), &byte_value);
                return errc{};
            }
            return errc{std::errc::result_out_of_range};
        } else {
            auto p = data.data();
            do {
                // clang-format off
                value_type next_byte;
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 0)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 1)); if (next_byte < 0x80) [[likely]] { break; }
                if constexpr (varint_max_size<value_type> > 2) {
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 2)); if (next_byte < 0x80) [[likely]] { break; }
                if constexpr (varint_max_size<value_type> > 3) {
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 3)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 4)); if (next_byte < 0x80) [[likely]] { break; }
                if constexpr (varint_max_size<value_type> > 5) {
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 5)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 6)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 7)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x7f) << ((CHAR_BIT - 1) * 8)); if (next_byte < 0x80) [[likely]] { break; }
                next_byte = value_type(*p++); value |= ((next_byte & 0x01) << ((CHAR_BIT - 1) * 9)); if (next_byte < 0x80) [[likely]] { break; } }}}
                return errc{std::errc::value_too_large};
                // clang-format on
            } while (false);
            if constexpr (varint_encoding::zig_zag == Encoding) {
                self.value =
                    decltype(self.value)((value >> 1) ^ -(value & 0x1));
            } else {
                self.value = decltype(self.value)(value);
            }
            archive.position() += std::distance(data.data(), p);
            return errc{};
        }
    }

    template <typename Archive, typename Type, varint_encoding Encoding>
    constexpr auto
    serialize(Archive & archive,
            varint<Type, Encoding> && self) requires(Archive::kind() ==
                                                    kind::in) = delete;

    using vint32_t = varint<std::int32_t>;
    using vint64_t = varint<std::int64_t>;

    using vuint32_t = varint<std::uint32_t>;
    using vuint64_t = varint<std::uint64_t>;

    using vsint32_t = varint<std::int32_t, varint_encoding::zig_zag>;
    using vsint64_t = varint<std::int64_t, varint_encoding::zig_zag>;

    using vsize_t = varint<std::size_t>;

    inline namespace options
    {
        struct size_varint : option<size_varint>
        {
            using default_size_type = vsize_t;
        };
    } // namespace options

}