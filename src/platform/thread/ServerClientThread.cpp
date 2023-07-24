#include <platform/thread/ServerClientThread.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerClientThread<ServerClientThreadCallback, DataHandler>::ServerClientThread(const std::string& name,
    SeverClientThreadConfig serverConfig)
    :
    m_serverThreadClientConfig(serverConfig),
    m_eventLoop(),
    m_eventChannel(*this),
    m_thread(),
    m_name(name)
{

}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerClientThread<ServerClientThreadCallback, DataHandler>::ServerClientThread(ServerClientThread&& other):
    m_severClientThreadCallback(std::move(other.m_severClientThreadCallback)),
    m_serverThreadClientConfig(std::move(other.m_serverThreadClientConfig)),
    m_eventLoop(std::move(other.m_eventLoop)),
    m_eventChannel(std::move(other.m_eventChannel)),
    m_thread(std::move(other.m_thread)),
    m_name(std::move(other.m_name))
{

}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerClientThread<ServerClientThreadCallback, DataHandler>& ServerClientThread<ServerClientThreadCallback, DataHandler>::operator=(ServerClientThread<ServerClientThreadCallback, DataHandler>&& other)
{
    m_severClientThreadCallback = std::move(other.m_severClientThreadCallback);
    m_serverThreadClientConfig = other.m_serverThreadClientConfig;
    m_eventLoop = std::move(other.m_eventLoop);
    m_eventChannel = std::move(other.m_eventChannel);
    m_thread = std::move(other.m_thread);
    m_name = std::move(other.m_name);
    return *this;
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
bool ServerClientThread<ServerClientThreadCallback, DataHandler>::init()
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

    if (m_eventLoop.add(m_eventChannel) == false)
    {
        LOG(ERROR) << "Thead[" << m_name << "] failed add event channel[" << (int) m_eventChannel.getInFD() << "]";
        return false;
    }

    return false;
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::start()
{
    std::thread(&ServerClientThread::run, this);
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::run()
{
    LOG(INFO) << "Started " << m_name;
    m_severClientThreadCallback.onStart();    
    m_eventLoop.start(m_serverThreadClientConfig.NUMBER_OF_EVENTS_PER_HANDLE);
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
{
    auto dataSocket = new lu::platform::socket::DataSocket<ServerClientThreadCallback, DataHandler>(m_severClientThreadCallback, std::move(*baseSocket));
    m_severClientThreadCallback.onNewConnection(dataSocket);
    m_eventLoop.add(*dataSocket);
    delete baseSocket;
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::join()
{
    m_thread.join();
    m_severClientThreadCallback.onExit();
}
