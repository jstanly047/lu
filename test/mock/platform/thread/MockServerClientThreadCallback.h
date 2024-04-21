#pragma once
#include <platform/socket/data_handler/String.h>
#include <platform/thread/IClientThreadCallback.h>
#include <platform/EventChannel.h>
#include <gmock/gmock.h>

namespace lu::platform::thread
{
    class MockServerClientThreadCallback : public IClientThreadCallback
    {
    public:
        MockServerClientThreadCallback() {}
        MockServerClientThreadCallback(MockServerClientThreadCallback&&) {}
        MockServerClientThreadCallback& operator=(MockServerClientThreadCallback&&) { return *this; }
        
        MOCK_METHOD(bool, onInit, (), (override));
        MOCK_METHOD(void, onStart, (), (override));
        //MOCK_METHOD(void, onStartComplete, (), (override));
        MOCK_METHOD(void, onExit, (), (override));
        MOCK_METHOD(void, onNewConnection, ((lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&)), (override));
        MOCK_METHOD(void, onNewConnection, ((lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String, lu::platform::socket::SSLSocket>&)), (override));
        MOCK_METHOD(void, onAppMsg, (void*, lu::platform::thread::channel::ChannelID), (override));
        MOCK_METHOD(void, onTimer, (const lu::platform::FDTimer<IClientThreadCallback>&), (override));
        MOCK_METHOD(void, onClientClose, ((lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&)), (override));
        MOCK_METHOD(void, onClientClose, ((lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String, lu::platform::socket::SSLSocket>&)), (override));
        MOCK_METHOD(void, onData, ((lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&), void* ), (override));
        MOCK_METHOD(void, onData, ((lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String, lu::platform::socket::SSLSocket>&), void* ), (override));
    };
}
