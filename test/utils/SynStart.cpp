#include <utils/WaitForCount.h>

using namespace lu::utils;

WaitForCount::WaitForCount(unsigned int numOfCountWaiting) : m_numberOfCountWaiting(numOfCountWaiting)
{
}

void WaitForCount::increment()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_count++;
    if (m_count == m_numberOfCountWaiting)
    {
        m_cv.notify_all();
    }
}

void WaitForCount::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [&]
              { return m_count == m_numberOfCountWaiting; });
}

 void  WaitForCount::update(unsigned int numOfCountWaiting)
 {
    m_numberOfCountWaiting = numOfCountWaiting;
 }
