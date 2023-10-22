#pragma once

#include <common/TemplateConstraints.h>
#include <platform/thread/LuThread.h>
#include <platform/thread/channel/TransferQueue.h>

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
            m_inputChannel.getTransferQueue().push({getChannelID(), nullptr});
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
                channel::ChannelData channelData = m_inputChannel.getTransferQueue().pop();
                m_luThreadCallback.onMsg(channelData);

                if (UNLIKELY(channelData.data == nullptr))
                {
                    break;
                }
            }
        }

        LuThreadCallback& m_luThreadCallback;
    };
} 