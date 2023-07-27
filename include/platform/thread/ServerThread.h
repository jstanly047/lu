#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ClientThread.h>
#include <platform/thread/Config.h>
#include <glog/logging.h>

#include <thread>
#include <vector>


namespace lu::platform::thread
{
    class IServerThreadCallback
    {
    public:
        virtual void onStart() = 0;
        virtual void onStartComplete() = 0;
        virtual void nExit() = 0;
        virtual void onNewConnection(lu::platform::socket::DataSocket<IServerThreadCallback, lu::platform::socket::data_handler::String>* dataSocket) = 0;
        virtual void onTimer(const lu::platform::FDTimer<IServerThreadCallback>&) = 0;
        virtual void onClientClose(lu::platform::socket::DataSocket<IServerThreadCallback, lu::platform::socket::data_handler::String>&) = 0;
        virtual void onData(lu::platform::socket::DataSocket<IServerThreadCallback, lu::platform::socket::data_handler::String>&, void*) = 0;
    };

    template <typename Base, typename Derived>
    concept BaseOrSameClass = std::is_base_of_v<Base, Derived> || std::is_same_v<Base, Derived>;


    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataHandler, lu::common::NonPtrClassOrStruct ClientThreadCallback, lu::common::NonPtrClassOrStruct BaseClientThreadCallback=ClientThreadCallback>
    class ServerThread : public ClientThread<BaseClientThreadCallback, DataHandler>
    {
        static_assert(BaseOrSameClass<BaseClientThreadCallback, ClientThreadCallback>, "BaseClientThreadCallback must be a base class of BaseClientThreadCallback or the same as BaseClientThreadCallback");
    public:
        ServerThread(ServerThread&& other) = delete;
        ServerThread& operator=(ServerThread&& other) = delete;
        ServerThread(const ServerThread&) = delete;
        ServerThread& operator=(const ServerThread&) = delete;

        ServerThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, 
                    SeverConfig serverConfig = SeverConfig{}, bool reuseAddAndPort = true):
                    ClientThread<BaseClientThreadCallback, DataHandler>(static_cast<BaseClientThreadCallback&>(serverThreadCallback), name,  EventThreadConfig(serverConfig)),
                    m_serverThreadCallback(serverThreadCallback),
                    m_serverSocket(service, *this, reuseAddAndPort),
                    m_currentClientHandler(0u),
                    m_serverClientThreads(),
                    m_serverConfig(serverConfig)
        {
            if (serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS >= std::thread::hardware_concurrency() - 1u)
            {
                serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS = std::thread::hardware_concurrency() - 1u;
            }

            m_serverClientThreads.reserve(serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
            m_serverClientThreadsCallbacks.reserve(serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS);
            m_serverClientThreadsCallbacksPtr.reserve(serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS + 1);
            ClientThreadConfig clientThreadConfig(m_serverConfig);
            
            

            for (unsigned int i = 0u; i < serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS; i++)
            {
                std::string clientThreadName = this->m_name + "_client_handler_" + std::to_string(i);
                clientThreadConfig.TIMER_NAME += std::to_string(i+1);
                m_serverClientThreadsCallbacks.emplace_back(ClientThreadCallback());
                m_serverClientThreads.emplace_back(ServerClientThread<BaseClientThreadCallback, DataHandler>(static_cast<BaseClientThreadCallback&>(m_serverClientThreadsCallbacks.back()), clientThreadConfig));
                m_serverClientThreadsCallbacksPtr.push_back(&m_serverClientThreadsCallbacks.back());
            }

            if (m_serverConfig.ACT_AS_CLIENT_HANDLER)
            {
                m_serverClientThreadsCallbacksPtr.push_back(&m_serverThreadCallback);
            }
        }


        ~ServerThread() {}

        bool init()
        {
            for (auto &serverClientThread : m_serverClientThreads)
            {
                if (serverClientThread.getClientThreadCallback().onInit() == false)
                {
                    return false;
                }
            }

            ClientThread<BaseClientThreadCallback, DataHandler>::init();
            return true;
        }

        void start()
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

        void run()
        {
            LOG(INFO) << "Started " << this->m_name;
            m_serverThreadCallback.onStart();
            
            if (m_serverSocket.setUpTCP(m_serverConfig.NUMBER_OF_CONNECTION_IN_WAITING_QUEUE) == false)
            {
                return;
            }

            this->m_serverSocket.getBaseSocket().setNonBlocking();
            this->m_eventLoop.add(m_serverSocket);
            ClientThread<BaseClientThreadCallback, DataHandler>::run();
            m_serverThreadCallback.onStartComplete();
            this->m_eventLoop.start(m_serverConfig.NUMBER_OF_EVENTS_PER_HANDLE);
        }

        void onNewConnection(lu::platform::socket::BaseSocket* baseSocket)
        {
            if (m_currentClientHandler == m_serverClientThreads.size())
            {
                if (m_serverConfig.ACT_AS_CLIENT_HANDLER)
                {
                    auto dataSocket = new lu::platform::socket::DataSocket<ServerThreadCallback, DataHandler>(m_serverThreadCallback, std::move(*baseSocket));
                    m_serverThreadCallback.onNewConnection(baseSocket);
                    dataSocket->getBaseSocket().setNonBlocking();
                    this->m_eventLoop.add(*dataSocket);
                    delete baseSocket;
                    m_currentClientHandler = 0;
                    return;
                }
                
                m_currentClientHandler = 0;
            }

            m_serverClientThreads[m_currentClientHandler++].getEventChannel().notify(lu::platform::EventData(lu::platform::EventData::NewConnection, baseSocket));
        }

        void join()
        {
            for (auto& serverClientThread : m_serverClientThreads)
            {
                serverClientThread.join();
                serverClientThread.getClientThreadCallback().onExit();
            }

            ClientThread<BaseClientThreadCallback, DataHandler>::join();
            m_serverThreadCallback.onExit();
        }

        std::vector<BaseClientThreadCallback*>& getClientThreadCallbacks() { return m_serverClientThreadsCallbacksPtr; }

    private:
        ServerThreadCallback& m_serverThreadCallback;
        lu::platform::socket::ServerSocket<ServerThread> m_serverSocket;
        unsigned int m_currentClientHandler{};
        std::vector<ClientThreadCallback> m_serverClientThreadsCallbacks;
        std::vector<BaseClientThreadCallback*> m_serverClientThreadsCallbacksPtr;
        std::vector<ServerClientThread<BaseClientThreadCallback, DataHandler>> m_serverClientThreads;
        SeverConfig m_serverConfig;
    };
}

