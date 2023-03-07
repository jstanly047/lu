#pragma once

#include <thread>
#include <string>
#include <queue/SPSCQueue.h>

namespace lu::thread{
    class WorkerThread;

    class ServerThread
    {
    public:
        ServerThread(const ServerThread& ) = delete;
        ServerThread& operator=(const ServerThread& ) = delete;
        ServerThread() = default;
        void start(const std::string& service);
        std::thread startNewThread(const std::string& service);
        void linkOutput(WorkerThread& workerThread);


    private:
        lu::queue::SPSCQueue<1024> m_outputMsgQueue;
        lu::thread::WorkerThread* m_outputThread = nullptr;
    };
}