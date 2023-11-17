#pragma once

#include <reflection/Reflection.h>
#include <soci/soci.h>
#include <type_traits>

namespace lu::storage::db
{
    
}

#define MAKE_NAMES(...) #__VA_ARGS__,

#define MEMBER_PTR(STRUCT_NAME, member)  &STRUCT_NAME::member
#define MAKE_ARG_LIST_HELPER(STRUCT_NAME, member, ...)                              \
        MEMBER_PTR(STRUCT_NAME, member)                                             \
        __VA_OPT__(, MAKE_ARG_LIST_AGAIN PARENS (STRUCT_NAME, __VA_ARGS__))
#define MAKE_ARG_LIST_AGAIN() MAKE_ARG_LIST_HELPER
#define MAKE_ARG_LIST(STRUCT_NAME, ...)                                             \
    __VA_OPT__(EXPAND(MAKE_ARG_LIST_HELPER(STRUCT_NAME, __VA_ARGS__)))

#define POPULATE_SOCI_USE_HELPER(obj, member, ...)                                  \
        soci::use(obj.member)                                                       \
        __VA_OPT__(, POPULATE_SOCI_USE_AGAIN PARENS (obj, __VA_ARGS__))
#define POPULATE_SOCI_USE_AGAIN() POPULATE_SOCI_USE_HELPER
#define POPULATE_SOCI_USE(obj, ...)                                                 \
    __VA_OPT__(EXPAND(POPULATE_SOCI_USE_HELPER(obj, __VA_ARGS__)))

#define POPULATE_SOCI_FROM_BASE_HELPER(values, obj, member, ...)                     \
        obj.member =  values.get<decltype(obj.member)>(#member);                     \
        __VA_OPT__(POPULATE_SOCI_FROM_BASE_AGAIN PARENS (values, obj, __VA_ARGS__))
#define POPULATE_SOCI_FROM_BASE_AGAIN() POPULATE_SOCI_FROM_BASE_HELPER
#define POPULATE_SOCI_FROM_BASE(values, obj, ...)                                    \
    __VA_OPT__(EXPAND(POPULATE_SOCI_FROM_BASE_HELPER(values, obj, __VA_ARGS__)))

#define POPULATE_SOCI_TO_BASE_HELPER(values, obj, member, ...)                       \
        values.set(CONV_TO_STRING(member), obj.member);                              \
        __VA_OPT__(POPULATE_SOCI_TO_BASE_AGAIN PARENS (values, obj, __VA_ARGS__))
#define POPULATE_SOCI_TO_BASE_AGAIN() POPULATE_SOCI_TO_BASE_HELPER
#define POPULATE_SOCI_TO_BASE(values, obj, ...)                                      \
    __VA_OPT__(EXPAND(POPULATE_SOCI_TO_BASE_HELPER(values, obj, __VA_ARGS__)))




/////////////////////////////////////////////////////////////////////////////////////////
#define MAKE_META_DB_DATA_IMPL(STRUCT_NAME, TABLE_NAME, N,...)                          \
namespace soci                                                                          \
{                                                                                       \
    template<>                                                                          \
    struct type_conversion<STRUCT_NAME>                                                 \
    {                                                                                   \
        typedef values base_type;                                                       \
        static void from_base(const values& v, indicator /* ind */, STRUCT_NAME& event) \
        {                                                                               \
            POPULATE_SOCI_FROM_BASE(v, event, __VA_ARGS__)                              \
        }                                                                               \
                                                                                        \
        static void to_base(const STRUCT_NAME& event, values& v, indicator& ind)        \
        {                                                                               \
            POPULATE_SOCI_TO_BASE(v, event, __VA_ARGS__)                                \
            ind = i_ok;                                                                 \
        }                                                                               \
    };                                                                                  \
}                                                                                       \
                                                                                        \
namespace lu::storage::db                                                               \
{                                                                                       \
    [[maybe_unused]] inline static auto getDBReflection(STRUCT_NAME)             \
    {                                                                                   \
        struct DBReflection {                                                           \
        constexpr const char* getInsertSQL()                                            \
        {                                                                               \
            return CONCAT_REC("INSERT INTO " , CONV_TO_STRING(TABLE_NAME) ,             \
                "(" , CREAT_STRING_LIST(",", "", "", __VA_ARGS__ ) ,                    \
                ") VALUES (" , CREAT_STRING_LIST(",",":","", __VA_ARGS__)  , ")");      \
        }                                                                               \
                                                                                        \
        void writeToDB(soci::session& session, const STRUCT_NAME & obj)                 \
        {                                                                               \
            session << getInsertSQL() , POPULATE_SOCI_USE(obj, __VA_ARGS__);            \
        }                                                                               \
                                                                                        \
        soci::rowset<STRUCT_NAME> getFromDB(soci::session& session)                     \
        {                                                                               \
            soci::rowset<STRUCT_NAME> retVal =                                          \
            session.prepare << CONCAT_REC("SELECT * FROM ",CONV_TO_STRING(TABLE_NAME)); \
            return retVal;                                                              \
        }                                                                               \
                                                                                        \
        void truncate(soci::session& session)                                           \
        {                                                                               \
            session << CONCAT_REC("TRUNCATE TABLE ",CONV_TO_STRING(TABLE_NAME));        \
        }                                                                               \
        };                                                                              \
        return DBReflection{};                                                          \
    }                                                                                   \
}
/////////////////////////////////////////////////////////////////////////////////////////////

#define DB_REFLECTION_ALIAS(STRUCT_NAME, TABLE_NAME, ...)                                                      \
    MAKE_META_DB_DATA_IMPL(STRUCT_NAME, TABLE_NAME, lu::reflection::number_of_members<STRUCT_NAME>(), __VA_ARGS__)

#define DB_REFLECTION(STRUCT_NAME, ...)                                                                        \
    MAKE_META_DB_DATA_IMPL(STRUCT_NAME, STRUCT_NAME, lu::reflection::number_of_members<STRUCT_NAME>(), __VA_ARGS__)


