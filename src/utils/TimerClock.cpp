#include <utils/TimerClock.h>
#include <assert.h>

using namespace lu::utils;

namespace
{
    constexpr double getMilliSec(int long long nanoSec)
    {
        return nanoSec / (1'000'000);
    }

    constexpr double getSec(int long long nanoSec)
    {
        return nanoSec / (1'000'000'000);
    }
}

void TimerClock::begin()
{
    m_start = std::chrono::system_clock::now();
}

long long int  TimerClock::getElapsedTimeMSec() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_start).count();
}

long long int TimerClock::getElapsedTimeSec() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - m_start).count();
}

void AccumulateAndAverage::begin()
{
    TimerClock::begin();
}

void AccumulateAndAverage::end(unsigned long long int qty)
{
    m_totalTimeSpendInNanoSec += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - m_start).count();
    m_totalQty += qty;
}

long long int AccumulateAndAverage::getTotalElapsedTimeMSec() const
{
    return getMilliSec(m_totalTimeSpendInNanoSec);
}

long long int AccumulateAndAverage::getTotalElapsedTimeSec() const
{
    return getSec(m_totalTimeSpendInNanoSec);
}

std::string AccumulateAndAverage::getStatInMilliSec() const
{
    return "Total Time (ms): " + std::to_string(getMilliSec(m_totalTimeSpendInNanoSec)) +
                            " Total Qty : " + std::to_string(m_totalQty) +
                            " PerMSAvg : " + std::to_string(m_totalQty / getMilliSec(m_totalTimeSpendInNanoSec) );
}

std::string AccumulateAndAverage::getStatInSec() const
{
    return " Total Time (sec): " + std::to_string(getSec(m_totalTimeSpendInNanoSec)) +
                            " Total Qty : " + std::to_string(m_totalQty) +
                            " PerSecAvg : " + std::to_string(m_totalQty / getSec(m_totalTimeSpendInNanoSec) );
}