#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <utils/TimerClock.h>

using namespace lu::utils;

TEST(TestTimerClock, ElapsedTimeInMSec) 
{
    TimerClock timer;
    timer.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    long long int elapsed = timer.getElapsedTimeMSec();
    EXPECT_GE(elapsed, 100);
    EXPECT_LE(elapsed, 150);
}

TEST(TestTimerClock, ElapsedTimeInSec) 
{
    TimerClock timer;
    timer.begin();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    long long int elapsed = timer.getElapsedTimeSec();
    EXPECT_GE(elapsed, 1);
    EXPECT_LE(elapsed, 1);
}

TEST(TestTimerClock, StatInMSec) 
{
    AccumulateAndAverage accumulator;
    accumulator.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    accumulator.end(1000);
    EXPECT_GE(accumulator.getTotalElapsedTimeMSec(), 100);
    EXPECT_LE(accumulator.getTotalElapsedTimeMSec(), 150);
    EXPECT_LE(accumulator.getTotalQty(), 1000);
    accumulator.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    accumulator.end(2000);
    EXPECT_GE(accumulator.getTotalElapsedTimeMSec(), 200);
    EXPECT_LE(accumulator.getTotalElapsedTimeMSec(), 250);
    EXPECT_LE(accumulator.getTotalQty(), 3000);
    std::string stat = accumulator.getStatInMilliSec();
    EXPECT_EQ(stat, "Total Time (ms): " + std::to_string(accumulator.getTotalElapsedTimeMSec()) + ".000000 Total Qty : 3000 PerMSAvg : 15.000000");
}

TEST(TestTimerClock, StatInSec) 
{
    AccumulateAndAverage accumulator;
    accumulator.begin();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    accumulator.end(5);
    EXPECT_GE(accumulator.getTotalElapsedTimeSec(), 1);
    EXPECT_LE(accumulator.getTotalElapsedTimeSec(), 1);
    EXPECT_LE(accumulator.getTotalQty(), 5);
    accumulator.begin();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    accumulator.end(11);
    EXPECT_GE(accumulator.getTotalElapsedTimeSec(), 2);
    EXPECT_LE(accumulator.getTotalElapsedTimeSec(), 3);
    EXPECT_LE(accumulator.getTotalQty(), 16);
    std::string stat = accumulator.getStatInSec();
    EXPECT_EQ(stat, "Total Time (sec): " + std::to_string(accumulator.getTotalElapsedTimeSec()) + ".000000 Total Qty : 16 PerSecAvg : 8.000000");
}