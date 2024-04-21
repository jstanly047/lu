#pragma once

#include <platform/socket/BaseSocket.h>
#include <platform/socket/websocket/HandshakeRequest.h>
#include <platform/socket/websocket/HandshakeResponse.h>
#include <platform/socket/websocket/Frame.h>
#include <common/TemplateConstraints.h>
#include <common/Defs.h>
#include <platform/IFDEventHandler.h>
#include <platform/Defs.h>
#include <platform/socket/Defs.h>

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

    template <lu::common::NonPtrClassOrStruct DataSocketCallback, unsigned int MaxMessageSize, lu::common::NonPtrClassOrStruct DataHandler, lu::common::NonPtrClassOrStruct SocketType=BaseSocket, typename CustomObjectPtrType=void>
    class HybridDataSocket : public lu::platform::IFDEventHandler
    {
        template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ClientThread;

        template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ConnectionThread;

        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ServerSingleThread;
        
        enum struct SocketStatus : char
        {
            Connecting = 'C',
            ConnectedAsWebSocket = 'T',
            ConnectedAsTCP = 'W',
            Closing ='X'
        };

    public:
        // both constants are taken from the default settings of Apache
        // see: http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfieldsize and
        // http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfields
        static constexpr auto MAX_HEADER_LINE_LENGTH = 8 * 1024U; // maximum length of a http request header line
        static constexpr auto MAX_HEADER_LINES = 100U; 
        static constexpr auto RESOURCE = "/comms";
        static constexpr auto BUFFER_SIZE = std::max((const unsigned int) ((float)MaxMessageSize * 1.25F), RECV_BUFFER_SIZE);

        HybridDataSocket(const HybridDataSocket &) = delete;
        HybridDataSocket &operator=(const HybridDataSocket &) = delete;
        HybridDataSocket(HybridDataSocket &&other) = delete;
        HybridDataSocket &operator=(HybridDataSocket &&other) = delete;

        HybridDataSocket(DataSocketCallback &dataSocketCallback, SocketType &&baseSocket) : IFDEventHandler(IFDEventHandler::DataSocket),
                                                                                      m_socket(std::move(baseSocket)),
                                                                                      m_dataSocketCallback(dataSocketCallback),
                                                                                      m_dataHandler(m_buffer.data()),
                                                                                      m_socketStatus(SocketStatus::Connecting),
                                                                                      m_frame(MaxMessageSize),
                                                                                      m_receiveBufferShiftSize(BUFFER_SIZE / 4),
                                                                                      m_numberOfBytesLeftToRecv(BUFFER_SIZE)

        {
        }

        virtual ~HybridDataSocket() {}

        bool Receive()
        {
            for(;;)
            {
                ssize_t numberOfBytesRead = 0;

                if (m_socket.readDataSocket(m_buffer.data() + m_readOffset + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
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
            if (m_socketStatus == SocketStatus::ConnectedAsTCP)
            {
                send(buffer, size);
            }
            else if (m_socketStatus == SocketStatus::ConnectedAsWebSocket)
            {
                return websocket::Frame::sendMsg(buffer, size, MaxMessageSize, *this, isBinary, m_mustMask);
            }

            return 0;
        }

        int close()
        {
            int retVal = m_socket.close();

            if (retVal = 0)
            {
                delete this;
            }

            return retVal;
        }

        int send(void *buffer, ssize_t size)
        {
            return m_socket.send(buffer, size);
        }

        BaseSocket &getBaseSocket() { return m_socket; }
        SocketType &getSocket() { return m_socket; }
        const std::string &getIP() const { return m_socket.getIP(); }
        int getPort() const { return m_socket.getPort(); }
        int stop(ShutSide shutSide) { return m_socket.stop(shutSide); }
        void setCustomObjectPtr(CustomObjectPtrType * ptr) { m_customObjectPtr = ptr; }
        CustomObjectPtrType* getCustomObjectPtr() const { return m_customObjectPtr; }

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

            if (m_socketStatus == SocketStatus::ConnectedAsTCP)
            {
                readRawTCPMessages();
            }
            else if (m_socketStatus == SocketStatus::ConnectedAsWebSocket)
            {
                websocket::SocketData socketData(m_buffer.data() + m_readOffset, m_numberOfBytesLeftToRead);
                updateForDataRead(m_frame.processMessage(socketData, m_dataSocketCallback, *this));

                if (m_frame.getOpCode() == websocket::Frame::Close)
                {
                    this->stop(ShutdownReadWrite);
                }
            }
            else
            {
                LOG(ERROR) << "Unknown protocol " << (char) m_socketStatus << " [" <<  m_socket.getIP() << "]";
                this->stop(ShutdownReadWrite);
            }
        }

        inline void readRawTCPMessages()
        {
            unsigned int expectedMsgSize = 0;

            for (;;)
            {
                if (m_numberOfBytesLeftToRead >= m_dataHandler.getHeaderSize())
                {
                    // TODO the reader function must handle the alignment and endianess
                    expectedMsgSize = m_dataHandler.readHeader(m_readOffset);
                    // updateForDataRead(m_headerSize);

                    if (expectedMsgSize == 0U)
                    {
                        LOG(ERROR) << "Invalid message size 0 in [" <<  m_socket.getIP() << "]";
                        this->stop(ShutdownReadWrite);
                    }
                }
                else
                {
                    return;
                }

                if (m_numberOfBytesLeftToRead >= expectedMsgSize)
                {
                    m_dataSocketCallback.onData(*this, m_dataHandler.readMessage(m_readOffset, expectedMsgSize));
                    updateForDataRead(expectedMsgSize);
                    continue;
                }

                return;
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
                    websocket::HandshakeRequest handshakeRequest(m_socket);
                    std::string_view dataStringView(reinterpret_cast<const char*>(m_buffer.data()), m_readWebsocketHeaderOffset);
                    static std::vector<int> supportedVersion = {13};
                    auto response  = handshakeRequest.getResponse(dataStringView, RESOURCE, "LuBinary", supportedVersion);

                    if (response.empty())
                    {
                        LOG(ERROR) << "Invalid request [" << m_socket.getIP() << "]"; 
                        this->stop(ShutdownReadWrite);
                        return;
                    }

                    if (handshakeRequest.getUpgrade() == "tcp")
                    {
                        m_socketStatus = SocketStatus::ConnectedAsTCP;
                    }
                    else
                    {
                        m_socketStatus = SocketStatus::ConnectedAsWebSocket;
                    }
                    
                    updateForDataRead(m_readWebsocketHeaderOffset);
                    m_readWebsocketHeaderOffset = 0U;
                    this->send(response.data(), response.size());
                    m_dataSocketCallback.onOpen(*this);
                    return;
                }

                m_readWebsocketHeaderOffset = location + 2U;
            }

            if (m_numberOfBytesLeftToRecv == 0)
            {
                LOG(ERROR) << "Too large header [" << m_socket.getIP() << "]";
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
                LOG(ERROR) << "Read failed for data socket FD[" << (int) m_socket.getFD() << "]!";
                m_dataSocketCallback.onClientClose(*this);
                return false;
            }

            return true;
        }

        const lu::platform::FileDescriptor &getFD() const override final { return m_socket.getFD(); }

    protected:
        SocketType m_socket;
        DataSocketCallback &m_dataSocketCallback;
        ssize_t m_receiveBufferShiftSize{};
        ssize_t m_numberOfBytesInBuffer{};
        ssize_t m_readOffset{};
        ssize_t m_numberOfBytesLeftToRead{};
        ssize_t m_numberOfBytesLeftToRecv{};
        std::array<uint8_t, BUFFER_SIZE> m_buffer{};
        DataHandler m_dataHandler;
        SocketStatus m_socketStatus;
        websocket::Frame m_frame;
        unsigned int m_readWebsocketHeaderOffset{};
        bool m_mustMask = false;
        CustomObjectPtrType* m_customObjectPtr{};
    };
}
