#pragma once
#include <platform/thread/IWorkerThreadCallback.h>
#include <gmock/gmock.h>

namespace lu::platform::thread
{
    class MockWorkerThread : public IWorkerThreadCallback
    {
    public:
        MockWorkerThread() {}
        MockWorkerThread(MockWorkerThread&&) {}
        MockWorkerThread& operator=(MockWorkerThread&&) { return *this; }

        
        MOCK_METHOD(void, onMsg, (channel::ChannelData), (override));
        MOCK_METHOD(bool, onInit, (), (override));
        MOCK_METHOD(void, onStart, (), (override));
        MOCK_METHOD(void, onExit, (), (override));
    };
}
