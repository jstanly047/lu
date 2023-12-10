#pragma once
#include <storage/db/MetaData.h>
#include <common/FixedString.h>

namespace lu::storage::db
{
    constexpr char const* const connectString = "db=lu_test user=stanly password='zoo9Ieho!1qaz' host=127.0.0.1 port=3306";

    struct GTestClass
    {
        std::string name;
        int age;
        double salary;
    };


    struct MyClass
    {
        std::string name;
        int age;
    };

    struct FixedStr
    {
        lu::common::FixedString<15> name;
        int age;
        lu::common::FixedString<31> description;
    };

}

DB_REFLECTION_ALIAS(lu::storage::db::GTestClass, gtest_class, name, age, salary);
DB_REFLECTION_ALIAS(lu::storage::db::MyClass, MyClass, name, age);
DB_REFLECTION_ALIAS(lu::storage::db::FixedStr, FixedStr, name, age, description);