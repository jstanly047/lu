#pragma once
#include <list>
#include <utility>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <iostream>

namespace queue{

    template <unsigned int N = 1032>
    class SPSCQueue
    {
        struct Queue{
            std::atomic<void*> m_buffer[N];
            Queue* m_next = nullptr;
        };

    public:
        SPSCQueue(const SPSCQueue& ) = delete;
        SPSCQueue& operator=(const SPSCQueue& ) = delete;
        

        SPSCQueue(): m_currentHeadQueue(new Queue()),
                     m_currentTailQueue(this->m_currentHeadQueue),
                     m_freeSlotsHead(new Queue()),
                     m_lastIndex(N)
        {
        }

        ~SPSCQueue()
        {
            auto freeQueue = [](Queue *queue)
            {
               // for (; queue != nullptr;)
                {
                    // auto temp = std::exchange(queue, queue->m_next);
                    //delete temp;
                }
            };

            freeQueue(m_currentHeadQueue);
            freeQueue(m_freeSlotsHead);
        }

        void push_back(void* ptr)
        {
            m_currentTailQueue->m_buffer[m_tail].store(ptr, std::memory_order_relaxed);
            m_tail++;

            if (m_tail == m_lastIndex)
            {

                Queue *newQueue = m_freeSlotsHead.load();

                if (newQueue == nullptr)
                {
                    newQueue = new Queue();
                }
                else
                {
                    while (!m_freeSlotsHead.compare_exchange_weak(newQueue, newQueue->m_next))
                    {
                    }
                }

                m_currentTailQueue->m_next = newQueue;
                m_currentTailQueue = newQueue;
                m_tail = 0;
                assert(m_currentTailQueue != nullptr);
            }

            auto count = m_dataInQueue.fetch_add(1, std::memory_order_seq_cst);
            if (count == 1)
            {
                //std::cout << "NOTFIY " << (unsigned int) *(unsigned int*) ptr << std::endl;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                }
                m_cv.notify_one();
            }
        }

        void *pop()
        {
            if (m_head == m_lastIndex)
            {
                auto consumedQueue = m_currentHeadQueue;
                m_currentHeadQueue = m_currentHeadQueue->m_next;

                if (m_currentHeadQueue == nullptr){
                    assert(false);
                    // race need to handle
                }

                consumedQueue->m_next = m_freeSlotsHead;

                while (!m_freeSlotsHead.compare_exchange_weak(consumedQueue->m_next, consumedQueue))
                {
                }

                m_head = 0;
            }

            auto retVal = m_currentHeadQueue->m_buffer[m_head].load();

            while (retVal == nullptr)
            {
                if (m_dataInQueue.load() == 0)
                {
                   //std::cout << "Wait " << std::endl;
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [&] {return m_dataInQueue.load() != 0;});
                }
                retVal = m_currentHeadQueue->m_buffer[m_head].load();
                //std::cout << "RESUME " << (unsigned int) * (unsigned int*)retVal <<std::endl;
            }

            m_currentHeadQueue->m_buffer[m_head] = nullptr;
            m_head++;
            m_dataInQueue.fetch_sub(1, std::memory_order_relaxed);
            assert (expected == (unsigned int) * (unsigned int*) retVal);
            expected++;
            return retVal;
        }

    private:
        Queue* m_currentHeadQueue{};
        Queue* m_currentTailQueue{};
        std::atomic<Queue*> m_freeSlotsHead{};
        std::condition_variable m_cv{};
        std::mutex m_mutex{};
        std::atomic<unsigned int> m_dataInQueue{0};
        const unsigned int m_lastIndex{};
        unsigned int m_head{0};
        unsigned int m_tail{0};
        unsigned int expected = 0;
    };
}