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
    class IClientThreadCallback
    {
    public:
        virtual bool onInit() = 0;
        virtual void onStart() = 0;
        virtual void onExit() = 0;
        virtual void onNewConnection(lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>* dataSocket) = 0;
        virtual void onTimer(const lu::platform::FDTimer<IClientThreadCallback>&) = 0;
        virtual void onClientClose(lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&) = 0;
        virtual void onData(lu::platform::socket::DataSocket<IClientThreadCallback, lu::platform::socket::data_handler::String>&, void* ) = 0;
    };

    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataHandler>
    class ClientThread
    {
    public:
        ClientThread(const ClientThread&) = delete;
        ClientThread& operator=(const ClientThread&) = delete;
        ClientThread(ClientThread&& other);
        ClientThread& operator=(ClientThread&& other);
        
        bool init();
        void start();
        void run();
        void stop();
        void join();

        const std::string& getName() const { return m_name; }
        ClientThreadCallback& getClientThreadCallback() { return m_severClientThreadCallback; }

    protected:
        ClientThread(ClientThreadCallback& severClientThreadCallback, const std::string& name, ClientThreadConfig serverClientThreadConfig);
        virtual ~ClientThread() {}

        std::string m_name;
        ClientThreadCallback& m_severClientThreadCallback;
        ClientThreadConfig m_clientThreadConfig;
        lu::platform::FDEventLoop m_eventLoop;
        lu::platform::FDTimer<ClientThreadCallback> m_timer;
        std::thread m_thread;
    };
}