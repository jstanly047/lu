#pragma once
#include <platform/thread/ServerConfig.h>
#include <platform/FDEventLoop.h>
#include <platform/EventChannel.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IDataSocketCallback.h>
#include <platform/FDTimer.h>
#include <platform/ITimerCallback.h>

#include <unordered_map>
#include <thread>

namespace lu::platform::thread
{
    class IServerClientThreadCallback
    {
    public:
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        virtual void onExit() = 0;
        virtual void onNewConnection(lu::platform::socket::DataSocket<IServerClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket) = 0;
        virtual void onTimer(const lu::platform::FDTimer<IServerClientThreadCallback>&) = 0;
        virtual void onClientClose(lu::platform::socket::DataSocket<IServerClientThreadCallback, lu::platform::socket::data_handler::String>&) = 0;
        virtual void onData(lu::platform::socket::DataSocket<IServerClientThreadCallback, lu::platform::socket::data_handler::String>&, void* ) = 0;
    };

    template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback,  lu::common::NonPtrClassOrStruct DataHandler>
    class ServerClientThread
    {
    public:
        ServerClientThread(const ServerClientThread&) = delete;
        ServerClientThread& operator=(const ServerClientThread&) = delete;
        ServerClientThread(ServerClientThread&& other);
        ServerClientThread& operator=(ServerClientThread&& other);
        ServerClientThread(ServerClientThreadCallback& severClientThreadCallback, const std::string& name, SeverClientThreadConfig serverClientThreadConfig);
        ~ServerClientThread() {}

        bool init();
        void start();
        void run();
        void stop();
        void onNewConnection(lu::platform::socket::BaseSocket* baseSocket);
        void join();

        const lu::platform::EventChannel<ServerClientThread>& getEventChannel() const { return m_eventChannel; }
        const std::string& getName() const { return m_name; }
        ServerClientThreadCallback& getServerClientThreadCallback() { return m_severClientThreadCallback; }

    private:
        std::string m_name;
        ServerClientThreadCallback& m_severClientThreadCallback;
        SeverClientThreadConfig m_serverThreadClientConfig;
        lu::platform::FDEventLoop m_eventLoop;
        lu::platform::FDTimer<ServerClientThreadCallback> m_timer;
        lu::platform::EventChannel<ServerClientThread> m_eventChannel;
        std::thread m_thread; 
    };
}