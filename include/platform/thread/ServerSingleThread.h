#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ClientThread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>
#include <vector>
#include <list>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
    class ServerSingleThread : public EventThread<ServerThreadCallback>
    {
    public:
        ServerSingleThread(ServerSingleThread&& other) = delete;
        ServerSingleThread& operator=(ServerSingleThread&& other) = delete;
        ServerSingleThread(const ServerSingleThread&) = delete;
        ServerSingleThread& operator=(const ServerSingleThread&) = delete;

        ServerSingleThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, 
                    ServerConfig serverConfig = ServerConfig{}, bool reuseAddAndPort = true)
                    :
                    EventThread<ServerThreadCallback>(serverThreadCallback, name, EventThreadConfig(serverConfig)),
                    m_serverThreadCallback(serverThreadCallback),
                    m_serverSocket(service, *this, reuseAddAndPort),
                    m_serverConfig(serverConfig),
                    m_syncStart(0u)
        {
            if constexpr (std::is_same_v<decltype(std::declval<DataSocketType>().getSocket()), lu::platform::socket::SSLSocket&>)
            {
                SSL_library_init();
                OpenSSL_add_all_algorithms();
                SSL_load_error_strings();
                m_sslCtx = ::SSL_CTX_new(::TLS_server_method());

                if (::SSL_CTX_use_certificate_file(m_sslCtx, "resource/server.crt", SSL_FILETYPE_PEM) <= 0)
                {
                    LOG(ERROR) << "Error loading certificate";
                    std::abort();
                }
                if (::SSL_CTX_use_PrivateKey_file(m_sslCtx, "resource/server.key", SSL_FILETYPE_PEM) <= 0)
                {
                    LOG(ERROR) << "Error loading private key";
                    std::abort();
                }
            }

            if (m_serverConfig.CREATE_NEW_THREAD)
            {
                m_syncStart.update(1u);
            }
        }


        ~ServerSingleThread() 
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
            return EventThread<ServerThreadCallback>::init();
        }

        void start()
        {
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

                std::unique_ptr<DataSocketType> dataSocket(new DataSocketType(this->m_serverThreadCallback, lu::platform::socket::SSLSocket(ssl, std::move(*baseSocket))));
                LOG(INFO) << "[" << this->getName() << "] New connection Socket[" << dataSocket.get() << "] FD[" << (int) dataSocket->getBaseSocket().getFD() << "] "
                            << " IP[" << dataSocket->getIP() << ":" << dataSocket->getPort()  << "]";
                this->m_serverThreadCallback.onNewConnection(*dataSocket.get());
                this->addToEventLoop(std::move(dataSocket));
                delete baseSocket;
            }
            else
            {
                std::unique_ptr<DataSocketType> dataSocket(new DataSocketType(this->m_serverThreadCallback, std::move(*baseSocket)));
                LOG(INFO) << "[" << this->getName() << "] New connection Socket[" << dataSocket.get() << "] FD[" << (int) dataSocket->getBaseSocket().getFD() << "] "
                            << " IP[" <<dataSocket->getIP() << ":" << dataSocket->getPort()  << "]";
                this->m_serverThreadCallback.onNewConnection(*dataSocket.get());
                this->addToEventLoop(std::move(dataSocket));
                delete baseSocket;
            }
        }

        void join()
        {
            EventThread<ServerThreadCallback>::join();
        }

        void stop()
        {
            EventThread<ServerThreadCallback>::stop();
        }

        void onAppMsg(void* msg, lu::platform::thread::channel::ChannelID channelID)
        {
            this->m_serverThreadCallback.onAppMsg(msg, channelID);
        }

        const lu::platform::EventNotifier& getEventNotifier() const { return *m_eventNotifier; }
        ServerThreadCallback& getCallback() { return m_serverThreadCallback; }

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

            EventThread<ServerThreadCallback>::run();
        }

        ServerThreadCallback& m_serverThreadCallback;
        lu::platform::socket::ServerSocket<ServerSingleThread> m_serverSocket;
        ServerConfig m_serverConfig;
        lu::utils::WaitForCount m_syncStart;
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ServerSingleThread>> m_eventChannelForConnectingThreads;
        ::SSL_CTX *m_sslCtx{};
    };
}
