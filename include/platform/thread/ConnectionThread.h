#pragma once
#include <platform/thread/EventThread.h>
#include <platform/socket/ConnectSocket.h>
#include <platform/thread/ServerThread.h>

#include <memory>
#include <list>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ConnectionThread : public EventThread<ConnectionThreadCallback>
    {
        struct Service
        {
            Service(const std::string& h, const std::string& s)
                :
                host(h),
                service(s),
                connection()
            {
            }

            std::string host;
            std::string service;
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

        virtual ~ConnectionThread() {}

        bool init()
        {
            for (Service& service : m_services)
            {
                service.connection.reset(new lu::platform::socket::ConnectSocket<ConnectionThreadCallback, DataSocketType>(service.host, service.service));
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

        void connectTo(const std::string& host, const std::string& service)
        {
            m_services.emplace_back(Service{host, service});
        }

        void onNewConnection([[maybe_unused]]lu::platform::socket::BaseSocket *baseSocket)
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
            std::list<Service*> servicesConnected;
            for (auto& service : m_services)
            {
                if (service.connection->getDataSocket() != nullptr)
                {
                    continue;
                }

                if (service.connection->connectToTCP(m_connectionThreadCallback))
                {
                    servicesConnected.push_back(&service);
                }
            }
            
            for (auto service : servicesConnected)
            {
                if (service->connection->getDataSocket() == nullptr)
                {
                    continue;
                }

                m_connectionThreadCallback.onConnection(*(service->connection->getDataSocket()));
                this->addToEventLoop(std::move(service->connection->getDataSocket()));
                assert(service->connection->getDataSocket() == nullptr);
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
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ConnectionThread>> m_eventChannelForConnectingThreads;
    };
}

