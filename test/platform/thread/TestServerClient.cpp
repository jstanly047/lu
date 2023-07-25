#include <platform/thread/ServerClientThread.h>
#include <platform/thread/ServerThread.h>
#include <platform/socket/data_handler/String.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <thread>
#include <condition_variable>

#include<mock/platform/thread/MockServerClientThreadCallback.h>
#include<mock/platform/thread/MockServerThreadCallback.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace lu::platform::thread;


class TestServerClient : public ::testing::Test
{
public:
    TestServerClient() :
                    serverThread("TestServer", mockServerThreadCallback, "10000")
    {

    }
protected:
    void SetUp() override 
    {
        
    }

    void TearDown() override 
    {

    }

    void accept()
    {

    }

    lu::mock::platform::thread::MockServerThreadCallback mockServerThreadCallback;
    lu::platform::thread::ServerThread<IServerThreadCallback, IServerClientThreadCallback, lu::platform::socket::data_handler::String> serverThread;
};

