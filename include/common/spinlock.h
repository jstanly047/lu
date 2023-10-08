#pragma once

#include <atomic>
#include <cstdlib>
#include <mutex>
#include <pthread.h>
#include <common/defs.h>


namespace lu::common 
{
    class Spinlock 
    {
    public:
        using scoped_lock = std::lock_guard<Spinlock>;
        Spinlock(Spinlock const&) = delete;
        Spinlock& operator=(Spinlock const&) = delete;

        Spinlock() noexcept;
        ~Spinlock() noexcept;
        void lock() noexcept;
        void unlock() noexcept;

    private:
        pthread_spinlock_t m_pthreadSpinlockT{};
    };

    class TicketSpinlock 
    {
    public:
        class LockGuard 
        {
        public:
            LockGuard(LockGuard const&) = delete;
            LockGuard& operator=(LockGuard const&) = delete;
            LockGuard(TicketSpinlock& ticketSpinlock) noexcept;
            ~LockGuard() noexcept;

        private:
            TicketSpinlock* const m_ticketSpinlock;
            unsigned const m_ticket;
        };

        using scoped_lock = LockGuard;

        TicketSpinlock(TicketSpinlock const&) = delete;
        TicketSpinlock& operator=(TicketSpinlock const&) = delete;
        TicketSpinlock() noexcept = default;

        NOINLINE unsigned lock() noexcept;
        void unlock() noexcept;
        void unlock(unsigned ticket) noexcept;

    private:
        alignas(CACHE_LINE_SIZE) std::atomic<unsigned> m_ticket{0};
        alignas(CACHE_LINE_SIZE) std::atomic<unsigned> m_next{0};
    };


    class UnfairSpinlock {
    public:
        using scoped_lock = std::lock_guard<UnfairSpinlock>;

        UnfairSpinlock(UnfairSpinlock const&) = delete;
        UnfairSpinlock& operator=(UnfairSpinlock const&) = delete;

        void lock() noexcept;
        void unlock() noexcept;

    private:
        std::atomic<unsigned> m_lock{0};
    };
}