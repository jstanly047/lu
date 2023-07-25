#pragma once
#include <platform/socket/ServerSocket.h>
#include <platform/thread/ServerClientThread.h>
#include <platform/thread/ServerConfig.h>
#include <platform/FDEventLoop.h>
#include <platform/FDTimer.h>
#include <platform/ITimerCallback.h>

#include <thread>
#include <vector>


namespace lu::platform::thread
{
        class IServerThreadCallback : public lu::platform::socket::IDataSocketCallback, public lu::platform::ITimerCallback
        {
        public:
            virtual void onStart() {}
            virtual void onStartComplete() {}
            virtual void onExit() {}
            virtual void onNewConnection([[maybe_unused]] lu::platform::socket::BaseSocket&& baseSocket) {}
            virtual void onTimer([[maybe_unused]] const lu::platform::FDTimer<ITimerCallback>& ) override {}
        };

        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct SeverClientThreadCallback, template<typename> class DataHandler>
        class ServerThread
        {
        public:
            ServerThread(ServerThread&& other) = delete;
            ServerThread& operator=(ServerThread&& other) = delete;
            ServerThread(const ServerThread&) = delete;
            ServerThread& operator=(const ServerThread&) = delete;

            ServerThread(const std::string& name, ServerThreadCallback& serverThreadCallback, const std::string& service, 
                        SeverConfig serverConfig = SeverConfig{}, bool reuseAddAndPort = true);
            ~ServerThread() {}
            bool init();
            void start();
            void run();
            void onNewConnection(lu::platform::socket::BaseSocket* baseSocket);
            void join();

            const auto& getName() const { return m_name; }
            const auto& getSeverClientThreads() const { return m_serverClientThreads; }

        private:
            std::string m_name;
            ServerThreadCallback& m_serverThreadCallback;
            lu::platform::FDEventLoop m_eventLoop;
            lu::platform::socket::ServerSocket<ServerThread> m_serverSocket;
            lu::platform::FDTimer<ServerThreadCallback> m_timer;
            unsigned int m_currentClientHandler{};
            std::vector<ServerClientThread<SeverClientThreadCallback, DataHandler> > m_serverClientThreads;
            const SeverConfig m_serverConfig;
            std::thread m_thread;
        };
}

