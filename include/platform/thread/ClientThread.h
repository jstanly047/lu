#pragma once

#include <platform/thread/EventThread.h>
#include <utils/WaitForCount.h>
#include <list>

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

            m_eventNotifier.reset(m_eventChannel.getEventNotifier());
            return EventThread<ClientThreadCallback>::init();
        }

        void onNewConnection(lu::platform::socket::BaseSocket *baseSocket)
        {
            std::unique_ptr<DataSocketType> dataSocket(new DataSocketType(this->m_clientThreadCallback, std::move(*baseSocket)));
            LOG(INFO) << "[" << this->getName() << "] New connection Socket[" << dataSocket.get() << "] FD[" << (int) dataSocket->getBaseSocket().getFD() << "] "
                        << " IP[" << baseSocket->getIP() << ":" << baseSocket->getPort()  << "]";
            // TODO check onNewCOnnection used should be able to send an data
            this->m_clientThreadCallback.onNewConnection(*dataSocket.get());
            //dataSocket->getBaseSocket().setNonBlocking();
            this->addToEventLoop(std::move(dataSocket));
            delete baseSocket;
        }

        void onAppMsg(void* msg, lu::platform::thread::channel::ChannelID channelID)
        {
            this->m_clientThreadCallback.onAppMsg(msg, channelID);
        }

        const lu::platform::EventNotifier& getEventNotifier() const { return *m_eventNotifier; }
        ClientThreadCallback& getCallback() { return m_clientThreadCallback; }

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

            this->addToEventLoop(m_eventChannel);
            m_syncStart.increment();
            EventThread<ClientThreadCallback>::run();
        }

        ClientThreadCallback& m_clientThreadCallback;
        lu::platform::EventChannel<ClientThread> m_eventChannel;
        lu::utils::WaitForCount& m_syncStart;
        std::unique_ptr<lu::platform::EventNotifier> m_eventNotifier;
        std::list<lu::platform::EventChannel<ClientThread>> m_eventChannelForConnectingThreads;
    };
}