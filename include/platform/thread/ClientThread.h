#pragma once

#include <platform/thread/EventThread.h>
#include <utils/WaitForCount.h>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ClientThread : public EventThread<ClientThreadCallback>
    {
    public:
        ClientThread(const ClientThread&) = delete;
        ClientThread& operator=(const ClientThread&) = delete;
        ClientThread(ClientThread&& other) = delete;
        ClientThread& operator=(ClientThread&& other) = delete;

        ClientThread(ClientThreadCallback &clientThreadCallback, const std::string &name,
                     EventThreadConfig threadConfig, lu::utils::WaitForCount &syncStart)
            : EventThread<ClientThreadCallback>(clientThreadCallback, name, threadConfig),
              m_clientThreadCallback(clientThreadCallback),
              m_eventChannel(*this, name),
              m_syncStart(syncStart)
        {
        }
        
        virtual ~ClientThread() {}

        bool init()
        {
            if (m_eventChannel.init() == false)
            {
                LOG(ERROR) << "[" << this->m_name << "] Failed to create event channel!";
                return false;
            }

            return EventThread<ClientThreadCallback>::init();
        }

        void start()
        {
            this->m_thread = std::thread(&ClientThread::run, this);
        }

        void run()
        {
            this->m_eventLoop.add(m_eventChannel);
            m_syncStart.increment();
            EventThread<ClientThreadCallback>::run();
        }

        void onNewConnection(lu::platform::socket::BaseSocket *baseSocket)
        {
            auto dataSocket = new DataSocketType(this->m_clientThreadCallback, std::move(*baseSocket));
            // TODO check onNewCOnnection used should be able to send an data
            this->m_clientThreadCallback.onNewConnection(dataSocket);
            dataSocket->getBaseSocket().setNonBlocking();
            this->m_eventLoop.add(*dataSocket);
            delete baseSocket;
            LOG(INFO) << "[" << this->getName() << "] New connection Socket[" << dataSocket << "]";
        }

        const lu::platform::EventChannel<ClientThread>& getEventChannel() const { return m_eventChannel; }
        ClientThreadCallback& getCallback() { return m_clientThreadCallback; }

    private:
        ClientThreadCallback& m_clientThreadCallback;
        lu::platform::EventChannel<ClientThread> m_eventChannel;
        lu::utils::WaitForCount& m_syncStart;
    };
}