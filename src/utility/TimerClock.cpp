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

TimerClock::TimerClock(const std::string& statName) : m_statName(statName)
{

}

void TimerClock::begin()
{
    timespec_get(&m_start, TIME_UTC);
}

long long int TimerClock::end()
{
    assert(m_start.tv_sec != 0 || m_start.tv_nsec != 0);
    timespec_get(&m_end, TIME_UTC);
    m_diff = (m_end.tv_sec - m_start.tv_sec) * 1'000'000'000 + (m_end.tv_nsec - m_start.tv_nsec);
    return m_diff;
}

std::string TimerClock::getStatInMilliSec() const
{
    return m_statName + " - Total Time (ms): " + std::to_string(getMilliSec(m_diff));
}

std::string TimerClock::getStatInSec() const
{
    return m_statName + " - Total Time (sec): " + std::to_string(getSec(m_diff));
}

AccumulateAndAverage::AccumulateAndAverage(const std::string& statName) : TimerClock(statName)
{

}

void AccumulateAndAverage::begin()
{
    TimerClock::begin();
}

void AccumulateAndAverage::end(unsigned long long int qty)
{
    m_totalTimeSpendInNanoSec += TimerClock::end();
    m_totalQty += qty;
}

std::string AccumulateAndAverage::getStatInMilliSec() const
{
    return m_statName + " - Total Time (ms): " + std::to_string(getMilliSec(m_totalTimeSpendInNanoSec)) +
                            " Total Qty : " + std::to_string(m_totalQty) +
                            " PerMSAvg : " + std::to_string(m_totalQty / getMilliSec(m_totalTimeSpendInNanoSec) );
}

std::string AccumulateAndAverage::getStatInSec() const
{
    return m_statName + " - Total Time (sec): " + std::to_string(getSec(m_totalTimeSpendInNanoSec)) +
                            " Total Qty : " + std::to_string(m_totalQty) +
                            " PerSecAvg : " + std::to_string(m_totalQty / getSec(m_totalTimeSpendInNanoSec) );
}