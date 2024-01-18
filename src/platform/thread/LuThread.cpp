#include <platform/thread/LuThread.h>
#include <glog/logging.h>

using namespace  lu::platform::thread;

namespace
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local std::string m_sThreadLocalName="None";
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local LuThread* m_sCurrentLuThread = nullptr;
}


LuThread::LuThread(const std::string &name) : m_name(name)
{
}

void LuThread::init()
{
}

void LuThread::start()
{
    this->m_thread = std::thread(&LuThread::threadRun, this);
}

void LuThread::threadRun()
{
    m_sThreadLocalName = m_name;
    m_sCurrentLuThread = this;
    #ifdef __linux__
        ::pthread_setname_np(::pthread_self(), m_name.c_str());
    #endif
    m_channelID = channel::OutputChannel::getChannelID();
    LOG(INFO) << "[" << m_name << "] Started channelID:" << m_channelID;
    run();
}

const std::string &LuThread::getCurrentThreadName()
{
    return m_sThreadLocalName;
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

void LuThread::connectTo(channel::ChannelID channelID, lu::platform::EventNotifier* eventNotifier)
{
    m_eventNotifiers.insert({channelID, std::unique_ptr<lu::platform::EventNotifier>(eventNotifier)});
}

void LuThread::transferMsg(const std::string &threadName, void *msg)
{
    m_sCurrentLuThread->m_outputChannel.transferMsg(threadName, msg);
}

void LuThread::transferMsg(unsigned int threadIndex, void *msg)
{
    m_sCurrentLuThread->m_outputChannel.transferMsg(threadIndex, msg);
}

void LuThread::thisTransferMsg(unsigned int threadIndex, void *msg)
{
    m_outputChannel.transferMsg(threadIndex, msg);
}

void LuThread::transferMsgToIOThread(channel::ChannelID channelID, void *msg)
{
    m_sCurrentLuThread->m_eventNotifiers[channelID]->notify(lu::platform::EventData(lu::platform::EventData::AppMessage, m_sCurrentLuThread->m_channelID, msg));
}

 channel::ChannelID LuThread::getCurrentThreadChannelID()
 {
    return m_sCurrentLuThread->m_channelID;
 }

unsigned int LuThread::getThreadIndex(const std::string& threadName) 
{
    return m_sCurrentLuThread->m_outputChannel.getThreadIndx(threadName);
}