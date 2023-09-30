#include <platform/thread/LuThread.h>
#include <glog/logging.h>
#include <iostream>

using namespace  lu::platform::thread;

thread_local std::string lu::platform::thread::gtlThreadName="None";


LuThread::LuThread(const std::string &name) : m_name(name)
{
}

void LuThread::init()
{
}

void LuThread::run()
{
    gtlThreadName = m_name;
    m_channelID = m_outputChannel.getChannelID();
    LOG(INFO) << "Started " << m_name << ", channelID:" << m_channelID;
}

void LuThread::stop()
{
    LOG(INFO) << "Stopping " << m_name;
}

void LuThread::join()
{
    LOG(INFO) << "Join " << m_name;
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