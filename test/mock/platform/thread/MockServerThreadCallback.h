#pragma once

#include <platform/thread/ServerThread.h>
#include <platform/socket/data_handler/String.h>
#include <platform/thread/MockServerThreadCallback.h>
#include <platform/thread/MockServerClientThreadCallback.h>
#include <platform/FDTimer.h>
#include <gmock/gmock.h>

namespace lu::platform::thread
{
    class MockServerThreadCallback
    {
    public:
        MockServerThreadCallback() {}
        MockServerThreadCallback(MockServerThreadCallback&&) {}
        MockServerThreadCallback& operator=(MockServerThreadCallback&&) { return *this; }
        
        MOCK_METHOD(void, onStart, ());
        MOCK_METHOD(void, onStartComplete, ());
        MOCK_METHOD(void, onExit, ());
        MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket* baseSocket));
        MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<MockServerThreadCallback>&));
        MOCK_METHOD(void, onClientClose, ((lu::platform::socket::DataSocket<MockServerThreadCallback, lu::platform::socket::data_handler::String>&)));
        MOCK_METHOD(void, onMessage, ( void* ));
    };
}
