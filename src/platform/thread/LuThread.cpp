#include <platform/thread/LuThread.h>
#include <glog/logging.h>
#include <iostream>

using namespace  lu::platform::thread;

thread_local std::string LuThread::m_sThreadLocalName="None";


LuThread::LuThread(const std::string &name) : m_name(name)
{
}

void LuThread::init()
{
}

void LuThread::run()
{
    m_sThreadLocalName = m_name;
    m_channelID = m_outputChannel.getChannelID();
    LOG(INFO) << "[" << m_name << "] Started channelID:" << m_channelID;
}

void LuThread::stop()
{
    LOG(INFO) << "[" << m_name << "] Stopping ";
}

void LuThread::join()
{
    LOG(INFO) << "[" << m_name << "] Join";
    m_thread.join(); 
}

void LuThread::connect(LuThread &readerThread)
{
    m_outputChannel.add(readerThread.getName(), readerThread.m_inputChannel.getTransferQueue());
}

void LuThread::transferMsg(const std::string &threadName, void *msg)
{
    m_outputChannel.transferMsg(threadName, msg);
}

void LuThread::transferMsg(unsigned int threadIndex, void *msg)
{
    m_outputChannel.transferMsg(threadIndex, msg);
}

unsigned int LuThread::getThreadIndex(const std::string& threadName) const
{
    return m_outputChannel.getThreadIndx(threadName);
}