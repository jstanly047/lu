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
            for (auto serviceItr = m_services.begin() ; serviceItr != m_services.end(); )
            {
                if (serviceItr->connection->getDataSocket() != nullptr)
                {
                    serviceItr++;
                    continue;
                }

                if (serviceItr->connection->connectToTCP(m_connectionThreadCallback))
                {
                    m_connectionThreadCallback.onConnection(*(serviceItr->connection->getDataSocket()));
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
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ConnectionThread>> m_eventChannelForConnectingThreads;
    };
}

