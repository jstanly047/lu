#pragma once
#include <queue/SPSCQueue.h>
#include <thread>
#include <vector>
#include <mutex>

namespace lu::socket
{
    class DataSocket;
    class ServerSocket;
}

namespace lu::thread
{
    class AccumulatorThread
    {
    public:
        AccumulatorThread(const AccumulatorThread& ) = delete;
        AccumulatorThread& operator=(const AccumulatorThread& ) = delete;

        AccumulatorThread(const std::string& service);
        void addInputQueue(lu::queue::SPSCQueue<1024> *inputMsgQueue) { m_inputMsgQueues.push_back(inputMsgQueue); }
        void start();
        void join() { m_thread.join(); }
        static void handleConnectionBreak();

    private:
        void run();
        static void handleEpollEvent();
        static void sendToClient(lu::socket::DataSocket& dataSocket);

        bool m_isStarted = false;
        std::string m_service;
        static std::vector<lu::queue::SPSCQueue<1024> *> m_inputMsgQueues;
        std::thread m_thread;
        static int m_epollFd;
        static lu::socket::ServerSocket* m_serverSocket;
    };
}