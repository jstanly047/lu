#include <reflex/Reflex.h>
#include <string>
#include <array>
#include <string_view>
#include <tuple>

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

    template <typename... Args, typename F, std::size_t... Idx>
    constexpr void for_each(std::tuple<Args...> &t, F &&f, std::index_sequence<Idx...>)
    {
        (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}), ..);
    }

    template <typename... Args, typename F, std::size_t... Idx>
    constexpr void for_each(const std::tuple<Args...> &t, F &&f, std::index_sequence<Idx...>)
    {
        (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}), ...);
    }

    template <typename T>
    using Reflect_members = decltype(reflect_members(std::declval<T>()));

    template <class T>
    struct is_signed_intergral_like
        : std::integral_constant<bool, (std::is_integral<T>::value) && std::is_signed<T>::value>
    {
    };

    template <class T>
    struct is_unsigned_intergral_like : std::integral_constant<bool, (std::is_integral<T>::value) && std::is_unsigned<T>::value>
    {
    };

    template <template <typename...> class U, typename T>
    struct is_template_instant_of : std::false_type
    {
    };

    template <template <typename...> class U, typename... args>
    struct is_template_instant_of<U, U<args...>> : std::true_type
    {
    };

    template <typename T>
    struct is_stdstring : is_template_instant_of<std::basic_string, T>
    {
    };

    template <typename T>
    struct is_tuple : is_template_instant_of<std::tuple, T>
    {
    };

    template <class T>
    struct is_sequence_container
        : std::integral_constant<
              bool, is_template_instant_of<std::deque, T>::value ||
                        is_template_instant_of<std::list, T>::value ||
                        is_template_instant_of<std::vector, T>::value ||
                        is_template_instant_of<std::queue, T>::value>
    {
    };

    template <class T>
    struct is_associat_container
        : std::integral_constant<
              bool, is_template_instant_of<std::map, T>::value ||
                        is_template_instant_of<std::unordered_map, T>::value>
    {
    };

    template <class T>
    struct is_emplace_back_able
        : std::integral_constant<
              bool, is_template_instant_of<std::deque, T>::value ||
                        is_template_instant_of<std::list, T>::value ||
                        is_template_instant_of<std::vector, T>::value>
    {
    };

    template <typename T, typename Tuple>
    struct has_type;

    template <typename T, typename... Us>
    struct has_type<T, std::tuple<Us...>>
        : std::disjunction<std::is_same<T, Us>...>
    {
    };

    template <typename T>
    inline constexpr bool is_int64_v =
        std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>;

    template <typename T, typename = void>
    struct is_public_reflection : std::false_type
    {
    };

    template <typename T>
    struct is_public_reflection<
        T, std::void_t<decltype(reflect_members(std::declval<T>()))>>
        : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_public_reflection_v = is_public_reflection<T>::value;

    template <typename T, typename = void>
    struct is_private_reflection : std::false_type
    {
    };

    template <typename T>
    struct is_private_reflection<T, std::void_t<decltype(std::declval<T>().reflect_members( std::declval<T>()))>> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_private_reflection_v = is_private_reflection<T>::value;

    template <typename T, typename = void>
    struct is_reflection : std::false_type
    {
    };

    template <typename T>
    struct is_reflection<T, std::enable_if_t<is_private_reflection_v<T>>>
        : std::true_type
    {
    };

    template <typename T>
    struct is_reflection<T, std::enable_if_t<is_public_reflection_v<T>>>
        : std::true_type
    {
    };

    template <typename... Args, typename F, std::size_t... Idx>
    constexpr void for_each(std::tuple<Args...> &t, F &&f,
                            std::index_sequence<Idx...>)
    {
        (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}),
         ...);
    }

    template <typename... Args, typename F, std::size_t... Idx>
    constexpr void for_each(const std::tuple<Args...> &t, F &&f,
                            std::index_sequence<Idx...>)
    {
        (std::forward<F>(f)(std::get<Idx>(t), std::integral_constant<size_t, Idx>{}),
         ...);
    }

    template <typename T, typename F>
    constexpr std::enable_if_t<is_reflection<T>::value> for_each(T &&t, F &&f)
    {
        using M = decltype(iguana_reflect_type(std::forward<T>(t)));
        for_each(M::apply_impl(), std::forward<F>(f),
                 std::make_index_sequence<M::value()>{});
    }

    template <typename T, typename F>
    constexpr std::enable_if_t<is_tuple<std::decay_t<T>>::value> for_each(T &&t,
                                                                          F &&f)
    {
        constexpr const size_t SIZE = std::tuple_size_v<std::decay_t<T>>;
        for_each(std::forward<T>(t), std::forward<F>(f),
                 std::make_index_sequence<SIZE>{});
    }
}
