#include <thread/AccumulatorThread.h>
#include <socket/ServerSocket.h>
#include <socket/DataSocket.h>
#include <utils/Utils.h>
#include <array>
#include <sys/epoll.h>
#include <csignal>

using namespace lu::thread;

constexpr int MAX_EVENT = 2;
std::vector<lu::queue::SPSCQueue<1024> *> AccumulatorThread::m_inputMsgQueues;
int AccumulatorThread::m_epollFd = 0;
lu::socket::ServerSocket* AccumulatorThread::m_serverSocket = nullptr;

AccumulatorThread::AccumulatorThread(const std::string& service): m_service(service)
{

}

void AccumulatorThread::start()
{
    if (m_isStarted)
    {
        return;
    }

    m_isStarted = true;
    m_thread = std::thread(&AccumulatorThread::run, this);
}

void AccumulatorThread::run()
{
    signal(SIGPIPE, AccumulatorThread::handleConnectionBreak);

    m_epollFd = epoll_create1(0);

    if (m_epollFd < 0) 
    {
        lu::utils::Utils::DieWithUserMessage(__func__, "Error in open an epoll file descriptor");
    }

    m_serverSocket = new lu::socket::ServerSocket(m_service);
    m_serverSocket->setNonBlocking();

    if (m_serverSocket->setUpTCP() == false)
    {
        lu::utils::Utils::DieWithUserMessage(__func__, "SetupTCPServerSocket() failed");
    }

    if (m_serverSocket->registerEpoll(m_epollFd) == false) 
    {
        lu::utils::Utils::DieWithUserMessage(__func__, "Failed to add listen socket to epoll");
    }

    handleEpollEvent();
}

void AccumulatorThread::handleEpollEvent()
{
    std::array<struct epoll_event, MAX_EVENT> events;

    while (true) 
    {
        int n = epoll_wait(m_epollFd, events.data(), MAX_EVENT, -1);

        for (int i = 0; i < n; ++i) 
        {
            if (events[i].data.ptr == m_serverSocket) 
            {
                if ((events[0].events & EPOLLERR) ||
                    !(events[0].events & EPOLLIN))
                {
                    //?? Need to check further
                    continue;
                }

                auto dataSocket = m_serverSocket->acceptDataSocket();
                dataSocket->setTCPNoDelay();
                sendToClient(*dataSocket);
            }
            else
            {
                auto dataSocket = reinterpret_cast<lu::socket::DataSocket*>(events[i].data.ptr);
                if ((events[i].events & EPOLLHUP ||
                    events[i].events & EPOLLERR) ||
                    !(events[i].events & EPOLLIN))
                {
                    delete dataSocket;
                    continue;
                }
            }
        }
    }
}

void AccumulatorThread::handleConnectionBreak(int signal)
{
    handleEpollEvent();
}

void AccumulatorThread::sendToClient(lu::socket::DataSocket& dataSocket)
{
    // TODO: now only one upstream thread available , need to change for multiple upstream
}


