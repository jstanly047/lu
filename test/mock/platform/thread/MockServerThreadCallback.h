#pragma once
#include <platform/thread/ServerThread.h>
#include <platform/thread/IServerThreadCallback.h>
#include <platform/socket/data_handler/String.h>
#include <platform/thread/MockServerThreadCallback.h>
#include <platform/thread/MockServerClientThreadCallback.h>
#include <platform/FDTimer.h>
#include <gmock/gmock.h>

namespace lu::platform::thread
{
    class MockServerThreadCallback : public IServerThreadCallback
    {
    public:
        MockServerThreadCallback() {}
        MockServerThreadCallback(MockServerThreadCallback&&) {}
        MockServerThreadCallback& operator=(MockServerThreadCallback&&) { return *this; }

        MOCK_METHOD(bool, onInit, (), (override));
        MOCK_METHOD(void, onStart, (), (override));
        //MOCK_METHOD(void, onStartComplete, ());
        MOCK_METHOD(void, onExit, (), (override));
        MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket& baseSocket), (override));
        MOCK_METHOD(void, onNewConnection, (lu::platform::socket::SSLSocket& baseSocket), (override));
        MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<IServerThreadCallback>&), (override));
    };
}
