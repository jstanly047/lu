#pragma once

#include <platform/thread/ServerClientThread.h>
#include <platform/socket/data_handler/String.h>
#include <platform/EventChannel.h>
#include <gmock/gmock.h>

namespace lu::platform::thread
{
    class MockServerClientThreadCallback 
    {
    public:
        MockServerClientThreadCallback() {}
        MockServerClientThreadCallback(MockServerClientThreadCallback&&) {}
        MockServerClientThreadCallback& operator=(MockServerClientThreadCallback&&) { return *this; }
        
        MOCK_METHOD(bool, onInit, ());
        MOCK_METHOD(void, onStart, ());
        MOCK_METHOD(void, onExit, ());
        MOCK_METHOD(void, onNewConnection, ((lu::platform::socket::DataSocket<MockServerClientThreadCallback, lu::platform::socket::data_handler::String>* baseSocket)));
        MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<MockServerClientThreadCallback>&));
        MOCK_METHOD(void, onClientClose, ((lu::platform::socket::DataSocket<MockServerClientThreadCallback, lu::platform::socket::data_handler::String>&)));
        MOCK_METHOD(void, onMessage, (void* ));
    };
}
