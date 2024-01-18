#pragma once
#include <platform/thread/channel/OutputChannel.h>
#include <platform/thread/channel/InputChannel.h>
#include <common/TemplateConstraints.h>
#include <platform/EventChannel.h>

#include <unordered_map>
#include <thread>
#include <string>


namespace  lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ClientThread;

    template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ConnectionThread;

    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
    class ServerSingleThread;

    class LuThread
    {
        template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ClientThread;

        template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ConnectionThread;

        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ServerSingleThread;

    public:
        LuThread(const std::string& name);
        LuThread(const LuThread&) = delete;
        LuThread& operator=(const LuThread&) = delete;
        LuThread(LuThread&& other) = delete;
        LuThread& operator=(LuThread&& other) = delete;
        virtual ~LuThread() {}

        static const std::string& getCurrentThreadName();
        static void transferMsg(const std::string& threadName, void * msg);
        static void transferMsg(unsigned int threadIndex, void * msg);
        void thisTransferMsg(unsigned int threadIndex, void * msg);
        static void transferMsgToIOThread(channel::ChannelID channelID, void *msg);
        static channel::ChannelID getCurrentThreadChannelID();
        static unsigned int getThreadIndex(const std::string& threadName);

        void init();
        void start();
        void join();
        void stop();
        void connect(LuThread& readerThread);
        
        const std::string& getName() const { return m_name; }
        auto getChannelID() const { return m_channelID; }
        //unsigned int getThreadIndex(const std::string& threadName) const;
        virtual void run() = 0;


    protected:
        std::string m_name;
        std::thread m_thread;
        channel::InputChannel m_inputChannel;

        void threadRun();
       
    private:
        void connectTo(channel::ChannelID channelID, lu::platform::EventNotifier* eventNotifier);  

        channel::OutputChannel m_outputChannel;
        channel::ChannelID m_channelID{};
        std::unordered_map<channel::ChannelID, std::unique_ptr<lu::platform::EventNotifier>> m_eventNotifiers;
        //static thread_local std::string m_sThreadLocalName;
    };
} 