#include <platform/FDTimer.h>
#include <platform/ITimerHandler.h>

#include <fcntl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


class MockTimerCallback : public lu::platform::ITimerHandler
{
public:
    MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<lu::platform::ITimerHandler>&), (override));
};

class TestFDTimerTest : public testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create the timer and set up the interval for testing
        fdTimer = new lu::platform::FDTimer<lu::platform::ITimerHandler>(mockTimerCallback);
        ASSERT_NE(fdTimer->getFD(), nullptr);
        fd = fdTimer->getFD();
    }

    void TearDown() override 
    {
        delete fdTimer;
        int flags = ::fcntl(fd, F_GETFL);
        ASSERT_EQ(flags, lu::platform::NULL_FD);
    }

    MockTimerCallback mockTimerCallback;
    lu::platform::FDTimer<lu::platform::ITimerHandler>* fdTimer = nullptr;
    int fd = lu::platform::NULL_FD;
};

TEST_F(TestFDTimerTest, createFDTimer)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
}

TEST_F(TestFDTimerTest, moveConstruct)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    lu::platform::FDTimer<lu::platform::ITimerHandler> tempFDTimer(std::move(*fdTimer));
    int flags = ::fcntl(fd, F_GETFL);
    ASSERT_NE(flags, lu::platform::NULL_FD);
    ASSERT_EQ(fdTimer->getFD(), nullptr);
    int tempFD = tempFDTimer.getFD();
    ASSERT_EQ(tempFD, fd);
}

TEST_F(TestFDTimerTest, moveOperator)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    *fdTimer = lu::platform::FDTimer<lu::platform::ITimerHandler>(mockTimerCallback);
    int flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
    int tempFD = fdTimer->getFD();
    ASSERT_NE(tempFD, fd);
}

TEST_F(TestFDTimerTest, checkTimerSec)
{
    EXPECT_CALL(mockTimerCallback, onTimer(::testing::Ref(*fdTimer))).Times(0);
    fdTimer->start(1, 0);
    int64_t res;
    auto ret = ::read(fdTimer->getFD(), &res, sizeof(res));
    ASSERT_EQ(ret, 0);
}