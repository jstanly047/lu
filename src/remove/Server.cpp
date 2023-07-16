#include <socket/Server.h>
#include <socket/ServerSocket.h>
#include <socket/DataSocket.h>
#include<utils/TimerClock.h>

#include <glog/logging.h>

#include <array>



using namespace lu::socket;

template<typename DataHandler, typename ServerCallback>
bool Server<DataHandler, ServerCallback>::init(int timerInMSec)
{
    
}

template<typename DataHandler, typename ServerCallback>
bool Server<DataHandler, ServerCallback>::start(const std::string& service, int maxEvents)
{
    assert(m_epollFD > 0);
    lu::socket::ServerSocket<DataHandler> serverSocket(service);
    serverSocket.setNonBlocking();
    serverSocket.setReuseAddAndPort(true);
}