#include <platform/socket/websocket/Frame.h>
#include <common/Defs.h>
#include <arpa/inet.h>
#include <limits>
#include <cstring>
#include <glog/logging.h>

using namespace lu::platform::socket::websocket;

# if __BYTE_ORDER == __BIG_ENDIAN
# define ntohll(x)	__uint64_identity (x)
# define htonll(x)	__uint64_identity (x)
# else
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#   define ntohll(x)	__bswap_64 (x)
#   define htonll(x)	__bswap_64 (x)
#  endif
# endif


namespace
{
    inline bool isOpCodeReserved(Frame::OpCode code)
    {
        return ((code > Frame::OpCode::Binary) && (code < Frame::OpCode::Close)) || (code > Frame::OpCode::Pong);
    }
}

Frame::Frame(const unsigned int& maxAllowedMessageSize) : m_maxAllowedMessageSize(maxAllowedMessageSize), m_message(maxAllowedMessageSize)
{
}

void Frame::readFrame(SocketData &socketData)
{
    while (true)
    {
        switch (m_processingState)
        {
        case ReadHeader:
            m_processingState = readFrameHeader(socketData);
            if (m_processingState == WaitForData)
            {
                m_processingState = ReadHeader;
                return;
            }
            break;

        case ReadPayloadLength:
            m_processingState = readFramePayloadLength(socketData);
            if (m_processingState == WaitForData)
            {
                m_processingState = ReadPayloadLength;
                return;
            }
            break;

        case ReadMask:
            m_processingState = readFrameMask(socketData);
            if (m_processingState == WaitForData)
            {
                m_processingState = ReadMask;
                return;
            }
            break;

        case ReadPayload:
            m_processingState = readFramePayload(socketData);
            if (m_processingState == WaitForData)
            {
                m_processingState = ReadPayload;
                return;
            }
            break;

        case Done:
            return;

        default:
            break;
        }
    }
}

Frame::ProcessingState Frame::readFrameHeader(SocketData& socketData)
{
    if (socketData.getDataLeftToRead() >= 2)
    {
        
        // FIN, RSV1-3, Opcode
        m_isFinalFrame = (socketData.data[socketData.readOffset] & 0x80) != 0;
        m_rsv1 = (socketData.data[socketData.readOffset] & 0x40);
        m_rsv2 = (socketData.data[socketData.readOffset] & 0x20);
        m_rsv3 = (socketData.data[socketData.readOffset] & 0x10);
        m_opCode = static_cast<OpCode>(socketData.data[socketData.readOffset] & 0x0F);

        // Mask
        // Use zero as mask value to mean there's no mask to read.
        // When the mask value is read, it over-writes this non-zero value.
        m_mask = socketData.data[socketData.readOffset + 1] & 0x80;
        // PayloadLength
        m_length = (socketData.data[socketData.readOffset + 1] & 0x7F);

        socketData.readOffset += 2U;

        if (!checkValidity())
            return Done;

        switch (m_length)
        {
        case 126:
        case 127:
            return ReadPayloadLength;
        default:
            return hasMask() ? ReadMask : ReadPayload;
        }
    }

    return WaitForData;
}

bool Frame::checkValidity() 
{
    if (m_rsv1 || m_rsv2 || m_rsv3) 
    {
        setError(CloseCode::CloseCodeProtocolError, "Rsv field is non-zero");
        LOG(ERROR) << "Rsv field is non-zero";
    }
    else if (isOpCodeReserved(m_opCode))
    {
        setError(CloseCode::CloseCodeProtocolError, "Used reserved opcode");
        LOG(ERROR) << "Used reserved opcode";
    }
    else if (isControlFrame()) 
    {
        if (m_length > 125)
        {
            setError(CloseCodeProtocolError,
                     "Control frame is larger than 125 bytes");
            LOG(ERROR) << "Control frame is larger than 125 bytes";
        }
        else if (!m_isFinalFrame)
        {
            setError(CloseCodeProtocolError,
                     "Control frames cannot be fragmented");
            LOG(ERROR) << "Control frames cannot be fragmented";
        }
        else
        {
            m_isValid = true;
        }
    }
    else
    {
        m_isValid = true;
    }

    return m_isValid;
}

