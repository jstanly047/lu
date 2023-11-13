#pragma once

#include <platform/thread/EventThread.h>
#include <utils/WaitForCount.h>

#include <list>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct ServerDataSocketType, 
            lu::common::NonPtrClassOrStruct ServerClientThreadCallback, 
            lu::common::NonPtrClassOrStruct ServerBaseClientThreadCallback>
    class ServerThread;

    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ClientThread : public EventThread<ClientThreadCallback>
    {
        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct ServerDataSocketType, 
            lu::common::NonPtrClassOrStruct ServerClientThreadCallback, 
            lu::common::NonPtrClassOrStruct ServerBaseClientThreadCallback>
        friend class ServerThread;

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

            m_eventNotifier.reset(m_eventChannel.getEventNotifier());
            return EventThread<ClientThreadCallback>::init();
        }

        void onNewConnection(lu::platform::socket::BaseSocket *baseSocket)
        {
            auto dataSocket = new DataSocketType(this->m_clientThreadCallback, std::move(*baseSocket));
            // TODO check onNewCOnnection used should be able to send an data
            this->m_clientThreadCallback.onNewConnection(dataSocket);
            //dataSocket->getBaseSocket().setNonBlocking();
            this->addToEventLoop(*dataSocket);
            delete baseSocket;
            LOG(INFO) << "[" << this->getName() << "] New connection Socket[" << dataSocket << "]";
        }

        void onAppMsg(void* msg)
        {
            this->m_clientThreadCallback.onAppMsg(msg);
        }

        const lu::platform::EventNotifier& getEventNotifier() const { return *m_eventNotifier; }
        ClientThreadCallback& getCallback() { return m_clientThreadCallback; }

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

            this->addToEventLoop(m_eventChannel);
            m_syncStart.increment();
            EventThread<ClientThreadCallback>::run();
        }

        void addConnectingThread(LuThread& thread)
        {
            m_threadsConnecting.push_back(&thread);
        }

        ClientThreadCallback& m_clientThreadCallback;
        lu::platform::EventChannel<ClientThread> m_eventChannel;
        lu::utils::WaitForCount& m_syncStart;
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ClientThread>> m_eventChannelForConnectingThreads;
        std::vector<LuThread*> m_threadsConnecting;
    };
}