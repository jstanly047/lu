#pragma once
#include <type_traits>


namespace lu::common
{
    template <typename T>
    concept ClassOrStruct = std::is_class_v<T>;


    template <typename T>
    concept NonPtrClassOrStruct = ClassOrStruct<T> && !std::is_pointer_v<T>;
}