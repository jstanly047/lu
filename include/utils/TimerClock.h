#pragma once

#include <string>
#include <chrono>

namespace lu::utils
{
    class TimerClock
    {
    public:
        void begin();
        long long int getElapsedTimeMSec() const;
        long long int getElapsedTimeSec() const;

    protected:
        std::chrono::system_clock::time_point m_start;
    };

    class AccumulateAndAverage : private TimerClock
    {
    public:
        void begin();
        void end(unsigned long long int qty);

        std::string getStatInMilliSec() const;
        std::string getStatInSec() const;
        long long int getTotalElapsedTimeMSec() const;
        long long int getTotalElapsedTimeSec() const;
        long long int getTotalQty() const { return m_totalQty; }
    
    private:
        long long int m_totalTimeSpendInNanoSec = 0;
        unsigned long long int m_totalQty = 0;
    };
}
