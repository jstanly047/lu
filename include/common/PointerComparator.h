#pragma once
#include <memory>
#include <type_traits>

namespace lu::common
{
    template <class T>
    struct PointerComparator
    {
        typedef std::true_type is_transparent;

        struct helper
        {
            T *ptr;
            helper() : ptr(nullptr) {}
            helper(helper const &) = default;
            helper(T *p) : ptr(p) {}
            
            template <class U, class... Ts>
            helper(std::shared_ptr<U, Ts...> const &sp) : ptr(sp.get()) {}

            template <class U, class... Ts>
            helper(std::unique_ptr<U, Ts...> const &up) : ptr(up.get()) {}

            bool operator<(helper o) const
            {
                return std::less<T *>()(ptr, o.ptr);
            }
        };

        bool operator()(helper const &&lhs, helper const &&rhs) const
        {
            return lhs < rhs;
        }
    };
}