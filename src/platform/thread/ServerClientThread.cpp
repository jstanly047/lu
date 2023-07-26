#include <platform/thread/ServerClientThread.h>
#include <platform/socket/IDataHandler.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerClientThread<ServerClientThreadCallback, DataHandler>::ServerClientThread(ServerClientThreadCallback& severClientThreadCallback,
    const std::string& name, ClientThreadConfig serverConfig) : ClientThread(severClientThreadCallback, name, serverConfig)
    :
    m_eventChannel(*this)
{
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerClientThread<ServerClientThreadCallback, DataHandler>::ServerClientThread(ServerClientThread&& other):
    ClientThread(std::move(other)),
    m_eventChannel(std::move(other.m_eventChannel))    
{

}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerClientThread<ServerClientThreadCallback, DataHandler>& ServerClientThread<ServerClientThreadCallback, DataHandler>::operator=(ServerClientThread<ServerClientThreadCallback, DataHandler>&& other)
{
    ClientThread::operator=(std::move(other));
    m_eventChannel = std::move(other.m_eventChannel);
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

    return ClientThread::init();
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::start()
{
    std::thread(&ServerClientThread::run, this);
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::run()
{
    m_eventLoop.add(m_eventChannel);
    ClientThread::run();
}

template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerClientThread<ServerClientThreadCallback, DataHandler>::onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
{
    auto dataSocket = new lu::platform::socket::DataSocket<ServerClientThreadCallback, DataHandler>(m_severClientThreadCallback, std::move(*baseSocket));
    m_severClientThreadCallback.onNewConnection(dataSocket);
    dataSocket->getBaseSocket().setNonBlocking();
    m_eventLoop.add(*dataSocket);
    delete baseSocket;
}

template class ServerClientThread<IClientThreadCallback, lu::platform::socket::data_handler::String>;