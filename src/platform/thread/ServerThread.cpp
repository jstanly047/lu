#include <platform/thread/ServerThread.h>
#include <glog/logging.h>

using namespace lu::platform::thread;

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::ServerThread(const std::string& name, 
    ServerThreadCallback& serverThreadCallback,  const std::string& service, SeverConfig serverConfig, bool reuseAddAndPort):
    m_name(name),
    m_serverThreadCallback(serverThreadCallback),
    m_eventLoop(),
    m_serverSocket(service, *this, reuseAddAndPort),
    m_timer(serverThreadCallback, m_serverConfig.TIMER_NAME),
    m_currentClientHandler(0u),
    m_serverClientThreads(),
    m_serverConfig(serverConfig),
    m_thread()
{
    //TODO: One is deducted for server accept thread
    if (serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS >= std::thread::hardware_concurrency() - 1u)
    {
        serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS = std::thread::hardware_concurrency() - 1u;
    }

    m_serverClientThreads.reserve(serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
    SeverClientThreadConfig serverClientThreadConfig(m_serverConfig);
    

    for (unsigned int i = 0u; i < serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS; i++)
    {
        std::string clientThreadName = m_name + "_client_handler_" + std::to_string(i);
        serverClientThreadConfig.TIMER_NAME += std::to_string(i+1);
        m_serverClientThreads.emplace_back(ServerClientThread<SeverClientThreadCallback, DataHandler>(clientThreadName, serverClientThreadConfig));
    }
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
bool ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::init()
{
    for (auto &serverClientThread : m_serverClientThreads)
    {
        if (serverClientThread.getServerClientThreadCallback().onInit() == false)
        {
            return false;
        }
    }

    if (m_eventLoop.init() == false)
    {
        LOG(ERROR) << "Thread[" << m_name << "] failed to create event channel!";
        return false;
    }

    if (m_serverConfig.TIMER_IN_MSEC == 0u)
    {
        if (m_timer.init() == false)
        {
            LOG(ERROR) << "Thread[" << m_name << "] failed to create timer!";
            return false;
        }
    }

    return true;
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
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

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
void ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::run()
{
    LOG(INFO) << "Started " << m_name;
    m_serverThreadCallback.onStart();
    
    if (m_serverSocket.setUpTCP(m_serverConfig.NUMBER_OF_CONNECTION_IN_WAITING_QUEUE) == false)
    {
        return;
    }

    m_serverSocket.getBaseSocket().setNonBlocking();
    m_eventLoop.add(m_serverSocket);
    m_timer.setToNonBlocking();
    m_eventLoop.add(m_timer);
    m_serverThreadCallback.onStartComplete();
    m_timer.start(0, (int) m_serverConfig.TIMER_IN_MSEC * 1000, m_serverConfig.REPEAT_TIMER);
    m_eventLoop.start(m_serverConfig.NUMBER_OF_EVENTS_PER_HANDLE);
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
void ServerThread<ServerThreadCallback, SeverClientThreadCallback, DataHandler>::onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
{
    if (m_currentClientHandler == m_serverClientThreads.size())
    {
        if (m_serverConfig.ACT_AS_CLIENT_HANDLER)
        {
            auto dataSocket = new lu::platform::socket::DataSocket<ServerThreadCallback, DataHandler>(m_serverThreadCallback, std::move(*baseSocket));
            m_serverThreadCallback.onNewConnection(std::move(*baseSocket));
            dataSocket->getBaseSocket().setNonBlocking();
            m_eventLoop.add(*dataSocket);
            delete baseSocket;
            m_currentClientHandler = 0;
            return;
        }
        
        m_currentClientHandler = 0;
    }

    m_serverClientThreads[m_currentClientHandler++].getEventChannel().notify(lu::platform::EventData(lu::platform::EventData::NewConnection, baseSocket));
}

template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
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
                        lu::platform::thread::IServerClientThreadCallback, lu::platform::socket::IDataHandler>;
template class lu::platform::FDTimer<lu::platform::thread::IServerThreadCallback>;
