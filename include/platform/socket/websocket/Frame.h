#pragma once
#include <crypto/DataWrap.h>
#include <common/Defs.h>
#include <utils/Utils.h>

#include <cstdint>
#include <string>
#include <span>
#include <arpa/inet.h>

#include <glog/logging.h>

namespace lu::platform::socket::websocket
{
    struct SocketData
    {
        SocketData(const SocketData&)               = delete;
        SocketData& operator=(const SocketData&)    = delete;
        SocketData(SocketData&& other)              = delete;
        SocketData& operator=(SocketData&& other)   = delete;

        SocketData (uint8_t* d, unsigned int l) : data(d), length(l), readOffset(0U) {}
        uint8_t* data;
        unsigned int length;
        unsigned int readOffset;

        unsigned int getDataLeftToRead() { return length - readOffset; }
    };
    
    class Frame
    {
    public:
        enum ProcessingState
        {
            ReadHeader,
            ReadPayloadLength,
            ReadMask,
            ReadPayload,
            WaitForData,
            Done,
            Error
        };

        enum OpCode
        {
            Continue = 0x0,
            Text = 0x1,
            Binary = 0x2,
            Reserved3 = 0x3,
            Reserved4 = 0x4,
            Reserved5 = 0x5,
            Reserved6 = 0x6,
            Reserved7 = 0x7,
            Close = 0x8,
            Ping = 0x9,
            Pong = 0xA,
            ReservedB = 0xB,
            ReservedC = 0xC,
            ReservedD = 0xD,
            ReservedE = 0xE,
            ReservedF = 0xF
        };

        enum CloseCode
        {
            CloseCodeNormal = 1000,
            CloseCodeGoingAway = 1001,
            CloseCodeProtocolError = 1002,
            CloseCodeDatatypeNotSupported = 1003,
            CloseCodeReserved1004 = 1004,
            CloseCodeMissingStatusCode = 1005,
            CloseCodeAbnormalDisconnection = 1006,
            CloseCodeWrongDatatype = 1007,
            CloseCodePolicyViolated = 1008,
            CloseCodeTooMuchData = 1009,
            CloseCodeMissingExtension = 1010,
            CloseCodeBadOperation = 1011,
            CloseCodeTlsHandshakeFailed = 1015
        };

        Frame(const unsigned int& maxAllowedMessageSize);

        void readFrame(SocketData& socketData);
        bool isControlFrame() const;
        bool isDataFrame() const { return !isControlFrame(); }
        bool isContinuationFrame() const { return isDataFrame() && (m_opCode == Continue);}
        bool isValid() const { return m_isValid; }
        auto getPayloadLength() const { return m_length; }
        auto getOpCode() const { return m_opCode; }
        void reset();

        static bool isCloseCodeValid(int closeCode);

        static lu::crypto::DataWrap getFrameHeader(OpCode opCode, uint64_t payloadLength, uint32_t maskingKey,
                       bool lastFrame);
        static void mask(char*  payload, uint32_t size, uint32_t maskingKey);
        

        template <typename Callback, typename DataSocket>
        ssize_t processMessage(SocketData& socketData, Callback& cb, DataSocket& dataSocket)
        {
            for (;;)
            {
                bool isDone = false;

                while (isDone == false)
                {
                    if (socketData.getDataLeftToRead() == 0U)
                    {
                        return socketData.readOffset;
                    }

                    readFrame(socketData);

                    if (m_processingState != Done)
                    {
                        // waiting for more data available;
                        return socketData.readOffset;
                    }
                    else if (isValid())
                    {
                        if (isControlFrame())
                        {
                            isDone = processControlFrame(socketData, cb, dataSocket);
                        }
                        else
                        {
                            // we have a data frame; opcode can be OC_CONTINUE, OC_TEXT or OC_BINARY
                            if (m_isFragmented == false && isContinuationFrame())
                            {
                                // reset();
                                // m_message.clear();
                                LOG(ERROR) << "Received Continuation frame, while there is nothing to continue.nothing to continue";
                                m_opCode = websocket::Frame::Close;
                                return socketData.readOffset;
                            }
                            if (m_isFragmented && isDataFrame() &&
                                isContinuationFrame() == false)
                            {
                                // reset();
                                // m_message.clear();
                                LOG(ERROR) << "All data frames after the initial data frame must have opcode 0 (continuation)";
                                m_opCode = websocket::Frame::Close;
                                return socketData.readOffset;
                            }
                            if (isContinuationFrame() == false)
                            {
                                m_opCode = getOpCode();
                                m_isFragmented = !m_isFinalFrame;
                            }

                            if (m_isFragmented)
                            {
                                if (append(socketData.data + socketData.readOffset))
                                {
                                    // reset();
                                    // m_message.clear();
                                    LOG(ERROR) << "Received message is too big";
                                    m_opCode = websocket::Frame::Close;
                                    return socketData.readOffset;
                                }
                            }

                            if (m_isFinalFrame)
                            {
                                isDone = true;

                                if (m_isFragmented)
                                {
                                    std::span payload(reinterpret_cast<uint8_t *>(const_cast<unsigned char *>(m_message.getData())), m_length);
                                    cb.onData(dataSocket, payload);
                                    m_message.clear();
                                }
                                else
                                {
                                    std::span payload(socketData.data + socketData.readOffset, m_length);
                                    cb.onData(dataSocket, payload);
                                }
                            }

                            socketData.readOffset += m_length;
                            reset();
                        }
                    }
                    else
                    {
                        // reset();
                        // m_message.clear();
                        // isDone = true;
                        m_opCode = websocket::Frame::Close;
                        return socketData.readOffset;
                    }

                    reset();
                }
            }

            return socketData.readOffset;
        }

