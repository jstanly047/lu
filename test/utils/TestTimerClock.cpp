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

    double actualTime;
    double actualPerSecAvg;
    std::sscanf(accumulator.getStatInMilliSec().c_str(), "Total Time (ms): %lf Total Qty : 3000 PerMSAvg : %lf", &actualTime, &actualPerSecAvg);

    // Compare floating-point values with a tolerance (e.g., 1e-6)
    EXPECT_GE(actualTime, 200.0);
    EXPECT_LE(actualTime, 214.0);
    EXPECT_GE(actualPerSecAvg, 14.0);
    EXPECT_LE(actualPerSecAvg, 15.0);
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

    double actualTime;
    double actualPerSecAvg;
    std::sscanf(accumulator.getStatInSec().c_str(), "Total Time (sec): %lf Total Qty : 16 PerSecAvg : %lf", &actualTime, &actualPerSecAvg);

    // Compare floating-point values with a tolerance (e.g., 1e-6)
    EXPECT_GE(actualTime, 2.0);
    EXPECT_LE(actualTime, 3.0);
    EXPECT_GE(actualPerSecAvg, 7.0);
    EXPECT_LE(actualPerSecAvg, 8.1);
}