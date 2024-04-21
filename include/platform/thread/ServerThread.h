#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ClientThread.h>
#include <platform/socket/SSLSocket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <vector>
#include <type_traits>

namespace lu::platform::thread
{
    template <typename Base, typename Derived>
    concept BaseOrSameClass = std::is_base_of_v<Base, Derived> || std::is_same_v<Base, Derived>;

    template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct CTDataSocketType>
    class ConnectionThread;

    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType, 
            lu::common::NonPtrClassOrStruct ClientThreadCallback, 
            lu::common::NonPtrClassOrStruct BaseClientThreadCallback=ClientThreadCallback>
    class ServerThread : public EventThread<ServerThreadCallback>
    {
        static_assert(BaseOrSameClass<BaseClientThreadCallback, ClientThreadCallback>);

        template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct CTDataSocketType>
        friend class ConnectionThread;

    public:
        ServerThread(ServerThread&& other) = delete;
        ServerThread& operator=(ServerThread&& other) = delete;
        ServerThread(const ServerThread&) = delete;
        ServerThread& operator=(const ServerThread&) = delete;

        ServerThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, 
                    ServerConfig serverConfig = ServerConfig{}, bool reuseAddAndPort = true)
                    :
                    EventThread<ServerThreadCallback>(serverThreadCallback, name, EventThreadConfig(serverConfig)),
                    m_serverThreadCallback(serverThreadCallback),
                    m_serverSocket(service, *this, reuseAddAndPort),
                    m_currentClientHandler(0u),
                    m_serverClientThreads(),
                    m_serverConfig(serverConfig),
                    m_syncStart(m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS)
        {

            if constexpr (std::is_same_v<decltype(std::declval<DataSocketType>().getSocket()), lu::platform::socket::SSLSocket&>)
            {
                ::SSL_library_init();
                ::OpenSSL_add_all_algorithms();
                ::SSL_load_error_strings();
                m_sslCtx = ::SSL_CTX_new(::TLS_server_method());

                if (SSL_CTX_use_certificate_file(m_sslCtx, "resource/server.crt", SSL_FILETYPE_PEM) <= 0)
                {
                    LOG(ERROR) << "Error loading certificate";
                    std::abort();
                }
                if (SSL_CTX_use_PrivateKey_file(m_sslCtx, "resource/server.key", SSL_FILETYPE_PEM) <= 0)
                {
                    LOG(ERROR) << "Error loading private key";
                    std::abort();
                }
            }

            if (m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS >= std::thread::hardware_concurrency())
            {
                m_serverConfig.NUMBER_OF_CLIENT_HANDLE_THREADS = std::thread::hardware_concurrency();
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
                //m_serverClientThreadsCallbacks.back().setThread(*m_serverClientThreads.back());
            }
        }


        ~ServerThread() 
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
            for (auto &serverClientThread : m_serverClientThreads)
            {
                for (auto &threadSendingMessage : this->m_threadsSendingMsg)
                {
                    threadSendingMessage.get().connect(*serverClientThread);
                }

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
            if constexpr (std::is_same_v<decltype(std::declval<DataSocketType>().getSocket()), lu::platform::socket::SSLSocket&>)
            {
                ::SSL *ssl = ::SSL_new(m_sslCtx);

                if (ssl == nullptr)
                {
                    LOG(ERROR) << "Error performing TLS handshake [" << baseSocket->getIP() << ":" << baseSocket->getPort() << "]";
                    delete baseSocket;
                    return;
                }

                ::SSL_set_fd(ssl, baseSocket->getFD());

                if (::SSL_accept(ssl) <= 0) 
                {
                    LOG(ERROR) << "Error performing TLS handshake [" << baseSocket->getIP() << ":" << baseSocket->getPort() << "]";
                    ::SSL_free(ssl);
                    delete baseSocket;
                    return;
                }

                auto sslSocket = new lu::platform::socket::SSLSocket(ssl, std::move(*baseSocket));
                m_currentClientHandler = m_currentClientHandler % m_serverClientThreads.size();
                m_serverClientThreads[m_currentClientHandler]->getEventNotifier().notify(lu::platform::EventData(lu::platform::EventData::NewConnection, 0, sslSocket));
                m_currentClientHandler++;
                delete baseSocket;
            }
            else
            {
                m_currentClientHandler = m_currentClientHandler % m_serverClientThreads.size();
                m_serverClientThreads[m_currentClientHandler]->getEventNotifier().notify(lu::platform::EventData(lu::platform::EventData::NewConnection, 0, baseSocket));
                m_currentClientHandler++;
            }
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

        void connect(LuThread& luThread)
        {
            for (auto& serverClientThread : m_serverClientThreads)
            {
                serverClientThread->connect(luThread);
            }
        }

        std::vector<BaseClientThreadCallback*>& getClientThreadCallbacks() { return m_serverClientThreadsCallbacksPtr; }

    private:
        bool isIOThread() override final { return true; }

        void run() override final
        {
            if (m_serverSocket.setUpTCP(m_serverConfig.NUMBER_OF_CONNECTION_IN_WAITING_QUEUE, m_serverConfig.IPV6) == false)
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
        ServerConfig m_serverConfig;
        lu::utils::WaitForCount m_syncStart;
        ::SSL_CTX *m_sslCtx{};
    };
}
