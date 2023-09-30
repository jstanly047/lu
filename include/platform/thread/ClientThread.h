#pragma once

#include <platform/thread/EventThread.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>
#include <glog/logging.h>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataHandler>
    class ClientThread : public EventThread<ClientThreadCallback>
    {
    public:
        ClientThread(const ClientThread&) = delete;
        ClientThread& operator=(const ClientThread&) = delete;
        ClientThread(ClientThread&& other) = delete;
        ClientThread& operator=(ClientThread&& other) = delete;

        ClientThread(ClientThreadCallback &clientThreadCallback, const std::string &name, 
                      EventThreadConfig threadConfig) : EventThread<ClientThreadCallback>(clientThreadCallback, name, threadConfig),
                                                        m_clientThreadCallback(clientThreadCallback),
                                                        m_eventChannel(*this)
        {
        }
        
        virtual ~ClientThread() {}

        bool init()
        {
            if (m_eventChannel.init() == false)
            {
                LOG(ERROR) << "Thead[" << this->m_name << "] failed to create event channel!";
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
            EventThread<ClientThreadCallback>::run();
        }

        void onNewConnection(lu::platform::socket::BaseSocket *baseSocket)
        {
            auto dataSocket = new lu::platform::socket::DataSocket<ClientThreadCallback, DataHandler>(this->m_clientThreadCallback, std::move(*baseSocket));
            this->m_clientThreadCallback.onNewConnection(dataSocket);
            dataSocket->getBaseSocket().setNonBlocking();
            this->m_eventLoop.add(*dataSocket);
            delete baseSocket;
        }

        const lu::platform::EventChannel<ClientThread>& getEventChannel() const { return m_eventChannel; }
        ClientThreadCallback& getCallback() { return m_clientThreadCallback; }

    private:
        ClientThreadCallback& m_clientThreadCallback;
        lu::platform::EventChannel<ClientThread> m_eventChannel;
    };
}