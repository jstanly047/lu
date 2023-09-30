#pragma once

#include <common/TemplateConstraints.h>
#include <platform/thread/LuThread.h>

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

        void start()
        {
            this->m_thread = std::thread(&WorkerThread::run, this);
        }

        void run()
        {
            LuThread::run();
            m_luThreadCallback.onStart();

            while (m_stop == false)
            {
                 m_luThreadCallback.onMsg(m_inputChannel.getTransferQueue().pop());
            };
        }

        void stop()
        {
            LuThread::stop();
            m_stop = true;
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

        LuThreadCallback& m_luThreadCallback;
        bool m_stop = false;
    };
} 