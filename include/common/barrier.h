#include <atomic>

namespace lu::common
{
    class Barrier 
    {
    public:
        void wait() noexcept;
        void release(unsigned expected_counter) noexcept;
    
    private:
        std::atomic<unsigned> m_counter{};
    };
}