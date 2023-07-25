#include <platform/FDEventLoop.h>
#include <platform/FDTimer.h>
#include <platform/ITimerCallback.h>

#include <fcntl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockTimerCallback : public lu::platform::ITimerCallback
{
public:
    MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<lu::platform::ITimerCallback>&), (override));
};


TEST(TestEventLoop, testWithTimer)
{
    lu::platform::FDEventLoop eventLoop;
    ASSERT_EQ(eventLoop.init(), true);
    int fd = eventLoop.getFD();
    ASSERT_NE(fd, lu::platform::NULL_FD);

    MockTimerCallback timerCallback1;
    lu::platform::FDTimer<lu::platform::ITimerCallback> timer1(timerCallback1, "timer1");
    timer1.init();
    timer1.setToNonBlocking();

    MockTimerCallback timerCallback2;
    lu::platform::FDTimer<lu::platform::ITimerCallback> timer2(timerCallback2, "timer2");
    timer2.init();
    timer2.setToNonBlocking();
    
    ASSERT_EQ(eventLoop.add(timer1), true);
    ASSERT_EQ(eventLoop.add(timer2), true);

    int timer2CallCount = 0;

    EXPECT_CALL(timerCallback1, onTimer(::testing::Ref(timer1))).Times(1);
    EXPECT_CALL(timerCallback2, onTimer(::testing::Ref(timer2))).WillRepeatedly(::testing::Invoke([&](const lu::platform::FDTimer<lu::platform::ITimerCallback>& timer){
        timer2CallCount++;

        if (timer2CallCount == 2)
        {
            eventLoop.stop();
            timer2.stop();
        }

        ASSERT_EQ(timer.getFD(), timer2.getFD());
    }));

    timer1.start(0, 5000'000, false);
    timer2.start(0, 5000'000);
    eventLoop.start(4);
    ASSERT_EQ(timer2CallCount, 2);
    int flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, lu::platform::NULL_FD);
}
