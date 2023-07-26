#pragma once

#include <platform/socket/data_handler/String.h>
#include <platform/socket/IDataSocketCallback.h>
#include <gmock/gmock.h>

namespace lu::platform::socket
{
    class MockDataSocketCallback : public IDataSocketCallback<data_handler::String>
    {
    public:
        MockDataSocketCallback() {}
        MockDataSocketCallback(MockDataSocketCallback&&) {}
        MockDataSocketCallback& operator=(MockDataSocketCallback&&) { return *this; }

        MOCK_METHOD(void, onClientClose, ((DataSocket<MockDataSocketCallback, data_handler::String>&)));
        MOCK_METHOD(void, onData, ( void*));
    };
}