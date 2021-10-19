#pragma once
#include <list>
#include <utility>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <iostream>
#include <chrono>
#include <queue/LockFreeList.h>


using namespace std::chrono_literals;

namespace queue{

    template <unsigned int N = 1032>
    class SPSCQueue
    {
        struct Queue{
            std::atomic<void*> m_buffer[N]{nullptr};
            Queue* next{nullptr};
        };

    public:
        SPSCQueue(const SPSCQueue& ) = delete;
        SPSCQueue& operator=(const SPSCQueue& ) = delete;
        

        SPSCQueue(): m_currentHeadQueue(new Queue()),
                     m_currentTailQueue(this->m_currentHeadQueue),
                     m_lastIndex(N)
        {
        }

        ~SPSCQueue()
        {
            delete m_currentHeadQueue;
        }

        void push_back(void* ptr)
        {
            if (m_tail == m_lastIndex)
            {
                Queue *newQueue = m_lockFreeList.pop_front() ?:new Queue();

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_currentTailQueue->next = newQueue;
                }

                m_cv.notify_one();
                m_currentTailQueue = newQueue;
                assert(m_currentTailQueue != nullptr);
                m_tail = 0;
            }

            m_currentTailQueue->m_buffer[m_tail].store(ptr, std::memory_order_relaxed);

            if (m_dataInQueue.fetch_add(1, std::memory_order_acq_rel) == 1 || m_consumerWait.load(std::memory_order_acq_rel) == true)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_consumerWait = false;
                }
                m_cv.notify_one();
            }

            m_tail++;
        }

        void *pop()
        {
            if (m_head == m_lastIndex)
            {
                Queue* consumedQueue = nullptr;
                {
                    consumedQueue = m_currentHeadQueue;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_currentHeadQueue = m_currentHeadQueue->next;
                    }

                    while (m_currentHeadQueue == nullptr)
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_cv.wait(lock, [&] {return m_dataInQueue.load(std::memory_order_acq_rel) != 0;});
                        m_currentHeadQueue = consumedQueue->next;
                    }
                    
                }

                m_lockFreeList.push_front(consumedQueue);
                m_head = 0;
            }

            auto retVal = m_currentHeadQueue->m_buffer[m_head].load(std::memory_order_acquire);

            while (retVal == nullptr)
            {
                if (m_dataInQueue.load(std::memory_order_acq_rel) == 0)
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_consumerWait = true;
                    m_cv.wait(lock, [&] {return m_dataInQueue.load(std::memory_order_acq_rel) != 0;});
                }
                retVal = m_currentHeadQueue->m_buffer[m_head].load(std::memory_order_acquire);
            }

            m_currentHeadQueue->m_buffer[m_head].store(nullptr, std::memory_order_release);
            m_dataInQueue.fetch_sub(1, std::memory_order_relaxed);
            m_head++;
            return retVal;
        }

    private:
        Queue* m_currentHeadQueue{};
        Queue* m_currentTailQueue{};
        LockFreeList<Queue> m_lockFreeList{};
        std::condition_variable m_cv{};
        std::mutex m_mutex{};
        std::atomic<unsigned int> m_dataInQueue{0};
        const unsigned int m_lastIndex{};
        unsigned int m_head{0};
        unsigned int m_tail{0};
        std::atomic<bool> m_consumerWait = true;
    };
}