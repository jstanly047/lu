#pragma once

#include <platform/thread/ServerThread.h>
#include <gmock/gmock.h>

namespace lu::mock::platform::thread
{
    class MockServerThreadCallback : public lu::platform::thread::IServerThreadCallback
    {
    public:
        MOCK_METHOD(void, onStart, (), (override));
        MOCK_METHOD(void, onStartComplete, (), (override));
        MOCK_METHOD(void, onExit, (), (override));
        MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket&& baseSocket), (override));
        MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<ITimerCallback>&), (override));
        MOCK_METHOD(void, onClientClose, ((const lu::platform::socket::DataSocket<lu::platform::socket::IDataSocketCallback, lu::platform::socket::IDataHandler>&)), (override));
        MOCK_METHOD(void, onMessage, ((const lu::platform::socket::DataSocket<lu::platform::socket::IDataSocketCallback, lu::platform::socket::IDataHandler>&), void* ), (override));
    };
}