#pragma once
#include <platform/thread/EventThread.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/data_handler/String.h>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataHandler>
    class ClientThread : public EventThread<ClientThreadCallback>
    {
    public:
        ClientThread(const ClientThread&) = delete;
        ClientThread& operator=(const ClientThread&) = delete;
        ClientThread(ClientThread&& other);
        ClientThread& operator=(ClientThread&& other);
        ClientThread(ClientThreadCallback& clientThreadCallback, const std::string& name, EventThreadConfig threadConfig);
        virtual ~ClientThread() {}

        bool init();
        void start();
        void run();
        void onNewConnection(lu::platform::socket::BaseSocket* baseSocket);

        const lu::platform::EventChannel<ClientThread>& getEventChannel() const { return m_eventChannel; }

    private:
        ClientThreadCallback& m_clientThreadCallback;
        lu::platform::EventChannel<ClientThread> m_eventChannel;
    };
}