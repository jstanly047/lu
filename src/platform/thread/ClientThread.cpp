#include <platform/thread/ClientThread.h>
#include <platform/thread/IClientThreadCallback.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ClientThread<ClientThreadCallback, DataHandler>::ClientThread(ClientThreadCallback& clientThreadCallback,
    const std::string& name, EventThreadConfig threadConfig) : 
    EventThread<ClientThreadCallback>(clientThreadCallback, name, threadConfig),
    m_clientThreadCallback(clientThreadCallback),
    m_eventChannel(*this)
{
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ClientThread<ClientThreadCallback, DataHandler>::ClientThread(ClientThread&& other):
    EventThread<ClientThreadCallback>(std::move(other)),
    m_clientThreadCallback(other.m_clientThreadCallback),
    m_eventChannel(std::move(other.m_eventChannel))    
{

}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ClientThread<ClientThreadCallback, DataHandler>& ClientThread<ClientThreadCallback, DataHandler>::operator=(ClientThread<ClientThreadCallback, DataHandler>&& other)
{
    EventThread<ClientThreadCallback>::operator=(std::move(other));
    m_clientThreadCallback = other.m_clientThreadCallback;
    m_eventChannel = std::move(other.m_eventChannel);
    return *this;
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
bool ClientThread<ClientThreadCallback, DataHandler>::init()
{
    if (m_eventChannel.init() == false)
    {
        LOG(ERROR) << "Thead[" << this->m_name << "] failed to create event channel!";
        return false;
    }

    return ClientThread<ClientThreadCallback, DataHandler>::init();
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::start()
{
    std::thread(&ClientThread::run, this);
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::run()
{
    this->m_eventLoop.add(m_eventChannel);
    ClientThread<ClientThreadCallback, DataHandler>::run();
}

template<lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ClientThread<ClientThreadCallback, DataHandler>::onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
{
    auto dataSocket = new lu::platform::socket::DataSocket<ClientThreadCallback, DataHandler>(this->m_clientThreadCallback, std::move(*baseSocket));
    this->m_clientThreadCallback.onNewConnection(dataSocket);
    dataSocket->getBaseSocket().setNonBlocking();
    this->m_eventLoop.add(*dataSocket);
    delete baseSocket;
}

template class ClientThread<IClientThreadCallback, lu::platform::socket::data_handler::String>;