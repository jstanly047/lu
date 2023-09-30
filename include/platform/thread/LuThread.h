#pragma once
#include <platform/thread/channel/OutputChannel.h>
#include <platform/thread/channel/InputChannel.h>
#include <thread>
#include <string>

namespace  lu::platform::thread
{
    extern thread_local std::string gtlThreadName;

    class LuThread
    {
    public:
        LuThread(const std::string& name);
        LuThread(const LuThread&) = delete;
        LuThread& operator=(const LuThread&) = delete;
        LuThread(LuThread&& other) = delete;
        LuThread& operator=(LuThread&& other) = delete;

        void init();
        void run();
        void join();
        void stop();
        void connect(LuThread& readerThread);
        auto getChannelID() const { return m_outputChannel.getChannelID();}
        const std::string& getName() const { return m_name; }


    protected:
        std::string m_name;
        std::thread m_thread;
        channel::OutputChannel m_outputChannel;
        channel::InputChannel m_inputChannel;
    };
} 