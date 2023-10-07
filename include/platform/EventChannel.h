#pragma once
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <platform/socket/BaseSocket.h>
#include <utils/Utils.h>
#include <memory>
#include <glog/logging.h>

#include <unistd.h>
#include <sys/epoll.h>
#include <cassert>

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
        void *data;

        EventData(EventType type = EventType::None, void *ptr = nullptr) : eventType(type), data(ptr) {}
    };

    constexpr std::size_t BUFFER_SIZE = sizeof(EventData) * 200;
    constexpr std::size_t READ_BUFFER_SHIFT_SIZE = sizeof(EventData) * 10;

    template <lu::common::NonPtrClassOrStruct EventChannelHandler>
    class EventChannel : public IFDEventHandler
    {
        
    public:
        EventChannel(const EventChannel &) = delete;
        EventChannel &operator=(const EventChannel &) = delete;
        EventChannel(EventChannel &&other) = delete;
        EventChannel &operator=(EventChannel &&other) = delete;

        EventChannel(EventChannelHandler &eventChannelHandler, const std::string &name) : m_eventChannelHandler(eventChannelHandler),
                                                                                          m_name(name)
        {
        }

        ~EventChannel() {}

        bool init()
        {
            int pipeFDs[2];
            if (::pipe(pipeFDs) == -1)
            {
                return false;
            }

            m_in.reset(new FileDescriptor(pipeFDs[0]));
            m_out.reset(new FileDescriptor(pipeFDs[1]));
            m_in->setToNonBlocking();
            m_out->setToNonBlocking();
            LOG(INFO) << "[" << m_name << "] File created channel out[" << (int)*m_in << "], out[" << (int)*m_out << "]";
            return true;
        }

        bool notify(const EventData &eventData) const
        {
            return ::write(*m_out, &eventData, sizeof(eventData));
        }

        const FileDescriptor &getInFD() const
        {
            return getFD();
        }

    private:
        inline void readMessages()
        {
            for (;;)
            {
                if (m_numberOfBytesLeftToRead >=  sizeof(EventData))
                {
                    // TODO make it non blocking
                    EventData eventData;
                    std::memcpy(&eventData, (void*) (m_buffer + m_readOffset), sizeof(EventData));
                    m_readOffset += sizeof(EventData);
                    m_numberOfBytesLeftToRead -= sizeof(EventData);

                    switch (eventData.eventType)
                    {
                    case EventData::NewConnection:
                        m_eventChannelHandler.onNewConnection(reinterpret_cast<lu::platform::socket::BaseSocket *>(eventData.data));
                        break;
                    default:
                        break;
                    }

                    continue;
                }

                return;
            }
        }

        bool Receive()
        {
            ssize_t numberOfBytesRead = 0;

            if (lu::utils::Utils::readDataFile(*m_in, m_buffer + m_readOffset + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
            {
                return false;
            }

            m_numberOfBytesLeftToRead += numberOfBytesRead;
            readMessages();

            if (m_numberOfBytesLeftToRead == 0)
            {
                m_readOffset = 0;
                m_numberOfBytesLeftToRecv = BUFFER_SIZE;
            }
            else if (m_numberOfBytesLeftToRead <= READ_BUFFER_SHIFT_SIZE)
            {
                std::memcpy(m_buffer, m_buffer + m_readOffset, m_numberOfBytesLeftToRead);
                m_readOffset = 0;
                m_numberOfBytesLeftToRecv = BUFFER_SIZE - m_numberOfBytesLeftToRead;
                assert(m_numberOfBytesLeftToRecv > 0u);
            }

            return true;
        }

        void onEvent(struct ::epoll_event &event) override final
        {
            if ((event.events & EPOLLERR))
            {
                m_in.reset(nullptr);
                return;
            }
            else if (!(event.events & EPOLLIN))
            {
                return;
            }

            Receive();
        }

        const FileDescriptor &getFD() const override final
        {
            static lu::platform::FileDescriptor nullFileDescriptor(nullptr);
            if (m_in == nullptr)
            {
                return nullFileDescriptor;
            }

            return *m_in;
        }

    private:
        EventChannelHandler &m_eventChannelHandler;
        std::unique_ptr<FileDescriptor> m_in;
        std::unique_ptr<FileDescriptor> m_out;
        std::string m_name;
        uint8_t m_buffer[BUFFER_SIZE];
        std::size_t m_numberOfBytesInBuffer{};
        std::size_t m_readOffset{};
        std::size_t m_numberOfBytesLeftToRead{};
        std::size_t m_numberOfBytesLeftToRecv = BUFFER_SIZE;
    };
}