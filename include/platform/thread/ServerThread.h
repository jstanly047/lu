#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ServerClientThread.h>
#include <platform/thread/ServerConfig.h>
#include <platform/FDEventLoop.h>
#include <platform/FDTimer.h>
#include <platform/ITimerCallback.h>
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
        virtual void onClientClose(lu::platform::socket::DataSocket<IServerThreadCallback, lu::platform::socket::data_handler::String>&) =0 ;
        virtual void onData(lu::platform::socket::DataSocket<IServerThreadCallback, lu::platform::socket::data_handler::String>&, void*) = 0;
    };

    template <typename Base, typename Derived>
    concept BaseOrSameClass = std::is_base_of_v<Base, Derived> || std::is_same_v<Base, Derived>;


    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataHandler, lu::common::NonPtrClassOrStruct BaseSeverClientThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback>
    class ServerThread
    {
        static_assert(BaseOrSameClass<BaseSeverClientThreadCallback, SeverClientThreadCallback>, "BaseSeverClientThreadCallback must be a base class of BaseSeverClientThreadCallback or the same as BaseSeverClientThreadCallback");
    public:
        ServerThread(ServerThread&& other) = delete;
        ServerThread& operator=(ServerThread&& other) = delete;
        ServerThread(const ServerThread&) = delete;
        ServerThread& operator=(const ServerThread&) = delete;

        ServerThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, 
                    SeverConfig serverConfig = SeverConfig{}, bool reuseAddAndPort = true):
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


        ~ServerThread() {}

        bool init()
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

            if (m_serverConfig.TIMER_IN_MSEC != 0u)
            {
                if (m_timer.init() == false)
                {
                    LOG(ERROR) << "Thread[" << m_name << "] failed to create timer!";
                    return false;
                }
            }

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
            LOG(INFO) << "Started " << m_name;
            m_serverThreadCallback.onStart();
            
            if (m_serverSocket.setUpTCP(m_serverConfig.NUMBER_OF_CONNECTION_IN_WAITING_QUEUE) == false)
            {
                return;
            }

            m_serverSocket.getBaseSocket().setNonBlocking();
            m_eventLoop.add(m_serverSocket);

            if (m_serverConfig.TIMER_IN_MSEC != 0u)
            {
                m_timer.setToNonBlocking();
                m_timer.start(0, (int) m_serverConfig.TIMER_IN_MSEC * 1'000'000u);
                m_eventLoop.add(m_timer);
            }

            m_serverThreadCallback.onStartComplete();
            m_eventLoop.start(m_serverConfig.NUMBER_OF_EVENTS_PER_HANDLE);
        }

        void stop()
        {
            LOG(INFO) << "Stop " << m_name;

            if (m_serverConfig.TIMER_IN_MSEC == 0u)
            {
                m_timer.init();
                m_timer.setToNonBlocking();
                m_eventLoop.add(m_timer);
                m_timer.start(0, 1'000'000u, false);
            }

            m_eventLoop.stop();
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
                    m_eventLoop.add(*dataSocket);
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
                serverClientThread.getServerClientThreadCallback().onExit();
            }

            m_thread.join();
            m_serverThreadCallback.onExit();
        }

        const auto& getName() const { return m_name; }
        auto& getSeverClientThreads() { return m_serverClientThreads; }

    private:
        std::string m_name;
        ServerThreadCallback& m_serverThreadCallback;
        lu::platform::FDEventLoop m_eventLoop;
        lu::platform::socket::ServerSocket<ServerThread> m_serverSocket;
        lu::platform::FDTimer<ServerThreadCallback> m_timer;
        unsigned int m_currentClientHandler{};
        std::vector<ServerClientThread<BaseSeverClientThreadCallback, DataHandler>> m_serverClientThreads;
        const SeverConfig m_serverConfig;
        std::thread m_thread;
    };
}

