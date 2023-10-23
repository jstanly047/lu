#include <platform/EventChannel.h>
#include <platform/thread/EventThread.h>
#include <utils/WaitForCount.h>

#include <fcntl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::platform;


class MockEventHandler
{
public:
    MOCK_METHOD(void, onNewConnection, (socket::BaseSocket *));
};

class MockThreadEventHandler
{
public:
    MOCK_METHOD(bool, onInit,());
    MOCK_METHOD(void, onStart,());
    MOCK_METHOD(void, onExit,());
    MOCK_METHOD(void, onTimer,(FDTimer<MockThreadEventHandler>&));
};


TEST(TestEventChannel, FileDescriptorNull)
{
    MockEventHandler eventHandler;
    EventChannel<MockEventHandler> eventChannel(eventHandler, "TestEventHandler");
    IFDEventHandler& ifdEventHandler = eventChannel;
    EXPECT_EQ((int)ifdEventHandler.getFD(), NULL_FD);
}

TEST(TestEventChannel, init)
{
    MockEventHandler eventHandler;
    EventChannel<MockEventHandler> eventChannel(eventHandler, "TestEventHandler");
    IFDEventHandler& ifdEventHandler = eventChannel;
    eventChannel.init();
    EXPECT_NE((int)ifdEventHandler.getFD(), NULL_FD);
}

TEST(TestEventChannel, destructor)
{
    MockEventHandler eventHandler;
    auto eventChannel = new EventChannel<MockEventHandler>(eventHandler, "TestEventHandler");
    IFDEventHandler* ifdEventHandler = eventChannel;
    eventChannel->init();
    EXPECT_NE((int)ifdEventHandler->getFD(), NULL_FD);
    int fd = ifdEventHandler->getFD();
    delete eventChannel;
    int flags = ::fcntl(fd, F_GETFL);
    ASSERT_EQ(flags, NULL_FD);
}

TEST(TestEventChannel, checkCallback)
{
    MockEventHandler eventHandler;
    MockThreadEventHandler threadEventHandler;
    EventChannel<MockEventHandler> eventChannel(eventHandler, "TestEventHandler");
    lu::utils::WaitForCount waitFor(1U);
    
    thread::EventThreadConfig eventThreadConfig;
    eventThreadConfig.TIMER_IN_MSEC = 0U;
    thread::EventThread<MockThreadEventHandler> thread(threadEventHandler,"EventHandlerThread", eventThreadConfig);

    EXPECT_CALL(threadEventHandler, onInit()).Times(1).WillRepeatedly(::testing::Invoke(
        [&]()->bool
        {
            return eventChannel.init();
        }
    ));

    EXPECT_CALL(threadEventHandler, onStart()).Times(1).WillRepeatedly(::testing::Invoke(
        [&]()
        {
            thread.addToEventLoop(eventChannel);
            waitFor.increment();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));  
        }
    ));

    EXPECT_CALL(threadEventHandler, onTimer(::testing::_)).Times(0);
    EXPECT_CALL(threadEventHandler, onExit()).Times(1);
    EXPECT_CALL(eventHandler, onNewConnection(::testing::_)).Times(1000).WillRepeatedly(::testing::Invoke(
        [&](socket::BaseSocket * baseSocket)
        {
            delete baseSocket;
            static unsigned count = 0;
            count++;
            if (count == 1000U)
            {
                thread.stop();
            }
        }
    ));

    thread.init();
    thread.start();

    waitFor.wait();

    for (unsigned int i = 1U; i <= 196; i++)
    {
        if (i % 2 == 0)
        {
            eventChannel.notify(EventData(EventData::NewConnection, new socket::BaseSocket()));
        }
        else
        {
            eventChannel.notify(EventData());
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    for (unsigned int i = 1U; i <= 1804; i++)
    {
        if (i % 2 == 0)
        {
            eventChannel.notify(EventData(EventData::NewConnection, new socket::BaseSocket()));
        }
        else
        {
            eventChannel.notify(EventData());
        }
    }


    thread.join();
}