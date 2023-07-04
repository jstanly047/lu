#include <common/spinlock.h>
#include <common/defs.h>
#include <cstdlib>

using namespace lu::common;

Spinlock::Spinlock() noexcept 
{
    if(UNLIKELY(::pthread_spin_init(&m_pthreadSpinlockT, 0)))
        std::abort();
}

Spinlock::~Spinlock() noexcept 
{
    ::pthread_spin_destroy(&m_pthreadSpinlockT);
}

void Spinlock::lock() noexcept 
{
    if(UNLIKELY(::pthread_spin_lock(&m_pthreadSpinlockT)))
        std::abort();
}

void Spinlock::unlock() noexcept 
{
    if(UNLIKELY(::pthread_spin_unlock(&m_pthreadSpinlockT)))
        std::abort();
}




TicketSpinlock::LockGuard::LockGuard(TicketSpinlock& ticketSpinlock) noexcept
    : m_ticketSpinlock(&ticketSpinlock)
    , m_ticket(ticketSpinlock.lock())
{}


TicketSpinlock::LockGuard::~LockGuard() noexcept 
{
    m_ticketSpinlock->unlock(m_ticket);
}



NOINLINE unsigned TicketSpinlock::lock() noexcept 
{
    auto ticket = m_ticket.fetch_add(1, std::memory_order_relaxed);
    for(;;) 
    {
        auto position = ticket - m_next.load(std::memory_order_acquire);

        if(LIKELY(!position))
        {
            break;
        }
        
        do
        {
            spinLoopPause();
        }while(--position);
    }
    return ticket;
}

void TicketSpinlock::unlock() noexcept 
{
    unlock(m_next.load(std::memory_order_relaxed) + 1);
}

void TicketSpinlock::unlock(unsigned ticket) noexcept 
{
    m_next.store(ticket + 1, std::memory_order_release);
}


void UnfairSpinlock::lock() noexcept 
{
    for(;;) 
    {
        if(!m_lock.load(std::memory_order_relaxed) && 
            !m_lock.exchange(1, std::memory_order_acquire))
        {
            return;
        }
        spinLoopPause();
    }
}

void UnfairSpinlock::unlock() noexcept 
{
    m_lock.store(0, std::memory_order_release);
}
