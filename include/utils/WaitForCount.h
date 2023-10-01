#pragma once

#include <mutex>
#include <condition_variable>

namespace lu::utils
{
    class WaitForCount
    {
    public:
        WaitForCount(unsigned int numOfCountWaiting);
        void increment();
        void wait();
        void update(unsigned int numOfCountWaiting);

    private:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        unsigned int m_numberOfCountWaiting{};
        unsigned int m_count{};
    };
}