#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <utils/TimerClock.h>

using namespace lu::utils;

TEST(TestTimerClock, ElapsedTime) {
    TimerClock timer;
    timer.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    long long int elapsed = timer.getElapsedTimeMSec();
    EXPECT_GE(elapsed, 100);
    EXPECT_LE(elapsed, 150);
}

TEST(TestTimerClock, Stat) {
    AccumulateAndAverage accumulator;
    accumulator.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    accumulator.end(5);
    std::string stat = accumulator.getStatInMilliSec();
    EXPECT_GE(accumulator.getTotalElapsedTimeMSec(), 100);
    EXPECT_LE(accumulator.getTotalElapsedTimeMSec(), 150);
    EXPECT_LE(accumulator.getTotalQty(), 5);
    accumulator.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    accumulator.end(12);
    EXPECT_GE(accumulator.getTotalElapsedTimeMSec(), 200);
    EXPECT_LE(accumulator.getTotalElapsedTimeMSec(), 250);
    EXPECT_LE(accumulator.getTotalQty(), 17);
}