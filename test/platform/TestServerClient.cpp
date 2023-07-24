#include <platform/socket/ServerSocket.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IServerSocketCallback.h>
#include <platform/socket/IDataSocketCallback.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

using namespace lu::platform::socket;

class MockDataHandler :  public IDataHandler
{
public:
    MOCK_METHOD(uint8_t*, getReceiveBufferToFill, (),(override));
    MOCK_METHOD(std::size_t, getReceiveBufferSize, (), (override));
    MOCK_METHOD(std::size_t, getHeaderSize, (), (override));
    MOCK_METHOD(std::size_t, readHeader, (std::size_t), (override));
    MOCK_METHOD(void, readMessage, (std::size_t, std::size_t), (override));
};

class MockSeverSocketCallback : public IServerSocketCallback
{
    MOCK_METHOD(void, onNewConnection, (BaseSocket*), (override));
};


class TestServerClient : public ::testing::Test
{
public:
    TestServerClient() :  connectSocket("localhost", "10000"),
                    serverSocket("10000", mockConnectionHandler, true)
    {

    }
protected:
    void SetUp() override 
    {
        serverSocket.setUpTCP(4);
        serverThead = std::thread(&TestServerClient::accept, this);

        {
            std::unique_lock<std::mutex> lock(startMutex);
            startCondition.wait(lock, [&] { return threadStarted; });
        }

        connectSocket.connectToTCP(dataCallback);
    }

    void TearDown() override 
    {
        delete dataSocket;
        dataSocket = nullptr;
        serverThead.join();
    }

    void accept()
    {
        {
            std::lock_guard<std::mutex> lock(startMutex);
            threadStarted = true;
            startCondition.notify_one();
        }
       
        dataSocket = serverSocket.acceptDataSocket();
    }

    MockSeverSocketCallback mockConnectionHandler;
    IDataSocketCallback<IDataHandler> dataCallback;
    ConnectSocket<IDataSocketCallback<IDataHandler>, IDataHandler> connectSocket;
    ServerSocket<IServerSocketCallback> serverSocket;
    std::thread serverThead;
    BaseSocket* dataSocket = nullptr;
    std::mutex startMutex;
    std::condition_variable startCondition;
    bool threadStarted = false;
};
