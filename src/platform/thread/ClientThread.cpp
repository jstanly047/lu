#include <platform/thread/ClientThread.h>
#include <platform/socket/IDataHandler.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ClientThread<ClientThreadCallback, DataHandler>::ClientThread(ClientThreadCallback& severClientThreadCallback,
    const std::string& name, ClientThreadConfig serverConfig)
    :
    m_name(name),
    m_severClientThreadCallback(severClientThreadCallback),
    m_clientThreadConfig(serverConfig),
    m_eventLoop(),
    m_timer(m_severClientThreadCallback, serverConfig.TIMER_NAME),
    m_thread()
{

}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ClientThread<ClientThreadCallback, DataHandler>::ClientThread(ClientThread&& other):
    m_name(std::move(other.m_name)),
    m_severClientThreadCallback(other.m_severClientThreadCallback),
    m_clientThreadConfig(std::move(other.m_clientThreadConfig)),
    m_eventLoop(std::move(other.m_eventLoop)),
    m_timer(std::move(other.m_timer)),
    m_eventChannel(std::move(other.m_eventChannel)),
    m_thread(std::move(other.m_thread))
    
{

}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ClientThread<ClientThreadCallback, DataHandler>& ClientThread<ClientThreadCallback, DataHandler>::operator=(ClientThread<ClientThreadCallback, DataHandler>&& other)
{
    m_name = std::move(other.m_name);
    m_severClientThreadCallback = std::move(other.m_severClientThreadCallback);
    m_clientThreadConfig = other.m_clientThreadConfig;
    m_eventLoop = std::move(other.m_eventLoop);
    m_timer = std::move(other.m_timer);
    m_thread = std::move(other.m_thread);
    return *this;
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
bool ClientThread<ClientThreadCallback, DataHandler>::init()
{
    if (m_eventChannel.init() == false)
    {
        LOG(ERROR) << "Thead[" << m_name << "] failed to create event channel!";
        return false;
    }

    if (m_eventLoop.init() == false)
    {
        LOG(ERROR) << "Thead[" << m_name << "] failed init FD event loop!";
        return false;
    }

    if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
    {
        if (m_timer.init() == false)
        {
            LOG(ERROR) << "Thread[" << m_name << "] failed to create timer!";
            return false;
        }
    }

    return true;
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::start()
{
    std::thread(&ClientThread::run, this);
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::run()
{
    LOG(INFO) << "Started " << m_name;
    m_severClientThreadCallback.onStart();
    m_eventLoop.add(m_eventChannel);

    if (m_clientThreadConfig.TIMER_IN_MSEC != 0u)
    {
        m_timer.setToNonBlocking();
        m_eventLoop.add(m_timer);
        m_timer.start(0, (int) m_clientThreadConfig.TIMER_IN_MSEC * 1'000'1000);
    }

    m_eventLoop.start(m_clientThreadConfig.NUMBER_OF_EVENTS_PER_HANDLE);
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::stop()
{
    LOG(INFO) << "Stopping " << m_name;

    if (m_clientThreadConfig.TIMER_IN_MSEC == 0u)
    {
        m_timer.init();
        m_timer.setToNonBlocking();
        m_eventLoop.add(m_timer);
        m_timer.start(0, 1'000'1000u, false);
    }

    m_eventLoop.stop(); //Expect to stop in next timer
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::join()
{
    m_thread.join();
    m_severClientThreadCallback.onExit();
}

template class ClientThread<IClientThreadCallback, lu::platform::socket::data_handler::String>;