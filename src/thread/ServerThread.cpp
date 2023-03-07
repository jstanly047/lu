#include <thread/ServerThread.h>
#include <socket/ServerSocket.h>
#include <socket/DataSocket.h>
#include <thread/WorkerThread.h>
#include <utils/Utils.h>
#include <thread>
#include <array>
#include <sys/epoll.h>

using namespace lu::thread;

constexpr int MAX_EVENT = 10;

void ServerThread::start(const std::string& service)
{
    if (m_outputThread)
    {
        m_outputThread->start();
    }

    int epollFd = epoll_create1(0);

    if (epollFd < 0) 
    {
        lu::utils::Utils::DieWithUserMessage(__func__, "Error in open an epoll file descriptor");
    }

    lu::socket::ServerSocket serverSocket(service);
    serverSocket.setNonBlocking();
    serverSocket.setReuseAddAndPort(true);

    if (serverSocket.setUpTCP() == false)
    {
        lu::utils::Utils::DieWithUserMessage(__func__, "SetupTCPServerSocket() failed");
    }

    if (serverSocket.registerEpoll(epollFd) == false) 
    {
        lu::utils::Utils::DieWithUserMessage(__func__, "Failed to add listen socket to epoll");
    }

    std::array<struct epoll_event, MAX_EVENT> events;

    for(;;) 
    {
        int n = epoll_wait(epollFd, events.data(), MAX_EVENT, -1);

        for (int i = 0; i < n; ++i) 
        {
            if (events[i].data.ptr == &serverSocket) 
            {
                if ((events[i].events & EPOLLERR) ||
                    !(events[i].events & EPOLLIN))
                {
                    continue;
                }

                auto dataSocket = serverSocket.acceptDataSocket();
                dataSocket->registerEpoll(epollFd);
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

                for (;;)
                {
                    auto endCodeOrder = dataSocket->getNextMessage();

                    if (endCodeOrder.first == nullptr)
                    {
                        break;
                    }

                    m_outputMsgQueue.push_back(endCodeOrder.first);
                }
            }
        }
    }
}

std::thread ServerThread::startNewThread(const std::string& service)
{
    return std::thread(&ServerThread::start, this, service);
}

void ServerThread::linkOutput(WorkerThread& workerThread)
{
    workerThread.setInputQueue(&m_outputMsgQueue);
    m_outputThread = &workerThread;
}
