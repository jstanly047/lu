#pragma once
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <memory>

namespace lu::platform
{
    

    struct EventData
    {
        enum EventType
        {
            None,
            NewConnection
        };

        EventType eventType;
        void* data;

        EventData(EventType type = EventType::None, void* ptr = nullptr) : eventType(type), data(ptr) {}
    };

    template<lu::common::NonPtrClassOrStruct EventChannelHandler>
    class EventChannel : public IFDEventHandler
    {  
    public:
        EventChannel(const EventChannel&) = delete;
        EventChannel& operator=(const EventChannel&) = delete;
        EventChannel(EventChannel&& other) = delete;
        EventChannel& operator=(EventChannel&& other)  = delete;

        EventChannel(EventChannelHandler& eventChannelHandler);
        ~EventChannel() {}

        bool init();
        bool notify(const EventData& eventData) const;
        const FileDescriptor& getInFD() const;
        
    private:
        void onEvent(struct ::epoll_event& event) override final;
        const FileDescriptor& getFD() const override final;

    private:
        EventChannelHandler &m_eventChannelHandler;
        std::unique_ptr<FileDescriptor> m_in;
        std::unique_ptr<FileDescriptor> m_out;
    };
}