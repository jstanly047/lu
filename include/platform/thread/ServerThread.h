#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ClientThread.h>

#include <vector>

namespace lu::platform::thread
{
    template <typename Base, typename Derived>
    concept BaseOrSameClass = std::is_base_of_v<Base, Derived> || std::is_same_v<Base, Derived>;


    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType, 
            lu::common::NonPtrClassOrStruct ClientThreadCallback, 
            lu::common::NonPtrClassOrStruct BaseClientThreadCallback=ClientThreadCallback>
    class ServerThread : public EventThread<ServerThreadCallback>
    {
        static_assert(BaseOrSameClass<BaseClientThreadCallback, ClientThreadCallback>);
    public:
        ServerThread(ServerThread&& other) = delete;
        ServerThread& operator=(ServerThread&& other) = delete;
        ServerThread(const ServerThread&) = delete;
        ServerThread& operator=(const ServerThread&) = delete;

        ServerThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, 
                    SeverConfig serverConfig = SeverConfig{}, bool reuseAddAndPort = true)
                    :
                    EventThread<ServerThreadCallback>(serverThreadCallback, name, EventThreadConfig(serverConfig)),
                    m_serverThreadCallback(serverThreadCallback),
                    m_serverSocket(service, *this, reuseAddAndPort),
                    m_currentClientHandler(0u),
                    m_serverClientThreads(),
                    m_serverConfig(serverConfig),
                    m_syncStart(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS)
        {
            if (m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS >= std::thread::hardware_concurrency() - 1u)
            {
                m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS = std::thread::hardware_concurrency() - 1u;
                m_syncStart.update(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
            }

            if (m_serverConfig.CREATE_NEW_THREAD)
            {
                m_syncStart.update(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS + 1);
            }

            m_serverClientThreads.reserve(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
            m_serverClientThreadsCallbacks.reserve(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
            m_serverClientThreadsCallbacksPtr.reserve(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
            EventThreadConfig eventThreadConfig(m_serverConfig);

            for (unsigned int i = 0u; i < m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS; i++)
            {
                std::string clientThreadName = this->m_name + "_" + std::to_string(i+1);
                eventThreadConfig.TIMER_NAME += std::to_string(i+1);
                m_serverClientThreadsCallbacks.emplace_back(ClientThreadCallback());
                m_serverClientThreads.emplace_back(std::make_unique<ClientThread<BaseClientThreadCallback, DataSocketType>>(static_cast<BaseClientThreadCallback&>(m_serverClientThreadsCallbacks.back()), 
                                                clientThreadName, eventThreadConfig, m_syncStart));
                m_serverClientThreadsCallbacksPtr.push_back(&m_serverClientThreadsCallbacks.back());
                m_serverClientThreadsCallbacks.back().setThread(*m_serverClientThreads.back());
            }
        }


        ~ServerThread() {}

        bool init()
        {
            for (auto &serverClientThread : m_serverClientThreads)
            {
                serverClientThread->init();
            }

            EventThread<ServerThreadCallback>::init();
            return true;
        }

        void start()
        {
            for (auto& serverClientThread : m_serverClientThreads)
            {
                serverClientThread->start();
            }

            if (m_serverConfig.CREATE_NEW_THREAD)
            {
                LuThread::start();
                m_syncStart.wait();
            }
            else
            {
                run();
            }
        }

        void onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
        {
            m_currentClientHandler = m_currentClientHandler % m_serverClientThreads.size();
            m_serverClientThreads[m_currentClientHandler]->getEventChannel().notify(lu::platform::EventData(lu::platform::EventData::NewConnection, baseSocket));
            m_currentClientHandler++;
        }

        void join()
        {
            for (auto& serverClientThread : m_serverClientThreads)
            {
                serverClientThread->join();
            }

            EventThread<ServerThreadCallback>::join();
        }

        void stop()
        {
            for (auto& serverClientThread : m_serverClientThreads)
            {
                serverClientThread->stop();
            }

            EventThread<ServerThreadCallback>::stop();
        }

        std::vector<BaseClientThreadCallback*>& getClientThreadCallbacks() { return m_serverClientThreadsCallbacksPtr; }

    private:
        void run() override final
        {
            if (m_serverSocket.setUpTCP(m_serverConfig.NUMBER_OF_CONNECTION_IN_WAITING_QUEUE) == false)
            {
                return;
            }

            //this->m_serverSocket.getBaseSocket().setNonBlocking();
            this->addToEventLoop(m_serverSocket);

            if (m_serverConfig.CREATE_NEW_THREAD)
            {
                m_syncStart.increment();
            }
            else
            {
                m_syncStart.wait();
            }

            EventThread<ServerThreadCallback>::run();
        }

        ServerThreadCallback& m_serverThreadCallback;
        lu::platform::socket::ServerSocket<ServerThread> m_serverSocket;
        unsigned int m_currentClientHandler{};
        std::vector<ClientThreadCallback> m_serverClientThreadsCallbacks;
        std::vector<BaseClientThreadCallback*> m_serverClientThreadsCallbacksPtr;
        std::vector<std::unique_ptr<ClientThread<BaseClientThreadCallback, DataSocketType>>> m_serverClientThreads;
        SeverConfig m_serverConfig;
        lu::utils::WaitForCount m_syncStart;
    };
}
