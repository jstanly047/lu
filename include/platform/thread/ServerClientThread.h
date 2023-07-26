#pragma once
#include <platform/thread/ClientThread.h>

namespace lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback,  lu::common::NonPtrClassOrStruct DataHandler>
    class ServerClientThread : public ClientThread<ServerClientThreadCallback, DataHandler>
    {
    public:
        ServerClientThread(const ServerClientThread&) = delete;
        ServerClientThread& operator=(const ServerClientThread&) = delete;
        ServerClientThread(ServerClientThread&& other);
        ServerClientThread& operator=(ServerClientThread&& other);
        ServerClientThread(ServerClientThreadCallback& severClientThreadCallback, const std::string& name, ClientThreadConfig serverClientThreadConfig);
        virtual ~ServerClientThread() {}

        bool init();
        void start();
        void run();
        void onNewConnection(lu::platform::socket::BaseSocket* baseSocket);

        const lu::platform::EventChannel<ServerClientThread>& getEventChannel() const { return m_eventChannel; }

    private:
        lu::platform::EventChannel<ServerClientThread> m_eventChannel;
    };
}