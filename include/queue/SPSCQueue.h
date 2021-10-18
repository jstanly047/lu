#pragma once
#include <list>
#include <utility>
#include <memory>
#include <condition_variable>
#include<atomic>

namespace queue{

    template <unsigned int N = 1032>
    class SPSCQueue
    {
        struct Queue{
            std::atomic<void*> m_buffer[N];
            std::atomic<Queue*> m_next = nullptr;
        };

    public:
        SPSCQueue(const SPSCQueue& ) = delete;
        SPSCQueue& operator=(const SPSCQueue& ) = delete;
        

        SPSCQueue(): m_currentHeadQueue(new Queue()),
                     m_currentTailQueue(this->m_currentHeadQueue),
                     m_freeSlotsHead(new Queue()),
                     m_lastIndex(N - 1)
        {
        }

        ~SPSCQueue()
        {
            auto freeQueue = [](Queue *queue)
            {
                for (; queue != nullptr;)
                {
                    auto temp = std::exchange(queue, queue->m_next);
                    delete temp;
                }
            };

            freeQueue(m_currentHeadQueue);
            freeQueue(m_freeSlotsHead);
        }

        void push_back(void* ptr)
        {
            if (m_lastIndex == N)
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

                m_currentHeadQueue->m_next = newQueue;
                newQueue[0] = ptr;
                m_tail = 0;
            }
            else
            {
                m_currentTailQueue[++m_tail] = ptr;
            }

            if (m_dateInQueue == false)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_dateInQueue = true;
                }
                m_cv.notify_one();
            }
        }

        void *pop()
        {
            if (m_head = m_lastIndex)
            {
                auto consumedQueue = m_currentHeadQueue;
                m_currentHeadQueue = m_currentHeadQueue->m_next;
                consumedQueue->m_next = m_freeSlotsHead;
                while (!m_freeSlotsHead.compare_exchange_weak(consumedQueue->m_next, consumedQueue))
                {
                }

                m_head = -1;
            }

            auto retVal = m_currentHeadQueue[++m_head];

            while (retVal == nullptr)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_dateInQueue = false;
                m_cv.wait(lock, [&] {return m_dateInQueue;});
                retVal = m_currentHeadQueue[m_head];
            }

            m_currentHeadQueue[m_head] = nullptr;
            return retVal;
        }

    private:
        Queue* m_currentHeadQueue{};
        Queue* m_currentTailQueue{};
        std::atomic<Queue*> m_freeSlotsHead{};
        std::condition_variable m_cv{};
        std::mutex m_mutex{};
        std::atomic<bool> m_dateInQueue = false;
        const unsigned int m_lastIndex{};
        int m_head{-1};
        int m_tail{-1};
    };
}