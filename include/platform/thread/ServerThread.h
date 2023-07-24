#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ServerClientThread.h>
#include <platform/FDEventLoop.h>

#include <thread>
#include <vector>

namespace lu::platform::thread
{
        class IServerThreadCallback
        {
        public:
            virtual void onStart() {}
            virtual void onExit() {}
            virtual void onNewConnection(lu::platform::socket::BaseSocket&& baseSocket) {}
        };

        struct SeverConfig
        {
            bool CREATE_NEW_THREAD = false;
            bool ACT_AS_CLIENT_HANDLER = false;
            unsigned int NUMBER_OF_CLIENT_HANDLE_THREADS = 2u;
            unsigned int NUMBER_OF_CONNECTION_IN_WAITING_QUEUE = 50u;
            unsigned int NUMBER_OF_EVENTS_PER_HANDLE = 10u;
            unsigned int CLIENT_HANDLER_NUMBER_OF_EVENTS_PER_HANDLE = NUMBER_OF_EVENTS_PER_HANDLE;
        };

        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, lu::common::NonPtrClassOrStruct DataHandler>
        class ServerThread
        {
        public:
            ServerThread(ServerThread&& other) = delete;
            ServerThread& operator=(ServerThread&& other) = delete;
            ServerThread(const ServerThread&) = delete;
            ServerThread& operator=(const ServerThread&) = delete;

            ServerThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, SeverConfig serverConfig = SeverConfig{}, bool reuseAddAndPort = true);
            ~ServerThread() {}
            bool init();
            void start();
            void run();
            void onNewConnection(lu::platform::socket::BaseSocket* baseSocket);
            void join();

            const std::string& getName() const { return m_name; }

        private:
            lu::platform::socket::ServerSocket<ServerThread> m_serverSocket;
            ServerThreadCallback& m_serverThreadCallback;
            const SeverConfig m_serverConfig;
            lu::platform::FDEventLoop m_eventLoop;
            std::thread m_thread;
            std::vector<ServerClientThread<SeverClientThreadCallback, DataHandler>> m_serverClientThreads;
            std::string m_name;
            unsigned int m_currentClientHandler{};
        };
}

