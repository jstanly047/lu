#include <thread/WorkerThread.h>
#include <thread/AccumulatorThread.h>

using namespace lu::thread;

void WorkerThreadCallback::sendMsgToOutput(void* msg)
{
    assert(m_workerThread);
    m_workerThread->sendMsgToOutput(msg);
}

WorkerThread::WorkerThread(WorkerThreadCallback& workerThreadCallback):m_workerThreadCallback(workerThreadCallback)
{
    m_workerThreadCallback.m_workerThread = this;
}

void WorkerThread::linkOutput(WorkerThread& workerThread)
{
    workerThread.setInputQueue(&m_outputMsgQueue);
    m_outputThread = &workerThread;
}

void WorkerThread::linkAccumulator(AccumulatorThread& accumulatorThread)
{
    accumulatorThread.addInputQueue(&m_outputMsgQueue);
    m_accumulatorThread = &accumulatorThread;
}

void WorkerThread::start()
{
    assert(m_inputMsgQueue);
    assert( (m_outputThread == nullptr && m_accumulatorThread != nullptr) ||
            (m_outputThread != nullptr && m_accumulatorThread == nullptr) ||
            (m_outputThread == nullptr && m_accumulatorThread == nullptr));

    if (m_outputThread)
    {
        m_outputThread->start();
    }
    else if (m_accumulatorThread)
    {
        m_accumulatorThread->start();
    }

    m_thread = std::thread(&WorkerThread::run, this);
}

void WorkerThread::run()
{    for (;;)
    {
        m_workerThreadCallback.onMsg(m_inputMsgQueue->pop());
    }
}

void WorkerThread::sendMsgToOutput(void* msg)
{
    m_outputMsgQueue.push_back(msg);
}