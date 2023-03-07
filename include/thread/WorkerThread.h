#pragma once

#include <queue/SPSCQueue.h>
#include <thread>

namespace lu::thread
{
    class WorkerThread;
    class AccumulatorThread;

    class WorkerThreadCallback
    {
    public:
        virtual void onMsg(void* msg) = 0;

    protected:
        void sendMsgToOutput(void* msg);

    private:
        WorkerThread* m_workerThread;

        friend class WorkerThread;
    };

    class WorkerThread
    {
    public:
        WorkerThread(WorkerThreadCallback& workerThreadCallback);
        WorkerThread(const WorkerThread& ) = delete;
        WorkerThread& operator=(const WorkerThread& ) = delete;

        void setInputQueue(lu::queue::SPSCQueue<1024> *inputMsgQueue) { m_inputMsgQueue = inputMsgQueue; }
        void linkOutput(WorkerThread& workerThread);
        void linkAccumulator(AccumulatorThread& accumulatorThread);
        void start();
        void join() { m_thread.join(); }

    private:
        void sendMsgToOutput(void* msg);
        void run();

        lu::queue::SPSCQueue<1024> *m_inputMsgQueue = nullptr;
        lu::thread::WorkerThread* m_outputThread = nullptr;
        lu::thread::AccumulatorThread* m_accumulatorThread = nullptr;

        lu::queue::SPSCQueue<1024> m_outputMsgQueue;
        WorkerThreadCallback& m_workerThreadCallback;
        std::thread m_thread;

        friend class WorkerThreadCallback;
    };
}