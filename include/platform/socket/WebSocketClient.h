#pragma once

#include <platform/socket/BaseSocket.h>
#include <platform/socket/websocket/HandshakeRequest.h>
#include <platform/socket/websocket/HandshakeSeverResponse.h>
#include <platform/socket/websocket/Frame.h>
#include <common/TemplateConstraints.h>
#include <common/Defs.h>
#include <platform/IFDEventHandler.h>
#include <platform/Defs.h>
#include <utils/Utils.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <map>
#include <span>
#include <random>

#include <glog/logging.h>

namespace lu::platform::socket
{
    template <lu::common::NonPtrClassOrStruct DataSocketCallback, unsigned int MaxMessageSize, typename CustomObjectPtrType=void>
    class WebSocket : public lu::platform::IFDEventHandler
    {
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
            m_message.reserve(MaxMessageSize);
        }

        virtual ~WebSocket() {}

        void startHandshake(const websocket::InitialRequestInfo& initialRequestInfo)
        {
            m_key = websocket::HandshakeRequest::generateKey();
            auto request = websocket::HandshakeRequest::createHandShakeRequest(initialRequestInfo, m_key);
            send(request.data(), request.size());
        }

        bool Receive()
        {
            for(;;)
            {
                ssize_t numberOfBytesRead = 0;

                if (lu::utils::Utils::readDataSocket(m_baseSocket.getFD(), m_buffer.data() + m_readOffset + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
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
            auto firstOpCode = isBinary ?
                websocket::Frame::OpCode::Binary : websocket::Frame::OpCode::Text;

            int numFrames = size / MaxMessageSize;
            auto sizeLeft = size % MaxMessageSize;

            if (LIKELY(sizeLeft))
            {
                ++numFrames;
            }

            if (UNLIKELY(numFrames == 0))
            {
                numFrames = 1;
            }

            ssize_t currentPosition = 0;
            ssize_t bytesLeft = size;

            for (int i = 0; i < numFrames; i++)
            {
                uint32_t maskingKey = 0;

                if (m_mustMask)
                {
                    maskingKey = generateRandomUint32();
                }

                const bool isLastFrame = (i == (numFrames - 1));
                const bool isFirstFrame = (i == 0);

                auto size = std::min(bytesLeft, (ssize_t) MaxMessageSize);

                const auto opcode = isFirstFrame ? firstOpCode
                                                 : websocket::Frame::OpCode::Continue;

                 auto frame = websocket::Frame::getFrameHeader(opcode, size, maskingKey, isLastFrame);
                 send(frame.getData(), frame.getDataLength());

                 if (LIKELY(size > 0))
                 {
                    char * currentData = reinterpret_cast<char*>(buffer) + currentPosition;

                    if (m_mustMask)
                    {
                        websocket::Frame::mask(currentData, size, maskingKey);
                    }

                    send(currentData, size);
                 }

                 currentPosition += size;
                 bytesLeft -= size;
            }

            return currentPosition;
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

    private:
        inline void readMessages()
        {
            if (m_socketStatus == SocketStatus::Connecting)
            {
                processServerReply();

                if (m_socketStatus == SocketStatus::Connecting)
                {
                    return;
                }
            }

            updateForDataRead(processMessage());
        }

        inline void processServerReply()
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
                    websocket::HandshakeSeverResponse handshakeSeverResponse(m_baseSocket);

                    if (handshakeSeverResponse.readServerResponse(reinterpret_cast<const char*>(m_buffer.data()), m_readWebsocketHeaderOffset, m_key) == false)
                    {
                        this->stop(ShutdownReadWrite);
                        return;
                    }

                    updateForDataRead(m_readWebsocketHeaderOffset);
                    m_readWebsocketHeaderOffset = 0U;
                    m_socketStatus = SocketStatus::Connected;
                    m_dataSocketCallback.onOpen((this));
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

        ssize_t processMessage()
        {
            bool isDone = false;
            websocket::SocketData socketData(m_buffer.data() + m_readOffset, m_numberOfBytesLeftToRead);

            while (isDone == false)
            {
                m_frame.readFrame(socketData);

                if (m_frame.isDone() == false)
                {
                    // waiting for more data available;
                    return socketData.readOffset;
                }
                else if (m_frame.isValid())
                {
                    if (m_frame.isControlFrame())
                    {
                        isDone = processControlFrame(socketData);
                    }
                    else
                    {
                        // we have a data frame; opcode can be OC_CONTINUE, OC_TEXT or OC_BINARY
                        if (m_isFragmented == false && m_frame.isContinuationFrame())
                        {
                            reset();
                            LOG(ERROR) << "Received Continuation frame, while there is nothing to continue.nothing to continue[" << m_baseSocket.getIP() << "]";
                            return socketData.readOffset;
                        }
                        if (m_isFragmented && m_frame.isDataFrame() &&
                                       m_frame.isContinuationFrame() == false)
                        {
                            reset();
                            LOG(ERROR) << "All data frames after the initial data frame must have opcode 0 (continuation)[" << m_baseSocket.getIP() << "]";
                            return socketData.readOffset;
                        }
                        if (m_frame.isContinuationFrame() == false)
                        {
                            m_opCode = m_frame.getOpCode();
                            m_isFragmented = !m_frame.isFinalFrame();
                        }

                        bool isFinalFrame = m_frame.isFinalFrame();

                        if (m_isFragmented)
                        {
                            if ((m_message.size() + m_frame.getPayloadLength()) >
                                MaxMessageSize)
                            {
                                reset();
                                LOG(ERROR) << "Received message is too big [" << m_baseSocket.getIP() << "]";
                                return socketData.readOffset;
                            }

                            std::memcpy(m_message.data() + m_message.size(), socketData.data + socketData.readOffset, m_frame.getPayloadLength());
                        }

                        if (isFinalFrame)
                        {
                            isDone = true;

                            if (m_isFragmented)
                            {
                                std::span payload(m_message.data(), m_message.size());
                                m_dataSocketCallback.onData(*this, payload);
                                m_message.clear();
                            }
                            else
                            {
                                std::span payload(socketData.data + socketData.readOffset, m_frame.getPayloadLength());
                                m_dataSocketCallback.onData(*this, payload);
                            }
                        }
                        
                        socketData.readOffset += m_frame.getPayloadLength();
                        m_frame.reset();
                    }
                }
                else
                {
                    reset();
                    isDone = true;
                    this->stop(ShutdownWrite);
                }

                m_frame.reset();
            }

            return socketData.readOffset;
        }

        bool processControlFrame(websocket::SocketData& socketData)
        {
            bool mustStopProcessing = true; // control frames never expect additional frames to be processed
            switch (m_frame.getOpCode())
            {
            case websocket::Frame::Ping:
            {
                m_dataSocketCallback.onPing(*this);
                break;
            }
            case websocket::Frame::Pong:
            {
                m_dataSocketCallback.onPong(*this);
                break;
            }
            case websocket::Frame::Close:
            {
                auto closeCode = websocket::Frame::CloseCode::CloseCodeNormal;
                std::string closeReason;
                if (m_frame.getPayloadLength() == 1U)
                {
                    // size is either 0 (no close code and no reason)
                    // or >= 2 (at least a close code of 2 bytes)
                    closeCode = websocket::Frame::CloseCode::CloseCodeProtocolError;
                    LOG(ERROR) << "Payload of close frame is too small [" << m_baseSocket.getIP() << "]";
                }
                else if (m_frame.getPayloadLength() > 1U)
                {
                    // close frame can have a close code and reason
                    closeCode = static_cast<websocket::Frame::CloseCode>(::ntohs(*(reinterpret_cast<uint16_t *>(socketData.data + socketData.readOffset))));

                    if (!websocket::Frame::isCloseCodeValid(closeCode))
                    {
                        closeCode = websocket::Frame::CloseCode::CloseCodeProtocolError;
                        LOG(ERROR) << "Invalid close code %1 detected [" << m_baseSocket.getIP() << "]";
                    }
                    else
                    {
                        /*if (m_frame.getPayloadLength() > 2U)
                        {
                            auto toUtf16 = QStringDecoder(QStringDecoder::Utf8,
                                                          QStringDecoder::Flag::Stateless | QStringDecoder::Flag::ConvertInvalidToNull);
                            closeReason = toUtf16(QByteArrayView(payload).sliced(2));
                            if (toUtf16.hasError())
                            {
                                closeCode = websocket::Frame::CloseCode::CloseCodeWrongDatatype;
                                LOG(ERROR) << "Invalid UTF-8 code encountered [" << m_baseSocket.getIP() << "]";
                            }
                        }*/
                    }
                }

                m_dataSocketCallback.onClose(*this, closeCode);
                break;
            }

            case websocket::Frame::Continue:
            case websocket::Frame::Binary:
            case websocket::Frame::Text:
            case websocket::Frame::Reserved3:
            case websocket::Frame::Reserved4:
            case websocket::Frame::Reserved5:
            case websocket::Frame::Reserved6:
            case websocket::Frame::Reserved7:
            case websocket::Frame::ReservedC:
            case websocket::Frame::ReservedB:
            case websocket::Frame::ReservedD:
            case websocket::Frame::ReservedE:
            case websocket::Frame::ReservedF:
                break;

            default:
                break;
            }
            return mustStopProcessing;
        }

        void reset()
        {
            m_isFinalFrame = false;
            m_isFragmented = false;
            m_opCode = websocket::Frame::OpCode::Close;
            m_message.clear();
            m_frame.reset();
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

        int send(const void *buffer, ssize_t size)
        {
            if (m_baseSocket.getFD() == nullptr)
            {
                return false;
            }

            ssize_t totalSent = 0;
            auto uint8_tBuffer = reinterpret_cast<const uint8_t *>(buffer);

            while (totalSent < size)
            {
                // TODO we can try replace this by ::writev (scatter/gather IO)
                ssize_t numBytesSend = ::send(m_baseSocket.getFD(), uint8_tBuffer + totalSent, size, MSG_DONTWAIT);

                if (numBytesSend < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    return totalSent;
                }
                else if (numBytesSend != size)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOBUFS)
                    {
                        totalSent += numBytesSend;
                        size -= numBytesSend;
                        continue;
                    }

                    return totalSent;
                }

                totalSent += numBytesSend;
            }

            return totalSent;
        }

        uint32_t generateRandomUint32()
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dis;
            return dis(gen);
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
        CustomObjectPtrType* m_customObjectPtr{};
        std::array<uint8_t, BUFFER_SIZE> m_buffer;
        //std::array<uint8_t, BUFFER_SIZE> m_textMessage;
        std::vector<uint8_t> m_message;
        SocketStatus m_socketStatus;
        websocket::Frame m_frame;
        unsigned int m_readWebsocketHeaderOffset{};
        bool m_isFinalFrame = false;
        bool m_isFragmented = false;
        websocket::Frame::OpCode m_opCode = websocket::Frame::OpCode::Close;
        bool m_isControlFrame = false;
        bool m_mustMask = true;
        std::string m_key;
    };
}
