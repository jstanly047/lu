#pragma once
#include <platform/FDEventLoop.h>
#include <platform/EventChannel.h>
#include <platform/socket/DataSocket.h>
#include <platform/socket/IDataHandler.h>
#include <platform/socket/IDataSocketCallback.h>

#include <unordered_map>
#include <thread>

namespace lu::platform::thread
{
        template<lu::common::NonPtrClassOrStruct DataHandler>
        class IServerClientThreadCallback : public lu::platform::socket::IDataSocketCallback<DataHandler>
        {
        public:
            virtual bool onInit() { return true; }
            virtual void onStart() {}
            virtual void onExit() {}
            virtual void onNewConnection(const lu::platform::socket::DataSocket<lu::platform::thread::IServerClientThreadCallback<DataHandler>, DataHandler>* baseSocket) {}
        };


        /*
        class IDataHandler
        {
        public:
            virtual bool onInit() { return true; }
            virtual void onStart() {}
            virtual void onExit() {}
            virtual void onNewConnection(const lu::platform::socket::BaseSocket<DataHandler>& baseSocket) {}
        };*/

        struct SeverClientThreadConfig
        {
            int NUMBER_OF_EVENTS_PER_HANDLE = 10;
        };

        template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
        class ServerClientThread
        {
        public:
            ServerClientThread(const ServerClientThread&) = delete;
            ServerClientThread& operator=(const ServerClientThread&) = delete;
            ServerClientThread(ServerClientThread&& other);
            ServerClientThread& operator=(ServerClientThread&& other);
            ServerClientThread(const std::string& name, SeverClientThreadConfig serverClientThreadConfig);
            ~ServerClientThread() {}

            bool init();
            void start();
            void run();
            void onNewConnection(lu::platform::socket::BaseSocket* baseSocket);
            void join();

            const lu::platform::EventChannel<ServerClientThread>& getEventChannel() const { return m_eventChannel; }
            const std::string& getName() const { return m_name; }
            ServerClientThreadCallback& getServerClientThreadCallback() { return m_severClientThreadCallback; }

        private:
            ServerClientThreadCallback m_severClientThreadCallback;
            SeverClientThreadConfig m_serverThreadClientConfig;
            lu::platform::FDEventLoop m_eventLoop;
            lu::platform::EventChannel<ServerClientThread> m_eventChannel;
            std::thread m_thread;
            std::string m_name;
        };
}

template class lu::platform::thread::ServerClientThread<lu::platform::thread::IServerClientThreadCallback<lu::platform::socket::IDataHandler>, 
                                                        lu::platform::socket::IDataHandler>;