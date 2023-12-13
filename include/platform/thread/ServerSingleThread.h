#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ClientThread.h>
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
                    SeverConfig serverConfig = SeverConfig{}, bool reuseAddAndPort = true)
                    :
                    EventThread<ServerThreadCallback>(serverThreadCallback, name, EventThreadConfig(serverConfig)),
                    m_serverThreadCallback(serverThreadCallback),
                    m_serverSocket(service, *this, reuseAddAndPort),
                    m_serverConfig(serverConfig),
                    m_syncStart(0u)
        {
            if (m_serverConfig.CREATE_NEW_THREAD)
            {
                m_syncStart.update(1u);
            }
        }


        ~ServerSingleThread() {}

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
            std::unique_ptr<DataSocketType> dataSocket(new DataSocketType(this->m_serverThreadCallback, std::move(*baseSocket)));
            LOG(INFO) << "[" << this->getName() << "] New connection Socket[" << dataSocket.get() << "] FD[" << (int) baseSocket->getFD() << "] "
                        << " IP[" << baseSocket->getIP() << ":" << baseSocket->getPort()  << "]";
            this->m_serverThreadCallback.onNewConnection(*dataSocket.get());
            this->addToEventLoop(std::move(dataSocket));
            delete baseSocket;
        }

        void join()
        {
            EventThread<ServerThreadCallback>::join();
        }

        void stop()
        {
            EventThread<ServerThreadCallback>::stop();
        }

        void connectTo(LuThread &luThread)
        {

            this->connect(luThread);
        }

        void connectFrom(LuThread& thread)
        {
            m_threadsConnecting.push_back(&thread);
        }

        void onAppMsg(void* msg, lu::platform::thread::channel::ChannelID channelID)
        {
            this->m_serverThreadCallback.onAppMsg(msg, channelID);
        }

        const lu::platform::EventNotifier& getEventNotifier() const { return *m_eventNotifier; }
        ServerThreadCallback& getCallback() { return m_serverThreadCallback; }

    private:
        void run() override final
        {
            for (auto thread : m_threadsConnecting)
            {
                this->m_eventChannelForConnectingThreads.emplace_back(*this, this->getName());
                if (m_eventChannelForConnectingThreads.back().init() == false)
                {
                    LOG(ERROR) << "Failed to init event channel!";
                    std::abort();
                }

                thread->connectTo(this->getChannelID(), m_eventChannelForConnectingThreads.back().getEventNotifier());
                LOG(INFO) << "Connect thread " << thread->getName() << " to server thread " << this->getName(); 
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
        SeverConfig m_serverConfig;
        lu::utils::WaitForCount m_syncStart;
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ServerSingleThread>> m_eventChannelForConnectingThreads;
        std::vector<LuThread*> m_threadsConnecting;
    };
}
