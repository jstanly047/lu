#include <platform/socket/ServerSocket.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IConnectionHandler.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Define a mock DataHandler class


class MockDataHandler : public lu::platform::socket::IConnectionHandler, public lu::platform::socket::IDataHandler
{
public:
    MOCK_METHOD(uint8_t*, getReceiveBufferToFill, (),(override));
    MOCK_METHOD(std::size_t, getReceiveBufferSize, (), (override));
    MOCK_METHOD(std::size_t, getHeaderSize, (), (override));
    MOCK_METHOD(std::size_t, readHeader, (std::size_t), (override));
    MOCK_METHOD(void, readMessage, (std::size_t, std::size_t), (override));
    MOCK_METHOD(void, onClose, (lu::platform::socket::DataSocket<lu::platform::socket::IDataHandler>*), (override));
    MOCK_METHOD(void, onNewConnection, (lu::platform::socket::BaseSocket*), (override));
};

// Test ServerSocket
TEST(ServerSocketTest, SetUpTCP) {
    MockDataHandler mockDataHandler;

    // Create a ServerSocket instance
    lu::platform::socket::ServerSocket<lu::platform::socket::IConnectionHandler> serverSocket("8080", mockDataHandler, 1);

    // Set up TCP and check the result
    bool result = serverSocket.setUpTCP(10);
    EXPECT_TRUE(result);
}

TEST(ServerSocketTest, AcceptDataSocket) {
    MockDataHandler mockDataHandler;

    // Create a ServerSocket instance
    lu::platform::socket::ServerSocket<lu::platform::socket::IConnectionHandler> serverSocket("8080", mockDataHandler, true);

    // Accept a DataSocket and check the result
    lu::platform::socket::DataSocket<lu::platform::socket::IDataHandler> dataSocket(std::move(*serverSocket.acceptDataSocket()), mockDataHandler);

}

// Test ConnectSocket
TEST(ConnectSocketTest, ConnectToTCP) {
    MockDataHandler mockDataHandler;

    // Create a ConnectSocket instance
    lu::platform::socket::ConnectSocket<lu::platform::socket::IDataHandler> connectSocket("localhost", "8080");

    // Connect to TCP and check the result
    bool result = connectSocket.connectToTCP(mockDataHandler);
    EXPECT_TRUE(result);
}