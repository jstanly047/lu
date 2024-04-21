#pragma once
#include <platform/thread/EventThread.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/thread/ServerThread.h>
#include <platform/socket/websocket/HandshakeRequest.h>

#include <memory>
#include <list>

namespace lu::platform::thread
{
    template <typename T, typename = void>
    struct HasStartHandshake : std::false_type
    {
    };

    template <typename T>
    struct HasStartHandshake<T, std::void_t<decltype(std::declval<T>().startHandshake(std::declval<const  lu::platform::socket::websocket::InitialRequestInfo& >()))>> : std::true_type
    {
    };

    template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ConnectionThread : public EventThread<ConnectionThreadCallback>
    {
        struct Service
        {
            Service(const std::string& h, const std::string& s, bool isWebSocketPara = false)
                :
                host(h),
                service(s),
                isWebSocket(isWebSocketPara),
                connection()
            {
            }

            std::string host;
            std::string service;
            bool isWebSocket;
            std::unique_ptr<lu::platform::socket::ConnectSocket<ConnectionThreadCallback, DataSocketType>> connection;
        };

    public:
        ConnectionThread(const ConnectionThread&) = delete;
        ConnectionThread& operator=(const ConnectionThread&) = delete;
        ConnectionThread(ConnectionThread&& other)= delete;
        ConnectionThread& operator=(ConnectionThread&& other) = delete;
        
        ConnectionThread(const std::string& name,  ConnectionThreadCallback& connectionThreadCallback, EventThreadConfig threadConfig) 
            :
            EventThread<ConnectionThreadCallback>(connectionThreadCallback, name, threadConfig),
            m_connectionThreadCallback(connectionThreadCallback)
        {

        }

        virtual ~ConnectionThread()
        {
           if (m_sslCtx != nullptr)
            {
                ::SSL_CTX_free(m_sslCtx);
                ERR_free_strings();
                EVP_cleanup();
            } 
        }

        bool init()
        {
            if constexpr (std::is_same_v<decltype(std::declval<DataSocketType>().getSocket()), lu::platform::socket::SSLSocket&>)
            {
                SSL_library_init();
                OpenSSL_add_all_algorithms();
                SSL_load_error_strings();
                m_sslCtx = ::SSL_CTX_new(::TLS_client_method());

                if (m_sslCtx == nullptr)
                {
                    LOG(ERROR) << "Can not create TLS context";
                    return false;
                }
            }

            return EventThread<ConnectionThreadCallback>::init();
        }

        void start(bool createThread = false)
        {
            if (createThread == false)
            {
                run();
                return;
            }

            lu::platform::thread::LuThread::start();
        }

        void connectTo(const std::string& host, const std::string& service, bool duplicateCheck = true)
        {
            if (duplicateCheck)
            {
                auto itr = std::find_if(m_services.begin(), m_services.end(), [&](const Service &item)
                                    { return item.host == host && item.service == service; });

                if (itr != m_services.end())
                {
                    return;
                }
            }

            m_services.push_back(Service{host, service});
            m_services.back().connection.reset(new lu::platform::socket::ConnectSocket<ConnectionThreadCallback, DataSocketType>(host, service));
        }

        void connectTo(lu::platform::socket::websocket::InitialRequestInfo initialRequestInfo, bool duplicateCheck = true)
        {
            if (duplicateCheck)
            {
                auto itr = std::find_if(m_websocketServices.begin(), m_websocketServices.end(), [&](auto &item)
                                    { return item.host == initialRequestInfo.host && item.port == initialRequestInfo.port; });

                if (itr != m_websocketServices.end())
                {
                    return;
                }
            }
            m_websocketServices.push_back(initialRequestInfo);
            m_services.push_back(Service{initialRequestInfo.host, initialRequestInfo.port, true});
            m_services.back().connection.reset(new lu::platform::socket::ConnectSocket<ConnectionThreadCallback, DataSocketType>(initialRequestInfo.host, initialRequestInfo.port));
        }

        void onNewConnection([[maybe_unused]]lu::platform::socket::BaseSocket *baseSocket)
        {
        }

        void onNewConnection([[maybe_unused]]lu::platform::socket::SSLSocket *sslSocket)
        {
        }

        void onAppMsg(void* msg, lu::platform::thread::channel::ChannelID channelID)
        {
            this->m_connectionThreadCallback.onAppMsg(msg, channelID);
        }

        void connect(LuThread& thread)
        {
            LuThread::connect(thread);
        }

        void connect()
        {
            for (auto serviceItr = m_services.begin() ; serviceItr != m_services.end(); )
            {
                if (serviceItr->connection->getDataSocket() != nullptr)
                {
                    serviceItr++;
                    continue;
                }

                if (serviceItr->connection->connectToTCP(m_connectionThreadCallback, m_sslCtx))
                {
                    m_connectionThreadCallback.onConnection(*(serviceItr->connection->getDataSocket()));

                    if constexpr (HasStartHandshake<DataSocketType>::value)
                    {
                        if (serviceItr->isWebSocket)
                        {
                            auto itr = std::find_if(m_websocketServices.begin(), m_websocketServices.end(), [&](auto &item)
                                                    { return item.host == serviceItr->host && item.port == serviceItr->service; });

                            if (itr != m_websocketServices.end())
                            {
                                serviceItr->connection->getDataSocket()->startHandshake(*itr);
                            }
                        }
                    }

                    this->addToEventLoop(std::move(serviceItr->connection->getDataSocket()));
                    assert(serviceItr->connection->getDataSocket() == nullptr);
                    serviceItr = m_services.erase(serviceItr);
                    continue;
                }
                
                serviceItr++;
            }
        }

    private:
        bool isIOThread() override final { return true; }

        void run() override final
        {
            for (auto& thread : this->m_threadsSendingMsg)
            {
                this->m_eventChannelForConnectingThreads.emplace_back(*this, this->getName());
                if (m_eventChannelForConnectingThreads.back().init() == false)
                {
                    LOG(ERROR) << "Failed to init event channel!";
                    std::abort();
                }

                thread.get().connectTo(this->getChannelID(), m_eventChannelForConnectingThreads.back().getEventNotifier());
                LOG(INFO) << "Connect thread " << thread.get().getName() << " to server thread " << this->getName(); 
                this->addToEventLoop(m_eventChannelForConnectingThreads.back());
            }
            
            connect();
            EventThread<ConnectionThreadCallback>::run();
        }

        ConnectionThreadCallback& m_connectionThreadCallback;
        std::list<Service> m_services;
        std::list<lu::platform::socket::websocket::InitialRequestInfo> m_websocketServices;
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ConnectionThread>> m_eventChannelForConnectingThreads;
        ::SSL_CTX* m_sslCtx{};
    };
}

