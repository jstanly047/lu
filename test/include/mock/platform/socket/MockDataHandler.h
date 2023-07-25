#pragma once

#include <platform/socket/IDataHandler.h>
#include <platform/socket/IDataSocketCallback.h>
#include <gmock/gmock.h>

namespace lu::mock::platform::socket
{
    class MockDataHandler :  public lu::platform::socket::IDataHandler<lu::platform::socket::IDataSocketCallback>
    {
    public:
        MOCK_METHOD(uint8_t*, getReceiveBufferToFill, (),(override));
        MOCK_METHOD(std::size_t, getReceiveBufferSize, (), (override));
        MOCK_METHOD(std::size_t, getHeaderSize, (), (override));
        MOCK_METHOD(std::size_t, readHeader, (std::size_t), (override));
        MOCK_METHOD(void, readMessage, (std::size_t, std::size_t), (override));
    };
}