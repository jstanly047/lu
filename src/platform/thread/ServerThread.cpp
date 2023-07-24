#include <platform/thread/ServerThread.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::ServerThread(const std::string& name, 
    ServerThreadCallback& serverThreadCallback,  const std::string& service, SeverConfig serverConfig, bool reuseAddAndPort):
    m_serverSocket(service, *this, reuseAddAndPort),
    m_serverThreadCallback(serverThreadCallback),
    m_serverConfig(serverConfig),
    m_name(name),
    m_currentClientHandler(0)
{
    //TODO: One is deducted for server accept thread
    if (serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS >= std::thread::hardware_concurrency() - 1u)
    {
        serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS = std::thread::hardware_concurrency() - 1u;
    }

    m_serverClientThreads.reserve(serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
    SeverClientThreadConfig serverClientThreadConfig;
    serverClientThreadConfig.NUMBER_OF_EVENTS_PER_HANDLE = m_serverConfig.CLIENT_HANDLER_NUMBER_OF_EVENTS_PER_HANDLE;

    for (unsigned int i = 0u; i < serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS; i++)
    {
        std::string clientThreadName = m_name + "_client_handler_" + std::to_string(i);
        m_serverClientThreads.emplace_back(ServerClientThread<SeverClientThreadCallback, DataHandler>(clientThreadName, serverClientThreadConfig));
    }
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
bool ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::init()
{
    for (auto &serverClientThread : m_serverClientThreads)
    {
        if (serverClientThread.getServerClientThreadCallback().onInit() == false)
        {
            return false;
        }
    }

    return m_eventLoop.init();
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::start()
{
    for (auto& serverClientThread : m_serverClientThreads)
    {
        serverClientThread.start();
    }

    if (m_serverConfig.CREATE_NEW_THREAD)
    {
        std::thread(&ServerThread::run, this);
    }
    else
    {
        run();
    }
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::run()
{
    LOG(INFO) << "Started " << m_name;
    m_serverThreadCallback.onStart();
    m_eventLoop.add(m_serverSocket);
    

    if (m_serverSocket.setUpTCP(m_serverConfig.NUMBER_OF_CONNECTION_IN_WAITING_QUEUE) == false)
    {
        return;
    }

    m_serverSocket.getBaseSocket().setNonBlocking();
    m_eventLoop.start(m_serverConfig.NUMBER_OF_EVENTS_PER_HANDLE);
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
{
    if (m_currentClientHandler == m_serverClientThreads.size() && m_serverConfig.ACT_AS_CLIENT_HANDLER)
    {
        m_serverThreadCallback.onNewConnection(std::move(*baseSocket));
        delete baseSocket;
        m_currentClientHandler = 0;
    }

    m_serverClientThreads[m_currentClientHandler++].getEventChannel().notify(lu::platform::EventData(lu::platform::EventData::NewConnection, baseSocket));
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
void ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::join()
{
    for (auto& serverClientThread : m_serverClientThreads)
    {
        serverClientThread.join();
        serverClientThread.getServerClientThreadCallback().onExit();
    }

    m_thread.join();
    m_serverThreadCallback.onExit();
}

template class lu::platform::thread::ServerThread<lu::platform::thread::IServerThreadCallback, 
                        lu::platform::thread::IServerClientThreadCallback<lu::platform::socket::IDataHandler>, lu::platform::socket::IDataHandler>;
