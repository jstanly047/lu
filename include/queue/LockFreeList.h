#pragma once
#include <utility>
#include <memory>
#include <condition_variable>
#include <atomic>

/*
This has ABA problem to solve it...need to use share_ptr

C++20 has support for atomic<shared_ptr>>
For C+11/14/17
share_ptr<T> a == automic<shared_ptr<T>> a
auto p = a.load() == auto p = atomic_load(&a)
a.compare_exchange_week(e,d) == automic_compare_exchange_weak(&a, &e, d)
*/

namespace queue{
    template<typename Node>
    class LockFreeList{
        public:
            LockFreeList() = default;
            ~LockFreeList()
            {
                auto p = m_head.load();
                for (;p != nullptr; p = p->next)
                {
                    delete p;
                }
            }

            void push_front(Node* n)
            {
                n->next = m_head;
                while(!m_head.compare_exchange_weak(n->next, n))
                {

                }
            }

            Node* pop_front(){
                auto n = m_head.load();

                while (n && !m_head.compare_exchange_weak(n, n->next))
                {  
                }

                if (n != nullptr)
                {
                    n->next = nullptr;
                }
                return n;
            }

        private:
            std::atomic<Node*> m_head;
    };
}