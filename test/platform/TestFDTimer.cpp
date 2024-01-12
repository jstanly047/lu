#include <platform/FDTimer.h>
#include <platform/ITimerCallback.h>

#include <fcntl.h>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


class MockFDTimerCallback : public lu::platform::ITimerCallback
{
public:
    MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<lu::platform::ITimerCallback>&), (override));
};

class TestFDTimerTest : public testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create the timer and set up the interval for testing
        fdTimer = new lu::platform::FDTimer<lu::platform::ITimerCallback>(mockTimerCallback, "TestTimer");
        ASSERT_EQ(fdTimer->getFD(), nullptr);
        ASSERT_EQ(fdTimer->init(), true);
        ASSERT_NE(fdTimer->getFD(), nullptr);
        fd = fdTimer->getFD();
        ASSERT_EQ(fdTimer->getName(), "TestTimer");
    }

    void TearDown() override 
    {
        delete fdTimer;
        int flags = ::fcntl(fd, F_GETFL);
        ASSERT_EQ(flags, lu::platform::NULL_FD);
    }

    MockFDTimerCallback mockTimerCallback;
    lu::platform::FDTimer<lu::platform::ITimerCallback>* fdTimer = nullptr;
    int fd = lu::platform::NULL_FD;
};

TEST_F(TestFDTimerTest, createFDTimer)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
}

TEST_F(TestFDTimerTest, moveConstruct)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    lu::platform::FDTimer<lu::platform::ITimerCallback> tempFDTimer(std::move(*fdTimer));
    int flags = ::fcntl(fd, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
    ASSERT_EQ(fdTimer->getFD(), nullptr);
    int tempFD = tempFDTimer.getFD();
    ASSERT_EQ(tempFD, fd);
    ASSERT_EQ(tempFDTimer.getName(), "TestTimer");
}

TEST_F(TestFDTimerTest, moveOperator)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    *fdTimer = lu::platform::FDTimer<lu::platform::ITimerCallback>(mockTimerCallback, "NewTimer");
    int flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
    int tempFD = fdTimer->getFD();
    ASSERT_NE(tempFD, fd);
    ASSERT_EQ(fdTimer->getName(), "NewTimer");
}


TEST_F(TestFDTimerTest, checkTimerSec)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    fdTimer->setToNonBlocking();
    fdTimer->start(3, 0);
    int64_t res;
    auto ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
    sleep(1);
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
    sleep(2);
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 8);
}

TEST_F(TestFDTimerTest, checkTimerNotRepeat)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    fdTimer->setToNonBlocking();
    fdTimer->start(1, 0, false);
    sleep(1);
    int64_t res;
    auto ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 8);
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
}

TEST_F(TestFDTimerTest, checkTimerRepeat)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    fdTimer->setToNonBlocking();
    fdTimer->start(1, 0);
    sleep(1);
    int64_t res;
    auto ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 8);
    sleep(1);
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 8);
}


TEST_F(TestFDTimerTest, checkTimerNanoSec)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    fdTimer->setToNonBlocking();
    fdTimer->start(0, 10'000'000);
    int64_t res;
    auto ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
    std::this_thread::sleep_for(std::chrono::nanoseconds(800'000));
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
    std::this_thread::sleep_for(std::chrono::nanoseconds(9'200'000));
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 8);
}

TEST_F(TestFDTimerTest, checkStop)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    fdTimer->setToNonBlocking();
    fdTimer->start(1, 0);
    sleep(1);
    int64_t res;
    auto ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 8);
    ASSERT_EQ(fdTimer->stop(), true);
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
    sleep(1);
    ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, -1);
}