        template <typename ControlCallback, typename DataSocket>
        bool processControlFrame(SocketData &socketData, ControlCallback &controlDB, DataSocket& dataSocket)
        {
            bool mustStopProcessing = true; // control frames never expect additional frames to be processed

            switch (m_opCode)
            {
                case websocket::Frame::Ping:
                {
                    std::span payload(socketData.data + socketData.readOffset, m_length);
                    controlDB.onPing(dataSocket, payload);
                    break;
                }
                case websocket::Frame::Pong:
                {
                    controlDB.onPong(dataSocket);
                    break;
                }
                case websocket::Frame::Close:
                {
                    auto closeCode = websocket::Frame::CloseCode::CloseCodeNormal;
                    std::string closeReason;
                    if (m_length == 1U)
                    {
                        // size is either 0 (no close code and no reason)
                        // or >= 2 (at least a close code of 2 bytes)
                        closeCode = websocket::Frame::CloseCode::CloseCodeProtocolError;
                        LOG(ERROR) << "Payload of close frame is too small";
                    }
                    else if (m_length > 1U)
                    {
                        // close frame can have a close code and reason
                        closeCode = static_cast<websocket::Frame::CloseCode>(::ntohs(*(reinterpret_cast<uint16_t *>(socketData.data + socketData.readOffset))));

                        if (!websocket::Frame::isCloseCodeValid(closeCode))
                        {
                            closeCode = websocket::Frame::CloseCode::CloseCodeProtocolError;
                            LOG(ERROR) << "Invalid close code %1 detected";
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

                    controlDB.onClose(dataSocket, closeCode);
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

        template <typename DataSocket>
        static int sendMsg(void *buffer, ssize_t size, int MaxMessageSize, DataSocket& dataSocket, websocket::Frame::OpCode firstOpCode, bool mask)
        {
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

                if (mask)
                {
                    maskingKey = lu::utils::Utils::generateRandomUint32();
                }

                const bool isLastFrame = (i == (numFrames - 1));
                const bool isFirstFrame = (i == 0);

                auto size = std::min(bytesLeft, (ssize_t)MaxMessageSize);

                const auto opcode = isFirstFrame ? firstOpCode
                                                 : websocket::Frame::OpCode::Continue;

                auto frame = websocket::Frame::getFrameHeader(opcode, size, maskingKey, isLastFrame);
                dataSocket.send(frame.getData(), frame.getDataLength());

                if (LIKELY(size > 0))
                {
                    char *currentData = reinterpret_cast<char *>(buffer) + currentPosition;

                    if (mask)
                    {
                        websocket::Frame::mask(currentData, size, maskingKey);
                    }

                    dataSocket.send(currentData, size);
                }

                currentPosition += size;
                bytesLeft -= size;
            }

            return currentPosition;
        }

    private:
        ProcessingState readFrameHeader(SocketData &socketData);
        ProcessingState readFramePayloadLength(SocketData &socketData);
        ProcessingState readFrameMask(SocketData &socketData);
        ProcessingState readFramePayload(SocketData& socketData);
        void mask(SocketData& socketData);

        bool checkValidity() ;
        bool hasMask() const { return m_mask != 0U; }
        void setError(CloseCode code, const std::string &closeReason);
        bool append(const void* data);
        

        const unsigned int& m_maxAllowedMessageSize;
        ProcessingState m_processingState = ProcessingState::ReadHeader;
        bool m_isFinalFrame = true;
        bool m_rsv1 = false;
        bool m_rsv2 = false;
        bool m_rsv3 = false;
        bool m_isValid = false;
        uint32_t m_mask{};
        uint32_t m_length{};
        OpCode m_opCode = ReservedC;
        CloseCode m_closeCode = CloseCodeNormal;
        std::string m_closeReason;
        bool m_isFragmented = false;
        lu::crypto::DataWrap m_message;
    };
}