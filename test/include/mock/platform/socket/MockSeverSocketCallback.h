#pragma once

#include <platform/socket/IServerSocketCallback.h>
#include <gmock/gmock.h>

namespace lu::mock::platform::socket
{
    class MockSeverSocketCallback : public lu::platform::socket::IServerSocketCallback
    {
        MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket*), (override));
    };
}