#include <string>
#include <vector>
#include <limits>
#include <type_traits>

namespace  lu::reflex
{
    #define PARENS () 

    #define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
    #define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
    #define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
    #define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
    #define EXPAND1(...) __VA_ARGS__

    #define CONCAT_STRING(str)  #str
    #define CREAT_STRING_LIST_HELPER(str, ...) \
            CONCAT_STRING(str) \
            __VA_OPT__(, CREAT_STRING_LIST_AGAIN PARENS (__VA_ARGS__))
    #define CREAT_STRING_LIST_AGAIN() CREAT_STRING_LIST_HELPER
    #define CREAT_STRING_LIST(...) \
        __VA_OPT__(EXPAND(CREAT_STRING_LIST_HELPER(__VA_ARGS__)))

    /*
    Should be able to remove this once this in standardize in c++27 or higher
    */
    template<typename Type>
    constexpr std::size_t NUMBER_OF_MEMBERS()
    {
        if      constexpr (requires { requires std::is_empty_v<Type> && sizeof(Type); }) { return 0; }
        /*else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 30; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 29; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 28; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 27; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 26; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 25; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 24; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 23; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 22; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 21; }*/
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}, any{}}; }) { return 20; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}, any{}}; }) { return 19; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}, any{}}; }) { return 18; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}, any{}}; }) { return 17; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{},any{}}; }) { return 16; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 15; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 14; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 13; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 12; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 11; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 10; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 9; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 8; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 7; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}, any{}}; }) { return 6; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}, any{}}; }) { return 5; }
        else if constexpr (requires { Type{any{}, any{}, any{}, any{}}; }) { return 4; }
        else if constexpr (requires { Type{any{}, any{}, any{}}; }) { return 3; }
        else if constexpr (requires { Type{any{}, any{}}; }) { return 2; }
        else if constexpr (requires { Type{any{}}; }) { return 1; }
        else if constexpr (requires { Type{}; }) { return 0; }
        else { static_assert(std::is_void_v<Type>, "NUMBER_OF_MEMBERS maximum member is reached...fix me :(");}
    }

    decltype(auto) VisitMembers(auto&& object, auto&& visitor)
    {
        constexpr auto numberOfMembers = NUMBER_OF_MEMBERS<decltype(object)>();

        if      constexpr (numberOfMembers == 0) { return visitor(); }
        else if constexpr (numberOfMembers == 1) { auto& [m1] = object; return visitor(m1); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2] = object; return visitor(m1, m2); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3] = object; return visitor(m1, m2, m3); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4] = object; return visitor(m1, m2, m3, m4); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5] = object; return visitor(m1, m2, m3, m4, m5); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6] = object; return visitor(m1, m2, m3, m4, m5, m6); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7] = object; return visitor(m1, m2, m3, m4, m5, m6, m7); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20); }
        /*else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29); }
        else if constexpr (numberOfMembers == 2) { auto& [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30] = object; return visitor(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30); }
        */
         else { static_assert(std::is_void_v<Type>, "NUMBER_OF_MEMBERS maximum member is reached...fix me :(");}
    } 

    #define MACRO_CONCAT(A, B) A##_##B

    #define MAKE_ARRAY_IMPL(STRUCT_NAME, N, ...) \
        constexpr std::array<const char*, N> arr_##STRUCT_NAME = { CREAT_STRING_LIST(...)}

    #define MAKE_ARRAY(STRUCT_NAME, ...)\
        MAKE_ARRAY_IMPL(STRUCT_NAME, NUMBER_OF_MEMBERS<STRUCT_NAME>, __VA_ARGS__)


    
    
}
