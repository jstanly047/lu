#pragma once

#include "BaseSocket.h"
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <string>
#include <platform/defs.h>
#include <utils/Utils.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <string>


namespace lu::platform::socket
{
    template <lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler>
    class DataSocket : public lu::platform::IFDEventHandler
    {
    public:
        DataSocket(const DataSocket &) = delete;
        DataSocket &operator=(const DataSocket &) = delete;
        DataSocket(DataSocket &&other) = delete;
        DataSocket &operator=(DataSocket &&other) = delete;

        DataSocket(DataSocketCallback &dataSocketCallback, BaseSocket &&baseSocket) : m_baseSocket(std::move(baseSocket)),
                                                                                      m_dataSocketCallback(dataSocketCallback),
                                                                                      m_dataHandler(),
                                                                                      m_headerSize(m_dataHandler.getHeaderSize()),
                                                                                      m_receiveBufferShiftSize(m_dataHandler.getReceiveBufferSize() / 4),
                                                                                      m_numberOfBytesLeftToRecv(m_dataHandler.getReceiveBufferSize())

        {
        }

        virtual ~DataSocket() {}

        bool Receive()
        {
            int numberOfBytesRead = 0;

            if (lu::utils::Utils::readDataSocket(m_baseSocket.getFD(), m_dataHandler.getReceiveBufferToFill() + m_readOffset + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
            {
                return false;
            }

            m_numberOfBytesLeftToRead += numberOfBytesRead;
            readMessages();

            if (m_numberOfBytesLeftToRead == 0)
            {
                m_readOffset = 0;
                m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize();
            }
            else if (m_numberOfBytesLeftToRead <= m_receiveBufferShiftSize)
            {
                std::memcpy(m_dataHandler.getReceiveBufferToFill(), m_dataHandler.getReceiveBufferToFill() + m_readOffset, m_numberOfBytesLeftToRead);
                m_readOffset = 0;
                m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize() - m_numberOfBytesLeftToRead;
                assert(m_numberOfBytesLeftToRecv > 0u);
            }

            return true;
        }

        int sendMsg(void *buffer, ssize_t size)
        {
            if (m_baseSocket.getFD() == nullptr)
            {
                return false;
            }

            ssize_t totalSent = 0;
            auto uint8_tBuffer = reinterpret_cast<uint8_t *>(buffer);

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

        int sendFile(int fileDescriptor, int size)
        {
            off_t offset = 0;
            ssize_t sendBytes = ::sendfile(m_baseSocket.getFD(), fileDescriptor, &offset, size);

            if (sendBytes == -1)
            {
                return false;
            }

            int totalSent = 0;

            while (totalSent < size)
            {
                ssize_t numBytesSend = ::sendfile(m_baseSocket.getFD(), fileDescriptor, &offset, size);

                if (numBytesSend < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    return totalSent;
                }
                else if (numBytesSend != (ssize_t)size)
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

            return sendBytes;
        }

        BaseSocket &getBaseSocket() { return m_baseSocket; }
        const std::string &getIP() const { return m_baseSocket.getIP(); }
        int getPort() const { return m_baseSocket.getPort(); }

    private:
        inline void readMessages()
        {
            unsigned int expectedMsgSize = 0;

            for (;;)
            {
                if (m_numberOfBytesLeftToRead >= m_headerSize)
                {
                    // TODO the reader function must handle the alignment and endianess
                    expectedMsgSize = m_dataHandler.readHeader(m_readOffset);
                    // updateForDataRead(m_headerSize);
                }
                else
                {
                    return;
                }

                if (m_numberOfBytesLeftToRead + m_headerSize >= expectedMsgSize)
                {
                    m_dataSocketCallback.onData(*this, m_dataHandler.readMessage(m_readOffset, expectedMsgSize));
                    updateForDataRead(expectedMsgSize);
                    continue;
                }

                return;
            }
        }

        inline void updateForDataRead(std::size_t size)
        {
            m_readOffset += size;
            m_numberOfBytesLeftToRead -= size;
        }

        void onEvent(struct ::epoll_event &event) override final
        {
            if ((event.events & EPOLLHUP || event.events & EPOLLERR))
            {
                m_dataSocketCallback.onClientClose(*this);
                return;
            }
            else if (!(event.events & EPOLLIN))
            {
                return;
            }

            Receive();
        }

        const lu::platform::FileDescriptor &getFD() const override final { return m_baseSocket.getFD(); }

    protected:
        BaseSocket m_baseSocket;
        DataSocketCallback &m_dataSocketCallback;
        DataHandler m_dataHandler;
        std::size_t m_headerSize{};
        std::size_t m_receiveBufferShiftSize{};
        std::size_t m_numberOfBytesInBuffer{};
        std::size_t m_readOffset{};
        std::size_t m_numberOfBytesLeftToRead{};
        std::size_t m_numberOfBytesLeftToRecv{};
    };
}
