#pragma once

#include <common/TemplateConstraints.h>
#include <platform/thread/LuThread.h>
#include <platform/thread/channel/TransferQueue.h>
#include <common/Defs.h>

namespace  lu::platform::thread
{
    template<lu::common::NonPtrClassOrStruct LuThreadCallback>
    class WorkerThread : public LuThread
    {
    public:
        WorkerThread(const std::string& name, LuThreadCallback& luThreadCallback) : LuThread(name),
                                                                                    m_luThreadCallback(luThreadCallback)
        {
        }

        bool init()
        {
            LuThread::init();
            return m_luThreadCallback.onInit();
        }

        void stop()
        {
            LuThread::stop();
            m_inputChannel.push({getChannelID(), nullptr});
        }

        void join()
        {
            LuThread::join();
            m_luThreadCallback.onExit();
        }

        LuThreadCallback& getThreadCallback() { return m_luThreadCallback; }

        virtual ~WorkerThread(){}

    private:
        void run() override final
        {
            m_luThreadCallback.onStart();

            for(;;) 
            {
                channel::ChannelData channelData = m_inputChannel.pop();
                
                if (UNLIKELY(channelData.data == nullptr))
                {
                    break;
                }

                m_luThreadCallback.onMsg(channelData);
            }
        }

        LuThreadCallback& m_luThreadCallback;
    };
} 