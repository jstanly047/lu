#pragma once
#include <time.h>
#include <string>

namespace lu::utils
{
    class TimerClock
    {
    public:
        TimerClock(const std::string& statName);
        void begin();
        long long int end();
        std::string getStatInMilliSec() const;
        std::string getStatInSec() const;

    protected:
        std::string m_statName;
        
    private:
        struct timespec m_start{};
        struct timespec m_end{};
        unsigned long long int m_diff = 0;
    };

    class AccumulateAndAverage : private TimerClock
    {
    public:
        AccumulateAndAverage(const std::string& statName);
        void begin();
        void end(unsigned long long int qty);
        std::string getStatInMilliSec() const;
        std::string getStatInSec() const;
    
    private:
        unsigned long long int m_totalTimeSpendInNanoSec = 0;
        unsigned long long int m_totalQty = 0;
    };
}