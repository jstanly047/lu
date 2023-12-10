#include <common/barrier.h>
#include <common/Defs.h>

using namespace lu::common;

void Barrier::wait() noexcept 
{
    m_counter.fetch_add(1, std::memory_order_acquire);
    while(m_counter.load(std::memory_order_relaxed) != 0U)
    {
        spinLoopPause();
    }
}

void Barrier::release(unsigned expected_counter) noexcept 
{
    while(expected_counter != m_counter.load(std::memory_order_relaxed))
    {
        spinLoopPause();
    }
    
    m_counter.store(0, std::memory_order_release);
}

