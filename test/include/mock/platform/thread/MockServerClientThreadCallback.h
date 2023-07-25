#pragma once

#include <platform/thread/ServerClientThread.h>
#include <gmock/gmock.h>

namespace lu::mock::platform::thread
{
    class MockServerClientThreadCallback : public lu::platform::thread::IServerClientThreadCallback
    {
    public:
        MOCK_METHOD(bool, onInit, (), (override));
        MOCK_METHOD(void, onStart, (), (override));
        MOCK_METHOD(void, onExit, (), (override));
        MOCK_METHOD(void, onNewConnection, ((const lu::platform::socket::DataSocket<lu::platform::thread::IServerClientThreadCallback, lu::platform::socket::IDataHandler>* baseSocket)), (override));
        MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<ITimerCallback>&), (override));
        MOCK_METHOD(void, onClientClose, ((const lu::platform::socket::DataSocket<lu::platform::socket::IDataSocketCallback, lu::platform::socket::IDataHandler>&)), (override));
        MOCK_METHOD(void, onMessage, ((const lu::platform::socket::DataSocket<lu::platform::socket::IDataSocketCallback, lu::platform::socket::IDataHandler>&), void* ), (override));
    };
}