#include <reflex/Reflex.h>

template<typename T, unsigned int i=std::numeric_limits<unsigned int>::max()>
struct FieldData{};

// FOR_EACH
#define REFLEX_REGISTER_MEMBER(CLASS_OR_STRUCT, MEMBER, INDEX) \
    template<> \
    struct FieldData<CLASS_OR_STRUCT, INDEX> \
    { \
        static constexpr const char* getName() { return #MEMBER; } \
        static constexpr unsigned int getSize() { return sizeof(CLASS_OR_STRUCT::MEMBER); } \
    };

#define FOR_EACH_HELPER(CLASS_OR_STRUCT, COUNTER, MEMBER, ...) \
    REFLEX_REGISTER_MEMBER(CLASS_OR_STRUCT, MEMBER, COUNTER) \
    __VA_OPT__(FOR_EACH_AGAIN PARENS (CLASS_OR_STRUCT, COUNTER + 1, __VA_ARGS__))

#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define FOR_EACH(CLASS_OR_STRUCT, COUNTER, ...) \
    __VA_OPT__(EXPAND(FOR_EACH_HELPER(CLASS_OR_STRUCT, COUNTER, __VA_ARGS__)))


#define REFLEX_REGISTER(CLASS_OR_STRUCT, ...) \
    template<> \
    struct FieldData<CLASS_OR_STRUCT, std::numeric_limits<unsigned int>::max()> \
    { \
        static constexpr const char* getName() { return #CLASS_OR_STRUCT; } \
        static constexpr unsigned int getSize() { return sizeof(CLASS_OR_STRUCT); } \
    }; \
    FOR_EACH (CLASS_OR_STRUCT, 0, __VA_ARGS__)

#define GET_BINDING_LIST_HELPER(MEMBER, ...) \
    ",:" #MEMBER \
    __VA_OPT__(GET_BINDING_LIST_AGAIN PARENS (CLASS_OR_STRUCT, COUNTER + 1, __VA_ARGS__))

#define GET_BINDING_LIST_AGAIN() GET_BINDING_LIST_HELPER

#define GET_BINDING_LIST(MEMBER, ...) \
    ":" #MEMBER \
    __VA_OPT__(EXPAND(GET_BINDING_LIST_HELPER(__VA_ARGS__)))


#define GET_BINDING_VALUE_LIST_HELPER(MEMBER, ...) \
    , soci::use(MEMBER) \
    __VA_OPT__(GET_BINDING_VALUE_LIST_AGAIN PARENS (CLASS_OR_STRUCT, COUNTER + 1, __VA_ARGS__))

#define GET_BINDING_VALUE_LIST_AGAIN() GET_BINDING_VALUE_LIST_HELPER

#define GET_BINDING_VALUE_LIST(...) \
    __VA_OPT__(EXPAND(GET_BINDING_VALUE_LIST_HELPER(__VA_ARGS__)))



#define GENERATE_SOCI_WRITE(CLASS_OR_STRUCT, ...) \
    void write(soci::session& session) \
    { \
        session << "insert into " #CLASS_OR_STRUCT " (" #__VA_ARGS__ ") values( "\
        GET_BINDING_LIST(__VA_ARGS__) ")" \
        GET_BINDING_VALUE_LIST(__VA_ARGS__); \
    }


class SQL
{
    auto getSqlOne(auto&& object)
    {
        if constexpr FieldData<decltype(object)>
    }

    auto getSqlMany(auto& first, auto&& ... remains)
    {
        if (auto result = getSqlOne(first); failure(result))
        {
            return result;
        }

        return getSqlMany(remains...);
    }

    auto getSqlMany() { return ""; }
};