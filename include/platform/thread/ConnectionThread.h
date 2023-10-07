#pragma once
#include <platform/thread/EventThread.h>
#include <platform/socket/ConnectSocket.h>

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

            this->m_thread = std::thread(&ConnectionThread::run, this);
        }

        void run()
        {
            for (Service& service : m_services)
            {
                service.connection->connectToTCP(m_connectionThreadCallback);
            }
            
            for (Service& service : m_services)
            {
                if (service.connection->getDataSocket() == nullptr)
                {
                    continue;
                }

                m_connectionThreadCallback.onConnection(*(service.connection->getDataSocket()));
                this->m_eventLoop.add(*(service.connection->getDataSocket()));
            }

            EventThread<ConnectionThreadCallback>::run();

            m_connectionThreadCallback.onExit();
        }

        void connectTo(const std::string& host, const std::string& service)
        {
            m_services.emplace_back(Service{host, service});
        }

    private:
        ConnectionThreadCallback& m_connectionThreadCallback;
        std::list<Service> m_services;
    };
}

