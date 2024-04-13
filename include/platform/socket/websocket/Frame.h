#pragma once
#include <crypto/DataWrap.h>

#include <cstdint>
#include <string>
#include <vector>

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
        bool isFinalFrame() const { return m_isFinalFrame; }
        bool isControlFrame() const;
        bool isDataFrame() const { return !isControlFrame(); }
        bool isContinuationFrame() const { return isDataFrame() && (m_opCode == Continue);}
        bool isDone() const { return m_processingState == Done; }
        bool isValid() const { return m_isValid; }
        auto getPayloadLength() const { return m_length; }
        auto getOpCode() const { return m_opCode; }
        void reset();

        static bool isCloseCodeValid(int closeCode);

        static lu::crypto::DataWrap getFrameHeader(OpCode opCode, uint64_t payloadLength, uint32_t maskingKey,
                       bool lastFrame);
        static void mask(char*  payload, uint32_t size, uint32_t maskingKey);

    private:
        ProcessingState readFrameHeader(SocketData &socketData);
        ProcessingState readFramePayloadLength(SocketData &socketData);
        ProcessingState readFrameMask(SocketData &socketData);
        ProcessingState readFramePayload(SocketData& socketData);
        void mask(SocketData& socketData);

        bool checkValidity() ;
        bool hasMask() const { return m_mask != 0U; }
        void setError(CloseCode code, const std::string &closeReason);
        

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
        
    };
}