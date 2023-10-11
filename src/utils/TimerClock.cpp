#include <cassert>
#include <utils/TimerClock.h>

using namespace lu::utils;

namespace
{
    constexpr double NANO_TO_MILLI_SEC = 1'000'000.0;
    constexpr double NANO_TO_SEC = 1'000'000'000.0;
    constexpr double getMilliSec(int long long nanoSec)
    {
        return static_cast<double>(nanoSec) / NANO_TO_MILLI_SEC;
    }

    constexpr double getSec(int long long nanoSec)
    {
        return static_cast<double>(nanoSec) / NANO_TO_SEC;
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
    return static_cast<long long int>(getMilliSec(m_totalTimeSpendInNanoSec));
}

long long int AccumulateAndAverage::getTotalElapsedTimeSec() const
{
    return static_cast<long long int>(getSec(m_totalTimeSpendInNanoSec));
}

std::string AccumulateAndAverage::getStatInMilliSec() const
{
    return "Total Time (ms): " + std::to_string(getMilliSec(m_totalTimeSpendInNanoSec)) +
                            " Total Qty : " + std::to_string(m_totalQty) +
                            " PerMSAvg : " + std::to_string(static_cast<double>(m_totalQty) / getMilliSec(m_totalTimeSpendInNanoSec) );
}

std::string AccumulateAndAverage::getStatInSec() const
{
    return "Total Time (sec): " + std::to_string(getSec(m_totalTimeSpendInNanoSec)) +
                            " Total Qty : " + std::to_string(m_totalQty) +
                            " PerSecAvg : " + std::to_string(static_cast<double>(m_totalQty) / getSec(m_totalTimeSpendInNanoSec) );
}