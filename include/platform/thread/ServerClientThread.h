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
        class IServerClientThreadCallback : public lu::platform::socket::IDataSocketCallback, public lu::platform::ITimerCallback
        {
        public:
            virtual bool onInit() { return true; }
            virtual void onStart() {}
            virtual void onExit() {}
            virtual void onNewConnection([[maybe_unused]] const lu::platform::socket::DataSocket<lu::platform::thread::IServerClientThreadCallback, lu::platform::socket::IDataHandler>* baseSocket) {}
            virtual void onTimer([[maybe_unused]] const lu::platform::FDTimer<ITimerCallback>& ) override {}
        };

        template<lu::common::NonPtrClassOrStruct ServerClientThreadCallback,  template<typename> class DataHandler>
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
            std::string m_name;
            ServerClientThreadCallback m_severClientThreadCallback;
            SeverClientThreadConfig m_serverThreadClientConfig;
            lu::platform::FDEventLoop m_eventLoop;
            lu::platform::FDTimer<ServerClientThreadCallback> m_timer;
            lu::platform::EventChannel<ServerClientThread> m_eventChannel;
            std::thread m_thread;
            
        };
}