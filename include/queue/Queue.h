#pragma once

#include <common/common.h>
#include <common/defs.h>
#include <atomic>
#include <cassert>

namespace lu::queue
{
    template<class Derived>
    class QueueBase 
    {
    protected:
        QueueBase() noexcept = default;

        QueueBase(const QueueBase& luQueue) noexcept
            : m_head(luQueue.m_head.load(std::memory_order_relaxed))
            , m_tail(luQueue.m_tail.load(std::memory_order_relaxed)) 
        {}

        QueueBase& operator=(const QueueBase& luQueue) noexcept 
        {
            m_head.store(luQueue.m_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
            m_tail.store(luQueue.m_tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
            return *this;
        }

        void swap(QueueBase& luQueue) noexcept 
        {
            unsigned h = m_head.load(std::memory_order_relaxed);
            unsigned t = m_tail.load(std::memory_order_relaxed);
            m_head.store(luQueue.m_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
            m_tail.store(luQueue.m_tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
            luQueue.m_head.store(h, std::memory_order_relaxed);
            luQueue.m_tail.store(t, std::memory_order_relaxed);
        }

        template<class T, T NIL>
        static T do_pop_atomic(std::atomic<T>& qElement) noexcept 
        {
            if(Derived::m_isSPSC) 
            {
                for(;;) 
                {
                    T element = qElement.load(std::memory_order_acquire);

                    if(LIKELY(element != NIL)) 
                    {
                        qElement.store(NIL, std::memory_order_relaxed);
                        return element;
                    }

                    if(Derived::m_isMaximizeThroughputEnable)
                        lu::common::spinLoopPause();
                }
            }
            else 
            {
                for(;;) 
                {
                    T element = qElement.exchange(NIL, std::memory_order_acquire); // (2) The store to wait for.
                    
                    if(LIKELY(element != NIL))
                    {
                        return element;
                    }

                    // Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
                    do
                    {
                        lu::common::spinLoopPause();
                    }while(Derived::m_isMaximizeThroughputEnable && qElement.load(std::memory_order_relaxed) == NIL);
                }
            }
        }

        template<class T, T NIL>
        static void do_push_atomic(T element, std::atomic<T>& qElement) noexcept 
        {
            assert(element != NIL);

            if(Derived::m_isSPSC) 
            {
                while(UNLIKELY(qElement.load(std::memory_order_relaxed) != NIL))
                {
                    if(Derived::m_isMaximizeThroughputEnable)
                    {
                        lu::common::spinLoopPause();
                    }
                }

                qElement.store(element, std::memory_order_release);
            }
            else 
            {
                for(T expected = NIL; 
                    UNLIKELY(!qElement.compare_exchange_strong(expected, element, std::memory_order_release, std::memory_order_relaxed)); 
                    expected = NIL) 
                {
                    do
                    {
                        lu::common::spinLoopPause(); // (1) Wait for store (2) to complete.
                    }while(Derived::m_isMaximizeThroughputEnable && qElement.load(std::memory_order_relaxed) != NIL);
                }
            }
        }

        enum State : unsigned char { EMPTY, STORING, STORED, LOADING };

        template<class T>
        static T do_pop_any(std::atomic<unsigned char>& state, T& qElement) noexcept
        {
            if(Derived::m_isSPSC) 
            {
                while(UNLIKELY(state.load(std::memory_order_acquire) != STORED))
                {
                    if(Derived::m_isMaximizeThroughputEnable)
                    {
                        lu::common::spinLoopPause();
                    }
                }

                T element{std::move(qElement)};
                state.store(EMPTY, std::memory_order_release);
                return element;
            }
            else 
            {
                for(;;)
                {
                    unsigned char expected = STORED;

                    if(LIKELY(state.compare_exchange_strong(expected, LOADING, std::memory_order_acquire, std::memory_order_relaxed))) 
                    {
                        T element{std::move(qElement)};
                        state.store(EMPTY, std::memory_order_release);
                        return element;
                    }

                    // Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
                    do
                    {
                        lu::common::spinLoopPause();
                    }while(Derived::m_isMaximizeThroughputEnable && state.load(std::memory_order_relaxed) != STORED);
                }
            }
        }

        template<class U, class T>
        static void do_push_any(U&& element, std::atomic<unsigned char>& state, T& qElement) noexcept 
        {
            if(Derived::m_isSPSC)
            {
                while(UNLIKELY(state.load(std::memory_order_acquire) != EMPTY))
                {
                    if(Derived::m_isMaximizeThroughputEnable)
                    {
                        lu::common::spinLoopPause();
                    }
                }
                qElement = std::forward<U>(element);
                state.store(STORED, std::memory_order_release);
            }
            else
            {
                for(;;) 
                {
                    unsigned char expected = EMPTY;
                    if(LIKELY(state.compare_exchange_strong(expected, STORING, std::memory_order_acquire, std::memory_order_relaxed))) 
                    {
                        qElement = std::forward<U>(element);
                        state.store(STORED, std::memory_order_release);
                        return;
                    }
                    // Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
                    do
                    {
                        lu::common::spinLoopPause();
                    }while(Derived::m_isMaximizeThroughputEnable && state.load(std::memory_order_relaxed) != EMPTY);
                }
            }
        }

    public:
        template<class T>
        bool try_push(T&& element) noexcept 
        {
            auto head = m_head.load(std::memory_order_relaxed);

            if(Derived::m_isSPSC)
            {
                if(static_cast<int>(head - m_tail.load(std::memory_order_relaxed)) >= static_cast<int>(static_cast<Derived&>(*this).m_queueSize))
                {
                    return false;
                }

                m_head.store(head + 1, std::memory_order_relaxed);
            }
            else
            {
                do
                {
                    if(static_cast<int>(head - m_tail.load(std::memory_order_relaxed)) >= static_cast<int>(static_cast<Derived&>(*this).m_queueSize))
                    {
                        return false;
                    }
                } while(UNLIKELY(!m_head.compare_exchange_strong(head, head + 1, std::memory_order_relaxed, std::memory_order_relaxed))); // This loop is not FIFO.
            }

            static_cast<Derived&>(*this).do_push(std::forward<T>(element), head);
            return true;
        }

        template<class T>
        bool try_pop(T& element) noexcept 
        {
            auto tail = m_tail.load(std::memory_order_relaxed);

            if(Derived::m_isSPSC) 
            {
                if(static_cast<int>(m_head.load(std::memory_order_relaxed) - tail) <= 0)
                {
                    return false;
                }

                m_tail.store(tail + 1, std::memory_order_relaxed);
            }
            else 
            {
                do
                {
                    if(static_cast<int>(m_head.load(std::memory_order_relaxed) - tail) <= 0)
                    {
                        return false;
                    }
                } while(UNLIKELY(!m_tail.compare_exchange_strong(tail, tail + 1, std::memory_order_relaxed, std::memory_order_relaxed))); // This loop is not FIFO.
            }

            element = static_cast<Derived&>(*this).do_pop(tail);
            return true;
        }

        template<class T>
        void push(T&& element) noexcept 
        {
            unsigned head;
            if(Derived::m_isSPSC) 
            {
                head = m_head.load(std::memory_order_relaxed);
                m_head.store(head + 1, std::memory_order_relaxed);
            }
            else 
            {
                constexpr auto memory_order = Derived::m_totalOrder ? std::memory_order_seq_cst : std::memory_order_relaxed;
                head = m_head.fetch_add(1, memory_order); // FIFO and total order on Intel regardless, as of 2019.
            }
            static_cast<Derived&>(*this).do_push(std::forward<T>(element), head);
        }

        auto pop() noexcept 
        {
            unsigned tail;
            if(Derived::m_isSPSC) 
            {
                tail = m_tail.load(std::memory_order_relaxed);
                m_tail.store(tail + 1, std::memory_order_relaxed);
            }
            else 
            {
                constexpr auto memory_order = Derived::m_totalOrder ? std::memory_order_seq_cst : std::memory_order_relaxed;
                tail = m_tail.fetch_add(1, memory_order); // FIFO and total order on Intel regardless, as of 2019.
            }
            return static_cast<Derived&>(*this).do_pop(tail);
        }

        bool was_empty() const noexcept 
        {
            return !was_size();
        }

        bool was_full() const noexcept 
        {
            return was_size() >= static_cast<int>(static_cast<Derived const&>(*this).m_queueSize);
        }

        unsigned was_size() const noexcept 
        {
            // m_tail can be greater than m_head because of consumers doing pop, rather that try_pop, when the queue is empty.
            return std::max(static_cast<int>(m_head.load(std::memory_order_relaxed) - m_tail.load(std::memory_order_relaxed)), 0);
        }

        unsigned capacity() const noexcept 
        {
            return static_cast<Derived const&>(*this).m_queueSize;
        }

    protected:
        alignas(lu::common::CACHE_LINE_SIZE) std::atomic<unsigned> m_head{};
        alignas(lu::common::CACHE_LINE_SIZE) std::atomic<unsigned> m_tail{};
    };

    template<class T, unsigned SIZE, T NIL = lu::common::nil<T>(), bool MINIMIZE_CONTENTION = true, bool MAXIMIZE_THROUGHPUT = true, bool TOTAL_ORDER = false, bool SPSC = false>
    class AtomicQueue : public QueueBase<AtomicQueue<T, SIZE, NIL, MINIMIZE_CONTENTION, MAXIMIZE_THROUGHPUT, TOTAL_ORDER, SPSC>> 
    {
    public:
        using value_type = T;

        AtomicQueue(AtomicQueue const&) = delete;
        AtomicQueue& operator=(AtomicQueue const&) = delete;

        AtomicQueue() noexcept 
        {
            assert(std::atomic<T>{NIL}.is_lock_free()); // Queue element type T is not atomic. Use AtomicQueue2/AtomicQueueB2 for such element types.
            if(lu::common::nil<T>() != NIL)
            {
                for(auto& element : m_elements)
                {
                    element.store(NIL, std::memory_order_relaxed);
                }
            }
        }
        
    private:
        using Base = QueueBase<AtomicQueue<T, SIZE, NIL, MINIMIZE_CONTENTION, MAXIMIZE_THROUGHPUT, TOTAL_ORDER, SPSC>>;
        friend Base;

        static constexpr unsigned m_queueSize = MINIMIZE_CONTENTION ? lu::common::nextPowerOfTwo(SIZE) : SIZE;
        static constexpr int m_shuffleBits = lu::common::GetIndexShuffleBits<MINIMIZE_CONTENTION, m_queueSize, lu::common::CACHE_LINE_SIZE / sizeof(std::atomic<T>)>::value;
        static constexpr bool m_totalOrder = TOTAL_ORDER;
        static constexpr bool m_isSPSC = SPSC;
        static constexpr bool m_isMaximizeThroughputEnable = MAXIMIZE_THROUGHPUT;

        alignas(lu::common::CACHE_LINE_SIZE) std::atomic<T> m_elements[m_queueSize] = {}; // Empty elements are NIL.

        T do_pop(unsigned tail) noexcept 
        {
            std::atomic<T>& qElement = lu::common::map<m_shuffleBits>(m_elements, tail % m_queueSize);
            return Base::template do_pop_atomic<T, NIL>(qElement);
        }

        void do_push(T element, unsigned head) noexcept 
        {
            std::atomic<T>& qElement = lu::common::map<m_shuffleBits>(m_elements, head % m_queueSize);
            Base::template do_push_atomic<T, NIL>(element, qElement);
        }
    };


    template<class T, unsigned SIZE, bool MINIMIZE_CONTENTION = true, bool MAXIMIZE_THROUGHPUT = true, bool TOTAL_ORDER = false, bool SPSC = false>
    class AtomicQueue2 : public QueueBase<AtomicQueue2<T, SIZE, MINIMIZE_CONTENTION, MAXIMIZE_THROUGHPUT, TOTAL_ORDER, SPSC>> {
        using Base = QueueBase<AtomicQueue2<T, SIZE, MINIMIZE_CONTENTION, MAXIMIZE_THROUGHPUT, TOTAL_ORDER, SPSC>>;
        using State = typename Base::State;
        friend Base;

        static constexpr unsigned m_queueSize = MINIMIZE_CONTENTION ? lu::common::nextPowerOfTwo(SIZE) : SIZE;
        static constexpr int m_shuffleBits = lu::common::GetIndexShuffleBits<MINIMIZE_CONTENTION, m_queueSize, lu::common::CACHE_LINE_SIZE / sizeof(State)>::value;
        static constexpr bool m_totalOrder = TOTAL_ORDER;
        static constexpr bool m_isSPSC = SPSC;
        static constexpr bool m_isMaximizeThroughputEnable = MAXIMIZE_THROUGHPUT;

        alignas(lu::common::CACHE_LINE_SIZE) std::atomic<unsigned char> m_states[m_queueSize] = {};
        alignas(lu::common::CACHE_LINE_SIZE) T m_elements[m_queueSize] = {};

        T do_pop(unsigned tail) noexcept 
        {
            unsigned index = lu::common::remapIndex<m_shuffleBits>(tail % m_queueSize);
            return Base::template do_pop_any(m_states[index], m_elements[index]);
        }

        template<class U>
        void do_push(U&& element, unsigned head) noexcept 
        {
            unsigned index = lu::common::remapIndex<m_shuffleBits>(head % m_queueSize);
            Base::template do_push_any(std::forward<U>(element), m_states[index], m_elements[index]);
        }

    public:
        using value_type = T;

        AtomicQueue2() noexcept = default;
        AtomicQueue2(AtomicQueue2 const&) = delete;
        AtomicQueue2& operator=(AtomicQueue2 const&) = delete;
    };

    template<class Queue>
    struct RetryDecorator : Queue 
    {
        using T = typename Queue::value_type;

        void push(T element) noexcept 
        {
            while(!this->try_push(element))
            {
                lu::common::spinLoopPause();
            }
        }

        T pop() noexcept 
        {
            T element;

            while(!this->try_pop(element))
            {
                lu::common::spinLoopPause();
            }

            return element;
        }
    };
}


