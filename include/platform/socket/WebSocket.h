#pragma once

#include <platform/socket/BaseSocket.h>
#include <platform/socket/websocket/HandshakeRequest.h>
#include <platform/socket/websocket/HandshakeResponse.h>
#include <platform/socket/websocket/Frame.h>
#include <common/TemplateConstraints.h>
#include <common/Defs.h>
#include <platform/IFDEventHandler.h>
#include <platform/Defs.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>

#include <glog/logging.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ClientThread;

    template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ConnectionThread;

    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
    class ServerSingleThread;

    template <lu::common::NonPtrClassOrStruct DataSocketCallback, unsigned int MaxMessageSize, typename CustomObjectPtrType=void>
    class WebSocket : public lu::platform::IFDEventHandler
    {
        template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ClientThread;

        template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ConnectionThread;

        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ServerSingleThread;
        
        enum struct SocketStatus
        {
            Connecting,
            Connected,
            Closing
        };

    public:
        // both constants are taken from the default settings of Apache
        // see: http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfieldsize and
        // http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfields
        static constexpr auto MAX_HEADER_LINE_LENGTH = 8 * 1024U; // maximum length of a http request header line
        static constexpr auto MAX_HEADER_LINES = 100U; 
        static constexpr auto END_OF_HEADER_MARKER = "\r\n\r\n";
        static constexpr auto RESOURCE = "/comms";
        static constexpr auto BUFFER_SIZE = MAX_HEADER_LINE_LENGTH * MAX_HEADER_LINES + std::char_traits<char>::length(END_OF_HEADER_MARKER);

        WebSocket(const WebSocket &) = delete;
        WebSocket &operator=(const WebSocket &) = delete;
        WebSocket(WebSocket &&other) = delete;
        WebSocket &operator=(WebSocket &&other) = delete;

        WebSocket(DataSocketCallback &dataSocketCallback, BaseSocket &&baseSocket) : IFDEventHandler(IFDEventHandler::DataSocket),
                                                                                      m_baseSocket(std::move(baseSocket)),
                                                                                      m_dataSocketCallback(dataSocketCallback),
                                                                                      m_socketStatus(SocketStatus::Connecting),
                                                                                      m_frame(MaxMessageSize),
                                                                                      m_receiveBufferShiftSize(BUFFER_SIZE / 4),
                                                                                      m_numberOfBytesLeftToRecv(BUFFER_SIZE)

        {
        }

        virtual ~WebSocket() {}

        bool Receive()
        {
            for(;;)
            {
                ssize_t numberOfBytesRead = 0;

                if (m_baseSocket.readDataSocket(m_buffer.data() + m_readOffset + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
                {
                    return false;
                }

                if (numberOfBytesRead == 0)
                {
                    break;
                }

                m_numberOfBytesLeftToRead += numberOfBytesRead;
                readMessages();

                if (m_numberOfBytesLeftToRead == 0)
                {
                    m_readOffset = 0;
                    m_numberOfBytesLeftToRecv = m_buffer.size();
                }
                else if (m_numberOfBytesLeftToRead <= m_receiveBufferShiftSize)
                {
                    std::memcpy(m_buffer.data(), m_buffer.data() + m_readOffset, m_numberOfBytesLeftToRead);
                    m_readOffset = 0;
                    m_numberOfBytesLeftToRecv = m_buffer.size() - m_numberOfBytesLeftToRead;
                    assert(m_numberOfBytesLeftToRecv > 0u);
                }
            }

            return true;
        }

        int sendMsg(void *buffer, ssize_t size, bool isBinary = true)
        {
            return websocket::Frame::sendMsg(buffer, size, MaxMessageSize, *this, isBinary, m_mustMask);
        }

        int send(void *buffer, ssize_t size)
        {
            return m_baseSocket.send(buffer, size);
        }

        BaseSocket &getBaseSocket() { return m_baseSocket; }
        const std::string &getIP() const { return m_baseSocket.getIP(); }
        int getPort() const { return m_baseSocket.getPort(); }

        int close()
        {
            int retVal = m_baseSocket.close();

            if (retVal = 0)
            {
                delete this;
            }

            return retVal;
        }

        int stop(ShutSide shutSide) { return m_baseSocket.stop(shutSide); }
        void setCustomObjectPtr(CustomObjectPtrType * ptr) { m_customObjectPtr = ptr; }
        CustomObjectPtrType* getCustomObjectPtr() const { return m_customObjectPtr; }
        bool isWebSocket() const { return true;  }

    private:
        inline void readMessages()
        {
            if (m_socketStatus == SocketStatus::Connecting)
            {
                processHandshake();

                if (m_socketStatus == SocketStatus::Connecting)
                {
                    return;
                }
            }

            websocket::SocketData socketData(m_buffer.data() + m_readOffset, m_numberOfBytesLeftToRead);
            updateForDataRead(m_frame.processMessage(socketData, m_dataSocketCallback, *this));

            if (m_frame.getOpCode() == websocket::Frame::Close)
            {
                this->stop(ShutdownReadWrite);
            }
        }

        inline void processHandshake()
        {
            std::string_view dataStringView(reinterpret_cast<const char*>(m_buffer.data()) + m_readWebsocketHeaderOffset, m_numberOfBytesLeftToRead - m_readWebsocketHeaderOffset);

            for (;;)
            {
                auto location = dataStringView.find("\r\n", m_readWebsocketHeaderOffset);

                if (location == std::string::npos)
                {
                    break;
                }

                if (location == m_readWebsocketHeaderOffset) // End of header
                {
                    m_readWebsocketHeaderOffset += 2U;
                    websocket::HandshakeRequest handshakeRequest(m_baseSocket);
                    std::string_view dataStringView(reinterpret_cast<const char*>(m_buffer.data()), m_readWebsocketHeaderOffset);
                    static std::vector<int> supportedVersion = {13};
                    auto response  = handshakeRequest.getResponse(dataStringView, RESOURCE, "LuBinary", supportedVersion);

                    if (response.empty())
                    {
                        LOG(ERROR) << "Invalid request [" << m_baseSocket.getIP() << "]"; 
                        this->stop(ShutdownReadWrite);
                        return;
                    }

                    updateForDataRead(m_readWebsocketHeaderOffset);
                    m_readWebsocketHeaderOffset = 0U;
                    this->send(response.data(), response.size());
                    m_socketStatus = SocketStatus::Connected;
                    m_dataSocketCallback.onOpen(*this);
                    return;
                }

                m_readWebsocketHeaderOffset = location + 2U;
            }

            if (m_numberOfBytesLeftToRecv == 0)
            {
                LOG(ERROR) << "Too large header [" << m_baseSocket.getIP() << "]";
                this->stop(ShutdownReadWrite);
            }
        }

        inline void updateForDataRead(ssize_t size)
        {
            m_readOffset += size;
            m_numberOfBytesLeftToRead -= size;
        }

        bool onEvent(struct ::epoll_event &event) override final
        {
            if ((event.events & EPOLLHUP || event.events & EPOLLERR))
            {
                m_dataSocketCallback.onClientClose(*this);
                return false;
            }
            else if (!(event.events & EPOLLIN))
            {
                return true;
            }

            if (Receive() == false)
            {
                // TODO : May be different callback since this some IO read error
                LOG(ERROR) << "Read failed for data socket FD[" << (int) m_baseSocket.getFD() << "]!";
                m_dataSocketCallback.onClientClose(*this);
                return false;
            }

            return true;
        }

        const lu::platform::FileDescriptor &getFD() const override final { return m_baseSocket.getFD(); }

    protected:
        BaseSocket m_baseSocket;
        DataSocketCallback &m_dataSocketCallback;
        ssize_t m_receiveBufferShiftSize{};
        ssize_t m_numberOfBytesInBuffer{};
        ssize_t m_readOffset{};
        ssize_t m_numberOfBytesLeftToRead{};
        ssize_t m_numberOfBytesLeftToRecv{};
        std::array<uint8_t, BUFFER_SIZE> m_buffer{};
        SocketStatus m_socketStatus;
        websocket::Frame m_frame;
        unsigned int m_readWebsocketHeaderOffset{};
        bool m_mustMask = false;
        CustomObjectPtrType* m_customObjectPtr{};
    };
}
