#include <string>
#include <type_traits>
#include <experimental/socket>


struct dummy
{
    
    int d;
    float number;
    std::string str;
};

struct UniversalType 
{
    template<typename T> operator T();
};

template <typename T, typename... Args> 
consteval auto member_count()
{
    static_assert(std::is_aggregate_v<std::remove_cvref_t<T>>);
    
    if constexpr (requires { T{{Args{}}..., {UniversalType{}}};} == false)
    {
        return sizeof...(Args);
    }
    else
    {
        return member_count<T, Args..., UniversalType>();
    }
}

decltype(auto) visit_members(auto&& object, auto&&visitor)
{
    using type = std::remove_cvref_t<decltype(object)>;

    constexpr auto Count = member_count<type>();

    if constexpr (Count == 0)
    {
        return visitor();
    }
    else if constexpr (Count ==1 )
    {
        return visitor(a1);
    }
    else if constexpr (Count ==2 )
    {
        auto &&[a1, a2] = object;
        return visitor(a1, a2);
    }
    else if constexpr (Count == 3)
    {
        auto &&[a1, a2, a3] = object;
        return visitor(a1, a2, a3);
    }
}

void serialize_one(auto&& item)
{
    using type = std::remove_cvref_t<decltype<item>;

    if constexpr (std::is_trivially_copyable_v<type>)
    {
        std::memcpy(data_ + pos_, &item, sizeof(type));
        pos_ += sizeof(type);
    }
}

void serialize_one(auto&& item)
{
    using type = std::remove_cvref_t<decltype<item>;

    if constexpr (std::is_trivially_copyable_v<type>)
    {
        std::memcpy(data_ + pos_, &item, sizeof(type));
        pos_ += sizeof(type);
    }
}

void serialize(dummy t)
{
    visit_members(t, [](auto&& ...items){
        serialize_many(items...);
    });
}
