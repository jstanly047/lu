#include <platform/thread/LuThread.h>
#include <glog/logging.h>

using namespace  lu::platform::thread;

thread_local std::string lu::platform::thread::gtlThreadName="None";


LuThread::LuThread(const std::string &name) : m_name(name),
                                              m_thread()
{
}

void LuThread::init()
{
}

void LuThread::run()
{
    gtlThreadName = m_name;
    LOG(INFO) << "Started " << m_name << ", channelID:" << getChannelID();
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