Frame::ProcessingState Frame::readFramePayloadLength(SocketData &socketData)
{
    switch (m_length)
    {
    case 126:
        if ((socketData.getDataLeftToRead()) >= 2U)
        {
            short len;
            std::memcpy(&len, socketData.data + socketData.readOffset, sizeof(len));
            m_length = ::ntohs(len);
            socketData.readOffset += 2U;

            if (m_length < 126)
            {
                setError(CloseCodeProtocolError, "Lengths smaller than 126 must be expressed as one byte.");
                LOG(ERROR) << "Lengths smaller than 126 must be expressed as one byte.";
                return Done;
            }

            return hasMask() ? ReadMask : ReadPayload;
        }
        break;
    case 127:
        if (socketData.getDataLeftToRead() >= 8U)
        {
            // Most significant bit must be set to 0 as
            // per https://tools.ietf.org/html/rfc6455#section-5.2
            uint64_t length;
            std::memcpy(&length, socketData.data + socketData.readOffset, sizeof(length));
            length = ::ntohll(length);
            m_length = static_cast<uint32_t>(length);
            socketData.readOffset += 8U;

            if (length > std::numeric_limits<uint32_t>::max() || m_length & 0b1000'0000'0000'0000)
            {
                setError(CloseCodeProtocolError, "Highest bit of payload length is not 0.");
                LOG(ERROR) << "Highest bit of payload length is not 0.";
                return Done;
            }
            if (m_length <= 0xFFFFu)
            {
                setError(CloseCodeProtocolError, "Lengths smaller than 65536 (2^16) must be expressed as 2 bytes.");
                LOG(ERROR) << "Lengths smaller than 65536 (2^16) must be expressed as 2 bytes.";
                return Done;
            }
            return hasMask() ? ReadMask : ReadPayload;
        }
        break;
    default:
        break;
    }
    return WaitForData;
}

Frame::ProcessingState Frame::readFrameMask(SocketData& socketData)
{
    if (socketData.getDataLeftToRead() >= 4U)
    {
        std::memcpy(&m_mask, socketData.data + socketData.readOffset, sizeof(m_mask));
        m_mask = ::ntohl(m_mask);
        socketData.readOffset += 4U;
        return ReadPayload;
    }
    return WaitForData;
}

Frame::ProcessingState Frame::readFramePayload(SocketData& socketData)
{
    if (m_length == 0U)
    {
        return Done;
    }

    if (m_length > m_maxAllowedMessageSize)
    {
        setError(CloseCodeTooMuchData, "Maximum framesize exceeded.");
        LOG(ERROR) << "Maximum framesize exceeded.";
        return Done;
    }

    if (socketData.getDataLeftToRead() >= m_length)
    {
        // m_length can be safely cast to an integer,
        // because MAX_FRAME_SIZE_IN_BYTES = MAX_INT
        if (hasMask())
        {
            mask(socketData);
        }

        return Done;
    }

    return WaitForData;
}

void Frame::mask(SocketData& socketData)
{
    mask(reinterpret_cast<char*>(socketData.data + socketData.readOffset), m_length, m_mask);
}

void Frame::mask(char* payload, uint32_t size, uint32_t maskingKey)
{
    const uint8_t mask[] = { uint8_t((maskingKey & 0xFF000000u) >> 24),
                            uint8_t((maskingKey & 0x00FF0000u) >> 16),
                            uint8_t((maskingKey & 0x0000FF00u) >> 8),
                            uint8_t((maskingKey & 0x000000FFu))
                          };
    uint64_t i = 0;
    while (size-- > 0)
    {
        *payload++ ^= mask[i++ % 4];
    }
}

void Frame::setError(CloseCode code, const std::string &closeReason)
{
    reset();
    m_closeCode = code;
    m_closeReason = closeReason;
    m_isValid = false;
}

void Frame::reset()
{
    m_closeCode = CloseCode::CloseCodeNormal;
    m_closeReason.clear();
    m_isFinalFrame = true;
    m_mask = 0;
    m_rsv1 = false;
    m_rsv2 = false;
    m_rsv3 = false;
    m_opCode = ReservedC;
    m_isFragmented = false;
    m_length = 0;
    //m_payload.clear();
    m_isValid = false;
    m_processingState = ReadHeader;
}

bool Frame::isControlFrame() const
{
    return (m_opCode & 0x08) == 0x08;
}

bool Frame::isCloseCodeValid(int closeCode)
{
    return (closeCode > 999) && (closeCode < 5000) &&
           (closeCode != Frame::CloseCode::CloseCodeReserved1004) && // see RFC6455 7.4.1
           (closeCode != Frame::CloseCode::CloseCodeMissingStatusCode) &&
           (closeCode != Frame::CloseCode::CloseCodeAbnormalDisconnection) &&
           ((closeCode >= 3000) || (closeCode < 1012));
}

lu::crypto::DataWrap Frame::getFrameHeader(OpCode opCode,
                                             uint64_t payloadLength, uint32_t maskingKey,
                                             bool lastFrame)
{
    lu::crypto::DataWrap header(14);
    bool ok = payloadLength <= 0x7FFFFFFFFFFFFFFFULL;

    if (UNLIKELY(ok == false)) {
        return header;
    }

    // FIN, RSV1-3, opcode (RSV-1, RSV-2 and RSV-3 are zero)
    auto byte = static_cast<uint8_t>((opCode & 0x0F) | (lastFrame ? 0x80 : 0x00));
    header.append(&byte, 1U);

    byte = 0x00;
    if (maskingKey != 0)
        byte |= 0x80;
    if (payloadLength <= 125)
    {
        byte |= static_cast<uint8_t>(payloadLength);
        header.append(&byte, 1U);
    }
    else if (payloadLength <= 0xFFFFU)
    {
        byte |= 126;
        header.append(&byte, 1U);
        auto payLoadLength16bit = static_cast<uint16_t>(payloadLength);
        payLoadLength16bit = htons(payLoadLength16bit);
        header.append(&payLoadLength16bit, sizeof(payLoadLength16bit));
    }
    else if (payloadLength <= 0x7FFFFFFFFFFFFFFFULL)
    {
        byte |= 127;
        header.append(&byte, 1);
        payloadLength = htonll(payloadLength);
        header.append(&payloadLength, sizeof(payloadLength));
    }

    if (maskingKey != 0)
    {
        maskingKey = htonl(maskingKey);
        header.append(&maskingKey, sizeof(maskingKey));
    }

    return header;
}

bool Frame::append(const void* data)
{
    if (m_length + m_message.getDataLength() > m_message.getCapacity())
    {
        return false;
    }

    m_message.append(data, m_length);
    return true;
}