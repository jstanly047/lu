#include <reflex/Reflex.h>
#include <string>
#include <array>
#include <string_view>

namespace lu::reflex
{
    #define MEMBER_PTR(STRUCT_NAME, member)  &STRUCT_NAME::member
    #define MAKE_ARG_LIST_HELPER(STRUCT_NAME, member, ...) \
            MEMBER_PTR(STRUCT_NAME, member) \
            __VA_OPT__(, MAKE_ARG_LIST_AGAIN PARENS (STRUCT_NAME, __VA_ARGS__))
    #define MAKE_ARG_LIST_AGAIN() MAKE_ARG_LIST_HELPER
    #define MAKE_ARG_LIST(STRUCT_NAME, ...) \
        __VA_OPT__(EXPAND(MAKE_ARG_LIST_HELPER(STRUCT_NAME, __VA_ARGS__)))

    #define MAKE_META_DATA_IMPL(STRUCT_NAME, N, ...)                                \
        struct reflect_members                                                      \
        {                                                                           \
            constexpr decltype(auto) static apply_impl()                            \
            {                                                                       \
                return std::make_tuple(__VA_ARGS__);                                \
            }                                                                       \
                                                                                    \
            using size_type = std::integral_constant<size_t, N>;                    \
            constexpr static size_t value() { return size_type::value; }            \
            constexpr static std::string_view name()                                \
            {                                                                       \
                return std::string_view(#STRUCT_NAME, sizeof(#STRUCT_NAME));        \
            }                                                                       \
                                                                                    \
            constexpr static std::array<std::string_view, size_type::value> arr()   \
            {                                                                       \
                return arr_##STRUCT_NAME;                                           \
            }                                                                       \
        };

    #define MAKE_META_DATA(STRUCT_NAME, N, ...) \
        constexpr std::array<const char*, N> arr_##STRUCT_NAME = { CREAT_STRING_LIST(...)} \
        MAKE_META_DATA_IMPL(STRUCT_NAME, N, MAKE_ARG_LIST(STRUCT_NAME, __VA_ARGS__))

    #define REFLECTION(STRUT_NAME, ...) \
        MAKE_META_DATA(STRUT_NAME, NUMBER_OF_MEMBERS<STRUCT_NAME>, __VA_ARGS__)

    template <typename... Args, typename F, std::size_t... Idx>
    constexpr void for_each(std::tuple<Args...> &t, F &&f, std::index_sequence<Idx...>) 
    {
        (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}), ...);
    }

    template <typename... Args, typename F, std::size_t... Idx>
    constexpr void for_each(const std::tuple<Args...> &t, F &&f, std::index_sequence<Idx...>)
    {
        (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}), ...);
    }

    void printObj(auto object)
    {
        for_each(object, [](const auto& item, auto idx))
        {
            LOG(DEBUG) << item << " " << index << " " << decltype(i)::value ;
        }
    }
}
