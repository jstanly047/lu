#pragma once
#include <platform/thread/channel/OutputChannel.h>
#include <platform/thread/channel/InputChannel.h>
#include <thread>
#include <string>

namespace  lu::platform::thread
{
    class LuThread
    {
    public:
        LuThread(const std::string& name);
        LuThread(const LuThread&) = delete;
        LuThread& operator=(const LuThread&) = delete;
        LuThread(LuThread&& other) = delete;
        LuThread& operator=(LuThread&& other) = delete;
        virtual ~LuThread() {}

        static const std::string& getCurrentThreadName(); 

        void init();
        void run();
        void join();
        void stop();
        void connect(LuThread& readerThread);
        auto getChannelID() const { return m_channelID;;}
        const std::string& getName() const { return m_name; }
        void transferMsg(const std::string& threadName, void * msg);
        void transferMsg(unsigned int threadIndex, void * msg);
        unsigned int getThreadIndex(const std::string& threadName) const;


    protected:
        std::string m_name;
        std::thread m_thread;
        channel::InputChannel m_inputChannel;
       

    private:
        channel::OutputChannel m_outputChannel;
        channel::ChannelID m_channelID{};
        //static thread_local std::string m_sThreadLocalName;
    };
} 